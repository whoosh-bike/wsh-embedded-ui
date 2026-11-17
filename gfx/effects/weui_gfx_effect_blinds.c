#include "weui_gfx_effect_blinds.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

#define WEUI_GFX_EFFECT_BLINDS_SLATS 5 /* down the panel */

static void WeUi_Gfx_Effect_Row_Blinds(const WeUi_Gfx_Effect_Ctx_t* pcCtx, WeUi_Gfx_PtPos_t y) {
	WeUi_Gfx_PtPos_t width = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	float slat			   = (float)pcCtx->Height / WEUI_GFX_EFFECT_BLINDS_SLATS;
	float open			   = slat * WeUi_Gfx_Effect_Smooth(pcCtx->Progress);
	float within		   = (float)y - slat * (float)(u32)((float)y / slat);

	if (within < open)
		WeUi_Gfx_Effect_RowNew(0, 0, y, width);
	else
		WeUi_Gfx_Effect_RowOld(0, 0, y, width);
}

RET_STATE_t WeUi_Gfx_Effect_Blinds(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunRows(WeUi_Gfx_Effect_Row_Blinds, NULL, frames, NULL, renderIncoming);
}
