#include <stdio.h>
#include "node-list.h"

int     queue_job(int msg_fd, const char *incoming_msg, node_list_t *node_list)

{
    node_t  *first_node = &NODE_LIST_COMPUTE_NODES_AE(node_list, 0);
    
    /*
     *  Temporarily allocate 1 core on first node unconditionally to test
     *  dispatch
     */

    dprintf(msg_fd, "%s 1", NODE_HOSTNAME(first_node));
    return 0;
}
