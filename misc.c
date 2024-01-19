#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sysexits.h>
#include <stdarg.h>
#include <unistd.h>

/*
 *  Avoid globals like the plague, but make an exception here so
 *  signal handlers can log messages.
 */
FILE    *Log_stream;

/***************************************************************************
 *  Description:
 *      Log messages to stream of choice, usually either stderr if running
 *      as a foreground process, /var/log/lpjs_* if daemonized.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-28  Jason Bacon Begin
 ***************************************************************************/

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
    lpjs_log("lpjs_compd received signal, shutting down...\n");

    exit(EX_OK);
}
