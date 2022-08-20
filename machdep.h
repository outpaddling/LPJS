#ifndef _MACHDEP_H_
#define _MACHDEP_H_

#if defined(__sun__)
int dprintf(int fd, const char * restrict format, ...);
#endif

#endif
