#include <inttypes.h>
#include <stdio.h>
#include <sys_os.h>
#include <sys_log.h>
#include "project_pin_use_config.h"
//#define mainTEST_TASK_PRIORITY (tskIDLE_PRIORITY)
//#define mainTEST_DELAY         (400 / portTICK_RATE_MS)

#if ( configGENERATE_RUN_TIME_STATS == 1 )

volatile long long FreeRTOSRunTimeTicks;
/**
 * @brief  Timer2 counter comparison match interrupt callback function.
 * @param  None
 * @retval None
 */
static void TMR2_Cmp_IrqCallback(void)
{
    TMR2_ClearStatus(CM_TMR2_1, TMR2_FLAG_MATCH_CH_A);
    FreeRTOSRunTimeTicks++;
}

/**
 * @brief  Timer2 interrupt configuration.
 * @param  None
 * @retval None
 */
static void Tmr2IrqConfig(void)
{
    stc_irq_signin_config_t stcIrq;

    stcIrq.enIntSrc    = INT_SRC_TMR2_1_CMP_A;
    stcIrq.enIRQn      = INT050_IRQn;
    stcIrq.pfnCallback = &TMR2_Cmp_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrq);

    NVIC_ClearPendingIRQ(stcIrq.enIRQn);
    NVIC_SetPriority(stcIrq.enIRQn, DDL_IRQ_PRIO_03);
    NVIC_EnableIRQ(stcIrq.enIRQn);

    /* Enable the specified interrupts of Timer2. */
    TMR2_IntCmd(CM_TMR2_1, TMR2_INT_MATCH_CH_A, ENABLE);
}

/**
 * @brief  Timer2 configuration.
 * @param  None
 * @retval None
 */
static void Tmr2Config(void)
{
    stc_tmr2_init_t stcTmr2Init;

    /* 1. Enable Timer2 peripheral clock. */
    FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMR2_1, ENABLE);

    /* 2. Set a default initialization value for stcTmr2Init. */
    (void)TMR2_StructInit(&stcTmr2Init);

    /* 3. Modifies the initialization values depends on the application. */
    stcTmr2Init.u32ClockSrc     = TMR2_CLK_PCLK1;
    stcTmr2Init.u32ClockDiv     = TMR2_CLK_DIV1;
    stcTmr2Init.u32Func         = TMR2_FUNC_CMP;
	//Tmr2CompareValue = (Tmr2Period(us) * [Tmr2ClockSource(MHz) / Tmr2ClockDiv]) - 1.
    stcTmr2Init.u32CompareValue = (120*50UL - 1U);
    (void)TMR2_Init(CM_TMR2_1, TMR2_CH_A, &stcTmr2Init);

    /* 4. Configures IRQ if needed. */
    Tmr2IrqConfig();
}
/*
 * This function is called by FreeRTOS if configGENERATE_RUN_TIME_STATS
 * macro is defined as 1.
 */
void portWMSDK_CONFIGURE_TIMER_FOR_RUN_TIME_STATS()
{
	/* GPT clock now should have started at 1 uS period */
    /* MCU Peripheral registers write unprotected. */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                 LL_PERIPH_EFM | LL_PERIPH_SRAM);
    /* Configures Timer2. */
    Tmr2Config();
    /* MCU Peripheral registers write protected. */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                 LL_PERIPH_EFM | LL_PERIPH_SRAM);
	FreeRTOSRunTimeTicks=0;
	TMR2_Start(CM_TMR2_1, TMR2_CH_A);
}

/*
 * This function is called by FreeRTOS if configGENERATE_RUN_TIME_STATS
 * macro is defined as 1.
 */
unsigned long portWMSDK_GET_RUN_TIME_COUNTER_VALUE()
{
	return FreeRTOSRunTimeTicks;
}
#endif /* CONFIG_ENABLE_FREERTOS_RUNTIME_STATS_SUPPORT */

