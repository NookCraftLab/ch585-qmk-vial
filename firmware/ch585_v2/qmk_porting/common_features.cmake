# Copyright 2022 Huckies <https://github.com/Huckies>
# Copyright 2022 zhaqian <https://github.com/zhaqian12>
# Copyright 2023 Jactry Zeng
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# MOUSE_ENABLE
if(MOUSE_ENABLE)
    add_definitions(-DMOUSE_ENABLE -DMOUSEKEY_ENABLE)
    message(STATUS "MOUSE_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/mousekey.c"
        "${QMK_BASE_DIR}/quantum/pointing_device_drivers.c"
        "${QMK_BASE_DIR}/quantum/pointing_device.c"
    )
endif()

# EXTRAKEY_ENABLE锛氬濯掍綋閿紙闊抽噺/浜害/鎾斁鎺у埗绛夛級锛屽鐞嗛€昏緫鍦╝ction.c閲岋紝涓嶉渶棰濆婧愭枃浠?
if(EXTRAKEY_ENABLE)
    add_definitions(-DEXTRAKEY_ENABLE)
    message(STATUS "EXTRAKEY_ENABLE")
endif()

# NKRO_ENABLE
if(NKRO_ENABLE)
    add_definitions(-DNKRO_ENABLE)
    message(STATUS "NKRO_ENABLE")
    list(APPEND quantum_SOURCES
    )
endif()

# GRAVE_ESC_ENABLE
if(GRAVE_ESC_ENABLE)
    message(STATUS "GRAVE_ESC_ENABLE")
    add_definitions(-DGRAVE_ESC_ENABLE)
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_grave_esc.c"
    )
endif()

# DYNAMIC_MACRO_ENABLE
if(DYNAMIC_MACRO_ENABLE)
    message(STATUS "DYNAMIC_MACRO_ENABLE")
    add_definitions(-DDYNAMIC_MACRO_ENABLE)
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_dynamic_macro.c"
    )
endif()

# ENCODER_ENABLE
if(ENCODER_ENABLE)
    add_definitions(-DENCODER_ENABLE)
    message(STATUS "ENCODER_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/encoder.c"
    )

    # ENCODER_MAP_ENABLE
    if(ENCODER_MAP_ENABLE)
        add_definitions(-DENCODER_MAP_ENABLE)
        message(STATUS "ENCODER_MAP_ENABLE")
    endif()

    if(ENCODER_DRIVER STREQUAL "quadrature")
        add_definitions(-DENCODER_DRIVER_QUADRATURE)
        message(STATUS "ENCODER_DRIVER = ${ENCODER_DRIVER}")
        list(APPEND quantum_SOURCES
            "${QMK_BASE_DIR}/drivers/encoder/encoder_quadrature.c"
        )
    elseif(DEFINED ENCODER_DRIVER AND EXISTS "${ENCODER_DRIVER}")
        add_definitions(-DENCODER_DRIVER_CUSTOM)
        message(STATUS "CUSTOM_ENCODER_DRIVER: ${ENCODER_DRIVER}")
        list(APPEND QMK_PORTING_SOURCES
            "${ENCODER_DRIVER}"
        )
    else()
        message(FATAL_ERROR "Unsupported Encoder Driver!")
    endif()
endif()

# UCIS_ENABLE
if(UCIS_ENABLE)
    add_definitions(-DUCIS_ENABLE)
    set(UNICODE_COMMON_ENABLE ON CACHE BOOL "QMK" FORCE)
    message(STATUS "UCIS_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_ucis.c"
        "${QMK_BASE_DIR}/quantum/unicode/ucis.c"
    )
endif()

# UNICODEMAP_ENABLE
if(UNICODEMAP_ENABLE)
    add_definitions(-DUNICODEMAP_ENABLE)
    set(UNICODE_COMMON_ENABLE ON CACHE BOOL "QMK" FORCE)
    message(STATUS "UNICODEMAP_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_unicodemap.c"
        "${QMK_BASE_DIR}/quantum/unicode/unicodemap.c"
    )
