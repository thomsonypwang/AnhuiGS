#include "healthmon.h"
#include "sys_log.h"
#include "sys_os.h"
#include "sys_errno.h"
#include "wdt_drv.h"

//#include <pwrmgr.h>

#define hmon_e(...)	log_e("hmon", ##__VA_ARGS__)
#define hmon_w(...)	log_w("hmon", ##__VA_ARGS__)

#ifdef CONFIG_HEALTHMON_DEBUG
	#define hmon_d(...)	log("hmon", ##__VA_ARGS__)
#else
	#define hmon_d(...)
#endif /* ! CONFIG_HEALTHMON_DEBUG */


struct healthmon_privdata 
{
	unsigned char	is_valid;
	unsigned char	failed;
	unsigned int	last_check;
};

static struct healthmon_gdata 
{
	struct healthmon_handler	handlers[MAX_HM_HANDLERS];
	struct healthmon_privdata	priv[MAX_HM_HANDLERS];
	void				(*final_about_to_die) ();
	os_semaphore_t			h_sem;
	unsigned int			last_strobe;
} hm_gdata;

static os_thread_t healthmon_thread;
static os_thread_stack_define(healthmon_stack, 1024);
static unsigned char healthmon_started;
static unsigned char healthmon_running;
static unsigned char healthmon_initialized;

int healthmon_set_final_about_to_die_handler(void (*fun)())
{
	if (!healthmon_initialized)
	{
		return SYS_E_PERM;
	}
	hm_gdata.final_about_to_die = fun;
	return SYS_OK;
}

void healthmon_display_stat(void)
{
	int i, ret;
	char c_is_sick, c_about_to_die;

	ret = os_semaphore_get(&hm_gdata.h_sem, OS_WAIT_FOREVER);
	if (ret != SYS_OK) 
	{
		hmon_e("Error getting h_sem.");
		return;
	}

	for (i = 0; i < MAX_HM_HANDLERS; i++)
	{
		if (!hm_gdata.priv[i].is_valid)
		{
			continue;
		}
		c_is_sick = hm_gdata.handlers[i].is_sick ? 'Y' : 'N';
		c_about_to_die = hm_gdata.handlers[i].about_to_die ? 'Y' : 'N';
		log_i("%s\t%c\t%c\t%d", hm_gdata.handlers[i].name,c_is_sick,c_about_to_die,hm_gdata.priv[i].failed);
	}

	ret = os_semaphore_put(&hm_gdata.h_sem);
	if (ret != SYS_OK) 
	{
		hmon_e("Error putting h_sem. ");
		return;
	}

	return;
}

static void call_die_handlers(int failed_module_index)
{
	int i;

	for (i = 0; i < MAX_HM_HANDLERS; i++) 
	{
		if (!hm_gdata.priv[i].is_valid)
		{
			continue;
		}
		if (!hm_gdata.handlers[i].about_to_die)
		{
			continue;
		}

		hm_gdata.handlers[i].about_to_die(i == failed_module_index);
	}

	if (hm_gdata.final_about_to_die != NULL)
	{
		hm_gdata.final_about_to_die();
	}
	return;
}

static void healthmon_loop(os_thread_arg_t data)
{
	int ret, i;
	unsigned int cur_msec;

	int should_probe_wd = 1;

	healthmon_running = 1;

	wdt_drv_set_timeout(3);
	wdt_drv_start();

	while (healthmon_running) 
	{
		//log_i("healthmon");
		ret = os_semaphore_get(&hm_gdata.h_sem, OS_WAIT_FOREVER);
		if (ret != SYS_OK) 
		{
			hmon_e("Error getting h_sem. ");
			continue;
		}
		for (i = 0; i < MAX_HM_HANDLERS; i++) 
		{
			cur_msec = os_ticks_to_msec(os_ticks_get());
			if (!hm_gdata.priv[i].is_valid)
				continue;

			if (cur_msec  < hm_gdata.priv[i].last_check + (hm_gdata.handlers[i].check_interval * 1000))
				continue;

			if (hm_gdata.handlers[i].is_sick) 
			{
				bool sick = (hm_gdata.handlers[i].is_sick)(cur_msec);
				hm_gdata.priv[i].last_check = cur_msec;

				if (sick) 
				{
					hm_gdata.priv[i].failed++;
				} 
				else 
				{
					hm_gdata.priv[i].failed = 0;
				}
			}

			if (hm_gdata.priv[i].failed >=hm_gdata.handlers[i].consecutive_failures) 
			{
				call_die_handlers(i);
				should_probe_wd = 0;
				break;
			}
		}

		ret = os_semaphore_put(&hm_gdata.h_sem);
		if (ret != SYS_OK) 
		{
			hmon_e("Error putting h_sem. ");
			continue;
		}

		if (should_probe_wd) 
		{
			//log_i("healthmon ::%d cur_msec=%d",(hm_gdata.last_strobe + (WD_STROBE_INTERVAL * 1000)),cur_msec);
			if ((hm_gdata.last_strobe + (WD_STROBE_INTERVAL * 1000)) <= cur_msec) 
			{
				wdt_drv_strobe();
				hm_gdata.last_strobe = cur_msec;
			}
		} 
		else 
		{
			break;
		}
		os_thread_sleep(os_msec_to_ticks(WKUP_INTERVAL * 1000));
	}
	os_thread_self_complete(&healthmon_thread);
}