int os_timer_activate(os_timer_t *timer_t)
{
    int ret;
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    if (timer_t == NULL || (*timer_t) == NULL)
    {
        return SYS_E_INVAL;
    }

    /* Note:
     * XTimerStart, seconds argument is xBlockTime which means, the time,
     * in ticks, that the calling task should be held in the Blocked
     * state, until timer command succeeds.
     * We are giving as 0, to be consistent with threadx logic.
     */
    if (is_isr_context() != 0U)
    {
        /* This call is from Cortex-M3 handler mode, i.e. exception
         * context, hence use FromISR FreeRTOS APIs.
         */
        ret = xTimerStartFromISR(*timer_t, &xHigherPriorityTaskWoken);
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    }
    else
    {
        ret = xTimerStart(*timer_t, 0);
    }
    return ret == pdPASS ? SYS_OK : SYS_FAIL;
}

int os_timer_create(os_timer_t *timer_t,
                    const char *name,
                    os_timer_tick ticks,
                    void (*call_back)(os_timer_arg_t xTimer),
                    void *cb_arg,
                    os_timer_reload_t reload,
                    os_timer_activate_t activate)
{
    int auto_reload = (reload == OS_TIMER_ONE_SHOT) ? pdFALSE : pdTRUE;

    *timer_t = xTimerCreate(name, ticks, (UBaseType_t)auto_reload, cb_arg, call_back);
    if (*timer_t == NULL)
    {
        return SYS_FAIL;
    }

    if (activate == OS_TIMER_AUTO_ACTIVATE)
    {
        return os_timer_activate(timer_t);
    }

    return SYS_OK;
}

int os_queue_create(os_queue_t *qhandle, const char *name, int msgsize, os_queue_pool_t *poolname)
{
    /** The size of the pool divided by the max. message size gives the
     * max. number of items in the queue. */
    os_dprintf(" Queue Create: name = %s poolsize = %d msgsize = %d\r\n", name, poolname->size, msgsize);
    *qhandle = xQueueCreate((UBaseType_t)(poolname->size / msgsize), (UBaseType_t)msgsize);
    os_dprintf(" Queue Create: handle %p\r\n", *qhandle);
    if (*qhandle != NULL)
    {
        return SYS_OK;
    }
    return SYS_FAIL;
}

void (*g_os_tick_hooks[MAX_CUSTOM_HOOKS])(void) = {NULL};
void (*g_os_idle_hooks[MAX_CUSTOM_HOOKS])(void) = {NULL};

/** The FreeRTOS Tick hook function. */
void vApplicationTickHook(void)
{
    int i;

    for (i = 0; i < MAX_CUSTOM_HOOKS; i++)
    {
        if (g_os_tick_hooks[i] != NULL)
        {
            g_os_tick_hooks[i]();
        }
    }
}

void vApplicationIdleHook(void)
{
    int i;
    for (i = 0; i < MAX_CUSTOM_HOOKS; i++)
    {
        if (g_os_idle_hooks[i] != NULL)
        {
            g_os_idle_hooks[i]();
        }
    }
}

/* Freertos handles this internally? */
void os_thread_stackmark(char *name)
{
    /* Nothing to-do */
}

typedef struct event_wait_t
{
    /* parameter passed in the event get call */
    unsigned thread_mask;
    /* The 'get' thread will wait on this sem */
    os_semaphore_t sem;
    struct event_wait_t *next;
    struct event_wait_t *prev;
} event_wait_t;

typedef struct event_group_t
{
    /* Main event flags will be stored here */
    unsigned flags;
    /* This flag is used to indicate deletion
     * of event group */
    bool delete_group;
    /* to protect this structure and the waiting list */
    os_mutex_t mutex;
    event_wait_t *list;
} event_group_t;

static inline void os_event_flags_remove_node(event_wait_t *node, event_group_t *grp_ptr)
{
    if (node->prev != NULL)
    {
        node->prev->next = node->next;
    }
    if (node->next != NULL)
    {
        node->next->prev = node->prev;
    }
    /* If only one node is present */
    if (node->next == NULL && node->prev == NULL)
    {
        grp_ptr->list = NULL;
    }
    os_mem_free(node);
}

int os_event_flags_create(event_group_handle_t *hnd)
{
    int ret;
    event_group_t *eG = os_mem_alloc(sizeof(event_group_t));
    if (eG == NULL)
    {
        os_dprintf("ERROR:Mem allocation\r\n");
        return SYS_FAIL;
    }
    (void)memset(eG, 0x00, sizeof(event_group_t));
    ret = os_mutex_create(&eG->mutex, "event-flag", OS_MUTEX_INHERIT);
    if (ret != SYS_OK)
    {
        os_mem_free(eG);
        return SYS_FAIL;
    }
    *hnd = (event_group_handle_t)eG;
    return SYS_OK;
}

