#ifndef _LPJS_NETWORK_H_
#define _LPJS_NETWORK_H_

#define LPJS_IP_MAX     64  // IPv6 max address size is 39
// FIXME: Pick a good default and check config file for override
#define LPJS_TCP_PORT   3000
#define LPJS_MSG_MAX    4096

#ifndef _NODE_LIST_H_
#include "node-list.h"
#endif

void resolve_hostname(const char *hostname, char *ip, size_t ip_buff_len);
int connect_to_dispatch(node_list_t *node_list);
int print_response(int msg_fd, const char *caller_name);
int send_msg(int msg_fd, const char *format, ...);

#endif
