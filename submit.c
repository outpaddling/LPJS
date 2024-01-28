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
#include <sys/stat.h>
#include <sys/socket.h>

#include <munge.h>
#include <xtend/string.h>
#include <xtend/file.h>     // xt_rmkdir()

#include "node-list.h"
#include "config.h"
#include "network.h"
#include "misc.h"
#include "lpjs.h"

int     main (int argc, char *argv[])
{
    int     msg_fd;
    ssize_t bytes;
    char    outgoing_msg[LPJS_MSG_LEN_MAX + 1],
	    incoming_msg[LPJS_MSG_LEN_MAX + 1],
	    *cred,
	    *script_name;
    node_list_t node_list;
    munge_err_t munge_status;
    // Shared functions may use lpjs_log
    extern FILE *Log_stream;
    
    if ( argc != 2 )
    {
	fprintf (stderr, "Usage: %s script.lpjs\n", argv[0]);
	return EX_USAGE;
    }
    script_name =argv[1];

    // FIXME: Prevent unchecked log growth
    if ( xt_rmkdir(LPJS_LOG_DIR, 0755) != 0 )
    {
	perror("Cannot create " LPJS_LOG_DIR);
	return EX_CANTCREAT;
    }
    
    Log_stream = fopen(LPJS_CHAPERONE_LOG, "a");
    if ( Log_stream == NULL )
    {
	perror("Cannot open " LPJS_CHAPERONE_LOG);
	return EX_CANTCREAT;
    }
    
    // Get hostname of head node
    lpjs_load_config(&node_list, LPJS_CONFIG_HEAD_ONLY, stderr);

    if ( (msg_fd = lpjs_connect_to_dispatchd(&node_list)) == -1 )
    {
	perror("lpjs-submit: Failed to connect to dispatch");
	return EX_IOERR;
    }

    outgoing_msg[0] = LPJS_REQUEST_SUBMIT;
    outgoing_msg[1] = '\0';
    if ( lpjs_send_msg(msg_fd, 0, outgoing_msg, 0) < 1 )
    {
	perror("lpjs-submit: Failed to send submit request to dispatch");
	close(msg_fd);
	return EX_IOERR;
    }
    
    if ( (munge_status = munge_encode(&cred, NULL, script_name, strlen(script_name) + 1)) != EMUNGE_SUCCESS )
    {
	lpjs_log("lpjs-submit: munge_encode() failed.\n");
	lpjs_log("Return code = %s\n", munge_strerror(munge_status));
	return EX_UNAVAILABLE; // FIXME: Check actual error
    }

    if ( lpjs_send_msg(msg_fd, 0, cred) < 0 )
    {
	perror("lpjs-submit: Failed to send credential to dispatch");
	close(msg_fd);
	free(cred);
	return EX_IOERR;
    }
    free(cred);
    
    if ( (bytes = lpjs_recv_msg(msg_fd, incoming_msg, LPJS_MSG_LEN_MAX, 0)) == -1 )
    {
	perror("lpjs-submit: Failed to read response from dispatch");
	close(msg_fd);
	return EX_IOERR;
    }
    incoming_msg[bytes] = '\0';
    puts(incoming_msg);

    return EX_OK;
}
