/* node-list.c */
node_list_t *node_list_new(void);
void node_list_init(node_list_t *node_list);
void node_list_update_compute(node_list_t *node_list, node_t *node);
void node_list_send_status(int msg_fd, node_list_t *node_list);
int node_list_add_compute_node(node_list_t *node_list, node_t *node);
node_t *node_list_find_hostname(node_list_t *node_list, const char *hostname);
int node_list_set_state(node_list_t *node_list, char *arg_string, uid_t munge_uid, int msg_fd);
