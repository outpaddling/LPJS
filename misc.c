#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sysexits.h>
#include <stdarg.h>
#include <unistd.h>

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


int     lpjs_log(const char *format, ...)

{
    static FILE *stream = NULL;
    int         status;
    va_list     ap;
    
    if ( isatty(fileno(stderr)) )
	stream = stderr;
    else if ( stream == NULL )
    {
	stream = fopen("/var/log/lpjs", "a");
	if ( stream == NULL )
	{
	    perror("lpjs_log(): Cannot append /var/log/lpjs");
	    exit(EX_CANTCREAT);
	}
    }
    
    va_start(ap, format);
    status = vfprintf(stream, format, ap);
    va_end(ap);
    
    return status;
}
