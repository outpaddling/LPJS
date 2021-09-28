/***************************************************************************
 *  Description:
 *  
 *  Arguments:
 *
 *  Returns:
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-23  Jason Bacon Begin
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "node-list.h"
#include "config.h"
#include "scheduler.h"
#include "lpjs.h"

int     main(int argc,char *argv[])

{
    node_list_t node_list;
    
    lpjs_load_config(&node_list, LPJS_CONFIG_ALL);
    return process_events(&node_list);
}


/***************************************************************************
 *  Description:
 *      Listen for messages on TCP port.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-25  Jason Bacon Begin
 ***************************************************************************/

int     process_events(node_list_t *node_list)

{
    int         listen_fd,
		msg_fd;
    ssize_t     bytes;
    short       tcp_port;   /* Need short for htons() */
    socklen_t   address_len = sizeof (struct sockaddr_in);
    char        incoming_msg[LPJS_IP_MSG_MAX + 1];
    // FIXME: Support IPV6
    struct sockaddr_in server_address = { 0 };

    // FIXME: Pick a good default and dheck config file for override
    tcp_port = 3000;
    if ((listen_fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
	fprintf (stderr, "Error opening socket \n");
	return EX_UNAVAILABLE;
    }
    printf("listen_fd = %d\n", listen_fd);

    /*
     *  FIXME
     *  Set handler so that listen_fd is properly closed before termination
     *  so that we don't get bind(): address alread in use during testing
     *  with its frequent restarts
     *  https://hea-www.harvard.edu/~fine/Tech/addrinuse.html
     */
    
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(tcp_port);

    // printf("%d %d\n", server_address.sin_len, sizeof(server_address));
    
    /* Bind socket to server address */
    if (bind (listen_fd, (struct sockaddr *) &server_address,
	      sizeof (server_address)) < 0)
    {
	perror ("bind() failed");
	return EX_UNAVAILABLE;
    }
    printf("Bound to port %d...\n", tcp_port);
    
    while ( 1 )
    {
	/* Listen for connection requests */
	if (listen (listen_fd, LPJS_MSG_QUEUE_MAX) != 0)
	{
	    fputs ("listen() failed.\n", stderr);
	    return EX_UNAVAILABLE;
	}
    
	/* Accept a connection request */
	if ((msg_fd = accept(listen_fd,
		(struct sockaddr *)&server_address, &address_len)) == -1)
	{
	    fputs ("accept() failed.\n", stderr);
	    return EX_UNAVAILABLE;
	}
	printf ("Accepted connection. fd = %d\n", msg_fd);
    
	/* Read a message through the socket */
	if ( (bytes = read(msg_fd, incoming_msg, 100)) == - 1)
	{
	    perror("read() failed");
	    return EX_IOERR;
	}
	printf("%zu %s\n", bytes, incoming_msg);
	
	/* Process request */
	if ( strcmp(incoming_msg, "nodes") == 0 )
	    node_list_send_specs(msg_fd, node_list);
	else if ( strcmp(incoming_msg, "jobs") == 0 )
	    dprintf(msg_fd, "jobs not yet implemented\n");
	else if ( memcmp(incoming_msg, "submit", 6) == 0 )
	    queue_job(msg_fd, incoming_msg, node_list);
	close(msg_fd);
    }
    close (listen_fd);
    return EX_OK;
}
