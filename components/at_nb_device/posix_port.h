#ifndef _POSIX_PORT_H_
#define _POSIX_PORT_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define IS_IRQ()                  (__get_IPSR() != 0U)

// Thread attributes (attr_bits in \ref osThreadAttr_t).
#define osThreadDetached      0x00000000U ///< Thread created in detached mode (default)
#define osThreadJoinable      0x00000001U ///< Thread created in joinable mode


#ifndef TZ_MODULEID_T
#define TZ_MODULEID_T
/// \details Data type that identifies secure software modules called by a process.
typedef uint32_t TZ_ModuleId_t;
#endif

/// Priority values.
typedef enum {
  osPriorityNone          =  0,         ///< No priority (not initialized).
  osPriorityIdle          =  1,         ///< Reserved for Idle thread.
  osPriorityLow           =  8,         ///< Priority: low
  osPriorityLow1          =  8+1,       ///< Priority: low + 1
  osPriorityLow2          =  8+2,       ///< Priority: low + 2
  osPriorityLow3          =  8+3,       ///< Priority: low + 3
  osPriorityLow4          =  8+4,       ///< Priority: low + 4
  osPriorityLow5          =  8+5,       ///< Priority: low + 5
  osPriorityLow6          =  8+6,       ///< Priority: low + 6
  osPriorityLow7          =  8+7,       ///< Priority: low + 7
  osPriorityBelowNormal   = 16,         ///< Priority: below normal
  osPriorityBelowNormal1  = 16+1,       ///< Priority: below normal + 1
  osPriorityBelowNormal2  = 16+2,       ///< Priority: below normal + 2
  osPriorityBelowNormal3  = 16+3,       ///< Priority: below normal + 3
  osPriorityBelowNormal4  = 16+4,       ///< Priority: below normal + 4
  osPriorityBelowNormal5  = 16+5,       ///< Priority: below normal + 5
  osPriorityBelowNormal6  = 16+6,       ///< Priority: below normal + 6
  osPriorityBelowNormal7  = 16+7,       ///< Priority: below normal + 7
  osPriorityNormal        = 24,         ///< Priority: normal
  osPriorityNormal1       = 24+1,       ///< Priority: normal + 1
  osPriorityNormal2       = 24+2,       ///< Priority: normal + 2
  osPriorityNormal3       = 24+3,       ///< Priority: normal + 3
  osPriorityNormal4       = 24+4,       ///< Priority: normal + 4
  osPriorityNormal5       = 24+5,       ///< Priority: normal + 5
  osPriorityNormal6       = 24+6,       ///< Priority: normal + 6
  osPriorityNormal7       = 24+7,       ///< Priority: normal + 7
  osPriorityAboveNormal   = 32,         ///< Priority: above normal
  osPriorityAboveNormal1  = 32+1,       ///< Priority: above normal + 1
  osPriorityAboveNormal2  = 32+2,       ///< Priority: above normal + 2
  osPriorityAboveNormal3  = 32+3,       ///< Priority: above normal + 3
  osPriorityAboveNormal4  = 32+4,       ///< Priority: above normal + 4
  osPriorityAboveNormal5  = 32+5,       ///< Priority: above normal + 5
  osPriorityAboveNormal6  = 32+6,       ///< Priority: above normal + 6
  osPriorityAboveNormal7  = 32+7,       ///< Priority: above normal + 7
  osPriorityHigh          = 40,         ///< Priority: high
  osPriorityHigh1         = 40+1,       ///< Priority: high + 1
  osPriorityHigh2         = 40+2,       ///< Priority: high + 2
  osPriorityHigh3         = 40+3,       ///< Priority: high + 3
  osPriorityHigh4         = 40+4,       ///< Priority: high + 4
  osPriorityHigh5         = 40+5,       ///< Priority: high + 5
  osPriorityHigh6         = 40+6,       ///< Priority: high + 6
  osPriorityHigh7         = 40+7,       ///< Priority: high + 7
  osPriorityRealtime      = 48,         ///< Priority: realtime
  osPriorityRealtime1     = 48+1,       ///< Priority: realtime + 1
  osPriorityRealtime2     = 48+2,       ///< Priority: realtime + 2
  osPriorityRealtime3     = 48+3,       ///< Priority: realtime + 3
  osPriorityRealtime4     = 48+4,       ///< Priority: realtime + 4
  osPriorityRealtime5     = 48+5,       ///< Priority: realtime + 5
  osPriorityRealtime6     = 48+6,       ///< Priority: realtime + 6
  osPriorityRealtime7     = 48+7,       ///< Priority: realtime + 7
  osPriorityISR           = 56,         ///< Reserved for ISR deferred thread.
  osPriorityError         = -1,         ///< System cannot determine priority or illegal priority.
  osPriorityReserved      = 0x7FFFFFFF  ///< Prevents enum down-size compiler optimization.
} osPriority_t;

