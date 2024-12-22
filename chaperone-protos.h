/* chaperone.c */
int lpjs_job_start_notice(int msg_fd, const char *hostname, const char *job_id, pid_t job_pid);
int lpjs_job_start_notice_loop(node_list_t *node_list, const char *hostname, const char *job_id, pid_t job_pid);
int lpjs_chaperone_completion(int msg_fd, const char *hostname, const char *job_id, int status);
int lpjs_chaperone_completion_loop(node_list_t *node_list, const char *hostname, const char *job_id, int status);
void chaperone_cancel_handler(int s2);
void whack_family(pid_t pid);
void enforce_resource_limits(pid_t pid, size_t mem_per_proc);
int xt_get_rss(pid_t pid, size_t *rss);

