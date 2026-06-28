/*
 * 文件名称: task_led.cpp
 * 功能说明: LED 刷新任务
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 周期性调用 drv_ws2812_refresh() 输出灯效到硬件
 */

#include "task_def.h"
#include "common/config.h"
#include "common/log.h"
#include "driver/drv_ws2812.h"

static void task_led_entry(void *pvParameters)
{
    (void)pvParameters;

    LOG_I("led task start");

    for (;;) {
        drv_ws2812_refresh();
        vTaskDelay(pdMS_TO_TICKS(TASK_LED_PERIOD_MS));
    }
}

app_err_t task_led_create(void)
{
    app_err_t ret = app_task_create(
        TASK_LED_NAME,
        task_led_entry,
        TASK_LED_STACK_SIZE,
        TASK_LED_PRIORITY,
        NULL
    );

    if (ret != APP_OK) {
        LOG_E("led task create failed");
        return ret;
    }

    LOG_I("led task created");
    return APP_OK;
}
