#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sysexits.h>
#include <stdarg.h>
#include <unistd.h>
#include <xtend/string.h>   // strlcat() on Linux

/***************************************************************************
 *  Description:
 *      Append an argv style list of arguments to a string.  This is
 *      useful for constructing a command to be passed to a shell via
 *      system() or similar methods.
 *
 *      Make this a libxtend function?
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-30  Jason Bacon Begin
 ***************************************************************************/

void    str_argv_cat(char *string, char *argv[], size_t string_buff_size)

{
    size_t  c;
    
    for (c = 1; argv[c] != NULL; ++c)
    {
	strlcat(string, argv[c], string_buff_size);
	strlcat(string, " ", string_buff_size);
    }
}


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
