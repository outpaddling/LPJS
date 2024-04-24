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
#include <sys/wait.h>       // FIXME: Replace wait() with active monitoring

#include <xtend/string.h>
#include <xtend/file.h>

#include "node-list.h"
#include "config.h"
#include "network.h"
#include "misc.h"
#include "lpjs.h"

int     main (int argc, char *argv[])

{
    int         msg_fd,
		status;
    unsigned    cores;
    unsigned long   mem_per_core;
    node_list_t *node_list = node_list_new();
    char        *job_script_name,
		*temp,
		*end,
		log_file[PATH_MAX + 1],
		outgoing_msg[LPJS_MSG_LEN_MAX + 1],
		wd[PATH_MAX + 1];
    pid_t       pid;
    extern FILE *Log_stream;
    char        hostname[sysconf(_SC_HOST_NAME_MAX) + 1];
    char        shared_fs_marker[PATH_MAX + 1],
		cmd[LPJS_CMD_MAX + 1];
    struct stat st;
    
    if ( argc != 2 )
    {
	fprintf(stderr, "Usage: %s job-script.lpjs\n", argv[0]);
	fprintf(stderr, "Job parameters are retrieved from LPJS_* env vars.\n");
	return EX_USAGE;
    }
    job_script_name = argv[1];
    
    temp = getenv("LPJS_CORES_PER_JOB");
    cores = strtoul(temp, &end, 10);
    if ( *end != '\0' )
    {
	fprintf(stderr, "Invalid LPJS_CORES_PER_JOB: %s\n", temp);
	exit(EX_USAGE);
    }
    
    temp = getenv("LPJS_MEM_PER_CORE");
    mem_per_core = strtoul(temp, &end, 10);
    if ( *end != '\0' )
    {
	fprintf(stderr, "Invalid LPJS_MEM_PER_CORE: %s\n", temp);
	exit(EX_USAGE);
    }

    // Chaperone outputs stderr to a log file in the working dir
    snprintf(log_file, PATH_MAX + 1, "chaperone-%s.stderr",
	     getenv("LPJS_JOB_ID"));
    
    if ( (Log_stream = lpjs_log_output(log_file)) == NULL )
    {
	fprintf(stderr, "chaperone: Failed to create log file.\n");
	return EX_CANTCREAT;
    }
    
    // Get hostname of head node
    lpjs_load_config(node_list, LPJS_CONFIG_HEAD_ONLY, stderr);

    if ( (msg_fd = lpjs_connect_to_dispatchd(node_list)) == -1 )
    {
	lpjs_log("lpjs-chaperone: Failed to connect to dispatch: %s",
		strerror(errno));
	return EX_IOERR;
    }
    
    // FIXME: Use fork() and exec() or posix_spawn(), and monitor
    // the child process for resource use
    // xt_str_argv_cat(cmd, argv, 1, LPJS_CMD_MAX + 1);
    // status = system(cmd);
    
    gethostname(hostname, sysconf(_SC_HOST_NAME_MAX));
    lpjs_log("CWD = %s\n", getcwd(wd, PATH_MAX + 1));
    lpjs_log("Running %s on %s with %u cores and %lu MiB.\n",
	    job_script_name, hostname, cores, mem_per_core);
    
    if ( (pid = fork()) == 0 )
    {
	// Child, run script
	// FIXME: Set CPU and memory (virtual and physical) limits
	fclose(Log_stream); // Not useful to child
	execl(job_script_name, job_script_name, NULL);
	lpjs_log("%s(): execl() failed: %s\n", __FUNCTION__, strerror(errno));
	return EX_UNAVAILABLE;
    }
    else
    {
	// FIXME: Chaperone, monitor resource use of child
	// Maybe ptrace(), though seemingly not well standardized
	wait(&status);
    }
    
    /* Send job completion message to dispatchd */
    snprintf(outgoing_msg, LPJS_MSG_LEN_MAX + 1, "%c%s %s %s %s\n",
	     LPJS_DISPATCHD_REQUEST_JOB_COMPLETE, hostname,
	     getenv("LPJS_JOB_ID"), getenv("LPJS_CORES_PER_JOB"),
	     getenv("LPJS_MEM_PER_CORE"));
    if ( lpjs_send_munge(msg_fd, outgoing_msg) != EX_OK )
    {
	lpjs_log("lpjs-chaperone: Failed to send message to dispatchd: %s",
		strerror(errno));
	close(msg_fd);
	return EX_IOERR;
    }
    close(msg_fd);
    
    // Transfer working dir to submit host or according to user
    // settings, if not shared
    lpjs_get_marker_filename(shared_fs_marker, getenv("LPJS_SUBMIT_HOST"),
			     PATH_MAX + 1);
    if ( stat(shared_fs_marker, &st) != 0 )
    {
	char    *sp, *submit_host, *submit_dir;
	size_t  c;
	
	lpjs_log("Transferring and removing temporary working dir: %s\n", wd);
	chdir("..");    // Can't remove dir while in use
	
	// FIXME: Check for errors
	// FIXME: Allow user to specify transfer command
	sp = getenv("LPJS_PUSH_COMMAND");
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
			submit_host = getenv("LPJS_SUBMIT_HOST");
			if ( c + strlen(submit_host) > LPJS_CMD_MAX )
			{
			    lpjs_log("LPJS_PUSH_COMMAND longer than %u, aborting.\n", LPJS_CMD_MAX);
			    return EX_DATAERR;
			}
			strlcpy(cmd + c, submit_host, LPJS_CMD_MAX + 1);
			c += strlen(submit_host);
			++sp;
			break;
		    case    'd':
			submit_dir = getenv("LPJS_SUBMIT_DIR");
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
	snprintf(cmd, LPJS_CMD_MAX + 1, "rsync -av %s %s:%s\n",
		 wd, getenv("LPJS_SUBMIT_HOST"),
		 getenv("LPJS_SUBMIT_DIRECTORY"));
	system(cmd);
	
	// No more lpjs_log() beyond here.  Log file already transferred.
	fclose(Log_stream);
	
	// Remove temporary working dir unless sysadmin allows retention
	// FIXME: Check for sysadmin-controlled "keep" option and errors
	snprintf(cmd, LPJS_CMD_MAX + 1, "rm -rf %s", wd);
	system(cmd);
    }

    return status;
}
