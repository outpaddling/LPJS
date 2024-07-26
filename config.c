#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <limits.h>

#include <xtend/dsv.h>
#include <xtend/string.h>   // xt_strtrim()

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
    lpjs_log("Loading config file %s...\n", config_file);
    if ( (config_fp = fopen(config_file, "r")) == NULL )
    {
	fprintf(error_stream, "Cannot open %s.\n", config_file);
	exit(EX_NOINPUT);
    }
    node_list_init(node_list);
    while ( ((delim = xt_dsv_read_field(config_fp, field, LPJS_FIELD_MAX + 1,
				     " \t", &len)) != EOF) )
    {
	// FIXME: Be more robust about what's a comment
	if ( field[0] == '#' )
	    xt_dsv_skip_rest_of_line(config_fp);    // Comment
	else if ( strcmp(field, "head") == 0 )
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
		    lpjs_load_compute_config(node_list, config_fp, config_file);
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
	fprintf(error_stream, "%u compute nodes found.\n",
		node_list_get_compute_node_count(node_list));
    fclose(config_fp);
    return delim;
}


/***************************************************************************
 *  Use auto-c2man to generate a man page from this comment
 *
 *  Name:
 *      -
 *
 *  Library:
 *      #include <>
 *      -l
 *
 *  Description:
 *  
 *  Arguments:
 *
 *  Returns:
 *
 *  Examples:
 *
 *  Files:
 *
 *  Environment
 *
 *  See also:
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-24  Jason Bacon Begin
 ***************************************************************************/

int     lpjs_load_compute_config(node_list_t *node_list, FILE *input_stream,
			      const char *conf_file)

{
    int     delim;
    char    field[LPJS_FIELD_MAX + 1];
    size_t  len;
    node_t  *node;
    
    while ( ((delim = xt_dsv_read_field(input_stream, field, LPJS_FIELD_MAX + 1,
				     ",", &len)) != '\n') &&
	    (delim != EOF) )
    {
	xt_strtrim(field, " ");
	// Terminates process if malloc() fails, no check required
	node = node_new();
	node_set_hostname(node, strdup(field));
	node_list_add_compute_node(node_list, node);
    }
    if ( delim == EOF )
    {
	lpjs_log("Unexpected EOF reading %s.\n", conf_file);
	exit(EX_DATAERR);
    }
    
    // Add last node read by while condition
    xt_strtrim(field, " ");
    // Terminates process if malloc() fails, no check required
    node = node_new();
    node_set_hostname(node, strdup(field));
    node_list_add_compute_node(node_list, node);
    
    return 0;   // NL_OK?
}

