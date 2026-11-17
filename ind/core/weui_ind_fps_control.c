#include "weui_ind_fps_control.h"

static WeUi_FpsCtrl_Ctx_t gCtx =
	WEUI_FPS_CTRL_CTX_INIT(WEUI_IND_FPS_CTRL_DEF_RENDER_FREQ, WEUI_IND_FPS_CTRL_MIN_RENDER_FREQ,
						   WEUI_IND_FPS_CTRL_MAX_RENDER_FREQ, WEUI_IND_FPS_CTRL_MIN_DELAY);

void WeUi_Ind_FpsCtrl_AttachDrawCallback(WeUi_Ind_FpsCtrl_DrawCb_t cb) {
	WeUi_FpsCtrl_AttachDrawCallback(&gCtx, cb);
}

u32 WeUi_Ind_FpsCtrl_GetMainFreq(void) {
	return WeUi_FpsCtrl_GetMainFreq(&gCtx);
}

void WeUi_Ind_FpsCtrl_SetMainFreq(u32 frameFreq) {
	WeUi_FpsCtrl_SetMainFreq(&gCtx, frameFreq);
}

void WeUi_Ind_FpsCtrl_SetMainFreqToDef(void) {
	WeUi_FpsCtrl_SetMainFreqToDef(&gCtx);
}

u32 WeUi_Ind_FpsCtrl_GetMinFreq(void) {
	return WeUi_FpsCtrl_GetMinFreq(&gCtx);
}

u32 WeUi_Ind_FpsCtrl_GetMaxFreq(void) {
	return WeUi_FpsCtrl_GetMaxFreq(&gCtx);
}

u32 WeUi_Ind_FpsCtrl_GetRenderCycle(void) {
	return WeUi_FpsCtrl_GetRenderCycle(&gCtx);
}

u32 WeUi_Ind_FpsCtrl_GetTickCnt(void) {
	return WeUi_FpsCtrl_GetTickCnt();
}

void WeUi_Ind_FpsCtrl_DrawAndKeepFPS(u32 frameFreq) {
	WeUi_FpsCtrl_DrawAndKeepFPS(&gCtx, frameFreq);
}

void WeUi_Ind_FpsCtrl_DrawAndKeepFPS_MainFreq(void) {
	WeUi_FpsCtrl_DrawAndKeepFPS_MainFreq(&gCtx);
}
