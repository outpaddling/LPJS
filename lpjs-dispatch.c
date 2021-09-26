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
#include <sysexits.h>
#include <limits.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "node-list.h"
#include "lpjs.h"

int     main(int argc,char *argv[])

{
    char    config_file[PATH_MAX+1];
    
    node_list_t nodes;
    
    node_list_init(&nodes);
    snprintf(config_file, PATH_MAX+1, "%s/etc/lpjs/config", LOCALBASE);
    node_list_populate(&nodes, config_file);
    return process_events(&nodes);
}


/***************************************************************************
 *  Description:
 *      Listen for messages on TCP port.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-25  Jason Bacon Begin
 ***************************************************************************/

int     process_events(node_list_t *nodes)

{
    int         fd;
    short       tcp_port;   /* Need short for htons() */
    socklen_t   address_len = sizeof (struct sockaddr_in);
    char        buff[LPJS_IP_MSG_MAX + 1];
    // FIXME: Support IPV6
    struct sockaddr_in server_address;

    // FIXME: Pick a good default and dheck config file for override
    tcp_port = 3000;
    if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
	fprintf (stderr, "Error opening socket \n");
	return EX_UNAVAILABLE;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl (INADDR_ANY);
    server_address.sin_port = htons (tcp_port);

    /* Bind socket to server address */
    if (bind (fd, (struct sockaddr *) &server_address,
	      sizeof (server_address)) < 0)
    {
	perror ("bind() failed");
	return EX_UNAVAILABLE;
    }

    while ( 1 )
    {
	/* Listen for connection requests */
	if (listen (fd, LPJS_MSG_QUEUE_MAX) != 0)
	{
	    fputs ("listen() failed.\n", stderr);
	    return EX_UNAVAILABLE;
	}
    
	/* Accept a connection request */
	if ((fd = accept(fd, (struct sockaddr *)&server_address,
		    &address_len)) == -1)
	{
	    fputs ("accept() failed.\n", stderr);
	    return EX_UNAVAILABLE;
	}
	printf ("Accepted connection. fd = %d\n", fd);
    
	/* Read a message through the socket */
	if ( read (fd, buff, 100) == - 1)
	{
	    perror("read() failed");
	    return EX_IOERR;
	}
	puts (buff);
    }
    close (fd);
    return EX_OK;
}
