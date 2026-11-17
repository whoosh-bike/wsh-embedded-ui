#include "weui_gfx_effect_reload.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

/* Wiping, the old picture stands right of the edge; loading, the incoming one stands left of
 * it. Everything else is dark */
static void WeUi_Gfx_Effect_Row_Reload(const WeUi_Gfx_Effect_Ctx_t* pcCtx, WeUi_Gfx_PtPos_t y) {
	WeUi_Gfx_PtPos_t width = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	float half			   = 2.0f * pcCtx->Progress;
	bool wiping			   = half < 1.0f;
	WeUi_Gfx_PtPos_t edge = (WeUi_Gfx_PtPos_t)((float)width * (wiping ? half : half - 1.0f) + 0.5f);
	WeUi_Gfx_ColorRaw_t blank = WeUi_Gfx_Effect_Blank();

	if (wiping) {
		WeUi_Gfx_Effect_RowFill(0, edge, blank);
		WeUi_Gfx_Effect_RowOld(edge, edge, y, width - edge);
		return;
	}
	WeUi_Gfx_Effect_RowNew(0, 0, y, edge);
	WeUi_Gfx_Effect_RowFill(edge, width - edge, blank);
}

RET_STATE_t WeUi_Gfx_Effect_Reload(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunRows(WeUi_Gfx_Effect_Row_Reload, NULL, frames, NULL, renderIncoming);
}
