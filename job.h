#ifndef _LPJS_JOB_H_
#define _LPJS_JOB_H_

typedef struct
{
    unsigned long   jobid;
    char            *script_path;
    char            *working_directory;
    char            *username;
    unsigned        cores;
    unsigned long   mem_per_core;
}   job_t;

#define JOB_SPEC_HEADER_FORMAT  "%7s %-20s %12s %4s %10s\n"
#define JOB_SPEC_FORMAT         "%-7lu %-20s %12s %4u %10lu\n"
#define JOB_FIELD_MAX_LEN       1024

/* job.c */
void    job_init(job_t *job);
void    job_print_params(job_t *job);
void    job_send_params(job_t *job, int msg_fd);
int     job_parse_script(job_t *job, const char *script_path);

#endif  // _LPJS_JOB_H_