endif()

# UNICODE_ENABLE
if(UNICODE_ENABLE)
    add_definitions(-DUNICODE_ENABLE)
    set(UNICODE_COMMON_ENABLE ON CACHE BOOL "QMK" FORCE)
    message(STATUS "UNICODE_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_unicode.c"
    )
endif()

# UNICODE_COMMON_ENABLE
if(UNICODE_COMMON_ENABLE)
    add_definitions(-DUNICODE_COMMON_ENABLE)
    message(STATUS "UNICODE_COMMON_ENABLE")
    include_directories(${QMK_BASE_DIR}/quantum/unicode)
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_unicode_common.c"
        "${QMK_BASE_DIR}/quantum/unicode/unicode.c"
        "${QMK_BASE_DIR}/quantum/unicode/utf8.c"
    )
endif()

# TAP_DANCE_ENABLE
if(TAP_DANCE_ENABLE)
    add_definitions(-DTAP_DANCE_ENABLE -DVIAL_TAP_DANCE_ENABLE)
    message(STATUS "TAP_DANCE_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_tap_dance.c"
    )
endif()

# COMBO_ENABLE
if(COMBO_ENABLE)
    add_definitions(-DCOMBO_ENABLE -DVIAL_COMBO_ENABLE)
    message(STATUS "COMBO_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_combo.c"
    )
endif()

# KEY_OVERRIDE_ENABLE
if(KEY_OVERRIDE_ENABLE)
    add_definitions(-DKEY_OVERRIDE_ENABLE -DVIAL_KEY_OVERRIDE_ENABLE)
    message(STATUS "KEY_OVERRIDE_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_key_override.c"
    )
endif()

# REPEAT_KEY_ENABLE
if(REPEAT_KEY_ENABLE)
    add_definitions(-DREPEAT_KEY_ENABLE -DVIAL_ALT_REPEAT_KEY_ENABLE)
    message(STATUS "REPEAT_KEY_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/repeat_key.c"
        "${QMK_BASE_DIR}/quantum/process_keycode/process_repeat_key.c"
    )
endif()

# ========================================================================
# QMK 鍘熺敓鍔熻兘锛堥粯璁ゅ叧闂紝闇€瑕佺殑璇濆湪 rules.cmake 閲屾妸瀵瑰簲寮€鍏虫敼鎴?ON锛?
# 杩欎簺鍔熻兘鐨勬簮鏂囦欢宸茶 GLOB_RECURSE 鑷姩鏀堕泦锛岃繖閲屽彧璐熻矗瀹氫箟瀹?
# ========================================================================

# ONESHOT_ENABLE锛氫竴娆℃€т慨楗伴敭锛屾寜涓€涓嬬矘鍒颁笅涓€閿嚜鍔ㄦ澗寮€
if(ONESHOT_ENABLE)
    add_definitions(-DONESHOT_ENABLE)
    message(STATUS "ONESHOT_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_oneshot.c"
    )
endif()

# LEADER_ENABLE锛氬紩瀵奸敭锛屾寜Leader+搴忓垪瑙﹀彂鍔熻兘
if(LEADER_ENABLE)
    add_definitions(-DLEADER_ENABLE)
    message(STATUS "LEADER_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_leader.c"
    )
endif()

# AUTO_SHIFT_ENABLE锛氶暱鎸夊瓧姣嶈嚜鍔ㄥぇ鍐?
if(AUTO_SHIFT_ENABLE)
    add_definitions(-DAUTO_SHIFT_ENABLE)
    message(STATUS "AUTO_SHIFT_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_auto_shift.c"
    )
endif()

# SPACECADET_ENABLE锛氬乏鍙砈hift鍗曞嚮鍙樻嫭鍙?
if(SPACECADET_ENABLE)
    add_definitions(-DSPACECADET_ENABLE)
    message(STATUS "SPACECADET_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_space_cadet.c"
    )
