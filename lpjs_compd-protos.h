/* lpjs_compd.c */
int lpjs_compd_checkin(int msg_fd, node_t *node);
int lpjs_compd_checkin_loop(node_list_t *node_list, node_t *node);
int lpjs_run_script(job_t *job, const char *script_start);
int run_chaperone(job_t *job, const char *job_script_name);
void lpjs_chown(job_t *job, const char *path);
