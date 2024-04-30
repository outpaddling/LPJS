#include <stdio.h>
#include <dirent.h>     // opendir(), ...
#include <stdlib.h>     // strtoul()
#include <limits.h>     // ULONG_MAX
#include <string.h>     // strerror()
#include <errno.h>
#include <unistd.h>     // close()
#include <sys/wait.h>

#include <xtend/file.h>
#include <xtend/math.h>     // XT_MIN()
#include <xtend/string.h>   // strlcat() on Linux

#include "lpjs.h"
#include "node-list.h"
#include "scheduler.h"
#include "network.h"
#include "misc.h"       // lpjs_log()

/***************************************************************************
 *  Description:
 *      Select nodes to run a pending job
 *
 *      Default to packing jobs as densely as possible, i.e. use up
 *      available procs on already busy nodes before allocating procs on
 *      idle nodes.  This will allow faster deployment of shared memory
 *      parallel jobs, which need many procs on the same node.
 *  
 *  Returns:
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-10-05  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_select_nodes()

{
    return 0;
}


/***************************************************************************
 *  Description:
 *      Check available nodes and the job queue, and dispatch the
 *      next job, if possible.
 *
 *  Returns:
 *      The number of jobs dispatched (0 or 1), or a negative error
 *      code if something went wrong.
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-22  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_dispatch_next_job(node_list_t *node_list,
			       job_list_t *pending_jobs,
			       job_list_t *running_jobs)

{
    job_t       *job;
    node_list_t *matched_nodes = node_list_new();
    char        pending_path[PATH_MAX + 1],
		running_path[PATH_MAX + 1],
		script_path[PATH_MAX + 1],
		script_buff[LPJS_SCRIPT_SIZE_MAX + 1],
		outgoing_msg[LPJS_JOB_MSG_MAX + 1];
    int         msg_fd,
		procs_used,
		dispatched;
    ssize_t     script_size;
    size_t      phys_MiB_used;
    extern FILE *Log_stream;
    
    /*
     *  Look through spool dir and determine requirements of the
     *  next job in the queue
     */
    
    if ( lpjs_select_next_job(pending_jobs, &job) < 1 )
	return 0;
    
    /*
     *  Look through available nodes and select the best match
     *  for the job requirements
     */
    
    if ( lpjs_match_nodes(job, node_list, matched_nodes) > 0 )
    {
	lpjs_log("%s(): Found %u available nodes.  Dispatching...\n",
		__FUNCTION__, node_list_get_compute_node_count(matched_nodes));
	
	/*
	 *  Move from pending to running
	 */
	
	snprintf(pending_path, PATH_MAX + 1,
		LPJS_PENDING_DIR "/%lu", job_get_job_id(job));
	snprintf(running_path, PATH_MAX + 1,
		LPJS_RUNNING_DIR "/%lu", job_get_job_id(job));
	lpjs_log("Moving %s to %s...\n", pending_path, running_path);
	rename(pending_path, running_path);
	
	job_list_add_job(running_jobs, job);
	job_list_remove_job(pending_jobs, job_get_job_id(job));
	
	/*
	 *  Load script from spool/lpjs/running
	 */
	
	snprintf(script_path, PATH_MAX + 1, "%s/%s",
		running_path, job_get_script_name(job));
	script_size = lpjs_load_script(script_path, script_buff,
				       LPJS_SCRIPT_SIZE_MAX + 1);
	// FIXME: Determine a real minimum script size
	if ( script_size < 1 )
	{
	    lpjs_log("%s(): Error reading script %s.\n",
		    __FUNCTION__, script_path);
	    return 0;
	}
	
	//lpjs_log("Script %s is %zd bytes.\n", script_path, script_size);
	//fprintf(Log_stream, "Script:\n%s\n", script_buff);
	
	/*
	 *  For each matching node
	 *      Update mem and proc availability
	 *      Send script to node and run
	 *          Use script cached in spool dir at submission
	 */
	
	for (int c = 0; c < node_list_get_compute_node_count(matched_nodes); ++c)
	{
	    // lpjs_log("Checking node[%d]\n", c);
	    node_t *node = node_list_get_compute_nodes_ae(matched_nodes, c);
	    
	    msg_fd = node_get_msg_fd(node);

	    lpjs_log("Dispatching job %lu to %s on socket fd %d...\n",
		    job_get_job_id(job), node_get_hostname(node), msg_fd);
	    
	    outgoing_msg[0] = LPJS_COMPD_REQUEST_NEW_JOB;
	    job_print_to_string(job, outgoing_msg + 1, LPJS_JOB_MSG_MAX + 1);
	    strlcat(outgoing_msg, script_buff, LPJS_JOB_MSG_MAX + 1);
	    // FIXME: Check for truncation
	    lpjs_log("%s(): outgoing job msg:\n%s\n", __FUNCTION__, outgoing_msg + 1);
	    
	    // FIXME: Needs adjustment for MPI jobs at the least
	    procs_used = node_get_procs_used(node);
	    node_set_procs_used(node, procs_used + job_get_procs_per_job(job));
	    phys_MiB_used = node_get_phys_MiB_used(node);
	    node_set_phys_MiB_used(node, phys_MiB_used +
		job_get_mem_per_proc(job) * job_get_procs_per_job(job));

	    // lpjs_log("procs per job = %u\n", job_get_procs_per_job(job));
	    // lpjs_log("MiB per proc = %zu\n", job_get_mem_per_proc(job));
	    // lpjs_log("New MiB used = %zu\n", node_get_phys_MiB_used(node));
	    
	    lpjs_send_munge(msg_fd, outgoing_msg);
	}
	
	/*
	 *  Log submission time and job specs
	 */
	
	free(matched_nodes);
	dispatched = 1;
    }
    else
    {
	// do nothing until next event that might make it possible to dispatch
	// Qualifying events: job completion, new node addition
	// maybe set a flag indicating that we're stuck until one of these
	// things happens, to avoid wasting time trying to dispatch this
	// job again when nothing has changed
	dispatched = 0;
    }
    
    return dispatched;
}


