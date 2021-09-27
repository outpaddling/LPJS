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
    ssize_t bytes;
    char    *message,
	    buff[LPJS_MSG_MAX+1];
    node_list_t node_list;
    
    if (argc != 1)
    {
	fprintf (stderr, "Usage: %s\n", argv[0]);
	return EX_USAGE;
    }

    lpjs_load_config(&node_list);

    // Does not return if connection files
    msg_fd = connect_to_dispatch(&node_list);

    /* Send a message to the server */
    message = "jobs";
    if ( write (msg_fd, message, strlen (message) + 1) == -1 )
    {
	perror("write() failed");
	close(msg_fd);
	return EX_IOERR;
    }
    
    if ( (bytes = read(msg_fd, buff, LPJS_MSG_MAX+1)) == -1 )
    {
	perror("read() failed");
	close(msg_fd);
	return EX_IOERR;
    }
    printf("%zu bytes read.\n", bytes);
    buff[bytes] = '\0';
    puts(buff);

    close (msg_fd);
    return EX_OK;
}

