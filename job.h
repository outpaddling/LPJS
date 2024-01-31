#ifndef _LPJS_JOB_H_
#define _LPJS_JOB_H_

// FIXME: Include all current fields
#define JOB_SPEC_HEADER_FORMAT  "%7s %-20s %12s %4s %10s\n"
#define JOB_SPEC_FORMAT         "%-7lu %-20s %12s %4u %10lu\n"
#define JOB_FIELD_MAX_LEN       1024

typedef struct job  job_t;

#include "job-rvs.h"
#include "job-accessors.h"
#include "job-mutators.h"
#include "job-protos.h"

#endif  // _LPJS_JOB_H_
