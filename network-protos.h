/* network.c */
int lpjs_connect_to_dispatchd(node_list_t *node_list);
int lpjs_print_response(int msg_fd, const char *caller_name);
ssize_t lpjs_send_msg(int msg_fd, int send_flags, const char *format, ...);
ssize_t lpjs_recv_msg(int msg_fd, char *buff, size_t buff_len, int flags);
ssize_t lpjs_send_eot(int msg_fd);
ssize_t lpjs_send_munge_msg(int msg_fd, char *msg);
int lpjs_server_safe_close(int msg_fd);
