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

// System headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>
#include <sys/types.h>  // inet_ntoa()
#include <arpa/inet.h>  // inet_ntoa()
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>      // open()
#include <pwd.h>        // getpwnam()
#include <grp.h>        // getgrnam()
#include <dirent.h>     // opendir(), ...

// Addons
#include <munge.h>
#include <xtend/proc.h>
#include <xtend/file.h>     // xt_rmkdir()
#include <xtend/string.h>   // strisint()

// Project headers
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
    // Terminates process if malloc() fails, no check required
    node_list_t *node_list = node_list_new();
    uid_t       daemon_uid;
    gid_t       daemon_gid;
    
    // Must be global for signal handler
    // FIXME: Maybe use ucontext to pass these to handler
    extern FILE         *Log_stream;
    extern node_list_t  *Node_list;

    Node_list = node_list;
    Log_stream = stderr;
    
    // Silence compiler warnings about initialization
    daemon_uid = getuid();
    daemon_gid = getgid();
    
    for (int arg = 1; arg < argc; ++arg)
    {
	if ( strcmp(argv[arg],"--daemonize") == 0 )
	{
	    // Redirect lpjs_log() output from stderr to file
	    if ( (Log_stream = lpjs_log_output(LPJS_DISPATCHD_LOG)) == NULL )
		return EX_CANTCREAT;
    
	    /*
	     *  Code run after this must not attempt to write to stdout or
	     *  stderr since they will be closed.  Use lpjs_log() for all
	     *  informative messages.
	     *  FIXME: Prevent unchecked log growth
	     */
	    
	    xt_daemonize(0, 0);
	}
	else if ( strcmp(argv[arg],"--log-output") == 0 )
	{
	    /*
	     *  Redirect lpjs_log() output without daemonizing via fork().
	     *  Used by some platforms for services.
	     */

	    if ( (Log_stream = lpjs_log_output(LPJS_DISPATCHD_LOG)) == NULL )
		return EX_CANTCREAT;
	}
	else if ( strcmp(argv[arg], "--user") == 0 )
	{
	    /*
	     *  Set uid under which daemon should run, instead of root.
	     *  Just determine user for now.  Create system files before
	     *  giving up root privs.
	     */
	    
	    // pw_ent points to internal static object
	    // OK since dispatchd is not multithreaded
	    struct passwd *pw_ent;
	    char *user_name = argv[++arg];
	    if ( (pw_ent = getpwnam(user_name)) == NULL )
	    {
		lpjs_log("%s(): Error: User %s does not exist.\n",
			 __FUNCTION__, user_name);
		return EX_NOUSER;
	    }
	    daemon_uid = pw_ent->pw_uid;
	}
	else if ( strcmp(argv[arg], "--group") == 0 )
	{
	    // gr_ent points to internal static object
	    // OK since dispatchd is not multithreaded
	    struct group *gr_ent;
	    char *group_name = argv[++arg];
	    if ( (gr_ent = getgrnam(group_name)) == NULL )
	    {
		lpjs_log("%s(): Error: Group %s does not exist.\n",
			__FUNCTION__, group_name);
		return EX_NOUSER;
	    }
	    daemon_gid = gr_ent->gr_gid;
	}
	else
	{
	    fprintf (stderr, "Usage: %s [--daemonize|--log-output] [--user username] [--group groupname]\n", argv[0]);
	    return EX_USAGE;
	}
    }
    
    // Make log file writable to daemon owner after root creates it
    chown(LPJS_LOG_DIR, daemon_uid, daemon_gid);
    chown(LPJS_DISPATCHD_LOG, daemon_uid, daemon_gid);
    
    // Go where daemon has write permissions, so a core can be dumped
    // in the event of a crash
    chdir(LPJS_LOG_DIR);
    
    // Parent of all new job directories
    if ( xt_rmkdir(LPJS_PENDING_DIR, 0755) != 0 )
    {
	fprintf(stderr, "Cannot create %s: %s\n", LPJS_PENDING_DIR, strerror(errno));
	return EX_CANTCREAT;
    }

    // Parent of all running job directories
    if ( xt_rmkdir(LPJS_RUNNING_DIR, 0755) != 0 )
    {
	fprintf(stderr, "Cannot create %s: %s\n", LPJS_RUNNING_DIR, strerror(errno));
	return EX_CANTCREAT;
    }
    
    // Make spool dir writable to daemon owner after root creates it
    chown(LPJS_PENDING_DIR, daemon_uid, daemon_gid);
    chown(LPJS_RUNNING_DIR, daemon_uid, daemon_gid);
    chown(LPJS_SPOOL_DIR "/next-job", daemon_uid, daemon_gid);

/*
 *  systemd needs a pid file for forking daemons.  BSD systems don't
 *  require this for rc scripts, so we don't bother with it.  PIDs
 *  are found dynamically there.
 */

#ifdef __linux__
    int         status;
    extern char Pid_path[PATH_MAX + 1];
    
    if ( xt_rmkdir(LPJS_RUN_DIR, 0755) != 0 )
	return EX_CANTCREAT;
    
    snprintf(Pid_path, PATH_MAX + 1, "%s/lpjs_dispatchd.pid", LPJS_RUN_DIR);
    status = xt_create_pid_file(Pid_path, Log_stream);
    if ( status != EX_OK )
	return status;
    chown(LPJS_RUN_DIR, daemon_uid, daemon_gid);
    chown(Pid_path, daemon_uid, daemon_gid);
#endif

    // setgid() must be done while still running as root, so do setuid() after
    if ( daemon_gid != 0 )
    {
	lpjs_log("%s(): Setting daemon_gid to %u.\n",
		__FUNCTION__, daemon_gid);
	if ( setgid(daemon_gid) != 0 )
	{
	    lpjs_log("%s(): setgid() failed: %s\n",
		    __FUNCTION__, strerror(errno));
	    return EX_NOPERM;
	}
    }
    
    if ( daemon_uid != 0 )
    {
	lpjs_log("%s(): Setting daemon_uid to %u.\n",
		__FUNCTION__, daemon_uid);
	if ( setuid(daemon_uid) != 0 )
	{
	    lpjs_log("%s(): Error: setuid() failed: %s\n",
		    __FUNCTION__, strerror(errno));
	    return EX_NOPERM;
	}
    }
    
    // Read etc/lpjs/config, created by lpjs-admin
    lpjs_load_config(node_list, LPJS_CONFIG_ALL, Log_stream);
    
    /*
     *  bind(): address already in use during testing with frequent restarts.
     *  Best approach is to ensure that client completes a close
     *  before the server closes.
     *  https://hea-www.harvard.edu/~fine/Tech/addrinuse.html
     *  Copy saved in ./bind-address-already-in-use.pdf
     *  FIXME: Does this handler actually help?  FDs are closed
     *  upon process termination anyway.
     */
    signal(SIGINT, lpjs_dispatchd_terminate_handler);
    signal(SIGTERM, lpjs_dispatchd_terminate_handler);
    
    /*
     *  dispatchd shouldn't be trying to write to broken pipes, but
     *  we don't want it to terminate due to minor bugs.
     */
    
    signal(SIGPIPE, lpjs_dispatchd_sigpipe);

    return lpjs_process_events(node_list);
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

