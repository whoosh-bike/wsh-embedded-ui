#include "weui_gfx_effect_fade.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

static WeUi_Gfx_ColorChannel_t WeUi_Gfx_Effect_FadeMix(WeUi_Gfx_ColorChannel_t from,
													   WeUi_Gfx_ColorChannel_t to, float mix) {
	return (WeUi_Gfx_ColorChannel_t)((float)from + ((float)to - (float)from) * mix + 0.5f);
}

/* `be` a constant at each call: the byte order is not asked again a pixel. Where both pictures
 * agree - a background - there is nothing to mix */
static inline void WeUi_Gfx_Effect_FadeRow565(const WeUi_Gfx_Effect_Rows565_t* pcRows,
											  WeUi_Gfx_PtPos_t width, u32 weight, bool be) {
	for (WeUi_Gfx_PtPos_t x = 0; x < width; x++) {
		u16 old = pcRows->pcOld[x], next = pcRows->pcNew[x];
		pcRows->pOut[x] =
			old == next ? old
						: WeUi_Gfx_Effect_Fold565(
							  WeUi_Gfx_Effect_Mix565(WeUi_Gfx_Effect_Spread565(old, be),
													 WeUi_Gfx_Effect_Spread565(next, be), weight),
							  be);
	}
}

static void WeUi_Gfx_Effect_Row_Fade(const WeUi_Gfx_Effect_Ctx_t* pcCtx, WeUi_Gfx_PtPos_t y) {
	float mix			   = WeUi_Gfx_Effect_Smooth(pcCtx->Progress);
	WeUi_Gfx_PtPos_t width = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	WeUi_Gfx_Effect_Rows565_t rows;

	if (WeUi_Gfx_Effect_Rows565(y, &rows)) {
		u32 weight = WeUi_Gfx_Effect_Weight565(mix);
		if (rows.Be)
			WeUi_Gfx_Effect_FadeRow565(&rows, width, weight, true);
		else
			WeUi_Gfx_Effect_FadeRow565(&rows, width, weight, false);
		return;
	}
	for (WeUi_Gfx_PtPos_t x = 0; x < width; x++) {
		WeUi_Gfx_Color_t old  = WeUi_Gfx_Transform_Bytes2Color(WeUi_Gfx_Effect_GetOld(x, y));
		WeUi_Gfx_Color_t next = WeUi_Gfx_Transform_Bytes2Color(WeUi_Gfx_Effect_GetNew(x, y));

		next.R = WeUi_Gfx_Effect_FadeMix(old.R, next.R, mix);
#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
		next.G = WeUi_Gfx_Effect_FadeMix(old.G, next.G, mix);
		next.B = WeUi_Gfx_Effect_FadeMix(old.B, next.B, mix);
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */

		WeUi_Gfx_Effect_RowSet(x, WeUi_Gfx_Transform_Color2Bytes(next));
	}
}

RET_STATE_t WeUi_Gfx_Effect_Fade(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunRows(WeUi_Gfx_Effect_Row_Fade, NULL, frames, NULL, renderIncoming);
}
