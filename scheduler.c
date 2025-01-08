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
 *      available processors on already busy nodes before allocating processors on
 *      idle nodes.  This will allow faster deployment of shared memory
 *      parallel jobs, which need many processors on the same node.
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
 *      The number of nodes matched, or a negative error
 *      code if something went wrong.  A positive number indicates
 *      that nodes were available and this function should be called
 *      again until it returns 0 or an error.
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
		script_path[PATH_MAX + 2],
		script_buff[LPJS_SCRIPT_SIZE_MAX + 1],
		outgoing_msg[LPJS_JOB_MSG_MAX + 1],
		*munge_payload;
    int         compd_msg_fd,
		node_count;
    ssize_t     script_size,
		payload_bytes;
    uid_t       uid;
    gid_t       gid;
    
    /*
     *  Look through spool dir and determine requirements of the
     *  next job in the queue
     */
    
    if ( lpjs_select_next_job(pending_jobs, &job) < 1 )
    {
	lpjs_log("%s(): No pending jobs.\n", __FUNCTION__);
	return 0;
    }
    
    /*
     *  Look through available nodes and select the best match
     *  for the job requirements
     */
    
    if ( (node_count = lpjs_match_nodes(job, node_list, matched_nodes)) > 0 )
    {
	lpjs_log("%s(): Found %u available nodes.\n",
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
	snprintf(script_path, PATH_MAX + 2, "%s/%s",
		 pending_path, job_get_script_name(job));
	script_size = lpjs_load_script(script_path, script_buff,
				       LPJS_SCRIPT_SIZE_MAX + 1);

	if ( script_size < LPJS_SCRIPT_MIN_SIZE )
	{
	    lpjs_log("%s(): Error: Script %s < %d characters.\n",
		    __FUNCTION__, script_path, LPJS_SCRIPT_MIN_SIZE);
	    return node_count;
	}
	
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
	    
	    compd_msg_fd = node_get_msg_fd(node);

	    lpjs_log("%s(): Dispatching job %lu to %s on socket fd %d...\n",
		    __FUNCTION__, job_get_job_id(job),
		    node_get_hostname(node), compd_msg_fd);
	    
	    outgoing_msg[0] = LPJS_COMPD_REQUEST_NEW_JOB;
	    job_print_to_string(job, outgoing_msg + 1, LPJS_JOB_MSG_MAX + 1);

	    lpjs_log("%s(): Job specs: %s\n", __FUNCTION__, outgoing_msg + 1);
	    
	    // FIXME: Check for truncation
	    strlcat(outgoing_msg, script_buff, LPJS_JOB_MSG_MAX + 1);
	    if ( lpjs_send_munge(compd_msg_fd, outgoing_msg,
				 lpjs_dispatchd_safe_close) != LPJS_MSG_SENT )
	    {
		lpjs_log("%s(): Error: Failed to send job to compd.\n", __FUNCTION__);
		free(matched_nodes);
		return node_count;
	    }
	    
	    /*
	     *  Get chaperone launch status back from compd.
	     *  FIXME: This can take a while and timeouts happen
	     *      Don't wait, but let compd/chaperone connect again
	     *      to send status.  Modify lpjs_compd.c
	     *      in tandem with this section to make this happen.
	     *      Add new LPJS_DISPATCHD_REQUEST_JOB_UPDATE
	     *      case to lpjs_check_listen_fd() for receiving
	     *      updates such as job failures?  Or just use
	     *      LPJS_DISPATCHD_REQUEST_JOB_COMPLETE?
	     */
	    
	    lpjs_log("%s(): Awaiting chaperone fork verification from %s compd...\n",
		     __FUNCTION__, node_get_hostname(node));
	    payload_bytes = lpjs_recv_munge(compd_msg_fd, &munge_payload,
					    0, LPJS_CHAPERONE_STATUS_TIMEOUT,
					    &uid, &gid,
					    lpjs_dispatchd_safe_close);
	    if ( payload_bytes == LPJS_RECV_TIMEOUT )
	    {
		lpjs_log("%s(): Error: Timed out awaiting dispatch status.\n",
			 __FUNCTION__);
		lpjs_log("%s(): Setting %s to down.\n", __FUNCTION__,
			 node_get_hostname(node));
		node_set_state(node, "down");
	    }
	    else if ( munge_payload[0] != LPJS_CHAPERONE_FORKED )
	    {
		lpjs_log("%s(): Bug: Should have received LPJS_CHAPERONE_FORKED.\n",
			 __FUNCTION__);
		lpjs_log("%s(): Got %d instead.\n",
			__FUNCTION__, munge_payload[0]);
		lpjs_log("%s(): Setting %s to down.\n", __FUNCTION__,
			 node_get_hostname(node));
		node_set_state(node, "down");
	    }
	    else
	    {
		/*
		 *  At this point, all we know is that the chaperone
		 *  process was forked successfully by lpjs_run_chaperone().
		 *  It has more work to do setting up the job, but does
		 *  not keep dispatchd waiting, as it can take a long time.
		 *  (more than 1 second in rare cases on a busy compute node).
		 *  But we must assume that job is running and update
		 *  available resources immediately, or this function
		 *  will never return 0 to lpjs_dispatch_jobs(), and
		 *  it will never exit the loop.  lpjs_compd and chaperone
		 *  must be good about reporting failures and job completion
		 *  to dispatchd, so we can free these resources ASAP.
		 */

		lpjs_debug("%s(): Chaperone fork verification received.\n",
			    __FUNCTION__);
		job_set_state(job, JOB_STATE_DISPATCHED);
		char *end;
		pid_t chaperone_pid = strtol(munge_payload+1, &end, 10);
		if ( *end != '\0' )
		{
		    lpjs_log("%s(): Bug: No PID found in chaperone fork verification message.\n",
			    __FUNCTION__);
		}
		job_set_chaperone_pid(job, chaperone_pid);
		
		/*
		 *  Reserve resources immediately to prevent a race
		 *  between chaperone checkin and new job submissions.
		 *  FIXME: This will need adjustment for MPI jobs at the least
		 */
		node_adjust_resources(node, job, NODE_RESOURCE_ALLOCATE);
	    }
	}
	
	/*
	 *  Log submission time and job specs
	 */
	
	free(matched_nodes);
    }
    
    return node_count;
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
    int     nodes;
    
    // Dispatch as many jobs as possible before resuming
    while ( (nodes = lpjs_dispatch_next_job(node_list, pending_jobs,
					    running_jobs)) > 0 )
	lpjs_log("%s(): %d nodes available.\n", __FUNCTION__, nodes);

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
	return 0;
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
		usable_processors,   // Procs with enough mem
		total_usable,
		total_required;
    
    lpjs_log("%s(): Job %u requires %u processors, %lu MiB / proc.\n",
	    __FUNCTION__,
	    job_get_job_id(job), job_get_threads_per_process(job),
	    job_get_phys_mib_per_processor(job));
    
    total_usable = 0;
    total_required = job_get_processors_per_job(job);
    for (c = node_count = 0;
	 (c < node_list_get_compute_node_count(node_list)) &&
	 (total_usable < total_required); ++c)
    {
	node = node_list_get_compute_nodes_ae(node_list, c);
	if ( strcmp(node_get_state(node), "up") != 0 )
	    lpjs_log("%s(): %s is unavailable.\n",
		    __FUNCTION__, node_get_hostname(node));
	else
	{
	    // lpjs_log("Checking %s...\n", node_get_hostname(node));
	    usable_processors = lpjs_get_usable_processors(job, node);
	    usable_processors = XT_MIN(usable_processors, total_required - total_usable);
	    
	    if ( usable_processors > 0 )
	    {
		lpjs_log("%s(): Using %u processors on %s.\n", __FUNCTION__,
			usable_processors, node_get_hostname(node));
		// FIXME: Set # processors to use on node
		node_list_add_compute_node(matched_nodes, node);
		total_usable += usable_processors;
		++node_count;
	    }
	}
    }
    
    if ( total_usable == total_required )
    {
	lpjs_log("%s(): Using nodes:\n", __FUNCTION__);
	for (c = 0; c < node_list_get_compute_node_count(matched_nodes); ++c)
	{
	    node = node_list_get_compute_nodes_ae(matched_nodes, c);
	    lpjs_log("%s(): %s\n", __FUNCTION__, node_get_hostname(node));
	}
    }
    else
	lpjs_log("%s(): Insufficient resources available.\n", __FUNCTION__);
    
    return node_count;
}


