#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdarg.h>
#include "xtend/string.h"   // strlcpy() on Linux
#include "xtend/net.h"
#include "node-list.h"
#include "network.h"
#include "lpjs.h"

/***************************************************************************
 *  Description:
 *      Open a socket connection to lpjs_dispatchd.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

int     connect_to_dispatch(node_list_t *node_list)

{
    char                head_ip[LPJS_IP_MAX + 1];
    short               tcp_port;   /* Need short for htons() */
    struct sockaddr_in  server_address;
    int                 msg_fd;
    
    // FIXME: Get this by resolving head hostname in config file
    if ( resolve_hostname(NODE_LIST_HEAD_NODE(node_list), head_ip, LPJS_IP_MAX + 1) != XT_OK )
	exit(EX_OSERR);
    tcp_port = LPJS_TCP_PORT;

    /* Set up socket structure */
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr (head_ip);
    server_address.sin_port = htons (tcp_port);

    /* Create a socket */
    if ((msg_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
	perror("connect_to_dispatch(): socket() failed");
	return -1;
    }

    /* Attempt to connect to server */
    if (connect(msg_fd, (struct sockaddr *)&server_address,
		 sizeof(server_address)) < 0)
    {
	perror("connect_to_dispatch(): connect() failed");
	fprintf(stderr, "hostname %s, ip = %s\n", 
		NODE_LIST_HEAD_NODE(node_list), head_ip);
	return -1;
    }

    return msg_fd;
}


/***************************************************************************
 *  Description:
 *      Echo a response from msg_fd directly to stdout.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

int     print_response(int msg_fd, const char *caller_name)

{
    ssize_t bytes;
    char    buff[LPJS_MSG_MAX+1];
    
    while ( (bytes = read(msg_fd, buff, LPJS_MSG_MAX + 1)) > 0 )
    {
	buff[bytes] = '\0';
	// FIXME: null-terminate at sender?
	printf("%s", buff);
    }
    
    if ( bytes == -1 )
    {
	fprintf(stderr, "%s: Failed to read response from dispatch",
		strerror(errno));
	return EX_IOERR;
    }
    return EX_OK;
}


/***************************************************************************
 *  Description:
 *      Construct and send a message through a socket.  The entire message
 *      + a null byte are sent in a single write().  Basically the same as
 *      dprintf(), except that it null-terminates the message.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-29  Jason Bacon Begin
 ***************************************************************************/

int     send_msg(int msg_fd, const char *format, ...)

{
    va_list     ap;
    int         status;
    char        buff[LPJS_MSG_MAX + 1];
    
    va_start(ap, format);
    status = vsnprintf(buff, LPJS_MSG_MAX + 1, format, ap);
    write(msg_fd, buff, strlen(buff) + 1);
    va_end(ap);
    
    return status;
}
