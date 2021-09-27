#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <limits.h>
#include <xtend/dsv.h>
#include "node-list.h"
#include "config.h"
#include "lpjs.h"

int     lpjs_load_config(node_list_t *node_list)

{
    FILE    *fp;
    char    field[LPJS_FIELD_MAX+1];
    char    config_file[PATH_MAX+1];
    int     delim;
    size_t  len;
    
    snprintf(config_file, PATH_MAX+1, "%s/etc/lpjs/config", LOCALBASE);
    // printf("Loading config file %s...\n", config_file);
    if ( (fp = fopen(config_file, "r")) == NULL )
    {
	fprintf(stderr, "Cannot open %s.\n", config_file);
	exit(EX_NOINPUT);
    }
    node_list_init(node_list);
    while ( ((delim = dsv_read_field(fp, field, LPJS_FIELD_MAX+1,
				     " \t", &len)) != EOF) )
    {
	if ( strcmp(field, "head") == 0 )
	{
	    if ( dsv_read_field(fp, field, LPJS_FIELD_MAX + 1, " \t", &len)
		 != '\n' )
	    {
		fprintf(stderr, "load_config(): 'head' must be followed by a single hostname.\n");
		exit(EX_DATAERR);
	    }

	    // FIXME: Check malloc() and sanity
	    node_list_set_head_node(node_list, strdup(field));
	    
	    // printf("Head node = %s\n", field);
	}
	else if ( strcmp(field, "compute") == 0 )
	{
	    // puts("Reading compute nodes...");
	    if ( delim != EOF )
		node_list_add_compute(node_list, fp, config_file);
	}
	else
	{
	    printf("Skipping unknown tag %s...", field);
	    dsv_skip_rest_of_line(fp);
	}
    }
    // printf("%u nodes found.\n", node_list->count);
    fclose(fp);
    return delim;
}
