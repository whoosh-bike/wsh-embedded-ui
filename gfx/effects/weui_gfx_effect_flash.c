#include "weui_gfx_effect_flash.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

/* The old picture burns out to white the first half, the incoming one comes up out of it the
 * second */
static void WeUi_Gfx_Effect_Row_Flash(const WeUi_Gfx_Effect_Ctx_t* pcCtx, WeUi_Gfx_PtPos_t y) {
	WeUi_Gfx_PtPos_t width = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	float half			   = 2.0f * pcCtx->Progress;
	bool old			   = half < 1.0f;
	float shine			   = WeUi_Gfx_Effect_Smooth(old ? half : 2.0f - half);

	WeUi_Gfx_Effect_Rows565_t rows;

	if (WeUi_Gfx_Effect_Rows565(y, &rows)) {
		const u16* pcSrc = old ? rows.pcOld : rows.pcNew;
		u32 weight		 = WeUi_Gfx_Effect_Weight565(shine);
		for (WeUi_Gfx_PtPos_t x = 0; x < width; x++)
			rows.pOut[x] = WeUi_Gfx_Effect_Fold565(
				WeUi_Gfx_Effect_Mix565(WeUi_Gfx_Effect_Spread565(pcSrc[x], rows.Be),
									   WEUI_GFX_EFFECT_565_SPREAD, weight),
				rows.Be);
		return;
	}
	for (WeUi_Gfx_PtPos_t x = 0; x < width; x++) {
		WeUi_Gfx_ColorRaw_t raw = old ? WeUi_Gfx_Effect_GetOld(x, y) : WeUi_Gfx_Effect_GetNew(x, y);
		WeUi_Gfx_Effect_RowSet(x, WeUi_Gfx_Effect_Lighten(raw, shine));
	}
}

RET_STATE_t WeUi_Gfx_Effect_Flash(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunRows(WeUi_Gfx_Effect_Row_Flash, NULL, frames, NULL, renderIncoming);
}
