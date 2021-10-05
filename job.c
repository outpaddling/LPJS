#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
#include <xtend/dsv.h>
#include "job.h"
#include "network.h"
#include "lpjs.h"

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
    job->jobname = NULL;
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
    printf(JOB_SPEC_FORMAT, job->jobid, job->jobname, job->username,
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

void    job_send_params(int msg_fd, job_t *job)

{
    /*
     *  Don't use send_msg() here, since there will be more text to send
     *  and send_msg() terminates the message.
     */
    if ( dprintf(msg_fd, JOB_SPEC_FORMAT, job->jobid, job->jobname, job->username,
		 job->cores, job->mem_per_core) < 0 )
    {
	perror("send_job_params(): dprintf() failed");
	exit(EX_IOERR);
    }
}
