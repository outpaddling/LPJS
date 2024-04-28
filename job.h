#ifndef _LPJS_JOB_H_
#define _LPJS_JOB_H_

// scanf() is good for converting numbers, but risks buffer overflows
// for strings, just like gets(), etc.
// Numeric fields must be grouped together before string fields
// for job_read_from_string()
#define JOB_SPEC_NUMS_FORMAT    "%9lu %3lu %4u %5u %6u %7lu"
#define JOB_SPEC_NUMERIC_FIELDS 6
// Complete job specs
#define JOB_SPEC_FORMAT         JOB_SPEC_NUMS_FORMAT " %s %s %s %s %s %s\n"
// For lpjs jobs output
#define JOB_BASIC_PARAMS_HEADER_FORMAT  "%9s %3s %4s %5s %6s %7s %s %s %s\n"
#define JOB_BASIC_PARAMS_FORMAT JOB_SPEC_NUMS_FORMAT " %s %s %s\n"
#define JOB_FIELD_MAX_LEN       1024
#define JOB_STR_MAX_LEN         2048    // Fixme: MAX_PATH + x?
#define JOB_SPECS_ITEMS         12      // # fields in the struct

typedef struct job  job_t;

#include <stdio.h>

#include "job-rvs.h"
#include "job-accessors.h"
#include "job-mutators.h"
#include "job-protos.h"

#endif  // _LPJS_JOB_H_
