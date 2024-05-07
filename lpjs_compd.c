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
		vis_msg[LPJS_MSG_LEN_MAX + 1],
		dispatch_response[LPJS_MSG_LEN_MAX + 1];
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
    
    snprintf(Pid_path, PATH_MAX + 1, "%s/%s.pid", LPJS_RUN_DIR, "lpjs_compd");
    status = xt_create_pid_file(Pid_path, Log_stream);
    if ( status != EX_OK )
	return status;
#endif

    // Get hostname of head node
    lpjs_load_config(node_list, LPJS_CONFIG_HEAD_ONLY, Log_stream);

    msg_fd = lpjs_compd_checkin_loop(node_list, node);
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
	    msg_fd = lpjs_compd_checkin_loop(node_list, node);
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
	    // lpjs_log("Received %zd bytes from dispatchd: \"%s\"\n", bytes, vis_msg);
	    
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
		msg_fd = lpjs_compd_checkin_loop(node_list, node);
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
		msg_fd = lpjs_compd_checkin_loop(node_list, node);
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
		// lpjs_log("New job received:\n");
		// job_print(job, Log_stream);
		// lpjs_log("Script:\n%s", script_start);
		// FIXME: Use submitter uid and gid, not dispatchd
		if ( lpjs_run_script(job, script_start) != EX_OK )
		{
		    lpjs_log("Failed to start script.\n");
		    snprintf(dispatch_response, LPJS_MSG_LEN_MAX + 1,
			    "%c", LPJS_DISPATCH_FAILED);
		}
		else
		    snprintf(dispatch_response, LPJS_MSG_LEN_MAX + 1,
			    "%c", LPJS_DISPATCH_OK);
		lpjs_send_munge(msg_fd, dispatch_response);
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
	    "%c%s", LPJS_DISPATCHD_REQUEST_COMPD_CHECKIN,
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


/***************************************************************************
 *  Description:
 *  
 *  Returns:
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-10  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_run_script(job_t *job, const char *script_start)

{
    char    temp_wd[PATH_MAX + 1],
	    log_dir[PATH_MAX + 1],
	    job_script_name[PATH_MAX + 1],
	    shared_fs_marker[PATH_MAX + 1],
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
    
    working_dir = job_get_submit_directory(job);
    
    lpjs_get_marker_filename(shared_fs_marker, job_get_submit_node(job),
			     PATH_MAX + 1);
    snprintf(shared_fs_marker_path, PATH_MAX + 1, "%s/%s",
	     working_dir, shared_fs_marker);
    if ( stat(shared_fs_marker_path, &st) != 0 )
    {
	struct passwd *pw_ent;
	
	// Use pwnam_r() if multithreading, not likely
	if ( (pw_ent = getpwnam(job_get_user_name(job))) == NULL )
	{
	    lpjs_log("No such user: %s\n", job_get_user_name(job));
	    // FIXME: Report job failure to dispatchd
	}
	else
	{
	    // Place temp working dirs in user's home dir
	    // FIXME: Check for failures
	    chdir(pw_ent->pw_dir);
	    
	    // FIXME: Remove LPJS-job-* from previous submissions
	    // This should replace temp workdir removal in chaperone
	    
	    snprintf(temp_wd, PATH_MAX + 1, "LPJS-job-%lu",
		    job_get_job_id(job));
	    lpjs_log("%s does not exist.  Using temp dir %s.\n",
		    working_dir, temp_wd);
	    
	    // If temp_wd exists, rename it first
	    if ( stat(temp_wd, &st) == 0 )
	    {
		char    save_wd[PATH_MAX + 1];
		int c = 0;
		do
		{
		    snprintf(save_wd, PATH_MAX + 1, "%s.%d", temp_wd, c++);
		}   while ( stat(save_wd, &st) == 0 );
		// lpjs_log("Renaming %s to %s\n", temp_wd, save_wd);
		rename(temp_wd, save_wd);
	    }
	    
	    mkdir(temp_wd, 0700);
	    working_dir = temp_wd;
	    
	    if ( getuid() == 0 )
		lpjs_chown(job, working_dir);
	    else
		lpjs_log("lpjs_compd running as uid %d, can't change working dir ownership.\n", getuid());
	}
    }
    
    lpjs_log("Changing to %s...\n", working_dir);
    if ( chdir(working_dir) != 0 )
    {
	lpjs_log("Failed to enter working dir: %s\n", working_dir);
	// FIXME: Notify dispatchd of job failure
    }
    getcwd(temp_wd, PATH_MAX + 1);
    lpjs_log("CWD = %s\n", temp_wd);
    lpjs_log("If CWD above printed as NULL, this is errno: %s\n", strerror(errno));
    if ( getcwd(temp_wd, PATH_MAX + 1) == NULL )
    {
	lpjs_log("chdir() failed: errno = %s\n", strerror(errno));
	#ifdef __APPLE__
	lpjs_log("You may need to grant lpjs_compd full disk access in\n"
		 "System Preferences -> Privacy and Security".  This\n"
		 "access might be revoked when LPJS is updated.  If you\n"
		 "find that you might repeatedly reset it, please report\n"
		 "the problem to Apple via the developer feedback assistant.\n");
	#endif
	return EX_NOPERM;
    }
    
    /*
     *  Save script
     */
    
    // FIXME: This is duplicated, factor it out
    snprintf(log_dir, PATH_MAX + 1, "LPJS-job-%lu-logs", job_get_job_id(job));

    mkdir(log_dir, 0700);
    snprintf(job_script_name, PATH_MAX + 1, "%s/%s",
	    log_dir, job_get_script_name(job));
    lpjs_log("Saving job script to %s.\n", job_script_name);
    if ( (fd = open(job_script_name, O_WRONLY|O_CREAT|O_TRUNC, 0700)) == -1 )
    {
	lpjs_log("Cannot create %s: %s\n", job_script_name, strerror(errno));
	// FIXME: Report job failure to dispatchd
    }
    write(fd, script_start, strlen(script_start));
    close(fd);
    
    /*
     *  Make sure script is owned by the submitting user.  If lpjs_compd
     *  is running as non-root, then only that user can run jobs.
     *  If running as root, chown the script to the appropriate user.
     */
    
    if ( getuid() == 0 )
    {
	lpjs_chown(job, log_dir);
	lpjs_chown(job, job_script_name);
    }
    else
	lpjs_log("lpjs_compd running as uid %d, can't change script ownership.\n", getuid());
    
    /*
     *  FIXME: Update node status (keep a copy here in case
     *  dispatchd is restarted)
     */
    
    /*
     *  Run script under chaperone
     */
    
    run_chaperone(job, job_script_name);
    
    return EX_OK;
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

