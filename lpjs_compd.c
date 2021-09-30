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
    char        cmd[LPJS_CMD_MAX + 1] = "";
    
    if (argc != 1)
    {
	fprintf (stderr, "Usage: %s\n", argv[0]);
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
    if ( send_msg(msg_fd, "compd-checkin") < 0 )
    {
	perror("lpjs-nodes: Failed to send message to dispatch");
	close(msg_fd);
	return EX_IOERR;
    }
    close (msg_fd);

    return EX_OK;
}
