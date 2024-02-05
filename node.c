#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sysexits.h>
#include <sys/utsname.h>
#include <sys/socket.h>     // MSG_WAIT_ALL

#include <xtend/file.h>     // xt_dprintf()

#include "node-private.h"
#include "network.h"
#include "lpjs.h"
#include "misc.h"


/***************************************************************************
 *  Description:
 *  
 *  Returns:
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  Jason Bacon Begin
 ***************************************************************************/

node_t  *node_new(void)

{
    node_t  *node;
    
    if ( (node = malloc(sizeof(node_t))) == NULL )
    {
	lpjs_log("%s(): malloc failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    node_init(node);
    
    return node;
}


/***************************************************************************
 *  Description:
 *      Constructor for node_t
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-23  Jason Bacon Begin
 ***************************************************************************/

void    node_init(node_t *node)

{
    node->hostname = NULL;
    node->phys_mem = 0;
    node->phys_mem_used = 0;
    node->cores = 0;
    node->cores_used = 0;
    node->zfs = 0;
    node->os = "Unknown";
    node->arch = "Unknown";
    node->state = "Unknown";
    node->msg_fd = NODE_MSG_FD_NOT_OPEN;
}


/***************************************************************************
 *  Description:
 *      Detect hardware specs and OS of the node running this function
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-10-02  Jason Bacon Begin
 ***************************************************************************/

void    node_detect_specs(node_t *node)

{
    struct utsname  u_name;
    char            temp_hostname[sysconf(_SC_HOST_NAME_MAX) + 1];
    
    /*
     *  hwloc is extremely complex and we don't need most of its functionality
     *  here, so just gather info the simple way
     */

    node_init(node);
    
    // FIXME: Verify malloc() success
    gethostname(temp_hostname, sysconf(_SC_HOST_NAME_MAX));
    node->hostname = strdup(temp_hostname);
    // May include SMT/Hyperthreading.  Disable in BIOS for Linux or using
    // sysctl on FreeBSD if you don't want to oversubscribe physical cores.
    node->cores = sysconf(_SC_NPROCESSORS_ONLN);
    node->phys_mem = sysconf(_SC_PAGESIZE) * sysconf(_SC_PHYS_PAGES)
		     / 1024 / 1024;
    /*
     *  Report 1 if ZFS filesystem found, so that additional memory
     *  can be reserved on compute nodes.
     *  FIXME: There should be a better approach to this.
     */
    node->zfs = ! system("mount | fgrep -q zfs");
    uname(&u_name);
    // FIXME: Verify malloc() success
    node->os = strdup(u_name.sysname);
    node->arch = strdup(u_name.machine);
}


/***************************************************************************
 *  Description:
 *      Print header for node status info
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-28  Jason Bacon Begin
 ***************************************************************************/

void    node_print_status_header(FILE *stream)

{
    fprintf(stderr, NODE_STATUS_HEADER_FORMAT, "Hostname", "State",
	  "Cores", "Used", "Physmem", "Used", "OS", "Arch");
}


/***************************************************************************
 *  Description:
 *      Print current nodes in human-readable form
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-23  Jason Bacon Begin
 ***************************************************************************/

void    node_print_status(FILE *stream, node_t *node)

{
    fprintf(stream, NODE_STATUS_FORMAT, node->hostname, node->state,
	   node->cores, node->cores_used,
	   node->phys_mem, node->phys_mem_used, node->os, node->arch);
}


/***************************************************************************
 *  Description:
 *      Send current node info to msg_fd in human-readable form, e.g. in
 *      response to lpjs-nodes
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-23  Jason Bacon Begin
 ***************************************************************************/

void    node_send_status(node_t *node, int msg_fd)

{
    /*
     *  Don't use send_msg() here, since there will be more text to send
     *  and send_msg() terminates the message.
     */
    if ( xt_dprintf(msg_fd, NODE_STATUS_FORMAT, node->hostname, node->state,        
		node->cores, node->cores_used,                                 
		node->phys_mem, node->phys_mem_used, node->os, node->arch) < 0 )
    {
	lpjs_log("send_node_specs(): xt_dprintf() failed: %s", strerror(errno));
	exit(EX_IOERR);
    }
}


/***************************************************************************
 *  Description:
 *      Send node hardware and OS specs to msg_fd in
 *      machine-readable for, e.g. from compd to dispatchd
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-10-02  Jason Bacon Begin
 ***************************************************************************/

ssize_t node_send_specs(node_t *node, int msg_fd)

{
    char        specs_msg[LPJS_MSG_LEN_MAX + 1];
    extern FILE *Log_stream;
    
    node_print_status_header(Log_stream);
    node_print_status(Log_stream, node);
    if ( snprintf(specs_msg, LPJS_MSG_LEN_MAX + 1,
		  "%s\t%s\t%u\t%lu\t%u\t%s\t%s\n",
		  node->hostname, node->state, node->cores,
		  node->phys_mem, node->zfs, node->os, node->arch) < 0 )
    {
	perror("send_node_specs(): snprintf() failed");
	exit(EX_IOERR);
    }
    
    return lpjs_send_msg(msg_fd, 0, specs_msg);
}


/***************************************************************************
 *  Description:
 *      Receive node hardware and OS specs via msg_fd in
 *      machine-readable for, e.g. from compd to dispatchd
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-10-02  Jason Bacon Begin
 ***************************************************************************/

ssize_t node_recv_specs(node_t *node, int msg_fd)

{
    char    specs_msg[LPJS_MSG_LEN_MAX + 1],
	    *field,
	    *stringp,
	    *end;
    ssize_t msg_len;
    
    node_init(node);
    
    msg_len = lpjs_recv_msg(msg_fd, specs_msg, LPJS_MSG_LEN_MAX, MSG_WAITALL, 0);
    if ( msg_len < 0 )
    {
	lpjs_log("node_recv_specs(): Failed to receive message.\n");
	node->state = "Unknown";
	node->os = "Unknown";
	node->arch = "Unknown";
	return -1;
    }
    else
    {
	stringp = specs_msg;
	
	if ( (field = strsep(&stringp, "\t")) == NULL )
	{
	    lpjs_log("node_recv_specs(): Failed to extract hostname from specs.\n");
	    return -1;
	}
	node->hostname = strdup(field);

	if ( (field = strsep(&stringp, "\t")) == NULL )
	{
	    lpjs_log("node_recv_specs(): Failed to extract state from specs.\n");
	    return -1;
	}
	node->state = strdup(field);

	if ( (field = strsep(&stringp, "\t")) == NULL )
	{
	    lpjs_log("node_recv_specs(): Failed to extract cores from specs.\n");
	    return -1;
	}
	node->cores = strtoul(field, &end, 10);
	if ( *end != '\0' )
	{
	    lpjs_log("node_recv_specs(): Cores field is not a valid number.\n");
	    return -1;
	}

	if ( (field = strsep(&stringp, "\t")) == NULL )
	{
	    lpjs_log("node_recv_specs(): Failed to extract physmem from specs.\n");
	    return -1;
	}
	node->phys_mem = strtoul(field, &end, 10);
	if ( *end != '\0' )
	{
	    lpjs_log("node_recv_specs(): Physmem field is not a valid number.\n");
	    return -1;
	}

	if ( (field = strsep(&stringp, "\t")) == NULL )
	{
	    lpjs_log("node_recv_specs(): Failed to extract ZFS Boolean from specs.\n");
	    return -1;
	}
	node->zfs = strtoul(field, &end, 10);
	if ( *end != '\0' )
	{
	    lpjs_log("node_recv_specs(): ZFS field is not a valid number (should be 0 or 1).\n");
	    return -1;
	}

	if ( (field = strsep(&stringp, "\t")) == NULL )
	{
	    lpjs_log("node_recv_specs(): Failed to extract OS from specs.\n");
	    return -1;
	}
	node->os = strdup(field);
    
	if ( (field = strsep(&stringp, "\t\n")) == NULL )
	{
	    lpjs_log("node_recv_specs(): Failed to extract arch from specs.\n");
	    return -1;
	}
	node->arch = strdup(field);
    }
    return 0;
}
