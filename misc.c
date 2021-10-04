#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sysexits.h>
#include <stdarg.h>
#include <unistd.h>
#include <xtend/string.h>   // strlcat() on Linux

// Make this a libxtend function?
void    argv_to_cmd(char *cmd, char *argv[], size_t buff_size)

{
    size_t  c;
    
    for (c = 1; argv[c] != NULL; ++c)
    {
	strlcat(cmd, argv[c], buff_size);
	strlcat(cmd, " ", buff_size);
    }
}


int     lpjs_log(FILE *stream, const char *format, ...)

{
    int         status;
    va_list     ap;
    
    va_start(ap, format);
    status = vfprintf(stream, format, ap);
    fflush(stream);
    fsync(fileno(stream));
    va_end(ap);
    
    return status;
}
