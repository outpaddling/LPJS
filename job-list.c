#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <string.h>

#include <xtend/dsv.h>      // xt_dsv_read_field()
#include <xtend/string.h>   // xt_strtrim()
#include <xtend/file.h>

#include "job-list.h"
#include "lpjs.h"

/***************************************************************************
 *  Description:
 *      Constructor for job_list_t
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

void    job_list_init(job_list_t *job_list)

{
    job_list->count = 0;
}


/***************************************************************************
 *  Description:
 *      Add a job to the queue
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

int     job_list_add_job(job_list_t *job_list, job_t *job)

{
    return 0;   // NL_OK?
}


/***************************************************************************
 *  Description:
 *      Send current jobs to msg_fd in human-readable format
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

void    job_list_send_params(int msg_fd, job_list_t *job_list)

{
    unsigned    c;

    job_send_spec_header(msg_fd);
    for (c = 0; c < job_list->count; ++c)
	job_send_params(job_list->jobs[c], msg_fd);
}
