/* scheduler.c */
int lpjs_select_nodes(void);
int lpjs_dispatch_next_job(node_list_t *node_list, job_list_t *pending_jobs, job_list_t *running_jobs);
int lpjs_dispatch_jobs(node_list_t *node_list, job_list_t *pending_jobs, job_list_t *running_jobs);
unsigned long lpjs_select_next_job(job_list_t *pending_jobs, job_t **job);
int lpjs_match_nodes(job_t *job, node_list_t *node_list, node_list_t *matched_nodes);
int lpjs_get_usable_procs(job_t *job, node_t *node);
job_t *lpjs_remove_job(job_list_t *running_jobs, unsigned long job_id);
