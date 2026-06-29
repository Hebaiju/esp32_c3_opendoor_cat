/*
 * 文件名称: srv_webconfig.h
 * 功能说明: Web 配置服务接口
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: HTTP服务器 + DNS劫持(Captive Portal) + WiFi配置页
 *           手机连上AP后自动弹出配置页面
 */

#ifndef _SRV_WEBCONFIG_H_
#define _SRV_WEBCONFIG_H_

#include "common/err_code.h"

#ifdef __cplusplus
extern "C" {
#endif

app_err_t srv_webconfig_init(void);
void      srv_webconfig_task_process(void);

#ifdef __cplusplus
}
#endif

#endif
