#include "weui_gfx_effect_interlace.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

/* Even rows come in from the left, odd ones from the right, `rest` short of home */
static void WeUi_Gfx_Effect_Row_Interlace(const WeUi_Gfx_Effect_Ctx_t* pcCtx,
										  WeUi_Gfx_PtPos_t y) {
	WeUi_Gfx_PtPos_t width = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	WeUi_Gfx_PtPos_t rest =
		(WeUi_Gfx_PtPos_t)((float)width * (1.0f - WeUi_Gfx_Effect_Smooth(pcCtx->Progress)) + 0.5f);

	if ((y % 2) == 0) {
		WeUi_Gfx_Effect_RowNew(0, rest, y, width - rest);
		WeUi_Gfx_Effect_RowOld(width - rest, width - rest, y, rest);
		return;
	}
	WeUi_Gfx_Effect_RowOld(0, 0, y, rest);
	WeUi_Gfx_Effect_RowNew(rest, 0, y, width - rest);
}

RET_STATE_t WeUi_Gfx_Effect_Interlace(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunRows(WeUi_Gfx_Effect_Row_Interlace, NULL, frames, NULL,
								   renderIncoming);
}
