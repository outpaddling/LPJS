#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sysexits.h>
#include <stdarg.h>
#include <unistd.h>

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

int     lpjs_log(FILE *stream, const char *format, ...)

{
    int         status;
    va_list     ap;
    
    va_start(ap, format);
    // FIXME: Add time stamp?
    status = vfprintf(stream, format, ap);
    fflush(stream);
    fsync(fileno(stream));
    va_end(ap);
    
    return status;
}
