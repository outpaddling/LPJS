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
#include <sys/stat.h>       // S_ISDIR()
#include <pwd.h>            // getpwnam()
#include <grp.h>            // getgrnam()
#include <fcntl.h>          // open()
#include <signal.h>
#include <sys/wait.h>
#include <sys/sysctl.h>

#include <xtend/string.h>
#include <xtend/proc.h>
#include <xtend/file.h>     // xt_rmkdir()

#include "lpjs.h"
#include "node-list.h"
#include "config.h"
#include "network.h"
#include "misc.h"
#include "job.h"
#include "lpjs_compd.h"

int     main (int argc, char *argv[])

{
    // Terminates process if malloc() fails, no check required
    node_list_t *node_list = node_list_new();
    // Terminates process if malloc() fails, no check required
    node_t      *node = node_new();
    char        *munge_payload,
		vis_msg[LPJS_MSG_LEN_MAX + 1];
    ssize_t     bytes;
    int         compd_msg_fd;
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
	// FIXME: Log_stream should use close on exec (fork chaperone)
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
    
    snprintf(Pid_path, PATH_MAX + 1, "%s/lpjs_compd.pid", LPJS_RUN_DIR);
    status = xt_create_pid_file(Pid_path, Log_stream);
    if ( status != EX_OK )
	return status;
#endif

    // Get hostname of head node
    lpjs_load_config(node_list, LPJS_CONFIG_HEAD_ONLY, Log_stream);

    compd_msg_fd = lpjs_compd_checkin_loop(node_list, node);
    poll_fd.fd = compd_msg_fd;
    // POLLERR and POLLHUP are actually always set.  Listing POLLHUP here just
    // for documentation.
    poll_fd.events = POLLIN | POLLHUP;
    
    // Now keep daemon running, awaiting jobs
    // Almost correct: https://unix.stackexchange.com/questions/581426/how-to-get-notified-when-the-other-end-of-a-socketpair-is-closed
    while ( true )
    {
	// Just poll the dedicated socket connection with dispatchd
	// Time out after 2 seconds
	poll(&poll_fd, 1, 2000);

	// dispatchd closed its end of the socket?
	if (poll_fd.revents & POLLHUP)
	{
	    poll_fd.revents &= ~POLLHUP;
	    
	    // Close this end, or dispatchd gets "address already in use"
	    // When trying to restart
	    close(compd_msg_fd);
	    lpjs_log("%s(): Lost connection to dispatchd: HUP received.\n",
		    __FUNCTION__);
	    sleep(LPJS_RETRY_TIME);  // No point trying immediately after drop
	    compd_msg_fd = lpjs_compd_checkin_loop(node_list, node);
	}
	
	if (poll_fd.revents & POLLERR)
	{
	    poll_fd.revents &= ~POLLERR;
	    lpjs_log("%s(): Error occurred polling dispatchd: %s\n",
		    __FUNCTION__, strerror(errno));
	    break;
	}
	
	if (poll_fd.revents & POLLIN)
	{
	    poll_fd.revents &= ~POLLIN;
	    // FIXME: Add a timeout and handling code
	    bytes = lpjs_recv_munge(compd_msg_fd, &munge_payload, 0, 0,
				    &uid, &gid, close);
	    if ( bytes < 0 )
	    {
		lpjs_log("%s(): Got < 0 bytes from dispatchd.  Something is wrong.\n",
			__FUNCTION__);
	    }
	    else if ( bytes == 0 )
	    {
		/*
		 *  Likely lost connection due to crash or other ungraceful
		 *  event.  Close connection so that dispatchd doesn't hang
		 *  with "address already in use".
		 */
		close(compd_msg_fd);
		lpjs_log("%s(): 0 bytes received from dispatchd.  Disconnecting...\n",
			__FUNCTION__);
		poll_fd.revents = 0;
		compd_msg_fd = lpjs_compd_checkin_loop(node_list, node);
	    }
	    else
	    {
		munge_payload[bytes] = '\0';
		xt_strviscpy((unsigned char *)vis_msg,
			 (unsigned char *)munge_payload, LPJS_MSG_LEN_MAX + 1);
		// lpjs_log("Received %zd bytes from dispatchd: \"%s\"\n", bytes, vis_msg);
		if ( munge_payload[0] == LPJS_EOT )
		{
		    // Close this end, or dispatchd gets "address already in use"
		    // When trying to restart
		    close(compd_msg_fd);
		    lpjs_log("%s(): Lost connection to dispatchd: EOT received.\n",
			    __FUNCTION__);
		    sleep(LPJS_RETRY_TIME);  // No point trying immediately after drop
    
		    // Ignore HUP that follows EOT
		    // FIXME: This might be bad timing
		    poll_fd.revents &= ~POLLHUP;
		    compd_msg_fd = lpjs_compd_checkin_loop(node_list, node);
		}
		else if ( munge_payload[0] == LPJS_COMPD_REQUEST_NEW_JOB )
		{
		    // Terminates process if malloc() fails, no check required
		    job_t   *job = job_new();
		    char    *script_start;
		    
		    lpjs_log("LPJS_COMPD_REQUEST_NEW_JOB\n");
		    
		    /*
		     *  Parse job specs
		     */
		    
		    job_read_from_string(job, munge_payload + 1, &script_start);
		    
		    /*
		     *  lpjs_run_chaperone() forks, and the child process
		     *  sends a response to lpjs_compd, depending on
		     *  whether the dispatch succeeds.
		     */
		    
		    lpjs_run_chaperone(job, script_start, compd_msg_fd, node_list);
		}
		else if ( munge_payload[0] == LPJS_COMPD_REQUEST_CANCEL )
		{
		    pid_t   chaperone_pid;
		    char    *end;
		    
		    lpjs_log("LPJS_COMPD_REQUEST_CANCEL\n");
		    lpjs_log("Payload = %s\n", munge_payload + 1);
		    
		    chaperone_pid = strtoul(munge_payload + 1, &end, 10);
		    if ( *end != '\0' )
			lpjs_log("Malformed cancel payload.  This is a software bug.\n");
		    else
		    {
			lpjs_log("Sending SIGHUP to %d...\n", chaperone_pid);
			kill(chaperone_pid, SIGHUP);
			// FIXME: Verify termination
		    }
		}
		free(munge_payload);
	    }
	}
    }

    close(compd_msg_fd);
    return EX_IOERR;
}


