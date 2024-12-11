#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sysexits.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>     // PATH_MAX
#include <fcntl.h>      // open()

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
bool        Debug = true;   // FIXME: Control with --debug flag
char        Pid_path[PATH_MAX + 1];

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

int     lpjs_log(const char *format, ...)

{
    int         status;
    va_list     ap;
    
    // Code duplicated in lpjs_debug(), but not worth factoring out
    va_start(ap, format);
    
    fprintf(Log_stream, "%s ", xt_str_localtime("%m-%d %H:%M:%S"));
    
    // FIXME: Add time stamp?
    status = vfprintf(Log_stream, format, ap);
    
    // Commit immediately so last message in the log is not
    // misleading in the event of a crash
    fflush(Log_stream);
    fsync(fileno(Log_stream));
    
    va_end(ap);
    
    return status;
}


int     lpjs_debug(const char *format, ...)

{
    int         status;
    va_list     ap;
    
    if ( Debug )
    {
	// Code duplicated in lpjs_log), but not worth factoring out
	va_start(ap, format);

	fprintf(Log_stream, "%s ", xt_str_localtime("%m-%d %H:%M:%S"));
	
	// FIXME: Add time stamp?
	status = vfprintf(Log_stream, format, ap);
	
	// Commit immediately so last message in the log is not
	// misleading in the event of a crash
	fflush(Log_stream);
	fsync(fileno(Log_stream));
	
	va_end(ap);
	return status;
    }
    else
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
 *  2024-01-27  Jason Bacon Begin
 ***************************************************************************/

FILE    *lpjs_log_output(const char *pathname)

{
    FILE    *fp;
    char    *log_dir,
	    *end;
    
    // FIXME: Prevent unchecked log growth
    // FIXME: Should we even be doing this here, or rely on the package
    // manager and compd to create directory structure?
    // FIXME: Error out if strdup() fails
    log_dir = strdup(pathname);
    if ( (end = strrchr(log_dir, '/')) != NULL )
    {
	*end = '\0';
	fprintf(stderr, "Creating %s...\n", log_dir);
	if ( xt_rmkdir(log_dir, 0755) != 0 )
	{
	    fprintf(stderr, "%s(): Error: Cannot create %s: %s\n", __FUNCTION__,
		    log_dir, strerror(errno));
	    return NULL;
	}
    }
    free(log_dir);

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
 *  2024-02-21  Jason Bacon Begin
 ***************************************************************************/

#define TIME_STR_MAX    32

char    *xt_str_localtime(const char *format)

{
    time_t      time_sec;
    struct tm   *tm;
    static char str[TIME_STR_MAX + 1];
    
    time(&time_sec);
    tm = localtime(&time_sec);
    strftime(str, TIME_STR_MAX + 1, format, tm);
    
    return str;
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
 *  2024-02-22  Jason Bacon Begin
 ***************************************************************************/

const char    *xt_basename(const char *restrict str)

{
    char    *p;
    
    if ( (p = strrchr(str, '/')) == NULL )
	return str;     // No '/' in path, path is a basename
    else
	return p + 1;   // Next char after last '/'
}


ssize_t lpjs_load_script(const char *script_path,
			 char *script_buff, size_t buff_size)

{
    ssize_t bytes;
    int     fd;
    extern FILE *Log_stream;
    
    if ( (fd = open(script_path, O_RDONLY)) == -1 )
    {
	lpjs_log("%s(): Error: Failed to open %s: %s\n", __FUNCTION__,
		script_path, strerror(errno));
	return -1;
    }
    
    bytes = read(fd, script_buff, buff_size + 1);
    if ( bytes == buff_size + 1 )
    {
	lpjs_log("%s(): Error: Script exceeds %zd.  Reduce script size or increase script_size_max.\n",
		__FUNCTION__, buff_size);
	close(fd);
	return -1;
    }
    close(fd);
    script_buff[bytes] = '\0';
    
    return bytes;
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
 *  2024-04-23  Jason Bacon Begin
 ***************************************************************************/

char *lpjs_get_marker_filename(char shared_fs_marker[], const char *hostname,
			      size_t array_size)

{
    // FIXME: Check for errors
    snprintf(shared_fs_marker, array_size + 1,
	     "lpjs-%s-shared-fs-marker", hostname);
    
    return shared_fs_marker;
}


/***************************************************************************
 *  Description:
 *  
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-05-10  Jason Bacon Begin
 ***************************************************************************/

void    lpjs_job_log_dir(const char *log_parent, unsigned long job_id,
			  char *log_dir, size_t array_size)

{
    snprintf(log_dir, array_size, "%s/Job-%lu",
	    log_parent, job_id);
}

