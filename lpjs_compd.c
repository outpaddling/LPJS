/***************************************************************************
 *  Description:
 *      LPJS compute node daemon.  Checks in with lpfs-dispatchd to signal
 *      that node is up and starts computational processes on compute nodes.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-30  Jason Bacon Begin
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <poll.h>
#include <stdbool.h>

#include <xtend/string.h>
#include <xtend/proc.h>

#include "lpjs.h"
#include "node-list.h"
#include "config.h"
#include "network.h"
#include "misc.h"
#include "lpjs_compd.h"

int     main (int argc, char *argv[])

{
    node_list_t node_list;
    node_t      node;
    char        incoming_msg[LPJS_MSG_LEN_MAX + 1];
    ssize_t     bytes;
    int         msg_fd;
    struct pollfd   poll_fd;
    extern FILE *Log_stream;

    if ( argc > 2 )
    {
	fprintf (stderr, "Usage: %s [--daemonize|--log-output]\n", argv[0]);
	return EX_USAGE;
    }
    else if ( (argc == 2) && (strcmp(argv[1],"--daemonize") == 0 ) )
    {
	if ( (Log_stream = lpjs_log_output(LPJS_COMPD_LOG)) == NULL )
	    return EX_CANTCREAT;

	/*
	 *  Code run after this must not attempt to write to stdout or stderr
	 *  since they will be closed.  Use lpjs_log() for all informative
	 *  messages.
	 *  FIXME: Prevent unchecked log growth
	 */
	xt_daemonize(0, 0);
    }
    else if ( (argc == 2) && (strcmp(argv[1],"--log-output") == 0 ) )
    {
	if ( (Log_stream = lpjs_log_output(LPJS_COMPD_LOG)) == NULL )
	    return EX_CANTCREAT;
    }
    else
	Log_stream = stderr;
    
    // Get hostname of head node
    lpjs_load_config(&node_list, LPJS_CONFIG_HEAD_ONLY, Log_stream);

    msg_fd = checkin(&node_list, &node);
    poll_fd.fd = msg_fd;
    poll_fd.events = POLLIN | LPJS_POLLHUP;    // POLLERR and POLLHUP always set
    
    // Now keep daemon running, awaiting jobs
    // Almost correct: https://unix.stackexchange.com/questions/581426/how-to-get-notified-when-the-other-end-of-a-socketpair-is-closed
    lpjs_log("Waiting for submissions...\n");
    while ( true )
    {
	poll(&poll_fd, 1, 2000);
	
	// FIXME: Send regular pings to lpjs_dispatchd?
	// Or monitor compd daemons with a separate process that
	// sends events to dispatchd?
	
	if (poll_fd.revents & LPJS_POLLHUP)
	{
	    // Close this end, or dispatchd gets "address already in use"
	    // When trying to restart
	    close(msg_fd);
	    lpjs_log("Lost connection to dispatchd: HUP received.\n");
	    sleep(LPJS_RETRY_TIME);  // No point trying immediately after drop
	    msg_fd = checkin(&node_list, &node);
	}
	
	if (poll_fd.revents & POLLERR) {
	    lpjs_log("Error occurred polling dispatchd: %s\n", strerror(errno));
	    break;
	}
	
	if (poll_fd.revents & POLLIN)
	{
	    bytes = lpjs_recv_msg(msg_fd, incoming_msg, LPJS_MSG_LEN_MAX, 0);
	    incoming_msg[bytes] = '\0';
	    
	    if ( incoming_msg[0] == 4 )
	    {
		// Close this end, or dispatchd gets "address already in use"
		// When trying to restart
		close(msg_fd);
		lpjs_log("Lost connection to dispatchd: EOT received.\n");
		sleep(LPJS_RETRY_TIME);  // No point trying immediately after drop
		msg_fd = checkin(&node_list, &node);
	    }
	    lpjs_log("Received from dispatchd: %s\n", incoming_msg);
	}
	poll_fd.revents = 0;
    }

    close(msg_fd);
    return EX_IOERR;
}


int     lpjs_compd_checkin(int msg_fd, node_t *node)

{
    char        outgoing_msg[LPJS_MSG_LEN_MAX + 1],
		incoming_msg[LPJS_MSG_LEN_MAX + 1];
    
    /* Send a message to the server */
    /* Need to send \0, so xt_dprintf() doesn't work here */
    outgoing_msg[0] = LPJS_REQUEST_COMPD_CHECKIN;
    outgoing_msg[1] = '\0';
    if ( lpjs_send_msg(msg_fd, 0, outgoing_msg) < 0 )
    {
	lpjs_log("lpjs_compd: Failed to send checkin message to dispatchd: %s",
		strerror(errno));
	close(msg_fd);
	return EX_IOERR;
    }
    lpjs_log("Sent checkin request.\n");

    // FIXME: Just sending a credential with no payload for now, to
    // authenticate the socket connection.  Not sure if we should worry
    // about a connection-oriented socket getting hijacked and
    // munge other communication as well.
    if ( lpjs_send_munge_msg(msg_fd, NULL) != EX_OK )
	return EX_DATAERR;

    node_detect_specs(node);
    node_send_specs(node, msg_fd);
    
    lpjs_recv_msg(msg_fd, incoming_msg, LPJS_MSG_LEN_MAX, 0);
    if ( strcmp(incoming_msg, "Node authorized") != 0 )
    {
	lpjs_log("This node is not authorized to connect.\n");
	lpjs_log("It must be added to the etc/lpjs/config on the head node.\n");
	exit(EX_NOPERM);
    }

    return EX_OK;
}



/***************************************************************************
 *  Use auto-c2man to generate a man page from this comment
 *
 *  Library:
 *      #include <>
 *      -l
 *
 *  Description:
 *  
 *  Arguments:
 *
 *  Returns:
 *
 *  Examples:
 *
 *  Files:
 *
 *  Environment
 *
 *  See also:
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-23  Jason Bacon Begin
 ***************************************************************************/

int     checkin(node_list_t *node_list, node_t *node)

{
    int     msg_fd,
	    status;
    
    while ( (msg_fd = lpjs_connect_to_dispatchd(node_list)) == -1 )
    {
	lpjs_log("lpjs_compd: Failed to reconnect to dispatchd: %s\n",
		strerror(errno));
	lpjs_log("Retry in %d seconds...\n", LPJS_RETRY_TIME);
	sleep(LPJS_RETRY_TIME);
    }
    
    while ( (status = lpjs_compd_checkin(msg_fd, node)) != EX_OK )
    {
	lpjs_log("compd-checkin failed.  Retry in %d seconds...\n", LPJS_RETRY_TIME);
	sleep(LPJS_RETRY_TIME);
    }
    lpjs_log("Checkin successful.\n");
    
    return msg_fd;
}
