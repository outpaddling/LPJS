/***************************************************************************
 *  This file is automatically generated by gen-get-set.  Be sure to keep
 *  track of any manual changes.
 *
 *  These generated functions are not expected to be perfect.  Check and
 *  edit as needed before adding to your code.
 ***************************************************************************/

#include <string.h>
#include <ctype.h>
#include <stdbool.h>        // In case of bool
#include <stdint.h>         // In case of int64_t, etc
#include <xtend/string.h>   // strlcpy() on Linux
#include "node-list-private.h"


/***************************************************************************
 *  Library:
 *      #include <node-list.h>
 *      
 *
 *  Description:
 *      Mutator for head_node member in a node_list_t structure.
 *      Use this function to set head_node in a node_list_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      head_node is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      node_list_ptr   Pointer to the structure to set
 *      new_head_node   The new value for head_node
 *
 *  Returns:
 *      NODE_LIST_DATA_OK if the new value is acceptable and assigned
 *      NODE_LIST_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_list_t     node_list;
 *      char *          new_head_node;
 *
 *      if ( node_list_set_head_node(&node_list, new_head_node)
 *              == NODE_LIST_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-23  gen-get-set Auto-generated from node-list-private.h
 ***************************************************************************/

int     node_list_set_head_node(node_list_t *node_list_ptr, char * new_head_node)

