#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

int queue_job(int msg_fd, const char *incoming_msg, node_list_t *node_list);

#endif
