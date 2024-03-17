/* lpjs_compd.c */
int lpjs_compd_checkin(int msg_fd, node_t *node);
int lpjs_checkin_loop(node_list_t *node_list, node_t *node);
int lpjs_run_script(job_t *job, const char *script_start, uid_t uid, gid_t gid);
int chaperone(job_t *job, const char *job_script_name, uid_t uid, gid_t gid);
