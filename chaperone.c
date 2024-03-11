/***************************************************************************
 *  Description:
 *      Chaperone for running jobs on a compute node.  When jobs are
 *      dispatched to a compute node, lpjs-chaperone is executed and
 *      the actually command is run as a child of lpjs-chaperone.
 *
 *      lpjs-chaperone monitors its child to enforce resource limits
 *      and report exit status back to the dispatcher.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-27  Jason Bacon Begin
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
#include <errno.h>

#include <xtend/string.h>
#include <xtend/file.h>

#include "node-list.h"
#include "config.h"
#include "network.h"
#include "misc.h"
#include "lpjs.h"

int     main (int argc, char *argv[])
{
    int         msg_fd,
		status;
    node_list_t *node_list = node_list_new();
    char        *script_name,
		log_file[PATH_MAX + 1];
    extern FILE *Log_stream;
    
    if ( argc != 4 )
    {
	fprintf (stderr, "Usage: %s cores mebibytes script\n", argv[0]);
	return EX_USAGE;
    }
    // cores = argv[1]
    // mem = argv[2]
    script_name = argv[3];

    // Open process-specific log file in var/log/lpjs
    // FIXME: Prevent log files from piling up
    snprintf(log_file, PATH_MAX + 1, "%s/chaperone-%d",
	    LPJS_LOG_DIR, getpid());
    
    if ( (Log_stream = lpjs_log_output(log_file)) == NULL )
	return EX_CANTCREAT;
    
    // Get hostname of head node
    lpjs_load_config(node_list, LPJS_CONFIG_HEAD_ONLY, stderr);

    if ( (msg_fd = lpjs_connect_to_dispatchd(node_list)) == -1 )
    {
	lpjs_log("lpjs-chaperone: Failed to connect to dispatch: %s",
		strerror(errno));
	return EX_IOERR;
    }
    
    // FIXME: Use fork() and exec() or posix_spawn(), and monitor
    // the child process for resource use
    // xt_str_argv_cat(cmd, argv, 1, LPJS_CMD_MAX + 1);
    // status = system(cmd);
    
    printf("Running %s with %s cores and %s MiB.\n",
	    argv[3], argv[1], argv[2]);
    status = 0;
    
    /* Send a message to the server */
    /* Need to send \0, so xt_dprintf() doesn't work here */
    if ( lpjs_send(msg_fd, 0, "job-complete\ncmd: %s\nstatus: %d\n",
		  script_name, status) < 0 )
    {
	lpjs_log("lpjs-chaperone: Failed to send message to dispatch: %s",
		strerror(errno));
	close(msg_fd);
	return EX_IOERR;
    }
    close(msg_fd);
    fclose(Log_stream);

    return EX_OK;
}
