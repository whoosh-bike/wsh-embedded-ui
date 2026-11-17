#include "weui_gfx_effect_melt.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

#define WEUI_GFX_EFFECT_MELT_SPREAD 0.4f /* of the run the latest column starts by */

static void WeUi_Gfx_Effect_Compose_Melt(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	u8 columnPx				= *(const u8*)pcCtx->ParamsPtr;
	WeUi_Gfx_PtPos_t height = (WeUi_Gfx_PtPos_t)pcCtx->Height;

	for (WeUi_Gfx_PtPos_t x = 0; x < (WeUi_Gfx_PtPos_t)pcCtx->Width; x++) {
		float run = WeUi_Gfx_Effect_ColumnRun(pcCtx, x, columnPx, WEUI_GFX_EFFECT_MELT_SPREAD);
		WeUi_Gfx_PtPos_t sag = (WeUi_Gfx_PtPos_t)((float)height * run * run + 0.5f);

		/* The old column, dropped by its sag; above it the incoming one shows */
		for (WeUi_Gfx_PtPos_t y = sag; y < height; y++)
			WeUi_Gfx_Effect_PutOld(x, y, x, y - sag);
	}
}

RET_STATE_t WeUi_Gfx_Effect_Melt(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
								 u8 columnPx) {
	return WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_Compose_Melt, frames, &columnPx,
										 renderIncoming);
}
