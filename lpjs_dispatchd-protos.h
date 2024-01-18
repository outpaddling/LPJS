/* lpjs_dispatchd.c */
int server_safe_close(int msg_fd);
int process_events(node_list_t *node_list, job_list_t *job_list);
void log_job(const char *incoming_msg);
