/* node.c */
node_t *node_new(void);
void node_init(node_t *node);
void node_detect_specs(node_t *node);
void node_print_status_header(FILE *stream);
void node_print_status(node_t *node, FILE *stream);
void node_status_to_str(node_t *node, char *str, size_t array_size);
void node_send_status(node_t *node, int msg_fd);
int node_print_specs_header(FILE *stream);
char *node_specs_to_str(node_t *node, char *str, size_t buff_len);
ssize_t node_str_to_specs(node_t *node, const char *str);
int node_adjust_resources(node_t *node, job_t *job, node_resource_t direction);
