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
#include <xtend/file.h>     // xt_rmkdir()

#include "lpjs.h"
#include "node-list.h"
#include "config.h"
#include "network.h"
#include "misc.h"
#include "lpjs_compd.h"

int     main (int argc, char *argv[])

{
    node_list_t *node_list = node_list_new();
    node_t      *node = node_new(); // FIXME: Does this new to be allocated?
    char        *munge_payload,
		vis_msg[LPJS_MSG_LEN_MAX + 1];
    ssize_t     bytes;
    int         msg_fd;
    struct pollfd   poll_fd;
    extern FILE *Log_stream;
    uid_t       uid;
    gid_t       gid;

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
    
#ifdef __linux__    // systemd needs a pid file for forking daemons
    // FIXME: Make sure Pid_path is removed no matter where the program exits
    int     status;
    extern char Pid_path[PATH_MAX + 1];
    
    if ( xt_rmkdir(LPJS_RUN_DIR, 0755) != 0 )
	return EX_CANTCREAT;
    
    snprintf(Pid_path, PATH_MAX + 1, "%s/%s.pid", LPJS_RUN_DIR, "lpjs_compd");
    status = xt_create_pid_file(Pid_path, Log_stream);
    if ( status != EX_OK )
	return status;
#endif

    // Get hostname of head node
    lpjs_load_config(node_list, LPJS_CONFIG_HEAD_ONLY, Log_stream);

    msg_fd = lpjs_checkin_loop(node_list, node);
    poll_fd.fd = msg_fd;
    // POLLERR and POLLHUP are actually always set.  Listing POLLHUP here just
    // for documentation.
    poll_fd.events = POLLIN | POLLHUP;
    
    // Now keep daemon running, awaiting jobs
    // Almost correct: https://unix.stackexchange.com/questions/581426/how-to-get-notified-when-the-other-end-of-a-socketpair-is-closed
    while ( true )
    {
	poll(&poll_fd, 1, 2000);
	
	// FIXME: Send regular pings to lpjs_dispatchd?
	// Or monitor compd daemons with a separate process that
	// sends events to dispatchd?
	
	if (poll_fd.revents & POLLHUP)
	{
	    poll_fd.revents &= ~POLLHUP;
	    
	    // Close this end, or dispatchd gets "address already in use"
	    // When trying to restart
	    close(msg_fd);
	    lpjs_log("Lost connection to dispatchd: HUP received.\n");
	    sleep(LPJS_RETRY_TIME);  // No point trying immediately after drop
	    msg_fd = lpjs_checkin_loop(node_list, node);
	}
	
	if (poll_fd.revents & POLLERR)
	{
	    poll_fd.revents &= ~POLLERR;
	    lpjs_log("Error occurred polling dispatchd: %s\n", strerror(errno));
	    break;
	}
	
	if (poll_fd.revents & POLLIN)
	{
	    poll_fd.revents &= ~POLLIN;
	    bytes = lpjs_recv_munge(msg_fd, &munge_payload, 0, 0, &uid, &gid);
	    munge_payload[bytes] = '\0';
	    xt_strviscpy((unsigned char *)vis_msg,
			 (unsigned char *)munge_payload, LPJS_MSG_LEN_MAX + 1);
	    lpjs_log("Received %zd bytes from dispatchd: \"%s\"\n",
		     bytes, vis_msg);
	    
	    if ( bytes == 0 )
	    {
		/*
		 *  Likely lost connection due to crash or other ungraceful
		 *  event.  Close connection so that dispatchd doesn't hang
		 *  with "address already in use".
		 */
		close(msg_fd);
		lpjs_log("%s(): Error reading from dispatchd.  Disconnecting...\n",
			__FUNCTION__);
		poll_fd.revents = 0;
		msg_fd = lpjs_checkin_loop(node_list, node);
	    }
	    else if ( munge_payload[0] == LPJS_EOT )
	    {
		// Close this end, or dispatchd gets "address already in use"
		// When trying to restart
		close(msg_fd);
		lpjs_log("Lost connection to dispatchd: EOT received.\n");
		sleep(LPJS_RETRY_TIME);  // No point trying immediately after drop

		// Ignore HUP that follows EOT
		// FIXME: This might be bad timing
		poll_fd.revents &= ~POLLHUP;
		msg_fd = lpjs_checkin_loop(node_list, node);
	    }
	    else if ( munge_payload[0] == LPJS_COMPD_REQUEST_NEW_JOB )
	    {
		/*
		 *  Pasrse job specs
		 */
		
		/*
		 *  Save script
		 */
		
		/*
		 *  Update node status (keep a copy here in case
		 *  dispatchd is restarted)
		 */
		
		/*
		 *  Run script under chaperone
		 */
	    }
	    free(munge_payload);
	}
    }

    close(msg_fd);
    return EX_IOERR;
}


