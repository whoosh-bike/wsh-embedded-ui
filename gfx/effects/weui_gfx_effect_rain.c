#include "weui_gfx_effect_rain.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

#define WEUI_GFX_EFFECT_RAIN_SPREAD 0.5f /* of the run the latest column starts by */

/* How far down each column the incoming picture has run, worked out on a frame's first row */
static WeUi_Gfx_PtPos_t WeUi_Gfx_Effect_RainWet[WEUI_DISPLAY_WIDTH];

static void WeUi_Gfx_Effect_Row_Rain(const WeUi_Gfx_Effect_Ctx_t* pcCtx, WeUi_Gfx_PtPos_t y) {
	u8 columnPx			   = GET_MAX(*(const u8*)pcCtx->ParamsPtr, 1u);
	WeUi_Gfx_PtPos_t width = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	ASSERT_CHECK(pcCtx->Width <= WEUI_DISPLAY_WIDTH);

	if (y == 0) {
		for (WeUi_Gfx_PtPos_t x = 0; x < width; x += columnPx) {
			/* Squared: a run speeds up as it falls */
			float run = WeUi_Gfx_Effect_ColumnRun(pcCtx, x, columnPx, WEUI_GFX_EFFECT_RAIN_SPREAD);
			WeUi_Gfx_Effect_RainWet[x] =
				(WeUi_Gfx_PtPos_t)((float)pcCtx->Height * run * run + 0.5f);
		}
	}
	for (WeUi_Gfx_PtPos_t x = 0; x < width; x += columnPx) {
		WeUi_Gfx_PtPos_t n = GET_MIN((WeUi_Gfx_PtPos_t)columnPx, width - x);
		if (y >= WeUi_Gfx_Effect_RainWet[x])
			WeUi_Gfx_Effect_RowOld(x, x, y, n);
		else
			WeUi_Gfx_Effect_RowNew(x, x, y, n);
	}
}

RET_STATE_t WeUi_Gfx_Effect_Rain(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
								 u8 columnPx) {
	return WeUi_Gfx_Effect_RunRows(WeUi_Gfx_Effect_Row_Rain, NULL, frames, &columnPx,
								   renderIncoming);
}
