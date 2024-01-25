#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <limits.h>

#include <xtend/dsv.h>

#include "node-list.h"
#include "config.h"
#include "misc.h"
#include "lpjs.h"

/***************************************************************************
 *  Description:
 *      Load LPJS config file, which contains the names of the head node
 *      and compute nodes.
 *  
 *  History: 
 *  Date        Name        Modification
 *  2021-09-23  Jason Bacon Begin
 ***************************************************************************/

/*
 *  Keep error_stream separate from Log_stream, as this function is
 *  called by both daemons and user commands.  error_stream should be
 *  stderr for non-daemons, and Log_stream for daemons.
 */

int     lpjs_load_config(node_list_t *node_list, int flags, FILE *error_stream)

{
    FILE    *config_fp;
    char    field[LPJS_FIELD_MAX + 1];
    char    config_file[PATH_MAX + 1];
    int     delim;
    size_t  len;
    
    snprintf(config_file, PATH_MAX + 1, "%s/etc/lpjs/config", PREFIX);
    // printf("Loading config file %s...\n", config_file);
    if ( (config_fp = fopen(config_file, "r")) == NULL )
    {
	fprintf(error_stream, "Cannot open %s.\n", config_file);
	exit(EX_NOINPUT);
    }
    node_list_init(node_list);
    while ( ((delim = xt_dsv_read_field(config_fp, field, LPJS_FIELD_MAX + 1,
				     " \t", &len)) != EOF) )
    {
	if ( strcmp(field, "head") == 0 )
	{
	    if ( xt_dsv_read_field(config_fp, field, LPJS_FIELD_MAX + 1, " \t", &len)
		 != '\n' )
	    {
		fprintf(error_stream, "load_config(): 'head' must be followed by a single hostname.\n");
		exit(EX_DATAERR);
	    }

	    // FIXME: Check malloc() and sanity
	    node_list_set_head_node(node_list, strdup(field));
	    
	    // printf("Head node = %s\n", field);
	}
	else if ( strcmp(field, "compute") == 0 )
	{
	    /*
	     *  Only dispatch daemon needs to query compute nodes for specs
	     *  Most other programs just need the head node hostname
	     */
	    // puts("Reading compute nodes...");
	    if ( delim != EOF )
	    {
		if ( flags == LPJS_CONFIG_ALL )
		    node_list_add_compute(node_list, config_fp, config_file);
		else
		    xt_dsv_skip_rest_of_line(config_fp);
	    }
	}
	else
	{
	    fprintf(error_stream, "Skipping unknown tag %s...", field);
	    xt_dsv_skip_rest_of_line(config_fp);
	}
    }
    if ( flags == LPJS_CONFIG_ALL )
	fprintf(error_stream, "%u compute nodes found.\n", node_list->count);
    fclose(config_fp);
    return delim;
}