/***************************************************************************
 *  Description:
 *      Check available nodes and the job queue, and dispatch as many new
 *      jobs as possible.  This should be called following any changes
 *      to the job queue (new submissions, completed jobs), and when
 *      a new node is added.  I.e. whenever it might become possible
 *      to start new jobs.
 *
 *  Returns:
 *      The number of jobs dispatched (0 or 1), or a negative error
 *      code if something went wrong.
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-29  Jason Bacon Begin
 ***************************************************************************/


int     lpjs_dispatch_jobs(node_list_t *node_list,
			   job_list_t *pending_jobs,
			   job_list_t *running_jobs)

{
    // Dispatch as many jobs as possible before resuming
    while ( lpjs_dispatch_next_job(node_list, pending_jobs, running_jobs) > 0 )
	;
    
    return 0;
}


/***************************************************************************
 *  Description:
 *      Examine the spooled jobs, if any, and determine the next one
 *      to be dispatched.
 *  
 *  Returns:
 *      The number of jobs selected (0 or 1)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-29  Jason Bacon Begin
 ***************************************************************************/

unsigned long   lpjs_select_next_job(job_list_t *pending_jobs, job_t **job)

{
    unsigned long   low_job_id;
    extern FILE     *Log_stream;
    
    if ( job_list_get_count(pending_jobs) == 0 )
    {
	lpjs_log("%s(): No pending jobs.\n", __FUNCTION__);
	return 0;
    }
    else
    {
	*job = job_list_get_jobs_ae(pending_jobs, 0);
	low_job_id = job_get_job_id(*job);
	lpjs_log("%s(): Selected job %lu to dispatch.\n",
		 __FUNCTION__, low_job_id);
	job_print(*job, Log_stream);
	return low_job_id;
    }
    
// Old method, searching spool dir
#if 0
    DIR             *dp;
    struct dirent   *entry;
    unsigned long   low_job_id,
		    int_dir_name;
    char            specs_path[PATH_MAX + 1],
		    *end;
    extern FILE     *Log_stream;
    
    /*
     *  Find spooled job with lowest job id
     */
    if ( (dp = opendir(LPJS_PENDING_DIR)) == NULL )
    {
	lpjs_log("%s(): Cannot open %s: %s\n", __FUNCTION__,
		LPJS_PENDING_DIR, strerror(errno));
	return 0;  // FIXME: Define error codes
    }
    
    low_job_id = ULONG_MAX;
    while ( (entry = readdir(dp)) != NULL )
    {
	// The directory name is the job ID
	int_dir_name = strtoul(entry->d_name, &end, 10);
	if ( *end == '\0' )
	{
	    if ( int_dir_name < low_job_id )
	    {
		low_job_id = int_dir_name;
		// script_name = 
	    }
	}
    }
    closedir(dp);
    
    if ( low_job_id == ULONG_MAX )
    {
	lpjs_log("%s(): No pending jobs.\n", __FUNCTION__);
	return 0;
    }
    else
    {
	lpjs_log("%s(): Selected job %lu to dispatch.\n",
		 __FUNCTION__, low_job_id);
	
	/*
	 *  Load job specs from file
	 */
	
	snprintf(specs_path, PATH_MAX + 1, "%s/%lu/%s",
		LPJS_PENDING_DIR, low_job_id, LPJS_SPECS_FILE_NAME);
	if ( job_read_from_file(job, specs_path) != JOB_SPECS_ITEMS )
	{
	    lpjs_log("%s(): Error reading specs for job %lu.\n",
		    __FUNCTION__, low_job_id);
	    return 0;
	}
	job_print(job, Log_stream);
	
	return low_job_id;
    }
#endif
}


