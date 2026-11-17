#ifndef __WEUI_IND_FPS_CONTROL_H
#define __WEUI_IND_FPS_CONTROL_H

#include "weui_fps_control.h"
#include "weui_ind_types.h"

typedef WeUi_FpsCtrl_DrawCb_t WeUi_Ind_FpsCtrl_DrawCb_t;

/** @brief Register the draw callback invoked to render the frame. Pass NULL to detach. */
void WeUi_Ind_FpsCtrl_AttachDrawCallback(WeUi_Ind_FpsCtrl_DrawCb_t cb);

/** @brief Get current target render frequency (Hz) */
u32 WeUi_Ind_FpsCtrl_GetMainFreq(void);

/** @brief Set target render frequency. Clamped to [WEUI_IND_FPS_CTRL_MIN_RENDER_FREQ, WEUI_IND_FPS_CTRL_MAX_RENDER_FREQ]. */
void WeUi_Ind_FpsCtrl_SetMainFreq(u32 frameFreq);

/** @brief Reset target frequency to WEUI_IND_FPS_CTRL_DEF_RENDER_FREQ */
void WeUi_Ind_FpsCtrl_SetMainFreqToDef(void);

/** @brief Return WEUI_IND_FPS_CTRL_MIN_RENDER_FREQ */
u32 WeUi_Ind_FpsCtrl_GetMinFreq(void);

/** @brief Return WEUI_IND_FPS_CTRL_MAX_RENDER_FREQ */
u32 WeUi_Ind_FpsCtrl_GetMaxFreq(void);

/** @brief Return frame counter (increments once per rendered frame) */
u32 WeUi_Ind_FpsCtrl_GetRenderCycle(void);

/** @brief Return current system tick count in milliseconds */
u32 WeUi_Ind_FpsCtrl_GetTickCnt(void);

/**
 * @brief Invoke draw callback and delay to maintain the target FPS.
 * @param frameFreq  Target frame rate in Hz
 */
void WeUi_Ind_FpsCtrl_DrawAndKeepFPS(u32 frameFreq);

/** @brief Shortcut for WeUi_Ind_FpsCtrl_DrawAndKeepFPS at the current main frequency. */
void WeUi_Ind_FpsCtrl_DrawAndKeepFPS_MainFreq(void);

#endif /* __WEUI_IND_FPS_CONTROL_H */
