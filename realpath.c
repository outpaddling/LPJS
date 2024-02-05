#include <stdio.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include <xtend/proc.h>
#include <xtend/string.h>   // strlcpy() on Linux

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
 *  2024-01-30  Jason Bacon Begin
 ***************************************************************************/

#define     PW_BUFF_SIZE    8192    // FIXME: Is this big enough??
#define     USER_NAME_MAX   128

char    *xt_realpath(const char *path,
		    char *absolute_path, size_t buff_size)

{
    struct passwd   pw_ent;
    char            buff[PW_BUFF_SIZE],
		    user_name[USER_NAME_MAX + 1],
		    my_home_dir[PATH_MAX + 1];
    int             c;
    
    printf("%s = ", path);
    
    if ( path[0] == '/' )
	strlcpy(absolute_path, path, buff_size);
    else if ( path[0] == '~' )
    {
	if ( (path[1] == '/') || (path[1] == '\0') )
	{
	    strlcpy(absolute_path,
		    xt_get_home_dir(my_home_dir, PATH_MAX + 1), buff_size);
	    strlcat(absolute_path, path + 1, buff_size);
	}
	else
	{
	    /*
	     *  getpwnam_r() is a mess, but getpwnam() is not thread-safe.
	     *  Neither FreeBSD nor Linux man pages state what buffer is for.
	     *  NetBSD says pw_ent strings are allocated from it.
	     */
	    
	    for (c = 0; (c < USER_NAME_MAX) && (path[c+1] != '/') &&
			(path[c+1] != '\0'); ++c)
		user_name[c] = path[c+1];
	    user_name[c] = 0;
	    
	    #ifdef __sun
	    if ( getpwnam_r(user_name, &pw_ent, buff, PW_BUFF_SIZE) != 0 )
	    {
		fprintf(stderr, "%s(): getpwnam_r(): %s\n", __FUNCTION__,
			strerror(errno));
		return NULL;
	    }
	    #else
	    struct passwd   *ptr;
	    if ( getpwnam_r(user_name, &pw_ent, buff, PW_BUFF_SIZE, &ptr) != 0 )
	    {
		fprintf(stderr, "%s(): getpwnam_r(): %s\n", __FUNCTION__,
			strerror(errno));
		return NULL;
	    }
	    else
	    {
		if ( ptr == NULL )
		{
		    return NULL;
		}
	    }
	    #endif
		    
	    strlcpy(absolute_path, pw_ent.pw_dir, buff_size);
	    strlcat(absolute_path, path + c + 1, buff_size);
	}
    }
    else
    {
	getcwd(absolute_path, buff_size);
	strlcat(absolute_path, "/", buff_size);
	strlcat(absolute_path, path, buff_size);
    }
    
    // FIXME: Compact path by removing ./ and ../
    
    return absolute_path;
}

/*
int     main()

{
    char    absolute_path[1024];
    
    puts(xt_realpath("~bacon/Prog", absolute_path, 1024));
    puts(xt_realpath("~", absolute_path, 1024));
    puts(xt_realpath("~/", absolute_path, 1024));
    puts(xt_realpath("~/Prog", absolute_path, 1024));
    
    return 0;
}
*/