int     lpjs_compd_checkin(int compd_msg_fd, node_t *node)

{
    char        outgoing_msg[LPJS_MSG_LEN_MAX + 1],
		*munge_payload,
		specs[NODE_SPECS_LEN + 1];
    ssize_t     bytes;
    uid_t       uid;
    gid_t       gid;
    extern FILE *Log_stream;
    
    /* Send a message to the server */
    /* Need to send \0, so xt_dprintf() doesn't work here */
    node_detect_specs(node);
    snprintf(outgoing_msg, LPJS_MSG_LEN_MAX + 1,
	    "%c%s", LPJS_DISPATCHD_REQUEST_COMPD_CHECKIN,
	    node_specs_to_str(node, specs, NODE_SPECS_LEN + 1));
    lpjs_log("%s(): Sending node specs:\n", __FUNCTION__);
    node_print_specs_header(Log_stream);
    fprintf(Log_stream, "%s\n", outgoing_msg + 1);
    if ( lpjs_send_munge(compd_msg_fd, outgoing_msg, close) != LPJS_MSG_SENT )
    {
	lpjs_log("%s(): Failed to send checkin message to dispatchd: %s",
		__FUNCTION__, strerror(errno));
	close(compd_msg_fd);
	return EX_IOERR;
    }
    lpjs_log("%s(): Sent checkin request.\n", __FUNCTION__);

    // FIXME: Add a timeout and handling code
    bytes = lpjs_recv_munge(compd_msg_fd, &munge_payload, 0, 0, &uid, &gid, close);
    if ( bytes < 1 )
    {
	lpjs_log("Error receving auth message.\n");
	exit(EX_IOERR); // FIXME: Should we retry?
    }
    else if ( strcmp(munge_payload, "Node authorized") != 0 )
    {
	lpjs_log("%s(): This node is not authorized to connect.\n"
		 "It must be added to the etc/lpjs/config on the head node.\n",
		 __FUNCTION__);
	exit(EX_NOPERM);
    }
    else
	lpjs_log("%s(): Received authorization from lpjs_dispatchd.\n",
		__FUNCTION__);

    free(munge_payload);
    
    return EX_OK;
}


