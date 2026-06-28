/*
 * 文件名称: srv_wifi.cpp
 * 功能说明: WiFi 服务实现
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 状态机管理 + 事件回调 + 指数退避重连
 *           与 LED 状态灯联动
 *           注册命令行和菜单项
 */

#include "srv_wifi.h"
#include "common/config.h"
#include "common/log.h"
#include "common/service_reg.h"
#include "service/srv_led.h"
#include "service/srv_cli.h"
#include "service/srv_sys_info.h"
#include <WiFi.h>
#include <Menu.h>
#include <string.h>

static wifi_state_t s_state = WIFI_STATE_IDLE;
static char s_ssid[32 + 1] = {0};
static char s_password[64 + 1] = {0};
static char s_ip_str[16] = {0};
static uint32_t s_retry_count = 0;
static uint32_t s_retry_delay_ms = WIFI_RETRY_BASE_DELAY_MS;
static uint32_t s_wait_start_ms = 0;
static uint32_t s_connect_start_ms = 0;
static bool s_initialized = false;

static void _on_wifi_event(WiFiEvent_t event)
{
    switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        LOG_I("wifi connected: %s", s_ssid);
        break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
        IPAddress ip = WiFi.localIP();
        snprintf(s_ip_str, sizeof(s_ip_str), "%d.%d.%d.%d",
                 ip[0], ip[1], ip[2], ip[3]);
        LOG_I("wifi got ip: %s", s_ip_str);
        s_state = WIFI_STATE_CONNECTED;
        s_retry_count = 0;
        s_retry_delay_ms = WIFI_RETRY_BASE_DELAY_MS;
        srv_led_clear_state(LED_STATE_CONNECTING);
        break;
    }

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        LOG_I("wifi disconnected");
        if (s_state == WIFI_STATE_CONNECTED || s_state == WIFI_STATE_CONNECTING) {
            s_retry_count++;
            s_state = WIFI_STATE_RETRY_WAIT;
            s_wait_start_ms = millis();
            srv_led_set_state(LED_STATE_CONNECTING);
            s_ip_str[0] = '\0';
            LOG_I("wifi retry #%lu, delay=%lu ms",
                  (unsigned long)s_retry_count,
                  (unsigned long)s_retry_delay_ms);
        }
        break;

    default:
        break;
    }
}

app_err_t srv_wifi_init(void)
{
    if (s_initialized) {
        return APP_OK;
    }

    WiFi.mode(WIFI_STA);
    WiFi.onEvent(_on_wifi_event);

    s_initialized = true;
    s_state = WIFI_STATE_IDLE;

    LOG_I("wifi service init ok");
    return APP_OK;
}

app_err_t srv_wifi_connect(const char *ssid, const char *password)
{
    if (!ssid || !password) {
        return APP_INVALID_PARAM;
    }

    strncpy(s_ssid, ssid, sizeof(s_ssid) - 1);
    s_ssid[sizeof(s_ssid) - 1] = '\0';
    strncpy(s_password, password, sizeof(s_password) - 1);
    s_password[sizeof(s_password) - 1] = '\0';

    s_retry_count = 0;
    s_retry_delay_ms = WIFI_RETRY_BASE_DELAY_MS;
    s_ip_str[0] = '\0';

    LOG_I("wifi connecting: ssid=%s", s_ssid);
    s_state = WIFI_STATE_CONNECTING;
    s_connect_start_ms = millis();
    srv_led_set_state(LED_STATE_CONNECTING);

    WiFi.begin(s_ssid, s_password);

    return APP_OK;
}

void srv_wifi_disconnect(void)
{
    LOG_I("wifi disconnect");
    WiFi.disconnect(true);
    s_state = WIFI_STATE_IDLE;
    s_retry_count = 0;
    s_retry_delay_ms = WIFI_RETRY_BASE_DELAY_MS;
    s_ip_str[0] = '\0';
    srv_led_clear_state(LED_STATE_CONNECTING);
}

void srv_wifi_reconnect(void)
{
    if (s_ssid[0] == '\0') {
        LOG_I("wifi reconnect: no ssid configured");
        return;
    }

    srv_wifi_disconnect();
    delay(100);
    srv_wifi_connect(s_ssid, s_password);
}

wifi_state_t srv_wifi_get_state(void)
{
    return s_state;
}

const char* srv_wifi_get_state_str(void)
{
    switch (s_state) {
    case WIFI_STATE_IDLE:        return "IDLE";
    case WIFI_STATE_CONNECTING:  return "CONNECTING";
    case WIFI_STATE_CONNECTED:   return "CONNECTED";
    case WIFI_STATE_RETRY_WAIT:  return "RETRY_WAIT";
    default:                      return "UNKNOWN";
    }
}

const char* srv_wifi_get_ssid(void)
{
    return s_ssid;
}

const char* srv_wifi_get_ip(void)
{
    return s_ip_str;
}

int8_t srv_wifi_get_rssi(void)
{
    if (s_state != WIFI_STATE_CONNECTED) {
        return 0;
    }
    return WiFi.RSSI();
}

uint32_t srv_wifi_get_retry_count(void)
{
    return s_retry_count;
}

