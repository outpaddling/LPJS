#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
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
    node->mem = 0;
    node->mem_used = 0;
    node->cores = 0;
    node->cores_used = 0;
    node->zfs = 0;
    node->os = "Unknown";
    node->arch = "Unknown";
    node->state = "Down";
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
	node->state = "Down";
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
	node->mem = strtoul(field, &end, 10);
	
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
	   node->mem, node->mem_used, node->os, node->arch);
}


void    node_send_specs(int msg_fd, node_t *node)

{
    /*
     *  Don't use send_msg() here, since there will be more text to send
     *  and send_msg() terminates the message.
     */
    if ( dprintf(msg_fd, NODE_SPEC_FORMAT, node->hostname, node->state,
		 node->cores, node->cores_used,
		 node->mem, node->mem_used, node->os, node->arch) < 0 )
    {
	perror("send_node_specs(): dprintf() failed");
	exit(EX_IOERR);
    }
}
