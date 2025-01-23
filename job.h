#ifndef _LPJS_JOB_H_
#define _LPJS_JOB_H_

// scanf() is good for converting numbers, but risks buffer overflows
// for strings, just like gets(), etc.
// Numeric fields must be grouped together before string fields
// for job_read_from_string()
// Numeric: job_id, array_index, job_count, processors_per_job, threads_per_process,
//          phys_mib_per_processor, chaperone_pid, job_pid, state
// String:  user_name, primary_group_name, submit_node, submit_dir,
//          script_name, compute_node, log_dir, pull_command, push_command
#define JOB_BASIC_NUMS_FORMAT   "%9lu %4lu %4u %3u %3u %5zu"
#define JOB_SPEC_NUMS_FORMAT    JOB_BASIC_NUMS_FORMAT " %u %u %u"
#define JOB_SPEC_STRINGS_FORMAT " %s %s %s %s %s %s %s\n%s\n%s\n%s\n"
#define JOB_SPEC_NUMERIC_FIELDS 9
// Complete job specs
#define JOB_SPEC_FORMAT         JOB_SPEC_NUMS_FORMAT JOB_SPEC_STRINGS_FORMAT
#define JOB_SPEC_STRING_FIELDS  10
// For lpjs jobs output
#define JOB_BASIC_PARAMS_HEADER \
    "    JobID  IDX  J/S P/J T/P MiB/P User Script Compute-node\n"
#define JOB_BASIC_PARAMS_FORMAT JOB_BASIC_NUMS_FORMAT " %s %s %s\n"
#define JOB_SPECS_ITEMS         (JOB_SPEC_NUMERIC_FIELDS + JOB_SPEC_STRING_FIELDS)

#define JOB_FIELD_MAX_LEN       1024
#define JOB_STR_MAX_LEN         2048    // Fixme: MAX_PATH + x?

#define JOB_NO_PATH             "not-set"
#define JOB_NO_PULL_CMD         "not-set"
#define JOB_NO_PUSH_CMD         "not-set"

typedef enum
{
    JOB_STATE_PENDING = 0,
    JOB_STATE_DISPATCHED,
    JOB_STATE_CANCELED,
    JOB_STATE_RUNNING
}   job_state_t;

typedef struct job  job_t;

#ifndef _STDIO_H_
#include <stdio.h>
#endif

#ifndef _UNISTD_H_
#include <unistd.h>
#endif

#include "job-rvs.h"
#include "job-accessors.h"
#include "job-mutators.h"
#include "job-protos.h"

#endif  // _LPJS_JOB_H_
