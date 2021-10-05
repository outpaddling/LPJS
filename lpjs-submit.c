/***************************************************************************
 *  Description:
 *      Submit a job to lpjs_dispatchd.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
#include <munge.h>
#include "node-list.h"
#include "config.h"
#include "network.h"
#include "misc.h"
#include "lpjs.h"

int     main (int argc, char *argv[])
{
    int     msg_fd;
    ssize_t bytes;
    char    buff[LPJS_MSG_MAX+1],
	    cmd[LPJS_CMD_MAX + 150],
	    remote_cmd[LPJS_CMD_MAX + 1] = "",
	    host[128],
	    *cred;
    unsigned cores;
    node_list_t node_list;
    munge_err_t munge_status;
    
    if (argc < 2)
    {
	fprintf (stderr, "Usage: %s command [args]\n", argv[0]);
	return EX_USAGE;
    }

    // Get hostname of head node
    lpjs_load_config(&node_list, LPJS_CONFIG_HEAD_ONLY, stderr);

    if ( (msg_fd = connect_to_dispatch(&node_list)) == -1 )
    {
	perror("lpjs-submit: Failed to connect to dispatch");
	return EX_IOERR;
    }

    if ( send_msg(msg_fd, "submit") < 0 )
    {
	perror("lpjs-submit: Failed to send submit request to dispatch");
	close(msg_fd);
	return EX_IOERR;
    }
    
    if ( (munge_status = munge_encode(&cred, NULL, NULL, 0)) != EMUNGE_SUCCESS )
    {
	fputs("lpjs-submit: munge_encode() failed.\n", stderr);
	fprintf(stderr, "Return code = %s\n", munge_strerror(munge_status));
	return EX_UNAVAILABLE; // FIXME: Check actual error
    }

    if ( send_msg(msg_fd, cred) < 0 )
    {
	perror("lpjs-submit: Failed to send credential to dispatch");
	close(msg_fd);
	free(cred);
	return EX_IOERR;
    }
    free(cred);
    
    if ( (bytes = read(msg_fd, buff, LPJS_MSG_MAX+1)) == -1 )
    {
	perror("lpjs-submit: Failed to read response from dispatch");
	close(msg_fd);
	return EX_IOERR;
    }
    buff[bytes] = '\0';
    puts(buff);
    
    /*
     *  Temporarily run 1 process interactively via ssh to test
     *  remote execution under lpjs-chaperone
     */
    
    sscanf(buff, "%s %u", host, &cores);
    str_argv_cat(remote_cmd, argv, LPJS_CMD_MAX + 1);
    puts(remote_cmd);
    snprintf(cmd, LPJS_CMD_MAX + 150, "ssh %s lpjs-chaperone %s\n",
	     host, remote_cmd);
    puts(cmd);
    system(cmd);
    close (msg_fd);

    return EX_OK;
}
