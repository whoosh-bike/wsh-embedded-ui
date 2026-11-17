#ifndef __WEUI_FPS_CONTROL_H
#define __WEUI_FPS_CONTROL_H

#include "weui_common_types.h"

typedef void (*WeUi_FpsCtrl_DrawCb_t)(void);
typedef void (*WeUi_FpsCtrl_AppCb_t)(void);

/**
 * @brief FPS controller instance. One per subsystem (gfx, ind, ...).
 *        Initialise with WEUI_FPS_CTRL_CTX_INIT or WeUi_FpsCtrl_Init.
 */
typedef struct {
	u32 DefFreq;
	u32 MinFreq;
	u32 MaxFreq;
	u32 MinDelay;
	u32 RenderFreq;
	u32 RenderCycle;
	u32 LastWakeTime;
	WeUi_FpsCtrl_DrawCb_t DrawCb;
	WeUi_FpsCtrl_AppCb_t AppCb;
} WeUi_FpsCtrl_Ctx_t;

#define WEUI_FPS_CTRL_CTX_INIT(_def, _min, _max, _delay) \
	{                                                    \
		.DefFreq	  = (_def),                          \
		.MinFreq	  = (_min),                          \
		.MaxFreq	  = (_max),                          \
		.MinDelay	  = (_delay),                        \
		.RenderFreq	  = (_def),                          \
		.RenderCycle  = 0,                               \
		.LastWakeTime = 0,                               \
		.DrawCb		  = NULL,                            \
		.AppCb		  = NULL,                            \
	}

/** @brief Attach draw callback (invoked once per frame to render content). Pass NULL to detach. */
void WeUi_FpsCtrl_AttachDrawCallback(WeUi_FpsCtrl_Ctx_t* ctx, WeUi_FpsCtrl_DrawCb_t cb);

/** @brief Return the attached draw callback, or NULL. */
WeUi_FpsCtrl_DrawCb_t WeUi_FpsCtrl_GetDrawCallback(const WeUi_FpsCtrl_Ctx_t* ctx);

/** @brief Attach app callback (invoked once per frame, e.g. watchdog reset). Pass NULL to detach. */
void WeUi_FpsCtrl_AttachAppCallback(WeUi_FpsCtrl_Ctx_t* ctx, WeUi_FpsCtrl_AppCb_t cb);

/** @brief Get current target render frequency (Hz) */
u32 WeUi_FpsCtrl_GetMainFreq(const WeUi_FpsCtrl_Ctx_t* ctx);

/** @brief Set target render frequency. Clamped to [MinFreq, MaxFreq]. */
void WeUi_FpsCtrl_SetMainFreq(WeUi_FpsCtrl_Ctx_t* ctx, u32 frameFreq);

/** @brief Reset target frequency to DefFreq */
void WeUi_FpsCtrl_SetMainFreqToDef(WeUi_FpsCtrl_Ctx_t* ctx);

/** @brief Return MinFreq */
u32 WeUi_FpsCtrl_GetMinFreq(const WeUi_FpsCtrl_Ctx_t* ctx);

/** @brief Return MaxFreq */
u32 WeUi_FpsCtrl_GetMaxFreq(const WeUi_FpsCtrl_Ctx_t* ctx);

/** @brief Return frame counter (increments once per rendered frame) */
u32 WeUi_FpsCtrl_GetRenderCycle(const WeUi_FpsCtrl_Ctx_t* ctx);

/** @brief Return current system tick count in milliseconds */
u32 WeUi_FpsCtrl_GetTickCnt(void);

/**
 * @brief Invoke draw/app callbacks and delay to maintain the target FPS.
 *        Measures actual render time and sleeps for the remainder of the frame budget.
 *        If frame overran the budget, resets timing reference to prevent drift accumulation.
 * @param frameFreq  Target frame rate in Hz
 */
void WeUi_FpsCtrl_DrawAndKeepFPS(WeUi_FpsCtrl_Ctx_t* ctx, u32 frameFreq);

/** @brief Shortcut: draw at ctx->RenderFreq. */
void WeUi_FpsCtrl_DrawAndKeepFPS_MainFreq(WeUi_FpsCtrl_Ctx_t* ctx);

#endif /* __WEUI_FPS_CONTROL_H */
