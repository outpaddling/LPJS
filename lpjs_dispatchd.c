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
#include "lpjs_dispatchd.h"

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
    signal(SIGINT, lpjs_terminate_handler);
    signal(SIGTERM, lpjs_terminate_handler);

    return process_events(&node_list, &job_list);
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
    char        incoming_msg[LPJS_MSG_LEN_MAX + 1];
    struct sockaddr_in server_address = { 0 };
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
	lpjs_log("bind() failed: %s\n", strerror(errno));
	lpjs_log("Retry in 10 seconds...\n");
	sleep(10);
    }
    lpjs_log("Bound to port %d...\n", LPJS_IP_TCP_PORT);
    
    /*
     *  Create queue for incoming connection requests
     */
    if (listen(listen_fd, LPJS_CONNECTION_QUEUE_MAX) != 0)
    {
	lpjs_log("listen() failed.\n");
	return EX_UNAVAILABLE;
    }

    /*
     *  Step 2: Accept new connections, and create a separate socket
     *  for communication with each new compute node.
     */
    
    while ( true )
    {
	fd_set  read_fds;
	int     nfds, highest_fd;
	
	FD_ZERO(&read_fds);
	FD_SET(listen_fd, &read_fds);
	highest_fd = listen_fd;
	for (unsigned c = 0; c < NODE_LIST_COUNT(node_list); ++c)
	{
	    node_t *node = &NODE_LIST_COMPUTE_NODES_AE(node_list, c);
	    //lpjs_log("Checking node %s, fd = %d...\n",
	    //        NODE_HOSTNAME(node), NODE_MSG_FD(node));
	    if ( NODE_MSG_FD(node) != NODE_MSG_FD_NOT_OPEN )
	    {
		lpjs_log("Adding fd %d, node %s to fd set.\n",
			NODE_MSG_FD(node),
			NODE_HOSTNAME(node));
		FD_SET(NODE_MSG_FD(node), &read_fds);
		if ( NODE_MSG_FD(node) > highest_fd )
		    highest_fd = NODE_MSG_FD(node);
	    }
	}
	lpjs_log("highest_fd = %d\n", highest_fd);
	nfds = highest_fd + 1;
	
	if ( select(nfds, &read_fds, NULL, NULL, LPJS_NO_SELECT_TIMEOUT) > 0 )
	{
	    lpjs_log("Back from select.\n");
	    
	    // Top priority: Active compute nodes (move existing jobs along)
	    // Second priority: New compute node checkins (make resources available)
	    // Lowest priority: User commands
	    for (unsigned c = 0; c < NODE_LIST_COUNT(node_list); ++c)
	    {
		node_t *node = &NODE_LIST_COMPUTE_NODES_AE(node_list, c);
		int     fd = NODE_MSG_FD(node);
		if ( (fd != NODE_MSG_FD_NOT_OPEN) && FD_ISSET(fd, &read_fds) )
		{
		    lpjs_log("Activity on fd %d\n", fd);
		    
		    /*
		     *  select() returns when a peer has closed the connection.
		     *  lpjs_recv_msg() will return 0 in this case.
		     */
		    
		    bytes = lpjs_recv_msg(fd, incoming_msg, LPJS_MSG_LEN_MAX, 0);
		    if ( bytes == 0 )
		    {
			lpjs_log("Lost connection to %s.  Closing...\n",
				NODE_HOSTNAME(node));
			close(fd);
			node_set_msg_fd(node, NODE_MSG_FD_NOT_OPEN);
			node_set_state(node, "Down");
		    }
		    else
		    {
			switch(incoming_msg[0])
			{
			    case    LPJS_NOTICE_JOB_COMPLETE:
				lpjs_log("Job completion report.\n");
				lpjs_log_job(incoming_msg);
				break;
				
			    default:
				lpjs_log("Invalid notification on fd %d: %d\n",
					fd, incoming_msg[0]);
			}
		    }
		}
	    }
	    
	    if ( FD_ISSET(listen_fd, &read_fds) )
	    {
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
	
		    /* Read a message through the socket */
		    while ( (bytes = lpjs_recv_msg(msg_fd,
				 incoming_msg, LPJS_MSG_LEN_MAX, 0)) < 1 )
		    {
			lpjs_log("lpjs_recv_msg() failed: %s", strerror(errno));
			sleep(1);
		    }
	    
		    /* Process request */
		    switch(incoming_msg[0])
		    {
			case    LPJS_REQUEST_COMPD_CHECKIN:
			    // FIXME: Check for duplicate checkins.  We should not get
			    // a checkin request while one is already open
			    lpjs_log("compd-checkin requested.\n");
			    // lpjs_log(Log_stream, "compd checkin.\n");
			    
			    /* Get munge credential */
			    // FIXME: What is the maximum cred length?
			    if ( (bytes = lpjs_recv_msg(msg_fd, incoming_msg, LPJS_MSG_LEN_MAX, 0)) < 1 )
			    {
				lpjs_server_safe_close(msg_fd);
				lpjs_log("Failed to read munge credential: %s", strerror(errno));
			    }
			    else
			    {
				lpjs_log("Munge credential message length = %zd\n", bytes);
				lpjs_log("munge msg: %s\n", incoming_msg);
				
				munge_status = munge_decode(incoming_msg, NULL, NULL, 0, &uid, &gid);
				if ( munge_status != EMUNGE_SUCCESS )
				{
				    lpjs_server_safe_close(msg_fd);
				    lpjs_log("munge_decode() failed.  Error = %s\n",
					     munge_strerror(munge_status));
				}
				else
				{
				    lpjs_log("Checkin from uid %d, gid %d\n", uid, gid);
				    
				    // FIXME: Only accept compd checkins from root
			
				    /*
				     *  Get specs from node and add msg_fd
				     */
				    
				    lpjs_send_msg(msg_fd, 0, "Munge credentials verified");
				    
				    node_recv_specs(&new_node, msg_fd);
				    
				    // Keep in sync with node_list_send_status()
				    printf(NODE_STATUS_HEADER_FORMAT, "Hostname", "State",
					"Cores", "Used", "Physmem", "Used", "OS", "Arch");
				    node_print_status(&new_node);
				    
				    // Make sure node name is valid
				    // Note: For real security, only authorized
				    // nodes should be allowed to pass through
				    // the network firewall.
				    bool valid_node = false;
				    for (unsigned c = 0; c < NODE_LIST_COUNT(node_list); ++c)
				    {
					node_t *node = &NODE_LIST_COMPUTE_NODES_AE(node_list, c);
					// If config has short hostnames, just match that
					int valid_hostname_len = strlen(NODE_HOSTNAME(node));
					if ( memcmp(NODE_HOSTNAME(node), NODE_HOSTNAME(&new_node), valid_hostname_len) == 0 )
					    valid_node = true;
				    }
				    if ( ! valid_node )
				    {
					lpjs_log("Unauthorized checkin request from %s.\n", NODE_HOSTNAME(&new_node));
					close(msg_fd);
				    }
				    else
				    {
					lpjs_send_msg(msg_fd, 0, "Node authorized");
					node_set_msg_fd(&new_node, msg_fd);
					
					// Nodes were added to node_list by lpjs_load_config()
					// Just update the fields here
					node_list_update_compute(node_list, &new_node);
					
					puts("Done adding node.");
					// FIXME: Acknowledge checkin
				    }
				}
			    }
			    break;
			
			case    LPJS_REQUEST_NODE_STATUS:
			    lpjs_log("Request for node status.\n");
			    node_list_send_status(msg_fd, node_list);
			    lpjs_server_safe_close(msg_fd);
			    break;
			
			case    LPJS_REQUEST_JOB_STATUS:
			    lpjs_log("Request for job status.\n");
			    job_list_send_params(msg_fd, job_list);
			    lpjs_server_safe_close(msg_fd);
			    break;
			
			case    LPJS_REQUEST_SUBMIT:
			    // FIXME: Don't accept job submissions from root until
			    // security issues have been considered
			    
			    lpjs_log("Request for job submission.\n");
			    
			    /* Get munge credential */
			    // FIXME: What is the maximum cred length?
			    if ( (bytes = lpjs_recv_msg(msg_fd, incoming_msg, 4096, 0)) == - 1)
			    {
				lpjs_log("lpjs_recv_msg() failed: %s", strerror(errno));
				return EX_IOERR;
			    }
			    munge_status = munge_decode(incoming_msg, NULL, NULL, 0, &uid, &gid);
			    if ( munge_status != EMUNGE_SUCCESS )
				lpjs_log("munge_decode() failed.  Error = %s\n",
					 munge_strerror(munge_status));
			    lpjs_log("Submit from %d, %d\n", uid, gid);
			    lpjs_queue_job(msg_fd, incoming_msg, node_list);
			    lpjs_server_safe_close(msg_fd);
			    break;
			
			default:
			    lpjs_log("Invalid request on listen_fd: %d\n",
				    incoming_msg[0]);
			    
		    }   // switch
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

void    lpjs_log_job(const char *incoming_msg)

{
    // FIXME: Add extensive info about the job
    lpjs_log(incoming_msg);
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
