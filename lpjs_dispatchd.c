/***************************************************************************
 *  Description:
 *      This is the main controller daemon that runs on the head node.
 *      It listens for socket connections and takes requests for information
 *      from lpjs-nodes (for node status), lpjs-jobs (for job status),
 *      job submissions from lpjs-submit, and job completion reports from
 *      lpjs-chaperone.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-23  Jason Bacon Begin
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <munge.h>
#include <stdbool.h>
#include <xtend/proc.h>
#include "lpjs.h"
#include "node-list.h"
#include "job-list.h"
#include "config.h"
#include "scheduler.h"
#include "network.h"
#include "misc.h"

int     main(int argc,char *argv[])

{
    node_list_t node_list;
    job_list_t  job_list;
    extern FILE    *Log_stream; // Must be global for signal handler
    
    if ( argc > 2 )
    {
	fprintf (stderr, "Usage: %s [--daemonize]\n", argv[0]);
	return EX_USAGE;
    }
    else if ( (argc == 2) && (strcmp(argv[1],"--daemonize") == 0 ) )
    {
	/*
	 *  Code run after this must not attempt to write to stdout or stderr
	 *  since they will be closed.  Use lpjs_log() for all informative
	 *  messages.
	 */
	Log_stream = fopen("/var/log/lpjs_dispatchd", "a");
	if ( Log_stream == NULL )
	{
	    perror("Cannot open /var/log/lpjs_dispatchd");
	    return EX_CANTCREAT;
	}
	xt_daemonize(0, 0);
    }
    else
	Log_stream = stderr;
    
    lpjs_load_config(&node_list, LPJS_CONFIG_ALL, Log_stream);
    job_list_init(&job_list);

    /*
     *  bind(): address already in use during testing with frequent restarts.
     *  Best approach is to ensure that client completes a close
     *  before the server closes.
     *  https://hea-www.harvard.edu/~fine/Tech/addrinuse.html
     *  Copy saved in ./bind-address-already-in-use.pdf
     */
    signal(SIGINT, terminate_handler);
    signal(SIGTERM, terminate_handler);

    return process_events(&node_list, &job_list);
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

int     server_safe_close(int msg_fd)

