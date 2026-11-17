#include "weui_gfx_effect_ripple.h"
#include "mathlib_common.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

#define WEUI_GFX_EFFECT_RIPPLE_WAVES  7.0f /* crests round the ring */
#define WEUI_GFX_EFFECT_RIPPLE_WOBBLE 1.5f /* px the ring wavers by */
#define WEUI_GFX_EFFECT_RIPPLE_SPIN	  6.0f /* rad the waves travel round over the run */
#define WEUI_GFX_EFFECT_RIPPLE_CREST  2.0f /* px of lit water behind the front */
#define WEUI_GFX_EFFECT_RIPPLE_SHINE  0.6f /* how far the crest is lifted towards white */

/* Distance from the centre in x pixels: a circle's, or a diamond's whose corners
 * sit on the panel's - the y axis stretched to the panel's aspect */
static float WeUi_Gfx_Effect_RippleDist(WEUI_GFX_EFFECT_RIPPLE_SHAPE_t shape, float dx, float dy,
										float aspect) {
	if (shape == WEUI_GFX_EFFECT_RIPPLE_DIAMOND)
		return fabsf(dx) + fabsf(dy) * aspect;

	return sqrtf(dx * dx + dy * dy);
}

static void WeUi_Gfx_Effect_Compose_Ripple(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	WEUI_GFX_EFFECT_RIPPLE_SHAPE_t shape = *(const WEUI_GFX_EFFECT_RIPPLE_SHAPE_t*)pcCtx->ParamsPtr;
	float cx							 = ((float)pcCtx->Width - 1.0f) / 2.0f;
	float cy							 = ((float)pcCtx->Height - 1.0f) / 2.0f;
	float aspect						 = (cx + 1.0f) / (cy + 1.0f);

	/* Far enough that the wobble and the crest have left the corners at the end */
	float reach = WeUi_Gfx_Effect_RippleDist(shape, cx + 1.0f, cy + 1.0f, aspect) +
				  WEUI_GFX_EFFECT_RIPPLE_WOBBLE + WEUI_GFX_EFFECT_RIPPLE_CREST;
	float front = reach * pcCtx->Progress;
	float spin	= WEUI_GFX_EFFECT_RIPPLE_SPIN * pcCtx->Progress;

	for (WeUi_Gfx_PtPos_t y = 0; y < (WeUi_Gfx_PtPos_t)pcCtx->Height; y++) {
		for (WeUi_Gfx_PtPos_t x = 0; x < (WeUi_Gfx_PtPos_t)pcCtx->Width; x++) {
			float dx	= (float)x - cx;
			float dy	= (float)y - cy;
			float edge	= front + WEUI_GFX_EFFECT_RIPPLE_WOBBLE *
									  sinf(atan2f(dy, dx) * WEUI_GFX_EFFECT_RIPPLE_WAVES + spin);
			float depth = edge - WeUi_Gfx_Effect_RippleDist(shape, dx, dy, aspect);

			if (depth < 0.0f) {
				WeUi_Gfx_Effect_PutOld(x, y, x, y);
				continue;
			}

			if (depth < WEUI_GFX_EFFECT_RIPPLE_CREST) {
				WeUi_Gfx_Point_t pt = {.X = x, .Y = y};
				float shine =
					WEUI_GFX_EFFECT_RIPPLE_SHINE * (1.0f - depth / WEUI_GFX_EFFECT_RIPPLE_CREST);
				WeUi_Gfx_Adapter_SetPixelRaw(
					&pt, WeUi_Gfx_Effect_Lighten(WeUi_Gfx_Adapter_GetPixelRaw(&pt), shine));
			}
		}
	}
}

RET_STATE_t WeUi_Gfx_Effect_Ripple(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
								   WEUI_GFX_EFFECT_RIPPLE_SHAPE_t shape) {
	return WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_Compose_Ripple, frames, &shape,
										 renderIncoming);
}
