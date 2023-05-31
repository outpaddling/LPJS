#ifndef _NODE_H_
#define _NODE_H_

typedef struct
{
    char            *hostname;
    unsigned        cores;
    unsigned        cores_used;
    unsigned long   phys_mem;
    unsigned long   phys_mem_used;
    int             zfs;        // 0 or 1
    char            *os;
    char            *arch;
    char            *state;
    int             msg_fd;
}   node_t;

#define NODE_STATUS_HEADER_FORMAT "%-12s %-8s %5s %4s %7s %7s %-9s %-9s\n"
#define NODE_STATUS_FORMAT        "%-12s %-8s %5u %4u %7lu %7lu %-9s %-9s\n"

/* Return values for mutator functions */
#define LPSC_NODE_DATA_OK              0
#define LPSC_NODE_DATA_INVALID         -1      // Catch-all for non-specific error
#define LPSC_NODE_DATA_OUT_OF_RANGE    -2
    
/*
 *  Generated by /home/bacon/auto-gen-get-set
 *
 *  Accessor macros.  Use these to access structure members from functions
 *  outside the node_t class.
 *
 *  These generated macros are not expected to be perfect.  Check and edit
 *  as needed before adding to your code.
 */

#define NODE_HOSTNAME(ptr)              ((ptr)->hostname)
#define NODE_HOSTNAME_AE(ptr,c)         ((ptr)->hostname[c])
#define NODE_CORES(ptr)                 ((ptr)->cores)
#define NODE_CORES_USED(ptr)            ((ptr)->cores_used)
#define NODE_PHYS_MEM(ptr)              ((ptr)->phys_mem)
#define NODE_PHYS_MEM_USED(ptr)         ((ptr)->phys_mem_used)
#define NODE_ZFS(ptr)                   ((ptr)->zfs)
#define NODE_OS(ptr)                    ((ptr)->os)
#define NODE_OS_AE(ptr,c)               ((ptr)->os[c])
#define NODE_ARCH(ptr)                  ((ptr)->arch)
#define NODE_ARCH_AE(ptr,c)             ((ptr)->arch[c])
#define NODE_MSG_FD(ptr)                ((ptr)->msg_fd)
#define NODE_STATE(ptr)                 ((ptr)->state)
#define NODE_STATE_AE(ptr,c)            ((ptr)->state[c])

/*
 *  Generated by /home/bacon/auto-gen-get-set
 *
 *  Mutator macros for setting with no sanity checking.  Use these to
 *  set structure members from functions outside the node_t
 *  class.  These macros perform no data validation.  Hence, they achieve
 *  maximum performance where data are guaranteed correct by other means.
 *  Use the mutator functions (same name as the macro, but lower case)
 *  for more robust code with a small performance penalty.
 *
 *  These generated macros are not expected to be perfect.  Check and edit
 *  as needed before adding to your code.
 */

#define NODE_SET_HOSTNAME(ptr,val)              ((ptr)->hostname = (val))
#define NODE_SET_HOSTNAME_CPY(ptr,val,array_size) strlcpy((ptr)->hostname,val,array_size)
#define NODE_SET_HOSTNAME_AE(ptr,c,val)         ((ptr)->hostname[c] = (val))
#define NODE_SET_CORES(ptr,val)                 ((ptr)->cores = (val))
#define NODE_SET_CORES_USED(ptr,val)            ((ptr)->cores_used = (val))
#define NODE_SET_PHYS_MEM(ptr,val)              ((ptr)->mem = (val))
#define NODE_SET_PHYS_MEM_USED(ptr,val)         ((ptr)->mem_used = (val))
#define NODE_SET_ZFS(ptr,val)                   ((ptr)->zfs = (val))
#define NODE_SET_OS(ptr,val)                    ((ptr)->os = (val))
#define NODE_SET_OS_CPY(ptr,val,array_size)     strlcpy((ptr)->os,val,array_size)
#define NODE_SET_OS_AE(ptr,c,val)               ((ptr)->os[c] = (val))
#define NODE_SET_ARCH(ptr,val)                  ((ptr)->arch = (val))
#define NODE_SET_ARCH_CPY(ptr,val,array_size)   strlcpy((ptr)->arch,val,array_size)
#define NODE_SET_ARCH_AE(ptr,c,val)             ((ptr)->arch[c] = (val))
#define NODE_SET_STATE(ptr,val)                 ((ptr)->state = (val))
#define NODE_SET_STATE_CPY(ptr,val,array_size)  strlcpy((ptr)->state,val,array_size)
#define NODE_SET_STATE_AE(ptr,c,val)            ((ptr)->state[c] = (val))
#define NODE_SET_SOCKET_FD(ptr,val)             ((ptr)->socket_fd = (val))

/* node.c */
void    node_init(node_t *node);
void    node_print_status(node_t *node);
void    node_send_specs(node_t *node, int fd);
void    node_send_status(node_t *node, int fd);
void    node_detect_specs(node_t *node);
int     node_receive_specs(node_t *node, int msg_fd);

/* node-mutators.c */
int node_set_hostname(node_t *node_ptr, char *new_hostname);
int node_set_hostname_ae(node_t *node_ptr, size_t c, char new_hostname_element);
int node_set_hostname_cpy(node_t *node_ptr, char *new_hostname, size_t array_size);
int node_set_cores(node_t *node_ptr, unsigned new_cores);
int node_set_cores_used(node_t *node_ptr, unsigned new_cores_used);
int node_set_phys_mem(node_t *node_ptr, unsigned long new_phys_mem);
int node_set_phys_mem_used(node_t *node_ptr, unsigned long new_phys_mem_used);
int node_set_zfs(node_t *node_ptr, int new_zfs);
int node_set_os(node_t *node_ptr, char *new_os);
int node_set_os_ae(node_t *node_ptr, size_t c, char new_os_element);
int node_set_os_cpy(node_t *node_ptr, char *new_os, size_t array_size);
int node_set_arch(node_t *node_ptr, char *new_arch);
int node_set_arch_ae(node_t *node_ptr, size_t c, char new_arch_element);
int node_set_arch_cpy(node_t *node_ptr, char *new_arch, size_t array_size);
int node_set_msg_fd(node_t *node_ptr, int new_socket_fd);
int node_set_state(node_t *node_ptr, char *new_state);
int node_set_state_ae(node_t *node_ptr, size_t c, char new_state_element);
int node_set_state_cpy(node_t *node_ptr, char *new_state, size_t array_size);

#endif  // _NODE_H_
