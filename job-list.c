#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <string.h>

#include <xtend/dsv.h>      // xt_dsv_read_field()
#include <xtend/string.h>   // xt_strtrim()
#include <xtend/file.h>

#include "job-list-private.h"
#include "lpjs.h"
#include "misc.h"           // lpjs_log()


/***************************************************************************
 *  Description:
 *  
 *  Arguments:
 *
 *  Returns:
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-04-28  Jason Bacon Begin
 ***************************************************************************/

job_list_t  *job_list_new(void)

{
    job_list_t  *job_list;
    
    job_list = malloc(sizeof(job_list_t));
    if ( job_list == NULL )
    {
	lpjs_log("%s(): Error: malloc() failed.\n", __FUNCTION__);
	exit(EX_SOFTWARE);
    }
    job_list_init(job_list);
    
    return job_list;
}


/***************************************************************************
 *  Description:
 *      Constructor for job_list_t
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

void    job_list_init(job_list_t *job_list)

{
    job_list->count = 0;
}


/***************************************************************************
 *  Description:
 *      Add a job to the queue
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

int     job_list_add_job(job_list_t *job_list, job_t *job)

{
    if ( job_list->count < JOB_LIST_MAX_JOBS )
    {
	job_list->jobs[job_list->count++] = job;
	//lpjs_debug("%s(): Added job id %lu, new count = %u\n", __FUNCTION__,
	//        job_get_job_id(job), job_list->count);
    }
    else
	lpjs_log("%s(): Error: Maximum job count = %u reached.\n",
		 __FUNCTION__, JOB_LIST_MAX_JOBS);
    
    return 0;   // NL_OK?
}


size_t  job_list_find_job_id(job_list_t *job_list, unsigned long job_id)

{
    size_t  c;
    
    // FIXME: Use bsearch(), even though this array will never be very large
    // Must then keep list sorted by job_id
    for (c = 0; c < job_list->count; ++c)
    {
	if ( job_get_job_id(job_list->jobs[c]) == job_id )
	    return c;
    }
    
    return JOB_LIST_NOT_FOUND;
}


job_t   *job_list_remove_job(job_list_t *job_list, unsigned long job_id)

{
    size_t  job_array_index;
    extern FILE *Log_stream;
    job_t   *job;
    
    job_array_index = job_list_find_job_id(job_list, job_id);
    if ( job_array_index == JOB_LIST_NOT_FOUND )
	return NULL;
    
    // lpjs_debug("%s(): Removing job %lu from list\n", __FUNCTION__, job_id);
    job = job_list->jobs[job_array_index];
    job_print_full_specs(job, Log_stream);
    
    for (int c = job_array_index; c < job_list->count - 1; ++c)
    {
	fflush(Log_stream);
	job_list->jobs[c] = job_list->jobs[c + 1];
    }
    --job_list->count;

    return job;
}


/***************************************************************************
 *  Description:
 *      Send current jobs to msg_fd in human-readable format
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

void    job_list_send_params(int msg_fd, job_list_t *job_list)

{
    unsigned    c;

    job_send_basic_params_header(msg_fd);
    for (c = 0; c < job_list->count; ++c)
	job_send_basic_params(job_list->jobs[c], msg_fd);
}


/***************************************************************************
 *  Description:
 *      Sort job list numerically by job id
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-05-08  Jason Bacon Begin
 ***************************************************************************/

void    job_list_sort(job_list_t *job_list)

{
    qsort(job_list->jobs, job_list->count, sizeof(job_t *),
	    (int(*)(const void *, const void *))job_id_cmp);
}
