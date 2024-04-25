#ifndef _LPJS_JOB_H_
#define _LPJS_JOB_H_

// job_id job_count cores_per_job min_cores_per_node mem_per_core user_name
// submit_directory/script_name
#define JOB_SPEC_HEADER_FORMAT  "%9s %3s %9s %9s %10s %9s %s %s %s %s %s %s\n"
// scanf() is good for converting numbers, but risks buffer overflows
// for strings, just like gets(), etc.
// Numeric fields must be grouped together before string fields
// for job_read_from_string()
#define JOB_SPEC_NUMS_FORMAT    "%9lu %3lu %9u %9u %10u %9lu"
#define JOB_SPEC_NUMERIC_FIELDS 6
#define JOB_SPEC_FORMAT         JOB_SPEC_NUMS_FORMAT " %s %s %s %s %s %s\n"
#define JOB_FIELD_MAX_LEN       1024
#define JOB_STR_MAX_LEN         2048    // Fixme: MAX_PATH + x?
#define JOB_SPECS_ITEMS         12      // # fields in the struct

typedef struct job  job_t;

#include "job-rvs.h"
#include "job-accessors.h"
#include "job-mutators.h"
#include "job-protos.h"

#endif  // _LPJS_JOB_H_
