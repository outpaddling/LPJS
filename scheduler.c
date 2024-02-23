#include <stdio.h>
#include <dirent.h>     // opendir(), ...
#include <stdlib.h>     // strtoul()
#include <limits.h>     // ULONG_MAX
#include <string.h>     // strerror()
#include <errno.h>

#include <xtend/file.h>

#include "lpjs.h"
#include "node-list.h"
#include "scheduler.h"
#include "misc.h"       // lpjs_log()

/***************************************************************************
 *  Description:
 *      Select nodes to run a pending job
 *
 *      Default to packing jobs as densely as possible, i.e. use up
 *      available cores on already busy nodes before allocating cores on
 *      idle nodes.  This will allow faster deployment of shared memory
 *      parallel jobs, which need many cores on the same node.
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

int     lpjs_dispatch_next_job(node_list_t *node_list, job_list_t *job_list)

{
    job_t   *job = job_new();    // exits if malloc fails, no need to check
    
    /*
     *  Look through spool dir and determine requirements of the
     *  next job in the queue
     */
    
    if ( lpjs_select_next_job(job) < 1 )
	return 0;
    
    /*
     *  Look through available nodes and select the best match
     *  for the job requirements
     */
    
    // if ( match )
	/*
	 *  Move from pending to running
	 */
	
	/*
	 *  Update job in job_list.  This is only a cache of information stored
	 *  on disk, for quick access during queries.
	 */
	
	/*
	 *  Log submission time and stats
	 */
    
    // else do nothing until next event that might make it possible to dispatch
    // Qualifying events: job completion, new node addition
    // maybe set a flag indicating that we're stuck until one of these
    // things happens, to avoid wasting time trying to dispatch this
    // job again when nothing has changed
    
    return 0;
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


int     lpjs_dispatch_jobs(node_list_t *node_list, job_list_t *job_list)

{
    while ( lpjs_dispatch_next_job(node_list, job_list) > 0 )
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

int     lpjs_select_next_job(job_t *job)

{
    DIR             *dp;
    FILE            *specs_stream;
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
	 *  Get job specs and script from spool dir
	 */
	
	snprintf(specs_path, PATH_MAX + 1, "%s/%lu/job.specs",
		LPJS_PENDING_DIR, low_job_id);
	if ( (specs_stream = fopen(specs_path, "r")) == NULL )
	{
	    fprintf(stderr, ": Could not open %s for read: %s.\n",
		    specs_path, strerror(errno));
	    return 0;
	}
	job_read(job, specs_stream);
	job_print(job, Log_stream);
	fclose(specs_stream);
	
	// job_parse_script(&job, script_path);
	
	return low_job_id;
    }
}
