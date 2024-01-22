/* lpjs_dispatchd.c */
int lpjs_process_events(node_list_t *node_list, job_list_t *job_list);
void lpjs_log_job(const char *incoming_msg);
int lpjs_server_safe_close(int msg_fd);
void lpjs_check_comp_fds(node_list_t *node_list, fd_set *read_fds);
int lpjs_listen(struct sockaddr_in *server_address);
