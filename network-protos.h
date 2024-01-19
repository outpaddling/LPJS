/* network.c */
int lpjs_connect_to_dispatchd(node_list_t *node_list);
int lpjs_print_response(int msg_fd, const char *caller_name);
int lpjs_send_msg(int msg_fd, const char *format, ...);
ssize_t lpjs_send_eot(int msg_fd);
