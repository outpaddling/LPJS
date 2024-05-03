/* job.c */
job_t *job_new(void);
void job_init(job_t *job);
job_t *job_dup(job_t *job);
int job_print_full_specs(job_t *job, FILE *stream);
int job_print_to_string(job_t *job, char *str, size_t buff_size);
void job_send_basic_params(job_t *job, int msg_fd);
int job_parse_script(job_t *job, const char *script_name);
int job_read_from_string(job_t *job, const char *string, char **end);
int job_read_from_file(job_t *job, const char *path);
void job_free(job_t **job);
void job_send_basic_params_header(int msg_fd);
void job_print_basic_params_header(FILE *stream);
void job_setenv(job_t *job);
