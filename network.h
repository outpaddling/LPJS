#ifndef _LPJS_NETWORK_H_
#define _LPJS_NETWORK_H_

/*
 *  First byte of each new conversation is a request code.  All codes
 *  to dispatchd must be unique as must codes to compd.
 *  Don't start codes at 0.  It will be interpreted as a null-terminator.
 */

enum
{
    // Internal events
    LPJS_DISPATCHD_REQUEST_COMPD_CHECKIN = 1,
    LPJS_DISPATCHD_REQUEST_CHAPERONE_STATUS,
    LPJS_DISPATCHD_REQUEST_JOB_STARTED,
    LPJS_DISPATCHD_REQUEST_JOB_COMPLETE,
    // User command requests
    LPJS_DISPATCHD_REQUEST_NODE_LIST,
    LPJS_DISPATCHD_REQUEST_JOB_LIST,
    LPJS_DISPATCHD_REQUEST_SUBMIT,
    LPJS_DISPATCHD_REQUEST_CANCEL,
    LPJS_DISPATCHD_REQUEST_PAUSE,
    LPJS_DISPATCHD_REQUEST_RESUME
};

enum
{
    LPJS_COMPD_REQUEST_NEW_JOB = 1,
    LPJS_COMPD_REQUEST_CANCEL
};

typedef enum
{
    // Sent right after fork on the new job request msg_fd
    // dispatchd should either receive either this or time out waiting
    // This msg_fd is closed after sending, so dispatchd can resume
    // monitoring for new requests
    LPJS_CHAPERONE_FORKED = 1,
    
    // The rest are sent by chaperone on a new socket connection
    LPJS_CHAPERONE_OK,
    LPJS_CHAPERONE_SCRIPT_FAILED,
    LPJS_CHAPERONE_OSERR,
    LPJS_CHAPERONE_CANTCREAT,
    LPJS_CHAPERONE_EXEC_FAILED
}   chaperone_status_t;


#define LPJS_MSG_SENT       0
#define LPJS_SEND_FAILED    -2
#define LPJS_MUNGE_FAILED   -3

// Must be <= 0, since recv returns number of bytes
#define LPJS_RECV_FAILED    -1  // bytes returned
#define LPJS_RECV_TIMEOUT   -2  // bytes returned
// FIXME: Getting spurious timeouts on dispatch response
// Keep timeouts small so dispatchd doesn't hang waiting for a msg
#define LPJS_CHAPERONE_STATUS_TIMEOUT   500000
#define LPJS_PRINT_RESPONSE_TIMEOUT     500000
#define LPJS_CONNECT_TIMEOUT            500000

#define LPJS_EOT                '\004'
#define LPJS_EOT_MSG            "\004"

// IPv6 max address size is 39
#define LPJS_TEXT_IP_ADDRESS_MAX    64
// FIXME: 4096 is just a guestimate
#define LPJS_MSG_LEN_MAX            LPJS_PAYLOAD_MAX + 4096
#define LPJS_CONNECTION_QUEUE_MAX   4096    // Should be more than enough

/*
 *  Use a different TCP port than SLURM (6817), so that the same head
 *  node can be used to manage both LPJS and SLURM compute nodes.
 *  This allows gradual migration of a cluster between schedulers,
 *  or partitioning of compute resources for other reasons.
 *  LPJS and SLURM compute nodes must be mutually exclusive.  A node
 *  cannot be used by both.
 */
#define LPJS_IP_TCP_PORT                (short)6818 // Need short for htons()
#define LPJS_RETRY_TIME                 5
#define LPJS_WRONG_VERSION_RETRY_TIME   60
#define LPJS_MUNGE_CRED_VERIFIED_MSG    "MCD"
#define LPJS_WRONG_VERSION_MSG          "Wrong LPJS version"
#define LPJS_NODE_AUTHORIZED_MSG        "Node authorized"
#define LPJS_NODE_NOT_AUTHORIZED_MSG    "Node not authorized"

#ifndef _SYS_POLL_H_
#include <sys/poll.h>
#endif

#ifndef _NODE_LIST_H_
#include "node-list.h"
#endif

#include "network-protos.h"

#endif
