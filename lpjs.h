#ifndef _LPJS_H_
#define _LPJS_H_

#ifndef _NODE_LIST_H_
#include "node-list.h"
#endif

#ifndef _JOB_LIST_H_
#include "job-list.h"
#endif

#define LPJS_FIELD_MAX          1024
#define LPJS_CMD_MAX            4096
#define LPJS_PAYLOAD_MAX_LEN    4096    // FIXME: Does munge have a max?
#define LPJS_NO_SELECT_TIMEOUT  NULL

#define LPJS_LOG_DIR            PREFIX "/var/log/lpjs"
#define LPJS_COMPD_LOG          LPJS_LOG_DIR "/compd"
#define LPJS_DISPATCHD_LOG      LPJS_LOG_DIR "/dispatchd"
#define LPJS_JOB_LOG            LPJS_LOG_DIR "/jobs"

#define LPJS_SPOOL_DIR          PREFIX "/var/spool/lpjs"
#define LPJS_PENDING_DIR        LPJS_SPOOL_DIR "/pending"
#define LPJS_RUNNING_DIR        LPJS_SPOOL_DIR "/running"

#define LPJS_RUN_DIR            PREFIX "/var/run/lpjs"

#define LPJS_SCRIPT_SIZE_MAX    64 * 1024

#define LPJS_MB                 1000000
#define LPJS_MiB                1048576
#define LPJS_GB                 1000000000
#define LPJS_GiB                1073741824

// Don't start codes at 0.  It will be interpreted as a null-terminator
enum
{
    LPJS_REQUEST_COMPD_CHECKIN = 1,
    LPJS_REQUEST_NODE_STATUS,
    LPJS_REQUEST_JOB_STATUS,
    LPJS_REQUEST_SUBMIT
};

// Don't start codes at 0.  It will be interpreted as a null-terminator
enum
{
    LPJS_NOTICE_JOB_COMPLETE = 1
};

#endif  // _LPJS_H_
