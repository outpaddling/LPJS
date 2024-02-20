#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
#include <limits.h>     // PATH_MAX
#include <errno.h>

#include <xtend/dsv.h>
#include <xtend/file.h>
#include <xtend/string.h>   // xt_strblank()
#include <xtend/proc.h>     // xt_get_user_name()

#include "job-private.h"
#include "network.h"
#include "lpjs.h"
#include "misc.h"
#include "realpath-protos.h"

/***************************************************************************
 *  Description:
 *  
 *  Returns:
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-31  Jason Bacon Begin
 ***************************************************************************/

job_t   *job_new(void)

{
    job_t   *job;
    
    if ( (job = malloc(sizeof(job_t))) == NULL )
    {
	lpjs_log("%s: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    job_init(job);
    
    return job;
}


/***************************************************************************
 *  Description:
 *      Constructor for job_t
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

void    job_init(job_t *job)

{
    job->job_id = 0;
    job->script_name = NULL;
    job->working_directory = NULL;
    job->user_name = NULL;
    job->cores_per_job = 0;
    job->cores_per_node = 0;
    job->mem_per_core = 0;
}


/***************************************************************************
 *  Description:
 *      Print job parameters in a readable format
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

void    job_print_params(FILE *stream, job_t *job)

{
    fprintf(stream, JOB_SPEC_FORMAT, job->job_id, job->job_count, job->cores_per_job,
	    job->cores_per_node, job->mem_per_core, job->user_name,
	    job->working_directory, job->script_name);
}


/***************************************************************************
 *  Description:
 *      Print job parameters in a readable format
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

int     job_print_params_to_string(job_t *job, char *str, size_t buff_size)

{
    return snprintf(str, buff_size, JOB_SPEC_FORMAT,
		    job->job_id, job->job_count, job->cores_per_job,
		    job->cores_per_node, job->mem_per_core, job->user_name,
		    job->working_directory, job->script_name);
}


/***************************************************************************
 *  Description:
 *      Send job parameters to msg_fd, e.g. in response to lpjs-jobs request
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

void    job_send_params(job_t *job, int msg_fd)

{
    /*
     *  Don't use send() here, since there will be more text to send
     *  and send() terminates the message.
     */
    if ( xt_dprintf(msg_fd, JOB_SPEC_FORMAT, job->job_id, job->script_name,
	    job->user_name, job->cores_per_job, job->mem_per_core) < 0 )
    {
	perror("send_job_params(): xt_dprintf() failed");
	exit(EX_IOERR);
    }
}


/***************************************************************************
 *  Description:
 *  
 *  Arguments:
 *
 *  Returns:
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-30  Jason Bacon Begin
 ***************************************************************************/

int     job_parse_script(job_t *job, const char *script_name)

{
    FILE    *fp;
    char    script_path[PATH_MAX + 1],
	    field[JOB_FIELD_MAX_LEN + 1],
	    var[JOB_FIELD_MAX_LEN + 1],
	    val[JOB_FIELD_MAX_LEN + 1],
	    temp_user_name[65],
	    *end;
    size_t  field_len;
    int     var_delim, val_delim;
    
    // Note: Get job_id from dispatchd later
    
    job_init(job);
    
    xt_realpath(script_name, script_path, PATH_MAX + 1);
    if ( (fp = fopen(script_path, "r")) == NULL )
    {
	fprintf(stderr, "Cannot open %s: %s\n", script_path, strerror(errno));
	return -1;  // FIXME: Define error code
    }
    
    if ( (job->working_directory = getcwd(NULL, 0)) == NULL )
    {
	fprintf(stderr, "%s: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    
    if ( (job->script_name = strdup(script_name)) == NULL )
    {
	fprintf(stderr, "%s: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    
    // FIXME: Make all functions here and in libs take actual array size, including '\0'?
    xt_get_user_name(temp_user_name, 64);
    if ( (job->user_name = strdup(temp_user_name)) == NULL )
    {
	fprintf(stderr, "%s: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    
    // FIXME: Check return value and update xt_dsv_read_field() man page
    // regarding EOF
    while ( xt_dsv_read_field(fp, field, JOB_FIELD_MAX_LEN + 1, " \t", &field_len) != EOF )
    {
	// printf("First field = '%s'\n", field);
	if ( strcmp(field, "#lpjs") == 0 )
	{
	    var_delim = xt_dsv_read_field(fp, var, JOB_FIELD_MAX_LEN + 1, " \t", &field_len);
	    val_delim = xt_dsv_read_field(fp, val, JOB_FIELD_MAX_LEN + 1, " \t", &field_len);
	    if ( (strchr(" \t", var_delim) == NULL)
		 || (strchr(" \t\n", val_delim) == NULL) )
	    {
		fprintf(stderr,
		    "Error parsing #lpjs directive: var = '%s', val = '%s'\n",
		    var, val);
		exit(EX_DATAERR);
	    }
	    if ( strcmp(var, "jobs") == 0 )
	    {
		job->job_count = strtoul(val, &end, 10);
		if ( *end != '\0' )
		{
		    fprintf(stderr, "Error: #lpjs jobs '%s' is not a decimal integer.\n", val);
		    exit(EX_DATAERR);
		}
	    }
	    else if ( strcmp(var, "cores-per-job") == 0 )
	    {
		job->cores_per_job = strtoul(val, &end, 10);
		if ( *end != '\0' )
		{
		    fprintf(stderr, "Error: #lpjs cores_per_job '%s' is not a decimal integer.\n", val);
		    exit(EX_DATAERR);
		}
	    }
	    else if ( strcmp(var, "cores-per-node") == 0 )
	    {
		if ( strcmp(val, "all") == 0 )
		    job->cores_per_node = 0;
		else
		{
		    job->cores_per_node = strtoul(val, &end, 10);
		    if ( *end != '\0' )
		    {
			fprintf(stderr, "Error: #lpjs cores_per_node '%s' is not a decimal integer.\n", val);
			exit(EX_DATAERR);
		    }
		}
	    }
	    else if ( strcmp(var, "mem-per-core") == 0 )
	    {
		job->mem_per_core = strtoul(val, &end, 10);
		
		// Convert to MiB
		// Careful with the integer arithmetic, to avoid overflows
		// and 0 results
		if ( strcmp(end, "MB") == 0 )
		    job->mem_per_core = job->mem_per_core * LPJS_MB / LPJS_MiB;
		else if ( strcmp(end, "MiB") == 0 )
		    ;
		else if ( strcmp(end, "GB") == 0 )
		    job->mem_per_core = job->mem_per_core * LPJS_GB / LPJS_MiB;
		else if ( strcmp(end, "GiB") == 0 )
		    job->mem_per_core = job->mem_per_core * LPJS_GiB / LPJS_MiB;
		else
		{
		    fprintf(stderr, "Error: #lpjs mem_per_core '%s':\n", val);
		    fprintf(stderr, "Requires a decimal number followed by MB, MiB, GB, or GiB.\n");
		    exit(EX_DATAERR);
		}
	    }
	    else
	    {
		fprintf(stderr, "Unrecognized #lpjs variable: '%s'\n", var);
		exit(EX_DATAERR);
	    }
	}
	else if ( !xt_strblank(field) )
	    xt_dsv_skip_rest_of_line(fp);
    }
    fclose(fp);
    
    if ( job->cores_per_node == 0 )
	fprintf(stderr, "%u job, %u cores per job, all cores per node, %zu MiB\n",
		job->job_count, job->cores_per_job,
		job->mem_per_core);
    else
	fprintf(stderr, "%u job, %u cores per job, %u cores per node, %zu MiB\n",
		job->job_count, job->cores_per_job,
		job->cores_per_node, job->mem_per_core);
    return 0;
}


/***************************************************************************
 *  Use auto-c2man to generate a man page from this comment
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
 *  2024-01-31  Jason Bacon Begin
 ***************************************************************************/

int     job_read_from_string(job_t *job, const char *string)

{
    int         items,
		tokens;
    const char  *start;
    char        *temp,
		*p;
    
    items = sscanf(string, JOB_SPEC_NUMS_FORMAT,
	    &job->job_id, &job->job_count, &job->cores_per_job,
	    &job->cores_per_node, &job->mem_per_core);
    
    // Skips past numeric fields
    for (start = string, tokens = 0;
	    (*start != '\0') && (tokens < JOB_SPEC_NUMERIC_FIELDS); ++start)
    {
	while ( (*start != '\0') && isspace(*start) )
	    ++start;
	while ( (*start != '\0') && ! isspace(*start) )
	    ++start;
	++tokens;
    }
    if ( *start == '\0' )
    {
	lpjs_log("%s: Malformed job spec string: %s\n", __FUNCTION__, string);
	exit(EX_DATAERR);
    }
    
    // Duplicate string fields
    while ( isspace(*start) )
	++start;
    if ( (temp = strdup(start)) == NULL )
    {
	lpjs_log("%s(): malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    p = temp;
    
    if ( (job->user_name = strdup(strsep(&p, " \t"))) == NULL )
    {
	lpjs_log("%s(): malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    if ( (job->working_directory = strdup(strsep(&p, " \t"))) == NULL )
    {
	lpjs_log("%s(): malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    if ( (job->script_name = strdup(strsep(&p, " \t\n"))) == NULL )
    {
	lpjs_log("%s(): malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    
    free(temp);
    
    return items;
}


/***************************************************************************
 *  Use auto-c2man to generate a man page from this comment
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
 *  2024-01-31  Jason Bacon Begin
 ***************************************************************************/

void    job_free(job_t **job)

{
    free((*job)->user_name);
    free((*job)->working_directory);
    free((*job)->script_name);
    free(*job);
}


/***************************************************************************
 *  Description:
 *  
 *  Returns:
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  Jason Bacon Begin
 ***************************************************************************/

void    job_send_spec_header(int msg_fd)

{
    xt_dprintf(msg_fd, JOB_SPEC_HEADER_FORMAT, "JobID", "Job-count",
		"Cores/job", "Cores/node", "Mem/core",
		"User-name", "Working-directory", "Script-name");
}


/***************************************************************************
 *  Description:
 *  
 *  Returns:
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  Jason Bacon Begin
 ***************************************************************************/

void    job_print_spec_header(FILE *stream)

{
    fprintf(stream, JOB_SPEC_HEADER_FORMAT, "JobID", "Job-count",
		"Cores/job", "Cores/node", "Mem/core",
		"User-name", "Working-directory", "Script-name");
}
