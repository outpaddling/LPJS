/***************************************************************************
 *  Description:
 *      LPJS compute node daemon.  Checks in with lpfs-dispatchd to signal
 *      that node is up and starts computational processes on compute nodes.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-30  Jason Bacon Begin
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

// Must be global for signal handler
FILE    *Log_stream;

int     main (int argc, char *argv[])
{
    int         msg_fd,
		status;
    node_list_t node_list;
    node_t      node;
    char        cmd[LPJS_CMD_MAX + 1] = "",
		buff[LPJS_MSG_MAX + 1],
		*cred;
    munge_err_t munge_status;
    ssize_t     bytes;

    if ( argc > 2 )
    {
	fprintf (stderr, "Usage: %s [--daemonize]\n", argv[0]);
	return EX_USAGE;
    }
    else if ( (argc == 2) && (strcmp(argv[1],"--daemonize") == 0 ) )
    {
	/*
	 *  Code run after this must not attempt to write to stdout or stderr
	 *  since they will be closed.  Use lpjs_log() for all informative
	 *  messages.
	 */
	Log_stream = fopen("/var/log/lpjs_compd", "a");
	if ( Log_stream == NULL )
	{
	    perror("Cannot open /var/log/lpjs_compd");
	    return EX_CANTCREAT;
	}
	daemon(0, 0);
    }
    else
	Log_stream = stderr;
    
    // Get hostname of head node
    lpjs_load_config(&node_list, LPJS_CONFIG_HEAD_ONLY, Log_stream);

    if ( (msg_fd = connect_to_dispatch(&node_list)) == -1 )
    {
	perror("lpjs-nodes: Failed to connect to dispatch");
	return EX_IOERR;
    }

    /* Send a message to the server */
    /* Need to send \0, so dprintf() doesn't work here */
    argv_to_cmd(cmd, argv, LPJS_CMD_MAX + 1);
    status = system(cmd);
    if ( send_msg(msg_fd, "compd-checkin") < 0 )
    {
	perror("lpjs-nodes: Failed to send message to dispatch");
	close(msg_fd);
	return EX_IOERR;
    }

    if ( (munge_status = munge_encode(&cred, NULL, NULL, 0)) != EMUNGE_SUCCESS )
    {
	fputs("lpjs-submit: munge_encode() failed.\n", Log_stream);
	fprintf(Log_stream, "Return code = %s\n", munge_strerror(munge_status));
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
    
    // Debug
    bytes = read(msg_fd, buff, LPJS_MSG_MAX+1);
    fprintf(Log_stream, "%s\n", buff);
    
    node_detect_specs(&node);
    node_send_specs(&node, msg_fd);
    
    // Now keep daemon running, awaiting jobs
    // FIXME: Block read until message received instead of sleep
    while ( (bytes = read(msg_fd, buff, LPJS_MSG_MAX+1)) == 0 )
	sleep(10);
    if ( bytes == -1 )
    {
	perror("lpjs-submit: Failed to read response from dispatch");
	close(msg_fd);
	return EX_IOERR;
    }
    fprintf(Log_stream, "%s\n", buff);

    close(msg_fd);
    fclose(Log_stream);

    return EX_OK;
}

