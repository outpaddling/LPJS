/***************************************************************************
 *  Description:
 *      Chaperone for running jobs on a compute node.  When jobs are
 *      dispatched to a compute node, lpjs-chaperone is executed and
 *      the actually command is run as a child of lpjs-chaperone.
 *
 *      lpjs-chaperone monitors its child to enforce resource limits
 *      and report exit status back to the dispatcher.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-27  Jason Bacon Begin
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
#include <errno.h>
#include <fcntl.h>          // open()
#include <sys/wait.h>       // FIXME: Replace wait() with active monitoring
#include <signal.h>
#include <sys/sysctl.h>
#include <sys/resource.h>   // setrlimit()

#include <xtend/string.h>
#include <xtend/file.h>
#include <xtend/proc.h>     // xt_get_home_dir()

#include "node-list.h"
#include "config.h"
#include "network.h"
#include "misc.h"
#include "lpjs.h"
#include "chaperone.h"

// Must be global for job cancel signal handler
pid_t   Pid;

int     main (int argc, char *argv[])

{
    int         status,
		push_status;
    unsigned    procs_per_job;
    unsigned long   pmem_per_proc;
    // Terminates process if malloc() fails, no check required
    node_list_t *node_list = node_list_new();
    char        *job_script_name,
		*temp,
		*end,
		*job_id,
		log_dir[PATH_MAX + 1 - 20],
		log_file[PATH_MAX + 1],
		wd[PATH_MAX + 1 - 20],
		hostname[sysconf(_SC_HOST_NAME_MAX) + 1],
		shared_fs_marker[PATH_MAX + 1],
		cmd[LPJS_CMD_MAX + 1],
		new_path[LPJS_PATH_ENV_MAX + 1],
		home_dir[PATH_MAX + 1];
    extern FILE *Log_stream;
    struct stat st;
    size_t      rss, peak_rss;
    struct rusage   rusage;

    signal(SIGHUP, chaperone_cancel_handler);
    
    if ( argc != 2 )
    {
	fprintf(stderr, "Usage: %s job-script.lpjs\n", argv[0]);
	fprintf(stderr, "Job parameters are retrieved from LPJS_* env vars.\n");
	return EX_USAGE;
    }
    job_script_name = argv[1];

    // Chaperone outputs stderr to a log file in the working dir
    job_id = getenv("LPJS_JOB_ID");
    lpjs_job_log_dir(getenv("LPJS_JOB_LOG_DIR"), strtoul(job_id, &end, 10),
		      log_dir, PATH_MAX + 1 - 20);

    snprintf(log_file, PATH_MAX + 1, "%s/chaperone-%s.stderr",
	     log_dir, job_id);
    if ( (Log_stream = lpjs_log_output(log_file, "w")) == NULL )
    {
	fprintf(stderr, "chaperone: Failed to create log file: %s.\n",
		log_file);
	return EX_CANTCREAT;
    }
    
    snprintf(new_path, LPJS_PATH_ENV_MAX + 1, "%s/bin:%s/bin:%s", LOCALBASE, PREFIX, getenv("PATH"));
    setenv("PATH", new_path, 1);
    // lpjs_debug("PATH = %s\n", new_path);
    
    xt_get_home_dir(home_dir, PATH_MAX + 1);
    setenv("LPJS_HOME_DIR", home_dir, 1);
    
    temp = getenv("LPJS_PROCS_PER_JOB");
    procs_per_job = strtoul(temp, &end, 10);
    if ( *end != '\0' )
    {
	fprintf(stderr, "Invalid LPJS_PROCS_PER_JOB: %s\n", temp);
	exit(EX_USAGE);
    }
    
    temp = getenv("LPJS_PMEM_PER_PROC");
    pmem_per_proc = strtoul(temp, &end, 10);
    if ( *end != '\0' )
    {
	fprintf(stderr, "Invalid LPJS_PMEM_PER_PROC: %s\n", temp);
	exit(EX_USAGE);
    }
    
    // Get hostname of head node
    lpjs_load_config(node_list, LPJS_CONFIG_HEAD_ONLY, stderr);
    
    gethostname(hostname, sysconf(_SC_HOST_NAME_MAX));
    getcwd(wd, PATH_MAX + 1 - 20);
    lpjs_log("%s(): Running %s in %s on %s with %u procs and %lu MiB.\n",
	    __FUNCTION__, job_script_name, wd, hostname, procs_per_job, pmem_per_proc);
    
    if ( stat(shared_fs_marker, &st) != 0 )
    {
	// FIXME: Implement input file transfer here
    }
    
    if ( (Pid = fork()) == 0 )
    {
	struct rlimit   rss_limit;
	
	// Create new process group with the script's PID
	// This helps track processes that are part of a job
	setpgid(0, getpid());
	
	// Suggest resource limits.  RSS cannot be controlled at all
	// on Linux, so this has no effect.  On BSD systems, RSS limits
	// influence pager behavior, so that processes exceeding their
	// RSS limit are preferentially paged out when memory is tight.
	// This will only come into play between total RSS samplings
	// for the job, which happen every few seconds. Might help
	// a little on occasion, though.
	rss_limit.rlim_cur = procs_per_job * pmem_per_proc * KIB_PER_MIB * BYTES_PER_KIB;
	rss_limit.rlim_max = procs_per_job * pmem_per_proc * KIB_PER_MIB * BYTES_PER_KIB;
	setrlimit(RLIMIT_RSS, &rss_limit);
	
	// Not yet working: Need to enforce total for process group
	// enforce_resource_limits(getpid(), pmem_per_proc);
    
	// Child, run script
	fclose(Log_stream); // Not useful to child
	execl(job_script_name, job_script_name, NULL);
	lpjs_log("%s(): Error: execl() failed: %s\n", __FUNCTION__, strerror(errno));
	return EX_UNAVAILABLE;
    }
    // No need for else since child calls execl() and exits if it fails
    
    lpjs_job_start_notice_loop(node_list, hostname, job_id, Pid);
    
    /*
     *  Monitor resource use of child.  xt_get_family_rss() returns -1 when
     *  pid no longer exists.  This is a fallback option for installations
     *  that cannot enforce resource limits at the OS level.  E.g., FreeBSD
     *  should use rctl if compd is running as root, and Linux should use
     *  cgroups, if possible.  If those systems are not enabled, the
     *  periodic sampling here should catch most resource violations
     *  soon enough to prevent major problems.
     */
    
    peak_rss = 0;
    while ( xt_get_family_rss(Pid, &rss) == 0 )
    {
	// pmem_per_proc is in MiB, rss in KiB
	if ( rss > procs_per_job * pmem_per_proc * KIB_PER_MIB)
	{
	    lpjs_log("%s(): Terminating job for resident memory violation: %zu KiB > %zu KiB\n",
		    __FUNCTION__, rss, procs_per_job * pmem_per_proc * KIB_PER_MIB);
	    // Termination of child processes should be enough
	    // This chaperone process will detect the
	    // exit using wait and report back to dispatchd
	    whack_family(Pid);
	    break;  // Exit loop and proceed to wait4()
	}
	if ( rss > peak_rss )
	{
	    peak_rss = rss;
	    lpjs_debug("%s(): Peak total job RSS = %zu\n", __FUNCTION__, rss);
	}
	sleep(5);   // FIXME: Make this tunable
    }
    
    // Get exit status of child process
    // FIXME: Record peak resource usage in job log
    // FIXME: Check for allocations much larger than used
    //        Blacklist script for greed until fixed
    wait4(Pid, &status, WEXITED, &rusage);
    lpjs_log("%s(): Info: Job process exited with status %d.\n", __FUNCTION__, status);

    lpjs_chaperone_completion_loop(node_list, hostname, job_id, status, peak_rss);
    
    // Transfer working dir to submit host or according to user
    // settings, if not shared
    lpjs_get_marker_filename(shared_fs_marker, getenv("LPJS_SUBMIT_HOST"),
			     PATH_MAX + 1);
    if ( stat(shared_fs_marker, &st) != 0 )
    {
	char    *sp, *submit_node, *submit_dir;
	size_t  c;
	
	chdir("..");    // Can't remove dir while in use
	
	if ( (sp = getenv("LPJS_PUSH_COMMAND")) == NULL )
	{
	    lpjs_log("%s(): Bug: No LPJS_PUSH_COMMAND in env.\n", __FUNCTION__);
	    exit(EX_SOFTWARE);
	}
	else
	    lpjs_log("%s(): LPJS_PUSH_COMMAND = %s\n", __FUNCTION__, sp);
	c =0;
	while ( (*sp != '\0') && (c < LPJS_CMD_MAX) )
	{
	    if ( *sp == '%' )
	    {
		++sp;
		switch(*sp)
		{
		    case    'w':
			if ( c + strlen(wd) > LPJS_CMD_MAX )
			{
			    lpjs_log("%s(): Error: LPJS_PUSH_COMMAND longer than %u, aborting.\n",
				    __FUNCTION__, LPJS_CMD_MAX);
			    return EX_DATAERR;
			}
			strlcpy(cmd + c, wd, LPJS_CMD_MAX + 1);
			c += strlen(wd);
			++sp;
			break;
			
		    case    'h':
			submit_node = getenv("LPJS_SUBMIT_HOST");
			if ( c + strlen(submit_node) > LPJS_CMD_MAX )
			{
			    lpjs_log("%s(): Error: LPJS_PUSH_COMMAND longer than %u, aborting.\n",
				    __FUNCTION__, LPJS_CMD_MAX);
			    return EX_DATAERR;
			}
			strlcpy(cmd + c, submit_node, LPJS_CMD_MAX + 1);
			c += strlen(submit_node);
			++sp;
			break;
			
		    case    'd':
			submit_dir = getenv("LPJS_SUBMIT_DIRECTORY");
			if ( c + strlen(submit_dir) > LPJS_CMD_MAX )
			{
			    lpjs_log("%s(): Error: LPJS_PUSH_COMMAND longer than %u, aborting.\n",
				    __FUNCTION__, LPJS_CMD_MAX);
			    return EX_DATAERR;
			}
			strlcpy(cmd + c, submit_dir, LPJS_CMD_MAX + 1);
			c += strlen(submit_dir);
			++sp;
			break;
			
		    default:
			lpjs_log("%s(): Error: Invalid placeholder in LPJS_PUSH_COMMAND: %%%c\n",
				__FUNCTION__, *sp);
			return EX_DATAERR;
		}
	    }
	    else
		cmd[c++] = *sp++;
	}
	*sp = '\0';
	lpjs_log("%s(): Transferring temporary working dir: %s\n",
		__FUNCTION__, wd);
	lpjs_log("%s(): push command = %s\n", __FUNCTION__, cmd);
	
	// No more lpjs_log() beyond here.  Log file already transferred.
	// Close log before pushing temp dir to ensure that it's complete
	fclose(Log_stream);
	push_status = system(cmd);
	
	// Remove temporary working dir if successfully transferred
	if ( push_status == 0 )
	{
	    lpjs_log("%s(): Removing temporary working dir...\n", __FUNCTION__);
	    snprintf(cmd, LPJS_CMD_MAX + 1, "rm -rf %s", wd);
	    system(cmd);    // Log is closed, no point checking status
	}
	else
	{
	    // Mark this directory
	    // FIXME: Check time stamps on markers and remove them if expired
	    char    marker[PATH_MAX + 1];
	    int     fd;
	    
	    snprintf(marker, PATH_MAX + 1, "%s/lpjs-remove-me", wd);
	    if ( (fd = open(marker, O_WRONLY|O_CREAT, 0644)) != -1 )
		close(fd);
	}
    }

    lpjs_log("%s(): Info: Chaperone exiting with status %d...\n", __FUNCTION__, status);
    return status;
}


