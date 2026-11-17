#include "weui_gfx_effect_dim.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

/* Eased rather than linear, so neither picture is seen braking into the dark. The first half
 * the old picture dims, the second the incoming one comes up */
static void WeUi_Gfx_Effect_Row_Dim(const WeUi_Gfx_Effect_Ctx_t* pcCtx, WeUi_Gfx_PtPos_t y) {
	WeUi_Gfx_PtPos_t width = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	float half			   = 2.0f * pcCtx->Progress;

	if (half >= 1.0f) {
		WeUi_Gfx_Effect_RowFaded(false, 0, 0, y, width, WeUi_Gfx_Effect_Smooth(half - 1.0f));
		return;
	}
	WeUi_Gfx_Effect_RowFaded(true, 0, 0, y, width, WeUi_Gfx_Effect_Smooth(1.0f - half));
}

RET_STATE_t WeUi_Gfx_Effect_Dim(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunRows(WeUi_Gfx_Effect_Row_Dim, NULL, frames, NULL, renderIncoming);
}