int     run_chaperone(job_t *job, const char *job_script_name)

{
    char        *chaperone_bin = PREFIX "/libexec/lpjs/chaperone",
		out_file[PATH_MAX + 1],
		err_file[PATH_MAX + 1];
    extern FILE *Log_stream;
    
    signal(SIGCHLD, sigchld_handler);
    
    if ( fork() == 0 )
    {
	/*
	 *  Child, exec the chaperone command with the script as an arg.
	 *  The chaperone runs in the background, monitoring the job,
	 *  enforcing resource limits, and reporting exit status and
	 *  stats to dispatchd.
	 */
	
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
		lpjs_log("%s(): %s: No such user.\n", __FUNCTION__, user_name);
		// FIXME: Disable this node and reschedule this job
		return EX_NOUSER;
	    }
	    
	    group_name = job_get_primary_group_name(job);
	    if ( (gr_ent = getgrnam(group_name)) == NULL )
	    {
		lpjs_log("%s(): Info: %s: No such group.\n", __FUNCTION__, group_name);
		gid = pw_ent->pw_gid;
	    }
	    else
		gid = gr_ent->gr_gid;
	    
	    // Set gid before uid, while still running as root
	    if ( setgid(gid) != 0 )
		lpjs_log("%s(): Warning: Failed to set gid to %u.\n", __FUNCTION__, gid);
	    
	    uid = pw_ent->pw_uid;
	    if ( setuid(uid) != 0 )
	    {
		lpjs_log("%s(): Failed to set uid to %u.\n", __FUNCTION__, uid);
		// FIXME: Disable this node and reschedule this job
		return EX_NOPERM;
	    }
	    
	    lpjs_log("%s(): user = %s  group = %s\n", __FUNCTION__,
		    user_name, group_name);
	    lpjs_log("%s(): uid = %u  gid = %u\n", __FUNCTION__, uid, gid);
	}
	
	/*
	 *  Set env vars
	 */
	job_setenv(job);
	
	// FIXME: Make sure filenames are not truncated
	
	// Redirect stdout
	strlcpy(out_file, job_script_name, PATH_MAX + 1);
	strlcat(out_file, ".stdout", PATH_MAX + 1);
	close(1);
	open(out_file, O_WRONLY|O_CREAT, 0644);
	
	// Redirect stderr
	strlcpy(err_file, job_script_name, PATH_MAX + 1);
	strlcat(err_file, ".stderr", PATH_MAX + 1);
	close(2);
	if ( open(err_file, O_WRONLY|O_CREAT, 0644) == -1 )
	{
	    lpjs_log("%s(): Could not open %s: %s\n", __FUNCTION__,
		     err_file, strerror(errno));
	}
	
	// FIXME: Build should use realpath
	lpjs_log("Running chaperone: %s %s...\n", chaperone_bin, job_script_name);
	fflush(Log_stream);
	// FIXME: Failing on macOS
	execl(chaperone_bin, chaperone_bin, job_script_name, NULL);
	
	// We only get here if execl() failed
	lpjs_log("%s(): Failed to exec %s %u %u %s\n",
		__FUNCTION__, chaperone_bin, job_script_name);
	// FIXME: Disable this node and reschedule this job
	return EX_UNAVAILABLE;
    }
    
    /*
     *  lpjs_compd does not wait for chaperone, but resumes listening
     *  for more jobs.
     */

    // FIXME: Reap exited chaperone processes
    
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
	lpjs_log("User %u changing ownership of %s to group %u.\n",
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
