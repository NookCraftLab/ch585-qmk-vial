/*
 * CH585 EVB 平台配置
 */

#pragma once

/* 无串口排查方案：DEBUG 保持关闭，启动阶段由板载 LED(PB15) 闪烁报障，
 * 不依赖 USB-TTL 模块。需要串口日志时再改为 Debug_UART1(TX=PA9)。 */
// #define DEBUG                Debug_UART1
#define DEBUG_BAUDRATE       460800
#define DCDC_ENABLE          1
#define FREQ_SYS             62400000
#define LSE_ENABLE           0
#define BLE_SLOT_NUM         4
#define HSE_LOAD_CAPACITANCE 20 // in pF unit
#define LSE_LOAD_CAPACITANCE 19 // in pF unit
