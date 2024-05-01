/* chaperone.c */
int lpjs_chaperone_checkin(int msg_fd, char *job_id, pid_t job_pid);
int lpjs_chaperone_checkin_loop(node_list_t *node_list, char *job_id, pid_t job_pid);
