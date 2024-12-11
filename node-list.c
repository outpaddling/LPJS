#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <string.h>
#include <stdlib.h>         // malloc()

#include <xtend/dsv.h>      // xt_dsv_read_field()
#include <xtend/file.h>
#include <xtend/string.h>   // strlcpy() on Linux

#include "node-list-private.h"
#include "network.h"
#include "lpjs.h"
#include "misc.h"


/***************************************************************************
 *  Use auto-c2man to generate a man page from this comment
 *
 *  Name:
 *      -
 *
 *  Library:
 *      #include <>
 *      -l
 *
 *  Description:
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
 *  2024-02-23  Jason Bacon Begin
 ***************************************************************************/

node_list_t *node_list_new(void)

{
    node_list_t *new_list;
    
    if ( (new_list = malloc(sizeof(node_list_t))) == NULL )
    {
	lpjs_log("%s(): Error: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    node_list_init(new_list);
    return new_list;
}


/***************************************************************************
 *  Description:
 *      Constructor for node_list_t
 *  
 *  History: 
 *  Date        Name        Modification
 *  2021-09-24  Jason Bacon Begin
 ***************************************************************************/

void    node_list_init(node_list_t *node_list)

{
    node_list->head_node = NULL;
    node_list->compute_node_count = 0;
}


/***************************************************************************
 *  Description:
 *      Update state and specs of a node after receiving info, e.g. from
 *      lpjs_compd
 *  
 *  History: 
 *  Date        Name        Modification
 *  2021-10-02  Jason Bacon Begin
 ***************************************************************************/

void    node_list_update_compute(node_list_t *node_list, node_t *node)

{
    size_t  c;
    char    *hostname;
    
    // Note: FQDN is required
    hostname = node_get_hostname(node);
    for (c = 0; c < node_list->compute_node_count; ++c)
    {
	if ( strcmp(node_get_hostname(node_list->compute_nodes[c]), hostname) == 0 )
	{
	    // lpjs_debug("Updating compute node %zu %s\n", c, NODE_HOSTNAME(node_list->compute_nodes[c]));
	    node_set_state(node_list->compute_nodes[c], "up");
	    node_set_procs(node_list->compute_nodes[c], node_get_procs(node));
	    node_set_phys_MiB(node_list->compute_nodes[c], node_get_phys_MiB(node));
	    node_set_zfs(node_list->compute_nodes[c], node_get_zfs(node));
	    node_set_os(node_list->compute_nodes[c], strdup(node_get_os(node)));
	    node_set_arch(node_list->compute_nodes[c], strdup(node_get_arch(node)));
	    node_set_msg_fd(node_list->compute_nodes[c], node_get_msg_fd(node));
	    node_set_last_ping(node_list->compute_nodes[c], node_get_last_ping(node));
	    return;
	}
    }
}


/***************************************************************************
 *  Description:
 *      Send current node list to msg_fd in human-readable format
 *  
 *  History: 
 *  Date        Name        Modification
 *  2021-09-26  Jason Bacon Begin
 ***************************************************************************/

void    node_list_send_status(int msg_fd, node_list_t *node_list)

{
    unsigned        c,
		    procs_up,
		    procs_up_used,
		    procs_down;
    unsigned long   mem_up,
		    mem_up_used,
		    mem_down;
    char            temp[LPJS_MSG_LEN_MAX + 1],
		    outgoing_msg[LPJS_MSG_LEN_MAX + 1];
    
    outgoing_msg[0] = '\0';
    
    snprintf(temp, LPJS_MSG_LEN_MAX,
	    NODE_STATUS_HEADER_FORMAT, "Hostname", "State",
	    "Procs", "Used", "PhysMiB", "Used", "OS", "Arch");
    strlcat(outgoing_msg, temp, LPJS_MSG_LEN_MAX + 1);
    
    procs_up = procs_up_used = procs_down = 0;
    mem_up = mem_up_used = mem_down = 0;
    
    for (c = 0; c < node_list->compute_node_count; ++c)
    {
	node_status_to_str(node_list->compute_nodes[c], temp, LPJS_MSG_LEN_MAX + 1);
	strlcat(outgoing_msg, temp, LPJS_MSG_LEN_MAX + 1);
	if ( strcmp(node_get_state(node_list->compute_nodes[c]), "up") == 0 )
	{
	    procs_up += node_get_procs(node_list->compute_nodes[c]);
	    procs_up_used += node_get_procs_used(node_list->compute_nodes[c]);
	    mem_up += node_get_phys_MiB(node_list->compute_nodes[c]);
	    mem_up_used += node_get_phys_MiB_used(node_list->compute_nodes[c]);
	}
	else
	{
	    procs_down += node_get_procs(node_list->compute_nodes[c]);
	    mem_down += node_get_phys_MiB(node_list->compute_nodes[c]);
	}
    }
    
    // lpjs_debug("Sending summary...\n");
    snprintf(temp, LPJS_MSG_LEN_MAX + 1,
	    "\n" NODE_STATUS_FORMAT, "Total", "up",
	    procs_up, procs_up_used, mem_up, mem_up_used, "-", "-");
    strlcat(outgoing_msg, temp, LPJS_MSG_LEN_MAX + 1);
    
    snprintf(temp, LPJS_MSG_LEN_MAX,
	    NODE_STATUS_FORMAT, "Total", "down",
	    procs_down, 0, mem_down, (size_t)0, "-", "-");
    strlcat(outgoing_msg, temp, LPJS_MSG_LEN_MAX + 1);

    // Only dispatchd calls this function, so wait for client to close first
    if ( lpjs_send_munge(msg_fd, outgoing_msg,
			 lpjs_dispatchd_safe_close) != LPJS_MSG_SENT )
    {
	lpjs_log("%s(): Error: Failed to send node list info.\n", __FUNCTION__);
	lpjs_dispatchd_safe_close(msg_fd);
	return; // FIXME: Define return codes
    }
    
    /*
     *  Closing the listener first results in "address already in use"
     *  errors on restart.  Send an EOT character to signal the end of
     *  transmission, so the client can close first and avoid a wait
     *  state for the socket.
     */
    if ( lpjs_send_munge(msg_fd, LPJS_EOT_MSG, lpjs_dispatchd_safe_close)
			 != LPJS_MSG_SENT )
	lpjs_log("%s(): Error: Failed to send EOT.\n", __FUNCTION__);
    else
	lpjs_log("%s(): EOT sent.\n", __FUNCTION__);
}


/***************************************************************************
 *  Use auto-c2man to generate a man page from this comment
 *
 *  Name:
 *      -
 *
 *  Library:
 *      #include <>
 *      -l
 *
 *  Description:
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
 *  2024-02-24  Jason Bacon Begin
 ***************************************************************************/

int     node_list_add_compute_node(node_list_t *node_list, node_t *node)

{
    if ( node_list->compute_node_count == LPJS_MAX_NODES )
    {
	lpjs_log("%s(): Error: LPJS_MAX_NODES reached.  Cannot add new node.\n", __FUNCTION__);
	return -1;
    }
    
    // lpjs_debug("%s(): Adding %s\n", __FUNCTION__, node_get_hostname(node));
    node_list->compute_nodes[node_list->compute_node_count++] = node;
    
    return 0;   // FIXME: Define return codes
}


node_t  *node_list_find_hostname(node_list_t *node_list, const char *hostname)

{
    int     c;
    node_t  *node;
    
    for (c = 0; c < node_list->compute_node_count; ++c)
    {
	node = node_list->compute_nodes[c];
	// lpjs_debug("%s(): Checking %s\n", __FUNCTION__, node_get_hostname(node));
	if ( strcmp(node_get_hostname(node), hostname) == 0 )
	    return node;
    }
    
    // hostname not found
    return NULL;
}


/***************************************************************************
 *  Use auto-c2man to generate a man page from this comment
 *
 *  Name:
 *      -
 *
 *  Library:
 *      #include <>
 *      -l
 *
 *  Description:
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
 *  2024-05-10  Jason Bacon Begin
 ***************************************************************************/

int     node_list_set_state(node_list_t *node_list, char *arg_string)

{
    char    *p,
	    *state,
	    *node_name;
    node_t  *node;
    
    puts(arg_string);
    
    p = arg_string;
    state = strsep(&p, " ");
    
    // Point state to static data so it will survive function exit
    if ( strcmp(state, "paused") == 0 )
	state = "paused";
    else if ( strcmp(state, "updating") == 0 )
	state = "updating";
    else
	state = "up";
    
    node_name = strsep(&p, " ");
    // nodes.c ensures that "all" is the only argument
    if ( strcmp(node_name, "all") == 0 )
    {
	printf("Setting all nodes to %s...\n", state);
	for (size_t c = 0; c < node_list->compute_node_count; ++c)
	    node_set_state(node_list->compute_nodes[c], state);
    }
    else
    {
	while ( node_name != NULL )
	{
	    node = node_list_find_hostname(node_list, node_name);
	    if ( node == NULL )
		lpjs_log("%s(): Error: Node %s not found.\n", __FUNCTION__, node_name);
	    else
	    {
		printf("Setting node %s to %s...\n", node_name, state);
		node_set_state(node, state);
	    }
	    node_name = strsep(&p, " ");
	}
    }
    
    return 0;   // FIXME: Define return codes
}