int os_event_flags_get(event_group_handle_t hnd,
                       unsigned requested_flags,
                       flag_rtrv_option_t option,
                       unsigned *actual_flags_ptr,
                       unsigned wait_option)
{
    bool wait_done = false;
    unsigned status;
    int ret;
    *actual_flags_ptr = 0;
    event_wait_t *tmp = NULL, *node = NULL;
    if (hnd == 0U)
    {
        os_dprintf("ERROR:Invalid event flag handle\r\n");
        return SYS_FAIL;
    }
    if (requested_flags == 0U)
    {
        os_dprintf("ERROR:Requested flag is zero\r\n");
        return SYS_FAIL;
    }
    if (actual_flags_ptr == NULL)
    {
        os_dprintf("ERROR:Flags pointer is NULL\r\n");
        return SYS_FAIL;
    }
    event_group_t *eG = (event_group_t *)hnd;

check_again:
    (void)os_mutex_get(&eG->mutex, OS_WAIT_FOREVER);

    if ((option == EF_AND) || (option == EF_AND_CLEAR))
    {
        if ((eG->flags & requested_flags) == requested_flags)
        {
            status = eG->flags;
        }
        else
        {
            status = 0;
        }
    }
    else if ((option == EF_OR) || (option == EF_OR_CLEAR))
    {
        status = (requested_flags & eG->flags);
    }
    else
    {
        os_dprintf("ERROR:Invalid event flag get option\r\n");
        (void)os_mutex_put(&eG->mutex);
        return SYS_FAIL;
    }
    /* Check flags */
    if (status != 0U)
    {
        *actual_flags_ptr = status;

        /* Clear the requested flags from main flag */
        if ((option == EF_AND_CLEAR) || (option == EF_OR_CLEAR))
        {
            eG->flags &= ~status;
        }

        if (wait_done)
        {
            /*Delete the created semaphore */
            (void)os_semaphore_delete(&tmp->sem);
            /* Remove ourselves from the list */
            os_event_flags_remove_node(tmp, eG);
        }
        (void)os_mutex_put(&eG->mutex);
        return SYS_OK;
    }
    else
    {
        if (wait_option != 0U)
        {
            if (wait_done == false)
            {
                /* Add to link list */
                /* Prepare a node to add in the link list */
                node = os_mem_alloc(sizeof(event_wait_t));
                if (node == NULL)
                {
                    os_dprintf("ERROR:memory alloc\r\n");
                    (void)os_mutex_put(&eG->mutex);
                    return SYS_FAIL;
                }
                (void)memset(node, 0x00, sizeof(event_wait_t));
                /* Set the requested flag in the node */
                node->thread_mask = requested_flags;
                /* Create a semaophore */
                ret = os_semaphore_create(&node->sem, "wait_thread");
                if (ret != 0)
                {
                    os_dprintf("ERROR:In creating semaphore\r\n");
                    os_mem_free(node);
                    (void)os_mutex_put(&eG->mutex);
                    return SYS_FAIL;
                }
                /* If there is no node present */
                if (eG->list == NULL)
                {
                    eG->list = node;
                    tmp      = eG->list;
                }
                else
                {
                    tmp = eG->list;
                    /* Move to last node */
                    while (tmp->next != NULL)
                    {
                        os_dprintf("waiting \r\n");
                        tmp = tmp->next;
                    }
                    tmp->next  = node;
                    node->prev = tmp;
                    tmp        = tmp->next;
                }
                /* Take semaphore first time */
                ret = os_semaphore_get(&tmp->sem, OS_WAIT_FOREVER);
                if (ret != SYS_OK)
                {
                    os_dprintf("ERROR:1st sem get error\r\n");
                    (void)os_mutex_put(&eG->mutex);
                    /*Delete the created semaphore */
                    (void)os_semaphore_delete(&tmp->sem);
                    /* Remove ourselves from the list */
                    os_event_flags_remove_node(tmp, eG);
                    return SYS_FAIL;
                }
            }
            (void)os_mutex_put(&eG->mutex);
            /* Second time get is performed for work-around purpose
            as in current implementation of semaphore 1st request
            is always satisfied */
            ret = os_semaphore_get(&tmp->sem, os_msec_to_ticks(wait_option));
            if (ret != SYS_OK)
            {
                (void)os_mutex_get(&eG->mutex, OS_WAIT_FOREVER);
                /*Delete the created semaphore */
                (void)os_semaphore_delete(&tmp->sem);
                /* Remove ourselves from the list */
                os_event_flags_remove_node(tmp, eG);
                (void)os_mutex_put(&eG->mutex);
                return EF_NO_EVENTS;
            }

            /* We have woken up */
            /* If the event group deletion has been requested */
            if (eG->delete_group)
            {
                (void)os_mutex_get(&eG->mutex, OS_WAIT_FOREVER);
                /*Delete the created semaphore */
                (void)os_semaphore_delete(&tmp->sem);
                /* Remove ourselves from the list */
                os_event_flags_remove_node(tmp, eG);
                (void)os_mutex_put(&eG->mutex);
                return SYS_FAIL;
            }
            wait_done = true;
            goto check_again;
        }
        else
        {
            (void)os_mutex_put(&eG->mutex);
            return EF_NO_EVENTS;
        }
    }
}

