#ifndef _LPJS_JOB_PRIVATE_H_
#define _LPJS_JOB_PRIVATE_H_

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef __NODE_LIST_H__
#include "node-list.h"
#endif

struct job
{
    unsigned long   job_id;
    unsigned        job_count;
    unsigned        cores_per_job;
    unsigned        min_cores_per_node;
    size_t          mem_per_core;
    char            *user_name;
    char            *primary_group_name;
    char            *working_directory;
    char            *script_name;
};

#ifndef __JOB_H__
#include "job.h"
#endif

#ifdef  __cplusplus
}
#endif

#endif  // #ifndef _LPJS_JOB_PRIVATE_H_
