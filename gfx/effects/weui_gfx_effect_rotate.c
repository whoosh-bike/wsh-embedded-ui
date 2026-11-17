#include "weui_gfx_effect_rotate.h"
#include "mathlib_common.h"
#include "weui_gfx_base.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

/* Edge-on the card is this dark: the turn reads as depth, not as a squeeze */
#define WEUI_GFX_EFFECT_ROTATE_EDGE_BRI 0.3f

/**
 * @brief Squeeze one row towards its centre to `width` of its span, black beside it.
 *
 * Off the aux frame the source is untouched; off the drawing frame each pixel
 * reads one further out than itself, so walking from the centre outwards it reads
 * what is not written yet - the same in-place shrink as the zoom's.
 */
static void WeUi_Gfx_Effect_RotateRow(WeUi_Gfx_PtPos_t y, WeUi_Gfx_PtPos_t span, float width,
									  bool fromAux) {
	float centre			  = ((float)span - 1.0f) / 2.0f;
	float reach				  = (float)span / 2.0f * width;
	WeUi_Gfx_ColorRaw_t blank = WeUi_Gfx_Effect_Blank();

	for (WeUi_Gfx_PtPos_t step = 0; step <= span / 2; step++) {
		WeUi_Gfx_PtPos_t sides[] = {(WeUi_Gfx_PtPos_t)(centre + 0.5f) + step,
									(WeUi_Gfx_PtPos_t)centre - step};

		for (u32 side = 0; side < NUM_ELEMENTS(sides); side++) {
			WeUi_Gfx_PtPos_t pos = sides[side];
			if (pos < 0 || pos >= span)
				continue;

			WeUi_Gfx_Point_t dst = {.X = pos, .Y = y};
			float off			 = (float)pos - centre;

			if (off > reach || -off > reach) {
				WeUi_Gfx_Adapter_SetPixelRaw(&dst, blank);
				continue;
			}

			WeUi_Gfx_PtPos_t src = (WeUi_Gfx_PtPos_t)(centre + off / width + 0.5f);
			src					 = CUT_MIN_MAX(src, 0, span - 1);

			WeUi_Gfx_Point_t from = {.X = src, .Y = y};
			WeUi_Gfx_Adapter_SetPixelRaw(&dst, fromAux ? WeUi_Gfx_Adapter_GetPixelRaw_Aux(&from)
													   : WeUi_Gfx_Adapter_GetPixelRaw(&from));
		}
	}
}

static void WeUi_Gfx_Effect_Compose_Rotate(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	/* The card's apparent width, |cos| of the turn: the old side going, then the new */
	float turn	 = cosf(pcCtx->Progress * WEUI_PI_F);
	bool oldSide = turn > 0.0f;
	float width	 = oldSide ? turn : -turn;

	if (width >= 1.0f)
		return;

	for (WeUi_Gfx_PtPos_t y = 0; y < (WeUi_Gfx_PtPos_t)pcCtx->Height; y++)
		WeUi_Gfx_Effect_RotateRow(y, (WeUi_Gfx_PtPos_t)pcCtx->Width, width, oldSide);

	WeUi_Gfx_Frame_Fade(WEUI_GFX_EFFECT_ROTATE_EDGE_BRI +
						(1.0f - WEUI_GFX_EFFECT_ROTATE_EDGE_BRI) * width);
}

RET_STATE_t WeUi_Gfx_Effect_Rotate(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_Compose_Rotate, frames, NULL,
										 renderIncoming);
}
