#ifndef _LPJS_CONFIG_H_
#define _LPJS_CONFIG_H_

#ifndef _NODE_LIST_H_
#include "node-list.h"
#endif

#define LPJS_CONFIG_ALL         0
#define LPJS_CONFIG_HEAD_ONLY   1

int     lpjs_load_config(node_list_t *node_list, int flags, FILE *error_stream);

#endif
