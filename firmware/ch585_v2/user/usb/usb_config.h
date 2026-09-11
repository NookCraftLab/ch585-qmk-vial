/*
 * USB配置文件
 * 通过宏定义选择使用哪个USB口
 */

#ifndef __USB_CONFIG_H
#define __USB_CONFIG_H

// USB口配置选项
#define USB_PORT_HIGH_SPEED    1  // 仅高速口（右边USB2）
#define USB_PORT_FULL_SPEED    2  // 仅全速口（左边USB1）
#define USB_PORT_DUAL_BOTH     4  // 双口同时工作（不切换，插哪个都能用）

// 当前USB口配置（修改这里即可切换）
#define USB_PORT_CONFIG  USB_PORT_DUAL_BOTH

#endif
