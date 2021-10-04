#ifndef _LPJS_MISC_H_
#define _LPJS_MISC_H_

void    argv_to_cmd(char *cmd, char *argv[], size_t buff_size);
int     lpjs_log(FILE *stream, const char *format, ...);

#endif

