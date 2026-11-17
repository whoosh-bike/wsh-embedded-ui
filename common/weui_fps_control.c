#include "weui_fps_control.h"
#include "log.h"
#include "weui_cfg.h"  //for WEUI_DELAY_UNTIL_MS WEUI_DELAY_MS WEUI_TICK_GET_MS_CNT macro

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT LOG_RAW_DIRECT_NL
#else /* LOCAL_DEBUG_PRINT_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* LOCAL_DEBUG_PRINT_ENABLE */

void WeUi_FpsCtrl_AttachDrawCallback(WeUi_FpsCtrl_Ctx_t* ctx, WeUi_FpsCtrl_DrawCb_t cb) {
	ctx->DrawCb = cb;
}

WeUi_FpsCtrl_DrawCb_t WeUi_FpsCtrl_GetDrawCallback(const WeUi_FpsCtrl_Ctx_t* ctx) {
	return ctx->DrawCb;
}

void WeUi_FpsCtrl_AttachAppCallback(WeUi_FpsCtrl_Ctx_t* ctx, WeUi_FpsCtrl_AppCb_t cb) {
	ctx->AppCb = cb;
}

u32 WeUi_FpsCtrl_GetMainFreq(const WeUi_FpsCtrl_Ctx_t* ctx) {
	return ctx->RenderFreq;
}

void WeUi_FpsCtrl_SetMainFreq(WeUi_FpsCtrl_Ctx_t* ctx, u32 frameFreq) {
	ctx->RenderFreq = CUT_MIN_MAX(frameFreq, ctx->MinFreq, ctx->MaxFreq);
	LOG_LVL(LOG_LVL_TRACE, "New render freq: %d", ctx->RenderFreq);
}

void WeUi_FpsCtrl_SetMainFreqToDef(WeUi_FpsCtrl_Ctx_t* ctx) {
	ctx->RenderFreq = ctx->DefFreq;
}

u32 WeUi_FpsCtrl_GetMinFreq(const WeUi_FpsCtrl_Ctx_t* ctx) {
	return ctx->MinFreq;
}

u32 WeUi_FpsCtrl_GetMaxFreq(const WeUi_FpsCtrl_Ctx_t* ctx) {
	return ctx->MaxFreq;
}

u32 WeUi_FpsCtrl_GetRenderCycle(const WeUi_FpsCtrl_Ctx_t* ctx) {
	return ctx->RenderCycle;
}

u32 WeUi_FpsCtrl_GetTickCnt(void) {
	return WEUI_TICK_GET_MS_CNT();
}

void WeUi_FpsCtrl_DrawAndKeepFPS(WeUi_FpsCtrl_Ctx_t* ctx, u32 frameFreq) {
	if (ctx->LastWakeTime == 0)
		ctx->LastWakeTime = WEUI_TICK_GET_MS_CNT();

	if (ctx->DrawCb)
		ctx->DrawCb();

	WEUI_DELAY_MS(ctx->MinDelay);
	ctx->RenderCycle++;

	if (ctx->AppCb)
		ctx->AppCb();

	frameFreq		   = CUT_MIN_MAX(frameFreq, ctx->MinFreq, ctx->MaxFreq);
	u32 frameDuration  = lroundf(1000.0f / (float)frameFreq);
	u32 renderDuration = WEUI_TICK_GET_MS_CNT() - ctx->LastWakeTime;

	u32 delayStart = WEUI_TICK_GET_MS_CNT();
	WEUI_DELAY_UNTIL_MS(&ctx->LastWakeTime, frameDuration);
	u32 delayTime = WEUI_TICK_GET_MS_CNT() - delayStart;

	if (delayTime == 0)
		ctx->LastWakeTime = WEUI_TICK_GET_MS_CNT();

	float realFreq = 1000.0f / (float)(renderDuration + delayTime);
	float fpsRatio = realFreq / (float)frameFreq;
	LOCAL_DEBUG_PRINT(
		"Freq/dur: %3d/%3d; rendered for %3d, delayed on %3d, real fps: %6.3f, fps ratio: %5.2f",
		frameFreq, frameDuration, renderDuration, delayTime, realFreq, fpsRatio);
	DISCARD_UNUSED(renderDuration);
	DISCARD_UNUSED(realFreq);
	DISCARD_UNUSED(fpsRatio);
}

void WeUi_FpsCtrl_DrawAndKeepFPS_MainFreq(WeUi_FpsCtrl_Ctx_t* ctx) {
	WeUi_FpsCtrl_DrawAndKeepFPS(ctx, ctx->RenderFreq);
}