/***************************************************************************
 *  Description:
 *  
 *  Returns:
 *      LPJS_SUCCESS, etc.
 *
 *  History: 
 *  Date        Name        Modification
 *  ~2024-05-01 Jason Bacon Begin
 ***************************************************************************/

int     lpjs_job_start_notice(int msg_fd,
			       const char *hostname, const char *job_id,
			       pid_t job_pid)

{
    char        outgoing_msg[LPJS_MSG_LEN_MAX + 1],
		*munge_payload;
    ssize_t     bytes;
    uid_t       uid;
    gid_t       gid;
    extern FILE *Log_stream;
    
    /* Send a message to the server */
    /* Need to send \0, so xt_dprintf() doesn't work here */
    snprintf(outgoing_msg, LPJS_MSG_LEN_MAX + 1,
	    "%c%s %s %u %d", LPJS_DISPATCHD_REQUEST_JOB_STARTED,
	    hostname, job_id, getpid(), job_pid);
    lpjs_log("%s(): Sending new PIDs to dispatchd:\n", __FUNCTION__);
    lpjs_debug("%s(): msg = %s\n", __FUNCTION__, outgoing_msg + 1);
    if ( lpjs_send_munge(msg_fd, outgoing_msg, close) != LPJS_MSG_SENT )
    {
	lpjs_log("%s(): Error: Failed to send job start message to dispatchd: %s",
		__FUNCTION__, strerror(errno));
	return LPJS_WRITE_FAILED;
    }
    lpjs_debug("%s(): Sent job start request.\n", __FUNCTION__);

    // lpjs_recv(msg_fd, incoming_msg, LPJS_MSG_LEN_MAX, 0, 0);
    // FIXME: Add a timeout and handling code
    bytes = lpjs_recv_munge(msg_fd, &munge_payload, 0, 0, &uid, &gid, close);

    if ( bytes < 1 )
    {
	lpjs_log("%s(): Error: Failed to receive auth message.\n", __FUNCTION__);
	exit(EX_IOERR); // FIXME: Should we retry?
    }
    else if ( strcmp(munge_payload, "Node authorized") != 0 )
    {
	lpjs_log("%s(): Error: This node is not authorized to connect.\n"
		 "It must be added to the etc/lpjs/config on the head node.\n",
		 __FUNCTION__);
	exit(EX_NOPERM);
    }
    else
	lpjs_log("%s(): Received authorization from lpjs_dispatchd.\n",
		__FUNCTION__);
    free(munge_payload);
    
    return LPJS_SUCCESS;
}


