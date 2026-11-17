#ifndef __WEUI_CFG_H
#define __WEUI_CFG_H

/* weui config of the host test build - as little as weui_cfg_check.h allows, the rest is
 * defaulted. Panel size, display type and colour scheme come from the Makefile */

#include "mathlib_common.h"

#if WEUI_TEST_DISPLAY_GRAY
#define WEUI_DISPLAY_GRAY
#else
#define WEUI_DISPLAY_COLOR
#endif

#if WEUI_TEST_LIB_GRAY
#define WEUI_LIB_COLOR_SCHEME__GRAY
#else
#define WEUI_LIB_COLOR_SCHEME__RGB
#endif

#define WEUI_USE_COMPONENT__IMAGE 1

/* Virtual clock: time moves only when the library sleeps, so a run is deterministic
 * and takes no wall time */
#define WEUI_TICK_GET_MS_CNT()			Sim_TickMs()
#define WEUI_DELAY_MS(a)				Sim_DelayMs(a)
#define WEUI_DELAY_UNTIL_MS(prev, incr) Sim_DelayUntilMs(prev, incr)

#define WEUI_CRITICAL_DECL()
#define WEUI_ENTER_CRITICAL()
#define WEUI_EXIT_CRITICAL()

#endif /* __WEUI_CFG_H */
