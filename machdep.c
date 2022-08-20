#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>

#if defined(__sun__)
int     dprintf(int fd, const char * restrict format, ...)

{
    char    *buff;
    int     count;
    va_list ap;
    
    va_start(ap, format);
    count = vasprintf(&buff, format, ap);
    write(fd, buff, strlen(buff));
    free(buff);
    va_end(ap);
    return count;
}
#endif
