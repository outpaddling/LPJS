#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/utsname.h>
#include <xtend/dsv.h>
#include <xtend/file.h>
#include "node.h"
#include "network.h"
#include "lpjs.h"
#include "misc.h"

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
    node->msg_fd = -1;
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
    char            temp_hostname[sysconf(_SC_HOST_NAME_MAX)+1];
    
    /*
     *  hwloc is extremely complex and we don't need most of its functionality
     *  here, so just gather info the simple way
     */

    node_init(node);
    
    // FIXME: Verify malloc() success
    gethostname(temp_hostname, sysconf(_SC_HOST_NAME_MAX));
    node->hostname = strdup(temp_hostname);
    node->cores = sysconf(_SC_NPROCESSORS_ONLN);
    node->phys_mem = sysconf(_SC_PAGESIZE) * sysconf(_SC_PHYS_PAGES)
		     / 1024 / 1024;
    lpjs_log("CPUs\t%u\n", node->cores);
    lpjs_log("Physmem\t%lu\n", node->phys_mem);
    /*
     *  Report 1 if ZFS filesystem found, so that additional memory
     *  can be reserved on compute nodes.
     *  FIXME: There should be a better approach to this.
     */
    node->zfs = ! system("mount | fgrep -q zfs");
    lpjs_log("ZFS\t%u\n", node->zfs);
    uname(&u_name);
    // FIXME: Verify malloc() success
    node->os = strdup(u_name.sysname);
    node->arch = strdup(u_name.machine);
    lpjs_log("OS\t%s\nArch\t%s\n", node->os, node->arch);
}


/***************************************************************************
 *  Description:
 *      Print current nodes in human-readable form
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-23  Jason Bacon Begin
 ***************************************************************************/

void    node_print_status(node_t *node)

{
    printf(NODE_STATUS_FORMAT, node->hostname, node->state,
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
	perror("send_node_specs(): xt_dprintf() failed");
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

void    node_send_specs(node_t *node, int msg_fd)

{
    /*
     *  Don't use send_msg() here, since there will be more text to send
     *  and send_msg() terminates the message.
     *  FIXME: What additional text??
     */
    lpjs_log("Sending specs...\n");
    lpjs_log("%s\t%s\t%u\t%lu\t%u\t%s\t%s\n",
		 node->hostname, node->state, node->cores,
		 node->phys_mem, node->zfs, node->os, node->arch);
    if ( xt_dprintf(msg_fd, "%s\t%s\t%u\t%lu\t%u\t%s\t%s\n",
		 node->hostname, node->state, node->cores,
		 node->phys_mem, node->zfs, node->os, node->arch) < 0 )
    {
	perror("send_node_specs(): xt_dprintf() failed");
	exit(EX_IOERR);
    }
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

int     node_receive_specs(node_t *node, int msg_fd)

{
    FILE    *fp;
    char    field[LPJS_FIELD_MAX],
	    *end;
    size_t  len;

    // FIXME: NetBSD doesn't have fdclose()
    
    lpjs_log("In node_receive_specs()...\n");
    if ( (fp = fdopen(msg_fd, "r")) == NULL )
	return -1;
    
    xt_dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
    lpjs_log("msg len = %zu\n", len);
    
    // FIXME: This is not such a reliable test
    if ( *field == '\0' )
    {
	lpjs_log("Got empty field.  Closing...\n");
	node->state = "Unknown";
	node->os = "Unknown";
	node->arch = "Unknown";
	fdclose(fp, NULL);
    }
    else
    {
	lpjs_log("Reading fields...\n");
	
	// FIXME: Add sanity checks
	node->hostname = strdup(field);
	lpjs_log("Hostname = %s\n", node->hostname);
	
	xt_dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	// FIXME
	// node->state = strdup(field);
	node->state = "Up";
	lpjs_log("State = %s\n", node->state);
	
	xt_dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	node->cores = strtoul(field, &end, 10);
	lpjs_log("Cores = %u\n", node->cores);
	
	xt_dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	node->phys_mem = strtoul(field, &end, 10);
	lpjs_log("phys_mem = %zu\n", node->phys_mem);
	
	xt_dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	node->zfs = strtoul(field, &end, 10);
	lpjs_log("zfs = %d\n", node->zfs);
    
	xt_dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	node->os = strdup(field);
	lpjs_log("OS = %s\n", node->os);
    
	xt_dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	node->arch = strdup(field);
	lpjs_log("Arch = %s\n", node->arch);
    
	fdclose(fp, NULL);
    }
    return 0;
}