/***************************************************************************
 *  Use auto-c2man to generate a man page from this comment
 *
 *  Name:
 *      -
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
 *  2024-12-05  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_dispatchd_connect_loop(node_list_t *node_list)

{
    int     compd_msg_fd;
    
    // Retry socket connection indefinitely
    while ( (compd_msg_fd = lpjs_connect_to_dispatchd(node_list)) == -1 )
    {
	lpjs_log("%s(): Failed to connect to dispatchd: %s\n",
		__FUNCTION__, strerror(errno));
	lpjs_log("Retry in %d seconds...\n", LPJS_RETRY_TIME);
	sleep(LPJS_RETRY_TIME);
    }
    
    return compd_msg_fd;
}


/***************************************************************************
 *  Description:
 *      Connect to dispatchd and send checkin request.
 *      Retry indefinitely if failure occurs.
 *
 *  Returns:
 *      File descriptor for ongoing connection to dispatchd.
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-23  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_compd_checkin_loop(node_list_t *node_list, node_t *node)

{
    int     compd_msg_fd,
	    status;

    // Does not return until successful
    compd_msg_fd = lpjs_dispatchd_connect_loop(node_list);
    
    // Retry checking request indefinitely
    while ( (status = lpjs_compd_checkin(compd_msg_fd, node)) != EX_OK )
    {
	// In case failure is due to disconnect
	close(compd_msg_fd);
	compd_msg_fd = lpjs_dispatchd_connect_loop(node_list);
	
	lpjs_log("%s(): compd-checkin failed.  Retry in %d seconds...\n",
		 __FUNCTION__, LPJS_RETRY_TIME);
	sleep(LPJS_RETRY_TIME);
    }
    
    lpjs_log("%s(): Checkin successful.\n", __FUNCTION__);
    
    return compd_msg_fd;
}


/***************************************************************************
 *  Description:
 *      Save script copy, create log dir
 *  
 *  Returns:
 *      LPJS_SUCCESS, etc.
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-05-10  Jason Bacon Factored out from lpjs_run_script()
 ***************************************************************************/

int     lpjs_working_dir_setup(job_t *job, const char *script_start,
				char *job_script_name, size_t maxlen)

