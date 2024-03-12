#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>     // close(), ??
#include <sysexits.h>
#include <limits.h>     // PATH_MAX
#include <errno.h>
#include <fcntl.h>      // open()

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
    job->job_count = 0;
    job->cores_per_job = 0;
    job->min_cores_per_node = 0;
    job->mem_per_core = 0;
    job->user_name = NULL;
    job->primary_group_name = NULL;
    job->working_directory = NULL;
    job->script_name = NULL;
}


/***************************************************************************
 *  Description:
 *      Print job parameters in a readable format
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

int     job_print(job_t *job, FILE *stream)

{
    return fprintf(stream, JOB_SPEC_FORMAT, job->job_id, job->job_count,
	    job->cores_per_job,
	    job->min_cores_per_node, job->mem_per_core,
	    job->user_name, job->primary_group_name,
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

int     job_print_to_string(job_t *job, char *str, size_t buff_size)

{
    return snprintf(str, buff_size, JOB_SPEC_FORMAT,
		    job->job_id, job->job_count, job->cores_per_job,
		    job->min_cores_per_node, job->mem_per_core,
		    job->user_name, job->primary_group_name,
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

void    job_send_as_msg(job_t *job, int msg_fd)

{
    char    msg[LPJS_MSG_LEN_MAX + 1];
    
    snprintf(msg, LPJS_MSG_LEN_MAX + 1, JOB_SPEC_FORMAT,
	    job->job_id, job->job_count,
	    job->cores_per_job,
	    job->min_cores_per_node, job->mem_per_core,
	    job->user_name, job->primary_group_name,
	    job->working_directory, job->script_name);
    
    if ( lpjs_send_munge(msg_fd, msg) != EX_OK )
    {
	lpjs_log("%s(): send failed.\n", __FUNCTION__);
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
	    temp_group_name[65],
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
    
    // FIXME: Make all functions here and in libs take actual array size, including '\0'?
    xt_get_primary_group_name(temp_group_name, 64);
    if ( (job->primary_group_name = strdup(temp_group_name)) == NULL )
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
		    fprintf(stderr, "Error: #lpjs cores-per-job '%s' is not a decimal integer.\n", val);
		    exit(EX_DATAERR);
		}
	    }
	    else if ( strcmp(var, "min-cores-per-node") == 0 )
	    {
		if ( strcmp(val, "all") == 0 )
		    job->min_cores_per_node = job->cores_per_job;
		else
		{
		    job->min_cores_per_node = strtoul(val, &end, 10);
		    if ( *end != '\0' )
		    {
			fprintf(stderr, "Error: #lpjs min-cores-per-node '%s' is not a decimal integer.\n", val);
			exit(EX_DATAERR);
		    }
		    if ( job->min_cores_per_node > job->cores_per_job )
		    {
			fprintf(stderr, "Error: #lpjs min-cores-per-node cannot be greater then cores-per-job.\n");
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
		    fprintf(stderr, "Error: #lpjs mem-per-core '%s':\n", val);
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
    
    // FIXME: Error out if not all required parameters present
    // jobs, cores-per-job, mem-per-core
    
    if ( job->min_cores_per_node == 0 )
	fprintf(stderr, "%u job, %u cores per job, all cores per node, %zu MiB\n",
		job->job_count, job->cores_per_job,
		job->mem_per_core);
    else
	fprintf(stderr, "%u job, %u cores per job, %u cores per node, %zu MiB\n",
		job->job_count, job->cores_per_job,
		job->min_cores_per_node, job->mem_per_core);
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

int     job_read_from_string(job_t *job, const char *string, char **end)

{
    int         items,
		tokens;
    const char  *start;
    char        *temp,
		*p;
    
    items = sscanf(string, JOB_SPEC_NUMS_FORMAT,
	    &job->job_id, &job->job_count, &job->cores_per_job,
	    &job->min_cores_per_node, &job->mem_per_core);
    
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
    ++items;
    
    if ( (job->primary_group_name = strdup(strsep(&p, " \t"))) == NULL )
    {
	lpjs_log("%s(): malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    ++items;
    
    if ( (job->working_directory = strdup(strsep(&p, " \t"))) == NULL )
    {
	lpjs_log("%s(): malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    ++items;
    
    if ( (job->script_name = strdup(strsep(&p, " \t\n"))) == NULL )
    {
	lpjs_log("%s(): malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    ++items;
    
    *end = p;
    free(temp);
    
    return items;
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
 *  2024-03-06  Jason Bacon Begin
 ***************************************************************************/

int     job_read_from_file(job_t *job, const char *path)

{
    int     fd;
    char    buff[JOB_STR_MAX_LEN + 1],
	    *end;
    ssize_t bytes;
    
    // lpjs_log("%s(): Reading job specs from %s...\n", __FUNCTION__, path);

    if ( (fd = open(path, O_RDONLY)) == -1 )
	return -1;
    
    bytes = read(fd, buff, JOB_STR_MAX_LEN);
    close(fd);
    
    if ( bytes < 1 )
    {
	lpjs_log("%s(): Read error: %s\n", __FUNCTION__, strerror(errno));
	return -1;
    }
    
    if ( bytes == JOB_STR_MAX_LEN )
    {
	lpjs_log("%s(): Buffer full reading specs file\n", __FUNCTION__);
	return -1;
    }
    
    buff[bytes] = '\0';
    
    return job_read_from_string(job, buff, &end);
}


/***************************************************************************
 *  Description:
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-31  Jason Bacon Begin
 ***************************************************************************/

void    job_free(job_t **job)

{
    free((*job)->user_name);
    free((*job)->primary_group_name);
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
    char    msg[LPJS_MSG_LEN_MAX + 1];
    
    snprintf(msg, LPJS_MSG_LEN_MAX + 1,
	    JOB_SPEC_HEADER_FORMAT, "JobID", "Job-count",
	    "Cores/job", "Cores/node", "Mem/core",
	    "User-name", "Group-name", "Working-directory", "Script-name");
    lpjs_send_munge(msg_fd, msg);
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
		"User-name", "Group-name", "Working-directory", "Script-name");
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
 *  2024-03-11  Jason Bacon Begin
 ***************************************************************************/

void    job_setenv(job_t *job)

{
    char    str[LPJS_MAX_INT_DIGITS + 1];
    
    setenv("LPJS_JOB_ID", xt_ltostrn(str, job->job_id, 10,
	    LPJS_MAX_INT_DIGITS + 1), 1);
    setenv("LPJS_JOB_COUNT", xt_ltostrn(str, job->job_count, 10,
	    LPJS_MAX_INT_DIGITS + 1), 1);
    setenv("LPJS_CORES_PER_JOB", xt_ltostrn(str, job->cores_per_job, 10,
	    LPJS_MAX_INT_DIGITS + 1), 1);
    setenv("LPJS_MIN_CORES_PER_NODE", xt_ltostrn(str, job->min_cores_per_node, 10,
	    LPJS_MAX_INT_DIGITS + 1), 1);
    setenv("LPJS_MEM_PER_CORE", xt_ltostrn(str, job->mem_per_core, 10,
	    LPJS_MAX_INT_DIGITS + 1), 1);
    setenv("LPJS_USER_NAME", job->user_name, 1);
    setenv("LPJS_PRIMARY_GROUP_NAME", job->primary_group_name, 1);
    setenv("LPJS_WORKING_DIRECTORY", job->working_directory, 1);
    setenv("LPJS_SCRIPT_NAME", job->script_name, 1);
}
