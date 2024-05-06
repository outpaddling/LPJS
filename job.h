#ifndef _LPJS_JOB_H_
#define _LPJS_JOB_H_

// scanf() is good for converting numbers, but risks buffer overflows
// for strings, just like gets(), etc.
// Numeric fields must be grouped together before string fields
// for job_read_from_string()
// Numeric: job_id, array_index, job_count, procs_per_job, min_procs_per_node,
//          mem_per_proc, chaperone_pid, job_pid
// String:  user_name, primary_group_name, submit_node, submit_directory,
//          script_name, run_host, push_command
#define JOB_BASIC_NUMS_FORMAT   "%9lu %4lu %4u %3u %3u %5zu"
#define JOB_SPEC_NUMS_FORMAT    JOB_BASIC_NUMS_FORMAT " %u %u %d"
#define JOB_SPEC_NUMERIC_FIELDS 9
// Complete job specs
#define JOB_SPEC_FORMAT         JOB_SPEC_NUMS_FORMAT " %s %s %s %s %s %s %s\n"
#define JOB_SPEC_STRING_FIELDS  7
// For lpjs jobs output
#define JOB_BASIC_PARAMS_HEADER \
    "    JobID  IDX Jobs P/J P/N MiB/P User Submit-host Script\n"
#define JOB_BASIC_PARAMS_FORMAT JOB_BASIC_NUMS_FORMAT " %s %s %s\n"
#define JOB_FIELD_MAX_LEN       1024
#define JOB_STR_MAX_LEN         2048    // Fixme: MAX_PATH + x?
#define JOB_SPECS_ITEMS         (JOB_SPEC_NUMERIC_FIELDS + JOB_SPEC_STRING_FIELDS)

typedef struct job  job_t;

#include <stdio.h>

#include "job-rvs.h"
#include "job-accessors.h"
#include "job-mutators.h"
#include "job-protos.h"

#endif  // _LPJS_JOB_H_
