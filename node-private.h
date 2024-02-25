#ifndef __H_
#define __H_

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef _SYS_STDINT_H_
#include <stdint.h>
#endif

#ifndef _TIME_H_
#include <time.h>
#endif

struct node
{
    char            *hostname;
    unsigned        cores;
    unsigned        cores_used;
    uint64_t        phys_mem;
    uint64_t        phys_mem_used;
    int             zfs;        // 0 or 1
    char            *os;
    char            *arch;
    char            *state;     // FIXME: Use an enum, not a string
    int             msg_fd;
    // For detecting odd comm issues, where socket connection drop
    // cannot be detected directly
    time_t          last_ping;
};

#include "node.h"

#ifdef  __cplusplus
}
#endif

#endif
