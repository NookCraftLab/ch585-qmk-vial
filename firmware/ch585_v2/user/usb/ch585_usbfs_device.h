/*
 * CH585 全速USB设备驱动（USB1，左边type C）
 * 标准键盘+鼠标复合HID设备
 */

#ifndef __CH585_USBFS_DEVICE_H
#define __CH585_USBFS_DEVICE_H

#include "CH58x_common.h"

// 全速USB设备初始化
void USBFS_Device_Init(void);

// 全速USB设备枚举状态
extern volatile uint8_t USBFS_DevEnumStatus;

// 发送键盘报告（8字节）
void USBFS_SendKeyboardReport(uint8_t *report);

// 发送鼠标报告（4字节）
void USBFS_SendMouseReport(uint8_t *report);

// 发送Raw HID数据
uint8_t USBFS_RawHID_Send(uint8_t *data, uint8_t len);

#endif
