#ifndef _LPJS_JOB_PRIVATE_H_
#define _LPJS_JOB_PRIVATE_H_

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef _UNISTD_H_
#include <unistd.h>
#endif

#ifndef __NODE_LIST_H__
#include "node-list.h"
#endif

#ifndef __JOB_H__
#include "job.h"
#endif

struct job
{
    unsigned long   job_id;
    unsigned long   array_index;
    unsigned        job_count;
    unsigned        processors_per_job;
    unsigned        threads_per_process;
    size_t          phys_mib_per_processor;
    pid_t           chaperone_pid;
    pid_t           job_pid;
    job_state_t     state;
    char            *user_name;
    char            *primary_group_name;
    char            *submit_node;
    char            *submit_dir;
    char            *script_name;
    char            *compute_node;
    char            *log_dir;
    char            *cmd_search_path;
    char            *pull_command;
    char            *push_command;
};

#ifdef  __cplusplus
}
#endif

#endif  // #ifndef _LPJS_JOB_PRIVATE_H_
