#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <string.h>
#include <xtend/dsv.h>      // dsv_read_field()
#include <xtend/string.h>   // strtrim()
#include "job-list.h"
#include "lpjs.h"

/*
 *  Constructor for job_list_t
 */

void    job_list_init(job_list_t *job_list)

{
    job_list->count = 0;
}


int     job_list_add_job(job_list_t *job_list, job_t *job)

{
    return 0;   // NL_OK?
}


void    job_list_send_params(int msg_fd, job_list_t *job_list)

{
    unsigned    c;
    
    dprintf(msg_fd, JOB_SPEC_HEADER_FORMAT, "JobID", "Jobname",
	    "Username", "Cores", "Mem-per-core");
    for (c = 0; c < job_list->count; ++c)
	job_send_params(msg_fd, &job_list->jobs[c]);
}