{
    if ( new_head_node == NULL )
	return NODE_LIST_DATA_OUT_OF_RANGE;
    else
    {
	node_list_ptr->head_node = new_head_node;
	return NODE_LIST_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node-list.h>
 *      
 *
 *  Description:
 *      Mutator for an array element of head_node member in a node_list_t
 *      structure. Use this function to set node_list_ptr->head_node[c]
 *      in a node_list_t object from non-member functions.
 *
 *  Arguments:
 *      node_list_ptr   Pointer to the structure to set
 *      c               Subscript to the head_node array
 *      new_head_node_element The new value for head_node[c]
 *
 *  Returns:
 *      NODE_LIST_DATA_OK if the new value is acceptable and assigned
 *      NODE_LIST_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_list_t     node_list;
 *      size_t          c;
 *      char *          new_head_node_element;
 *
 *      if ( node_list_set_head_node_ae(&node_list, c, new_head_node_element)
 *              == NODE_LIST_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      NODE_LIST_SET_HEAD_NODE_AE(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-23  gen-get-set Auto-generated from node-list-private.h
 ***************************************************************************/

int     node_list_set_head_node_ae(node_list_t *node_list_ptr, size_t c, char  new_head_node_element)

{
    if ( false )
	return NODE_LIST_DATA_OUT_OF_RANGE;
    else
    {
	node_list_ptr->head_node[c] = new_head_node_element;
	return NODE_LIST_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node-list.h>
 *      
 *
 *  Description:
 *      Mutator for head_node member in a node_list_t structure.
 *      Use this function to set head_node in a node_list_t object
 *      from non-member functions.  This function copies the array pointed to
 *      by new_head_node to node_list_ptr->head_node.
 *
 *  Arguments:
 *      node_list_ptr   Pointer to the structure to set
 *      new_head_node   The new value for head_node
 *      array_size      Size of the head_node array.
 *
 *  Returns:
 *      NODE_LIST_DATA_OK if the new value is acceptable and assigned
 *      NODE_LIST_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_list_t     node_list;
 *      char *          new_head_node;
 *      size_t          array_size;
 *
 *      if ( node_list_set_head_node_cpy(&node_list, new_head_node, array_size)
 *              == NODE_LIST_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      NODE_LIST_SET_HEAD_NODE(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-23  gen-get-set Auto-generated from node-list-private.h
 ***************************************************************************/

int     node_list_set_head_node_cpy(node_list_t *node_list_ptr, char * new_head_node, size_t array_size)

{
    if ( new_head_node == NULL )
	return NODE_LIST_DATA_OUT_OF_RANGE;
    else
    {
	// FIXME: Assuming char array is a null-terminated string
	strlcpy(node_list_ptr->head_node, new_head_node, array_size);
	return NODE_LIST_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node-list.h>
 *      
 *
 *  Description:
 *      Mutator for compute_node_count member in a node_list_t structure.
 *      Use this function to set compute_node_count in a node_list_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      compute_node_count is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      node_list_ptr   Pointer to the structure to set
 *      new_compute_node_count The new value for compute_node_count
 *
 *  Returns:
 *      NODE_LIST_DATA_OK if the new value is acceptable and assigned
 *      NODE_LIST_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_list_t     node_list;
 *      unsigned        new_compute_node_count;
 *
 *      if ( node_list_set_compute_node_count(&node_list, new_compute_node_count)
 *              == NODE_LIST_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-23  gen-get-set Auto-generated from node-list-private.h
 ***************************************************************************/

int     node_list_set_compute_node_count(node_list_t *node_list_ptr, unsigned new_compute_node_count)

{
    if ( false )
	return NODE_LIST_DATA_OUT_OF_RANGE;
    else
    {
	node_list_ptr->compute_node_count = new_compute_node_count;
	return NODE_LIST_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node-list.h>
 *      
 *
 *  Description:
 *      Mutator for an array element of compute_nodes member in a node_list_t
 *      structure. Use this function to set node_list_ptr->compute_nodes[c]
 *      in a node_list_t object from non-member functions.
 *
 *  Arguments:
 *      node_list_ptr   Pointer to the structure to set
 *      c               Subscript to the compute_nodes array
 *      new_compute_nodes_element The new value for compute_nodes[c]
 *
 *  Returns:
 *      NODE_LIST_DATA_OK if the new value is acceptable and assigned
 *      NODE_LIST_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_list_t     node_list;
 *      size_t          c;
 *      node_t *        new_compute_nodes_element;
 *
 *      if ( node_list_set_compute_nodes_ae(&node_list, c, new_compute_nodes_element)
 *              == NODE_LIST_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      NODE_LIST_SET_COMPUTE_NODES_AE(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-23  gen-get-set Auto-generated from node-list-private.h
 ***************************************************************************/

int     node_list_set_compute_nodes_ae(node_list_t *node_list_ptr, size_t c, node_t *new_compute_nodes_element)

{
    if ( false )
	return NODE_LIST_DATA_OUT_OF_RANGE;
    else
    {
	node_list_ptr->compute_nodes[c] = new_compute_nodes_element;
	return NODE_LIST_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node-list.h>
 *      
 *
 *  Description:
 *      Mutator for compute_nodes member in a node_list_t structure.
 *      Use this function to set compute_nodes in a node_list_t object
 *      from non-member functions.  This function copies the array pointed to
 *      by new_compute_nodes to node_list_ptr->compute_nodes.
 *
 *  Arguments:
 *      node_list_ptr   Pointer to the structure to set
 *      new_compute_nodes The new value for compute_nodes
 *      array_size      Size of the compute_nodes array.
 *
 *  Returns:
 *      NODE_LIST_DATA_OK if the new value is acceptable and assigned
 *      NODE_LIST_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_list_t     node_list;
 *      node_t *        new_compute_nodes;
 *      size_t          array_size;
 *
 *      if ( node_list_set_compute_nodes_cpy(&node_list, new_compute_nodes, array_size)
 *              == NODE_LIST_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      NODE_LIST_SET_COMPUTE_NODES(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-23  gen-get-set Auto-generated from node-list-private.h
 ***************************************************************************/

int     node_list_set_compute_nodes_cpy(node_list_t *node_list_ptr, node_t * new_compute_nodes[], size_t array_size)

{
    if ( new_compute_nodes == NULL )
	return NODE_LIST_DATA_OUT_OF_RANGE;
    else
    {
	size_t  c;
	
	// FIXME: Assuming all elements should be copied
	for (c = 0; c < array_size; ++c)
	    node_list_ptr->compute_nodes[c] = new_compute_nodes[c];
	return NODE_LIST_DATA_OK;
    }
}