/***************************************************************************
 *  Description:
 *      Connect to dispatchd and send job start notification.
 *      Retry indefinitely if failure occurs.
 *
 *  Returns:
 *      File descriptor for ongoing connection to dispatchd.
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-23  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_job_start_notice_loop(node_list_t *node_list,
				    const char *hostname,
				    const char *job_id, pid_t job_pid)

{
    int     msg_fd,
	    status;
    
    /*
     *  Retry socket connection and job start request indefinitely.
     *  Close msg_fd if request fails, to prevent deamons from
     *  hanging on an open socket connection.
     */
    
    do
    {
	msg_fd = lpjs_connect_to_dispatchd(node_list);
	if ( msg_fd == -1 )
	{
	    lpjs_log("%s(): Error: Failed to connect to dispatchd: %s\n",
		    __FUNCTION__, strerror(errno));
	    lpjs_log("%s(): Retry in %d seconds...\n",
		    __FUNCTION__, LPJS_RETRY_TIME);
	    sleep(LPJS_RETRY_TIME);
	}
	else
	{
	    status = lpjs_job_start_notice(msg_fd, hostname, job_id, job_pid);
	    if ( status != LPJS_SUCCESS )
	    {
		lpjs_log("%s(): Error: Chaperone start notice failed.\n",
			__FUNCTION__);
		lpjs_log("%s(): Retry in %d seconds...\n",
			 __FUNCTION__, LPJS_RETRY_TIME);
		sleep(LPJS_RETRY_TIME);
	    }
	    close(msg_fd);
	}
    }   while ( (msg_fd == -1) || (status != LPJS_SUCCESS) );
    
    lpjs_debug("%s(): Start notice successful.\n", __FUNCTION__);
    
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
 *  2024-05-04  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_chaperone_completion(int msg_fd, const char *hostname,
				  const char *job_id, int status,
				  size_t peak_rss)

