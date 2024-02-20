#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdarg.h>

#include <munge.h>
#include <xtend/string.h>   // strlcpy() on Linux
#include <xtend/net.h>
#include <xtend/file.h>

#include "node-list.h"
#include "network.h"
#include "lpjs.h"
#include "misc.h"

/***************************************************************************
 *  Description:
 *      Open a socket connection to lpjs_dispatchd.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_connect_to_dispatchd(node_list_t *node_list)

{
    char                head_text_ip[LPJS_TEXT_IP_ADDRESS_MAX + 1];
    struct sockaddr_in  server_address;     // sockaddr_in = inet4
    int                 msg_fd;

    /*
     *  Create a socket endpoint to pair with the endpoint on the server.
     *  AF_INET and PF_INET have the same value, but PF_INET is more
     *  correct according to BSD and Linux man pages, which indicate
     *  that a protocol family should be specified.  In theory, a
     *  protocol family can support more than one address family.
     *  SOCK_STREAM indicates a reliable stream oriented protocol,
     *  such as TCP, vs. unreliable unordered datagram protocols like UDP.
     */
    if ((msg_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
	lpjs_log("connect_to_dispatchd(): socket() failed: %s", strerror(errno));
	return -1;
    }

    // AF_INET = inet4 (IPv4), AF_INET6 for inet6 (IPv6)
    server_address.sin_family = AF_INET;
    
    // Convert head node hostname from LPJS config file to IP
    if ( xt_resolve_hostname(NODE_LIST_HEAD_NODE(node_list), head_text_ip,
			  LPJS_TEXT_IP_ADDRESS_MAX + 1) != XT_OK )
	exit(EX_OSERR);
    
    // Convert inet4 string xxx.xxx.xxx.xxx to 32-bit IP in network byte order
    server_address.sin_addr.s_addr = inet_addr(head_text_ip);
    
    // Convert 16-bit port number to network byte order
    server_address.sin_port = htons(LPJS_IP_TCP_PORT);

    /* Attempt to connect to dispatchd server */
    if ( connect(msg_fd, (struct sockaddr *)&server_address,
		 sizeof(server_address)) < 0 )
    {
	lpjs_log("connect_to_dispatchd(): connect() failed: %s\n", strerror(errno));
	lpjs_log("hostname %s, ip = %s\n", 
		NODE_LIST_HEAD_NODE(node_list), head_text_ip);
	return -1;
    }
    
    /*
     *  FIXME: Is there a way to terminate the connection to the listener
     *  socket (port 6817 by default) immediately after msg_fd connects?
     *  It lingers for a minute or so, causing bind() to fail on the
     *  head node when restarting dispatchd.
     */

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

int     lpjs_print_response(int msg_fd, const char *caller_name)

{
    ssize_t bytes;
    char    buff[LPJS_MSG_LEN_MAX + 1];
    bool    eot_received = false;
    
    while ( ! eot_received &&
	    (bytes = recv(msg_fd, buff, LPJS_MSG_LEN_MAX, 0)) > 0 )
    {
	eot_received = (buff[bytes-1] == 4);
	if ( eot_received )
	{
	    --bytes;
	    // lpjs_log("EOT received.\n");
	}
	buff[bytes] = '\0';
	printf("%s", buff);
    }
    
    if ( bytes == -1 )
    {
	lpjs_log("%s: Failed to read response from dispatchd",
		strerror(errno));
	return EX_IOERR;
    }
    return EX_OK;
}


/***************************************************************************
 *  Description:
 *      Construct and send a message through a socket.  The length of
 *      the entire message is sent as a uint16_t in network byte order
 *      first, so the receiver knows exactly how many bytes to read.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-29  Jason Bacon Begin
 ***************************************************************************/

ssize_t lpjs_send(int msg_fd, int send_flags, const char *format, ...)

{
    va_list     ap;
    int         status;
    uint16_t    msg_len;
    char        buff[LPJS_MSG_LEN_MAX + 1];
    
    va_start(ap, format);
    status = vsnprintf(buff, LPJS_MSG_LEN_MAX + 1, format, ap);
   
    // vsnprintf() returns the length the the string would have been
    // if buff were unlimited, so we have to use strlen.
    msg_len = htons(strlen(buff));
    send(msg_fd, &msg_len, sizeof(uint16_t), 0);
    
    // Also send '\0' byte to mark end of message
    send(msg_fd, buff, strlen(buff), send_flags);
    va_end(ap);
    
    return status;
}


/***************************************************************************
 *  Description:
 *      Receive a message sent by lpjs_send().  A uint16_t containing
 *      the message length in network byte order is received first,
 *      followed by the message.  The interface is idential to recv(2).
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-19  Jason Bacon Begin
 ***************************************************************************/

ssize_t lpjs_recv(int msg_fd, char *buff, size_t buff_len, int flags,
		      int timeout)

{
    uint16_t    msg_len;
    ssize_t     bytes_read;
    fd_set      read_fds;
    struct timeval  timeout_tv = { timeout, 0 };    // timeout sec, 0 us
    
    // Use select() to implement timeout without using non-blocking fds
    if ( timeout != 0 )
    {
	FD_ZERO(&read_fds);
	FD_SET(msg_fd, &read_fds);
	lpjs_log("%s: Entering select()...\n", __FUNCTION__);
	if ( select(msg_fd + 1, &read_fds, NULL, NULL, &timeout_tv) == 0 )
	{
	    lpjs_log("select() returned 0.\n");
	    return 0;
	}
    }

    // lpjs_log("Receiving message...\n");
    bytes_read = recv(msg_fd, &msg_len, sizeof(uint16_t), flags | MSG_WAITALL);
    if ( bytes_read == 0 )
	return 0;
    else if ( bytes_read == -1 )
    {
	lpjs_log("lpjs_recv(): recv() returned -1: %s\n", strerror(errno));
	return 0;
    }
    else if ( bytes_read != sizeof(uint16_t) )
    {
	lpjs_log("lpjs_recv(): Read partial msg_len: %zd bytes.\n",
		bytes_read);
	exit(EX_DATAERR);
    }
    msg_len = ntohs(msg_len);
    // lpjs_log("lpjs_recv(): msg_len = %u\n", msg_len);
    
    if ( msg_len > buff_len - 1 )
    {
	lpjs_log("lpjs_recv(): msg_len > buff_len -1.\n");
	lpjs_log("This is a software bug.\n");
	exit(EX_SOFTWARE);
    }
    
    bytes_read = recv(msg_fd, buff, msg_len, flags | MSG_WAITALL);
    buff[bytes_read] = '\0';
    // lpjs_log("lpjs_recv(): Got '%s'.\n", buff);
    
    return bytes_read;
}


/***************************************************************************
 *  Description:
 *      Receive a message sent by lpjs_send().  A uint16_t containing
 *      the message length in network byte order is received first,
 *      followed by the message.  The interface is idential to recv(2).
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-19  Jason Bacon Begin
 ***************************************************************************/

ssize_t lpjs_recv_munge(int msg_fd, char **payload,
			int flags, int timeout, uid_t *uid, gid_t *gid)

{
    ssize_t     bytes_read;
    int         payload_len;
    munge_err_t munge_status;
    char        incoming_msg[LPJS_MSG_LEN_MAX + 1];
    
    bytes_read = lpjs_recv(msg_fd, incoming_msg, LPJS_MSG_LEN_MAX + 1,
			    flags, timeout);
    
    if ( bytes_read == -1 )
    {
	lpjs_log("%s: lpjs_recv() failed: %s", __FUNCTION__, strerror(errno));
	return -1;
    }
    
    if ( bytes_read > 0 )
    {
	lpjs_log("%s(): Unmunging %zd bytes...\n", __FUNCTION__, bytes_read);
	munge_status = munge_decode(incoming_msg, NULL, (void **)payload,
				    &payload_len, uid, gid);
	lpjs_log("%s(): Payload len = %d\n", __FUNCTION__, payload_len);
	if ( munge_status != EMUNGE_SUCCESS )
	{
	    lpjs_server_safe_close(msg_fd);
	    lpjs_log("%s(): munge_decode() failed.  Error = %s\n",
		     __FUNCTION__, munge_strerror(munge_status));
	    return -1;
	}
	
	// Acknolwedge successful receipt of message
	lpjs_send(msg_fd, 0, LPJS_MUNGE_CRED_VERIFIED);
	return payload_len;
    }
    else
	return 0;
}


/***************************************************************************
 *  Description:
 *      Send EOT (\004) char to signal end of communication.
 *      Note that end of message is signalled by \000.  EOT
 *      means we're done talking and the socket should be closed.
 *
 *      FIXME: Can we use send(fd, buff, len, MSG_EOF) instead?
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-17  Jason Bacon Begin
 ***************************************************************************/

ssize_t lpjs_send_eot(int msg_fd)

{
    char    buff[2] = "\004";
    ssize_t bytes;
    
    bytes = lpjs_send(msg_fd, 0, buff);
    if ( bytes != 1 )
	lpjs_log("send_eot(): Failed to send EOT.\n");
    
    return bytes;
}


/***************************************************************************
 *  Description:
 *      Send a munge-encoded message
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-21  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_send_munge(int msg_fd, char *msg)

{
    char        *cred,
		incoming_msg[LPJS_MSG_LEN_MAX + 1];
    ssize_t     bytes;
    munge_err_t munge_status;
    
    if ( (munge_status = munge_encode(&cred, NULL, msg, strlen(msg))) != EMUNGE_SUCCESS )
    {
	lpjs_log("lpjs_compd: munge_encode() failed.\n");
	lpjs_log("Return code = %s\n", munge_strerror(munge_status));
	return EX_UNAVAILABLE; // FIXME: Check actual error
    }

    // printf("Sending %zd bytes: %s...\n", strlen(cred), cred);
    if ( lpjs_send(msg_fd, 0, cred) < 0 )
    {
	perror("lpjs_compd: Failed to send credential to dispatchd");
	close(msg_fd);
	free(cred);
	return EX_IOERR;
    }
    free(cred);
    
    // Read acknowledgment from dispatchd before node_send_specs().
    bytes = lpjs_recv(msg_fd, incoming_msg, LPJS_MSG_LEN_MAX, 0, 0);
    // lpjs_log("Response: %zu '%s'\n", bytes, incoming_msg);
    if ( (bytes == 0) || (strcmp(incoming_msg, LPJS_MUNGE_CRED_VERIFIED) != 0) )
    {
	lpjs_log("lpjs_compd_checkin(): Expected %s.\n", LPJS_MUNGE_CRED_VERIFIED);
	return EX_DATAERR;
    }
    // lpjs_log("%s(): Munge message acknolwedged.\n", __FUNCTION__);

    return EX_OK;
}


/***************************************************************************
 *  Description:
 *      Safely close a socket by ensuring first that the remote end
 *      is closed first.  This avoids
 *
 *      bind(): Address already in use
 *  
 *  History: 
 *  Date        Name        Modification
 *  2024-01-14  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_server_safe_close(int msg_fd)

{
    char    buff[64];
    
    /*
     *  Client must be looking for the EOT character at the end of
     *  a read, or this is useless.
     */
    lpjs_send_eot(msg_fd);
    
    /*
     *  Wait until EOF is signaled due to the other end being closed.
     *  FIXME: No data should be read here.  The first read() should
     *  return EOF.  Add a check for this.
     */
    while ( read(msg_fd, buff, 64) > 0 )
	sleep(1);
    
    // lpjs_log(Log_stream, "Closing msg_fd.\n");
    return close(msg_fd);
}
