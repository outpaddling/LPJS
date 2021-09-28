#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "xtend/string.h"   // strlcpy() on Linux
#include "node-list.h"
#include "network.h"

void    resolve_hostname(const char *hostname, char *ip, size_t ip_buff_len)

{
    struct hostent  *ent;
    struct in_addr  **address_list;

    /*
     *  FIXME: Reimplement with getaddrinfo() to better support IPv6
     *  gethostbyname() is simpler and will suffice for now
     */
    
    if ( (ent = gethostbyname(hostname)) == NULL )
    {
	herror("resolve_hostname(): gethostbyname() failed");
	exit(EX_OSERR);
    }

    // Just take first address
    address_list = (struct in_addr **)ent->h_addr_list;
    strlcpy(ip, inet_ntoa(*address_list[0]), ip_buff_len);
}


int     connect_to_dispatch(node_list_t *node_list)

{
    char                head_ip[LPJS_IP_MAX + 1];
    short               tcp_port;   /* Need short for htons() */
    struct sockaddr_in  server_address;
    int                 msg_fd;
    
    // FIXME: Get this by resolving head hostname in config file
    resolve_hostname(NODE_LIST_HEAD_NODE(node_list), head_ip, LPJS_IP_MAX + 1);
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
	return -1;
    }

    return msg_fd;
}


int     print_response(int msg_fd, const char *caller_name)

{
    ssize_t bytes;
    char    buff[LPJS_MSG_MAX+1];
    
    while ( (bytes = read(msg_fd, buff, LPJS_MSG_MAX + 1)) > 0 )
    {
	buff[bytes] = '\0';
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