int     lpjs_process_events(node_list_t *node_list)

{
    int                 listen_fd;
    struct sockaddr_in  server_address = { 0 };
    // job_list_new() terminates process if malloc fails, no need to check
    job_list_t          *pending_jobs = job_list_new(),
			*running_jobs = job_list_new();

    lpjs_load_job_list(pending_jobs, node_list, LPJS_PENDING_DIR);
    lpjs_load_job_list(running_jobs, node_list, LPJS_RUNNING_DIR);
    
    /*
     *  Step 1: Create a socket for listening for new connections.
     */
    
    listen_fd = lpjs_listen(&server_address);

    /*
     *  Step 2: Accept new connections, and create a separate socket
     *  for communication with each new compute node.
     */
    
    while ( true )
    {
	fd_set  read_fds;
	int     nfds, highest_fd;
	
	// FIXME: Might this erase pending messages?
	// Use poll() instead of select()?
	FD_ZERO(&read_fds);
	FD_SET(listen_fd, &read_fds);
	highest_fd = listen_fd;
	
	for (unsigned c = 0; c < node_list_get_compute_node_count(node_list); ++c)
	{
	    node_t *node = node_list_get_compute_nodes_ae(node_list, c);
	    //lpjs_debug("%s(): Checking node %s, fd = %d...\n",
	    //        __FUNCTION__, node_get_hostname(node), node_get_msg_fd(node));
	    if ( node_get_msg_fd(node) != NODE_MSG_FD_NOT_OPEN )
	    {
		FD_SET(node_get_msg_fd(node), &read_fds);
		if ( node_get_msg_fd(node) > highest_fd )
		    highest_fd = node_get_msg_fd(node);
	    }
	}

	/*
	 *  The nfds (# of file descriptors) argument to select is a
	 *  bit confusing.  It's actually the highest descriptor + 1,
	 *  not the number of open descriptors.  E.g., to check only
	 *  descriptors 3 and 8, nfds must be 9, not 2.  This is
	 *  different from the analogous poll() function, which takes
	 *  an array of open descriptors.
	 */
	nfds = highest_fd + 1;
	
	lpjs_log("%s(): Waiting for input events...\n", __FUNCTION__);
	if ( select(nfds, &read_fds, NULL, NULL, LPJS_NO_SELECT_TIMEOUT) > 0 )
	{
	    //lpjs_debug("%s(): Checking comp fds...\n", __FUNCTION__);
	    // compd doesn't presently initiate conversations on
	    // the persistend socket.  It is used only for dispatchd
	    // to send new jobs to compd.  This function only serves
	    // to check for lost connections with compd daemons.
	    lpjs_check_comp_fds(&read_fds, node_list, running_jobs);
	    
	    //lpjs_debug("%s(): Checking listen fd...\n", __FUNCTION__);
	    // Check FD_ISSET before calling function to avoid overhead
	    if ( FD_ISSET(listen_fd, &read_fds) )
		lpjs_check_listen_fd(listen_fd, &read_fds,
				     node_list, pending_jobs, running_jobs);
	}
	else
	    lpjs_log("%s(): Bug: select() returned 0. This should never happen with no timeout.\n");
    }
    
    // Never actually get here, but make the compiler happy
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
 *      Check all connected sockets for messages
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-22  Jason Bacon Factor out from lpjs_process_events()
 ***************************************************************************/

void    lpjs_check_comp_fds(fd_set *read_fds, node_list_t *node_list,
			    job_list_t *running_jobs)

{
    // Terminates process if malloc() fails, no check required
    node_t  *node = node_new();
    int     fd;
    ssize_t bytes;
    char    *munge_payload;
    uid_t   uid;
    gid_t   gid;
    
    // Top priority: Active compute nodes (move existing jobs along)
    // Second priority: New compute node checkins (make resources available)
    // Lowest priority: User commands
    for (unsigned c = 0; c < node_list_get_compute_node_count(node_list); ++c)
    {
	node = node_list_get_compute_nodes_ae(node_list, c);
	fd = node_get_msg_fd(node);
	if ( (fd != NODE_MSG_FD_NOT_OPEN) && FD_ISSET(fd, read_fds) )
	{
	    // lpjs_debug("Activity on fd %d\n", fd);
	    
	    /*
	     *  select() returns when a peer has closed the connection.
	     *  lpjs_recv() will return 0 in this case.
	     */
	    
	    // FIXME: Verify that lost connections are handled properly
	    // FIXME: Use lpjs_recv_munge
	    bytes = lpjs_recv_munge(fd, &munge_payload,
				    0, 0, &uid, &gid,
				    lpjs_dispatchd_safe_close);
	    if ( bytes < 1 )
	    {
		lpjs_log("%s(): Lost connection to %s.  Closing %d...\n",
			__FUNCTION__, node_get_hostname(node), fd);
		lpjs_dispatchd_safe_close(fd);
		node_set_msg_fd(node, NODE_MSG_FD_NOT_OPEN);
		node_set_state(node, "down");
	    }
	    else
	    {
		// At present, compd never messages dispatchd after checkin
		switch(munge_payload[0])
		{
		    default:
			lpjs_log("%s(): Error: Invalid notification on fd %d: %d\n",
				__FUNCTION__, fd, munge_payload[0]);
		}
		free(munge_payload);
	    }
	}
    }
}


/***************************************************************************
 *  Description:
 *      Create listener socket
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-22  Jason Bacon Factor out from lpjs_process_events()
 ***************************************************************************/

int     lpjs_listen(struct sockaddr_in *server_address)

{
    int     listen_fd;
    
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
	lpjs_log("%s(): Error: Can't create listener socket.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }

    /*
     *  Port on which to listen for new connections from compute nodes.
     *  Convert 16-bit port number from host byte order to network byte order.
     */
    server_address->sin_port = htons(LPJS_IP_TCP_PORT);
    
    // AF_INET = inet4, AF_INET6 = inet6
    server_address->sin_family = AF_INET;
    
    /*
     *  Listen on all local network interfaces for now (INADDR_ANY).
     *  We may allow the user to specify binding to a specific IP address
     *  in the future, for multihomed servers acting as gateways, etc.
     *  Convert 32-bit host address to network byte order.
     */
    server_address->sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind socket fd and server address
    while ( bind(listen_fd, (struct sockaddr *)server_address,
	      sizeof (*server_address)) < 0 )
    {
	lpjs_log("%s(): Error: bind() failed: %s\n", __FUNCTION__, strerror(errno));
	lpjs_log("%s(): Retry in 10 seconds...\n", __FUNCTION__);
	sleep(10);
    }
    lpjs_log("%s(): Bound to port %d...\n", __FUNCTION__, LPJS_IP_TCP_PORT);
    
    /*
     *  Create queue for incoming connection requests
     */
    if (listen(listen_fd, LPJS_CONNECTION_QUEUE_MAX) != 0)
    {
	lpjs_log("%s(): Error: listen() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    return listen_fd;
}



/***************************************************************************
 *  Description
 *      Process events arriving on the listening socket
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-22  Jason Bacon Factor out from lpjs_process_events()
 ***************************************************************************/

int     lpjs_check_listen_fd(int listen_fd, fd_set *read_fds,
			     node_list_t *node_list,
			     job_list_t *pending_jobs, job_list_t *running_jobs)

{
    int             msg_fd,
		    chaperone_status,
		    exit_status;
    ssize_t         bytes;
    char            *munge_payload,
		    *p,
		    *hostname,
		    chaperone_hostname[LPJS_HOSTNAME_MAX + 1];
    socklen_t       address_len = sizeof (struct sockaddr_in);
    uid_t           munge_uid;
    gid_t           munge_gid;
    unsigned long   job_id;
    node_t          *node;
    int             items;
    job_t           *job;
    struct sockaddr_in client_address = { 0 };
    
    bytes = 0;
    /* Accept a connection request */
    if ((msg_fd = accept(listen_fd,
	    (struct sockaddr *)&client_address, &address_len)) == -1)
    {
	lpjs_log("%s(): Error: accept() failed, even though select indicated listen_fd.\n",
		__FUNCTION__);
	return -1;
    }
    else
    {
	lpjs_log("%s(): Accepted connection. fd = %d  addr = %s  port = %u\n",
		 __FUNCTION__, msg_fd, inet_ntoa(client_address.sin_addr),
		 client_address.sin_port);

	/* Read a message through the socket */
	// FIXME: Timeouts temporarily disabled to debug hung connections
	//              &munge_payload, 0, LPJS_CONNECT_TIMEOUT,
	bytes = lpjs_recv_munge(msg_fd,
		     &munge_payload, 0, 0,
		     &munge_uid, &munge_gid,
		     lpjs_dispatchd_safe_close);

	lpjs_debug("%s(): Got %zd byte message.\n", __FUNCTION__, bytes);

	if ( bytes == LPJS_RECV_TIMEOUT )
	{
	    lpjs_log("%s(): Error: lpjs_recv_munge() timed out after %dus: %s, closing %d.\n",
		    __FUNCTION__, LPJS_CONNECT_TIMEOUT, strerror(errno), msg_fd);
	    lpjs_dispatchd_safe_close(msg_fd);
	    // Nothing to free if munge_decode() failed, since it
	    // allocates the buffer
	    // free(munge_payload);
	    return LPJS_RECV_TIMEOUT;
	}
	else if ( bytes == LPJS_RECV_FAILED )
	{
	    lpjs_log("%s(): Error: lpjs_recv_munge() failed (%zd bytes): %s, closing %d.\n",
		    __FUNCTION__, bytes, strerror(errno), msg_fd);
	    lpjs_dispatchd_safe_close(msg_fd);
	    // Nothing to free if munge_decode() failed, since it
	    // allocates the buffer
	    // free(munge_payload);
	    return LPJS_RECV_FAILED;
	}
	// bytes must be at least 1, or no mem is allocated
	else if ( bytes < 1 )
	{
	    lpjs_log("%s(): Bug: Invalid return code from lpjs_recv_munge(): %d\n",
		     __FUNCTION__, bytes);
	    return LPJS_RECV_FAILED;
	}
	
	/* Process request */
	switch(munge_payload[0])
	{
	    case    LPJS_DISPATCHD_REQUEST_COMPD_CHECKIN:
		lpjs_log("%s(): LPJS_DISPATCHD_REQUEST_COMPD_CHECKIN\n",
			__FUNCTION__);
		lpjs_process_compute_node_checkin(msg_fd, munge_payload,
						  node_list, munge_uid, munge_gid);
		lpjs_dispatch_jobs(node_list, pending_jobs, running_jobs);
		// This connection is sustained, don't close it
		break;
	    
	    case    LPJS_DISPATCHD_REQUEST_NODE_LIST:
		lpjs_log("%s(): LPJS_DISPATCHD_REQUEST_NODE_STATUS\n",
			__FUNCTION__);
		node_list_send_status(msg_fd, node_list);
		// lpjs_dispatchd_safe_close(msg_fd);
		// node_list_send_status() sends EOT,
		// so don't use safe_close here.
		lpjs_debug("%s(): Closing %d.\n", __FUNCTION__, msg_fd);
		lpjs_wait_close(msg_fd);
		break;
	    
	    case    LPJS_DISPATCHD_REQUEST_PAUSE:
		lpjs_log("%s(): LPJS_DISPATCHD_REQUEST_PAUSE\n",
			__FUNCTION__);
		lpjs_wait_close(msg_fd);
		
		node_list_set_state(node_list, munge_payload + 1);
		break;
		
	    case    LPJS_DISPATCHD_REQUEST_RESUME:
		lpjs_log("%s(): LPJS_DISPATCHD_REQUEST_RESUME\n",
			__FUNCTION__);
		lpjs_wait_close(msg_fd);
		
		node_list_set_state(node_list, munge_payload + 1);
		// New resources might be available
		lpjs_dispatch_jobs(node_list, pending_jobs, running_jobs);
		break;
	    
	    case    LPJS_DISPATCHD_REQUEST_JOB_LIST:
		lpjs_log("%s(): LPJS_DISPATCHD_REQUEST_JOB_STATUS\n",
			__FUNCTION__);
		
		// FIXME: factor out to lpjs_send_job_list(), check
		// all messages for success
		if ( lpjs_send_munge(msg_fd, "Running\n\n",
				lpjs_dispatchd_safe_close) != LPJS_MSG_SENT )
		{
		    lpjs_log("%s(): Error: Failed to send \"Running\".\n", __FUNCTION__);
		    break;
		}
		job_list_send_params(msg_fd, running_jobs);
		if ( lpjs_send_munge(msg_fd, "\nPending\n\n",
				lpjs_dispatchd_safe_close) != LPJS_MSG_SENT )
		{
		    lpjs_log("%s(): Failed to send \"Pending\".\n", __FUNCTION__);
		    break;
		}
		job_list_send_params(msg_fd, pending_jobs);
		// Need to send EOT after job list
		lpjs_dispatchd_safe_close(msg_fd);
		break;
	    
	    case    LPJS_DISPATCHD_REQUEST_SUBMIT:
		lpjs_log("%s(): LPJS_DISPATCHD_REQUEST_SUBMIT\n",
			__FUNCTION__);
		lpjs_submit(msg_fd, munge_payload, node_list,
			    pending_jobs, running_jobs,
			    munge_uid, munge_gid);
		lpjs_wait_close(msg_fd);
		
		lpjs_dispatch_jobs(node_list, pending_jobs, running_jobs);
		break;
	    
	    case    LPJS_DISPATCHD_REQUEST_CANCEL:
		lpjs_log("%s(): LPJS_DISPATCHD_REQUEST_CANCEL\n",
			__FUNCTION__);
		
		lpjs_cancel(msg_fd, munge_payload + 1, node_list,
			    pending_jobs, running_jobs,
			    munge_uid, munge_gid);
		lpjs_wait_close(msg_fd);
		
		// Resources might become available here
		lpjs_dispatch_jobs(node_list, pending_jobs, running_jobs);
		break;
		
	    case    LPJS_DISPATCHD_REQUEST_CHAPERONE_STATUS:
		// This is a temporary connection from the chaperone
		// for just this message.  Don't keep it open.
		// FIXME: Seeing if this causes stalls
		// lpjs_dispatchd_safe_close(msg_fd);
		lpjs_log("%s(): LPJS_DISPATCHD_REQUEST_CHAPERONE_STATUS\n",
			__FUNCTION__);
		// FIXME: %s is unsafe.  Send hostname first and use strsep().
		sscanf(munge_payload+1, "%lu %d %s",
		       &job_id, &chaperone_status, chaperone_hostname);
		lpjs_debug("%s(): job_id = %lu status = %d  hostname = %s\n",
			 __FUNCTION__, job_id, chaperone_status,
			 chaperone_hostname);
		
		// Errors that occur before exec()ing script
		if ( chaperone_status == LPJS_CHAPERONE_SCRIPT_FAILED )
		{
		    lpjs_log("%s(): Error: Job script failed to start: %d\n",
			    __FUNCTION__, chaperone_status);
		    // Don't try to restart a script that failed
		    // Either the user needs to fix it, or something
		    // is not installed properly
		    adjust_resources(node_list, pending_jobs, hostname, job_id, NODE_RESOURCE_RELEASE);
		    lpjs_remove_pending_job(pending_jobs, job_id);
		}
		else if ( (chaperone_status == LPJS_CHAPERONE_OSERR) ||
			  (chaperone_status == LPJS_CHAPERONE_EXEC_FAILED) )
		{
		    lpjs_log("%s(): Error: OS error or failed exec() detected on %s.\n",
			    __FUNCTION__, chaperone_hostname);
		    
		    lpjs_log("%s(): Releasing resourcesfor job %lu...\n",
			     __FUNCTION__, job_id);
		    adjust_resources(node_list, pending_jobs, hostname,
				     job_id, NODE_RESOURCE_RELEASE);

		    // FIXME: Node should not come back up from here when daemons
		    // are restarted.  It should require "lpjs nodes up nodename"
		    // node_set_state(node, "malfunction");
		    lpjs_log("%s(): Setting %s state to down...\n",
			     __FUNCTION__, chaperone_hostname);
		    node = node_list_find_hostname(node_list, chaperone_hostname);
		    if ( node == NULL )
			lpjs_log("%s(): Bug: No such node in list.\n",
				 __FUNCTION__);
		    else
			node_set_state(node, "down");
		    lpjs_debug("%s(): Done.\n");
		    // FIXME: Make sure job state is reset, but don't remove
		}
		else if ( chaperone_status == LPJS_CHAPERONE_OK )
		{
		    lpjs_log("%s(): Chaperone status OK.\n",__FUNCTION__);
		    // FIXME: Anything to do here?
		}
		else
		{
		    lpjs_log("%s(): Error: Unknown chaperone_status for job %lu: %d\n",
			     __FUNCTION__, job_id, chaperone_status);
		}
		break;

	    case    LPJS_DISPATCHD_REQUEST_JOB_STARTED:
		lpjs_log("%s(): LPJS_DISPATCHD_REQUEST_JOB_STARTED:\n",
			__FUNCTION__);
		lpjs_send_munge(msg_fd, "Node authorized",
				lpjs_wait_close);
		// lpjs_debug("%s(): Auth sent.\n", __FUNCTION__);

		// This is a temporary connection from the chaperone
		// for just this message.  Don't keep it open.
		// Don't sent EOT, but wait for other end to close
		lpjs_wait_close(msg_fd);
		
		/*
		 *  No change in node status, don't try to dispatch jobs.
		 *  Resources were allocated at dispatch time.
		 */
		
		// Job compute node and PIDs are in text form following
		// the one byte LPJS_DISPATCHD_REQUEST_JOB_STARTED
		lpjs_update_job(node_list, munge_payload + 1, pending_jobs, running_jobs);
		break;
		
	    case    LPJS_DISPATCHD_REQUEST_JOB_COMPLETE:
		lpjs_log("%s(): LPJS_DISPATCHD_REQUEST_JOB_COMPLETE\n",
			__FUNCTION__);
		p = munge_payload + 1;
		hostname = strsep(&p, " ");
		lpjs_debug("%s(): hostname = %s ", __FUNCTION__, hostname);
		node = node_list_find_hostname(node_list, hostname);
		if ( node == NULL )
		{
		    lpjs_log("%s(): Error: Invalid hostname in job completion report.\n",
			    __FUNCTION__);
		    break;
		}
		if ( (items = sscanf(p, "%lu %d", &job_id,
				     &exit_status)) != 2 )
		{
		    lpjs_log("%s(): Error: Got %d items reading job_id, procs, mem, status.\n",
			    items);
		    break;
		}
		lpjs_debug("%s(): job_id = %lu  status = %d\n",
		    __FUNCTION__, job_id, exit_status);
		
		adjust_resources(node_list, running_jobs, hostname, job_id, NODE_RESOURCE_RELEASE);
		
		/*
		 *  FIXME:
		 *      Write a completed job record to accounting log
		 *      Note the job completion in the main log
		 */
		// lpjs_log_job();
		
		// FIXME: This is duplicated in lpjs_cancel()
		if ( (job = lpjs_remove_running_job(running_jobs,
						    job_id)) != NULL )
		    job_free(&job);
		else
		    lpjs_log("%s(): Error: remove_running_job returned NULL.  This is a bug.\n",
			    __FUNCTION__);
		
		lpjs_dispatch_jobs(node_list, pending_jobs, running_jobs);

		// This is a temporary connection from the chaperone
		// for just this message.  Don't keep it open.
		// Don't sent EOT, but wait for other end to close
		lpjs_wait_close(msg_fd);
		break;
		
	    default:
		lpjs_log("%s(): Error: Invalid request code byte on listen_fd: %d\n",
			__FUNCTION__, munge_payload[0]);
		
	}   // switch
	free(munge_payload);
    }
    
    return bytes;
}


/***************************************************************************
 *  Description:
 *      Process a compute node checkin request
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-22  Jason Bacon Factor out from lpjs_process_events()
 ***************************************************************************/

void    lpjs_process_compute_node_checkin(int msg_fd, const char *incoming_msg,
					  node_list_t *node_list,
					  uid_t munge_uid, gid_t munge_gid)

{
    // Terminates process if malloc() fails, no check required
    node_t      *new_node = node_new();
    extern FILE *Log_stream;
    
    // FIXME: Check for duplicate checkins.  We should not get
    // a checkin request while one is already open
    // lpjs_log("Munge credential message length = %zd\n", bytes);
    // lpjs_log("munge msg: %s\n", incoming_msg);
    
    lpjs_log("%s(): Checkin from munge_uid %d, munge_gid %d\n",
	    __FUNCTION__, munge_uid, munge_gid);
    
    // FIXME: Record username of compd checkin.  If not root, then only
    // that user can submit jobs to the node.

    /*
     *  Get specs from node and add msg_fd
     */
    
    // Extract specs from incoming_msg + 1
    // node_recv_specs(new_node, msg_fd);
    
    // +1 to skip command code
    node_str_to_specs(new_node, incoming_msg + 1);
    
    // Keep in sync with node_list_send_status()
    node_print_status_header(Log_stream);
    node_print_status(new_node, Log_stream);
    
    // Make sure node name is valid
    // Note: For real security, only authorized
    // nodes should be allowed to pass through
    // the network firewall.
    bool valid_node = false;
    for (unsigned c = 0; c < node_list_get_compute_node_count(node_list); ++c)
    {
	node_t *node = node_list_get_compute_nodes_ae(node_list, c);
	// If config has short hostnames, just match that
	int valid_hostname_len = strlen(node_get_hostname(node));
	if ( memcmp(node_get_hostname(node), node_get_hostname(new_node), valid_hostname_len) == 0 )
	    valid_node = true;
    }
    if ( ! valid_node )
    {
	lpjs_log("%s(): Warning: Unauthorized checkin request from host %s.\n",
		__FUNCTION__, node_get_hostname(new_node));
	lpjs_dispatchd_safe_close(msg_fd);
    }
    else
    {
	lpjs_send_munge(msg_fd, "Node authorized", lpjs_dispatchd_safe_close);
	node_set_msg_fd(new_node, msg_fd);
	
	// Nodes were added to node_list by lpjs_load_config()
	// Just update the fields here
	node_list_update_compute(node_list, new_node);
    }
}


/***************************************************************************
 *  Description:
 *      Add a new submission to the queue
 *  
 *  History: 
 *  Date        Name        Modification
 *  2024-01-22  Jason Bacon Factor out from lpjs_process_events()
 ***************************************************************************/

int     lpjs_submit(int msg_fd, const char *incoming_msg,
		    node_list_t *node_list,
		    job_list_t *pending_jobs, job_list_t *running_jobs,
		    uid_t munge_uid, gid_t munge_gid)

{
    char        script_path[PATH_MAX + 1],
		*end,
		*script_text;
    // Terminates process if malloc() fails, no check required
    job_t       *submission = job_new(),
		*job;
    int         c, job_array_index;
    
    // Payload from lpjs submit is a job description in JOB_SPEC_FORMAT
    job_read_from_string(submission, incoming_msg + 1, &end);
    
    if ( strcmp(job_get_user_name(submission), "root") == 0 )
    {
	lpjs_log("%s(): Error: Rejecting job submission from root.\n",
		__FUNCTION__);
	// msg_fd is closed below
	lpjs_send_munge(msg_fd, "Error: Cannot run jobs as root.\n", lpjs_no_close);
    }
    else
    {
	// Should only be a newline between job specs and script
	script_text = end + 1;
	
	snprintf(script_path, PATH_MAX + 1, "%s/%s",
		 job_get_submit_dir(submission), job_get_script_name(submission));
	for (c = 0; c < job_get_job_count(submission); ++c)
	{
	    lpjs_log("%s(): Submit script %s:%s from %d, %d\n", __FUNCTION__,
		    job_get_submit_node(submission), script_path, munge_uid,
		    munge_gid);
	    job_array_index = c + 1;    // Job arrays are 1-based
	    
	    // Create a separate job_t object for each member of the job array
	    // job_dup() terminates process if malloc() fails
	    job = job_dup(submission);
	    lpjs_queue_job(msg_fd, pending_jobs, job, job_array_index, script_text);
	}
    }
    
    lpjs_dispatchd_safe_close(msg_fd);
    job_free(&submission);
    
    return EX_OK;
}


/***************************************************************************
 *  Description:
 *      Add a new submission to the queue
 *
 *  Returns:
 *      LPJS_SUCCESS, etc.
 *  
 *  History: 
 *  Date        Name        Modification
 *  2024-01-22  Jason Bacon Factor out from lpjs_process_events()
 ***************************************************************************/

int     lpjs_cancel(int msg_fd, const char *incoming_msg,
		    node_list_t *node_list,
		    job_list_t *pending_jobs, job_list_t *running_jobs,
		    uid_t munge_uid, gid_t munge_gid)

{
    unsigned long   job_id;
    char            *end;
    job_t           *job;
    size_t          index;
    char            *hostname;
    
    lpjs_debug("%s(): Incoming = '%s'\n", __FUNCTION__, incoming_msg);
    job_id = strtoul(incoming_msg, &end, 10);
    if ( *end != '\0' )
    {
	lpjs_log("%s(): Bug: Malformed job_id: '%s'\n",
		__FUNCTION__, incoming_msg);
	return -1;
    }
    
    // If job is pending, but dispatched, wait for chaperone checkin
    // before removing it, so the processes can be terminated.
    if ( (index = job_list_find_job_id(pending_jobs, job_id)) != JOB_LIST_NOT_FOUND )
    {
	lpjs_debug("%s(): index = %zu\n", __FUNCTION__, index);
	if ( (job = job_list_get_jobs_ae(pending_jobs, index)) != NULL )
	{
	    if ( job_get_state(job) == JOB_STATE_DISPATCHED )
	    {
		job_set_state(job, JOB_STATE_CANCELED);
		// Resources are reserved as soon as chaperone is forked,
		// before job state is changed to running
		hostname = job_get_compute_node(job);
		adjust_resources(node_list, pending_jobs, hostname,
				 job_id, NODE_RESOURCE_RELEASE);
		lpjs_log("%s(): Pending job %lu is dispatched.  Scheduled for removal after chaperone checkin.\n",
			__FUNCTION__, job_id);
	    }
	    else
	    {
		lpjs_remove_pending_job(pending_jobs, job_id);
		job_free(&job);
		lpjs_log("%s(): Canceled pending job %lu...\n", __FUNCTION__, job_id);
	    }
	}
	else
	    lpjs_log("%s(): Bug: Got valid index for pending job, but no job object.\n",
		    __FUNCTION__);
    }
    // Do not remove the job here.  chaperone will send a completion report
    // else if ( (job = lpjs_remove_running_job(running_jobs, job_id)) != NULL )
    else if ( (index = job_list_find_job_id(running_jobs, job_id)) != JOB_LIST_NOT_FOUND )
    {
	lpjs_log("%s(): Terminating processes for job %lu...\n", __FUNCTION__, job_id);
	if ( (job = job_list_get_jobs_ae(pending_jobs, index)) != NULL )
	    lpjs_kill_processes(node_list, job);
	lpjs_log("%s(): Bug: Got valid index for running job, but no job object.\n",
		__FUNCTION__);
    }
    else
	lpjs_log("%s(): Error: No such active job ID: %lu.\n", __FUNCTION__, job_id);
	
    lpjs_dispatchd_safe_close(msg_fd);
    
    return LPJS_SUCCESS;
}


/***************************************************************************
 *  Description:
 *      Terminate processes associated with a job
 *
 *  Returns:
 *      Number of jobs killed
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-05-08  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_kill_processes(node_list_t *node_list, job_t *job)

{
    char            *compute_node_name,
		    outgoing_msg[LPJS_MSG_LEN_MAX + 1];
    pid_t           chaperone_pid;
    node_t          *compute_node;
    int             compute_node_fd;
    
    if ( job == NULL )
    {
	lpjs_log("%s(): Error: Received NULL job pointer.  This is a bug.\n",
		__FUNCTION__);
	return 0;
    }
    
    if ( (compute_node_name = job_get_compute_node(job)) == NULL )
    {
	lpjs_log("%s(): Error: Received job with no compute node name.  This is a bug.\n",
		__FUNCTION__);
	return 0;
    }
    
    if ( (chaperone_pid = job_get_chaperone_pid(job)) == 0 )
    {
	lpjs_log("%s(): Error: Received job with no chaperone PID.  This is a bug.\n",
		__FUNCTION__);
	return 0;
    }
    
    lpjs_log("%s(): Signaling chaperone PID %lu on %s\n",
	    __FUNCTION__, chaperone_pid, compute_node_name);
    
    compute_node = node_list_find_hostname(node_list, compute_node_name);
    if ( compute_node == NULL )
    {
	lpjs_log("%s(): Error: Compute node name not in node list.\n",
		__FUNCTION__);
	return 0;
    }
    
    if ( (compute_node_fd = node_get_msg_fd(compute_node)) == -1 )
    {
	lpjs_log("%s(): Error: Compute node msg_fd is not open.\n",
		__FUNCTION__);
	return 0;
    }

    snprintf(outgoing_msg, LPJS_MSG_LEN_MAX + 1, "%c%u",
	    LPJS_COMPD_REQUEST_CANCEL, chaperone_pid);
    if ( lpjs_send_munge(compute_node_fd, outgoing_msg,
			 lpjs_dispatchd_safe_close) != LPJS_MSG_SENT )
    {
	lpjs_log("%s(): Error: Failed to send cancel request.\n", __FUNCTION__);
	return 0;
    }
    
    return 1;   // FIXME: Define return codes
}


/***************************************************************************
 *  Description:
 *      Add a job to the queue
 *
 *  Returns:
 *      LPJS_SUCCESS on success
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-30  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_queue_job(int msg_fd, job_list_t *pending_jobs, job_t *job,
		       unsigned long job_array_index, const char *script_text)

{
    char    pending_dir[PATH_MAX + 1],
	    script_path[PATH_MAX + 2],
	    specs_path[PATH_MAX + 11],
	    job_id_path[PATH_MAX + 1],
	    job_id_buff[LPJS_MAX_INT_DIGITS + 1],
	    outgoing_msg[LPJS_MSG_LEN_MAX + 1];
    int     fd;
    ssize_t bytes;
    unsigned long   next_job_id;
    FILE    *fp;
    
    lpjs_log("%s(): Spooling %s...\n", __FUNCTION__, job_get_script_name(job));
    
    snprintf(job_id_path, PATH_MAX + 1, "%s/next-job", LPJS_SPOOL_DIR);
    if ( (fd = open(job_id_path, O_RDONLY)) == -1 )
	next_job_id = 1;
    else
    {
	bytes = read(fd, job_id_buff, LPJS_MAX_INT_DIGITS + 1);
	if ( bytes == -1 )
	{
	    lpjs_log("%s(): Error: Can't read %s: %s\n",
		    __FUNCTION__, job_id_path, strerror(errno));
	    exit(EX_DATAERR);
	}
	sscanf(job_id_buff, "%lu", &next_job_id);
	close(fd);
    }
    
    job_set_job_id(job, next_job_id);
    job_set_array_index(job, job_array_index);
    
    snprintf(pending_dir, PATH_MAX + 1, "%s/%lu", LPJS_PENDING_DIR,
	    next_job_id);
    if ( xt_rmkdir(pending_dir, 0755) != 0 )
    {
	fprintf(stderr, "Cannot create %s: %s\n", pending_dir, strerror(errno));
	return LPJS_WRITE_FAILED;
    }

    snprintf(script_path, PATH_MAX + 2, "%s/%s", pending_dir,
	    xt_basename(job_get_script_name(job)));
    
    if ( (fd = open(script_path, O_WRONLY|O_CREAT|O_TRUNC, 0644)) == -1 )
    {
	lpjs_log("%s(): Error: Failed to copy %s to %s: %s\n", __FUNCTION__,
		job_get_script_name(job), script_path, strerror(errno));
	return LPJS_WRITE_FAILED;
    }
    
    if ( write(fd, script_text, strlen(script_text)) == -1 )
    {
	lpjs_log("%s(): Error: write() failed for %s: %s\n", __FUNCTION__,
		script_path, strerror(errno));
	close(fd);
	return LPJS_WRITE_FAILED;
    }
    close(fd);
    
    /*
     *  Write basic job specs to a file for the dispatcher
     */
    
    snprintf(specs_path, PATH_MAX + 11, "%s/job.specs", pending_dir);
    // FIXME: Switch to low-level I/O?
    if ( (fp = fopen(specs_path, "w")) == NULL )
    {
	lpjs_log("%s(): Error: Cannot create %s: %s\n", __FUNCTION__,
		specs_path, strerror(errno));
	return LPJS_WRITE_FAILED;
    }
    if ( job_print_full_specs(job, fp) < 0 )
    {
	lpjs_log("%s(): Error: write() failed for %s: %s\n", __FUNCTION__,
		specs_path, strerror(errno));
	fclose(fp);
	return LPJS_WRITE_FAILED;
    }
    fclose(fp);
    
    // Back to submit command for terminal output
    snprintf(outgoing_msg, LPJS_MSG_LEN_MAX, "Spooled job %lu to %s.\n",
	    next_job_id, pending_dir);
    if ( lpjs_send_munge(msg_fd, outgoing_msg,
			 lpjs_dispatchd_safe_close) != LPJS_MSG_SENT )
    {
	lpjs_log("%s(): Error: Failed to send response.\n", __FUNCTION__);
	// FIXME: Should we continue?
    }
    
    // FIXME: Log job queue event to the job log
    // lpjs_log_job()
    
    // Bump job num after successful spool
    if ( (fd = open(job_id_path, O_WRONLY|O_CREAT|O_TRUNC, 0644)) == -1 )
    {
	lpjs_log("%s(): Error: Cannot update %s: %s\n", __FUNCTION__,
		job_id_path, strerror(errno));
	return LPJS_WRITE_FAILED;
    }
    else
    {
	if ( xt_dprintf(fd, "%lu\n", ++next_job_id) < 0 )
	{
	    lpjs_log("%s(): Error: write() failed for %job_id_path: %s\n", __FUNCTION__,
		    script_path, strerror(errno));
	    close(fd);
	    return LPJS_WRITE_FAILED;
	}
	close(fd);
    }
    
    job_list_add_job(pending_jobs, job);
    
    return LPJS_SUCCESS;
}


/***************************************************************************
 *  Description:
 *  
 *  Returns:
 *      LPJS_SUCCESS, etc.
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-05-01  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_update_job(node_list_t *node_list, char *payload,
			job_list_t *pending_jobs, job_list_t *running_jobs)

{
    char    *compute_node,
	    *p,
	    pending_job_dir[PATH_MAX + 1],
	    running_job_dir[PATH_MAX + 1 - 10],
	    specs_path[PATH_MAX + 1];
    FILE    *fp;
    unsigned long   job_id;
    pid_t   chaperone_pid, job_pid;
    size_t  job_list_index;
    job_t   *job;
    
    p = payload;
    compute_node = strsep(&p, " ");
    sscanf(p, "%lu %u %u", &job_id, &chaperone_pid, &job_pid);
    lpjs_log("%s(): job_id = %lu  chaperone_pid = %u  job_pid = %u\n",
	    __FUNCTION__, job_id, chaperone_pid, job_pid);
    
    job_list_index = job_list_find_job_id(pending_jobs, job_id);
    if ( job_list_index == JOB_LIST_NOT_FOUND )
	lpjs_log("%s(): Error: Job id not found.  This is a software bug.\n",
		__FUNCTION__);
    else
    {
	// FIXME: Check success of all steps below
	// Move job from pending spool dir to running
	snprintf(pending_job_dir, PATH_MAX + 1,
		LPJS_PENDING_DIR "/%lu", job_id);
	snprintf(running_job_dir, PATH_MAX + 1 - 10,
		LPJS_RUNNING_DIR "/%lu", job_id);
	// lpjs_debug("%s(): Moving %s to %s...\n", __FUNCTION__,
	//              pending_job_dir, running_job_dir);
	rename(pending_job_dir, running_job_dir);
	
	// Add node and PID info to job object
	job = job_list_get_jobs_ae(pending_jobs, job_list_index);
	// lpjs_debug("%s(): Adding %s %lu %lu to job %lu\n",
	//        __FUNCTION__, compute_node, chaperone_pid, job_pid, job_id);
	job_set_compute_node(job, strdup(compute_node));
	job_set_chaperone_pid(job, chaperone_pid);
	job_set_job_pid(job, job_pid);

	// Update in-memory job lists
	job_list_add_job(running_jobs, job);
	job_list_remove_job(pending_jobs, job_get_job_id(job));
	
	// FIXME: Update specs file in running dir with node and PIDs
	snprintf(specs_path, PATH_MAX + 1, "%s/job.specs", running_job_dir);
	lpjs_log("%s(): Storing updated specs to %s.\n",
		__FUNCTION__, specs_path);
	
	// FIXME: Switch to low-level I/O?
	if ( (fp = fopen(specs_path, "w")) == NULL )
	{
	    lpjs_log("%s(): Error: Cannot create %s: %s\n", __FUNCTION__,
		    specs_path, strerror(errno));
	    return LPJS_WRITE_FAILED;
	}
	job_print_full_specs(job, fp);
	fclose(fp);
	
	/*
	 *  If job was canceled while still pending but after dispatched,
	 *  we need to terminate the processes now.
	 */
	
	if ( job_get_state(job) == JOB_STATE_CANCELED )
	{
	    lpjs_log("%s(): Job %lu was canceled after dispatch.  Removing...\n",
		    __FUNCTION__, job_id);
	    lpjs_remove_running_job(running_jobs, job_id);
	    lpjs_kill_processes(node_list, job);
	    job_free(&job);
	}
    }
    
    return LPJS_SUCCESS;
}


/***************************************************************************
 *  Description:
 *
 *  Returns:
 *      LPJS_SUCCESS, etc.
 *  
 *  Arguments:
 *      spool_dir:  LPJS_PENDING_DIR or LPJS_RUNNING_DIR
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-05-08  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_load_job_list(job_list_t *job_list, node_list_t *node_list,
			   char *spool_dir)

{
    DIR             *dp;
    struct dirent   *entry;
    char            specs_path[PATH_MAX + 1];
    extern FILE     *Log_stream;
    node_t          *compute_node;
    
    lpjs_log("%s(): Reloading jobs from %s...\n", __FUNCTION__, spool_dir);
    if ( (dp = opendir(spool_dir)) == NULL )
    {
	lpjs_log("%s(): Error: Cannot open %s: %s\n", __FUNCTION__,
		spool_dir, strerror(errno));
	return LPJS_READ_FAILED;
    }
    
    while ( (entry = readdir(dp)) != NULL )
    {
	// Job directories are named after job #s
	if ( xt_strisint(entry->d_name, 10) )
	{
	    // job_new() terminates the process if malloc fails
	    job_t   *job = job_new();
	    
	    /*
	     *  Load job specs from file
	     */
	    
	    snprintf(specs_path, PATH_MAX + 1, "%s/%s/%s",
		    spool_dir, entry->d_name, LPJS_SPECS_FILE_NAME);
	    if ( job_read_from_file(job, specs_path) != JOB_SPECS_ITEMS )
	    {
		lpjs_log("%s(): Error: Can't read %s.\n",
			__FUNCTION__, specs_path);
		return LPJS_READ_FAILED;
	    }
	    lpjs_log("%s(): Loaded job #%s\n", __FUNCTION__, entry->d_name);
	    job_list_add_job(job_list, job);
	    
	    // FIXME: Update node status if job is running
	    // This code is untested
	    if ( strcmp(spool_dir, LPJS_RUNNING_DIR) == 0 )
	    {
		compute_node = node_list_find_hostname(node_list,
						       job_get_compute_node(job));
		node_adjust_resources(compute_node, job,
				      NODE_RESOURCE_ALLOCATE);
		/* Replaces...
		node_set_procs_used(compute_node,
				    node_get_procs_used(compute_node) +
				    job_get_procs_per_job(job));
		node_set_phys_MiB_used(compute_node,
				       node_get_phys_MiB_used(compute_node) +
				       job_get_procs_per_job(job) *
				       job_get_pmem_per_proc(job));
		*/
	    }
	}
    }
    closedir(dp);
    
    // FIXME: Sort numerically by job id
    job_list_sort(job_list);

    return LPJS_SUCCESS;   // FIXME: Define return codes
}


