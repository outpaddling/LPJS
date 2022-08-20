#ifndef _LPJS_H_
#define _LPJS_H_

#ifndef _NODE_LIST_H_
#include "node-list.h"
#endif

#ifndef _JOB_LIST_H_
#include "job-list.h"
#endif

#ifndef _MACHDEP_H_
#include "machdep.h"
#endif

#define LPJS_FIELD_MAX      1024
#define LPJS_CMD_MAX        4096
#define LPJS_IP_MSG_MAX     4096
#define LPJS_MSG_QUEUE_MAX  10

#define LPJS_CMD_MAX        4096

int process_events(node_list_t *nodes, job_list_t *job_list);
void terminate_daemon(int signal);
void log_job(const char *incoming_msg);

#endif
