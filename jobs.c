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
    int     msg_fd;
    node_list_t node_list;
    
    if (argc != 1)
    {
	fprintf (stderr, "Usage: %s\n", argv[0]);
	return EX_USAGE;
    }

    // Get hostname of head node
    lpjs_load_config(&node_list, LPJS_CONFIG_HEAD_ONLY, stderr);

    if ( (msg_fd = lpjs_connect_to_dispatchd(&node_list)) == -1 )
    {
	perror("lpjs-jobs: Failed to connect to dispatch");
	return EX_IOERR;
    }

    if ( lpjs_send_msg(msg_fd, "jobs") < 0 )
    {
	perror("lpjs-jobs: Failed to send message to dispatch");
	close(msg_fd);
	return EX_IOERR;
    }
    
    lpjs_print_response(msg_fd, "lpjs-jobs");
    close (msg_fd);

    return EX_OK;
}