{
    char    outgoing_msg[LPJS_MSG_LEN_MAX + 1];
    
    /* Send job completion message to dispatchd */
    // FIXME: Send collected stats: rss, user time, sys time,
    // elapsed time, etc.
    snprintf(outgoing_msg, LPJS_MSG_LEN_MAX + 1, "%c%s %s %d %zu\n",
	     LPJS_DISPATCHD_REQUEST_JOB_COMPLETE, hostname,
	     job_id, status, peak_rss);
    if ( lpjs_send_munge(msg_fd, outgoing_msg, close) != LPJS_MSG_SENT )
    {
	lpjs_log("%s(): Failed to send message to dispatchd: %s",
		__FUNCTION__, strerror(errno));
	return EX_IOERR;
    }
    
    return EX_OK;
}


/***************************************************************************
 *  Description:
 *      Connect to dispatchd and send job completion request.
 *      Retry indefinitely if failure occurs.
 *
 *  Returns:
 *      File descriptor for ongoing connection to dispatchd.
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-05-04  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_chaperone_completion_loop(node_list_t *node_list,
				    const char *hostname,
				    const char *job_id, int status,
				    size_t peak_rss)

{
    int     msg_fd;
    
    // Retry socket connection and message send indefinitely
    do
    {
	msg_fd = lpjs_connect_to_dispatchd(node_list);
	if ( msg_fd == -1 )
	{
	    lpjs_log("%s(): Error: Failed to connect to dispatchd: %s\n",
		    __FUNCTION__, strerror(errno));
	    lpjs_log("%s(): Retry in %d seconds...\n",
		    __FUNCTION__, LPJS_RETRY_TIME);
	    sleep(LPJS_RETRY_TIME);
	}
	else
	{
	    status = lpjs_chaperone_completion(msg_fd, hostname, job_id,
						status, peak_rss);
	    if ( status != EX_OK )
	    {
		lpjs_log("%s(): Error: Message send failed.\n",
			__FUNCTION__);
		lpjs_log("%s(): Retry in %d seconds...\n",
			 __FUNCTION__, LPJS_RETRY_TIME);
		sleep(LPJS_RETRY_TIME);
	    }
	    close(msg_fd);
	}
    }   while ( (msg_fd == -1) || (status != EX_OK) );
    lpjs_log("%s(): Completion report sent.\n", __FUNCTION__);
    
    return 0;   // FIXME: Define return codes
}


void    chaperone_cancel_handler(int s2)

{
    lpjs_log("%s(): Chaperone PID %d canceling job PID %d...\n",
	    __FUNCTION__, getpid(), Pid);
    
    /*
     *  Terminate mafia-style: Don't just terminate the process, go
     *  after his family as well.  Although chaperone
     *  creates a process group for the LPJS script, killpg() will
     *  not work if programs run by the script create process groups
     *  as well.  So we need to traverse the process tree, depth-first,
     *  kill children before their parents to ensure that all descendent
     *  processes are discovered.
     */
    
    whack_family(Pid);
}




