#include "weui_gfx_effect_blinds.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

#define WEUI_GFX_EFFECT_BLINDS_SLATS 5 /* down the panel */

static void WeUi_Gfx_Effect_Compose_Blinds(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	WeUi_Gfx_PtPos_t height = (WeUi_Gfx_PtPos_t)pcCtx->Height;
	float slat				= (float)height / WEUI_GFX_EFFECT_BLINDS_SLATS;
	float open				= slat * WeUi_Gfx_Effect_Smooth(pcCtx->Progress);

	for (WeUi_Gfx_PtPos_t y = 0; y < height; y++) {
		float within = (float)y - slat * (float)(u32)((float)y / slat);
		if (within < open)
			continue;

		for (WeUi_Gfx_PtPos_t x = 0; x < (WeUi_Gfx_PtPos_t)pcCtx->Width; x++)
			WeUi_Gfx_Effect_PutOld(x, y, x, y);
	}
}

RET_STATE_t WeUi_Gfx_Effect_Blinds(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_Compose_Blinds, frames, NULL,
										 renderIncoming);
}
