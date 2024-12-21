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
    unsigned    procs;
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
    if ( (Log_stream = lpjs_log_output(log_file)) == NULL )
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
    procs = strtoul(temp, &end, 10);
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
    
    // FIXME: Use fork() and exec() or posix_spawn(), and monitor
    // the child process for resource use
    // xt_str_argv_cat(cmd, argv, 1, LPJS_CMD_MAX + 1);
    // status = system(cmd);
    
    gethostname(hostname, sysconf(_SC_HOST_NAME_MAX));
    getcwd(wd, PATH_MAX + 1 - 20);
    lpjs_log("%s(): Running %s in %s on %s with %u procs and %lu MiB.\n",
	    __FUNCTION__, job_script_name, wd, hostname, procs, pmem_per_proc);
    
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
	rss_limit.rlim_cur = pmem_per_proc * 1024 * 1024;
	rss_limit.rlim_max = pmem_per_proc * 1024 * 1024;
	setrlimit(RLIMIT_RSS, &rss_limit);
	
	enforce_resource_limits(getpid(), pmem_per_proc);
    
	// Child, run script
	// FIXME: Set CPU and memory (virtual and physical) limits
	fclose(Log_stream); // Not useful to child
	execl(job_script_name, job_script_name, NULL);
	lpjs_log("%s(): Error: execl() failed: %s\n", __FUNCTION__, strerror(errno));
	return EX_UNAVAILABLE;
    }
    // No need for else since child calls execl() and exits if it fails
    
    lpjs_job_start_notice_loop(node_list, hostname, job_id, Pid);
    
    // FIXME: Monitor resource use of child
    // Maybe ptrace(), though seemingly not well standardized
    // Kludgy, but portable:
    // rss=<nothing> disables the RSS header line
    // while (status == 0 ) {
    // fork, exec(ps child-pid -o rss=)
    // sleep(1) }
    wait(&status);
    lpjs_log("%s(): Info: Process exited with status %d.\n", __FUNCTION__, status);

    lpjs_chaperone_completion_loop(node_list, hostname, job_id, status);
    
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

    lpjs_log("%s(): Info: Exiting with status %d...\n", __FUNCTION__, status);
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
				  const char *job_id, int status)

{
    char    outgoing_msg[LPJS_MSG_LEN_MAX + 1];
    
    /* Send job completion message to dispatchd */
    snprintf(outgoing_msg, LPJS_MSG_LEN_MAX + 1, "%c%s %s %d\n",
	     LPJS_DISPATCHD_REQUEST_JOB_COMPLETE, hostname,
	     job_id, status);
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
				    const char *job_id, int status)

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
						status);
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
    lpjs_log("%s(): Canceling PID %d...\n", __FUNCTION__, Pid);
    
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
    
    // Try SIGTERM first in case process catches it and cleans up
    lpjs_log("%s(): Sending SIGTERM to %d...\n", __FUNCTION__, Pid);
    kill(Pid, SIGTERM);
    sleep(2);
    // If SIGTERM worked, this will fail and have no effect
    lpjs_log("%s(): Sending SIGKILL to %d...\n", __FUNCTION__, Pid);
    kill(Pid, SIGKILL);
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
    
    // Get list of child processes in a kludgy, but portable way
    lpjs_log("%s(): Finding children of %d...\n", __FUNCTION__, pid);
    snprintf(cmd, LPJS_CMD_MAX + 1, "pgrep -P %u", pid);
    
    // Signal each child to terminate
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
	
	// Try SIGTERM first in case process catches it and cleans up
	lpjs_log("%s(): Sending SIGTERM to child %d...\n",
		__FUNCTION__, child_pid);
	kill(child_pid, SIGTERM);
	sleep(2);
	// If SIGTERM worked, this will fail and have no effect
	lpjs_log("%s(): Sending SIGKILL to child %d...\n",
		__FUNCTION__, child_pid);
	kill(child_pid, SIGKILL);
    }
    pclose(fp);
}


void    enforce_resource_limits(pid_t pid, size_t mem_per_proc)

{
#if defined(__APPLE__)
#elif defined(__DragonFly__)
#elif defined(__FreeBSD__)
    
    // Use rctl
    // FIXME: This needs to be run as root
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
		    pid, mem_per_proc * 1024 * 1024);
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
