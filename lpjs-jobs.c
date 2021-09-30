/***************************************************************************
 *  Description:
 *  
 *  Arguments:
 *
 *  Returns:
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
    int     msg_fd;
    node_list_t node_list;
    
    if (argc != 1)
    {
	fprintf (stderr, "Usage: %s\n", argv[0]);
	return EX_USAGE;
    }

    // Get hostname of head node
    lpjs_load_config(&node_list, LPJS_CONFIG_HEAD_ONLY);

    if ( (msg_fd = connect_to_dispatch(&node_list)) == -1 )
    {
	perror("lpjs-jobs: Failed to connect to dispatch");
	return EX_IOERR;
    }

    if ( send_msg(msg_fd, "jobs") < 0 )
    {
	perror("lpjs-jobs: Failed to send message to dispatch");
	close(msg_fd);
	return EX_IOERR;
    }
    
    print_response(msg_fd, "lpjs-nodes");
    close (msg_fd);

    return EX_OK;
}
