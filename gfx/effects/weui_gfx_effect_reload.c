#include "weui_gfx_effect_reload.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

static void WeUi_Gfx_Effect_Compose_Reload(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	WeUi_Gfx_PtPos_t width = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	float half			   = 2.0f * pcCtx->Progress;
	bool wiping			   = half < 1.0f;
	WeUi_Gfx_PtPos_t edge = (WeUi_Gfx_PtPos_t)((float)width * (wiping ? half : half - 1.0f) + 0.5f);
	WeUi_Gfx_ColorRaw_t blank = WeUi_Gfx_Effect_Blank();

	/* Wiping, the old picture stands right of the edge; loading, the incoming one
	 * stands left of it. Everything else is dark */
	for (WeUi_Gfx_PtPos_t y = 0; y < (WeUi_Gfx_PtPos_t)pcCtx->Height; y++) {
		for (WeUi_Gfx_PtPos_t x = 0; x < width; x++) {
			if (wiping && x >= edge) {
				WeUi_Gfx_Effect_PutOld(x, y, x, y);
				continue;
			}
			if (!wiping && x < edge)
				continue;

			WeUi_Gfx_Point_t pt = {.X = x, .Y = y};
			WeUi_Gfx_Adapter_SetPixelRaw(&pt, blank);
		}
	}
}

RET_STATE_t WeUi_Gfx_Effect_Reload(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_Compose_Reload, frames, NULL,
										 renderIncoming);
}
