/* lpjs_dispatchd.c */
int lpjs_process_events(node_list_t *node_list);
void    lpjs_log_job(job_list_t *job_list, const char *hostname, unsigned long job_id, int exit_status, size_t peak_rss);
void lpjs_check_comp_fds(fd_set *read_fds, node_list_t *node_list, job_list_t *running_jobs);
int lpjs_listen(struct sockaddr_in *server_address);
int lpjs_check_listen_fd(int listen_fd, fd_set *read_fds, node_list_t *node_list, job_list_t *pending_jobs, job_list_t *running_jobs);
void lpjs_process_compute_node_checkin(int msg_fd, char *incoming_msg, node_list_t *node_list, uid_t munge_uid, gid_t munge_gid);
int lpjs_submit(int msg_fd, const char *incoming_msg, node_list_t *node_list, job_list_t *pending_jobs, job_list_t *running_jobs, uid_t munge_uid, gid_t munge_gid);
int lpjs_cancel(int msg_fd, const char *incoming_msg, node_list_t *node_list, job_list_t *pending_jobs, job_list_t *running_jobs, uid_t munge_uid, gid_t munge_gid);
int lpjs_kill_processes(node_list_t *node_list, job_t *job);
int lpjs_queue_job(int msg_fd, job_list_t *pending_jobs, job_t *job, unsigned long job_array_index, const char *script_text);
int lpjs_update_job(node_list_t *node_list, char *payload, job_list_t *pending_jobs, job_list_t *running_jobs);
int lpjs_load_job_list(job_list_t *job_list, node_list_t *node_list, char *spool_dir);
void lpjs_dispatchd_terminate_handler(int s2);
void lpjs_dispatchd_sigpipe(int s2);
int adjust_resources(node_list_t *node_list, job_list_t *job_list, const char *hostname, unsigned long job_id, node_resource_t direction);
