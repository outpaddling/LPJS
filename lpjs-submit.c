#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
#include "node-list.h"
#include "config.h"
#include "network.h"
#include "lpjs.h"

int     main (int argc, char *argv[])
{
    int     msg_fd;
    ssize_t bytes;
    char    buff[LPJS_MSG_MAX+1];
    node_list_t node_list;
    
    if (argc != 1)
    {
	fprintf (stderr, "Usage: %s\n", argv[0]);
	return EX_USAGE;
    }

    // Get hostname of head node
    lpjs_load_config(&node_list);

    if ( (msg_fd = connect_to_dispatch(&node_list)) == -1 )
    {
	perror("lpjs-submit: Failed to connect to dispatch");
	return EX_IOERR;
    }

    /* Send a message to the server */
    /* Need to send \0, so dprintf() doesn't work here */
    if ( write(msg_fd, "submit", 7) == -1 )
    {
	perror("lpjs-submit: Failed to send message to dispatch");
	close(msg_fd);
	return EX_IOERR;
    }
    
    if ( (bytes = read(msg_fd, buff, LPJS_MSG_MAX+1)) == -1 )
    {
	perror("lpjs-submit: Failed to read response from dispatch");
	close(msg_fd);
	return EX_IOERR;
    }
    buff[bytes] = '\0';
    puts(buff);
    close (msg_fd);

    return EX_OK;
}
