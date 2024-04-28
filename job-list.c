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
    {
	job_list->jobs[job_list->count++] = job;
    }
    else
    {
	lpjs_log("%s(): Maximum job count = %u reached.\n",
		 __FUNCTION__, LPJS_MAX_JOBS);
    }
    
    return 0;   // NL_OK?
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
