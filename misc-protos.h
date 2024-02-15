/* misc.c */
int lpjs_log(const char *format, ...);
void lpjs_terminate_handler(int s2);
FILE *lpjs_log_output(char *pathname);
int xt_create_pid_file(const char *pid_path, FILE *log_stream);