{
    char    temp_wd[PATH_MAX + 1 - 20],
	    start_wd[PATH_MAX + 1 - 20],
	    log_dir[PATH_MAX + 1],
	    shared_fs_marker[LPJS_SHARED_FS_MARKER_MAX + 1],
	    shared_fs_marker_path[PATH_MAX + 1],
	    *working_dir;
    int     fd;
    // FIXME: Break out new functions for this
    struct stat st;
    extern FILE *Log_stream;
    
    /*
     *  Go to same directory from which job was submitted
     *  if it exists here (likely using NFS), otherwise
     *  go to user's home dir.
     */
    
    working_dir = job_get_submit_dir(job);
    
    lpjs_get_marker_filename(shared_fs_marker, job_get_submit_node(job),
			     PATH_MAX + 1);
    snprintf(shared_fs_marker_path, PATH_MAX + 1, "%s/%s",
	     working_dir, shared_fs_marker);
    
    lpjs_log("%s(): Checking for %s...\n", __FUNCTION__, shared_fs_marker_path);
    if ( stat(shared_fs_marker_path, &st) != 0 )
    {
	struct passwd *pw_ent;
	
	// Use pwnam_r() if multithreading, not likely
	if ( (pw_ent = getpwnam(job_get_user_name(job))) == NULL )
	{
	    lpjs_log("%s(): No such user: %s\n",
		    __FUNCTION__, job_get_user_name(job));
	    // FIXME: Report job failure to dispatchd
	}
	else
	{
	    // Place temp working dirs in user's home dir
	    // FIXME: Check for failures
	    chdir(pw_ent->pw_dir);
	    
	    // FIXME: Remove LPJS-job-* from previous submissions
	    // This should replace temp workdir removal in chaperone
	    
	    snprintf(temp_wd, PATH_MAX + 1 - 20, "LPJS-job-%lu",
		    job_get_job_id(job));
	    lpjs_log("%s(): %s does not exist.  Using temp dir %s.\n",
		    __FUNCTION__, working_dir, temp_wd);
	    
	    // If temp_wd exists, rename it first
	    // This will only happen if the job ID is duplicated
	    if ( stat(temp_wd, &st) == 0 )
	    {
		char    save_wd[PATH_MAX + 1];
		int c = 0;
		do
		{
		    snprintf(save_wd, PATH_MAX + 1, "%s.%d", temp_wd, c++);
		}   while ( stat(save_wd, &st) == 0 );
		rename(temp_wd, save_wd);
	    }
	    
	    mkdir(temp_wd, 0700);
	    working_dir = temp_wd;
	}
    }
    
    // FIXME: getcwd() hangs on NetBSD when running as a service
    // chdir() to $HOME beforehand fixes the issue
    xt_get_home_dir(start_wd, PATH_MAX + 1 - 20);
    chdir(start_wd);
    lpjs_log("Changing from %s to %s...\n", start_wd, working_dir);
    if ( chdir(working_dir) != 0 )
    {
	lpjs_log("Failed to enter working dir: %s\n", working_dir);
	// FIXME: Check for actual reason
	return EX_NOPERM;
    }
    
    if ( getcwd(temp_wd, PATH_MAX + 1 - 20) == NULL )
    {
	lpjs_log("getcwd() failed: errno = %s\n", strerror(errno));
	// Odd that chdir() indicates success, but getcwd() fails
	#ifdef __APPLE__
	lpjs_log("You may need to grant lpjs_compd full disk access in\n"
		 "System Preferences -> Privacy and Security.  This access\n"
		 "will be revoked when LPJS is updated.  If you find\n"
		 "that you might repeatedly reset it, please report the\n"
		 "problem to Apple via the developer feedback assistant.\n"
		 "They need to hear from multiple people before they will\n"
		 "take the issue seriously.\n");
	#endif
	
	return LPJS_CHAPERONE_OSERR;
    }
    else
	lpjs_log("Confirmed in %s.\n", temp_wd);
    
    /*
     *  Save script
     */
    
    lpjs_job_log_dir(job_get_log_dir(job), job_get_job_id(job),
		      log_dir, PATH_MAX + 1);
    xt_rmkdir(log_dir, 0700);
    snprintf(job_script_name, maxlen, "%s/%s",
	    log_dir, job_get_script_name(job));
    lpjs_log("Saving job script to %s.\n", job_script_name);
    if ( (fd = open(job_script_name, O_WRONLY|O_CREAT|O_TRUNC, 0700)) == -1 )
    {
	lpjs_log("%s(): Cannot create %s: %s\n",
		__FUNCTION__, job_script_name, strerror(errno));
	// FIXME: Report job failure to dispatchd
    }
    write(fd, script_start, strlen(script_start));
    close(fd);
    
    /*
     *  FIXME: Update node status (keep a copy here in case
     *  dispatchd is restarted)
     */
    return LPJS_SUCCESS;
}


/***************************************************************************
 *  Description:
 *      Attempt to send job completion report to dispatchd
 *
 *  Returns:
 *      EX_OK on success, EX_IOERR otherwise
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-12-07  Jason Bacon Adapt from lpjs_chaperone_completion
 ***************************************************************************/

int     lpjs_send_chaperone_status(int msg_fd, unsigned long job_id,
				   chaperone_status_t chaperone_status)

