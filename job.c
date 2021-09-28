#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
#include <xtend/dsv.h>
#include "job.h"
#include "network.h"
#include "lpjs.h"

/*
 *  Constructor for job_t
 */

void    job_init(job_t *job)

{
    job->jobname = NULL;
    job->username = NULL;
    job->cores = 0;
    job->mem_per_core = 0;
}


void    job_print_params(job_t *job)

{
    printf(JOB_SPEC_FORMAT, job->jobname, job->username,
	   job->cores, job->mem_per_core);
}


void    job_send_params(int fd, job_t *job)

{
    if ( dprintf(fd, JOB_SPEC_FORMAT, job->jobname, job->username,
		 job->cores, job->mem_per_core) < 0 )
    {
	perror("send_job_params(): write() failed");
	exit(EX_IOERR);
    }
}
