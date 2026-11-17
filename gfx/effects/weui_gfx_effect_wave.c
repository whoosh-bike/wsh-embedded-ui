#include "weui_gfx_effect_wave.h"
#include "mathlib_common.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

/* In shares of the panel, so a big one waves as an LED one does */
#define WEUI_GFX_EFFECT_WAVE_AMP_DIV 15.0f /* the front wavers this part of the width either way */
#define WEUI_GFX_EFFECT_WAVE_RAD	 12.5f /* of the wave down the panel: two crests */
#define WEUI_GFX_EFFECT_WAVE_SWELL	 9.0f  /* rad the waves travel over the run */

static void WeUi_Gfx_Effect_Row_Wave(const WeUi_Gfx_Effect_Ctx_t* pcCtx, WeUi_Gfx_PtPos_t y) {
	WeUi_Gfx_PtPos_t width = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	float amp			   = (float)pcCtx->Width / WEUI_GFX_EFFECT_WAVE_AMP_DIV;
	float length		   = WEUI_GFX_EFFECT_WAVE_RAD / (float)pcCtx->Height;
	/* From fully off the left edge to fully off the right, wobble included */
	float travel =
		((float)pcCtx->Width + 2.0f * amp) * WeUi_Gfx_Effect_Smooth(pcCtx->Progress) - amp;
	float swell = WEUI_GFX_EFFECT_WAVE_SWELL * pcCtx->Progress;
	float front = travel + amp * sinf((float)y * length + swell);
	/* The first column at or past the front: the old picture from there on */
	WeUi_Gfx_PtPos_t edge = (WeUi_Gfx_PtPos_t)CUT_MIN_MAX(ceilf(front), 0.0f, (float)width);

	WeUi_Gfx_Effect_RowNew(0, 0, y, edge);
	WeUi_Gfx_Effect_RowOld(edge, edge, y, width - edge);
}

RET_STATE_t WeUi_Gfx_Effect_Wave(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunRows(WeUi_Gfx_Effect_Row_Wave, NULL, frames, NULL, renderIncoming);
}
