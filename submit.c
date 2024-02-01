/***************************************************************************
 *  Description:
 *      Submit a job to lpjs_dispatchd.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <munge.h>
#include <xtend/string.h>
#include <xtend/file.h>     // xt_rmkdir()

#include "node-list.h"
#include "config.h"
#include "network.h"
#include "misc.h"
#include "lpjs.h"

int     main (int argc, char *argv[])

{
    int     msg_fd;
    char    outgoing_msg[LPJS_MSG_LEN_MAX + 1],
	    *cred,
	    *script_name,
	    *working_directory,
	    *ext,
	    payload[LPJS_PAYLOAD_MAX_LEN + 1];
    node_list_t node_list;
    job_t       *job;
    munge_err_t munge_status;
    // Shared functions may use lpjs_log
    extern FILE *Log_stream;
    
    Log_stream = stderr;
    
    if ( argc != 2 )
    {
	fprintf (stderr, "Usage: %s script.lpjs\n", argv[0]);
	return EX_USAGE;
    }
    script_name =argv[1];
    
    if ( ((ext = strchr(script_name, '.')) == NULL) ||
	 (strcmp(ext, ".lpjs") != 0 ) )
	fprintf(stderr, "Warning: Script name %s should end in \".lpjs\"\n",
		script_name);
    
    // FIXME: Warn about misleading shell extensions, e.g. .sh for bash
    
    // Get hostname of head node
    lpjs_load_config(&node_list, LPJS_CONFIG_HEAD_ONLY, stderr);
    
    job = job_new();    // Exits if malloc fails, no need to check
    job_parse_script(job, script_name);
    script_name = job_get_script_name(job);
    working_directory = job_get_working_directory(job);
    printf("Absolute path = %s/%s\n", working_directory, script_name);

    if ( (msg_fd = lpjs_connect_to_dispatchd(&node_list)) == -1 )
    {
	perror("lpjs-submit: Failed to connect to dispatch");
	return EX_IOERR;
    }

    outgoing_msg[0] = LPJS_REQUEST_SUBMIT;
    outgoing_msg[1] = '\0';
    if ( lpjs_send_msg(msg_fd, 0, outgoing_msg, 0) < 1 )
    {
	perror("lpjs-submit: Failed to send submit request to dispatch");
	close(msg_fd);
	return EX_IOERR;
    }
    
    // FIXME: Send full job specs from job_t class and entire script
    // snprintf(payload, LPJS_PAYLOAD_MAX_LEN, LPJS_JOB_FORMAT
    job_print_params_to_string(job, payload, LPJS_PAYLOAD_MAX_LEN + 1);
    if ( (munge_status = munge_encode(&cred, NULL, payload,
				strlen(payload) + 1)) != EMUNGE_SUCCESS )
    {
	lpjs_log("lpjs-submit: munge_encode() failed.\n");
	lpjs_log("Return code = %s\n", munge_strerror(munge_status));
	return EX_UNAVAILABLE; // FIXME: Check actual error
    }

    if ( lpjs_send_msg(msg_fd, 0, cred) < 0 )
    {
	perror("lpjs-submit: Failed to send credential to dispatch");
	close(msg_fd);
	free(cred);
	return EX_IOERR;
    }
    free(cred);

    lpjs_print_response(msg_fd, "lpjs submit");
    
    return EX_OK;
}