{
    char    outgoing_msg[LPJS_MSG_LEN_MAX + 1],
	    hostname[sysconf(_SC_HOST_NAME_MAX) + 1];
    
    lpjs_log("%s(): job_id %lu sending %d on %d\n", __FUNCTION__,
	     job_id, chaperone_status, msg_fd);
    /* Send job completion message to dispatchd */
    gethostname(hostname, sysconf(_SC_HOST_NAME_MAX));
    snprintf(outgoing_msg, LPJS_MSG_LEN_MAX + 1, "%c%lu %d %s",
	     LPJS_DISPATCHD_REQUEST_CHAPERONE_STATUS, job_id,
	     chaperone_status, hostname);
    lpjs_log(outgoing_msg + 1);
    if ( lpjs_send_munge(msg_fd, outgoing_msg, close) != LPJS_MSG_SENT )
    {
	lpjs_log("%s(): Failed to send message to dispatchd: %s\n",
		 __FUNCTION__, strerror(errno));
	close(msg_fd);
	return LPJS_WRITE_FAILED;
    }
    lpjs_log("%s(): Status %d sent by job_id %lu.\n",
	     __FUNCTION__, chaperone_status, job_id);
    
    return EX_OK;
}


/***************************************************************************
 *  Description:
 *      Connect to dispatchd and send checkin request.
 *      Retry indefinitely if failure occurs.
 *
 *  Returns:
 *      File descriptor for ongoing connection to dispatchd.
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-12-07  Jason Bacon Adapt from lpjs_chaperone_completion_loop
 ***************************************************************************/

int     lpjs_send_chaperone_status_loop(node_list_t *node_list,
					unsigned long job_id,
					chaperone_status_t chaperone_status)

{
    int     msg_fd, send_status;
    
    lpjs_log("%s(): job_id %lu sendind status %d\n", __FUNCTION__,
	     job_id, chaperone_status);
    // Retry socket connection and message send indefinitely
    do
    {
	msg_fd = lpjs_connect_to_dispatchd(node_list);
	if ( msg_fd == -1 )
	{
	    lpjs_log("%s(): Failed to connect to dispatchd: %s\n",
		    __FUNCTION__, strerror(errno));
	    lpjs_log("Retry in %d seconds...\n", LPJS_RETRY_TIME);
	    sleep(LPJS_RETRY_TIME);
	}
	else
	{
	    send_status = lpjs_send_chaperone_status(msg_fd, job_id,
						     chaperone_status);
	    if ( send_status != EX_OK )
	    {
		lpjs_log("%s(): Message send failed.  Retry in %d seconds...\n",
			 __FUNCTION__, LPJS_RETRY_TIME);
		sleep(LPJS_RETRY_TIME);
	    }
	    close(msg_fd);
	}
	lpjs_log("msg_fd = %d  send_status = %d\n", msg_fd, send_status);
    }   while ( (msg_fd == -1) || (send_status != EX_OK) );
    
    lpjs_log("%s(): Chaperone status %d sent.\n", __FUNCTION__, chaperone_status);
    
    return 0;   // FIXME: Define return codes
}


void    enforce_resource_limits(job_t *job)

{
#if defined(__APPLE__)
#elif defined(__DragonFly__)
#elif defined(__FreeBSD__)
    // Use rctl
    int     enabled;
    size_t  len = sizeof(int);
    
    sysctlbyname("kern.racct.enable", &enabled, &len, NULL, 0);
    if ( enabled == 1 )
    {
	// Set RSS for this process
	// rctl_add_rule();
    }
#elif defined(__Linux__)
    // Use cgroups?
#elif defined(__NetBSD__)
#elif defined(__OpenBSD__)
#endif
}


