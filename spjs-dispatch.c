/***************************************************************************
 *  Description:
 *  
 *  Arguments:
 *
 *  Returns:
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-23  Jason Bacon Begin
 ***************************************************************************/

#include <stdio.h>
#include <sysexits.h>
#include "node-list.h"

int     main(int argc,char *argv[])

{
    node_list_t nodes;
    
    node_list_init(&nodes);
    return EX_OK;
}

