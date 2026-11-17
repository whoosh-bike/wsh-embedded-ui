#include "weui_gfx_effect_rain.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

#define WEUI_GFX_EFFECT_RAIN_SPREAD 0.5f /* of the run the latest column starts by */

static void WeUi_Gfx_Effect_Compose_Rain(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	u8 columnPx				= *(const u8*)pcCtx->ParamsPtr;
	WeUi_Gfx_PtPos_t height = (WeUi_Gfx_PtPos_t)pcCtx->Height;

	for (WeUi_Gfx_PtPos_t x = 0; x < (WeUi_Gfx_PtPos_t)pcCtx->Width; x++) {
		/* Squared: a run speeds up as it falls */
		float run = WeUi_Gfx_Effect_ColumnRun(pcCtx, x, columnPx, WEUI_GFX_EFFECT_RAIN_SPREAD);
		WeUi_Gfx_PtPos_t wet = (WeUi_Gfx_PtPos_t)((float)height * run * run + 0.5f);

		for (WeUi_Gfx_PtPos_t y = wet; y < height; y++)
			WeUi_Gfx_Effect_PutOld(x, y, x, y);
	}
}

RET_STATE_t WeUi_Gfx_Effect_Rain(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
								 u8 columnPx) {
	return WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_Compose_Rain, frames, &columnPx,
										 renderIncoming);
}
