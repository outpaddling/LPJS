#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sysexits.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>     // PATH_MAX
#include <xtend/file.h> // xt_rmkdir()

#include "lpjs.h"
#include "misc.h"
#include "node-list.h"
#include "network.h"

/*
 *  Avoid globals like the plague, but make an exception here so
 *  signal handlers can log messages.  All commands might use
 *  lpjs_log(), so Log_stream must be always be initialized in main().
 */
FILE        *Log_stream;
node_list_t *Node_list;
char        *Pid_path;

/***************************************************************************
 *  Description:
 *      Log messages to stream of choice, usually either stderr if running
 *      as a foreground process, or PREFIX/var/log/lpjs by default
 *      if daemonized.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

// FIXME: Add debug levels argument
int     lpjs_log(const char *format, ...)

{
    int         status;
    va_list     ap;
    
    va_start(ap, format);
    
    // FIXME: Add time stamp?
    status = vfprintf(Log_stream, format, ap);
    
    // Commit immediately so last message in the log is not
    // misleading in the event of a crash
    fflush(Log_stream);
    fsync(fileno(Log_stream));
    
    va_end(ap);
    
    return status;
}


/***************************************************************************
 *  Description:
 *      Gracefully shut down in the event of an interrupt signal
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

void    lpjs_terminate_handler(int s2)

{
    node_t  *node = node_new();
    int     c;
    
    lpjs_log("Received signal, shutting down...\n");
    for (c = 0; c < NODE_LIST_COUNT(Node_list); ++c)
    {
	node = NODE_LIST_COMPUTE_NODES_AE(Node_list, c);
	if ( node_get_msg_fd(node) != -1 )
	{
	    lpjs_log("Closing connection with %s...\n", node_get_hostname(node));
	    lpjs_server_safe_close(node_get_msg_fd(node));
	}
    }
#ifdef __linux__
    remove(Pid_path);
#endif
    exit(EX_OK);
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
 *  2024-01-27  Jason Bacon Begin
 ***************************************************************************/

FILE    *lpjs_log_output(char *pathname)

{
    FILE    *fp;
    
    // FIXME: Prevent unchecked log growth
    if ( xt_rmkdir(LPJS_LOG_DIR, 0755) != 0 )
    {
	perror("Cannot create " LPJS_LOG_DIR);
	return NULL;
    }

    fp = fopen(pathname, "w");
    if ( fp == NULL )
    {
	fprintf(stderr, "Cannot open %s: %s\n", pathname, strerror(errno));
	return NULL;
    }
    return fp;
}


int     xt_create_pid_file(const char *pid_path, FILE *log_stream)

{
    struct stat st;
    FILE        *fp;
    
    if ( stat(pid_path, &st) == 0 )
    {
	fprintf(log_stream, "%s: %s already exists.\n", __FUNCTION__,
		pid_path);
	return EX_CANTCREAT;
    }
    
    if ( (fp = fopen(pid_path, "w")) == NULL )
    {
	fprintf(log_stream, "%s: fopen() failed on %s: %s.\n", __FUNCTION__,
		pid_path, strerror(errno));
	return EX_CANTCREAT;
    }
    
    if ( fprintf(fp, "%d\n", getpid()) < 0 )
    {
	fprintf(log_stream, "%s: fprintf() failed on %s: %s.\n", __FUNCTION__,
		pid_path, strerror(errno));
	return EX_IOERR;
    }
    
    fclose(fp);
    return EX_OK;
}
