#ifndef _LPJS_NODE_H_
#define _LPJS_NODE_H_

#ifndef _TIME_H_
#include <time.h>
#endif

#ifndef _STDIO_H_
#include <stdio.h>
#endif

#ifndef _LPJS_NODE_H_
#include "node.h"
#endif

#ifndef _LPJS_JOB_H_
#include "job.h"
#endif

typedef struct node node_t;

#define NODE_MSG_FD_NOT_OPEN        -1
#define NODE_STATUS_HEADER_FORMAT   "%-20s %-8s %5s %4s %7s %7s %-9s %-9s\n"
#define NODE_STATUS_FORMAT          "%-20s %-8s %5u %4u %7zu %7zu %-9s %-9s\n"
#define NODE_SPECS_LEN              1024

#include "node-rvs.h"
#include "node-accessors.h"
#include "node-mutators.h"
#include "node-protos.h"
#include "node-pseudo-protos.h"

#endif  // _LPJS_NODE_H_
