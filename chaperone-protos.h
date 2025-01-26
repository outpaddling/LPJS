/* chaperone.c */
int lpjs_job_start_notice(int msg_fd, const char *hostname, const char *job_id, pid_t job_pid);
int lpjs_job_start_notice_loop(node_list_t *node_list, const char *hostname, const char *job_id, pid_t job_pid);
int lpjs_chaperone_completion(int msg_fd, const char *hostname, const char *job_id, int status, size_t peak_rss);
int lpjs_chaperone_completion_loop(node_list_t *node_list, const char *hostname, const char *job_id, int status, size_t peak_rss);
void chaperone_cancel_handler(int s2);
void chaperone_lost_connection_handler(int s2);
void whack_family(pid_t pid);
void enforce_resource_limits(pid_t pid, size_t mem_per_proc);
int xt_get_family_rss(pid_t pid, size_t *rss);
int run_pull_command(const char *wd);
int run_push_command(const char *wd, const char *log_dir);
int parse_transfer_cmd(const char *sp, char *cmd, const char *wd);