/***************************************************************************
 *  Description:
 *  
 *  History: 
 *  Date        Name        Modification
 *  2024-02-23  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_match_nodes(job_t *job, node_list_t *node_list,
			    node_list_t *matched_nodes)

{
    node_t      *node;
    unsigned    node_count,
		c,
		usable_procs,   // Procs with enough mem
		total_usable,
		total_required;
    
    lpjs_log("Job %u requires %u procs, %lu MiB / proc.\n",
	    job_get_job_id(job), job_get_min_procs_per_node(job),
	    job_get_mem_per_proc(job));
    
    total_usable = 0;
    total_required = job_get_procs_per_job(job);
    for (c = node_count = 0;
	 (c < node_list_get_compute_node_count(node_list)) &&
	 (total_usable < total_required); ++c)
    {
	node = node_list_get_compute_nodes_ae(node_list, c);
	if ( strcmp(node_get_state(node), "Up") != 0 )
	    lpjs_log("%s is unavailable.\n", node_get_hostname(node));
	else
	{
	    // lpjs_log("Checking %s...\n", node_get_hostname(node));
	    usable_procs = lpjs_get_usable_procs(job, node);
	    usable_procs = XT_MIN(usable_procs, total_required - total_usable);
	    
	    if ( usable_procs > 0 )
	    {
		lpjs_log("Using %u procs on %s.\n", usable_procs,
			 node_get_hostname(node));
		// FIXME: Set # procs to use on node
		node_list_add_compute_node(matched_nodes, node);
		total_usable += usable_procs;
		++node_count;
	    }
	}
    }
    
    if ( total_usable == total_required )
    {
	lpjs_log("Using nodes:\n");
	for (c = 0; c < node_list_get_compute_node_count(matched_nodes); ++c)
	{
	    node = node_list_get_compute_nodes_ae(matched_nodes, c);
	    lpjs_log("%s\n", node_get_hostname(node));
	}
    }
    else
	lpjs_log("Insufficient resources available.\n");
    
    return node_count;
}


/***************************************************************************
 *  Description:
 *  
 *  History: 
 *  Date        Name        Modification
 *  2024-02-23  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_get_usable_procs(job_t *job, node_t *node)

{
    int         required_procs,
		available_procs,    // Total free
		usable_procs;       // Total free with enough mem
    size_t      available_mem;
    
    required_procs = job_get_min_procs_per_node(job);
    available_mem = node_get_phys_MiB_available(node);
    available_procs = node_get_procs(node) - node_get_procs_used(node);
    lpjs_log("%s: procs = %u  mem = %lu\n", node_get_hostname(node),
	     available_procs, available_mem);
    if ( available_procs >= required_procs )
    {
	if ( (available_mem >= job_get_mem_per_proc(job) * required_procs) )
	    usable_procs = required_procs;
	else
	{
	    lpjs_log("Not enough memory.\n");
	    usable_procs = 0;
	}
    }
    else
    {
	lpjs_log("Not enough procs available.\n");
	usable_procs = 0;
    }
    return usable_procs;
}


job_t   *lpjs_remove_job(job_list_t *running_jobs, unsigned long job_id)

{
    char    running_path[PATH_MAX + 1];
    pid_t   pid;
    int     status;
    
    if ( (pid = fork()) == 0 )
    {
	snprintf(running_path, PATH_MAX + 1, "%s/%lu",
		 LPJS_RUNNING_DIR, job_id);
	lpjs_log("Removing job %s...\n", running_path);
	execlp("rm", "rm", "-rf", running_path, NULL);
    }
    else
	// WEXITED is implicitly set for waitpid(), but specify for readability
	waitpid(pid, &status, WEXITED);
    
    // FIXME: Remove from running_jobs
    return job_list_remove_job(running_jobs, job_id);
}
