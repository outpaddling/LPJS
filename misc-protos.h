/* misc.c */
int lpjs_log(const char *format, ...);
void lpjs_terminate_handler(int s2);
FILE *lpjs_log_output(char *pathname);
char *xt_realpath(const char *relative_path, char *absolute_path, size_t buff_size);
