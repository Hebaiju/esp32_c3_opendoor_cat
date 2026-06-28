/*
 * 文件名称: service_reg.h
 * 功能说明: 服务注册机制（精简版）
 * 说    明: 各服务通过 SERVICE_REGISTER 宏注册初始化函数，
 *           系统启动时 service_init_all() 按优先级升序调用。
 */

#ifndef _SERVICE_REG_H_
#define _SERVICE_REG_H_

#include "common/err_code.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef app_err_t (*service_init_fn_t)(void);

typedef struct {
    const char       *name;
    service_init_fn_t init;
    uint8_t           priority;
} service_entry_t;

void service_register(const char *name, service_init_fn_t init, uint8_t priority);
app_err_t service_init_all(void);

#ifdef __cplusplus
}

#define SERVICE_REGISTER(name, init_fn, prio) \
    static void _svc_ctor_##init_fn(void) __attribute__((constructor)); \
    static void _svc_ctor_##init_fn(void) { service_register(name, init_fn, prio); }

#endif

#endif
