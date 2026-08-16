#ifndef __H_
#define __H_

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef _TIME_H_
#include <time.h>
#endif

#ifndef true
#include <stdbool.h>
#endif

struct node
{
    char            *hostname;
    unsigned        processors;
    unsigned        processors_used;
    // size_t will suffice even if head node is 32-bit (2^32-1 MiB)
    size_t          phys_MiB;
    size_t          phys_MiB_used;
    // These indicate that processors and phys_MiB were not specified in
    // the config file.  We can't just rely on processors == 0 or
    // phys_MiB == 0 to indicate that they are auto-detected.
    // They will be non-zero if the node has checked in before.
    bool            auto_processors;    // Processors are auto-detected
    bool            auto_phys_MiB;      // MiB is auto-detected
    int             zfs;                // 0 or 1
    char            *os;
    char            *arch;
    char            *state;
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
