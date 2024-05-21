#include <stdio.h>
#include <stdlib.h>     // strtoul()
#include <limits.h>     // ULONG_MAX
#include <string.h>     // strerror()
#include <errno.h>
#include <unistd.h>     // close()
#include <sys/wait.h>
#include <sysexits.h>

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
    // Terminates process if malloc() fails, no check required
    node_list_t *matched_nodes = node_list_new();
    char        pending_path[PATH_MAX + 1],
		script_path[PATH_MAX + 1],
		script_buff[LPJS_SCRIPT_SIZE_MAX + 1],
		outgoing_msg[LPJS_JOB_MSG_MAX + 1],
		*munge_payload;
    int         msg_fd,
		procs_used,
		dispatched,
		exit_code;
    ssize_t     script_size,
		payload_bytes;
    size_t      phys_MiB_used;
    extern FILE *Log_stream;
    uid_t       uid;
    gid_t       gid;
    
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
	 *  Do not move from pending to running yet.
	 *  Wait until chaperone checks in and provides the compute node
	 *  and PIDs.
	 */
	
	/*
	 *  Load script from spool/lpjs/pending
	 */
	
	snprintf(pending_path, PATH_MAX + 1, "%s/%lu",
		 LPJS_PENDING_DIR, job_get_job_id(job));
	snprintf(script_path, PATH_MAX + 1, "%s/%s",
		 pending_path, job_get_script_name(job));
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
	
	// FIXME: Revamp and verify handling of failed dispatches
	for (int c = 0; c < node_list_get_compute_node_count(matched_nodes); ++c)
	{
	    node_t *node = node_list_get_compute_nodes_ae(matched_nodes, c);
	    
	    msg_fd = node_get_msg_fd(node);

	    lpjs_log("Dispatching job %lu to %s on socket fd %d...\n",
		    job_get_job_id(job), node_get_hostname(node), msg_fd);
	    
	    outgoing_msg[0] = LPJS_COMPD_REQUEST_NEW_JOB;
	    job_print_to_string(job, outgoing_msg + 1, LPJS_JOB_MSG_MAX + 1);

	    lpjs_log("Job specs: %s\n", outgoing_msg + 1);
	    
	    // FIXME: Check for truncation
	    strlcat(outgoing_msg, script_buff, LPJS_JOB_MSG_MAX + 1);
	    if ( lpjs_send_munge(msg_fd, outgoing_msg,
				 lpjs_dispatchd_safe_close) != LPJS_MSG_SENT )
	    {
		lpjs_log("%s(): Failed to send job to compd.\n", __FUNCTION__);
		free(matched_nodes);
		return 0;
	    }
	    
	    // Get status back from compd
	    lpjs_log("Awaiting dispatch status from compd...\n");
	    payload_bytes = lpjs_recv_munge(msg_fd, &munge_payload,
					    0, 0, &uid, &gid,
					    lpjs_dispatchd_safe_close);
	    // lpjs_log("payload_bytes = %d\n", payload_bytes);
	    if ( payload_bytes > 0 )
	    {
		exit_code = munge_payload[0];
		if ( exit_code == LPJS_DISPATCH_SCRIPT_FAILED )
		{
		    lpjs_log("%s(): Job script failed to start: %d\n",
			    __FUNCTION__, exit_code);
		    lpjs_remove_pending_job(pending_jobs, job_get_job_id(job));
		}
		else if ( exit_code == LPJS_DISPATCH_OSERR )
		{
		    lpjs_log("%s(): OS error detected on %s.\n",
			    __FUNCTION__, node_get_hostname(node));
		    node_set_state(node, "down");
		    // FIXME: Make sure job state is fully reset
		}
		else
		{
		    job_set_state(job, JOB_STATE_DISPATCHED);
		    // Don't set compute node until chaperone confirms
		    // successful launch
		    
		    // FIXME: Needs adjustment for MPI jobs at the least
		    procs_used = node_get_procs_used(node);
		    node_set_procs_used(node, procs_used + job_get_procs_per_job(job));
		    phys_MiB_used = node_get_phys_MiB_used(node);
		    node_set_phys_MiB_used(node, phys_MiB_used +
			job_get_mem_per_proc(job) * job_get_procs_per_job(job));
		}
	    }
	    else
		lpjs_log("%s(): Failed to receive dispatch status from compd.\n",
			__FUNCTION__);
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
    size_t          c;
    job_t           *temp_job;
    
    if ( job_list_get_count(pending_jobs) == 0 )
    {
	lpjs_log("%s(): No pending jobs.\n", __FUNCTION__);
	return 0;
    }
    else
    {
	for (c = 0; c < job_list_get_count(pending_jobs); ++c)
	{
	    temp_job = job_list_get_jobs_ae(pending_jobs, c);
	    if ( job_get_state(temp_job) == JOB_STATE_PENDING )
		// Found a pending job not yet dispatched
		break;
	}
	if ( c == job_list_get_count(pending_jobs) )
	{
	    lpjs_log("%s(): All jobs already dispatched.\n", __FUNCTION__);
	    return 0;
	}
	
	*job = job_list_get_jobs_ae(pending_jobs, c);
	low_job_id = job_get_job_id(*job);
	lpjs_log("%s(): Selected job %lu to dispatch.\n",
		 __FUNCTION__, low_job_id);
	job_print_full_specs(*job, Log_stream);
	return low_job_id;
    }
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
	if ( strcmp(node_get_state(node), "up") != 0 )
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


/***************************************************************************
 *  Description:
 *  
 *  Returns:
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-05-03  Jason Bacon Begin
 ***************************************************************************/

job_t   *lpjs_remove_pending_job(job_list_t *pending_jobs, unsigned long job_id)

{
    char    pending_path[PATH_MAX + 1];
    pid_t   pid;
    int     status;
    
    if ( (pid = fork()) == 0 )
    {
	snprintf(pending_path, PATH_MAX + 1, "%s/%lu",
		 LPJS_PENDING_DIR, job_id);
	lpjs_log("Removing job %s...\n", pending_path);
	execlp("rm", "rm", "-rf", pending_path, NULL);
    }
    else
	// WEXITED is implicitly set for waitpid(), but specify for readability
	waitpid(pid, &status, WEXITED);
    
    return job_list_remove_job(pending_jobs, job_id);
}


job_t   *lpjs_remove_running_job(job_list_t *running_jobs, unsigned long job_id)

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