/***************************************************************************
 *  Description:
 *  
 *  Returns:
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-10  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_run_chaperone(job_t *job, const char *script_start,
			    int compd_msg_fd, node_list_t *node_list)

{
    char        *chaperone_bin = PREFIX "/libexec/lpjs/chaperone",
		chaperone_response[LPJS_MSG_LEN_MAX + 1],
		job_script_name[PATH_MAX + 1],
		out_file[PATH_MAX + 1],
		err_file[PATH_MAX + 1];
    unsigned long   job_id = job_get_job_id(job);
    extern FILE *Log_stream;
    
    signal(SIGCHLD, sigchld_handler);

    /*
     *  Child process must tell lpjs_compd whether chaperone was
     *  successfully launched.  Script failures are reported by
     *  chaperone directly to lpjs_dispatchd.
     */
    
    if ( fork() == 0 )
    {
	/*
	 *  Child: This is now the chaperone process.
	 *  exec() the chaperone command with the script as an argument.
	 *  The chaperone runs in the background, monitoring the job,
	 *  enforcing resource limits, and reporting exit status and
	 *  stats to dispatchd.
	 *
	 *  Exit with EX_OSERR to indicate a problem with the node.
	 *  This will cause LPJS to take the node down.
	 *  Permission problems, etc. should return other codes, which
	 *  merely lead to job failure.
	 */
	
	/*
	 *  Send verification that the chaperone process started
	 *  back to lpjs_dispatchd immediately, so it can resume
	 *  listening for new events.  The work that follows
	 *  (creating directories, redirecting, running the script,
	 *  etc) can take a while on a busy compute node, and we
	 *  don't want dispatchd stuck waiting.
	 */
	
	lpjs_log("Sending chaperone forked verification.\n");
	snprintf(chaperone_response, LPJS_MSG_LEN_MAX + 1,
		"%c", LPJS_CHAPERONE_FORKED);
	if ( lpjs_send_munge(compd_msg_fd, chaperone_response, close)
			     != LPJS_MSG_SENT )
	{
	    lpjs_log("%s(): Failed to send chaperone forked verification.\n",
		    __FUNCTION__);
	    close(compd_msg_fd);
	    exit(EX_UNAVAILABLE);
	}

	// We don't want chaperone and its children to inherit
	// the socket connection between dispatchd and compd.
	// The parent process lpjs_compd will continue to use it,
	// but we're done with it here.
	close(compd_msg_fd);
	
	enforce_resource_limits(job);
	
	// If lpjs_compd is running as root, use setuid() to switch
	// to submitting user
	if ( getuid() == 0 )
	{
	    struct passwd   *pw_ent;
	    uid_t           uid;
	    struct group    *gr_ent;
	    gid_t           gid;
	    char            *user_name, *group_name;
	    
	    uid = getuid();
	    gid = getgid();

	    user_name = job_get_user_name(job);
	    if ( (pw_ent = getpwnam(user_name)) == NULL )
	    {
		lpjs_log("%s(): ERROR: %s: No such user.\n", __FUNCTION__, user_name);
		lpjs_send_chaperone_status_loop(node_list, job_id,
						LPJS_CHAPERONE_OSERR);
		exit(EX_OSERR);
	    }
	    
	    group_name = job_get_primary_group_name(job);
	    if ( (gr_ent = getgrnam(group_name)) == NULL )
	    {
		lpjs_log("%s(): INFO: %s: No such group.\n", __FUNCTION__, group_name);
		gid = pw_ent->pw_gid;
	    }
	    else
		gid = gr_ent->gr_gid;
	    
	    // Set gid before uid, while still running as root
	    if ( setgid(gid) != 0 )
		lpjs_log("%s(): WARNING: Failed to set gid to %u.\n", __FUNCTION__, gid);
	    
	    uid = pw_ent->pw_uid;
	    if ( setuid(uid) != 0 )
	    {
		lpjs_log("%s(): ERROR: Failed to set uid to %u.\n", __FUNCTION__, uid);
		lpjs_send_chaperone_status_loop(node_list, job_id,
						LPJS_CHAPERONE_OSERR);
		exit(EX_OSERR);
	    }
	    
	    lpjs_log("%s(): user = %s  group = %s\n", __FUNCTION__,
		    user_name, group_name);
	    lpjs_log("%s(): uid = %u  gid = %u\n", __FUNCTION__, uid, gid);
	}
	
	/*
	 *  Set LPJS_USER, LPJS_SUBMIT_HOST, etc. for use in scripts
	 */
	job_setenv(job);
	
	if ( lpjs_working_dir_setup(job, script_start, job_script_name,
				    PATH_MAX + 1) != LPJS_SUCCESS )
	{
	    // FIXME: Take node down and reschedule jobs elsewhere
	    // FIXME: Terminating here causes dispatchd to crash
	    // dispatchd should be able to tolerate lost connections at any time
	    lpjs_log("%s(): lpjs_working_dir_setup() failed.\n", __FUNCTION__);
	    lpjs_send_chaperone_status_loop(node_list, job_id, LPJS_CHAPERONE_OSERR);
	    exit(EX_OSERR);
	}
	
	// FIXME: Make sure filenames are not truncated
	
	// Redirect stdout
	strlcpy(out_file, job_script_name, PATH_MAX + 1);
	strlcat(out_file, ".stdout", PATH_MAX + 1);
	close(1);
	if ( open(out_file, O_WRONLY|O_CREAT, 0644) == -1 )
	{
	    lpjs_log("%s(): Could not open %s: %s\n", __FUNCTION__,
		     out_file, strerror(errno));
	    lpjs_send_chaperone_status_loop(node_list, job_id,
					    LPJS_CHAPERONE_CANTCREAT);
	    exit(EX_CANTCREAT);
	}
	
	// Redirect stderr
	strlcpy(err_file, job_script_name, PATH_MAX + 1);
	strlcat(err_file, ".stderr", PATH_MAX + 1);
	close(2);
	if ( open(err_file, O_WRONLY|O_CREAT, 0644) == -1 )
	{
	    lpjs_log("%s(): Could not open %s: %s\n", __FUNCTION__,
		     err_file, strerror(errno));
	    lpjs_send_chaperone_status_loop(node_list, job_id,
					    LPJS_CHAPERONE_CANTCREAT);
	    exit(EX_CANTCREAT);
	}
	
	// Note: This will be redirected to err_file
	// lpjs_log("%s(): Running chaperone: %s %s...\n", __FUNCTION__,
	//         chaperone_bin, job_script_name);
	
	// FIXME: Build should use realpath
	// FIXME: This assumes execl() will succeed, which is all but certain.
	// It would be better to send msg_fd value to chaperone and let
	// it respond to dispatchd, or send a failure message after execl().
	lpjs_send_chaperone_status_loop(node_list, job_id, LPJS_CHAPERONE_OK);

	execl(chaperone_bin, chaperone_bin, job_script_name, NULL);
	
	// We only get here if execl() failed
	// Note: This will be redirected to err_file
	lpjs_log("%s(): Failed to exec %s %u %u %s\n",
		__FUNCTION__, chaperone_bin, job_script_name);
	// See FIXME above
	lpjs_send_chaperone_status_loop(node_list, job_id, LPJS_CHAPERONE_EXEC_FAILED);
	exit(EX_SOFTWARE);
    }
    
    /*
     *  No else clause to if ( fork() == 0 ):
     *  lpjs_compd does not wait for chaperone, but resumes listening
     *  for more jobs.
     */

    return EX_OK;
}


