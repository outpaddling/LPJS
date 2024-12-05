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
    if ( xt_resolve_hostname(node_list_get_head_node(node_list), head_text_ip,
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
		node_list_get_head_node(node_list), head_text_ip);
	close(msg_fd);
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
    char    *payload;
    bool    eot_received = false;
    uid_t   uid;
    gid_t   gid;
    
    // This function should never be called by dispatchd, so use
    // a normal close()
    // FIXME: Add timeout handling code
    while ( ! eot_received &&
	    (bytes = lpjs_recv_munge(msg_fd, &payload, 0,
				     LPJS_PRINT_RESPONSE_TIMEOUT,
				     &uid, &gid, close)) > 0 )
    {
	eot_received = (payload[bytes-1] == 4);
	if ( eot_received )
	    --bytes;
	payload[bytes] = '\0';
	printf("%s", payload);
	free(payload);
    }
    
    // FIXME: Distinguish between error and LPJS_RECV_TIMEOUT
    if ( bytes == LPJS_RECV_TIMEOUT )
    {
	close(msg_fd);
	lpjs_log("%s(): Timed out after %dus\n",
		 __FUNCTION__, LPJS_PRINT_RESPONSE_TIMEOUT);
	return LPJS_RECV_TIMEOUT;
    }
    else if ( bytes == LPJS_RECV_FAILED )
    {
	// This function should never be called by dispatchd, so
	// do a normal close() vs lpjs_dispatchd_safe_close()
	close(msg_fd);
	lpjs_log("%s(): Failed to read response from dispatchd: %s\n",
		__FUNCTION__, strerror(errno));
	return EX_IOERR;
    }
    else
    {
	lpjs_log("%s(): Internal error: Undefined return code from lpjs_recv(): %d\n",
		 __FUNCTION__, bytes);
	// FIXME: What should we really do here?
	return LPJS_RECV_FAILED;
    }
    return EX_OK;
}


/***************************************************************************
 *  Description:
 *      Construct and send a message through a socket.  The length of
 *      the entire message is sent as a uint32_t in network byte order
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
    uint32_t    msg_len;
    char        buff[LPJS_MSG_LEN_MAX + 1];
    
    // Append message after length bytes, which we insert later
    va_start(ap, format);
    status = vsnprintf(buff + sizeof(uint32_t),
		       LPJS_MSG_LEN_MAX + 1 - sizeof(uint32_t), format, ap);

    // vsnprintf() returns the length the the string would have been
    // if buff were unlimited, so we have to use strlen.
    // Also send '\0' byte to mark end of message
    msg_len = strlen(buff + sizeof(uint32_t)) + 1;
    // lpjs_log("buff = %s\n", buff + sizeof(uint32_t));
    // lpjs_log("%s(): msg_len = %" PRId32 "\n", __FUNCTION__, msg_len);
    
    // Prefix message with length in binary format, network byte-order
    *(uint32_t *)buff = htonl(msg_len);
   
    /*
     *  Send length and message in one send() to ensure they're
     *  received together, not interleaved with unrelated messages
     */
    send(msg_fd, buff, sizeof(uint32_t) + msg_len, send_flags);
    va_end(ap);
    
    return status;
}


/***************************************************************************
 *  Description:
 *      Receive a message sent by lpjs_send().  A uint32_t containing
 *      the message length in network byte order is received first,
 *      followed by the message.  The interface is idential to recv(2).
 *
 *  timeout:    timeout in microseconds
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-19  Jason Bacon Begin
 ***************************************************************************/

ssize_t lpjs_recv(int msg_fd, char *buff, size_t buff_len, int flags,
		  int timeout)

{
    uint32_t    msg_len;
    ssize_t     bytes_read;
    fd_set      read_fds;
    struct timeval  timeout_tv = { timeout / 1000000, timeout % 1000000 };
    
    // FIXME: Use poll() instead?
    // Use select() to implement timeout without using non-blocking fds
    if ( timeout != 0 )
    {
	// FIXME: select() will return if a lower fd is ready
	FD_ZERO(&read_fds);
	FD_SET(msg_fd, &read_fds);
	// lpjs_log("%s: Entering select()...\n", __FUNCTION__);
	if ( select(msg_fd + 1, &read_fds, NULL, NULL, &timeout_tv) == 0 )
	{
	    lpjs_log("%s(): select() timed out after %dus.\n",
		     __FUNCTION__, timeout);
	    return LPJS_RECV_TIMEOUT;
	}
    }

    // lpjs_log("Receiving message...\n");
    bytes_read = recv(msg_fd, &msg_len, sizeof(uint32_t), flags | MSG_WAITALL);
    if ( bytes_read == 0 )
	// Not a timeout, just got nothing
	// Should never happen on blocking reads
	return 0;
    else if ( bytes_read == -1 )
    {
	lpjs_log("lpjs_recv(): recv() returned -1: %s\n", strerror(errno));
	return LPJS_RECV_FAILED;
    }
    else if ( bytes_read != sizeof(uint32_t) )
    {
	lpjs_log("lpjs_recv(): Read partial msg_len: %zd bytes.\n",
		bytes_read);
	exit(EX_DATAERR);
    }
    msg_len = ntohl(msg_len);
    
    if ( msg_len > buff_len )
    {
	lpjs_log("lpjs_recv(): msg_len > buff_len -1.\n");
	lpjs_log("This is a software bug.\n");
	return 0;
    }
    
    bytes_read = recv(msg_fd, buff, msg_len, flags | MSG_WAITALL);
    
    return bytes_read;
}


