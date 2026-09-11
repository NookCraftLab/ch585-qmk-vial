/*
Copyright 2024 Huckies <https://github.com/Huckies>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

__attribute__((always_inline)) inline void run_qmk_task()
{
    extern void protocol_pre_task();
    extern void protocol_keyboard_task();
    extern void protocol_post_task();
    extern void housekeeping_task();

    protocol_pre_task();
    protocol_keyboard_task();
    protocol_post_task();

    // #ifdef RAW_ENABLE
    //             void raw_hid_task(void);
    //             raw_hid_task();
    // #endif

#ifdef CONSOLE_ENABLE
    void console_task(void);
    console_task();
#endif

#ifdef QUANTUM_PAINTER_ENABLE
    // Run Quantum Painter task
    void qp_internal_task(void);
    qp_internal_task();
#endif

#ifdef DEFERRED_EXEC_ENABLE
    // Run deferred executions
    void deferred_exec_task(void);
    deferred_exec_task();
#endif // DEFERRED_EXEC_ENABLE

    housekeeping_task();
}
