#ifndef _LPJS_JOB_H_
#define _LPJS_JOB_H_

// jobid job_count cores_per_job cores_per_node mem_per_core user_name
// working_directory/script_name
#define JOB_SPEC_HEADER_FORMAT  "%9s  %9s %3s %3s %9s  %s\n%s/%s\n"
// scanf() is good for converting numbers, but risks buffer overflows
// for strings, just like gets(), etc.
// Numeric fields must be grouped together before string fields
// for job_read_from_string()
#define JOB_SPEC_NUMS_FORMAT    "%9lu %6u %3u %3u %9lu"
#define JOB_SPEC_NUMERIC_FIELDS 5
#define JOB_SPEC_FORMAT         JOB_SPEC_NUMS_FORMAT " %s\n%s/%s\n"
#define JOB_FIELD_MAX_LEN       1024

typedef struct job  job_t;

#include "job-rvs.h"
#include "job-accessors.h"
#include "job-mutators.h"
#include "job-protos.h"

#endif  // _LPJS_JOB_H_
