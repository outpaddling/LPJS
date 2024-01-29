/* scheduler.c */
int lpjs_select_nodes(void);
int lpjs_dispatch_next_job(node_list_t *node_list, job_list_t *job_list);
int lpjs_dispatch_jobs(node_list_t *node_list, job_list_t *job_list);
int lpjs_select_next_job(job_t *job);
