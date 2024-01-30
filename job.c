#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
#include <limits.h>     // PATH_MAX
#include <errno.h>

#include <xtend/dsv.h>
#include <xtend/file.h>

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
    job->username = NULL;
    job->cores = 0;
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
    printf(JOB_SPEC_FORMAT, job->jobid, job->script_path, job->username,
	   job->cores, job->mem_per_core);
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
    if ( xt_dprintf(msg_fd, JOB_SPEC_FORMAT, job->jobid, job->script_path, job->username,
		 job->cores, job->mem_per_core) < 0 )
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
	    field[JOB_FIELD_MAX_LEN + 1];
    size_t  field_len;
    
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
    
    // FIXME: Check return value and update xt_dsv_read_field() man page
    // regarding EOF
    while ( xt_dsv_read_field(fp, field, JOB_FIELD_MAX_LEN + 1, " \t", &field_len) != EOF )
    {
	puts(field);
	if ( strcmp(field, "#lpjs") == 0 )
	{
	    printf("Parsing lpjs directive.\n");
	    xt_dsv_read_field(fp, field, JOB_FIELD_MAX_LEN + 1, " \t", &field_len);
	    puts(field);
	    xt_dsv_read_field(fp, field, JOB_FIELD_MAX_LEN + 1, " \t", &field_len);
	    puts(field);
	}
	else
	    xt_dsv_skip_rest_of_line(fp);
    }
    
    fclose(fp);
    
    return 0;
}