/***************************************************************************
 *  Description:
 *      Receive a message sent by lpjs_send().  A uint32_t containing
 *      the message length in network byte order is received first,
 *      followed by the message.  The interface is idential to recv(2).
 *
 *  Arguments:
 *      msg_fd          Socket file descriptor
 *      payload         Buffer to receive message
 *      flags           See recv(2)
 *      timeout         useconds, passed to lpjs_recv
 *      uid, gid        Owner of sending process
 *      close_function  Different close procedures for dispatch and compd
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-19  Jason Bacon Begin
 ***************************************************************************/

ssize_t lpjs_recv_munge(int msg_fd, char **payload, int flags, int timeout,
			uid_t *uid, gid_t *gid, int(*close_function)(int))

{
    ssize_t     bytes_read;
    int         payload_len;
    munge_err_t munge_status;
    char        incoming_msg[LPJS_MSG_LEN_MAX + 1];
    
    bytes_read = lpjs_recv(msg_fd, incoming_msg, LPJS_MSG_LEN_MAX + 1,
			   flags, timeout);
    
    if ( bytes_read == LPJS_RECV_FAILED )
    {
	lpjs_log("%s(): lpjs_recv() failed: %s", __FUNCTION__, strerror(errno));
	return LPJS_RECV_FAILED;
    }
    else if ( bytes_read == LPJS_RECV_TIMEOUT )
	return LPJS_RECV_TIMEOUT;
    else if ( bytes_read > 0 )
    {
	munge_status = munge_decode(incoming_msg, NULL, (void **)payload,
				    &payload_len, uid, gid);
	if ( munge_status != EMUNGE_SUCCESS )
	{
	    close_function(msg_fd);
	    lpjs_log("%s(): munge_decode() failed.  %zd bytes, Error = %s\n",
		     __FUNCTION__, bytes_read, munge_strerror(munge_status));
	    return -1;  // FIXME: Define return codes
	}
	
	// Acknolwedge successful receipt of message
	lpjs_send(msg_fd, 0, LPJS_MUNGE_CRED_VERIFIED);
	return payload_len;
    }
    else
    {
	lpjs_log("%s(): Internal error: Undefined return code from lpjs_recv(): %d\n",
		 __FUNCTION__, bytes_read);
	// FIXME: What should we really do here?
	return LPJS_RECV_FAILED;
    }
}


/***************************************************************************
 *  Description:
 *      Send a munge-encoded message
 *
 *  Returns:
 *      EX_OK on success, various other error codes
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-21  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_send_munge(int msg_fd, const char *msg, int(*close_function)(int))

{
    char        *cred,
		incoming_msg[LPJS_MSG_LEN_MAX + 1];
    ssize_t     bytes;
    munge_err_t munge_status;
    
    if ( (munge_status = munge_encode(&cred, NULL, msg, strlen(msg))) != EMUNGE_SUCCESS )
    {
	lpjs_log("lpjs_compd: munge_encode() failed.\n");
	lpjs_log("Return code = %s\n", munge_strerror(munge_status));
	// May be close(), lpjs_dispatchd_safe_close(), or lpjs_no_close()
	close_function(msg_fd);
	return LPJS_MUNGE_FAILED;
    }

    // printf("Sending %zd bytes: %s...\n", strlen(cred), cred);
    if ( lpjs_send(msg_fd, 0, cred) < 0 )
    {
	lpjs_log("%s(): Failed to send credential to dispatchd", __FUNCTION__);
	// May be close(), lpjs_dispatchd_safe_close(), or lpjs_no_close()
	close_function(msg_fd);
	free(cred);
	return LPJS_SEND_FAILED;
    }
    free(cred);
    
    // Read acknowledgment
    bytes = lpjs_recv(msg_fd, incoming_msg, LPJS_MSG_LEN_MAX, 0, 0);
    if ( bytes == LPJS_RECV_FAILED )
    {
	lpjs_log("%s(): lpjs_recv() failed.\n", __FUNCTION__);
	return LPJS_RECV_FAILED;
    }
    else if ( bytes == LPJS_RECV_TIMEOUT )
    {
	lpjs_log("%s(): lpjs_recv() timeout.\n", __FUNCTION__);
	return LPJS_RECV_TIMEOUT;
    }
    if ( (bytes < 1) || (strcmp(incoming_msg, LPJS_MUNGE_CRED_VERIFIED) != 0) )
    {
	lpjs_log("%s(): Expected %s, got %zd bytes on fd %d.\n", __FUNCTION__,
		 LPJS_MUNGE_CRED_VERIFIED, bytes, msg_fd);
	return LPJS_RECV_FAILED;
    }

    return LPJS_MSG_SENT;
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

int     lpjs_dispatchd_safe_close(int msg_fd)

{
    char    buff[64];
    
    /*
     *  Client must be looking for the EOT character at the end of
     *  a read, or this is useless.  If this fails, closing msg_fd
     *  will cause restart of dispatchd to fail with "address already in use"
     */
    
    lpjs_log("%s(): Sending EOT.  No response expected.\n", __FUNCTION__);
    if ( lpjs_send_munge(msg_fd, LPJS_EOT_MSG,
			 lpjs_no_close) == LPJS_MSG_SENT )
    {
	/*
	 *  Wait until EOF is signaled due to the other end being closed.
	 *  FIXME: No data should be read here.  The first read() should
	 *  return EOF.  Add a check for this.
	 */
	lpjs_log("%s(): Waiting for client to hang up...\n", __FUNCTION__);
	while ( read(msg_fd, buff, 64) > 0 )
	    sleep(1);
    }
    
    // lpjs_log(Log_stream, "Closing msg_fd.\n");
    return close(msg_fd);
}


int     lpjs_no_close(int fd)

{
    return 0;
}
