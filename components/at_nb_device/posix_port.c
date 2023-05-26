#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "aiot_state_api.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "posix_port.h"
#include <core_config.h>
 
#include "nb_uart_dir.h"
#include "sys_os.h"
#include "sys_log.h"


static os_thread_t nb_rx_thread= NULL;
static os_thread_stack_define(nb_rx_stack, 4*1024);

extern int32_t aiot_at_hal_recv_handle(uint8_t *data, uint32_t size);

 void nb_rx_control_thread(os_thread_arg_t data)
{
    int32_t len = 0;
    uint8_t buf[2048] = {0};
    while (1) 
	{
        len = at_uart_receive(buf, sizeof(buf),1000);
        if (len <= 4) 
		{
            os_thread_sleep(os_msec_to_ticks(1000));
            continue;
        }
        aiot_at_hal_recv_handle(buf, len);
        memset(buf, 0, sizeof(buf));
    }	
	
	vTaskDelete(nb_rx_thread); 
}

void nb_rx_process_init(void)
{
	int ret=SYS_FAIL;
	ret = os_thread_create(&nb_rx_thread, //任务控制块指针
							"nb_rx_thread",//任务名字
							nb_rx_control_thread, //任务入口函数
							NULL,//任务入口函数参数
							&nb_rx_stack,//任务栈大小
							OS_PRIO_9);	//任务的优先级							
   if(ret==SYS_FAIL)
   {
		log_e("nb","main thread create error!");
   }
}

int32_t at_hal_init(void)
{
	nbiot_uart_init();
	nb_rx_process_init();
    return STATE_SUCCESS;
}

void *core_sysdep_malloc(uint32_t size, char *name)
{
    return pvPortMalloc(size);
}

void core_sysdep_free(void *ptr)
{
    vPortFree(ptr);
}

int32_t core_usart_send(void *data, uint32_t size, uint32_t timeout)
{
    return at_uart_send(data, size, timeout);
}

uint64_t core_sysdep_time(void)
{
    return (uint64_t)(xTaskGetTickCount() * portTICK_RATE_MS);
}

void core_sysdep_sleep(uint64_t time_ms)
{
    vTaskDelay(pdMS_TO_TICKS(time_ms));
}

void *core_sysdep_mutex_init(void)
{
    return (void *)xSemaphoreCreateMutex();
}

void core_sysdep_mutex_lock(void *mutex)
{
    xSemaphoreTake((SemaphoreHandle_t)mutex, AIOT_WAIT_MAX_TIME);
}

void core_sysdep_mutex_unlock(void *mutex)
{
    xSemaphoreGive((SemaphoreHandle_t)mutex);
}

void core_sysdep_mutex_deinit(void **mutex)
{
    if (mutex == NULL || *mutex == NULL) 
	{
        return;
    }
    vSemaphoreDelete((SemaphoreHandle_t)*mutex);
    *mutex = NULL;
}

void *core_sysdep_sem_init(void)
{   
    return (void *)xSemaphoreCreateBinary();
}

int32_t core_sysdep_sem_take(void *sem, uint32_t timeout)
{
    return ((pdTRUE == xSemaphoreTake((SemaphoreHandle_t)sem, pdMS_TO_TICKS(timeout))) ? STATE_SUCCESS : STATE_FAILED);
}

void core_sysdep_sem_release(void *sem)
{
    xSemaphoreGive((SemaphoreHandle_t)sem);
}

void core_sysdep_sem_deinit(void **sem)
{
    vSemaphoreDelete((SemaphoreHandle_t)*sem);
}


