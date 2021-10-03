#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/utsname.h>
#include <xtend/dsv.h>
#include "node.h"
#include "network.h"
#include "lpjs.h"

/*
 *  Constructor for node_t
 */

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
    node->socket_fd = -1;
}


int     node_get_specs(node_t *node)

{
    FILE    *fp;
    char    field[LPJS_FIELD_MAX],
	    cmd[LPJS_CMD_MAX],
	    *end;
    size_t  len;

    /*
     *  Using ssh and daemonless compute nodes for early testing.
     *  Will likely use munge at a future date.
     */
    
    snprintf(cmd, LPJS_CMD_MAX, "ssh -o ConnectTimeout=2 %s lpjs-node-specs",
	     node->hostname);
    fp = popen(cmd, "r");
    
    dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
    // FIXME: This is not such a reliable test
    if ( *field == '\0' )
    {
	node->state = "Unknown";
	node->os = "Unknown";
	node->arch = "Unknown";
    }
    else
    {
	node->state = "Up";
	if ( strcmp(field, "CPUs") != 0 )
	{
	    fprintf(stderr, "Expected CPUs, got %s.\n", field);
	    exit(EX_DATAERR);
	}
	dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	node->cores = strtoul(field, &end, 10);
	
	dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	if ( strcmp(field, "Physmem") != 0 )
	{
	    fprintf(stderr, "Expected Phsymem, got %s.\n", field);
	    exit(EX_DATAERR);
	}
	dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	node->phys_mem = strtoul(field, &end, 10);
	
	dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	if ( strcmp(field, "ZFS") != 0 )
	{
	    fprintf(stderr, "Expected ZFS, got %s.\n", field);
	    exit(EX_DATAERR);
	}
	dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	node->zfs = strtoul(field, &end, 10);
    
	dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	if ( strcmp(field, "OS") != 0 )
	{
	    fprintf(stderr, "Expected OS, got %s.\n", field);
	    exit(EX_DATAERR);
	}
	dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	node->os = strdup(field);
    
	dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	if ( strcmp(field, "Arch") != 0 )
	{
	    fprintf(stderr, "Expected Arch, got %s.\n", field);
	    exit(EX_DATAERR);
	}
	dsv_read_field(fp, field, LPJS_FIELD_MAX, "\t", &len);
	node->arch = strdup(field);
    
	fclose(fp);
    }
    return 0;
}


void    node_print_specs(node_t *node)

{
    printf(NODE_SPEC_FORMAT, node->hostname, node->state,
	   node->cores, node->cores_used,
	   node->phys_mem, node->phys_mem_used, node->os, node->arch);
}


void    node_send_specs(node_t *node, int msg_fd)

{
    /*
     *  Don't use send_msg() here, since there will be more text to send
     *  and send_msg() terminates the message.
     */
    if ( dprintf(msg_fd, NODE_SPEC_FORMAT, node->hostname, node->state,
		 node->cores, node->cores_used,
		 node->phys_mem, node->phys_mem_used, node->os, node->arch) < 0 )
    {
	perror("send_node_specs(): dprintf() failed");
	exit(EX_IOERR);
    }
}


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
