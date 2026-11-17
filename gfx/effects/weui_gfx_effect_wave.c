#include "weui_gfx_effect_wave.h"
#include "mathlib_common.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

#define WEUI_GFX_EFFECT_WAVE_AMP_PX 6.0f /* how far the front wavers either way */
#define WEUI_GFX_EFFECT_WAVE_LENGTH 0.5f /* rad per row */
#define WEUI_GFX_EFFECT_WAVE_SWELL	9.0f /* rad the waves travel over the run */

static void WeUi_Gfx_Effect_Compose_Wave(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	/* From fully off the left edge to fully off the right, wobble included */
	float travel = ((float)pcCtx->Width + 2.0f * WEUI_GFX_EFFECT_WAVE_AMP_PX) *
					   WeUi_Gfx_Effect_Smooth(pcCtx->Progress) -
				   WEUI_GFX_EFFECT_WAVE_AMP_PX;
	float swell	 = WEUI_GFX_EFFECT_WAVE_SWELL * pcCtx->Progress;

	for (WeUi_Gfx_PtPos_t y = 0; y < (WeUi_Gfx_PtPos_t)pcCtx->Height; y++) {
		float front = travel + WEUI_GFX_EFFECT_WAVE_AMP_PX *
								   sinf((float)y * WEUI_GFX_EFFECT_WAVE_LENGTH + swell);

		for (WeUi_Gfx_PtPos_t x = 0; x < (WeUi_Gfx_PtPos_t)pcCtx->Width; x++)
			if ((float)x >= front)
				WeUi_Gfx_Effect_PutOld(x, y, x, y);
	}
}

RET_STATE_t WeUi_Gfx_Effect_Wave(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_Compose_Wave, frames, NULL,
										 renderIncoming);
}
