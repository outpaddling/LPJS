#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sysexits.h>
#include <sys/utsname.h>
#include <sys/socket.h>     // MSG_WAIT_ALL

#include <xtend/file.h>     // xt_dprintf()

#include "node-private.h"
#include "network.h"
#include "lpjs.h"
#include "misc.h"


/***************************************************************************
 *  Description:
 *  
 *  Returns:
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-02-01  Jason Bacon Begin
 ***************************************************************************/

node_t  *node_new(void)

{
    node_t  *node;
    
    if ( (node = malloc(sizeof(node_t))) == NULL )
    {
	lpjs_log("%s(): malloc failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    node_init(node);
    
    return node;
}


/***************************************************************************
 *  Description:
 *      Constructor for node_t
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-23  Jason Bacon Begin
 ***************************************************************************/

void    node_init(node_t *node)

{
    node->hostname = NULL;
    node->phys_MiB = 0;
    node->phys_MiB_used = 0;
    node->procs = 0;
    node->procs_used = 0;
    node->zfs = 0;
    node->os = "Unknown";
    node->arch = "Unknown";
    node->state = "Unknown";
    node->msg_fd = NODE_MSG_FD_NOT_OPEN;
    node->last_ping = 0;
}


/***************************************************************************
 *  Description:
 *      Detect hardware specs and OS of the node running this function
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-10-02  Jason Bacon Begin
 ***************************************************************************/

void    node_detect_specs(node_t *node)

{
    struct utsname  u_name;
    char            temp_hostname[sysconf(_SC_HOST_NAME_MAX) + 1],
		    temp_osname[128],
		    *ostype_path;
    FILE            *fp;
    struct stat     st;
    
    /*
     *  hwloc is extremely complex and we don't need most of its functionality
     *  here, so just gather info the simple way
     */

    // FIXME: Verify malloc() success
    gethostname(temp_hostname, sysconf(_SC_HOST_NAME_MAX));
    // FIXME: Check malloc success
    node->hostname = strdup(temp_hostname);
    
    // May include SMT/Hyperthreading.  Disable in BIOS for Linux or using
    // sysctl on FreeBSD if you don't want to oversubscribe physical procs.
    node->procs = sysconf(_SC_NPROCESSORS_ONLN);
    node->phys_MiB = sysconf(_SC_PAGESIZE) * sysconf(_SC_PHYS_PAGES)
		     / 1024 / 1024;
    /*
     *  Report 1 if ZFS filesystem found, so that additional memory
     *  can be reserved on compute nodes.
     *  FIXME: There should be a better approach to this.
     */
    node->zfs = ! system("mount | fgrep -q zfs");
    
    uname(&u_name);
    ostype_path = LOCALBASE "/bin/auto-ostype";
    if ( (stat(ostype_path, &st) == 0) && (fp = popen(ostype_path, "r")) != NULL )
    {
	xt_fgetline(fp, temp_osname, 128);
	if ( (node->os = strdup(temp_osname)) == NULL )
	{
	    lpjs_log("%s(): strdup() failed.\n", __FUNCTION__);
	    exit(EX_UNAVAILABLE);
	}
	pclose(fp);
    }
    else
    {
	if ( (node->os = strdup(u_name.sysname)) == NULL )
	{
	    lpjs_log("%s(): strdup() failed.\n", __FUNCTION__);
	    exit(EX_UNAVAILABLE);
	}
    }
    node->arch = strdup(u_name.machine);
}


/***************************************************************************
 *  Description:
 *      Print header for node status info
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-01-28  Jason Bacon Begin
 ***************************************************************************/

void    node_print_status_header(FILE *stream)

{
    fprintf(stderr, NODE_STATUS_HEADER_FORMAT, "Hostname", "State",
	  "Procs", "Used", "PhysMiB", "Used", "OS", "Arch");
}


/***************************************************************************
 *  Description:
 *      Print current nodes in human-readable form
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-23  Jason Bacon Begin
 ***************************************************************************/

void    node_print_status(node_t *node, FILE *stream)

{
    fprintf(stream, NODE_STATUS_FORMAT, node->hostname, node->state,
	   node->procs, node->procs_used,
	   node->phys_MiB, node->phys_MiB_used, node->os, node->arch);
}


void    node_status_to_str(node_t *node, char *str, size_t array_size)

{
    snprintf(str, array_size,
		NODE_STATUS_FORMAT, node->hostname, node->state,        
		node->procs, node->procs_used,                                 
		node->phys_MiB, node->phys_MiB_used, node->os, node->arch);
}


/***************************************************************************
 *  Description:
 *      Send current node info to msg_fd in human-readable form, e.g. in
 *      response to lpjs-nodes
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-09-23  Jason Bacon Begin
 ***************************************************************************/

void    node_send_status(node_t *node, int msg_fd)

{
    char    outgoing_msg[LPJS_MSG_LEN_MAX + 1];
    
    node_status_to_str(node, outgoing_msg, LPJS_MSG_LEN_MAX + 1);
    if ( lpjs_send_munge(msg_fd, outgoing_msg, close) != LPJS_MSG_SENT )
    {
	// This function should never be called by dispatchd, so
	// use a simple close() vs lpjs_dispatchd_safe_close()
	lpjs_log("send_node_specs(): xt_dprintf() failed: %s", strerror(errno));
	exit(EX_IOERR);
    }
}


int     node_print_specs_header(FILE *stream)

{
    return fprintf(stream,
		  "%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
		  "Hostname", "state", "procs",
		  "phys_MiB", "zfs", "os", "arch");
}


char    *node_specs_to_str(node_t *node, char *str, size_t buff_len)

{
    if ( snprintf(str, buff_len,
		  "%s\t%s\t%u\t%lu\t%u\t%s\t%s",
		  node->hostname, node->state, node->procs,
		  node->phys_MiB, node->zfs, node->os, node->arch) < 0 )
    {
	lpjs_log("%s(): snprintf() failed\n", __FUNCTION__);
	exit(EX_IOERR);
    }
    
    return str;
}


/***************************************************************************
 *  Description:
 *      Receive node hardware and OS specs via msg_fd in
 *      machine-readable for, e.g. from compd to dispatchd
 *
 *  History: 
 *  Date        Name        Modification
 *  2021-10-02  Jason Bacon Begin
 ***************************************************************************/

ssize_t node_str_to_specs(node_t *node, const char *str)

{
    char    *temp_str,
	    *field,
	    *stringp,
	    *end;
    
    node_init(node);
    
    if ( (temp_str = strdup(str)) == NULL )
    {
	lpjs_log("%s(): strdup() failed.\n", __FUNCTION__);
	exit(EX_UNAVAILABLE);
    }
    
    node->state = "Unknown";
    node->os = "Unknown";
    node->arch = "Unknown";

    stringp = temp_str;
    
    if ( (field = strsep(&stringp, "\t")) == NULL )
    {
	lpjs_log("%s(): Failed to extract hostname from specs.\n", __FUNCTION__);
	return -1;
    }
    node->hostname = strdup(field);

    if ( (field = strsep(&stringp, "\t")) == NULL )
    {
	lpjs_log("%s(): Failed to extract state from specs.\n", __FUNCTION__);
	return -1;
    }
    node->state = strdup(field);

    if ( (field = strsep(&stringp, "\t")) == NULL )
    {
	lpjs_log("%s(): Failed to extract procs from specs.\n", __FUNCTION__);
	return -1;
    }
    node->procs = strtoul(field, &end, 10);
    if ( *end != '\0' )
    {
	lpjs_log("%s(): Procs field is not a valid number.\n", __FUNCTION__);
	return -1;
    }

    if ( (field = strsep(&stringp, "\t")) == NULL )
    {
	lpjs_log("%s(): Failed to extract physMiB from specs.\n", __FUNCTION__);
	return -1;
    }
    node->phys_MiB = strtoul(field, &end, 10);
    if ( *end != '\0' )
    {
	lpjs_log("%s(): PhysMiB field is not a valid number.\n", __FUNCTION__);
	return -1;
    }

    if ( (field = strsep(&stringp, "\t")) == NULL )
    {
	lpjs_log("%s(): Failed to extract ZFS Boolean from specs.\n", __FUNCTION__);
	return -1;
    }
    node->zfs = strtoul(field, &end, 10);
    if ( *end != '\0' )
    {
	lpjs_log("%s(): ZFS field is not a valid number (should be 0 or 1).\n", __FUNCTION__);
	return -1;
    }

    if ( (field = strsep(&stringp, "\t")) == NULL )
    {
	lpjs_log("%s(): Failed to extract OS from specs.\n", __FUNCTION__);
	return -1;
    }
    node->os = strdup(field);

    if ( (field = strsep(&stringp, "\t\n")) == NULL )
    {
	lpjs_log("%s(): Failed to extract arch from specs.\n", __FUNCTION__);
	return -1;
    }
    node->arch = strdup(field);

    return 0;
}


/***************************************************************************
 *  Description:
 *      Release resources on node associated with job
 *  
 *  History: 
 *  Date        Name        Modification
 *  2024-12-08  Jason Bacon Begin
 ***************************************************************************/

int     node_release_resources(node_t *node, job_t *job)

{
    lpjs_log("%s(): Releasing %d procs and %lu MiB on %s.\n",
	     __FUNCTION__, job_get_procs_per_job(job), 
	     job_get_pmem_per_proc(job) * job_get_procs_per_job(job),
	     node_get_hostname(node));
    node->procs_used -= job_get_procs_per_job(job);
    node->phys_MiB_used -= job_get_pmem_per_proc(job) * job_get_procs_per_job(job);
    
    return 0;   // FIXME: Define return codes
}
