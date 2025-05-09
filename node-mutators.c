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
#include "node-private.h"


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for hostname member in a node_t structure.
 *      Use this function to set hostname in a node_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      hostname is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      new_hostname    The new value for hostname
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      char *          new_hostname;
 *
 *      if ( node_set_hostname(&node, new_hostname)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_hostname(node_t *node_ptr, char * new_hostname)

{
    if ( new_hostname == NULL )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node_ptr->hostname = new_hostname;
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for an array element of hostname member in a node_t
 *      structure. Use this function to set node_ptr->hostname[c]
 *      in a node_t object from non-member functions.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      c               Subscript to the hostname array
 *      new_hostname_element The new value for hostname[c]
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      size_t          c;
 *      char *          new_hostname_element;
 *
 *      if ( node_set_hostname_ae(&node, c, new_hostname_element)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      NODE_SET_HOSTNAME_AE(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_hostname_ae(node_t *node_ptr, size_t c, char  new_hostname_element)

{
    if ( false )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node_ptr->hostname[c] = new_hostname_element;
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for hostname member in a node_t structure.
 *      Use this function to set hostname in a node_t object
 *      from non-member functions.  This function copies the array pointed to
 *      by new_hostname to node_ptr->hostname.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      new_hostname    The new value for hostname
 *      array_size      Size of the hostname array.
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      char *          new_hostname;
 *      size_t          array_size;
 *
 *      if ( node_set_hostname_cpy(&node, new_hostname, array_size)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      NODE_SET_HOSTNAME(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_hostname_cpy(node_t *node_ptr, char * new_hostname, size_t array_size)

{
    if ( new_hostname == NULL )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	// FIXME: Assuming char array is a null-terminated string
	strlcpy(node_ptr->hostname, new_hostname, array_size);
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for processors member in a node_t structure.
 *      Use this function to set processors in a node_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      processors is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      new_processors       The new value for processors
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      unsigned        new_processors;
 *
 *      if ( node_set_processors(&node, new_processors)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_processors(node_t *node_ptr, unsigned new_processors)

{
    if ( false )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node_ptr->processors = new_processors;
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for processors_used member in a node_t structure.
 *      Use this function to set processors_used in a node_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      processors_used is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      new_processors_used  The new value for processors_used
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      unsigned        new_processors_used;
 *
 *      if ( node_set_processors_used(&node, new_processors_used)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_processors_used(node_t *node_ptr, unsigned new_processors_used)

{
    if ( false )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node_ptr->processors_used = new_processors_used;
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for phys_MiB member in a node_t structure.
 *      Use this function to set phys_MiB in a node_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      phys_MiB is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      new_phys_MiB    The new value for phys_MiB
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      unsigned long   new_phys_MiB;
 *
 *      if ( node_set_phys_MiB(&node, new_phys_MiB)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_phys_MiB(node_t *node_ptr, unsigned long new_phys_MiB)

{
    if ( false )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node_ptr->phys_MiB = new_phys_MiB;
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for phys_MiB_used member in a node_t structure.
 *      Use this function to set phys_MiB_used in a node_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      phys_MiB_used is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      new_phys_MiB_used The new value for phys_MiB_used
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      unsigned long   new_phys_MiB_used;
 *
 *      if ( node_set_phys_MiB_used(&node, new_phys_MiB_used)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_phys_MiB_used(node_t *node_ptr, unsigned long new_phys_MiB_used)

{
    if ( false )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node_ptr->phys_MiB_used = new_phys_MiB_used;
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for zfs member in a node_t structure.
 *      Use this function to set zfs in a node_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      zfs is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      new_zfs         The new value for zfs
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      int             new_zfs;
 *
 *      if ( node_set_zfs(&node, new_zfs)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_zfs(node_t *node_ptr, int new_zfs)

{
    if ( false )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node_ptr->zfs = new_zfs;
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for os member in a node_t structure.
 *      Use this function to set os in a node_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      os is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      new_os          The new value for os
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      char *          new_os;
 *
 *      if ( node_set_os(&node, new_os)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_os(node_t *node_ptr, char * new_os)

{
    if ( new_os == NULL )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node_ptr->os = new_os;
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for an array element of os member in a node_t
 *      structure. Use this function to set node_ptr->os[c]
 *      in a node_t object from non-member functions.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      c               Subscript to the os array
 *      new_os_element  The new value for os[c]
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      size_t          c;
 *      char *          new_os_element;
 *
 *      if ( node_set_os_ae(&node, c, new_os_element)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      NODE_SET_OS_AE(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_os_ae(node_t *node_ptr, size_t c, char  new_os_element)

{
    if ( false )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node_ptr->os[c] = new_os_element;
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for os member in a node_t structure.
 *      Use this function to set os in a node_t object
 *      from non-member functions.  This function copies the array pointed to
 *      by new_os to node_ptr->os.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      new_os          The new value for os
 *      array_size      Size of the os array.
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      char *          new_os;
 *      size_t          array_size;
 *
 *      if ( node_set_os_cpy(&node, new_os, array_size)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      NODE_SET_OS(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_os_cpy(node_t *node_ptr, char * new_os, size_t array_size)

{
    if ( new_os == NULL )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	// FIXME: Assuming char array is a null-terminated string
	strlcpy(node_ptr->os, new_os, array_size);
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for arch member in a node_t structure.
 *      Use this function to set arch in a node_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      arch is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      new_arch        The new value for arch
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      char *          new_arch;
 *
 *      if ( node_set_arch(&node, new_arch)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_arch(node_t *node_ptr, char * new_arch)

{
    if ( new_arch == NULL )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node_ptr->arch = new_arch;
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for an array element of arch member in a node_t
 *      structure. Use this function to set node_ptr->arch[c]
 *      in a node_t object from non-member functions.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      c               Subscript to the arch array
 *      new_arch_element The new value for arch[c]
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      size_t          c;
 *      char *          new_arch_element;
 *
 *      if ( node_set_arch_ae(&node, c, new_arch_element)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      NODE_SET_ARCH_AE(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_arch_ae(node_t *node_ptr, size_t c, char  new_arch_element)

{
    if ( false )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node_ptr->arch[c] = new_arch_element;
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for arch member in a node_t structure.
 *      Use this function to set arch in a node_t object
 *      from non-member functions.  This function copies the array pointed to
 *      by new_arch to node_ptr->arch.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      new_arch        The new value for arch
 *      array_size      Size of the arch array.
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      char *          new_arch;
 *      size_t          array_size;
 *
 *      if ( node_set_arch_cpy(&node, new_arch, array_size)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      NODE_SET_ARCH(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_arch_cpy(node_t *node_ptr, char * new_arch, size_t array_size)

{
    if ( new_arch == NULL )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	// FIXME: Assuming char array is a null-terminated string
	strlcpy(node_ptr->arch, new_arch, array_size);
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for state member in a node_t structure.
 *      Use this function to set state in a node_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      state is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      new_state       The new value for state
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      char *          new_state;
 *
 *      if ( node_set_state(&node, new_state)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_state(node_t *node_ptr, char * new_state)

{
    if ( new_state == NULL )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node_ptr->state = new_state;
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for an array element of state member in a node_t
 *      structure. Use this function to set node_ptr->state[c]
 *      in a node_t object from non-member functions.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      c               Subscript to the state array
 *      new_state_element The new value for state[c]
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      size_t          c;
 *      char *          new_state_element;
 *
 *      if ( node_set_state_ae(&node, c, new_state_element)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      NODE_SET_STATE_AE(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_state_ae(node_t *node_ptr, size_t c, char  new_state_element)

{
    if ( false )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node_ptr->state[c] = new_state_element;
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for state member in a node_t structure.
 *      Use this function to set state in a node_t object
 *      from non-member functions.  This function copies the array pointed to
 *      by new_state to node_ptr->state.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      new_state       The new value for state
 *      array_size      Size of the state array.
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      char *          new_state;
 *      size_t          array_size;
 *
 *      if ( node_set_state_cpy(&node, new_state, array_size)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      NODE_SET_STATE(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_state_cpy(node_t *node_ptr, char * new_state, size_t array_size)

{
    if ( new_state == NULL )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	// FIXME: Assuming char array is a null-terminated string
	strlcpy(node_ptr->state, new_state, array_size);
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for msg_fd member in a node_t structure.
 *      Use this function to set msg_fd in a node_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      msg_fd is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      new_msg_fd      The new value for msg_fd
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      int             new_msg_fd;
 *
 *      if ( node_set_msg_fd(&node, new_msg_fd)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_msg_fd(node_t *node_ptr, int new_msg_fd)

{
    if ( false )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node_ptr->msg_fd = new_msg_fd;
	return NODE_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <node.h>
 *      
 *
 *  Description:
 *      Mutator for last_ping member in a node_t structure.
 *      Use this function to set last_ping in a node_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      last_ping is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      node_ptr        Pointer to the structure to set
 *      new_last_ping   The new value for last_ping
 *
 *  Returns:
 *      NODE_DATA_OK if the new value is acceptable and assigned
 *      NODE_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      node_t          node;
 *      time_t          new_last_ping;
 *
 *      if ( node_set_last_ping(&node, new_last_ping)
 *              == NODE_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  gen-get-set Auto-generated from node-private.h
 ***************************************************************************/

int     node_set_last_ping(node_t *node_ptr, time_t new_last_ping)

{
    if ( false )
	return NODE_DATA_OUT_OF_RANGE;
    else
    {
	node_ptr->last_ping = new_last_ping;
	return NODE_DATA_OK;
    }
}
