/*
 *  Separation of interface and implementation is one of the core
 *  concepts of OOP.  This file contains pseudo-accessors and
 *  pseudo-mutators for the node class, i.e. accessors and mutators
 *  for members that don't really exist in the implementation.
 *  Storing available nodes and memory in the structure would be
 *  somewhat redundant, since it a trivial calculation total - used.
 *  Maintaining two separate variables/members that must be kept
 *  in sync would be an error-prone implementation strategy.
 */

#include "node-private.h"

unsigned    node_get_cores_available(node_t *node)

{
    return node->cores - node->cores_used;
}


int     node_set_cores_available(node_t *node, unsigned cores)

{
    if ( cores > node->cores )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node->cores_used = node->cores - cores;
	return NODE_DATA_OK;
    }
}


unsigned    node_get_phys_MiB_available(node_t *node)

{
    return node->phys_MiB - node->phys_MiB_used;
}


int     node_set_phys_MiB_available(node_t *node, unsigned phys_MiB)

{
    if ( phys_MiB > node->phys_MiB )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node->phys_MiB_used = node->phys_MiB - phys_MiB;
	return NODE_DATA_OK;
    }
}
