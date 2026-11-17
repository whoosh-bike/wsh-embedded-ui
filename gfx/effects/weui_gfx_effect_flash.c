#include "weui_gfx_effect_flash.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

static void WeUi_Gfx_Effect_Flash_Whiten(const WeUi_Gfx_Effect_Ctx_t* pcCtx, float shine) {
	for (WeUi_Gfx_PtPos_t y = 0; y < (WeUi_Gfx_PtPos_t)pcCtx->Height; y++) {
		for (WeUi_Gfx_PtPos_t x = 0; x < (WeUi_Gfx_PtPos_t)pcCtx->Width; x++) {
			WeUi_Gfx_Point_t pt = {.X = x, .Y = y};
			WeUi_Gfx_Adapter_SetPixelRaw(
				&pt, WeUi_Gfx_Effect_Lighten(WeUi_Gfx_Adapter_GetPixelRaw(&pt), shine));
		}
	}
}

static void WeUi_Gfx_Effect_Compose_Flash(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	float half = 2.0f * pcCtx->Progress;

	if (half < 1.0f) {
		WeUi_Gfx_Effect_PutOldFrame(pcCtx);
		WeUi_Gfx_Effect_Flash_Whiten(pcCtx, WeUi_Gfx_Effect_Smooth(half));
		return;
	}

	WeUi_Gfx_Effect_Flash_Whiten(pcCtx, WeUi_Gfx_Effect_Smooth(2.0f - half));
}

RET_STATE_t WeUi_Gfx_Effect_Flash(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_Compose_Flash, frames, NULL,
										 renderIncoming);
}
