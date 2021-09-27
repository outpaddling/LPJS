#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <string.h>
#include <xtend/dsv.h>      // dsv_read_field()
#include <xtend/string.h>   // strtrim()
#include "node-list.h"
#include "lpjs.h"

/*
 *  Constructor for node_list_t
 */

void    node_list_init(node_list_t *node_list)

{
    node_list->count = 0;
}


int     node_list_add_compute(node_list_t *node_list, FILE *fp,
				const char *conf_file)

{
    int     delim;
    char    field[LPJS_FIELD_MAX+1];
    size_t  len;
    
    while ( ((delim = dsv_read_field(fp, field, LPJS_FIELD_MAX+1,
				     ",", &len)) != '\n') &&
	    (delim != EOF) )
    {
	strtrim(field, " ");
	node_set_hostname(&node_list->compute_nodes[node_list->count], strdup(field));
	node_get_specs(&node_list->compute_nodes[node_list->count]);
	node_print_specs(&node_list->compute_nodes[node_list->count]);
	++node_list->count;
    }
    if ( delim == EOF )
    {
	fprintf(stderr, "Unexpected EOF reading %s.\n", conf_file);
	exit(EX_DATAERR);
    }
    strtrim(field, " ");
    node_set_hostname(&node_list->compute_nodes[node_list->count], strdup(field));
    node_get_specs(&node_list->compute_nodes[node_list->count]);
    node_print_specs(&node_list->compute_nodes[node_list->count]);
    ++node_list->count;
    return 0;   // NL_OK?
}


void    node_list_send_specs(int msg_fd, node_list_t *node_list)

{
    unsigned    c;
    
    for (c = 0; c < node_list->count; ++c)
	node_send_specs(msg_fd, &node_list->compute_nodes[c]);
}