endif()

# CAPS_WORD_ENABLE锛氬弻鍑籗hift鍚庝笅涓€涓崟璇嶈嚜鍔ㄥぇ鍐?
if(CAPS_WORD_ENABLE)
    add_definitions(-DCAPS_WORD_ENABLE)
    message(STATUS "CAPS_WORD_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_caps_word.c"
    )
endif()

# SECURE_ENABLE锛氬畨鍏ㄩ攣瀹氾紝闇€瑙ｉ攣搴忓垪鎵嶈兘鐢?
if(SECURE_ENABLE)
    add_definitions(-DSECURE_ENABLE)
    message(STATUS "SECURE_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_secure.c"
    )
endif()

# ACHORDION_ENABLE锛氬拰寮﹂槻璇Е锛屼慨楗伴敭瑕佸彟涓€鍙墜鍏堟寜鎵嶇敓鏁?
if(ACHORDION_ENABLE)
    add_definitions(-DACHORDION_ENABLE)
    message(STATUS "ACHORDION_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_achordion.c"
    )
endif()

# SENTENCE_CASE_ENABLE锛氬彞棣栬嚜鍔ㄥぇ鍐?
if(SENTENCE_CASE_ENABLE)
    add_definitions(-DSENTENCE_CASE_ENABLE)
    message(STATUS "SENTENCE_CASE_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/process_keycode/process_sentence.c"
    )
endif()

# VIA_ENABLE
if(VIA_ENABLE)
    add_definitions(-DVIA_ENABLE -DRAW_ENABLE -DDYNAMIC_KEYMAP_ENABLE)
    set(EEPROM_ENABLE ON CACHE BOOL "QMK" FORCE)
    message(STATUS "VIA_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/dynamic_keymap.c"
        "${QMK_BASE_DIR}/quantum/via.c"
    )
endif()

