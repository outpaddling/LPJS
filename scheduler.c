#include <stdio.h>
#include <dirent.h>     // opendir(), ...
#include <stdlib.h>     // strtoul()
#include <limits.h>     // ULONG_MAX

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
    job_t   job;
    
    /*
     *  Look through spool dir and determine requirements of the
     *  next job in the queue
     */
    
    if ( lpjs_select_next_job(&job) == 0 )
	return 0;
    
    /*
     *  Look through available nodes and select the best match
     *  for the job requirements
     */
    
    /*
     *  Log submission time and stats
     */
    
    /*
     *  Update job in job_list.  This is only a cache of information stored
     *  on disk, for quick access during queries.
     */
    
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
    struct dirent   *entry;
    unsigned long   low_job_id,
		    int_dir_name;
    char            *end;
    
    /*
     *  Find spooled job with lowest job id
     */
    dp = opendir(LPJS_SPOOL_DIR);
    low_job_id = ULONG_MAX;
    while ( (entry = readdir(dp)) != NULL )
    {
	// The directory name is the job ID
	int_dir_name = strtoul(entry->d_name, &end, 10);
	if ( *end == '\0' )
	{
	    if ( int_dir_name < low_job_id )
		low_job_id = int_dir_name;
	}
    }
    closedir(dp);
    lpjs_log("%s(): Selected job %lu to dispatch.\n", __FUNCTION__, low_job_id);
    
    /*
     *  Get job specs and script from spool dir
     */
    
    /*
     *  Move from pending to running
     */
    
    /*
     *  Update job in job_list.  This is only a cache of information stored
     *  on disk, for quick access during queries.
     */
    
    return 0;
}
