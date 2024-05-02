/* misc.c */
int lpjs_log(const char *format, ...);
void lpjs_terminate_handler(int s2);
FILE *lpjs_log_output(const char *pathname);
int xt_create_pid_file(const char *pid_path, FILE *log_stream);
char *xt_str_localtime(void);
const char *xt_basename(const char *restrict str);
ssize_t lpjs_load_script(const char *script_path, char *script_buff, size_t buff_size);
char *lpjs_get_marker_filename(char shared_fs_marker[], const char *hostname, size_t array_size);
