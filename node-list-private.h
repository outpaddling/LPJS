#ifndef __H_
#define __H_

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef _LPJS_NODE_H_
#include "node.h"
#endif

#include "node-list.h"

struct node_list
{
    char        *head_node;
    unsigned    compute_node_count;
    node_t      *compute_nodes[LPJS_MAX_NODES];
};

#ifdef  __cplusplus
}
#endif

#endif  // #ifndef __H_
