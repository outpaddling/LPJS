#include <stdio.h>
#include "node.h"

/*
 *  Constructor for node_t
 */

void    node_init(node_t *node)

{
    node->hostname = NULL;
    node->mem = 0;
    node->mem_used = 0;
    node->cores = 0;
    node->cores_used = 0;
}
