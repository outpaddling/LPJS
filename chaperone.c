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

#include <xtend/string.h>
#include <xtend/file.h>

#include "node-list.h"
#include "config.h"
#include "network.h"
#include "misc.h"
#include "lpjs.h"
#include "chaperone-protos.h"

// Must be global for job cancel signal handler
pid_t   Pid;

int     main (int argc, char *argv[])

{
    int         status,
		push_status;
    unsigned    procs;
    unsigned long   mem_per_proc;
    // Terminates process if malloc() fails, no check required
    node_list_t *node_list = node_list_new();
    char        *job_script_name,
		*temp,
		*end,
		*job_id,
		log_dir[PATH_MAX + 1],
		log_file[PATH_MAX + 1],
		wd[PATH_MAX + 1],
		hostname[sysconf(_SC_HOST_NAME_MAX) + 1],
		shared_fs_marker[PATH_MAX + 1],
		cmd[LPJS_CMD_MAX + 1],
		new_path[4096];
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
		      log_dir, PATH_MAX + 1);

    snprintf(log_file, PATH_MAX + 1, "%s/chaperone-%s.stderr",
	     log_dir, job_id);
    if ( (Log_stream = lpjs_log_output(log_file)) == NULL )
    {
	fprintf(stderr, "chaperone: Failed to create log file.\n");
	return EX_CANTCREAT;
    }
    
    snprintf(new_path, 4096, "%s/bin:%s/bin:%s", LOCALBASE, PREFIX, getenv("PATH"));
    setenv("PATH", new_path, 1);
    // lpjs_log("PATH = %s\n", new_path);
    
    temp = getenv("LPJS_CORES_PER_JOB");
    procs = strtoul(temp, &end, 10);
    if ( *end != '\0' )
    {
	fprintf(stderr, "Invalid LPJS_CORES_PER_JOB: %s\n", temp);
	exit(EX_USAGE);
    }
    
    temp = getenv("LPJS_MEM_PER_CORE");
    mem_per_proc = strtoul(temp, &end, 10);
    if ( *end != '\0' )
    {
	fprintf(stderr, "Invalid LPJS_MEM_PER_CORE: %s\n", temp);
	exit(EX_USAGE);
    }
    
    // Get hostname of head node
    lpjs_load_config(node_list, LPJS_CONFIG_HEAD_ONLY, stderr);
    
    // FIXME: Use fork() and exec() or posix_spawn(), and monitor
    // the child process for resource use
    // xt_str_argv_cat(cmd, argv, 1, LPJS_CMD_MAX + 1);
    // status = system(cmd);
    
    gethostname(hostname, sysconf(_SC_HOST_NAME_MAX));
    getcwd(wd, PATH_MAX + 1);
    lpjs_log("Running %s in %s on %s with %u procs and %lu MiB.\n",
	    job_script_name, wd, hostname, procs, mem_per_proc);
    
    if ( (Pid = fork()) == 0 )
    {
	// Create new process group with the script's PID
	setpgid(0, getpid());
	
	// Child, run script
	// FIXME: Set CPU and memory (virtual and physical) limits
	fclose(Log_stream); // Not useful to child
	execl(job_script_name, job_script_name, NULL);
	lpjs_log("%s(): execl() failed: %s\n", __FUNCTION__, strerror(errno));
	return EX_UNAVAILABLE;
    }
    
    lpjs_chaperone_checkin_loop(node_list, hostname, job_id, Pid);
    
    // FIXME: Chaperone, monitor resource use of child
    // Maybe ptrace(), though seemingly not well standardized
    wait(&status);
    lpjs_log("Process exited with status %d.\n", status);

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
	    lpjs_log("No LPJS_PUSH_COMMAND in env.  This is a bug.\n");
	    exit(EX_SOFTWARE);
	}
	lpjs_log("LPJS_PUSH_COMMAND = %s\n", sp);
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
			    lpjs_log("LPJS_PUSH_COMMAND longer than %u, aborting.\n", LPJS_CMD_MAX);
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
			    lpjs_log("LPJS_PUSH_COMMAND longer than %u, aborting.\n", LPJS_CMD_MAX);
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
			    lpjs_log("LPJS_PUSH_COMMAND longer than %u, aborting.\n", LPJS_CMD_MAX);
			    return EX_DATAERR;
			}
			strlcpy(cmd + c, submit_dir, LPJS_CMD_MAX + 1);
			c += strlen(submit_dir);
			++sp;
			break;
			
		    default:
			lpjs_log("Invalid placeholder in LPJS_PUSH_COMMAND: %%%c\n", *sp);
			return EX_DATAERR;
		}
	    }
	    else
		cmd[c++] = *sp++;
	}
	*sp = '\0';
	lpjs_log("Transferring temporary working dir: %s\n", wd);
	lpjs_log("push command = %s\n", cmd);
	
	// No more lpjs_log() beyond here.  Log file already transferred.
	// Close log before pushing temp dir to ensure that it's complete
	fclose(Log_stream);
	push_status = system(cmd);
	
	// Remove temporary working dir if successfully transferred
	if ( push_status == 0 )
	{
	    lpjs_log("Removing temporary working dir...\n");
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

    lpjs_log("Exiting with status %d...\n", status);
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

int     lpjs_chaperone_checkin(int msg_fd,
			       const char *hostname, const char *job_id,
			       pid_t job_pid)

{
    char        outgoing_msg[LPJS_MSG_LEN_MAX + 1],
		incoming_msg[LPJS_MSG_LEN_MAX + 1];
    extern FILE *Log_stream;
    
    /* Send a message to the server */
    /* Need to send \0, so xt_dprintf() doesn't work here */
    snprintf(outgoing_msg, LPJS_MSG_LEN_MAX + 1,
	    "%c%s %s %u %d", LPJS_DISPATCHD_REQUEST_CHAPERONE_CHECKIN,
	    hostname, job_id, getpid(), job_pid);
    lpjs_log("%s(): Sending PIDs to dispatchd:\n", __FUNCTION__);
    fprintf(Log_stream, "%s\n", outgoing_msg + 1);
    if ( lpjs_send_munge(msg_fd, outgoing_msg, close) != EX_OK )
    {
	lpjs_log("Failed to send checkin message to dispatchd: %s",
		strerror(errno));
	return LPJS_WRITE_FAILED;
    }
    lpjs_log("%s(): Sent checkin request.\n", __FUNCTION__);

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

    return LPJS_SUCCESS;
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

int     lpjs_chaperone_checkin_loop(node_list_t *node_list,
				    const char *hostname,
				    const char *job_id, pid_t job_pid)

{
    int     msg_fd,
	    status;
    
    /*
     *  Retry socket connection and checkin request indefinitely.
     *  Close msg_fd if checkin fails, to prevent deamons from
     *  hanging on an open socket connection.
     */
    
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
	    status = lpjs_chaperone_checkin(msg_fd, hostname, job_id, job_pid);
	    if ( status != LPJS_SUCCESS )
	    {
		lpjs_log("%s(): chaperone-checkin failed.  Retry in %d seconds...\n",
			 __FUNCTION__, LPJS_RETRY_TIME);
		sleep(LPJS_RETRY_TIME);
	    }
	    close(msg_fd);
	}
    }   while ( (msg_fd == -1) || (status != LPJS_SUCCESS) );
    
    lpjs_log("%s(): Checkin successful.\n", __FUNCTION__);
    
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
    snprintf(outgoing_msg, LPJS_MSG_LEN_MAX + 1, "%c%s %s %s %s %d\n",
	     LPJS_DISPATCHD_REQUEST_JOB_COMPLETE, hostname,
	     job_id, getenv("LPJS_CORES_PER_JOB"),
	     getenv("LPJS_MEM_PER_CORE"), status);
    if ( lpjs_send_munge(msg_fd, outgoing_msg, close) != EX_OK )
    {
	lpjs_log("lpjs-chaperone: Failed to send message to dispatchd: %s",
		strerror(errno));
	close(msg_fd);
	return EX_IOERR;
    }
    
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
	    lpjs_log("%s(): Failed to connect to dispatchd: %s\n",
		    __FUNCTION__, strerror(errno));
	    lpjs_log("Retry in %d seconds...\n", LPJS_RETRY_TIME);
	    sleep(LPJS_RETRY_TIME);
	}
	else
	{
	    status = lpjs_chaperone_completion(msg_fd, hostname, job_id,
						status);
	    if ( status != EX_OK )
	    {
		lpjs_log("%s(): Message send failed.  Retry in %d seconds...\n",
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
    lpjs_log("Canceling PID %d...\n", Pid);
    
    /*
     *  Terminate mafia-style: Don't just terminate the process, go
     *  after his family as well.  killpg() vs kill().  Although chaperone
     *  creates a process group for the LPJS script, it killpg() will
     *  not work if programs run by the script create process groups
     *  as well.  So we need to traverse the process tree, depth-first,
     *  kill children before their parents to ensure that all descendent
     *  processes are discovered.
     */
    
    whack_family(Pid);
    
    // Signal LPJS script to terminate.  Using kill() vs killpg()
    // shouldn't matter at this point, but we use killpg() in case
    // there are stragglers.
    if ( killpg(Pid, SIGTERM) != 0 )
	killpg(Pid, SIGKILL);
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
    snprintf(cmd, LPJS_CMD_MAX + 1, "pgrep -P %u", Pid);
    lpjs_log("Running %s...\n", cmd);
    
    // Signal each child to terminate
    if ( (fp = popen(cmd, "r")) == NULL )
    {
	lpjs_log("Failed to run %s.\n", cmd);
	return;
    }
    while ( fgets(pid_str, 100, fp) != NULL )
    {
	// FIXME: Check success
	child_pid = strtoul(pid_str, &end, 10);
	
	// Depth-first traversal of process tree
	if ( child_pid != pid )
	    whack_family(child_pid);
	
	lpjs_log("Signaling child PID %u...\n", child_pid);
	if ( kill(child_pid, SIGTERM) != 0 )
	{
	    lpjs_log("Sending SIGKILL...\n");
	    kill(child_pid, SIGKILL);
	}
    }
    pclose(fp);
}
