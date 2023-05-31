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
    printf("CPUs\t%u\n", node->cores);
    printf("Physmem\t%lu\n", node->phys_mem);
    /*
     *  Report 1 if ZFS filesystem found, so that additional memory
     *  can be reserved on compute nodes.
     *  FIXME: There should be a better approach to this.
     */
    node->zfs = ! system("mount | fgrep -q zfs");
    printf("ZFS\t%u\n", node->zfs);
    uname(&u_name);
    // FIXME: Verify malloc() success
    node->os = strdup(u_name.sysname);
    node->arch = strdup(u_name.machine);
    printf("OS\t%s\nArch\t%s\n", node->os, node->arch);
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
     */
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

    if ( (fp = fdopen(msg_fd, "r")) == NULL )
	return -1;
    
    dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
    // FIXME: This is not such a reliable test
    if ( *field == '\0' )
    {
	node->state = "Unknown";
	node->os = "Unknown";
	node->arch = "Unknown";
	fclose(fp);
    }
    else
    {
	node->state = "Up";
	
	// FIXME: Add sanity checks
	node->hostname = strdup(field);
	printf("Hostname = %s\n", node->hostname);
	
	dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	node->state = strdup(field);
	
	dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	node->cores = strtoul(field, &end, 10);
	
	dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	node->phys_mem = strtoul(field, &end, 10);
	
	dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	node->zfs = strtoul(field, &end, 10);
    
	dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	node->os = strdup(field);
    
	dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	node->arch = strdup(field);
    
	fclose(fp);
    }
    fclose(fp);
    return 0;
}
