#ifndef _JOB_LIST_H_
#define _JOB_LIST_H_

#ifndef _JOB_H_
#include "job.h"
#endif

#define LPJS_MAX_JOBS  1024

typedef struct
{
    unsigned    count;
    job_t       jobs[LPJS_MAX_JOBS];
}   job_list_t;

/* job-list.c */
void    job_list_init(job_list_t *job_list);
int     job_list_add_job(job_list_t *job_list, job_t *job);
void    job_list_send_params(int msg_fd, job_list_t *job_list);

#endif  // _JOB_LIST_H_
