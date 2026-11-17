#include "weui_gfx_effect_runner.h"
#include "log.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_fps_control.h"

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT LOG
#else /* LOCAL_DEBUG_PRINT_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* LOCAL_DEBUG_PRINT_ENABLE */

#define WEUI_GFX_EFFECT_MS_IN_SEC 1000u

static bool WeUi_Gfx_Effect_Playing;

bool WeUi_Gfx_Effect_IsPlaying(void) {
	return WeUi_Gfx_Effect_Playing;
}

u16 WeUi_Gfx_Effect_Frames(u32 ms, u32 fps) {
	u64 frames = ((u64)ms * fps + WEUI_GFX_EFFECT_MS_IN_SEC / 2) / WEUI_GFX_EFFECT_MS_IN_SEC;

	return (u16)CUT_MIN_MAX(frames, WEUI_GFX_EFFECT_FRAMES_MIN, UINT16_MAX);
}

/* Stepped through a u32: a u16 counter at 65535 frames wraps to 0 and the run never ends */
static void WeUi_Gfx_Effect_CtxSeek(WeUi_Gfx_Effect_Ctx_t* pCtx, u32 frame) {
	pCtx->Frame	   = (u16)frame;
	pCtx->Progress = (float)frame / (float)pCtx->Frames;
}

static WeUi_Gfx_Effect_Ctx_t WeUi_Gfx_Effect_CtxInit(u16 frames, const void* pcParams) {
	return (WeUi_Gfx_Effect_Ctx_t){
		.Frames	   = (frames < WEUI_GFX_EFFECT_FRAMES_MIN) ? WEUI_GFX_EFFECT_FRAMES_MIN : frames,
		.Width	   = WeUi_Gfx_Adapter_GetWidth(),
		.Height	   = WeUi_Gfx_Adapter_GetHeight(),
		.ParamsPtr = pcParams,
	};
}

RET_STATE_t WeUi_Gfx_Effect_RunSolo(WeUi_Gfx_Effect_FrameCb_t paint, u16 frames,
									const void* pcParams) {
	if (!paint)
		return RET_STATE_ERR_PARAM;

	if (WeUi_Gfx_Effect_Playing)
		return RET_STATE_ERR_BUSY;

	WeUi_Gfx_Effect_Playing = true;

	WeUi_Gfx_Effect_Ctx_t ctx = WeUi_Gfx_Effect_CtxInit(frames, pcParams);

	for (u32 frame = 1; frame <= ctx.Frames; frame++) {
		WeUi_Gfx_Effect_CtxSeek(&ctx, frame);

		WeUi_Gfx_Adapter_ClearDrawingFrame();
		paint(&ctx);

		LOCAL_DEBUG_PRINT("Animation frame %d of %d", ctx.Frame, ctx.Frames);

		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS_MainFreq();
	}

	WeUi_Gfx_Effect_Playing = false;

	return RET_STATE_SUCCESS;
}

RET_STATE_t WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_FrameCb_t compose, u16 frames,
										  const void* pcParams,
										  WeUi_Gfx_Effect_RenderCb_t renderIncoming) {
	if (!compose || !renderIncoming)
		return RET_STATE_ERR_PARAM;

	if (WeUi_Gfx_Effect_Playing)
		return RET_STATE_ERR_BUSY;

	WeUi_Gfx_Effect_Playing = true;

	WeUi_Gfx_Effect_Ctx_t ctx = WeUi_Gfx_Effect_CtxInit(frames, pcParams);

	/* The only frame that neither the incoming scene nor the frame swap overwrites */
	WeUi_Gfx_Adapter_SaveFrame_DisplayToAux();

	/* Detached, the incoming scene still renders and paces itself, but its
	 * half-composed frames never reach the panel. Restore what the host had */
	WeUi_Gfx_FpsCtrl_DrawCb_t drawCb = WeUi_Gfx_FpsCtrl_GetDrawCallback();
	WeUi_Gfx_FpsCtrl_AttachDrawCallback(NULL);

	for (u32 frame = 1; frame <= ctx.Frames; frame++) {
		WeUi_Gfx_Effect_CtxSeek(&ctx, frame);

		renderIncoming();
		compose(&ctx);

		LOCAL_DEBUG_PRINT("Transition frame %d of %d", ctx.Frame, ctx.Frames);

		WeUi_Gfx_Adapter_DrawFrame();
	}

	WeUi_Gfx_FpsCtrl_AttachDrawCallback(drawCb);
	WeUi_Gfx_Effect_Playing = false;

	return RET_STATE_SUCCESS;
}
