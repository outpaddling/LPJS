#ifndef _LPJS_NODE_LIST_H_
#define _LPJS_NODE_LIST_H_

#ifndef _LPJS_NODE_H_
#include "node.h"
#endif

typedef struct node_list node_list_t;

#define LPJS_MAX_NODES  1024

#include "node-list-rvs.h"
#include "node-list-accessors.h"
#include "node-list-mutators.h"
#include "node-list-protos.h"

/* Return values for mutator functions */
#define NODE_LIST_DATA_OK              0
#define NODE_LIST_DATA_INVALID         -1      // Catch-all for non-specific error
#define NODE_LIST_DATA_OUT_OF_RANGE    -2

#endif  // _LPJS_NODE_LIST_H_