static int lock_and_find_handler_by_name(const char *name)
{
	int i, ret;

	if (!name)
	{
		return -1;
	}
	ret = os_semaphore_get(&hm_gdata.h_sem, OS_WAIT_FOREVER);
	if (ret != SYS_OK) 
	{
		hmon_e("Error getting h_sem. ");
		return -1;
	}

	for (i = 0; i < MAX_HM_HANDLERS; i++) 
	{
		if (!hm_gdata.priv[i].is_valid) 
		{
			continue;
		}
		if (strncmp(hm_gdata.handlers[i].name,name, HM_NAME_MAX)) 
		{
			continue;
		}
		return i;
	}
	return -1;
}

static void unlock_handler_list(void)
{
	int ret;

	ret = os_semaphore_put(&hm_gdata.h_sem);
	if (ret != SYS_OK) 
	{
		hmon_e("Error putting h_sem. ");
	}
	return;
}

int healthmon_change_check_interval(const char *name, unsigned int interval)
{
	int index;

	if ((interval < WKUP_INTERVAL) || (interval % WKUP_INTERVAL))
	{
		return SYS_FAIL;
	}
	index = lock_and_find_handler_by_name(name);
	if (index < 0) 
	{
		hmon_e("Handler not found ");
		unlock_handler_list();
		return SYS_FAIL;
	}
	hm_gdata.handlers[index].check_interval = interval;
	unlock_handler_list();

	return SYS_OK;
}

int healthmon_unregister_handler(const char *name)
{
	int index;

	index = lock_and_find_handler_by_name(name);
	if (index < 0) 
	{
		hmon_e("Handler not found ");
		unlock_handler_list();
		return SYS_FAIL;
	}
	memset(&(hm_gdata.handlers[index]), 0, sizeof(struct healthmon_handler));
	hm_gdata.priv[index].is_valid = 0;

	unlock_handler_list();
	return SYS_OK;
}

int healthmon_register_handler(struct healthmon_handler *handler)
{
	int i, index;

	if (strlen(handler->name) == 0)
	{
		return SYS_FAIL;
	}
	if ((handler->check_interval < WKUP_INTERVAL) ||(handler->check_interval % WKUP_INTERVAL) != 0) 
	{
		hmon_e("Invalid wakeup interval specified. Wakeup interval should be multiple of %d "
			       "and greater than %d seconds ",WKUP_INTERVAL, WKUP_INTERVAL);
		return SYS_FAIL;
	}

	if ((!handler->is_sick) && (!handler->about_to_die)) 
	{
		hmon_e("No callback registered.");
		return SYS_FAIL;
	}

	index = lock_and_find_handler_by_name(handler->name);
	if (index >= 0) 
	{
		hmon_e("Handler already registered");
		unlock_handler_list();
		return SYS_FAIL;
	}

	for (i = 0; i < MAX_HM_HANDLERS; i++) 
	{
		if (!hm_gdata.priv[i].is_valid) 
		{
			memcpy(&(hm_gdata.handlers[i]), handler,sizeof(struct healthmon_handler));
			hm_gdata.priv[i].is_valid = 1;
			break;
		}
	}
	
	unlock_handler_list();
	if (i == MAX_HM_HANDLERS) 
	{
		hmon_e("Can't register more than %d handlers ",MAX_HM_HANDLERS);
		return SYS_FAIL;
	}
	return SYS_OK;
}

int healthmon_init(void)
{
	int i, ret;

	if (healthmon_initialized)
	{
		return SYS_OK;
	}
	healthmon_initialized = 1;
	healthmon_running = 0;

	for (i = 0; i < MAX_HM_HANDLERS; i++) 
	{
		memset(&(hm_gdata.handlers[i]), 0,sizeof(struct healthmon_handler));
		hm_gdata.priv[i].is_valid = 0;
		hm_gdata.priv[i].failed = 0;
	}

	ret = os_semaphore_create(&hm_gdata.h_sem, "healthmon_sem");
	if (ret) 
	{
		hmon_e("Failed to create healthmon semaphore");
		return SYS_FAIL;
	}

	return SYS_OK;
}

int healthmon_start(void)
{
	if (!healthmon_initialized)
	{
		return SYS_FAIL;
	}
	if (healthmon_started)
	{
		return SYS_OK;
	}
	healthmon_started = 1;

	return os_thread_create(&healthmon_thread, 
							"healthmon",
							healthmon_loop, 
							0,
							&healthmon_stack,
							OS_PRIO_0);
}
