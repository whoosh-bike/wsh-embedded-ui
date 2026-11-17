#include "weui_gfx_effect_melt.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

#define WEUI_GFX_EFFECT_MELT_SPREAD 0.4f /* of the run the latest column starts by */

/* How far each column of the old picture has dropped, worked out on a frame's first row */
static WeUi_Gfx_PtPos_t WeUi_Gfx_Effect_MeltSag[WEUI_DISPLAY_WIDTH];

/* The old column dropped by its sag; above it the incoming one shows */
static void WeUi_Gfx_Effect_Row_Melt(const WeUi_Gfx_Effect_Ctx_t* pcCtx, WeUi_Gfx_PtPos_t y) {
	u8 columnPx			   = GET_MAX(*(const u8*)pcCtx->ParamsPtr, 1u);
	WeUi_Gfx_PtPos_t width = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	ASSERT_CHECK(pcCtx->Width <= WEUI_DISPLAY_WIDTH);

	if (y == 0) {
		for (WeUi_Gfx_PtPos_t x = 0; x < width; x += columnPx) {
			float run = WeUi_Gfx_Effect_ColumnRun(pcCtx, x, columnPx, WEUI_GFX_EFFECT_MELT_SPREAD);
			WeUi_Gfx_Effect_MeltSag[x] =
				(WeUi_Gfx_PtPos_t)((float)pcCtx->Height * run * run + 0.5f);
		}
	}
	for (WeUi_Gfx_PtPos_t x = 0; x < width; x += columnPx) {
		WeUi_Gfx_PtPos_t n	 = GET_MIN((WeUi_Gfx_PtPos_t)columnPx, width - x);
		WeUi_Gfx_PtPos_t sag = WeUi_Gfx_Effect_MeltSag[x];
		if (y >= sag)
			WeUi_Gfx_Effect_RowOld(x, x, y - sag, n);
		else
			WeUi_Gfx_Effect_RowNew(x, x, y, n);
	}
}

RET_STATE_t WeUi_Gfx_Effect_Melt(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
								 u8 columnPx) {
	return WeUi_Gfx_Effect_RunRows(WeUi_Gfx_Effect_Row_Melt, NULL, frames, &columnPx,
								   renderIncoming);
}