void    lpjs_chown(job_t *job, const char *path)

{
    struct passwd *pw_ent;
    struct group *gr_ent;

    // FIXME: Use getpwnam_r() if multithreading, unlikely
    // FIXME: Terminate job if this fails
    pw_ent = getpwnam(job_get_user_name(job));
    lpjs_log("User %u changing ownership of %s to user %u.\n",
	    getuid(), path, pw_ent->pw_uid);
    if ( chown(path, pw_ent->pw_uid, -1) != 0 )
	lpjs_log("%s(): chown() failed.\n", __FUNCTION__);

    // It's OK if this fails, groups may differ on different nodes
    if ( (gr_ent = getgrnam(job_get_primary_group_name(job))) != NULL )
    {
	lpjs_log("User %u changing group ownership of %s to %u.\n",
		getuid(), path, gr_ent->gr_gid);
	if ( chown(path, -1, gr_ent->gr_gid) != 0 )
	    lpjs_log("%s(): chown() failed.\n", __FUNCTION__);
    }
    else
	lpjs_log("INFO: No %s group on this host.\n",
		job_get_primary_group_name(job));
}


/***************************************************************************
 *  Description:
 *      Per Alma sigaction man page, catching SIGCHLD and calling wait()
 *      is the only fully portable way to avoid zombie chaperone processes.
 *      Modern systems can just explictly set signal(SIGCHLD, SIG_IGN),
 *      but using a handler seems safer.
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-05-04  Jason Bacon Begin
 ***************************************************************************/

void    sigchld_handler(int s2)

{
    int     status;
    
    wait(&status);
}