/***************************************************************************
 *  Description:
 *      Kill descendents of a process in depth-first order
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-05-08  Jason Bacon Begin
 ***************************************************************************/

void    whack_family(pid_t pid)

{
    char    cmd[LPJS_CMD_MAX + 1],
	    pid_str[100],
	    *end;
    FILE    *fp;
    pid_t   child_pid;
    
    /*
     *  Get list of child processes in a kludgey, but portable way.
     *  It would be nice if there were a standard API for identifying
     *  members of a process group, but much of the functionality of
     *  ps(1) is sadly not exposed under POSIX.
     */
    
    // Recursively signal each child to terminate first
    lpjs_log("%s(): Finding children of %d...\n", __FUNCTION__, pid);
    snprintf(cmd, LPJS_CMD_MAX + 1, "pgrep -P %u", pid);
    if ( (fp = popen(cmd, "r")) == NULL )
    {
	lpjs_log("%s(): Error: Failed to run %s.\n", __FUNCTION__, cmd);
	return;
    }
    while ( fgets(pid_str, 100, fp) != NULL )
    {
	// FIXME: Check success
	child_pid = strtoul(pid_str, &end, 10);
	
	// Depth-first recursive traversal of process tree
	if ( child_pid != pid )
	    whack_family(child_pid);
	else
	    lpjs_log("%s(): Bug: child_pid %d = pid %d\n",
		    __FUNCTION__, child_pid, pid);
    }        
    pclose(fp);

    // Now that the children whacked, terminate this process
    // Try SIGTERM first in case process catches it and cleans up
    lpjs_log("%s(): Sending SIGTERM to PID %d...\n", __FUNCTION__, pid);
    kill(pid, SIGTERM);
    usleep(250000); // FIXME: Find a reasonable value
    // If SIGTERM worked, this will fail and have no effect
    lpjs_log("%s(): Sending SIGKILL to PID %d...\n", __FUNCTION__, pid);
    kill(pid, SIGKILL);
}


