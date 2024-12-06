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

#include <xtend/string.h>   // strlcat() on linux

#include "node-list.h"
#include "config.h"
#include "network.h"
#include "lpjs.h"
#include "misc.h"
#include "nodes-protos.h"

int     main (int argc, char *argv[])

{
    int         msg_fd;
    // Terminates process if malloc() fails, no check required
    node_list_t *node_list = node_list_new();
    char        outgoing_msg[LPJS_MSG_LEN_MAX + 1];
    extern FILE *Log_stream;
    
    switch(argc)
    {
	case    1:  // lpjs nodes
	    outgoing_msg[0] = LPJS_DISPATCHD_REQUEST_NODE_LIST;
	    outgoing_msg[1] = '\0';
	    break;
	
	default:
	    if ( (strcmp(argv[1], "paused") == 0) ||
		 (strcmp(argv[1], "updating") == 0) ||
		 (strcmp(argv[1], "up") == 0) )
		lpjs_set_node_state(argc, argv, outgoing_msg, LPJS_MSG_LEN_MAX + 1);
	    else
		usage(argv);
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

    if ( lpjs_send_munge(msg_fd, outgoing_msg, close) != LPJS_MSG_SENT )
    {
	perror("lpjs-nodes: Failed to send message to dispatch");
	close(msg_fd);
	return EX_IOERR;
    }

    // fprintf(stderr, "LPJS_DISPATCHD_REQUEST_NODE_STATUS sent.\n");
    lpjs_print_response(msg_fd, "lpjs-nodes");
    close(msg_fd);

    return EX_OK;
}


/***************************************************************************
 *  Use auto-c2man to generate a man page from this comment
 *
 *  Name:
 *      -
 *
 *  Library:
 *      #include <>
 *      -l
 *
 *  Description:
 *  
 *  Arguments:
 *
 *  Returns:
 *
 *  Examples:
 *
 *  Files:
 *
 *  Environment
 *
 *  See also:
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-05-10  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_set_node_state(int argc, char *argv[],
			     char *msg, size_t msg_max)

{
    // lpjs nodes pause all | nodename [nodename ...]
    if ( argc < 3 )
	usage(argv);
    // paused and updating are basically the same.  The latter is to
    // facilitate integration with SPCM, so it can easily identify
    // nodes that were paused specifically for updates.
    if ( (strcmp(argv[1], "paused") == 0) ||
	 (strcmp(argv[1], "updating") == 0) )
	snprintf(msg, msg_max, "%c%s", LPJS_DISPATCHD_REQUEST_PAUSE, argv[1]);
    else
	snprintf(msg, msg_max, "%c%s", LPJS_DISPATCHD_REQUEST_RESUME, argv[1]);
    for (int c = 2; c < argc; ++c)
    {
	if ( (strcmp(argv[c], "all") == 0) && ((c > 2) || (argc > 3)) )
	    usage(argv);
	strlcat(msg, " ", msg_max);
	strlcat(msg, argv[c], msg_max);
    }
    
    return 0;   // FIXME: Define return codes
}


void    usage(char *argv[])

{
    fprintf (stderr, "Usage: %s nodes [paused|updating|up all|nodename [nodename...]]\n", argv[0]);
    exit(EX_USAGE);
}
