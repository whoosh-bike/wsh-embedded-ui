#include "weui_gfx_effect_curtain.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

/* Each half keeps its own picture and slides out with it: along the parting axis, the old
 * picture from `parted` further in on either side, the incoming one in the gap between */
static void WeUi_Gfx_Effect_Row_Curtain(const WeUi_Gfx_Effect_Ctx_t* pcCtx, WeUi_Gfx_PtPos_t y) {
	bool vertical =
		*(const WEUI_GFX_EFFECT_AXIS_t*)pcCtx->ParamsPtr == WEUI_GFX_EFFECT_AXIS_VERTICAL;
	WeUi_Gfx_PtPos_t width = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	WeUi_Gfx_PtPos_t span  = (WeUi_Gfx_PtPos_t)(vertical ? pcCtx->Height : pcCtx->Width);
	WeUi_Gfx_PtPos_t half  = span / 2;
	/* The curtain gathers way, then slows into the wings */
	WeUi_Gfx_PtPos_t parted =
		(WeUi_Gfx_PtPos_t)((float)(span - half) * WeUi_Gfx_Effect_Smooth(pcCtx->Progress) + 0.5f);
	WeUi_Gfx_PtPos_t gap0 = GET_MAX(half - parted, 0);
	WeUi_Gfx_PtPos_t gap1 = GET_MIN(half + parted, span);

	if (vertical) {
		if (y < gap0)
			WeUi_Gfx_Effect_RowOld(0, 0, y + parted, width);
		else if (y >= gap1)
			WeUi_Gfx_Effect_RowOld(0, 0, y - parted, width);
		else
			WeUi_Gfx_Effect_RowNew(0, 0, y, width);
		return;
	}
	WeUi_Gfx_Effect_RowOld(0, parted, y, gap0);
	WeUi_Gfx_Effect_RowNew(gap0, gap0, y, gap1 - gap0);
	WeUi_Gfx_Effect_RowOld(gap1, gap1 - parted, y, span - gap1);
}

RET_STATE_t WeUi_Gfx_Effect_Curtain(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
									WEUI_GFX_EFFECT_AXIS_t axis) {
	return WeUi_Gfx_Effect_RunRows(WeUi_Gfx_Effect_Row_Curtain, NULL, frames, &axis,
								   renderIncoming);
}
