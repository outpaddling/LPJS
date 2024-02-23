/***************************************************************************
 *  Description:
 *      List currently running and queued nodes.  Node info is queried from
 *      lpjs_dispatchd.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-25  Jason Bacon Begin
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
#include "misc.h"

int     main (int argc, char *argv[])

{
    int         msg_fd;
    node_list_t *node_list = node_list_new();
    char        outgoing_msg[LPJS_MSG_LEN_MAX + 1];
    extern FILE *Log_stream;
    
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
	perror("lpjs-nodes: Failed to connect to dispatch");
	return EX_IOERR;
    }

    outgoing_msg[0] = LPJS_REQUEST_NODE_STATUS;
    outgoing_msg[1] = '\0';
    if ( lpjs_send_munge(msg_fd, outgoing_msg) != EX_OK )
    {
	perror("lpjs-nodes: Failed to send message to dispatch");
	close(msg_fd);
	return EX_IOERR;
    }

    fprintf(stderr, "LPJS_REQUEST_NODE_STATUS sent.\n");
    lpjs_print_response(msg_fd, "lpjs-nodes");
    close(msg_fd);

    return EX_OK;
}
