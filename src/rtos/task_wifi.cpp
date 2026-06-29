/*
 * 文件名称: task_wifi.cpp
 * 功能说明: WiFi 任务
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 轻量级任务，周期性调用 srv_wifi_task_process()
 *           处理超时检测、指数退避重连等逻辑
 */

#include "task_def.h"
#include "common/config.h"
#include "common/log.h"
#include "service/srv_wifi.h"
#include "service/srv_webconfig.h"

static void task_wifi_entry(void *pvParameters)
{
    (void)pvParameters;

    LOG_I("wifi task start");

    for (;;) {
        srv_wifi_task_process();
        srv_webconfig_task_process();
        vTaskDelay(pdMS_TO_TICKS(TASK_WIFI_PERIOD_MS));
    }
}

app_err_t task_wifi_create(void)
{
    app_err_t ret = app_task_create(
        TASK_WIFI_NAME,
        task_wifi_entry,
        TASK_WIFI_STACK_SIZE,
        TASK_WIFI_PRIORITY,
        NULL
    );

    if (ret != APP_OK) {
        LOG_E("wifi task create failed");
        return ret;
    }

    LOG_I("wifi task created");
    return APP_OK;
}
