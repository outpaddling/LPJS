#ifndef _LPJS_NETWORK_H_
#define _LPJS_NETWORK_H_

// IPv6 max address size is 39
#define LPJS_TEXT_IP_ADDRESS_MAX    64
#define LPJS_MSG_LEN_MAX            4096
#define LPJS_CONNECTION_QUEUE_MAX   4096    // Should be more than enough

/*
 *  Use the same default TCP port as SLURM (6817), since only one
 *  scheduler can be running on a given cluster.
 */
#define LPJS_IP_TCP_PORT        (short)6817 // Need short for htons()
#define LPJS_RETRY_TIME         5

#define LPJS_MUNGE_CRED_VERIFIED     "MCD"

#ifndef _SYS_POLL_H_
#include <sys/poll.h>
#endif

#ifndef _NODE_LIST_H_
#include "node-list.h"
#endif

#include "network-protos.h"

#endif
