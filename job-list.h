#ifndef _LPJS_JOB_LIST_H_
#define _LPJS_JOB_LIST_H_

#ifndef _LPJS_JOB_H_
#include "job.h"
#endif

#define LPJS_MAX_JOBS  100000

typedef struct job_list job_list_t;

#include "job-list-rvs.h"
#include "job-list-accessors.h"
#include "job-list-mutators.h"

#include "job-list-protos.h"

#endif  // _LPJS_JOB_LIST_H_
