#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "lpjs.h"

int     main (int argc, char *argv[])
{
    short   tcp_port;   /* Need short for htons() */
    int     fd;
    struct sockaddr_in server_address;
    char   *machine_address, *message;

    if (argc != 1)
    {
	fprintf (stderr, "Usage: %s\n", argv[0]);
	return EX_USAGE;
    }

    // FIXME: Get this by resolving head hostname in config file
    machine_address = "192.168.0.56";
    tcp_port = LPJS_TCP_PORT;

    /* Set up socket structure */
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr (machine_address);
    server_address.sin_port = htons (tcp_port);

    /* Create a socket */
    if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
	fprintf (stderr, "Error open socket of client\n");
	exit (0);
    }

    /* Attempt to connect to server */
    if (connect (fd, (struct sockaddr *)&server_address,
		 sizeof (server_address)) < 0)
    {
	fprintf (stderr, "Error connecting to client\n");
	exit (0);
    }

    /* Send a message to the server */
    message = "Hello, TCP world!\n";
    write (fd, message, strlen (message) + 1);

    close (fd);
    return EX_OK;
}