osThreadId_t osThreadNew (osThreadFunc_t func, void *argument, const osThreadAttr_t *attr) 
{
	const char *name;
	uint32_t stack;
	TaskHandle_t hTask;
	UBaseType_t prio;
	int32_t mem;
	BaseType_t xReturn = pdPASS;
	
	hTask = NULL;

	if (!IS_IRQ() && (func != NULL)) 
	{
		stack = configMINIMAL_STACK_SIZE;
		prio  = (UBaseType_t)osPriorityNormal;

		name = NULL;
		mem  = -1;

		if (attr != NULL) 
		{
			if (attr->name != NULL) 
			{
				name = attr->name;
			}
			if (attr->priority != osPriorityNone) 
			{
				prio = (UBaseType_t)attr->priority;
			}

			if ((prio < osPriorityIdle) || (prio > osPriorityISR) || ((attr->attr_bits & osThreadJoinable) == osThreadJoinable)) 
			{
				return (NULL);
			}

			if (attr->stack_size > 0U) 
			{
				/* In FreeRTOS stack is not in bytes, but in sizeof(StackType_t) which is 4 on ARM ports.       */
				/* Stack size should be therefore 4 byte aligned in order to avoid division caused side effects */
				stack = attr->stack_size / sizeof(StackType_t);
			}

			if ((attr->cb_mem    != NULL) && (attr->cb_size    >= sizeof(StaticTask_t)) &&(attr->stack_mem != NULL) && (attr->stack_size >  0U)) 
			{
				mem = 1;
			}
			else 
			{
				if ((attr->cb_mem == NULL) && (attr->cb_size == 0U) && (attr->stack_mem == NULL)) 
				{
					mem = 0;
				}
			}
		}
		else 
		{
			mem = 0;
		}

		if (mem == 1) 
		{
			#if (configSUPPORT_STATIC_ALLOCATION == 1)
			hTask = xTaskCreateStatic ((TaskFunction_t)func, name, stack, argument, prio, (StackType_t  *)attr->stack_mem,
								  (StaticTask_t *)attr->cb_mem);
			#endif
		}
		else 
		{
			if (mem == 0) 
			{
				log_i("prio=%d stack=%d name=%s",prio,stack,name);
				#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
				xReturn = xTaskCreate((TaskFunction_t )(TaskFunction_t)func,  //任务入口函数
							(const char*    )name,//任务名字
							(uint16_t       )(uint16_t)stack,  //任务栈大小
							(void*          )argument,//任务入口函数参数
							(UBaseType_t    )prio, //任务的优先级
							(TaskHandle_t*  )&hTask);//任务控制块指针
				if(pdPASS != xReturn)
				{
					hTask = NULL;
					log_i("hTask thread create error!");
				}
				#endif
			}
		}
	}
	return ((osThreadId_t)hTask);
}

osStatus_t osThreadTerminate (osThreadId_t thread_id) 
{
	TaskHandle_t hTask = (TaskHandle_t)thread_id;
	osStatus_t stat;
	#ifndef USE_FreeRTOS_HEAP_1
	eTaskState tstate;

	if (IS_IRQ()) 
	{
		stat = osErrorISR;
	}
	else if (hTask == NULL) 
	{
		stat = osErrorParameter;
	}
	else 
	{
		tstate = eTaskGetState (hTask);

		if (tstate != eDeleted) 
		{
			stat = osOK;
			vTaskDelete (hTask);
		} 
		else 
		{
			stat = osErrorResource;
		}
	}
	#else
		stat = osError;
	#endif

	return (stat);
}


//#define FUNCTION_NAME(x) #x
#define STR_VALUE(arg) #arg
#define FUNCTION_NAME(name) STR_VALUE(name)
void *core_sysdep_thread_create(core_os_thread_fun_t func, void *arg, uint32_t stack_size, uint32_t priority) 
{
    osThreadId_t thread_id = NULL;
    const osThreadAttr_t ATClientTask_attributes = 
	{
        .name = FUNCTION_NAME(client_parser),
        .stack_size = stack_size,
        .priority =priority,
    };
   thread_id = osThreadNew(func, arg, &ATClientTask_attributes);
    return (void *)thread_id;
}

void core_sysdep_thread_destroy(void **thread_id) 
{
	if (thread_id != NULL) 
	{
		osThreadTerminate(*(osThreadId_t **)thread_id);
		*thread_id = NULL;
		//printf("Deleted g_htask\n");
	}
}

aiot_sysdep_portfile_t g_aiot_sysdep_portfile = {
    .core_sysdep_time = core_sysdep_time,
    .core_usart_send = core_usart_send,
    .core_sysdep_thread_create = core_sysdep_thread_create,
    .core_sysdep_thread_destroy = core_sysdep_thread_destroy,
    .core_sysdep_malloc = core_sysdep_malloc,
    .core_sysdep_free = core_sysdep_free,
    .core_sysdep_sleep = core_sysdep_sleep,
    .core_sysdep_mutex_init = core_sysdep_mutex_init,
    .core_sysdep_mutex_lock = core_sysdep_mutex_lock,
    .core_sysdep_mutex_unlock = core_sysdep_mutex_unlock,
    .core_sysdep_mutex_deinit = core_sysdep_mutex_deinit,
    .core_sysdep_sem_init = core_sysdep_sem_init,
    .core_sysdep_sem_take = core_sysdep_sem_take,
    .core_sysdep_sem_release = core_sysdep_sem_release,
    .core_sysdep_sem_deinit = core_sysdep_sem_deinit,
};

