#ifndef _JOB_H_
#define _JOB_H_

typedef struct
{
    char            *jobname;
    char            *username;
    unsigned        cores;
    unsigned long   mem_per_core;
}   job_t;

#define JOB_SPEC_HEADER_FORMAT  "%-20s %12s %4s %10s\n"
#define JOB_SPEC_FORMAT         "%-20s %12s %4u %10lu\n"

/* job.c */
void    job_init(job_t *job);
void    job_print_params(job_t *job);
void    job_send_params(int fd, job_t *job);

#endif  // _JOB_H_
