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
	lpjs_log("%s(): malloc() failed.\n", __FUNCTION__);
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
    if ( job_list->count < LPJS_MAX_JOBS )
	job_list->jobs[job_list->count++] = job;
    else
	lpjs_log("%s(): Maximum job count = %u reached.\n",
		 __FUNCTION__, LPJS_MAX_JOBS);
    
    return 0;   // NL_OK?
}


int     job_list_remove_job(job_list_t *job_list, unsigned long job_id)

{
    int     c;
    extern FILE *Log_stream;
    
    // FIXME: Use bsearch(), even though this array will never be very large
    for (c = 0; c < job_list->count; ++c)
    {
	if ( job_get_job_id(job_list->jobs[c]) == job_id )
	{
	    lpjs_log("Removing job %lu\n", job_id);
	    job_print(job_list->jobs[c], Log_stream);
	    
	    job_free(&job_list->jobs[c]);
	    
	    for (int c2 = c; c2 < job_list->count - 1; ++c2)
	    {
		lpjs_log("c2 = %d  job_list->jobs[c2 + 1] = %p\n",
			c2, job_list->jobs[c2 + 1]);
		lpjs_log("job_list[%d] <- job_list[%d] (%lu)\n",
			 c2, c2 + 1, job_get_job_id(job_list->jobs[c2 + 1]));
		fflush(Log_stream);
		job_list->jobs[c2] = job_list->jobs[c2 + 1];
	    }
	    --job_list->count;
	    return 0;
	}
    }
    return 0;   // FIXME: Define return codes
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

    lpjs_log("%s(): %u jobs\n", __FUNCTION__, job_list_get_count(job_list));
    job_send_basic_params_header(msg_fd);
    for (c = 0; c < job_list->count; ++c)
	job_send_basic_params(job_list->jobs[c], msg_fd);
}
