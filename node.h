#ifndef _LPJS_NODE_H_
#define _LPJS_NODE_H_

#ifndef _TIME_H_
#include <time.h>
#endif

#ifndef _STDIO_H_
#include <stdio.h>
#endif

typedef struct
{
    char            *hostname;
    unsigned        cores;
    unsigned        cores_used;
    unsigned long   phys_mem;
    unsigned long   phys_mem_used;
    int             zfs;        // 0 or 1
    char            *os;
    char            *arch;
    char            *state;     // FIXME: Use an enum, not a string
    int             msg_fd;
    time_t          last_ping;
}   node_t;

#define NODE_MSG_FD_NOT_OPEN        -1
#define NODE_STATUS_HEADER_FORMAT   "%-20s %-8s %5s %4s %7s %7s %-9s %-9s\n"
#define NODE_STATUS_FORMAT          "%-20s %-8s %5u %4u %7lu %7lu %-9s %-9s\n"

#include "node-rvs.h"
#include "node-accessors.h"
#include "node-mutators.h"

/* node.c */
void    node_init(node_t *node);
void    node_print_status_header(FILE *stream);
void    node_print_status(FILE *stream, node_t *node);
ssize_t node_send_specs(node_t *node, int fd);
void    node_send_status(node_t *node, int fd);
void    node_detect_specs(node_t *node);
ssize_t node_recv_specs(node_t *node, int msg_fd);

#endif  // _LPJS_NODE_H_
