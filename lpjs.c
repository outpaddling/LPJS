/***************************************************************************
 *  Description:
 *      Wrapper to turn lpjs commands into subcommands.  This will help
 *      avoid future conflicts with other programs without sacrificing
 *      desriptive command names.
 *
 *      This wrapper can be installed under the standard
 *      PATH and used to to execute lpjs commands installed under a
 *      private prefix, without altering PATH, activating a special
 *      environment, opening a container, etc.  This sub-command paradigm
 *      is already familiar to bioinformaticians thanks to other suites
 *      like samtools, bedtools, etc.
 *
 *      The standard location for executables not meant to be in PATH
 *      is $PREFIX/libexec.
 *
 *      Example:
 *
 *          lpjs fastx-derep args
 *
 *      instead of one of the following:
 *
 *          prefix/bin/fastx-derep args
 *
 *          env PATH=prefix/bin:$PATH fastx-derep args
 *
 *  Arguments:
 *      The subcommand and its specific arguments, as if it were run
 *      directly.
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-13  Jason Bacon Begin
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sysexits.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "lpjs.h"

int     main(int argc,char *argv[])

{
    char            cmd[PATH_MAX + 1];
    DIR             *dp;
    struct dirent   *dir_entry;
    struct stat     inode;
    extern FILE     *Log_stream;
    
    if ( (argc == 2) && (strcmp(argv[1],"--version") == 0) )
    {
	printf("lpjs %s\n", VERSION);
	return EX_OK;
    }
    else if ( argc < 2 )
    {
	// LIBEXECDIR must be set by Makefile
	fprintf(stderr, "Usage: %s subcommand [args]\n", argv[0]);
	fprintf(stderr, "\nSubcommands:\n\n");
	if ( (dp = opendir(LIBEXECDIR)) != NULL )
	{
	    while ( (dir_entry = readdir(dp)) != NULL )
	    {
		if ( *dir_entry->d_name != '.' )
		    fprintf(stderr, "%s\n", dir_entry->d_name);
	    }
	    closedir(dp);
	}
	fprintf(stderr, "\nRun \"lpjs subcommand\" or \"man lpjs-subcommand\" for more details.\n\n");
	return EX_USAGE;
    }

    // Shared functions may use lpjs_log
    Log_stream = stderr;
    
    snprintf(cmd, PATH_MAX, "%s/%s", LIBEXECDIR, argv[1]);
    if ( stat(cmd, &inode) == 0 )
	execv(cmd, argv + 1);
    else
    {
	fprintf(stderr, "%s: No %s found in %s.\n", argv[0], argv[1], LIBEXECDIR);
	return EX_USAGE;
    }
}
