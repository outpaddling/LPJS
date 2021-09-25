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
#include <limits.h>
#include "node-list.h"

int     main(int argc,char *argv[])

{
    char    config_file[PATH_MAX+1];
    
    node_list_t nodes;
    
    node_list_init(&nodes);
    snprintf(config_file, PATH_MAX+1, "%s/etc/lpjs/config", LOCALBASE);
    node_list_populate(&nodes, config_file);
    return EX_OK;
}

