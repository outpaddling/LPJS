/***************************************************************************
 *  Description:
 *      Submit a job to lpjs_dispatchd.
 *      FIXME: submit should print a warning if the script seems too complex.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

// System headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>          // open()
#include <errno.h>

// Addons
#include <munge.h>
#include <xtend/string.h>
#include <xtend/file.h>     // xt_rmkdir()

// Project headers
#include "node-list.h"
#include "config.h"
#include "network.h"
#include "misc.h"
#include "lpjs.h"

char    *xt_shebang_check(const char *script_text, const char *ext)

{
    char    *shell_prog,
	    *warning = NULL,
	    first_line[80];
    static char warning_buff[80];
    int     c;
    
    for (c = 0; (c < 79) && (script_text[c] != '\n') && (script_text[c] != '\0'); ++c)
	first_line[c] = script_text[c];
    first_line[c] = '\0';
    if ( memcmp(first_line, "#!/", 3) != 0 )
    {
	warning = "Warning: No shebang line found.\n";
    }
    else
    {
	// Guaranteed to have a '/' at this point
	shell_prog = strrchr(first_line, '/') + 1;
	++ext;
	if ( strcmp(ext, shell_prog) != 0 )
	{
	    snprintf(warning_buff, 80, "Warning: Extension \"%s\" != shebang shell \"%s\".\n",
		    ext, shell_prog);
	    warning = warning_buff;
	}
    }
    return warning;
}


int     main (int argc, char *argv[])

{
    int     msg_fd,
	    fd;
    char    outgoing_msg[LPJS_JOB_MSG_MAX + 3],
	    *script_name,
	    *ext,
	    *warning,
	    job_string[JOB_STR_MAX_LEN + 1],
	    hostname[sysconf(_SC_HOST_NAME_MAX) + 1],
	    shared_fs_marker[PATH_MAX + 1],
	    script_text[LPJS_SCRIPT_SIZE_MAX + 1];
    ssize_t script_size;
    // Terminates process if malloc() fails, no check required
    node_list_t *node_list = node_list_new();
    job_t       *job;
    // Shared functions may use lpjs_log
    extern FILE *Log_stream;
    
    Log_stream = stderr;
    
    if ( (getuid() == 0) || (geteuid() == 0) )
    {
	fprintf(stderr, "Cannot run jobs as root.\n");
	return EX_USAGE;
    }
    
    if ( argc != 2 )
    {
	fprintf (stderr, "Usage: %s script.lpjs\n", argv[0]);
	return EX_USAGE;
    }
    script_name = argv[1];
    
    if ( ((ext = strchr(script_name, '.')) == NULL) ||
	 (strcmp(ext, ".lpjs") != 0 ) )
	fprintf(stderr, "Warning: Script name \"%s\" should end in \".lpjs\"\n",
		script_name);
    
    script_size = lpjs_load_script(script_name, script_text,
				   LPJS_SCRIPT_SIZE_MAX + 1);
    
    if ( (warning = xt_shebang_check(script_text, ext)) != NULL )
	fputs(warning, stderr);
    
    if ( script_size > LPJS_PAYLOAD_MAX )
    {
	fprintf(stderr, "Error: Script size cannot exceed LPJS_PAYLOAD_MAX = %u\n",
		LPJS_PAYLOAD_MAX);
	return EX_DATAERR;
    }
    
    if ( script_size < LPJS_SCRIPT_MIN_SIZE )
    {
	lpjs_log("%s(): Error: Script %s < %d chars.\n",
		__FUNCTION__, script_name, LPJS_SCRIPT_MIN_SIZE);
	return EX_DATAERR;
    }
    
    /*
     *  Create marker file in working directory on submit host.
     *  Submit scripts can check for this file to determine if they
     *  are running in a shared directory.  If not, they might have to
     *  use a file transfer tool to send back results.
     */
    
    gethostname(hostname, sysconf(_SC_HOST_NAME_MAX));
    lpjs_get_marker_filename(shared_fs_marker, hostname, PATH_MAX + 1);
    if ( (fd = open(shared_fs_marker, O_WRONLY|O_CREAT, 0644)) != -1 )
	close(fd);
    else
    {
	fprintf(stderr, "Error: Could not create %s: %s\n",
		shared_fs_marker, strerror(errno));
	return EX_CANTCREAT;
    }
    
    // Get hostname of head node
    lpjs_load_config(node_list, LPJS_CONFIG_HEAD_ONLY, stderr);
    
    // Terminates process if malloc() fails, no check required
    job = job_new();
    job_parse_script(job, script_name);
    
    script_name = job_get_script_name(job);

    if ( (msg_fd = lpjs_connect_to_dispatchd(node_list)) == -1 )
    {
	perror("lpjs-submit: Failed to connect to dispatch");
	return EX_IOERR;
    }
    
    job_print_to_string(job, job_string, LPJS_PAYLOAD_MAX + 1);

    snprintf(outgoing_msg, LPJS_JOB_MSG_MAX + 3, "%c%s\n%s",
	    LPJS_DISPATCHD_REQUEST_SUBMIT, job_string, script_text);

    // FIXME: Exiting here causes dispatchd to crash

    if ( lpjs_send_munge(msg_fd, outgoing_msg, close) != LPJS_MSG_SENT )
    {
	perror("lpjs-submit: Failed to send submit request to dispatch");
	close(msg_fd);
	return EX_IOERR;
    }
    
    lpjs_print_response(msg_fd, "lpjs submit");
    
    return EX_OK;
}