/***************************************************************************
 *  Description:
 *      Gracefully shut down in the event of an interrupt signal
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

void    lpjs_dispatchd_terminate_handler(int s2)

{
    node_t  *node;
    int     c;
    extern node_list_t *Node_list;
    
    lpjs_log("%s(): Received signal, shutting down...\n", __FUNCTION__);
    for (c = 0; c < node_list_get_compute_node_count(Node_list); ++c)
    {
	node = node_list_get_compute_nodes_ae(Node_list, c);
	if ( node_get_msg_fd(node) != -1 )
	{
	    lpjs_log("%s(): Closing connection with %s...\n",
		    __FUNCTION__, node_get_hostname(node));
	    lpjs_dispatchd_safe_close(node_get_msg_fd(node));
	}
    }
#ifdef __linux__
    extern char Pid_path[PATH_MAX + 1];
    remove(Pid_path);
#endif
    exit(EX_OK);
}


void    lpjs_dispatchd_sigpipe(int s2)

{
    lpjs_log("%s(): Bug: Ignoring SIGPIPE signal.\n", __FUNCTION__);
}


/***************************************************************************
 *  Description:
 *      Allocate or release resources allocated for a job
 *
 *  Arguments:
 *      node_list   Pointer to node_list object
 *      job_list    Pointer to job_list object (pending | running)
 *      hostname    Name of specific node within the list
 *      job_id      ID of job within the list
 *      direction   NODE_RESOURCE_ALLOCATE | NODE_RESOURCE_RELEASE
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-12-08  Jason Bacon Begin
 ***************************************************************************/

int     adjust_resources(node_list_t *node_list, job_list_t *job_list,
			   const char *hostname, unsigned long job_id,
			   node_resource_t direction)

{
    node_t  *node;
    job_t   *job;
    int     job_index;
    
    // FIXME: MPI jobs may use multiple nodes
    if ( (node = node_list_find_hostname(node_list, hostname)) == NULL )
    {
	lpjs_log("%s(): Error: %s not found in node list.\n", __FUNCTION__, hostname);
	return 1;
    }
    if ( (job_index = job_list_find_job_id(job_list, job_id)) == JOB_LIST_NOT_FOUND )
    {
	lpjs_log("%s(): Error: %lu not found in job list.\n", __FUNCTION__, job_id);
	return 1;
    }
    job = job_list_get_jobs_ae(job_list, job_index);
    node_adjust_resources(node, job, direction);
    
    return 0;   // FIXME: Define return codes
}
