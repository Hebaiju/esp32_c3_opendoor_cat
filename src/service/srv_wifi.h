/*
 * 文件名称: srv_wifi.h
 * 功能说明: WiFi 服务接口
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v2.0
 * 说    明: WiFi 连接管理 + AP+STA双模式 + NVS存储 + 扫描
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

#define WIFI_MAX_SSID_LEN    32
#define WIFI_MAX_PASS_LEN    64
#define WIFI_SCAN_MAX_NETS   30

typedef enum {
    WIFI_STATE_IDLE = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_RETRY_WAIT,
    WIFI_STATE_SCANNING,
} wifi_state_t;

typedef struct {
    char ssid[WIFI_MAX_SSID_LEN + 1];
    int8_t rssi;
    uint8_t enc_type;
    uint8_t channel;
} wifi_scan_item_t;

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

const char*  srv_wifi_get_ap_ssid(void);
const char*  srv_wifi_get_ap_ip(void);

app_err_t    srv_wifi_scan_start(void);
bool         srv_wifi_scan_done(void);
uint8_t      srv_wifi_scan_count(void);
bool         srv_wifi_scan_get_item(uint8_t idx, wifi_scan_item_t *item);
void         srv_wifi_scan_clear(void);

uint8_t      srv_wifi_saved_count(void);
bool         srv_wifi_saved_get(uint8_t idx, char *ssid, char *password);
app_err_t    srv_wifi_saved_add(const char *ssid, const char *password);
void         srv_wifi_saved_remove(uint8_t idx);
void         srv_wifi_saved_clear(void);

void srv_wifi_task_process(void);

#ifdef __cplusplus
}
#endif

#endif