# VIAL_ENABLE (Vial 鏄?VIA 鐨勮秴闆嗭紝闇€瑕?VIA_ENABLE 鍚屾椂寮€鍚?
if(VIAL_ENABLE)
    add_definitions(-DVIAL_ENABLE)
    message(STATUS "VIAL_ENABLE")

    # 鐢熸垚闅忔満 BUILD_ID锛堝拰瀹樻柟 QMK 涓€鑷达紝姣忔閰嶇疆 CMake 閮界敓鎴愭柊鍊硷級
    # 鏀瑰竷灞€鍚庨噸鏂伴厤缃?CMake锛孊UILD_ID 鍙樺寲浼氳嚜鍔ㄨЕ鍙?EEPROM 閲嶇疆
    execute_process(
        COMMAND python ${QMK_BASE_DIR}/util/build_id.py
        OUTPUT_VARIABLE BUILD_ID_HEADER
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # 浠庤緭鍑轰腑鎻愬彇 0xXXXXXXXX 鐨勫€?
    string(REGEX MATCH "0x[0-9A-Fa-f]+" BUILD_ID_VALUE "${BUILD_ID_HEADER}")
    add_definitions(-DBUILD_ID=${BUILD_ID_VALUE})
    message(STATUS "BUILD_ID: ${BUILD_ID_VALUE}")

    # 浠?vial.json 鐢熸垚 LZMA 鍘嬬缉鐨勫竷灞€瀹氫箟澶存枃浠讹紙閰嶇疆闃舵鐩存帴鐢熸垚锛?
    set(VIAL_GEN_DIR ${PROJECT_BINARY_DIR}/vial_generated)
    file(MAKE_DIRECTORY ${VIAL_GEN_DIR})
    set(VIAL_JSON ${KEYBOARD_ROOT}/vial.json)
    set(VIAL_HEADER ${VIAL_GEN_DIR}/vial_generated_keyboard_definition.h)

    execute_process(
        COMMAND python ${QMK_BASE_DIR}/util/vial_generate_definition.py ${VIAL_JSON} ${VIAL_HEADER}
        RESULT_VARIABLE VIAL_GEN_RESULT
        OUTPUT_VARIABLE VIAL_GEN_OUTPUT
        ERROR_VARIABLE VIAL_GEN_ERROR
    )
    if(NOT VIAL_GEN_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to generate Vial keyboard definition: ${VIAL_GEN_ERROR}")
    endif()
    message(STATUS "Generated Vial keyboard definition: ${VIAL_HEADER}")

    include_directories(${VIAL_GEN_DIR})
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/vial.c"
    )
endif()

# DIP_SWITCH_ENABLE
if(DIP_SWITCH_ENABLE)
    add_definitions(-DDIP_SWITCH_ENABLE)
    message(STATUS "DIP_SWITCH_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/dip_switch.c"
    )

    # DIP_SWITCH_MAP_ENABLE
    if(DIP_SWITCH_MAP_ENABLE)
        add_definitions(-DDIP_SWITCH_MAP_ENABLE)
        message(STATUS "DIP_SWITCH_MAP_ENABLE")
    endif()
endif()

# COMMAND_ENABLE
if(COMMAND_ENABLE)
    add_definitions(-DCOMMAND_ENABLE)
    set(EEPROM_ENABLE ON CACHE BOOL "QMK" FORCE)
    message(STATUS "COMMAND_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/command.c"
    )
endif()

# RGBLIGHT_ENABLE
if(RGBLIGHT_ENABLE)
    add_definitions(-DRGBLIGHT_ENABLE)
    set(EEPROM_ENABLE ON CACHE BOOL "QMK" FORCE)

    if(RGBLIGHT_DRIVER STREQUAL "WS2812")
        set(WS2812_REQUIRED ON CACHE BOOL "QMK" FORCE)
    else()
        message(FATAL_ERROR "Unsupported RGBLIGHT driver!")
    endif()

    message(STATUS "RGBLIGHT_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/rgblight/*.c"
        "${QMK_BASE_DIR}/quantum/color.c"
        "${QMK_BASE_DIR}/quantum/process_keycode/process_rgb.c"
    )
    set(CIE1931_CURVE ON CACHE BOOL "KB")
endif()

# RGB_MATRIX_ENABLE
if(RGB_MATRIX_ENABLE)
    add_definitions(-DRGB_MATRIX_ENABLE)
    set(EEPROM_ENABLE ON CACHE BOOL "QMK" FORCE)

    if(RGB_MATRIX_DRIVER STREQUAL "WS2812")
        set(WS2812_REQUIRED ON CACHE BOOL "QMK" FORCE)
    elseif(RGB_MATRIX_DRIVER STREQUAL "AW20216S")
        set(AW20216S_REQUIRED ON CACHE BOOL "QMK" FORCE)
    endif()

    message(STATUS "RGB_MATRIX_ENABLE")
    include_directories(${QMK_BASE_DIR}/lib/lib8tion)
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/rgb_matrix/*.c"
        "${QMK_BASE_DIR}/quantum/color.c"
        "${QMK_BASE_DIR}/quantum/process_keycode/process_rgb.c"
        "${QMK_BASE_DIR}/quantum/process_keycode/process_rgb_matrix.c"
        "${QMK_BASE_DIR}/lib/lib8tion/*.c"
    )
    set(CIE1931_CURVE ON CACHE BOOL "KB")
endif()

# AUXILIARY_RGB_ENABLE
if(AUXILIARY_RGB_ENABLE)
    if(NOT RGB_MATRIX_ENABLE)
        message(FATAL_ERROR "AUXILIARY_RGB_ENABLE requires RGB_MATRIX_ENABLE!")
    else()
        message(STATUS "AUXILIARY_RGB_ENABLE")
        add_definitions(-DRGB_MATRIX_CUSTOM_KB -DRGB_RAW_ENABLE)
        list(APPEND QMK_PORTING_SOURCES
            "${CMAKE_CURRENT_LIST_DIR}/protocol/auxiliary_rgb.c"
            "${CMAKE_CURRENT_LIST_DIR}/protocol/dynamic_lighting.c"
            "${CMAKE_CURRENT_LIST_DIR}/protocol/openrgb.c"
            "${CMAKE_CURRENT_LIST_DIR}/protocol/signalrgb.c"
        )
    endif()
endif()

# SPLIT_KEYBOARD
if(SPLIT_KEYBOARD)
    add_definitions(-DSPLIT_KEYBOARD)
    include_directories(${QMK_BASE_DIR}/quantum/split_common)
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/split_common/split_util.c"
    )

    if(NOT SPLIT_TRANSPORT STREQUAL "custom")
        message(FATAL_ERROR "Unsupported split keyboard driver!")
    else()
        list(APPEND QMK_PORTING_SOURCES
            "${CMAKE_CURRENT_LIST_DIR}/platforms/ch58x/split/transactions.c"
            "${CMAKE_CURRENT_LIST_DIR}/platforms/ch58x/split/transport.c"
        )
    endif()
endif()

# CIE1931_CURVE
if(CIE1931_CURVE)
    add_definitions(-DUSE_CIE1931_CURVE)
    set(LED_TABLES ON CACHE BOOL "KB" FORCE)
    message(STATUS "CIE1931_CURVE")
endif()

# LED_TABLES
if(LED_TABLES)
    message(STATUS "LED_TABLES")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/led_tables.c"
    )
endif()

# EEPROM_ENABLE
if(EEPROM_ENABLE)
    add_definitions(-DEEPROM_ENABLE -DEEPROM_DRIVER)
    message(STATUS "EEPROM_ENABLE")
    message(STATUS "EEPROM_DRIVER = ${EEPROM_DRIVER}")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/drivers/eeprom/eeprom_driver.c"
        "${QMK_BASE_DIR}/quantum/eeconfig.c"
        "${QMK_BASE_DIR}/quantum/process_keycode/process_default_layer.c"
        # vial-qmk NVM 鎶借薄灞傦紙EEPROM 鍚庣锛?
        "${QMK_BASE_DIR}/quantum/nvm/eeprom/nvm_eeconfig.c"
        "${QMK_BASE_DIR}/quantum/nvm/eeprom/nvm_dynamic_keymap.c"
        "${QMK_BASE_DIR}/quantum/nvm/eeprom/nvm_via.c"
    )

    if(EEPROM_DRIVER STREQUAL "custom")
        add_definitions(-DEEPROM_CUSTOM)
        include_directories(${QMK_BASE_DIR}/drivers/eeprom)
        list(APPEND QMK_PORTING_SOURCES
            "${CMAKE_CURRENT_LIST_DIR}/platforms/ch58x/eeprom/eeprom_ch58x_custom.c"
        )
    elseif(EEPROM_DRIVER STREQUAL "wear_leveling")
        add_definitions(-DEEPROM_WEAR_LEVELING)
        include_directories(${QMK_BASE_DIR}/drivers/eeprom)
        include_directories(${CMAKE_CURRENT_LIST_DIR}/platforms/ch58x/eeprom/fnv)
        list(APPEND QMK_PORTING_SOURCES
            "${CMAKE_CURRENT_LIST_DIR}/platforms/ch58x/eeprom/eeprom_wear_leveling.c"
            "${CMAKE_CURRENT_LIST_DIR}/platforms/ch58x/eeprom/wear_leveling_ch58x.c"
            "${CMAKE_CURRENT_LIST_DIR}/platforms/ch58x/eeprom/wear_leveling.c"
            "${CMAKE_CURRENT_LIST_DIR}/platforms/ch58x/eeprom/fnv/*.c"
        )
    else()
        message(FATAL_ERROR "Unsupported EEPROM driver!")
    endif()
endif()

# WS2812 REQUIRED
if(WS2812_REQUIRED)
    add_definitions(-DWS2812)

    if(RGBLIGHT_ENABLE)
        add_definitions(-DRGBLIGHT_WS2812)
    elseif(RGB_MATRIX_ENABLE)
        add_definitions(-DRGB_MATRIX_WS2812)
    endif()

    message(STATUS "WS2812_REQUIRED")
    message(STATUS "WS2812_DRIVER = ${WS2812_DRIVER}")
    include_directories(${QMK_BASE_DIR}/drivers)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/drivers/ws2812)

    if(WS2812_DRIVER STREQUAL "spi")
        add_definitions(-DWS2812_DRIVER_SPI)
        list(APPEND QMK_PORTING_SOURCES
            "${CMAKE_CURRENT_LIST_DIR}/drivers/ws2812/ws2812_spi.c"
            "${CMAKE_CURRENT_LIST_DIR}/drivers/ws2812/ws2812_supplement.c"
        )
        set(SPI_OCCUPIED ON CACHE BOOL "KB" FORCE)
    elseif(WS2812_DRIVER STREQUAL "pwm")
        add_definitions(-DWS2812_DRIVER_PWM)
        list(APPEND QMK_PORTING_SOURCES
            "${CMAKE_CURRENT_LIST_DIR}/drivers/ws2812/ws2812_pwm.c"
            "${CMAKE_CURRENT_LIST_DIR}/drivers/ws2812/ws2812_supplement.c"
        )
    else()
        message(FATAL_ERROR "Unsupported WS2812 Driver!")
    endif()
endif()

# OLED ENABLE
if(OLED_ENABLE)
    if(OLED_DRIVER STREQUAL "SSD1306")
        add_definitions(-DOLED_ENABLE)
        message(STATUS "OLED_ENABLE")
        message(STATUS "OLED_DRIVER = ${OLED_DRIVER}")
        include_directories(${QMK_BASE_DIR}/drivers/oled)
        list(APPEND quantum_SOURCES
            "${QMK_BASE_DIR}/drivers/oled/oled_driver.c"
        )
        string(TOLOWER ${OLED_TRANSPORT} OLED_TRANSPORT)

        if(OLED_TRANSPORT STREQUAL "i2c")
            set(I2C_MASTER_ENABLE ON CACHE BOOL "KB" FORCE)
            add_definitions(-DOLED_TRANSPORT_I2C)
        elseif(OLED_TRANSPORT STREQUAL "spi")
            set(SPI_MASTER_ENABLE ON CACHE BOOL "KB" FORCE)
            add_definitions(-DOLED_TRANSPORT_SPI)
        endif()
    elseif(DEFINED OLED_DRIVER AND EXISTS "${OLED_DRIVER}")
        add_definitions(-DOLED_ENABLE)
        message(STATUS "OLED_ENABLE")
        message(STATUS "CUSTOM_OLED_DRIVER: ${OLED_DRIVER}")
        include_directories(${QMK_BASE_DIR}/drivers/oled)
        list(APPEND QMK_PORTING_SOURCES
            "${OLED_DRIVER}"
        )
    else()
        message(FATAL_ERROR "Unsupported OLED driver!")
    endif()
endif()

# WPM ENABLE
if(WPM_ENABLE)
    add_definitions(-DWPM_ENABLE)
    message(STATUS "WPM_ENABLE")
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/wpm.c"
    )
endif()

# AW20216S REQUIRED
if(AW20216S_REQUIRED)
    add_definitions(-DAW20216S)

    if(RGBLIGHT_ENABLE)
        add_definitions(-DRGBLIGHT_AW20216S)
    elseif(RGB_MATRIX_ENABLE)
        add_definitions(-DRGB_MATRIX_AW20216S)
    endif()

    message(STATUS "AW20216S_REQUIRED")
    include_directories(${QMK_BASE_DIR}/drivers/led)
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/drivers/led/aw20216s.c"
    )
    list(APPEND QMK_PORTING_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/drivers/aw20216s/aw20216s_supplement.c"
    )
    set(SPI_MASTER_ENABLE ON CACHE BOOL "KB" FORCE)
endif()

# I2C_MASTER_ENABLE
if(I2C_MASTER_ENABLE)
    add_definitions(-DI2C_MASTER_ENABLE)
    message(STATUS "I2C_MASTER_ENABLE")
    list(APPEND QMK_PORTING_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/platforms/ch58x/i2c_master.c"
    )
endif()

# SPI_MASTER_ENABLE
if(SPI_MASTER_ENABLE)
    if(SPI_OCCUPIED)
        message(FATAL_ERROR "SPI controller has exclusive use!")
    endif()

    add_definitions(-DSPI_MASTER_ENABLE)
    message(STATUS "SPI_MASTER_ENABLE")
    list(APPEND QMK_PORTING_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/platforms/ch58x/spi_master.c"
    )
endif()

# UART_ENABLE
if(UART_ENABLE)
    add_definitions(-DUART_ENABLE)
    message(STATUS "UART_ENABLE")
    list(APPEND QMK_PORTING_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/platforms/ch58x/uart.c"
    )
endif()

# USB_ENABLE
if(USB_ENABLE)
    add_definitions(-DUSB_ENABLE)
    message(STATUS "USB_ENABLE")
    list(APPEND QMK_PORTING_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/protocol/protocol_usb.c"
        "${CMAKE_CURRENT_LIST_DIR}/platforms/ch58x/usb_main.c"
        "${CMAKE_CURRENT_LIST_DIR}/protocol/usb_interface.c"
        "${CMAKE_CURRENT_LIST_DIR}/protocol/usb_util.c"
    )
endif()

# BLE_ENABLE
if(BLE_ENABLE)
    if(ESB_ENABLE AND ESB_ROLE STREQUAL "dongle")
        message(STATUS "Dongle shouldn't have BLE_ENABLE, ignore")
    else()
        add_definitions(-DBLE_ENABLE)
        message(STATUS "BLE_ENABLE")

        if(NOT DEFINED NKRO_ENABLE)
            message(FATAL_ERROR "BLE_ENABLE requires NKRO_ENABLE!")
        endif()
    endif()
endif()

# ESB_ENABLE
if(ESB_ENABLE)
    if(ESB_ROLE STREQUAL "keyboard")
        message(STATUS "ESB_ENABLE")
        add_definitions(-DESB_ENABLE=1)
        message(STATUS "ESB_ROLE = keyboard")
    elseif(ESB_ROLE STREQUAL "dongle")
        message(STATUS "ESB_ENABLE")
        add_definitions(-DESB_ENABLE=2)
        message(STATUS "ESB_ROLE = dongle")

        if(NOT USB_ENABLE)
            message(WARNING "USB forced on for ESB dongle role.")
            add_definitions(-DUSB_ENABLE)
            list(APPEND QMK_PORTING_SOURCES
                "${CMAKE_CURRENT_LIST_DIR}/protocol/protocol_usb.c"
                "${CMAKE_CURRENT_LIST_DIR}/platforms/ch58x/usb_main.c"
                "${CMAKE_CURRENT_LIST_DIR}/protocol/usb_interface.c"
                "${CMAKE_CURRENT_LIST_DIR}/protocol/usb_util.c"
            )
        endif()
    endif()
endif()

# CUSTOM_MATRIX
if(DEFINED CUSTOM_MATRIX AND EXISTS "${CUSTOM_MATRIX}")
    add_definitions(-DCUSTOM_MATRIX)
    message(STATUS "CUSTOM_MATRIX: ${CUSTOM_MATRIX}")
    list(APPEND QMK_PORTING_SOURCES
        "${CUSTOM_MATRIX}"
    )
else()
    list(APPEND quantum_SOURCES
        "${QMK_BASE_DIR}/quantum/matrix.c"
    )
endif()