int os_event_flags_set(event_group_handle_t hnd, unsigned flags_to_set, flag_rtrv_option_t option)
{
    event_wait_t *tmp = NULL;

    if (hnd == 0U)
    {
        os_dprintf("ERROR:Invalid event flag handle\r\n");
        return SYS_FAIL;
    }
    if (flags_to_set == 0U)
    {
        os_dprintf("ERROR:Flags to be set is zero\r\n");
        return SYS_FAIL;
    }

    event_group_t *eG = (event_group_t *)hnd;

    (void)os_mutex_get(&eG->mutex, OS_WAIT_FOREVER);

    /* Set flags according to the set_option */
    if (option == EF_OR)
    {
        eG->flags |= flags_to_set;
    }
    else if (option == EF_AND)
    {
        eG->flags &= flags_to_set;
    }
    else
    {
        os_dprintf("ERROR:Invalid flag set option\r\n");
        (void)os_mutex_put(&eG->mutex);
        return SYS_FAIL;
    }

    if (eG->list != NULL)
    {
        tmp = eG->list;
        if (tmp->next == NULL)
        {
            if ((tmp->thread_mask & eG->flags) != 0U)
            {
                (void)os_semaphore_put(&tmp->sem);
            }
        }
        else
        {
            while (tmp != NULL)
            {
                if ((tmp->thread_mask & eG->flags) != 0U)
                {
                    (void)os_semaphore_put(&tmp->sem);
                }
                tmp = tmp->next;
            }
        }
    }
    (void)os_mutex_put(&eG->mutex);
    return SYS_OK;
}

int os_event_flags_delete(event_group_handle_t *hnd)
{
    int i, max_attempt = 3;
    event_wait_t *tmp = NULL;

    if (*hnd == 0U)
    {
        os_dprintf("ERROR:Invalid event flag handle\r\n");
        return SYS_FAIL;
    }
    event_group_t *eG = (event_group_t *)*hnd;

    (void)os_mutex_get(&eG->mutex, OS_WAIT_FOREVER);

    /* Set the flag to delete the group */
    eG->delete_group = 1;

    if (eG->list != NULL)
    {
        tmp = eG->list;
        if (tmp->next == NULL)
        {
            (void)os_semaphore_put(&tmp->sem);
        }
        else
        {
            while (tmp != NULL)
            {
                (void)os_semaphore_put(&tmp->sem);
                tmp = tmp->next;
            }
        }
    }
    (void)os_mutex_put(&eG->mutex);

    /* If still list is not empty then wait for 3 seconds */
    for (i = 0; i < max_attempt; i++)
    {
        (void)os_mutex_get(&eG->mutex, OS_WAIT_FOREVER);
        if (eG->list != NULL)
        {
            (void)os_mutex_put(&eG->mutex);
            os_thread_sleep(os_msec_to_ticks(1000));
        }
        else
        {
            (void)os_mutex_put(&eG->mutex);
            break;
        }
    }

    (void)os_mutex_get(&eG->mutex, OS_WAIT_FOREVER);
    if (eG->list != NULL)
    {
        (void)os_mutex_put(&eG->mutex);
        return SYS_FAIL;
    }
    else
    {
        (void)os_mutex_put(&eG->mutex);
    }

    /* Delete the event group */
    os_mem_free(eG);
    *hnd = 0;
    return SYS_OK;
}

