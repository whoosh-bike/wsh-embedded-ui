#include "weui_gfx_effect_rotate.h"
#include "mathlib_common.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

/* Edge-on the card is this dark: the turn reads as depth, not as a squeeze */
#define WEUI_GFX_EFFECT_ROTATE_EDGE_BRI 0.3f

/* Where each column of a frame reads from, -1 for black beside the card: worked out on the
 * frame's first row, the same for every row */
static WeUi_Gfx_PtPos_t WeUi_Gfx_Effect_RotateSrc[WEUI_DISPLAY_WIDTH];

/* Every row squeezed towards its centre to the card's apparent width, |cos| of the turn: the
 * old side going, then the new; the whole frame darker edge-on */
static void WeUi_Gfx_Effect_Row_Rotate(const WeUi_Gfx_Effect_Ctx_t* pcCtx, WeUi_Gfx_PtPos_t y) {
	WeUi_Gfx_PtPos_t span = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	float turn			  = cosf(pcCtx->Progress * WEUI_PI_F);
	bool oldSide		  = turn > 0.0f;
	float width			  = oldSide ? turn : -turn;
	ASSERT_CHECK(pcCtx->Width <= WEUI_DISPLAY_WIDTH);

	if (width >= 1.0f) {
		WeUi_Gfx_Effect_RowNew(0, 0, y, span);
		return;
	}
	if (y == 0) {
		float centre = ((float)span - 1.0f) / 2.0f;
		float reach	 = (float)span / 2.0f * width;
		for (WeUi_Gfx_PtPos_t pos = 0; pos < span; pos++) {
			float off = (float)pos - centre;
			if (off > reach || -off > reach) {
				WeUi_Gfx_Effect_RotateSrc[pos] = -1;
				continue;
			}
			WeUi_Gfx_PtPos_t src		   = (WeUi_Gfx_PtPos_t)(centre + off / width + 0.5f);
			WeUi_Gfx_Effect_RotateSrc[pos] = CUT_MIN_MAX(src, 0, span - 1);
		}
	}

	float bri = WEUI_GFX_EFFECT_ROTATE_EDGE_BRI + (1.0f - WEUI_GFX_EFFECT_ROTATE_EDGE_BRI) * width;
	WeUi_Gfx_ColorRaw_t blank = WeUi_Gfx_Effect_Blank();

	/* In 5-6-5 throughout, the row is gathered, then darkened at once */
	WeUi_Gfx_Effect_Rows565_t rows;
	if (WeUi_Gfx_Effect_Rows565(y, &rows)) {
		const u16* pcSrc = oldSide ? rows.pcOld : rows.pcNew;
		WeUi_Gfx_Effect_RowSet(0, blank);
		u16 blank565 = rows.pOut[0];
		for (WeUi_Gfx_PtPos_t pos = 0; pos < span; pos++) {
			WeUi_Gfx_PtPos_t src = WeUi_Gfx_Effect_RotateSrc[pos];
			rows.pOut[pos]		 = src < 0 ? blank565 : pcSrc[src];
		}
		WeUi_Gfx_Effect_RowFade565(0, span, bri);
		return;
	}
	blank = WeUi_Gfx_Effect_Faded(blank, bri);
	for (WeUi_Gfx_PtPos_t pos = 0; pos < span; pos++) {
		WeUi_Gfx_PtPos_t src = WeUi_Gfx_Effect_RotateSrc[pos];
		if (src < 0)
			WeUi_Gfx_Effect_RowSet(pos, blank);
		else
			WeUi_Gfx_Effect_RowFaded(oldSide, pos, src, y, 1, bri);
	}
}

RET_STATE_t WeUi_Gfx_Effect_Rotate(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunRows(WeUi_Gfx_Effect_Row_Rotate, NULL, frames, NULL, renderIncoming);
}
