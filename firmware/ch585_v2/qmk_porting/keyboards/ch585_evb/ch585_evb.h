/*
 * CH585 EVB 板级定义 —— 2x4 矩阵（EX002 触摸板，8个触摸键）
 * 使用自定义矩阵扫描（matrix_scan_custom），触摸事件直接映射为矩阵位
 */

#pragma once

#include "quantum.h"
#include "extra_keycode.h"

// clang-format off

#define LAYOUT_all( \
    K00, K01, K02, K03, \
    K10, K11, K12, K13  \
) \
{ \
    { K00, K01, K02, K03 }, \
    { K10, K11, K12, K13 }  \
}