int os_rwlock_create(os_rw_lock_t *plock, const char *mutex_name, const char *lock_name)
{
    return os_rwlock_create_with_cb(plock, mutex_name, lock_name, NULL);
}
int os_rwlock_create_with_cb(os_rw_lock_t *plock, const char *mutex_name, const char *lock_name, cb_fn r_fn)
{
    int ret = SYS_OK;
    ret     = os_mutex_create(&(plock->reader_mutex), mutex_name, OS_MUTEX_INHERIT);
    if (ret == SYS_FAIL)
    {
        return SYS_FAIL;
    }
    ret = os_semaphore_create(&(plock->rw_lock), lock_name);
    if (ret == SYS_FAIL)
    {
        return SYS_FAIL;
    }
    plock->reader_count = 0;
    plock->reader_cb    = r_fn;
    return ret;
}

int os_rwlock_read_lock(os_rw_lock_t *lock, unsigned int wait_time)
{
    int ret = SYS_OK;
    ret     = os_mutex_get(&(lock->reader_mutex), OS_WAIT_FOREVER);
    if (ret == SYS_FAIL)
    {
        return ret;
    }
    lock->reader_count++;
    if (lock->reader_count == 1U)
    {
        if (lock->reader_cb != NULL)
        {
            ret = lock->reader_cb(lock, wait_time);
            if (ret == SYS_FAIL)
            {
                lock->reader_count--;
                (void)os_mutex_put(&(lock->reader_mutex));
                return ret;
            }
        }
        else
        {
            /* If  1 it is the first reader and
             * if writer is not active, reader will get access
             * else reader will block.
             */
            ret = os_semaphore_get(&(lock->rw_lock), wait_time);
            if (ret == SYS_FAIL)
            {
                lock->reader_count--;
                (void)os_mutex_put(&(lock->reader_mutex));
                return ret;
            }
        }
    }
    (void)os_mutex_put(&(lock->reader_mutex));
    return ret;
}

int os_rwlock_read_unlock(os_rw_lock_t *lock)
{
    int ret = os_mutex_get(&(lock->reader_mutex), OS_WAIT_FOREVER);
    if (ret == SYS_FAIL)
    {
        return ret;
    }
    lock->reader_count--;
    if (lock->reader_count == 0U)
    {
        /* This is last reader so
         * give a chance to writer now
         */
        (void)os_semaphore_put(&(lock->rw_lock));
    }
    (void)os_mutex_put(&(lock->reader_mutex));
    return ret;
}

int os_rwlock_write_lock(os_rw_lock_t *lock, unsigned int wait_time)
{
    int ret = os_semaphore_get(&(lock->rw_lock), wait_time);
    return ret;
}

void os_rwlock_write_unlock(os_rw_lock_t *lock)
{
    (void)os_semaphore_put(&(lock->rw_lock));
}

void os_rwlock_delete(os_rw_lock_t *lock)
{
    lock->reader_cb = NULL;
    (void)os_semaphore_delete(&(lock->rw_lock));
    (void)os_mutex_delete(&(lock->reader_mutex));
    lock->reader_count = 0;
}

/* returns time in micro-secs since time began */
unsigned int os_get_timestamp(void)
{
    uint32_t nticks;
    uint32_t counter;

    vPortEnterCritical();
    nticks  = xTaskGetTickCount();
    counter = SysTick->VAL;

    /*
     * If this is called after SysTick counter
     * expired but before SysTick Handler got a
     * chance to run, then set the return value
     * to the start of next tick.
     */
    if ((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) != 0U)
    {
        nticks++;
        counter = CNTMAX;
    }

    vPortExitCritical();
    return ((CNTMAX - counter) / CPU_CLOCK_TICKSPERUSEC) + (nticks * USECSPERTICK);
}
