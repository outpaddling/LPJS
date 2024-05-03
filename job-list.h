#ifndef _LPJS_JOB_LIST_H_
#define _LPJS_JOB_LIST_H_

#ifndef _LPJS_JOB_H_
#include "job.h"
#endif

// Must be at least 1 < size_t max, so JOB_LIST_JOB_NOT_FOUND is never
// a valid subscript
#define JOB_LIST_MAX_JOBS   100000
#define JOB_LIST_NOT_FOUND  JOB_LIST_MAX_JOBS

typedef struct job_list job_list_t;

#include "job-list-rvs.h"
#include "job-list-accessors.h"
#include "job-list-mutators.h"

#include "job-list-protos.h"

#endif  // _LPJS_JOB_LIST_H_
