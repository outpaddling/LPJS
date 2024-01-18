/* network.c */
int connect_to_dispatchd(node_list_t *node_list);
int print_response(int msg_fd, const char *caller_name);
int send_msg(int msg_fd, const char *format, ...);
ssize_t send_eot(int msg_fd);
