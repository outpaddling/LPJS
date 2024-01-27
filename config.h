#ifndef _LPJS_CONFIG_H_
#define _LPJS_CONFIG_H_

#include <limits.h>     // PATH_MAX

#ifndef _NODE_LIST_H_
#include "node-list.h"
#endif

#define LPJS_CONFIG_ALL         0
#define LPJS_CONFIG_HEAD_ONLY   1

typedef struct
{
    char    log_dir[PATH_MAX + 1];
}   lpjs_config_t;

#include "config-protos.h"

#endif
