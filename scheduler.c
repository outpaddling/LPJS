#include <stdio.h>
#include "node-list.h"

/***************************************************************************
 *  Description:
 *      Add a job to the queue
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-30  Jason Bacon Begin
 ***************************************************************************/

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


/***************************************************************************
 *  Library:
 *      #include <>
 *      -l
 *
 *  Description:
 *      Select nodes to run a pending job
 *
 *      Default to packing jobs as densely as possible, i.e. use up
 *      available cores on already busy nodes before allocating cores on
 *      idle nodes.  This will allow faster deployment of shared memory
 *      parallel jobs, which need many cores on the same node.
 *  
 *  Arguments:
 *
 *  Returns:
 *
 *  Examples:
 *
 *  Files:
 *
 *  Environment
 *
 *  See also:
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-10-05  Jason Bacon Begin
 ***************************************************************************/

int     select_nodes()

{
    return 0;
}
