/***************************************************************************
 *  Description:
 *      Gather resource availability for a compute node
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-23  Jason Bacon Begin
 ***************************************************************************/

#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

int     main(int argc,char *argv[])

{
    unsigned int    cpus = 0;
    unsigned long   user_mem = 0;

    /*
     *  hwloc is extremely complex and we don't need most of its functionality
     *  here, so just gather info
     */

    cpus = sysconf(_SC_NPROCESSORS_ONLN);
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__APPLE__)
    // Linux _SC_AVPHYS_PAGES is about the same as BSD hw.usermem sysctl
    size_t          user_mem_size = 8;
    sysctlbyname("hw.usermem", &user_mem, &user_mem_size, NULL, 0);
#else
    user_mem = sysconf(_SC_PAGE_SIZE) * sysconf(_SC_AVPHYS_PAGES);
#endif
    printf("sizeof(int) = %zu\n", sizeof(int));
    printf("CPUs\t%u\n", cpus);
    printf("Usermem\t%lu\n", user_mem);
    return EX_OK;
}