void    enforce_resource_limits(pid_t pid, size_t mem_per_proc)

{
#if defined(__APPLE__)
#elif defined(__DragonFly__)
#elif defined(__FreeBSD__)
    
    // Use rctl
    // FIXME: This needs to be run as root
    // FIXME: Need to enforce limit for the entire process group,
    //        not just the child of chaperone
    #include <sys/rctl.h>
    int     enabled;
    size_t  len = sizeof(int);
    char    rule[LPJS_RCTL_RULE_MAX + 1];
    
    if ( sysctlbyname("kern.racct.enable", &enabled, &len, NULL, 0) != 0 )
	lpjs_log("%s(): sysctl kern.racct.enable failed.\n", __FUNCTION__);
    else
	if ( enabled == 1 )
	{
	    // Limit RSS to mem_per_proc MiB
	    snprintf(rule, LPJS_RCTL_RULE_MAX + 1,
		    "process:%d:memoryuse:deny=%zu",
		    pid, mem_per_proc * KIB_PER_MIB * BYTES_PER_KIB);
	    lpjs_log("%s(): Adding rctl rule: %s\n", __FUNCTION__, rule);
	    if ( rctl_add_rule(rule, LPJS_RCTL_RULE_MAX + 1, NULL, 0) != 0 )
		lpjs_log("%s(): rctl_add_rule() failed: %s\n",
			__FUNCTION__, strerror(errno));
	}
	else
	    lpjs_log("%s(): kern.racct.enable is not enabled.\n", __FUNCTION__);

#elif defined(__Linux__)
    // Use cgroups?
#elif defined(__NetBSD__)
#elif defined(__OpenBSD__)
#endif
}


/***************************************************************************
 *  Use auto-c2man to generate a man page from this comment
 *
 *  Section:
 *      3
 *
 *  Library:
 *      #include <xtend/proc.h>
 *      -lxtend
 *
 *  Description:
 *      Sample resource usage of an arbitrary process.
 *  
 *  Arguments:
 *      pid     Process ID of the process to sample
 *      rss     Resident memory use of pid returned to caller
 *
 *  Returns:
 *      SUCCESS, NO_SUCH_PID
 *
 *  Examples:
 *
 *  See also:
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-12-22  Jason Bacon Begin
 ***************************************************************************/

int     xt_get_family_rss(pid_t pid, size_t *rss)

