#include "weui_gfx_effect_pixelate.h"
#include "mathlib_common.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_effect_runner.h"
#include "weui_gfx_transform.h"

/* Coarsest block, as a share of the panel height: on 25 rows about three blocks
 * stand for the whole picture - abstract enough to hide the swap */
#define WEUI_GFX_EFFECT_PIXELATE_BLOCK_SHARE 0.35f

/** @brief One block of the drawing frame filled with its mean, off aux or drawing */
static void WeUi_Gfx_Effect_PixelateBlock(WeUi_Gfx_PtPos_t x0, WeUi_Gfx_PtPos_t y0,
										  WeUi_Gfx_PtPos_t x1, WeUi_Gfx_PtPos_t y1, bool fromAux) {
	/* The edge blocks of a centred grid can fall wholly off the panel */
	if (x1 <= x0 || y1 <= y0)
		return;

	u32 red = 0, count = 0;
#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
	u32 green = 0, blue = 0;
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */

	for (WeUi_Gfx_PtPos_t y = y0; y < y1; y++) {
		for (WeUi_Gfx_PtPos_t x = x0; x < x1; x++) {
			WeUi_Gfx_Point_t pt = {.X = x, .Y = y};
			WeUi_Gfx_ColorRaw_t raw =
				fromAux ? WeUi_Gfx_Adapter_GetPixelRaw_Aux(&pt) : WeUi_Gfx_Adapter_GetPixelRaw(&pt);
			WeUi_Gfx_Color_t color = WeUi_Gfx_Transform_Bytes2Color(raw);

			red += color.R;
#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
			green += color.G;
			blue += color.B;
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */
			count++;
		}
	}

	WeUi_Gfx_Color_t mean = {.R = (WeUi_Gfx_ColorChannel_t)(red / count)};
#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
	mean.G = (WeUi_Gfx_ColorChannel_t)(green / count);
	mean.B = (WeUi_Gfx_ColorChannel_t)(blue / count);
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */
	WeUi_Gfx_ColorRaw_t raw = WeUi_Gfx_Transform_Color2Bytes(mean);

	for (WeUi_Gfx_PtPos_t y = y0; y < y1; y++) {
		for (WeUi_Gfx_PtPos_t x = x0; x < x1; x++) {
			WeUi_Gfx_Point_t pt = {.X = x, .Y = y};
			WeUi_Gfx_Adapter_SetPixelRaw(&pt, raw);
		}
	}
}

static void WeUi_Gfx_Effect_Compose_Pixelate(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	WeUi_Gfx_PtPos_t width	= (WeUi_Gfx_PtPos_t)pcCtx->Width;
	WeUi_Gfx_PtPos_t height = (WeUi_Gfx_PtPos_t)pcCtx->Height;

	/* Coarser up to halfway, finer after: a tent over the run */
	bool oldSide		   = pcCtx->Progress < 0.5f;
	float tent			   = oldSide ? 2.0f * pcCtx->Progress : 2.0f * (1.0f - pcCtx->Progress);
	float blockMax		   = (float)height * WEUI_GFX_EFFECT_PIXELATE_BLOCK_SHARE;
	WeUi_Gfx_PtPos_t block = (WeUi_Gfx_PtPos_t)(1.0f + (blockMax - 1.0f) * tent + 0.5f);

	/* Sharp again: the incoming frame as drawn */
	if (block <= 1 && !oldSide)
		return;

	/* Blocks are laid from the centre, so the coarsest stay symmetric on the panel */
	WeUi_Gfx_PtPos_t startX = (width / 2) % block - block;
	WeUi_Gfx_PtPos_t startY = (height / 2) % block - block;

	for (WeUi_Gfx_PtPos_t y0 = startY; y0 < height; y0 += block) {
		for (WeUi_Gfx_PtPos_t x0 = startX; x0 < width; x0 += block) {
			WeUi_Gfx_Effect_PixelateBlock(GET_MAX(x0, 0), GET_MAX(y0, 0),
										  GET_MIN(x0 + block, width), GET_MIN(y0 + block, height),
										  oldSide);
		}
	}
}

RET_STATE_t WeUi_Gfx_Effect_Pixelate(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_Compose_Pixelate, frames, NULL,
										 renderIncoming);
}