/// Status code values returned by CMSIS-RTOS functions.
typedef enum {
  osOK                      =  0,         ///< Operation completed successfully.
  osError                   = -1,         ///< Unspecified RTOS error: run-time error but no other error message fits.
  osErrorTimeout            = -2,         ///< Operation not completed within the timeout period.
  osErrorResource           = -3,         ///< Resource not available.
  osErrorParameter          = -4,         ///< Parameter error.
  osErrorNoMemory           = -5,         ///< System is out of memory: it was impossible to allocate or reserve memory for the operation.
  osErrorISR                = -6,         ///< Not allowed in ISR context: the function cannot be called from interrupt service routines.
  osStatusReserved          = 0x7FFFFFFF  ///< Prevents enum down-size compiler optimization.
} osStatus_t;


/// Attributes structure for thread.
typedef struct {
  const char                   *name;   ///< name of the thread
  uint32_t                 attr_bits;   ///< attribute bits
  void                      *cb_mem;    ///< memory for control block
  uint32_t                   cb_size;   ///< size of provided memory for control block
  void                   *stack_mem;    ///< memory for stack
  uint32_t                stack_size;   ///< size of stack
  osPriority_t              priority;   ///< initial thread priority (default: osPriorityNormal)
  TZ_ModuleId_t            tz_module;   ///< TrustZone module identifier
  uint32_t                  reserved;   ///< reserved (must be 0)
} osThreadAttr_t;

/// \details Thread ID identifies the thread.
typedef void *osThreadId_t;

/// Entry point of a thread.
typedef void (*osThreadFunc_t) (void *argument);


typedef void (*core_os_thread_fun_t) (void *arg);
/**
 * @brief 用以向SDK描述其运行硬件平台的资源如何使用的方法结构体
 */
typedef struct {
    /**
     * @brief 获取当前的时间戳，SDK用于差值计算
     */
    uint64_t (*core_sysdep_time)(void);

    /**
     * @brief 向usart串口发送数据
     */
    int32_t (*core_usart_send)(void *data, uint32_t size, uint32_t timeout);

    /**
     * @brief 申请内存
     */
    void    *(*core_sysdep_malloc)(uint32_t size, char *name);
    /**
     * @brief 释放内存
     */
    void (*core_sysdep_free)(void *ptr);
    /**
     * @brief 睡眠指定的毫秒数
     */
    void (*core_sysdep_sleep)(uint64_t time_ms);
    /**
     * @brief 创建线程
     */
    void *(*core_sysdep_thread_create)(core_os_thread_fun_t func, void *arg, uint32_t stack_size, uint32_t priority);
    /**
     * @brief 删除线程
     */
    void (*core_sysdep_thread_destroy)(void **thread_id);
    /**
     * @brief 创建互斥锁
     */
    void    *(*core_sysdep_mutex_init)(void);
    /**
     * @brief 申请互斥锁
     */
    void (*core_sysdep_mutex_lock)(void *mutex);
    /**
     * @brief 释放互斥锁
     */
    void (*core_sysdep_mutex_unlock)(void *mutex);
    /**
     * @brief 销毁互斥锁
     */
    void (*core_sysdep_mutex_deinit)(void **mutex);
    /**
     * @brief 创建二值信号量
     */
    void    *(*core_sysdep_sem_init)(void);
    /**
     * @brief 申请二值信号量
     */
    int32_t (*core_sysdep_sem_take)(void *sem, uint32_t timeout);
    /**
     * @brief 释放二值信号量
     */
    void (*core_sysdep_sem_release)(void *sem);
    /**
     * @brief 销毁二值信号量
     */
    void (*core_sysdep_sem_deinit)(void **sem);


} aiot_sysdep_portfile_t;

void aiot_sysdep_set_portfile(aiot_sysdep_portfile_t *portfile);
aiot_sysdep_portfile_t *aiot_sysdep_get_portfile(void);

int32_t at_hal_init(void);

#if defined(__cplusplus)
}
#endif

#endif

