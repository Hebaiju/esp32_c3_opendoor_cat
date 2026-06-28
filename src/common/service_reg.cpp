/*
 * 文件名称: service_reg.cpp
 * 功能说明: 服务注册机制实现（精简版）
 */

#include "common/service_reg.h"
#include "common/log.h"

#define SERVICE_MAX_COUNT 16

static service_entry_t s_table[SERVICE_MAX_COUNT];
static int s_count = 0;

void service_register(const char *name, service_init_fn_t init, uint8_t priority)
{
    if (!name || !init || s_count >= SERVICE_MAX_COUNT) return;

    int i = s_count;
    while (i > 0 && s_table[i - 1].priority > priority) {
        s_table[i] = s_table[i - 1];
        i--;
    }
    s_table[i].name     = name;
    s_table[i].init     = init;
    s_table[i].priority = priority;
    s_count++;
}

app_err_t service_init_all(void)
{
    for (int i = 0; i < s_count; i++) {
        LOG_I("init: %s", s_table[i].name);
        if (s_table[i].init() != APP_OK) {
            LOG_E("init failed: %s", s_table[i].name);
            return APP_FAIL;
        }
    }
    return APP_OK;
}
