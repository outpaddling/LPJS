    
/*
 *  Generated by /usr/local/bin/auto-gen-get-set-opaque
 *
 *  Accessor macros.  Use these to access structure members from functions
 *  outside the node_t class.
 *
 *  These generated macros are not expected to be perfect.  Check and edit
 *  as needed before adding to your code.
 */

/* temp-node-accessors.c */
char *node_get_hostname(node_t *node_ptr);
char node_get_hostname_ae(node_t *node_ptr, size_t c);
unsigned node_get_processors(node_t *node_ptr);
unsigned node_get_processors_used(node_t *node_ptr);
unsigned long node_get_phys_MiB(node_t *node_ptr);
unsigned long node_get_phys_MiB_used(node_t *node_ptr);
int node_get_zfs(node_t *node_ptr);
char *node_get_os(node_t *node_ptr);
char node_get_os_ae(node_t *node_ptr, size_t c);
char *node_get_arch(node_t *node_ptr);
char node_get_arch_ae(node_t *node_ptr, size_t c);
char *node_get_state(node_t *node_ptr);
char node_get_state_ae(node_t *node_ptr, size_t c);
int node_get_msg_fd(node_t *node_ptr);
time_t node_get_last_ping(node_t *node_ptr);
