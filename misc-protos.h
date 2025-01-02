/* misc.c */
int lpjs_log(const char *format, ...);
int lpjs_debug(const char *format, ...);
FILE *lpjs_log_output(const char *pathname, const char *mode);
int xt_create_pid_file(const char *pid_path, FILE *log_stream);
char *xt_str_localtime(const char *format);
const char *xt_basename(const char *restrict str);
ssize_t lpjs_load_script(const char *script_path, char *script_buff, size_t buff_size);
char *lpjs_get_marker_filename(char shared_fs_marker[], const char *hostname, size_t array_size);
void lpjs_job_log_dir(const char *log_parent, unsigned long job_id, char *log_dir, size_t array_size);
size_t lpjs_parse_phys_MiB(char *str);