int     lpjs_compd_checkin(int msg_fd, node_t *node)

{
    char        outgoing_msg[LPJS_MSG_LEN_MAX + 1],
		incoming_msg[LPJS_MSG_LEN_MAX + 1],
		specs[NODE_SPECS_LEN + 1];
    extern FILE *Log_stream;
    
    /* Send a message to the server */
    /* Need to send \0, so xt_dprintf() doesn't work here */
    node_detect_specs(node);
    snprintf(outgoing_msg, LPJS_MSG_LEN_MAX + 1,
	    "%c%s", LPJS_REQUEST_COMPD_CHECKIN,
	    node_specs_to_str(node, specs, NODE_SPECS_LEN + 1));
    lpjs_log("%s(): Sending node specs:\n", __FUNCTION__);
    node_print_specs_header(Log_stream);
    fprintf(Log_stream, "%s\n", outgoing_msg + 1);
    if ( lpjs_send_munge(msg_fd, outgoing_msg) != EX_OK )
    {
	lpjs_log("lpjs_compd: Failed to send checkin message to dispatchd: %s",
		strerror(errno));
	close(msg_fd);
	return EX_IOERR;
    }
    lpjs_log("%s(): Sent checkin request.\n", __FUNCTION__);

    // FIXME: Just sending a credential with no payload for now, to
    // authenticate the socket connection.  Not sure if we should worry
    // about a connection-oriented socket getting hijacked and
    // munge other communication as well.
    // if ( lpjs_send_munge(msg_fd, NULL) != EX_OK )
    //     return EX_DATAERR;

    lpjs_recv(msg_fd, incoming_msg, LPJS_MSG_LEN_MAX, 0, 0);
    if ( strcmp(incoming_msg, "Node authorized") != 0 )
    {
	lpjs_log("%s(): This node is not authorized to connect.\n"
		 "It must be added to the etc/lpjs/config on the head node.\n",
		 __FUNCTION__);
	exit(EX_NOPERM);
    }
    else
	lpjs_log("%s(): Received authorization from lpjs_dispatchd.\n",
		__FUNCTION__);

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

int     lpjs_checkin_loop(node_list_t *node_list, node_t *node)

{
    int     msg_fd,
	    status;
    
    // Retry socket connection indefinitely
    while ( (msg_fd = lpjs_connect_to_dispatchd(node_list)) == -1 )
    {
	lpjs_log("%s(): Failed to connect to dispatchd: %s\n",
		__FUNCTION__, strerror(errno));
	lpjs_log("Retry in %d seconds...\n", LPJS_RETRY_TIME);
	sleep(LPJS_RETRY_TIME);
    }
    
    // Retry checking request indefinitely
    while ( (status = lpjs_compd_checkin(msg_fd, node)) != EX_OK )
    {
	lpjs_log("%s(): compd-checkin failed.  Retry in %d seconds...\n",
		 __FUNCTION__, LPJS_RETRY_TIME);
	sleep(LPJS_RETRY_TIME);
    }
    
    lpjs_log("%s(): Checkin successful.\n", __FUNCTION__);
    
    return msg_fd;
}
