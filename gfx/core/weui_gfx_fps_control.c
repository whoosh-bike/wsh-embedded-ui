#include "weui_gfx_fps_control.h"

static WeUi_FpsCtrl_Ctx_t gCtx =
	WEUI_FPS_CTRL_CTX_INIT(WEUI_GFX_FPS_CTRL_DEF_RENDER_FREQ, WEUI_GFX_FPS_CTRL_MIN_RENDER_FREQ,
						   WEUI_GFX_FPS_CTRL_MAX_RENDER_FREQ, WEUI_GFX_FPS_CTRL_MIN_DELAY);

void WeUi_Gfx_FpsCtrl_AttachAppCallback(WeUi_Gfx_FpsCtrl_AppCb_t cb) {
	WeUi_FpsCtrl_AttachAppCallback(&gCtx, cb);
}

void WeUi_Gfx_FpsCtrl_AttachDrawCallback(WeUi_Gfx_FpsCtrl_DrawCb_t cb) {
	WeUi_FpsCtrl_AttachDrawCallback(&gCtx, cb);
}

WeUi_Gfx_FpsCtrl_DrawCb_t WeUi_Gfx_FpsCtrl_GetDrawCallback(void) {
	return WeUi_FpsCtrl_GetDrawCallback(&gCtx);
}

u32 WeUi_Gfx_FpsCtrl_GetMainFreq(void) {
	return WeUi_FpsCtrl_GetMainFreq(&gCtx);
}

void WeUi_Gfx_FpsCtrl_SetMainFreq(u32 frameFreq) {
	WeUi_FpsCtrl_SetMainFreq(&gCtx, frameFreq);
}

void WeUi_Gfx_FpsCtrl_SetMainFreqToDef(void) {
	WeUi_FpsCtrl_SetMainFreqToDef(&gCtx);
}

u32 WeUi_Gfx_FpsCtrl_GetMinFreq(void) {
	return WeUi_FpsCtrl_GetMinFreq(&gCtx);
}

u32 WeUi_Gfx_FpsCtrl_GetMaxFreq(void) {
	return WeUi_FpsCtrl_GetMaxFreq(&gCtx);
}

u32 WeUi_Gfx_FpsCtrl_GetRenderCycle(void) {
	return WeUi_FpsCtrl_GetRenderCycle(&gCtx);
}

u32 WeUi_Gfx_FpsCtrl_GetTickCnt(void) {
	return WeUi_FpsCtrl_GetTickCnt();
}

void WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(u32 frameFreq) {
	WeUi_FpsCtrl_DrawAndKeepFPS(&gCtx, frameFreq);
}

void WeUi_Gfx_FpsCtrl_DrawAndKeepFPS_MainFreq(void) {
	WeUi_FpsCtrl_DrawAndKeepFPS_MainFreq(&gCtx);
}
