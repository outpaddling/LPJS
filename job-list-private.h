#ifndef __H_
#define __H_

#ifdef  __cplusplus
extern "C" {
#endif

#include "job-list.h"

struct job_list
{
    size_t  count;
    job_t   *jobs[JOB_LIST_MAX_JOBS];
};

#ifdef  __cplusplus
}
#endif

#endif  // #ifndef __H_
