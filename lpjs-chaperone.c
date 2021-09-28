/***************************************************************************
 *  Description:
 *      Chaperone for running jobs on a compute node.  When jobs are
 *      dispatched to a compute node, lpjs-chaperone is executed and
 *      the actually command is run as a child of lpjs-chaperone.
 *
 *      lpjs-chaperone monitors its child to enforce resource limits
 *      and report exit status back to the dispatcher.
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
#include "misc.h"
#include "lpjs.h"

int     main (int argc, char *argv[])
{
    int         msg_fd,
		status;
    node_list_t node_list;
    char        cmd[LPJS_CMD_MAX + 1] = "",
		msg[LPJS_MSG_MAX + 1];
    size_t      c;
    
    if (argc < 2)
    {
	fprintf (stderr, "Usage: %s command [args]\n", argv[0]);
	return EX_USAGE;
    }

    // Get hostname of head node
    lpjs_load_config(&node_list, LPJS_CONFIG_HEAD_ONLY);

    if ( (msg_fd = connect_to_dispatch(&node_list)) == -1 )
    {
	perror("lpjs-nodes: Failed to connect to dispatch");
	return EX_IOERR;
    }

    /* Send a message to the server */
    /* Need to send \0, so dprintf() doesn't work here */
    argv_to_cmd(cmd, argv, LPJS_CMD_MAX + 1);
    status = system(cmd);
    snprintf(msg, LPJS_MSG_MAX + 1, "Completed with status %d", status);
    if ( write(msg_fd, msg, strlen(msg) + 1) == -1 )
    {
	perror("lpjs-nodes: Failed to send message to dispatch");
	close(msg_fd);
	return EX_IOERR;
    }

    print_response(msg_fd, "lpjs-nodes");
    close (msg_fd);

    return EX_OK;
}
