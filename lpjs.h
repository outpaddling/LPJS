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
#define LPJS_NO_SELECT_TIMEOUT  NULL

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
