/***************************************************************************
 *  This file is automatically generated by gen-get-set.  Be sure to keep
 *  track of any manual changes.
 *
 *  These generated functions are not expected to be perfect.  Check and
 *  edit as needed before adding to your code.
 ***************************************************************************/

#include <string.h>
#include <ctype.h>
#include <stdbool.h>        // In case of bool
#include <stdint.h>         // In case of int64_t, etc
#include <xtend/string.h>   // strlcpy() on Linux
#include "job-private.h"


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for job_id member in a job_t structure.
 *      Use this function to set job_id in a job_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      job_id is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      new_job_id      The new value for job_id
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      unsigned long   new_job_id;
 *
 *      if ( job_set_job_id(&job, new_job_id)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_job_id(job_t *job_ptr, unsigned long new_job_id)

{
    if ( false )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	job_ptr->job_id = new_job_id;
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for array_index member in a job_t structure.
 *      Use this function to set array_index in a job_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      array_index is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      new_array_index The new value for array_index
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      unsigned long   new_array_index;
 *
 *      if ( job_set_array_index(&job, new_array_index)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_array_index(job_t *job_ptr, unsigned long new_array_index)

{
    if ( false )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	job_ptr->array_index = new_array_index;
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for job_count member in a job_t structure.
 *      Use this function to set job_count in a job_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      job_count is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      new_job_count   The new value for job_count
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      unsigned        new_job_count;
 *
 *      if ( job_set_job_count(&job, new_job_count)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_job_count(job_t *job_ptr, unsigned new_job_count)

{
    if ( false )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	job_ptr->job_count = new_job_count;
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for cores_per_job member in a job_t structure.
 *      Use this function to set cores_per_job in a job_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      cores_per_job is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      new_cores_per_job The new value for cores_per_job
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      unsigned        new_cores_per_job;
 *
 *      if ( job_set_cores_per_job(&job, new_cores_per_job)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_cores_per_job(job_t *job_ptr, unsigned new_cores_per_job)

{
    if ( false )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	job_ptr->cores_per_job = new_cores_per_job;
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for min_cores_per_node member in a job_t structure.
 *      Use this function to set min_cores_per_node in a job_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      min_cores_per_node is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      new_min_cores_per_node The new value for min_cores_per_node
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      unsigned        new_min_cores_per_node;
 *
 *      if ( job_set_min_cores_per_node(&job, new_min_cores_per_node)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_min_cores_per_node(job_t *job_ptr, unsigned new_min_cores_per_node)

{
    if ( false )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	job_ptr->min_cores_per_node = new_min_cores_per_node;
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for mem_per_core member in a job_t structure.
 *      Use this function to set mem_per_core in a job_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      mem_per_core is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      new_mem_per_core The new value for mem_per_core
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      size_t          new_mem_per_core;
 *
 *      if ( job_set_mem_per_core(&job, new_mem_per_core)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_mem_per_core(job_t *job_ptr, size_t new_mem_per_core)

{
    if ( false )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	job_ptr->mem_per_core = new_mem_per_core;
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for user_name member in a job_t structure.
 *      Use this function to set user_name in a job_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      user_name is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      new_user_name   The new value for user_name
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      char *          new_user_name;
 *
 *      if ( job_set_user_name(&job, new_user_name)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_user_name(job_t *job_ptr, char * new_user_name)

{
    if ( new_user_name == NULL )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	job_ptr->user_name = new_user_name;
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for an array element of user_name member in a job_t
 *      structure. Use this function to set job_ptr->user_name[c]
 *      in a job_t object from non-member functions.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      c               Subscript to the user_name array
 *      new_user_name_element The new value for user_name[c]
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      size_t          c;
 *      char *          new_user_name_element;
 *
 *      if ( job_set_user_name_ae(&job, c, new_user_name_element)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      JOB_SET_USER_NAME_AE(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_user_name_ae(job_t *job_ptr, size_t c, char  new_user_name_element)

{
    if ( false )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	job_ptr->user_name[c] = new_user_name_element;
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for user_name member in a job_t structure.
 *      Use this function to set user_name in a job_t object
 *      from non-member functions.  This function copies the array pointed to
 *      by new_user_name to job_ptr->user_name.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      new_user_name   The new value for user_name
 *      array_size      Size of the user_name array.
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      char *          new_user_name;
 *      size_t          array_size;
 *
 *      if ( job_set_user_name_cpy(&job, new_user_name, array_size)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      JOB_SET_USER_NAME(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_user_name_cpy(job_t *job_ptr, char * new_user_name, size_t array_size)

{
    if ( new_user_name == NULL )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	// FIXME: Assuming char array is a null-terminated string
	strlcpy(job_ptr->user_name, new_user_name, array_size);
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for primary_group_name member in a job_t structure.
 *      Use this function to set primary_group_name in a job_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      primary_group_name is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      new_primary_group_name The new value for primary_group_name
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      char *          new_primary_group_name;
 *
 *      if ( job_set_primary_group_name(&job, new_primary_group_name)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_primary_group_name(job_t *job_ptr, char * new_primary_group_name)

{
    if ( new_primary_group_name == NULL )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	job_ptr->primary_group_name = new_primary_group_name;
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for an array element of primary_group_name member in a job_t
 *      structure. Use this function to set job_ptr->primary_group_name[c]
 *      in a job_t object from non-member functions.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      c               Subscript to the primary_group_name array
 *      new_primary_group_name_element The new value for primary_group_name[c]
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      size_t          c;
 *      char *          new_primary_group_name_element;
 *
 *      if ( job_set_primary_group_name_ae(&job, c, new_primary_group_name_element)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      JOB_SET_PRIMARY_GROUP_NAME_AE(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_primary_group_name_ae(job_t *job_ptr, size_t c, char  new_primary_group_name_element)

{
    if ( false )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	job_ptr->primary_group_name[c] = new_primary_group_name_element;
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for primary_group_name member in a job_t structure.
 *      Use this function to set primary_group_name in a job_t object
 *      from non-member functions.  This function copies the array pointed to
 *      by new_primary_group_name to job_ptr->primary_group_name.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      new_primary_group_name The new value for primary_group_name
 *      array_size      Size of the primary_group_name array.
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      char *          new_primary_group_name;
 *      size_t          array_size;
 *
 *      if ( job_set_primary_group_name_cpy(&job, new_primary_group_name, array_size)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      JOB_SET_PRIMARY_GROUP_NAME(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_primary_group_name_cpy(job_t *job_ptr, char * new_primary_group_name, size_t array_size)

{
    if ( new_primary_group_name == NULL )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	// FIXME: Assuming char array is a null-terminated string
	strlcpy(job_ptr->primary_group_name, new_primary_group_name, array_size);
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for submit_host member in a job_t structure.
 *      Use this function to set submit_host in a job_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      submit_host is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      new_submit_host The new value for submit_host
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      char *          new_submit_host;
 *
 *      if ( job_set_submit_host(&job, new_submit_host)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_submit_host(job_t *job_ptr, char * new_submit_host)

{
    if ( new_submit_host == NULL )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	job_ptr->submit_host = new_submit_host;
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for an array element of submit_host member in a job_t
 *      structure. Use this function to set job_ptr->submit_host[c]
 *      in a job_t object from non-member functions.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      c               Subscript to the submit_host array
 *      new_submit_host_element The new value for submit_host[c]
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      size_t          c;
 *      char *          new_submit_host_element;
 *
 *      if ( job_set_submit_host_ae(&job, c, new_submit_host_element)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      JOB_SET_SUBMIT_HOST_AE(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_submit_host_ae(job_t *job_ptr, size_t c, char  new_submit_host_element)

{
    if ( false )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	job_ptr->submit_host[c] = new_submit_host_element;
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for submit_host member in a job_t structure.
 *      Use this function to set submit_host in a job_t object
 *      from non-member functions.  This function copies the array pointed to
 *      by new_submit_host to job_ptr->submit_host.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      new_submit_host The new value for submit_host
 *      array_size      Size of the submit_host array.
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      char *          new_submit_host;
 *      size_t          array_size;
 *
 *      if ( job_set_submit_host_cpy(&job, new_submit_host, array_size)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      JOB_SET_SUBMIT_HOST(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_submit_host_cpy(job_t *job_ptr, char * new_submit_host, size_t array_size)

{
    if ( new_submit_host == NULL )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	// FIXME: Assuming char array is a null-terminated string
	strlcpy(job_ptr->submit_host, new_submit_host, array_size);
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for working_directory member in a job_t structure.
 *      Use this function to set working_directory in a job_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      working_directory is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      new_working_directory The new value for working_directory
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      char *          new_working_directory;
 *
 *      if ( job_set_working_directory(&job, new_working_directory)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_working_directory(job_t *job_ptr, char * new_working_directory)

{
    if ( new_working_directory == NULL )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	job_ptr->working_directory = new_working_directory;
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for an array element of working_directory member in a job_t
 *      structure. Use this function to set job_ptr->working_directory[c]
 *      in a job_t object from non-member functions.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      c               Subscript to the working_directory array
 *      new_working_directory_element The new value for working_directory[c]
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      size_t          c;
 *      char *          new_working_directory_element;
 *
 *      if ( job_set_working_directory_ae(&job, c, new_working_directory_element)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      JOB_SET_WORKING_DIRECTORY_AE(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_working_directory_ae(job_t *job_ptr, size_t c, char  new_working_directory_element)

{
    if ( false )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	job_ptr->working_directory[c] = new_working_directory_element;
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for working_directory member in a job_t structure.
 *      Use this function to set working_directory in a job_t object
 *      from non-member functions.  This function copies the array pointed to
 *      by new_working_directory to job_ptr->working_directory.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      new_working_directory The new value for working_directory
 *      array_size      Size of the working_directory array.
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      char *          new_working_directory;
 *      size_t          array_size;
 *
 *      if ( job_set_working_directory_cpy(&job, new_working_directory, array_size)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      JOB_SET_WORKING_DIRECTORY(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_working_directory_cpy(job_t *job_ptr, char * new_working_directory, size_t array_size)

{
    if ( new_working_directory == NULL )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	// FIXME: Assuming char array is a null-terminated string
	strlcpy(job_ptr->working_directory, new_working_directory, array_size);
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for script_name member in a job_t structure.
 *      Use this function to set script_name in a job_t object
 *      from non-member functions.  This function performs a direct
 *      assignment for scalar or pointer structure members.  If
 *      script_name is a pointer, data previously pointed to should
 *      be freed before calling this function to avoid memory
 *      leaks.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      new_script_name The new value for script_name
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      char *          new_script_name;
 *
 *      if ( job_set_script_name(&job, new_script_name)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      (3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_script_name(job_t *job_ptr, char * new_script_name)

{
    if ( new_script_name == NULL )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	job_ptr->script_name = new_script_name;
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for an array element of script_name member in a job_t
 *      structure. Use this function to set job_ptr->script_name[c]
 *      in a job_t object from non-member functions.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      c               Subscript to the script_name array
 *      new_script_name_element The new value for script_name[c]
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      size_t          c;
 *      char *          new_script_name_element;
 *
 *      if ( job_set_script_name_ae(&job, c, new_script_name_element)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      JOB_SET_SCRIPT_NAME_AE(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_script_name_ae(job_t *job_ptr, size_t c, char  new_script_name_element)

{
    if ( false )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	job_ptr->script_name[c] = new_script_name_element;
	return JOB_DATA_OK;
    }
}


/***************************************************************************
 *  Library:
 *      #include <job.h>
 *      
 *
 *  Description:
 *      Mutator for script_name member in a job_t structure.
 *      Use this function to set script_name in a job_t object
 *      from non-member functions.  This function copies the array pointed to
 *      by new_script_name to job_ptr->script_name.
 *
 *  Arguments:
 *      job_ptr         Pointer to the structure to set
 *      new_script_name The new value for script_name
 *      array_size      Size of the script_name array.
 *
 *  Returns:
 *      JOB_DATA_OK if the new value is acceptable and assigned
 *      JOB_DATA_OUT_OF_RANGE otherwise
 *
 *  Examples:
 *      job_t           job;
 *      char *          new_script_name;
 *      size_t          array_size;
 *
 *      if ( job_set_script_name_cpy(&job, new_script_name, array_size)
 *              == JOB_DATA_OK )
 *      {
 *      }
 *
 *  See also:
 *      JOB_SET_SCRIPT_NAME(3)
 *
 *  History: 
 *  Date        Name        Modification
 *  2024-03-19  gen-get-set Auto-generated from job-private.h
 ***************************************************************************/

int     job_set_script_name_cpy(job_t *job_ptr, char * new_script_name, size_t array_size)

{
    if ( new_script_name == NULL )
	return JOB_DATA_OUT_OF_RANGE;
    else
    {
	// FIXME: Assuming char array is a null-terminated string
	strlcpy(job_ptr->script_name, new_script_name, array_size);
	return JOB_DATA_OK;
    }
}
