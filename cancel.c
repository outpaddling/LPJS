/***************************************************************************
 *  Description:
 *      Cancel a pending or running job
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-05-03  Jason Bacon Begin
 ***************************************************************************/

// System headers
#include <stdio.h>
#include <sysexits.h>
#include <ctype.h>
#include <stdlib.h>

// Addons
#include <munge.h>

// Project headers
#include "config.h"
#include "network.h"
#include "misc.h"
#include "lpjs.h"
#include "cancel-protos.h"

int     main (int argc, char *argv[])

{
    int             arg,
		    status,
		    temp_status;
    // Terminates process if malloc() fails, no check required
    node_list_t     *node_list = node_list_new();
    unsigned long   jobid, first_jobid, last_jobid;
    char            *end;
    // Shared functions may use lpjs_log
    extern FILE     *Log_stream;
    
    Log_stream = stderr;
    
    if ( argc < 2 )
	return usage(argv);
    
    // Get hostname of head node
    lpjs_load_config(node_list, LPJS_CONFIG_HEAD_ONLY, stderr);
    
    for (arg=1, status = EX_OK; arg<argc; ++arg)
    {
	if ( !isdigit(argv[arg][0]) )
	    return usage(argv);
	
	first_jobid = strtol(argv[arg], &end, 10);
	if ( *end == '-' )
	{
	    last_jobid = strtol(end + 1, &end, 10);
	    if ( *end != '\0' )
		return usage(argv);
	    
	    for (jobid = first_jobid; jobid <= last_jobid; ++jobid)
		temp_status = lpjs_request_cancel(node_list, jobid);
	}
	else
	    temp_status = lpjs_request_cancel(node_list, first_jobid);
	
	// If any cancel request fails, exit with the return code of the last
	// failure.
	if ( temp_status != EX_OK )
	    status = temp_status;
    }
    
    return status;
}


int     lpjs_request_cancel(node_list_t *node_list, unsigned long jobid)

{
    int     msg_fd;
    char    outgoing_msg[LPJS_MSG_LEN_MAX + 1];
    
    /*
     *  Connect to dispatchd before processing arguments, so that
     *  other events are on hold while we cancel jobs.  We don't want
     *  anything to move from pending to running during a cancel operation.
     */
    
    if ( (msg_fd = lpjs_connect_to_dispatchd(node_list)) == -1 )
    {
	perror("lpjs-cancel: Failed to connect to dispatch");
	return EX_IOERR;
    }
    
    snprintf(outgoing_msg, LPJS_MSG_LEN_MAX + 1, "%c%lu",
	    LPJS_DISPATCHD_REQUEST_CANCEL, jobid);
    lpjs_log("%s(): Canceling job %lu\n", __FUNCTION__, jobid);

    // FIXME: Exiting here causes dispatchd to crash

    if ( lpjs_send_munge(msg_fd, outgoing_msg, close) != LPJS_MSG_SENT )
    {
	perror("lpjs-cancel: Failed to send cancel request to dispatch");
	close(msg_fd);
	return EX_IOERR;
    }
    
    lpjs_print_response(msg_fd, "lpjs cancel");
    close(msg_fd);
    
    return EX_OK;
}


int     usage(char *argv[])

{
    fprintf(stderr, "Usage: %s jobid[-jobid] [jobid[-jobid] ...]\n", argv[0]);
    fprintf(stderr, "Note: No whitespace between jobids in a range.\n");
    return EX_USAGE;
}
