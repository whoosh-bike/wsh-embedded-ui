#include "weui_gfx_effect_runner.h"
#include "log.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_fps_control.h"
#if WEUI_DISPLAY_BANDS
#include "weui_gfx_fill.h"
#endif /* WEUI_DISPLAY_BANDS */

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

WeUi_Gfx_Effect_Mem_t WeUi_Gfx_Effect_Mem;
WeUi_Gfx_FrameMem_t WeUi_Gfx_Effect_Out;

/* After every push: the push swaps the drawing frame for the shown one */
static void WeUi_Gfx_Effect_MemTake(void) {
	WeUi_Gfx_Effect_Mem_t* pM = &WeUi_Gfx_Effect_Mem;
	pM->On = WeUi_Gfx_Adapter_FrameMem(WEUI_GFX_FRAME_DRAWING, &pM->New) &&
			 WeUi_Gfx_Adapter_FrameMem(WEUI_GFX_FRAME_AUX, &pM->Old) && pM->New.Px == pM->Old.Px;
}

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

/* Before anything else: SaveFrame_DisplayToAux() reads the frame on the panel. The FPS control's
 * draw callback is detached for the run and handed back by End() */
static WeUi_Gfx_FpsCtrl_DrawCb_t WeUi_Gfx_Effect_Begin(void) {
	WeUi_Gfx_Effect_Playing = true;

	/* The only frame that neither the incoming scene nor the frame swap overwrites */
	WeUi_Gfx_Adapter_SaveFrame_DisplayToAux();
	WeUi_Gfx_Effect_MemTake();

	/* Detached, the incoming scene still renders and paces itself, but its
	 * half-composed frames never reach the panel. Restore what the host had */
	WeUi_Gfx_FpsCtrl_DrawCb_t drawCb = WeUi_Gfx_FpsCtrl_GetDrawCallback();
	WeUi_Gfx_FpsCtrl_AttachDrawCallback(NULL);
	return drawCb;
}

static void WeUi_Gfx_Effect_End(WeUi_Gfx_FpsCtrl_DrawCb_t drawCb) {
	WeUi_Gfx_Effect_Mem.On = false;
	WeUi_Gfx_FpsCtrl_AttachDrawCallback(drawCb);
	WeUi_Gfx_Effect_Playing = false;
}

/* The incoming scene drawn every frame, composed over in place and pushed as a frame */
static void WeUi_Gfx_Effect_PlayLive(WeUi_Gfx_Effect_Ctx_t* pCtx, WeUi_Gfx_Effect_FrameCb_t compose,
									 WeUi_Gfx_Effect_RenderCb_t renderIncoming) {
	for (u32 frame = 1; frame <= pCtx->Frames; frame++) {
		WeUi_Gfx_Effect_CtxSeek(pCtx, frame);

		renderIncoming();
		compose(pCtx);

		LOCAL_DEBUG_PRINT("Transition frame %d of %d", pCtx->Frame, pCtx->Frames);

		WeUi_Gfx_Adapter_DrawFrame();
		WeUi_Gfx_Effect_MemTake();
	}
}

RET_STATE_t WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_FrameCb_t compose, u16 frames,
										  const void* pcParams,
										  WeUi_Gfx_Effect_RenderCb_t renderIncoming) {
	if (!compose || !renderIncoming)
		return RET_STATE_ERR_PARAM;

	if (WeUi_Gfx_Effect_Playing)
		return RET_STATE_ERR_BUSY;

	WeUi_Gfx_Effect_Ctx_t ctx		 = WeUi_Gfx_Effect_CtxInit(frames, pcParams);
	WeUi_Gfx_FpsCtrl_DrawCb_t drawCb = WeUi_Gfx_Effect_Begin();
	WeUi_Gfx_Effect_PlayLive(&ctx, compose, renderIncoming);
	WeUi_Gfx_Effect_End(drawCb);

	return RET_STATE_SUCCESS;
}

/* In place, a row at a time: each composed into the scratch row whole before it is put back, so
 * a row may read anywhere along itself, and the new picture on the rows below it */
static WeUi_Gfx_ColorRaw_t WeUi_Gfx_Effect_Scratch[WEUI_DISPLAY_WIDTH];
static WeUi_Gfx_Effect_RowCb_t WeUi_Gfx_Effect_Row;

static void WeUi_Gfx_Effect_Compose_Rows(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	WeUi_Gfx_PtPos_t width = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	ASSERT_CHECK(pcCtx->Width <= WEUI_DISPLAY_WIDTH);

	WeUi_Gfx_Effect_Out = (WeUi_Gfx_FrameMem_t){
		.pBuf	= WeUi_Gfx_Effect_Scratch,
		.Stride = pcCtx->Width,
		.Px		= WEUI_GFX_FRAME_PX_RAW,
	};
	for (WeUi_Gfx_PtPos_t y = 0; y < (WeUi_Gfx_PtPos_t)pcCtx->Height; y++) {
		WeUi_Gfx_Effect_Row(pcCtx, y);
		for (WeUi_Gfx_PtPos_t x = 0; x < width; x++)
			WeUi_Gfx_Effect_SetNew(x, y, WeUi_Gfx_Effect_Scratch[x]);
	}
}

