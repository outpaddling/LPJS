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
    char        *script_name,
		*temp,
		*end,
		log_file[PATH_MAX + 1];
    pid_t       pid;
    extern FILE *Log_stream;
    
    if ( argc != 1 )
    {
	fprintf(stderr, "Usage: %s\n", argv[0]);
	fprintf(stderr, "Job parameters are retrieved from LPJS_* env vars.\n");
	return EX_USAGE;
    }
    
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
    
    script_name = getenv("LPJS_SCRIPT_NAME");

    // Open process-specific log file in var/log/lpjs
    // FIXME: Prevent log files from piling up
    snprintf(log_file, PATH_MAX + 1, "%s/chaperone-%d",
	    LPJS_LOG_DIR, getpid());
    
    if ( (Log_stream = lpjs_log_output(log_file)) == NULL )
	return EX_CANTCREAT;
    
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
    
    printf("Running %s with %u cores and %lu MiB.\n",
	    script_name, cores, mem_per_core);
    
    if ( (pid = fork()) == 0 )
    {
	// Child, run script
	// FIXME: Set CPU and memory limits
	execl(script_name, script_name, NULL);
	lpjs_log("%s(): execl() failed: %s\n", __FUNCTION__, strerror(errno));
	return EX_UNAVAILABLE;
    }
    else
    {
	// FIXME: Chaperone, monitor resource use of child
	// Maybe ptrace(), though seemingly not well standardized
	wait(&status);
    }
    
    /* Send a message to the server */
    /* Need to send \0, so xt_dprintf() doesn't work here */
    // FIXME: Munge messages to dispatchd
    /*
    if ( lpjs_send(msg_fd, 0, "job-complete\ncmd: %s\nstatus: %d\n",
		  script_name, status) < 0 )
    {
	lpjs_log("lpjs-chaperone: Failed to send message to dispatch: %s",
		strerror(errno));
	close(msg_fd);
	return EX_IOERR;
    }
    */
    close(msg_fd);
    fclose(Log_stream);

    return status;
}
