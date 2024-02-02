/* node.c */
node_t *node_new(void);
void node_init(node_t *node);
void node_detect_specs(node_t *node);
void node_print_status_header(FILE *stream);
void node_print_status(FILE *stream, node_t *node);
void node_send_status(node_t *node, int msg_fd);
ssize_t node_send_specs(node_t *node, int msg_fd);
ssize_t node_recv_specs(node_t *node, int msg_fd);
