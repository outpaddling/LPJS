/***************************************************************************
 *  Description:
 *      List currently running and queued jobs.  Job info is queried from
 *      lpjs_dispatchd.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-27  Jason Bacon Begin
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>

#include "node-list.h"
#include "config.h"
#include "network.h"
#include "lpjs.h"

int     main(int argc,char *argv[])

{
    int         msg_fd;
    // Terminates process if malloc() fails, no check required
    node_list_t *node_list = node_list_new();
    extern FILE *Log_stream;
    char        outgoing_msg[LPJS_MSG_LEN_MAX + 1];
    
    if (argc != 1)
    {
	fprintf (stderr, "Usage: %s\n", argv[0]);
	return EX_USAGE;
    }

    // Shared functions may use lpjs_log
    Log_stream = stderr;
    
    // Get hostname of head node
    lpjs_load_config(node_list, LPJS_CONFIG_HEAD_ONLY, stderr);

    if ( (msg_fd = lpjs_connect_to_dispatchd(node_list)) == -1 )
    {
	perror("lpjs-jobs: Failed to connect to dispatch");
	return EX_IOERR;
    }

    outgoing_msg[0] = LPJS_DISPATCHD_REQUEST_JOB_STATUS;
    outgoing_msg[1] = '\0';
    if ( lpjs_send_munge(msg_fd, outgoing_msg, close) != EX_OK )
    {
	perror("lpjs-jobs: Failed to send message to dispatch");
	close(msg_fd);
	return EX_IOERR;
    }

    puts("\nLegend: P = processor  J = job  N = node  S = submission\n");
    lpjs_print_response(msg_fd, "lpjs-jobs");
    close (msg_fd);

    return EX_OK;
}
