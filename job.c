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
	lpjs_log("%s(): Error: malloc() failed.\n", __FUNCTION__);
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
    // FIXME: Check strdup() failure
    job->job_id = 0;
    job->job_count = 0;
    job->procs_per_job = 0;
    job->min_procs_per_node = 0;
    job->pmem_per_proc = 0;
    job->chaperone_pid = 0;
    job->job_pid = 0;
    job->state = JOB_STATE_PENDING;
    job->user_name = NULL;
    job->primary_group_name = NULL;
    job->submit_node = NULL;
    job->submit_dir = NULL;
    job->script_name = NULL;
    job->compute_node = "TBD";  // For lpjs jobs output
    job->log_dir = NULL;
    // Default: Send contents of temp working dir to working dir on submit host
    if ( (job->push_command = strdup("rsync -av %w/ %h:%d")) == NULL )
    {
	lpjs_log("%s(): Error: strdup() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
}


job_t   *job_dup(job_t *job)

{
    // Terminates process if malloc() fails, no check required
    job_t   *new_job = job_new();
    
    new_job->job_id = job->job_id;
    new_job->job_count = job->job_count;
    new_job->procs_per_job = job->procs_per_job;
    new_job->min_procs_per_node = job->min_procs_per_node;
    new_job->pmem_per_proc = job->pmem_per_proc;
    
    // FIXME: Check malloc success
    if ( job->user_name != NULL )
	new_job->user_name = strdup(job->user_name);
    if ( job->primary_group_name != NULL )
	new_job->primary_group_name = strdup(job->primary_group_name);
    if ( job->submit_node != NULL )
	new_job->submit_node = strdup(job->submit_node);
    if ( job->submit_dir != NULL )
	new_job->submit_dir = strdup(job->submit_dir);
    if ( job->script_name != NULL )
	new_job->script_name = strdup(job->script_name);
    if ( job->compute_node != NULL )
	new_job->compute_node = strdup(job->compute_node);
    if ( job->log_dir != NULL )
	new_job->log_dir = strdup(job->log_dir);
    if ( job->push_command != NULL )
	new_job->push_command = strdup(job->push_command);
    
    return new_job;
}


/***************************************************************************
 *  Description:
 *      Print job parameters in a readable format
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

int     job_print_full_specs(job_t *job, FILE *stream)

{
    return fprintf(stream, JOB_SPEC_FORMAT, job->job_id, job->array_index,
	    job->job_count, job->procs_per_job,
	    job->min_procs_per_node, job->pmem_per_proc,
	    job->chaperone_pid, job->job_pid, job->state,
	    job->user_name, job->primary_group_name,
	    job->submit_node, job->submit_dir,
	    job->script_name, job->compute_node,
	    job->log_dir, job->push_command);
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
		    job->job_id, job->array_index,
		    job->job_count, job->procs_per_job,
		    job->min_procs_per_node, job->pmem_per_proc,
		    job->chaperone_pid, job->job_pid, job->state,
		    job->user_name, job->primary_group_name,
		    job->submit_node, job->submit_dir,
		    job->script_name, job->compute_node,
		    job->log_dir, job->push_command);
}


/***************************************************************************
 *  Description:
 *      Send job parameters to msg_fd, e.g. in response to lpjs-jobs request
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

void    job_send_basic_params(job_t *job, int msg_fd)

{
    char    msg[LPJS_MSG_LEN_MAX + 1];
    
    snprintf(msg, LPJS_MSG_LEN_MAX + 1, JOB_BASIC_PARAMS_FORMAT,
	    job->job_id, job->array_index,
	    job->job_count, job->procs_per_job,
	    job->min_procs_per_node, job->pmem_per_proc,
	    job->user_name,
	    job->script_name,
	    job->compute_node);
    
    // FIXME: Combine into one message
    // Used by dispatchd to send to lpjs jobs command
    if ( lpjs_send_munge(msg_fd, msg, lpjs_dispatchd_safe_close) != LPJS_MSG_SENT )
    {
	lpjs_log("%s(): Error: Send failed.\n", __FUNCTION__);
	exit(EX_IOERR);
    }
}


/***************************************************************************
 *  Description:
 *      Take a blank job object and populate it using system calls to
 *      get username, etc. and other info from #lpjs directives in the
 *      job script.
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
	    temp_log_dir[PATH_MAX + 1],
	    *p,
	    *end,
	    temp_hostname[sysconf(_SC_HOST_NAME_MAX) + 1];
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
    
    gethostname(temp_hostname, sysconf(_SC_HOST_NAME_MAX));
    if ( (job->submit_node = strdup(temp_hostname)) == NULL )
    {
	fprintf(stderr, "%s: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    
    if ( (job->submit_dir = getcwd(NULL, 0)) == NULL )
    {
	fprintf(stderr, "%s: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    
    if ( (job->script_name = strdup(script_name)) == NULL )
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
	    
	    // Process directives where the value main contain whitespace first
	    if ( strcmp(var, "push-command") == 0 )
	    {
		int     c, ch;
		
		job->push_command = malloc(LPJS_CMD_MAX + 1);
		c = 0;
		// FIXME: Use mallocing read function
		while ( (c < LPJS_CMD_MAX) &&
				((ch = getc(fp)) != '\n') && (ch != EOF) )
		    job->push_command[c++] = ch;
		job->push_command[c] = '\0';
	    }
	    else
	    {
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
		else if ( strcmp(var, "procs-per-job") == 0 )
		{
		    job->procs_per_job = strtoul(val, &end, 10);
		    if ( *end != '\0' )
		    {
			fprintf(stderr, "Error: #lpjs procs-per-job '%s' is not a decimal integer.\n", val);
			exit(EX_DATAERR);
		    }
		}
		else if ( strcmp(var, "min-procs-per-node") == 0 )
		{
		    if ( strcmp(val, "procs-per-job") == 0 )
			job->min_procs_per_node = job->procs_per_job;
		    else
		    {
			job->min_procs_per_node = strtoul(val, &end, 10);
			if ( *end != '\0' )
			{
			    fprintf(stderr, "Error: #lpjs min-procs-per-node '%s' is not a decimal integer.\n", val);
			    exit(EX_DATAERR);
			}
			if ( job->min_procs_per_node > job->procs_per_job )
			{
			    fprintf(stderr, "Error: #lpjs min-procs-per-node cannot be greater then procs-per-job.\n");
			    exit(EX_DATAERR);
			}
		    }
		}
		else if ( strcmp(var, "pmem-per-proc") == 0 )
		{
		    job->pmem_per_proc = strtoul(val, &end, 10);
		    
		    // Convert to MiB
		    // Careful with the integer arithmetic, to avoid overflows
		    // and 0 results
		    if ( strcmp(end, "MB") == 0 )
			job->pmem_per_proc = job->pmem_per_proc * LPJS_MB / LPJS_MiB;
		    else if ( strcmp(end, "MiB") == 0 )
			;
		    else if ( strcmp(end, "GB") == 0 )
			job->pmem_per_proc = job->pmem_per_proc * LPJS_GB / LPJS_MiB;
		    else if ( strcmp(end, "GiB") == 0 )
			job->pmem_per_proc = job->pmem_per_proc * LPJS_GiB / LPJS_MiB;
		    else
		    {
			fprintf(stderr, "Error: #lpjs pmem-per-proc '%s':\n", val);
			fprintf(stderr, "Requires a decimal number followed by MB, MiB, GB, or GiB.\n");
			exit(EX_DATAERR);
		    }
		}
		else if ( strcmp(var, "log-dir") == 0 )
		{
		    // FIXME: Handle strdup() failure
		    if ( (job->log_dir = strdup(val)) == NULL )
		    {
			fprintf(stderr, "%s: malloc() failed.\n", __FUNCTION__);
			exit(EX_UNAVAILABLE);
		    }
		}
		else
		{
		    fprintf(stderr, "Unrecognized #lpjs variable: '%s'\n", var);
		    exit(EX_DATAERR);
		}
	    }
	}
	else if ( !xt_strblank(field) )
	    xt_dsv_skip_rest_of_line(fp);
    }
    fclose(fp);

    // Default log dir to LPJS-logs/script-name%.*
    if ( job->log_dir == NULL )
    {
	snprintf(temp_log_dir, PATH_MAX + 1, "LPJS-logs/%s", script_name);
	if ( (p = strrchr(temp_log_dir, '.')) != NULL )
	    *p = '\0';
	
	if ( (job->log_dir = strdup(temp_log_dir)) == NULL )
	{
	    fprintf(stderr, "%s: malloc() failed.\n", __FUNCTION__);
	    exit(EX_UNAVAILABLE);
	}
    }
    
    // FIXME: Error out if not all required parameters present
    // jobs, procs-per-job, pmem-per-proc
    
    if ( job->min_procs_per_node == 0 )
	fprintf(stderr, "%u job, %u procs per job, all procs per node, %zu MiB\n",
		job->job_count, job->procs_per_job,
		job->pmem_per_proc);
    else
	fprintf(stderr, "%u job, %u procs per job, %u procs per node, %zu MiB\n",
		job->job_count, job->procs_per_job,
		job->min_procs_per_node, job->pmem_per_proc);
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
	    &job->job_id, &job->array_index,
	    &job->job_count, &job->procs_per_job,
	    &job->min_procs_per_node, &job->pmem_per_proc,
	    &job->chaperone_pid, &job->job_pid, &job->state);
    
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
	lpjs_log("%s(): Error: Malformed job spec string: %s\n", __FUNCTION__, string);
	exit(EX_DATAERR);
    }
    
    // Duplicate string fields
    while ( isspace(*start) )
	++start;
    if ( (temp = strdup(start)) == NULL )
    {
	lpjs_log("%s(): Error: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    p = temp;
    
    if ( (job->user_name = strdup(strsep(&p, " \t"))) == NULL )
    {
	lpjs_log("%s(): Error: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    ++items;
    
    if ( (job->primary_group_name = strdup(strsep(&p, " \t"))) == NULL )
    {
	lpjs_log("%s(): Error: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    ++items;
    
    if ( (job->submit_node = strdup(strsep(&p, " \t"))) == NULL )
    {
	lpjs_log("%s(): Error: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    ++items;
    
    if ( (job->submit_dir = strdup(strsep(&p, " \t"))) == NULL )
    {
	lpjs_log("%s(): Error: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    ++items;
    
    if ( (job->script_name = strdup(strsep(&p, " \t\n"))) == NULL )
    {
	lpjs_log("%s(): Error: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    ++items;
    
    if ( (job->compute_node = strdup(strsep(&p, " \t\n"))) == NULL )
    {
	lpjs_log("%s(): Error: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    ++items;
    
    if ( (job->log_dir = strdup(strsep(&p, " \t\n"))) == NULL )
    {
	lpjs_log("%s(): Error: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    ++items;
    
    // May contain whitespace, must be last
    if ( (job->push_command = strdup(strsep(&p, "\n"))) == NULL )
    {
	lpjs_log("%s(): Error: malloc() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    ++items;
    
    // Same offset into original string as we are into temp copy
    *end = (char *)start + (p - temp);
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
    
    if ( (fd = open(path, O_RDONLY)) == -1 )
	return -1;
    
    bytes = read(fd, buff, JOB_STR_MAX_LEN);
    close(fd);
    
    if ( bytes < 1 )
    {
	lpjs_log("%s(): Error: Read error: %s\n", __FUNCTION__, strerror(errno));
	return -1;
    }
    
    if ( bytes == JOB_STR_MAX_LEN )
    {
	lpjs_log("%s(): Error: Buffer full reading specs file\n", __FUNCTION__);
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
    // FIXME: _init() sets many to NULL.  _free() should not be
    // called before populating the object, but just in case...
    free((*job)->user_name);
    free((*job)->primary_group_name);
    free((*job)->submit_node);
    free((*job)->submit_dir);
    free((*job)->script_name);
    if ( (*job)->compute_node != NULL )
	free((*job)->compute_node);
    free((*job)->log_dir);
    free((*job)->push_command);
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

void    job_send_basic_params_header(int msg_fd)

{
    // FIXME: Combine into one message
    lpjs_send_munge(msg_fd, JOB_BASIC_PARAMS_HEADER, lpjs_dispatchd_safe_close);
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

void    job_print_basic_params_header(FILE *stream)

{
    fprintf(stream, JOB_BASIC_PARAMS_HEADER);
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
    setenv("LPJS_ARRAY_INDEX", xt_ltostrn(str, job->array_index, 10,
	    LPJS_MAX_INT_DIGITS + 1), 1);
    setenv("LPJS_JOB_COUNT", xt_ltostrn(str, job->job_count, 10,
	    LPJS_MAX_INT_DIGITS + 1), 1);
    setenv("LPJS_PROCS_PER_JOB", xt_ltostrn(str, job->procs_per_job, 10,
	    LPJS_MAX_INT_DIGITS + 1), 1);
    setenv("LPJS_MIN_PROCS_PER_NODE", xt_ltostrn(str, job->min_procs_per_node, 10,
	    LPJS_MAX_INT_DIGITS + 1), 1);
    setenv("LPJS_PMEM_PER_CORE", xt_ltostrn(str, job->pmem_per_proc, 10,
	    LPJS_MAX_INT_DIGITS + 1), 1);
    setenv("LPJS_USER_NAME", job->user_name, 1);
    setenv("LPJS_PRIMARY_GROUP_NAME", job->primary_group_name, 1);
    setenv("LPJS_SUBMIT_HOST", job->submit_node, 1);
    setenv("LPJS_SUBMIT_DIRECTORY", job->submit_dir, 1);
    setenv("LPJS_SCRIPT_NAME", job->script_name, 1);
    setenv("LPJS_COMPUTE_NODE", job->compute_node, 1);
    setenv("LPJS_JOB_LOG_DIR", job->log_dir, 1);
    setenv("LPJS_PUSH_COMMAND", job->push_command, 1);
}


/***************************************************************************
 *  Description:
 *      Compare two jobs by numeric job_id, returning results like strcmp()
 *  
 *  History: 
 *  Date        Name        Modification
 *  2024-05-08  Jason Bacon Begin
 ***************************************************************************/

int     job_id_cmp(job_t **job1, job_t **job2)

{
    return (*job1)->job_id - (*job2)->job_id;
}
