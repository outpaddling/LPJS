#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sysexits.h>
#include <xtend/dsv.h>
#include "node.h"
#include "spjs.h"

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
}


int     node_get_specs(node_t *node)

{
    FILE    *fp;
    char    field[SPJS_FIELD_MAX],
	    cmd[SPJS_CMD_MAX],
	    *end;
    size_t  len;

    /*
     *  Using ssh and daemonless compute nodes for early testing.
     *  Will likely use munge at a future date.
     */
    
    snprintf(cmd, SPJS_CMD_MAX, "ssh %s spjs-node-specs", node->hostname);
    fp = popen(cmd, "r");
    dsv_read_field(fp, field, SPJS_FIELD_MAX, "\t", &len);
    if ( strcmp(field, "CPUs") != 0 )
    {
	fprintf(stderr, "Expected CPUs, got %s.\n", field);
	exit(EX_DATAERR);
    }
    dsv_read_field(fp, field, SPJS_FIELD_MAX, "\t", &len);
    node->cores = strtoul(field, &end, 10);
    
    dsv_read_field(fp, field, SPJS_FIELD_MAX, "\t", &len);
    if ( strcmp(field, "Physmem") != 0 )
    {
	fprintf(stderr, "Expected Phsymem, got %s.\n", field);
	exit(EX_DATAERR);
    }
    dsv_read_field(fp, field, SPJS_FIELD_MAX, "\t", &len);
    node->mem = strtoul(field, &end, 10);
    
    dsv_read_field(fp, field, SPJS_FIELD_MAX, "\t", &len);
    if ( strcmp(field, "ZFS") != 0 )
    {
	fprintf(stderr, "Expected ZFS, got %s.\n", field);
	exit(EX_DATAERR);
    }
    dsv_read_field(fp, field, SPJS_FIELD_MAX, "\t", &len);
    node->zfs = strtoul(field, &end, 10);

    fclose(fp);
    return 0;
}


void    node_print_specs(node_t *node)

{
    printf("%s %u %lu %d\n", node->hostname, node->cores, node->mem, node->zfs);
}