#if WEUI_DISPLAY_BANDS && WEUI_DISPLAY_FRAME_MEM
static bool WeUi_Gfx_Effect_BandsOn = true;

void WeUi_Gfx_Effect_SetBands(bool on) {
	WeUi_Gfx_Effect_BandsOn = on;
}

/* One frame, band by band, each row composed straight into the band's buffer */
static RET_STATE_t WeUi_Gfx_Effect_SendBands(const WeUi_Gfx_Effect_Ctx_t* pcCtx,
											 WeUi_Gfx_Effect_RowCb_t row) {
	u32 width			   = pcCtx->Width;
	WEUI_GFX_FRAME_PX_t px = WEUI_DISPLAY_BAND_FORMAT == WEUI_GFX_TARGET_RGB565_BE
								 ? WEUI_GFX_FRAME_PX_RGB565_BE
								 : WEUI_GFX_FRAME_PX_RGB565;

	for (u32 y = 0; y < pcCtx->Height;) {
		u32 pixels = 0;
		u16* pBuf  = WeUi_Gfx_Adapter_BandGet(&pixels);
		u32 rows   = pixels / width;
		rows -= rows % WEUI_DISPLAY_BAND_ALIGN;
		rows = GET_MIN(rows, pcCtx->Height - y);

		for (u32 r = 0; r < rows; r++) {
			WeUi_Gfx_Effect_Out =
				(WeUi_Gfx_FrameMem_t){.pBuf = &pBuf[r * width], .Stride = width, .Px = px};
			row(pcCtx, (WeUi_Gfx_PtPos_t)(y + r));
		}
		/* The buffer goes back to the driver whatever happens to the band */
		WeUi_Gfx_Adapter_BandSend(0, (u16)y, (u16)width, (u16)(y + rows), pBuf);
		if (rows == 0)
			return RET_STATE_ERR_MEMORY;
		y += rows;
	}
	return RET_STATE_SUCCESS;
}

/* The incoming scene drawn once; every frame composed from the two pictures as they stand and
 * sent in bands, paced at the main rate; the last one is the incoming frame itself */
static RET_STATE_t WeUi_Gfx_Effect_PlayBands(WeUi_Gfx_Effect_Ctx_t* pCtx,
											 WeUi_Gfx_Effect_RowCb_t row,
											 WeUi_Gfx_Effect_RenderCb_t renderIncoming) {
	renderIncoming();

	for (u32 frame = 1; frame < pCtx->Frames; frame++) {
		WeUi_Gfx_Effect_CtxSeek(pCtx, frame);
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Effect_SendBands(pCtx, row));

		LOCAL_DEBUG_PRINT("Transition frame %d of %d, in bands", pCtx->Frame, pCtx->Frames);

		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS_MainFreq();
	}

	WeUi_Gfx_Effect_CtxSeek(pCtx, pCtx->Frames);
	WeUi_Gfx_Adapter_DrawFrame();
	return RET_STATE_SUCCESS;
}
#endif /* WEUI_DISPLAY_BANDS && WEUI_DISPLAY_FRAME_MEM */

RET_STATE_t WeUi_Gfx_Effect_RunRows(WeUi_Gfx_Effect_RowCb_t row, WeUi_Gfx_Effect_FrameCb_t compose,
									u16 frames, const void* pcParams,
									WeUi_Gfx_Effect_RenderCb_t renderIncoming) {
	if (!row || !renderIncoming)
		return RET_STATE_ERR_PARAM;

	if (WeUi_Gfx_Effect_Playing)
		return RET_STATE_ERR_BUSY;

	WeUi_Gfx_Effect_Ctx_t ctx		 = WeUi_Gfx_Effect_CtxInit(frames, pcParams);
	WeUi_Gfx_FpsCtrl_DrawCb_t drawCb = WeUi_Gfx_Effect_Begin();
	RET_STATE_t rs					 = RET_STATE_SUCCESS;

#if WEUI_DISPLAY_BANDS && WEUI_DISPLAY_FRAME_MEM
	if (WeUi_Gfx_Effect_BandsOn && WeUi_Gfx_Effect_Mem.On) {
		rs = WeUi_Gfx_Effect_PlayBands(&ctx, row, renderIncoming);
		WeUi_Gfx_Effect_End(drawCb);
		return rs;
	}
#endif /* WEUI_DISPLAY_BANDS && WEUI_DISPLAY_FRAME_MEM */

	WeUi_Gfx_Effect_Row = row;
	WeUi_Gfx_Effect_PlayLive(&ctx, compose ? compose : WeUi_Gfx_Effect_Compose_Rows,
							 renderIncoming);
	WeUi_Gfx_Effect_End(drawCb);
	return rs;
}

void WeUi_Gfx_Effect_RowSample(const WeUi_Gfx_Effect_Ctx_t* ctx, s16 x, s16 sx, s16 sy, bool old) {
	if (x < 0 || x >= ctx->Width)
		return;
	if (sx < 0 || sy < 0 || sx >= ctx->Width || sy >= ctx->Height)
		WeUi_Gfx_Effect_RowSet(x, WeUi_Gfx_Effect_Blank());
	else if (old)
		WeUi_Gfx_Effect_RowOld(x, sx, sy, 1);
	else
		WeUi_Gfx_Effect_RowNew(x, sx, sy, 1);
}
