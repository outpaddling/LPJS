/* lpjs_dispatchd.c */
int process_events(node_list_t *node_list, job_list_t *job_list);
void lpjs_log_job(const char *incoming_msg);
int lpjs_server_safe_close(int msg_fd);
