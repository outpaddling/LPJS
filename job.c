#include <stdio.h>
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

#include "job.h"
#include "network.h"
#include "lpjs.h"
#include "misc.h"       // xt_realpath(), change to libxtend

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
    job->jobid = 0;
    job->script_path = NULL;
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

void    job_print_params(job_t *job)

{
    printf(JOB_SPEC_FORMAT, job->jobid, job->script_path, job->user_name,
	   job->cores_per_job, job->mem_per_core);
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
     *  Don't use send_msg() here, since there will be more text to send
     *  and send_msg() terminates the message.
     */
    if ( xt_dprintf(msg_fd, JOB_SPEC_FORMAT, job->jobid, job->script_path,
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

int     job_parse_script(job_t *job, const char *script_path)

{
    FILE    *fp;
    char    absolute_path[PATH_MAX + 1],
	    field[JOB_FIELD_MAX_LEN + 1],
	    var[JOB_FIELD_MAX_LEN + 1],
	    val[JOB_FIELD_MAX_LEN + 1],
	    temp_user_name[65],
	    *end;
    size_t  field_len;
    int     var_delim, val_delim;
    
    /*
    unsigned long   jobid;          //  Get from dispatchd later
    char            *script_path;
    char            *username;
    char            *working_directory;
    unsigned        cores;
    unsigned long   mem_per_core;
    */
    
    job_init(job);
    
    xt_realpath(script_path, absolute_path, PATH_MAX + 1);
    if ( (fp = fopen(absolute_path, "r")) == NULL )
    {
	fprintf(stderr, "Cannot open %s: %s\n", script_path, strerror(errno));
	return -1;  // FIXME: Define error code
    }
    
    if ( (job->script_path = strdup(script_path)) == NULL )
    {
	fprintf(stderr, "%s: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    
    // FIXME: Make all functions take actual array size, including '\0'?
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
		job->jobs = strtoul(val, &end, 10);
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
		// FIXME: Support 'm' and 'g' suffixes
		job->mem_per_core = strtoul(val, &end, 10);
		if ( *end != '\0' )
		{
		    fprintf(stderr, "Error: #lpjs mem_per_core '%s' is not a decimal integer.\n", val);
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
    
    job_print_params(job);
    
    return 0;
}
