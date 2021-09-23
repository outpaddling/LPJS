#ifndef _NODE_H_
#define _NODE_H_

typedef struct
{
    char            *hostname;
    unsigned        cores;
    unsigned        cores_used;
    unsigned long   mem;
    unsigned long   mem_used;
}   node_t;

void    node_init(node_t *node);

#endif  // _NODE_H_
