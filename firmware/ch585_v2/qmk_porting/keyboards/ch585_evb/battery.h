/*
 * battery.h — 电池管理接口（预留）
 *
 * 评估板限制：
 * - 没有电池接口、没有充电芯片、没有分压电路
 * - 无法实际检测电池电压和充电状态
 *
 * 本模块提供统一的电池管理接口，评估板上返回模拟值。
 * 后续最终键盘 PCB 上有电池硬件后，只需要修改 battery.c 内部实现，
 * 上层代码（power_management.c 等）不需要改动。
 *
 * 测试低电量提醒：
 * - 修改 BATTERY_SIMULATED_LEVEL 为低于 10 的值（如 5），重新编译
 * - 上电后 LED1 应该慢闪（低电量提醒）
 */

#ifndef __BATTERY_H
#define __BATTERY_H

#include <stdint.h>
#include <stdbool.h>

/* ========================================================================
 * 配置参数
 * ======================================================================== */

/* 评估板模拟电量百分比（0-100）
 * 评估板没有电池硬件，用这个模拟值测试低电量提醒
 * 测试低电量：改成 5（<10%），LED1 会慢闪
 * 正常使用：改成 100
 */
#define BATTERY_SIMULATED_LEVEL   100

/* 电池电压范围（用于后续真实硬件实现时的电压→百分比换算）*/
#define BATTERY_FULL_VOLTAGE_MV   4200   /* 满电电压 4.2V */
#define BATTERY_EMPTY_VOLTAGE_MV  3200   /* 空电电压 3.2V */

/* ========================================================================
 * API 函数
 * ======================================================================== */

/* 初始化电池管理 — 评估板上什么都不做（预留接口）*/
void battery_init(void);

/* 获取电池电量百分比（0-100）
 * 评估板返回 BATTERY_SIMULATED_LEVEL 模拟值
 * 后续真实硬件：读取 ADC 电压，换算成百分比
 */
uint8_t battery_get_level(void);

/* 获取电池电压（毫伏）
 * 评估板返回 0（无硬件）
 * 后续真实硬件：读取 ADC 分压电压，换算成电池电压
 */
uint16_t battery_get_voltage_mv(void);

/* 是否正在充电
 * 评估板返回 false（无充电硬件）
 * 后续真实硬件：读取充电芯片状态引脚
 */
bool battery_is_charging(void);

/* 是否电池供电（不是 USB 供电）
 * 评估板返回 false（评估板只能 USB 供电）
 * 后续真实硬件：检测 USB VBUS 是否存在
 */
bool battery_is_battery_powered(void);

#endif /* __BATTERY_H */
