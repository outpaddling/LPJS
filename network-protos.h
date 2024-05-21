/* network.c */
int lpjs_connect_to_dispatchd(node_list_t *node_list);
int lpjs_print_response(int msg_fd, const char *caller_name);
ssize_t lpjs_send(int msg_fd, int send_flags, const char *format, ...);
ssize_t lpjs_recv(int msg_fd, char *buff, size_t buff_len, int flags, int timeout);
ssize_t lpjs_recv_munge(int msg_fd, char **payload, int flags, int timeout, uid_t *uid, gid_t *gid, int (*close_function)(int));
int lpjs_send_munge(int msg_fd, const char *msg, int (*close_function)(int));
int lpjs_dispatchd_safe_close(int msg_fd);
int lpjs_no_close(int fd);
