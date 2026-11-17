#ifndef __WSH_EMBLIB_CFG_H
#define __WSH_EMBLIB_CFG_H

/* wsh-embedded-libs bound to the host: the virtual clock of display_sim.c */

#define WSH_USE_MATH_COMMON 1
#define EMBLIB_NO_LOG

#include <assert.h>
#include <stdlib.h>

#include "def_macro.h"
#include "def_types.h"

u32 Sim_TickMs(void);
void Sim_DelayMs(u32 ms);
void Sim_DelayUntilMs(u32* pPrev, u32 incr);

#define SYS_CRITICAL_ON()
#define SYS_CRITICAL_OFF()
#define SYS_CRITICAL_ON_ISR()		   (0U)
#define SYS_CRITICAL_OFF_ISR(a)		   ((void)(a))
#define SYS_OS_IS_RUNNING()			   (false)
#define SYS_TICK_GET_MS_CNT()		   Sim_TickMs()
#define SYS_DELAY_MS(a)				   Sim_DelayMs(a)
#define SYS_DELAY_UNTIL_MS(prev, incr) Sim_DelayUntilMs(prev, incr)

/* The tests abort on it, the lab reports it and keeps the page alive */
void Sim_Panic(const char* pcFile, int line);

#define PANIC()			   Sim_Panic(__FILE__, __LINE__)
#define ASSERT_CHECK(expr) ((expr) ? (void)0 : Sim_Panic(__FILE__, __LINE__))

#include "def_sys.h"

#endif /* __WSH_EMBLIB_CFG_H */