/***************************************************************************
 *  Description:
 *  
 *  History: 
 *  Date        Name        Modification
 *  2024-02-23  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_get_usable_processors(job_t *job, node_t *node)

{
    int         required_processors,
		available_processors,    // Total free
		usable_processors;       // Total free with enough mem
    size_t      available_mem;
    
    required_processors = job_get_threads_per_process(job);
    available_mem = node_get_phys_MiB_available(node);
    available_processors = node_get_processors(node) - node_get_processors_used(node);
    lpjs_log("%s(): %s: processors = %u  mem = %lu\n", __FUNCTION__,
	     node_get_hostname(node), available_processors, available_mem);
    if ( available_processors >= required_processors )
    {
	if ( (available_mem >= job_get_phys_mib_per_processor(job) * required_processors) )
	    usable_processors = required_processors;
	else
	{
	    lpjs_log("%s(): Not enough memory available.\n", __FUNCTION__);
	    usable_processors = 0;
	}
    }
    else
    {
	lpjs_log("%s(): Not enough processors available.\n", __FUNCTION__);
	usable_processors = 0;
    }
    return usable_processors;
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
	lpjs_log("%s(): Removing job %s...\n", __FUNCTION__, pending_path);
	execlp("rm", "rm", "-rf", pending_path, NULL);
	lpjs_log("%s(): Error: exec(rm -rf %s) failed.\n", __FUNCTION__, pending_path);
	exit(EX_OSERR);
    }
    else
    {
	// WEXITED is implicitly set for waitpid(), but specify for readability
	waitpid(pid, &status, WEXITED);
	if ( status != 0 )
	    lpjs_log("%s(): rm failed, status = %d.\n", __FUNCTION__, status);
    }
    
    return job_list_remove_job(pending_jobs, job_id);
}


// FIXME: merge this with above
job_t   *lpjs_remove_running_job(job_list_t *running_jobs, unsigned long job_id)

{
    char    running_path[PATH_MAX + 1];
    pid_t   pid;
    int     status;
    
    if ( (pid = fork()) == 0 )
    {
	snprintf(running_path, PATH_MAX + 1, "%s/%lu",
		 LPJS_RUNNING_DIR, job_id);
	lpjs_log("%s(): Removing job %s...\n", __FUNCTION__, running_path);
	execlp("rm", "rm", "-rf", running_path, NULL);
	lpjs_log("%s(): exec(rm -rf %s) failed.\n", __FUNCTION__, running_path);
	exit(EX_OSERR);
    }
    else
    {
	// WEXITED is implicitly set for waitpid(), but specify for readability
	waitpid(pid, &status, WEXITED);
	if ( status != 0 )
	    lpjs_log("%s(): rm failed, status = %d.\n", __FUNCTION__, status);
    }
    
    return job_list_remove_job(running_jobs, job_id);
}