{
    int     status, items;
    char    ps_output[PATH_MAX + 1],
	    pid_str[LPJS_MAX_INT_DIGITS + 1];
    FILE    *group_fp, *rss_fp;
    extern FILE *Log_stream;
    char    cmd[LPJS_CMD_MAX + 1],
	    *end;
    pid_t   child_pid;
    size_t  proc_rss;
    
    // Maybe ptrace(), though seemingly not well standardized
    // FIXME: Find a way to detect processor oversubscription
    
    /*
     *  Get RSS of the job process and all children
     */
     
    /*
     *  Get list of child processes in a kludgey, but portable way.
     *  It would be nice if there were a standard API for identifying
     *  members of a process group, but much of the functionality of
     *  ps(1) is sadly not exposed under POSIX.
     */
    
    // lpjs_debug("%s(): Finding children of %d...\n", __FUNCTION__, pid);
    snprintf(cmd, LPJS_CMD_MAX + 1, "pgrep -P %u", pid);
    
    // Signal each child to terminate
    if ( (group_fp = popen(cmd, "r")) == NULL )
    {
	lpjs_log("%s(): Error: Failed to run %s.\n", __FUNCTION__, cmd);
	return -1;  // FIXME: Define return codes
    }
    
    *rss = 0;
    while ( fgets(pid_str, LPJS_MAX_INT_DIGITS + 1, group_fp) != NULL )
    {
	// FIXME: Check success
	child_pid = strtoul(pid_str, &end, 10);
	// lpjs_debug("%d is a child of %d\n", child_pid, pid);
	
	// Depth-first recursive traversal of process tree
	if ( child_pid != pid )
	{
	    xt_get_family_rss(child_pid, &proc_rss);
	    *rss += proc_rss;   // Add sum of child RSSs from recursion
	}
	else
	    lpjs_debug("%s(): Bug: child_pid %d = pid %d\n", child_pid, pid);
    }
    pclose(group_fp);

    /*
     *  Done counting children, now count this PID
     */
    
    /*
     *  There does not seem to be a standard API for gathering process
     *  info, so we spawn a separate "ps" process and read stdout.
     *  This is kludgy, but portable.  Interface is separate from
     *  implementation, so we can improve on this if/when opportunity
     *  knocks without messing with anything else.
     */

    snprintf(ps_output, PATH_MAX + 1, "%d-ps-stdout", pid);
    // FIXME: Also collect other stats
    // -o usertime= -o systime=
    // keywords are not portable, e.g. no usertime= or systime= on Linux
    // time= is formatted differently on BSD and Linux
    status = xt_spawnlp(P_WAIT, P_NOECHO, NULL, ps_output, NULL, "ps", "-p",
	    xt_ltostrn(pid_str, pid, 10, LPJS_MAX_INT_DIGITS + 1),
	    "-o", "rss=", NULL);
    // lpjs_debug("xt_spawnlp() returned %d\n", status);
    if ( status != 0 )
	lpjs_log("%s(): ps failed.\n", __FUNCTION__);
    else
    {
	// lpjs_debug("Opening %s...\n", ps_output);
	if ( (rss_fp = fopen(ps_output, "r")) == NULL )
	{
	    lpjs_log("%s(): Bug: Cannot open %s: %s\n", __FUNCTION__,
		    ps_output, strerror(errno));
	}
	else
	{
	    // FIXME: fscanf() != 1 is not working as a check for failed ps
	    // lpjs_debug("Reading %s...\n", ps_output);
	    items = fscanf(rss_fp, "%zu", &proc_rss);
	    // lpjs_debug("%s(): fscanf() read %d items.\n", __FUNCTION__, items);
	    if ( (items != 1) || (proc_rss == 0) )
		// No such process, time to terminate
		status = -1;    // FIXME: Define return codes
	    else
	    {
		status = 0;
		*rss += proc_rss;
		// lpjs_debug("+%zu (proc %d) = %zu.\n", proc_rss, pid, *rss);
	    }
	    fclose(rss_fp);
	}
    }
    unlink(ps_output);
    return status;
}
