#include <string.h>

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
