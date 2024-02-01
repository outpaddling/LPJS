/* job.c */
job_t *job_new(void);
void job_init(job_t *job);
void job_print_params(FILE *stream, job_t *job);
int job_print_params_to_string(job_t *job, char *str, size_t buff_size);
void job_send_params(job_t *job, int msg_fd);
int job_parse_script(job_t *job, const char *script_name);
int job_read_from_string(job_t *job, const char *string);
void job_free(job_t **job);
void job_send_spec_header(int msg_fd);
void job_print_spec_header(FILE *stream);
