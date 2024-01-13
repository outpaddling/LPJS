#ifndef _LPJS_NETWORK_H_
#define _LPJS_NETWORK_H_

// IPv6 max address size is 39
#define LPJS_IP_MAX             64
// FIXME: Pick a good default and check config file for override
#define LPJS_IP_TCP_PORT        (short)3000 // Need short for htonl()
#define LPJS_IP_MSG_MAX         4096
#define LPJS_IP_MSG_QUEUE_MAX   10

#define LPJS_DISPATCHD_TCP_PORT   (short)3000 // Need short for htons()

#ifndef _NODE_LIST_H_
#include "node-list.h"
#endif

int connect_to_dispatchd(node_list_t *node_list);
int print_response(int msg_fd, const char *caller_name);
int send_msg(int msg_fd, const char *format, ...);

#endif
