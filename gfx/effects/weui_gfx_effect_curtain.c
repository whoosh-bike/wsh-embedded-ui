#include "weui_gfx_effect_curtain.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

static void WeUi_Gfx_Effect_Compose_Curtain(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	bool vertical =
		*(const WEUI_GFX_EFFECT_AXIS_t*)pcCtx->ParamsPtr == WEUI_GFX_EFFECT_AXIS_VERTICAL;

	/* Worked along the parting axis and across it, then mapped back to x and y */
	WeUi_Gfx_PtPos_t span  = (WeUi_Gfx_PtPos_t)(vertical ? pcCtx->Height : pcCtx->Width);
	WeUi_Gfx_PtPos_t lines = (WeUi_Gfx_PtPos_t)(vertical ? pcCtx->Width : pcCtx->Height);
	WeUi_Gfx_PtPos_t half  = span / 2;

	/* The curtain gathers way, then slows into the wings */
	WeUi_Gfx_PtPos_t parted =
		(WeUi_Gfx_PtPos_t)((float)(span - half) * WeUi_Gfx_Effect_Smooth(pcCtx->Progress) + 0.5f);

	for (WeUi_Gfx_PtPos_t line = 0; line < lines; line++) {
		for (WeUi_Gfx_PtPos_t a = 0; a < span; a++) {
			/* Each half keeps its own picture and slides out with it */
			WeUi_Gfx_PtPos_t from;
			if (a < half - parted)
				from = a + parted;
			else if (a >= half + parted)
				from = a - parted;
			else
				continue;

			if (vertical)
				WeUi_Gfx_Effect_PutOld(line, a, line, from);
			else
				WeUi_Gfx_Effect_PutOld(a, line, from, line);
		}
	}
}

RET_STATE_t WeUi_Gfx_Effect_Curtain(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
									WEUI_GFX_EFFECT_AXIS_t axis) {
	return WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_Compose_Curtain, frames, &axis,
										 renderIncoming);
}
