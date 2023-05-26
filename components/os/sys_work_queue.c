#include "sys_work_queue.h"
#include "sys_os.h"

static wq_handle_t sys_work_queue_handle;
static bool sys_work_queue_init_flag;

int sys_work_queue_init(void)
{
	int rv = SYS_OK;
	/* If the queue has already been initialized, just return SUCCESS */
	if (!sys_work_queue_init_flag) 
	{
		wq_cfg_t cfg;
		memset(&cfg, 0, sizeof(cfg));
		cfg.worker_stack_bytes = CONFIG_SYS_WQ_STACK;
		cfg.worker_priority = DEFAULT_WORKER_PRIO;
		cfg.worker_isr_jobs_reserve = DEFAULT_ISR_JOBS_RESERVE;
		rv = work_queue_init(&cfg, &sys_work_queue_handle);
		if (rv == SYS_OK)
		{
			sys_work_queue_init_flag = true;
		}
	}
	return rv;
}

wq_handle_t sys_work_queue_get_handle(void)
{
	return sys_work_queue_handle;
}
