/* scheduler.c */
int lpjs_select_nodes(void);
int lpjs_dispatch_next_job(node_list_t *node_list, job_list_t *job_list);
int lpjs_dispatch_jobs(node_list_t *node_list, job_list_t *job_list);
int lpjs_select_next_job(job_t *job);
int lpjs_match_nodes(job_t *job, node_list_t *node_list, node_list_t *matched_nodes);
int lpjs_get_usable_cores(job_t *job, node_t *node);
