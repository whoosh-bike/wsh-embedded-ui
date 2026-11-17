#ifndef __WEUI_GFX_FPS_CONTROL_H
#define __WEUI_GFX_FPS_CONTROL_H

#include "weui_fps_control.h"
#include "weui_gfx_types.h"

typedef WeUi_FpsCtrl_AppCb_t WeUi_Gfx_FpsCtrl_AppCb_t;
typedef WeUi_FpsCtrl_DrawCb_t WeUi_Gfx_FpsCtrl_DrawCb_t;

/** @brief Register a per-frame app callback (e.g. watchdog reset). Pass NULL to detach. */
void WeUi_Gfx_FpsCtrl_AttachAppCallback(WeUi_Gfx_FpsCtrl_AppCb_t cb);

/** @brief Register the draw callback invoked to render the frame. Pass NULL to detach. */
void WeUi_Gfx_FpsCtrl_AttachDrawCallback(WeUi_Gfx_FpsCtrl_DrawCb_t cb);

/** @brief Return the attached draw callback, or NULL. */
WeUi_Gfx_FpsCtrl_DrawCb_t WeUi_Gfx_FpsCtrl_GetDrawCallback(void);

/** @brief Get current target render frequency (Hz) */
u32 WeUi_Gfx_FpsCtrl_GetMainFreq(void);

/** @brief Set target render frequency. Clamped to [WEUI_GFX_FPS_CTRL_MIN_RENDER_FREQ, WEUI_GFX_FPS_CTRL_MAX_RENDER_FREQ]. */
void WeUi_Gfx_FpsCtrl_SetMainFreq(u32 frameFreq);

/** @brief Reset target frequency to WEUI_GFX_FPS_CTRL_DEF_RENDER_FREQ */
void WeUi_Gfx_FpsCtrl_SetMainFreqToDef(void);

/** @brief Return WEUI_GFX_FPS_CTRL_MIN_RENDER_FREQ */
u32 WeUi_Gfx_FpsCtrl_GetMinFreq(void);

/** @brief Return WEUI_GFX_FPS_CTRL_MAX_RENDER_FREQ */
u32 WeUi_Gfx_FpsCtrl_GetMaxFreq(void);

/** @brief Return frame counter (increments once per rendered frame) */
u32 WeUi_Gfx_FpsCtrl_GetRenderCycle(void);

/** @brief Return current system tick count in milliseconds */
u32 WeUi_Gfx_FpsCtrl_GetTickCnt(void);

/**
 * @brief Invoke draw/app callbacks and delay to maintain the target FPS.
 * @param frameFreq  Target frame rate in Hz
 */
void WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(u32 frameFreq);

/** @brief Shortcut for WeUi_Gfx_FpsCtrl_DrawAndKeepFPS at the current main frequency. */
void WeUi_Gfx_FpsCtrl_DrawAndKeepFPS_MainFreq(void);

#endif /* __WEUI_GFX_FPS_CONTROL_H */