{
    char    buff[64];
    
    /*
     *  Client must be looking for the EOT character at the end of
     *  a read, or this is useless.
     */
    send_eot(msg_fd);
    
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


/***************************************************************************
 *  Description:
 *      Listen for messages on LPJS_TCP_PORT and respond with either info
 *      (lpjs-nodes, lpjs-jobs, etc.) or actions (lpjs-submit).
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-25  Jason Bacon Begin
 ***************************************************************************/

int     process_events(node_list_t *node_list, job_list_t *job_list)

{
    ssize_t     bytes;
    socklen_t   address_len = sizeof (struct sockaddr_in);
    char        incoming_msg[LPJS_IP_MSG_MAX + 1];
    struct sockaddr_in server_address = { 0 };  // FIXME: Support IPV6
    uid_t       uid;
    gid_t       gid;
    munge_err_t munge_status;
    node_t      new_node;
    int         listen_fd, msg_fd;

    /*
     *  Step 1: Create a socket for listening for new connections.
     */
    
    /*
     *  Create a socket endpoint to pair with the endpoint on the client.
     *  This only creates a file descriptor.  It is not yet bound to
     *  any network interface and port.
     *  AF_INET and PF_INET have the same value, but PF_INET is more
     *  correct according to BSD and Linux man pages, which indicate
     *  that a protocol family should be specified.  In theory, a
     *  protocol family can support more than one address family.
     *  SOCK_STREAM indicates a reliable stream oriented protocol,
     *  such as TCP, vs. unreliable unordered datagram protocols like UDP.
     */
    if ((listen_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
	lpjs_log("Error opening listener socket.\n");
	return EX_UNAVAILABLE;
    }
    lpjs_log("listen_fd = %d\n", listen_fd);

    /*
     *  Port on which to listen for new connections from compute nodes.
     *  Convert 16-bit port number from host byte order to network byte order.
     */
    server_address.sin_port = htons(LPJS_IP_TCP_PORT);
    
    // AF_INET = inet4, AF_INET6 = inet6
    server_address.sin_family = AF_INET;
    
    /*
     *  Listen on all local network interfaces for now (INADDR_ANY).
     *  We may allow the user to specify binding to a specific IP address
     *  in the future, for multihomed servers acting as gateways, etc.
     *  Convert 32-bit host address to network byte order.
     */
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind socket fd and server address
    while ( bind(listen_fd, (struct sockaddr *) &server_address,
	      sizeof (server_address)) < 0 )
    {
	lpjs_log("bind() failed: %s: ", strerror(errno));
	lpjs_log("Retry in 10 seconds...\n");
	sleep(10);
    }
    lpjs_log("Bound to port %d...\n", LPJS_IP_TCP_PORT);
    
    /*
     *  Create queue for incoming connection requests
     */
    if (listen(listen_fd, LPJS_IP_MSG_QUEUE_MAX) != 0)
    {
	lpjs_log("listen() failed.\n");
	return EX_UNAVAILABLE;
    }

    /*
     *  Step 2: Accept new connections, and create a separate socket
     *  for communication with each new compute node.
     */
    
    // FIXME: Combine listen_fd and all client socket fds into
    // one fd_set and use select() to process events.
    // User commands may also connect from the head node or any other
    // node
    
    while ( true )
    {
	fd_set  read_fds;
	int     nfds;
	
	FD_ZERO(&read_fds);
	FD_SET(listen_fd, &read_fds);
	nfds = listen_fd + 1;   // FIXME: Use highest fd + 1
	
	// FIXME: Verify that compute nodes are alive at regular intervals
	// and update status accordingly.
	// Ping nodes from here or expect pings from compute nodes?
	// The latter would seem to fit with using select() here.
	// ping_all_nodes();
	
	if ( select(nfds, &read_fds, NULL, NULL, LPJS_NO_SELECT_TIMEOUT) > 0 )
	{
	    // Top priority: Active compute nodes (move existing jobs along)
	    // Second priority: New compute node checkins (make resources available)
	    // Lowest priority: User commands
	    
	    if ( FD_ISSET(listen_fd, &read_fds) )
	    {
		// FIXME: Only accept requests from designated cluster nodes
		/* Accept a connection request */
		if ((msg_fd = accept(listen_fd,
			(struct sockaddr *)&server_address, &address_len)) == -1)
		{
		    lpjs_log("accept() failed, even though select indicated listen_fd.\n");
		    exit(EX_SOFTWARE);
		}
		else
		{
		    puts("Accepted new connection.");
		    lpjs_log("Accepted connection. fd = %d\n", msg_fd);
	
		    // FIXME: Use poll() to detect lost connections?
		    
		    /* Read a message through the socket */
		    while ( (bytes = recv(msg_fd, incoming_msg, 100, 0)) < 1 )
		    {
			lpjs_log("recv() failed: %s", strerror(errno));
			sleep(1);
		    }
	    
		    /* Process request */
		    // FIXME: Check for duplicate checkins.  We should not get
		    // a checkin request while one is already open
		    if ( memcmp(incoming_msg, "compd-checkin", 13) == 0 )
		    {
			lpjs_log("msg = %s\n", incoming_msg);
			// Debug
			// lpjs_log(Log_stream, "compd checkin.\n");
			
			// sleep(5);
			
			/* Get munge credential */
			// FIXME: What is the maximum cred length?
			while ( (bytes = recv(msg_fd, incoming_msg, 4096, 0)) < 1 )
			{
			    lpjs_log("recv() failed: %s", strerror(errno));
			    puts("Waiting for checkin data...");
			    sleep(1);
			}
			lpjs_log("Munge credential message length = %zd\n", bytes);
			
			munge_status = munge_decode(incoming_msg, NULL, NULL, 0, &uid, &gid);
			if ( munge_status != EMUNGE_SUCCESS )
			    lpjs_log("munge_decode() failed.  Error = %s\n",
				     munge_strerror(munge_status));
			lpjs_log("Checkin from uid %d, gid %d\n", uid, gid);
			
			// FIXME: Only accept compd checkins from root
	    
			/*
			 *  Get specs from node and add msg_fd
			 */
			
			// Debug
			send_msg(msg_fd, "Ident verified.\n");
			
			node_receive_specs(&new_node, msg_fd);
			lpjs_log("Back from node_receive_specs().\n");
			
			// Keep in sync with node_list_send_status()
			printf(NODE_STATUS_HEADER_FORMAT, "Hostname", "State",
			    "Cores", "Used", "Physmem", "Used", "OS", "Arch");
			node_print_status(&new_node);
			node_list_update_compute(node_list, &new_node);
			node_set_msg_fd(&new_node, msg_fd);
			
			puts("Done adding node.");
			// FIXME: Acknowledge checkin
			
			// Do not close msg_fd. Used to communicate with
			// lpjs_compd processes.
		    }
		    else if ( strcmp(incoming_msg, "nodes") == 0 )
		    {
			lpjs_log("Request for node status.\n");
			node_list_send_status(msg_fd, node_list);
			server_safe_close(msg_fd);
		    }
		    else if ( strcmp(incoming_msg, "jobs") == 0 )
		    {
			lpjs_log("Request for job status.\n");
			job_list_send_params(msg_fd, job_list);
			server_safe_close(msg_fd);
		    }
		    else if ( memcmp(incoming_msg, "submit", 6) == 0 )
		    {
			// FIXME: Don't accept job submissions from root until
			// security issues have been considered
			
			lpjs_log("Request for job submission.\n");
			
			/* Get munge credential */
			// FIXME: What is the maximum cred length?
			if ( (bytes = recv(msg_fd, incoming_msg, 4096, 0)) == - 1)
			{
			    lpjs_log("recv() failed: %s", strerror(errno));
			    return EX_IOERR;
			}
			munge_status = munge_decode(incoming_msg, NULL, NULL, 0, &uid, &gid);
			if ( munge_status != EMUNGE_SUCCESS )
			    lpjs_log("munge_decode() failed.  Error = %s\n",
				     munge_strerror(munge_status));
			lpjs_log("Submit from %d, %d\n", uid, gid);
			queue_job(msg_fd, incoming_msg, node_list);
			server_safe_close(msg_fd);
		    }
		    
		    // FIXME: This probably shouldn't come in on listen_fd,
		    // but on one of the compd sockets
		    else if ( memcmp(incoming_msg, "job-complete", 12) == 0 )
		    {
			lpjs_log("Job completion report.\n");
			log_job(incoming_msg);
		    }
		}
	    }
	    
	    // Now check all socket fds returned by accept, i.e.
	    // compute node and user command messages.
	    
	}
	else
	    lpjs_log("select() returned 0.\n");
    }
    close(listen_fd);
    return EX_OK;
}


/***************************************************************************
 *  Description:
 *      Record job info such as command, exit status, run time, etc.
 *      Incoming message is sent by lpjs-chaperone when its child
 *      (a dispatched computational process) terminates.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

void    log_job(const char *incoming_msg)

{
    // FIXME: Add extensive info about the job
    lpjs_log(incoming_msg);
}
