#ifndef _NODE_LIST_H_
#define _NODE_LIST_H_

#ifndef _NODE_H_
#include "node.h"
#endif

#define LPJS_MAX_NODES  1024

typedef struct
{
    unsigned    count;
    node_t      nodes[LPJS_MAX_NODES];
}   node_list_t;

void    node_list_init(node_list_t *node_list);
int     node_list_populate(node_list_t *node_list, const char *filename);

#endif  // _NODE_LIST_H_
