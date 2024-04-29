#ifndef __H_
#define __H_

#ifdef  __cplusplus
extern "C" {
#endif

#include "job-list.h"

struct job_list
{
    unsigned long   count;
    job_t           *jobs[LPJS_MAX_JOBS];
};

#ifdef  __cplusplus
}
#endif

#endif  // #ifndef __H_
