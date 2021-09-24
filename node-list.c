#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <string.h>
#include <xtend/dsv.h>
#include "node-list.h"
#include "spjs.h"

/*
 *  Constructor for node_list_t
 */

void    node_list_init(node_list_t *node_list)

{
    node_list->count = 0;
}


int     node_list_populate(node_list_t *node_list, const char *conf_file)

{
    FILE    *fp;
    char    field[SPJS_FIELD_MAX+1];
    int     delim;
    size_t  len;
    
    if ( (fp = fopen(conf_file, "r")) == NULL )
    {
	fprintf(stderr, "Cannot open %s.\n", conf_file);
	exit(EX_NOINPUT);
    }
    node_list->count = 0;
    while ( ((delim = dsv_read_field(fp, field, SPJS_FIELD_MAX+1,
				     " \t", &len)) != EOF) &&
	    (strcmp(field, "nodes") != 0) )
    {
	puts("Skipping...");
	dsv_skip_rest_of_line(fp);
    }
    if ( delim != EOF )
    {
	puts("Found nodes.");
	while ( ((delim = dsv_read_field(fp, field, SPJS_FIELD_MAX+1,
					 ",", &len)) != '\n') &&
		(delim != EOF) )
	{
	    node_set_hostname(&node_list->nodes[node_list->count], strdup(field));
	    node_get_specs(&node_list->nodes[node_list->count]);
	    node_print_specs(&node_list->nodes[node_list->count]);
	    ++node_list->count;
	}
	if ( delim == EOF )
	{
	    fprintf(stderr, "Unexpected EOF reading %s.\n", conf_file);
	    exit(EX_DATAERR);
	}
	node_set_hostname(&node_list->nodes[node_list->count], strdup(field));
	node_get_specs(&node_list->nodes[node_list->count]);
	node_print_specs(&node_list->nodes[node_list->count]);
	++node_list->count;
    }
    printf("%u nodes found.\n", node_list->count);
    fclose(fp);
    return 0;   // NL_OK?
}