void srv_wifi_task_process(void)
{
    if (!s_initialized) {
        return;
    }

    if (s_state == WIFI_STATE_CONNECTING) {
        uint32_t elapsed = millis() - s_connect_start_ms;
        if (elapsed > WIFI_CONNECT_TIMEOUT_MS) {
            LOG_I("wifi connect timeout");
            WiFi.disconnect();
            s_retry_count++;
            s_state = WIFI_STATE_RETRY_WAIT;
            s_wait_start_ms = millis();
        }
        return;
    }

    if (s_state == WIFI_STATE_RETRY_WAIT) {
        uint32_t elapsed = millis() - s_wait_start_ms;
        if (elapsed >= s_retry_delay_ms) {
            s_retry_delay_ms *= 2;
            if (s_retry_delay_ms > WIFI_RETRY_MAX_DELAY_MS) {
                s_retry_delay_ms = WIFI_RETRY_MAX_DELAY_MS;
            }

            LOG_I("wifi retry #%lu, delay=%lu ms",
                  (unsigned long)s_retry_count,
                  (unsigned long)s_retry_delay_ms);

            s_state = WIFI_STATE_CONNECTING;
            s_connect_start_ms = millis();
            WiFi.begin(s_ssid, s_password);
        }
        return;
    }
}

/* ===== 命令行命令 ===== */

static void cmd_wifi_status(const char *args)
{
    (void)args;
    srv_cli_print("--- WiFi Status ---\r\n");
    srv_cli_print("State  : %s\r\n", srv_wifi_get_state_str());
    srv_cli_print("SSID   : %s\r\n", srv_wifi_get_ssid());
    srv_cli_print("IP     : %s\r\n", srv_wifi_get_ip());
    if (srv_wifi_get_state() == WIFI_STATE_CONNECTED) {
        srv_cli_print("RSSI   : %d dBm\r\n", srv_wifi_get_rssi());
    }
    srv_cli_print("Retry  : %lu\r\n", (unsigned long)srv_wifi_get_retry_count());
}

static void cmd_wifi_connect(const char *args)
{
    char ssid[33], pwd[65];
    int n = sscanf(args, "%32s %64s", ssid, pwd);
    if (n < 2) {
        srv_cli_print("usage: wifi_connect <ssid> <password>\r\n");
        return;
    }
    srv_wifi_connect(ssid, pwd);
    srv_cli_print("connecting to %s...\r\n", ssid);
}

static void cmd_wifi_disconnect(const char *args)
{
    (void)args;
    srv_wifi_disconnect();
    srv_cli_print("wifi disconnected\r\n");
}

static void cmd_wifi_reconnect(const char *args)
{
    (void)args;
    srv_wifi_reconnect();
    srv_cli_print("wifi reconnecting...\r\n");
}

/* ===== 菜单 ===== */

static void _menu_wifi_status(void)
{
    cmd_wifi_status(NULL);
}

static void _menu_wifi_reconnect(void)
{
    cmd_wifi_reconnect(NULL);
}

static void _menu_wifi_disconnect(void)
{
    cmd_wifi_disconnect(NULL);
}

static const menu_def_t s_wifi_menu[] = {
    { "查看WiFi状态",   MENU_ACTION, { .action = _menu_wifi_status } },
    { "重新连接",       MENU_ACTION, { .action = _menu_wifi_reconnect } },
    { "断开连接",       MENU_ACTION, { .action = _menu_wifi_disconnect } },
    { NULL, MENU_ACTION, {0} }
};

/* ===== 系统信息页 ===== */

static void _show_wifi_info(void)
{
    srv_cli_print("WiFi: %s", srv_wifi_get_state_str());
    if (srv_wifi_get_state() == WIFI_STATE_CONNECTED) {
        srv_cli_print(" %s", srv_wifi_get_ip());
    }
    if (srv_wifi_get_retry_count() > 0) {
        srv_cli_print(" (retry:%lu)", (unsigned long)srv_wifi_get_retry_count());
    }
    srv_cli_print("\r\n");
}

static app_err_t srv_wifi_post_init(void)
{
    srv_cli_register_cmd("wifi",        "查看WiFi状态",    cmd_wifi_status);
    srv_cli_register_cmd("wifi_connect","连接WiFi <ssid> <pwd>", cmd_wifi_connect);
    srv_cli_register_cmd("wifi_disconnect","断开WiFi",     cmd_wifi_disconnect);
    srv_cli_register_cmd("wifi_reconnect","重新连接WiFi",  cmd_wifi_reconnect);

    menu_register("WiFi 管理", s_wifi_menu);

    srv_sys_info_register_page("WiFi 信息", _show_wifi_info);

    return APP_OK;
}

static app_err_t srv_wifi_service_init(void)
{
    srv_wifi_init();
    srv_wifi_post_init();

    if (strlen(WIFI_SSID) > 0 && strlen(WIFI_PASSWORD) > 0) {
        srv_wifi_connect(WIFI_SSID, WIFI_PASSWORD);
    }

    return APP_OK;
}

SERVICE_REGISTER("wifi", srv_wifi_service_init, 60);
