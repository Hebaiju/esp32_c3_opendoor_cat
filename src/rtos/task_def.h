/*
 * 文件名称: task_def.h
 * 功能说明: RTOS 任务声明与创建辅助
 */

#ifndef _TASK_DEF_H_
#define _TASK_DEF_H_

#include "common/err_code.h"
#include "service/srv_sys_info.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

app_err_t task_uart_create(void);

#ifdef __cplusplus
}

static inline app_err_t app_task_create(const char *name,
                                        TaskFunction_t entry,
                                        uint32_t stack_size,
                                        UBaseType_t priority,
                                        void *param)
{
    TaskHandle_t handle = NULL;
    if (xTaskCreate(entry, name, stack_size, param, priority, &handle) != pdPASS || !handle) {
        return APP_FAIL;
    }
    srv_sys_info_register_task(name, handle, stack_size / sizeof(StackType_t));
    return APP_OK;
}

#endif

#define APP_TASK_CREATE(name, entry, stack_size, priority, param) \
    (void)app_task_create(name, entry, stack_size, priority, param)

#endif
