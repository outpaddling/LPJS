#ifndef _NODE_LIST_H_
#define _NODE_LIST_H_

#ifndef _NODE_H_
#include "node.h"
#endif

typedef struct
{
    unsigned    count;
    node_t      *nodes;
}   node_list_t;

void    node_list_init(node_list_t *node_list);

#endif  // _NODE_LIST_H_
