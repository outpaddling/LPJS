/* lpjs_compd.c */
int lpjs_compd_checkin(int msg_fd, node_t *node);
int lpjs_compd_checkin_loop(node_list_t *node_list, node_t *node);
int lpjs_working_dir_setup(job_t *job, const char *script_start, char *job_script_name, size_t maxlen);
int lpjs_send_chaperone_status(int msg_fd, unsigned long job_id, chaperone_status_t status);
int lpjs_send_chaperone_status_loop(node_list_t *node_list, unsigned long job_id, chaperone_status_t status);
int lpjs_run_chaperone(job_t *job, const char *script_start, int msg_fd, node_list_t *node_list);
void lpjs_chown(job_t *job, const char *path);
void sigchld_handler(int s2);
