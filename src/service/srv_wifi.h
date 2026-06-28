/*
 * 文件名称: srv_wifi.h
 * 功能说明: WiFi 服务接口
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: WiFi 连接管理 + 状态机 + 自动重连
 *           与 LED 状态灯联动
 *           提供命令行和菜单集成
 */

#ifndef _SRV_WIFI_H_
#define _SRV_WIFI_H_

#include "common/err_code.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_STATE_IDLE = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_RETRY_WAIT,
} wifi_state_t;

app_err_t srv_wifi_init(void);

app_err_t srv_wifi_connect(const char *ssid, const char *password);
void      srv_wifi_disconnect(void);
void      srv_wifi_reconnect(void);

wifi_state_t srv_wifi_get_state(void);
const char*  srv_wifi_get_state_str(void);
const char*  srv_wifi_get_ssid(void);
const char*  srv_wifi_get_ip(void);
int8_t       srv_wifi_get_rssi(void);
uint32_t     srv_wifi_get_retry_count(void);

void srv_wifi_task_process(void);

#ifdef __cplusplus
}
#endif

#endif
