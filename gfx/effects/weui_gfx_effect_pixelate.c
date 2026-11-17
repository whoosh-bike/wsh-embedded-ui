#include "weui_gfx_effect_pixelate.h"
#include "mathlib_common.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"
#include "weui_gfx_transform.h"

/* Coarsest block, as a share of the panel height: on 25 rows about three blocks
 * stand for the whole picture - abstract enough to hide the swap */
#define WEUI_GFX_EFFECT_PIXELATE_BLOCK_SHARE 0.35f

/* A frame's grid, laid on its first row, and the means of the row of blocks being composed:
 * worked out on its first row, from rows not composed yet */
typedef struct {
	WeUi_Gfx_PtPos_t Block;
	WeUi_Gfx_PtPos_t StartX;
	WeUi_Gfx_PtPos_t StartY;
	s32 MeansRow;
	WeUi_Gfx_ColorRaw_t Means[WEUI_DISPLAY_WIDTH + 2]; /* a block a px, cut at both edges */
} WeUi_Gfx_Effect_PixelateGrid_t;

static WeUi_Gfx_Effect_PixelateGrid_t WeUi_Gfx_Effect_Grid;

/** @brief The mean of a block in the 5-6-5 a frame keeps, while the row is 5-6-5 too */
static WeUi_Gfx_ColorRaw_t WeUi_Gfx_Effect_PixelateMean565(const WeUi_Gfx_FrameMem_t* pcSrc,
														   WeUi_Gfx_PtPos_t x0, WeUi_Gfx_PtPos_t y0,
														   WeUi_Gfx_PtPos_t x1,
														   WeUi_Gfx_PtPos_t y1) {
	bool be = pcSrc->Px == WEUI_GFX_FRAME_PX_RGB565_BE;
	u32 red = 0, green = 0, blue = 0;
	u32 count = (u32)(x1 - x0) * (u32)(y1 - y0);

	for (WeUi_Gfx_PtPos_t y = y0; y < y1; y++) {
		const u16* pcRow = &((const u16*)pcSrc->pBuf)[WeUi_Gfx_Effect_At(pcSrc, x0, y)];
		for (WeUi_Gfx_PtPos_t i = 0; i < x1 - x0; i++) {
			u32 px = be ? __builtin_bswap16(pcRow[i]) : pcRow[i];
			red += px >> 11;
			green += (px >> 5) & 0x3Fu;
			blue += px & 0x1Fu;
		}
	}
	return WeUi_Gfx_Adapter_RawFrom565(
		(u16)((red / count) << 11 | (green / count) << 5 | blue / count));
}

/** @brief The mean colour of a block, off the old picture or the new one */
static WeUi_Gfx_ColorRaw_t WeUi_Gfx_Effect_PixelateMean(WeUi_Gfx_PtPos_t x0, WeUi_Gfx_PtPos_t y0,
														WeUi_Gfx_PtPos_t x1, WeUi_Gfx_PtPos_t y1,
														bool fromOld) {
	if (WeUi_Gfx_Effect_Out565())
		return WeUi_Gfx_Effect_PixelateMean565(
			fromOld ? &WeUi_Gfx_Effect_Mem.Old : &WeUi_Gfx_Effect_Mem.New, x0, y0, x1, y1);

	u32 red = 0, count = 0;
#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
	u32 green = 0, blue = 0;
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */

	for (WeUi_Gfx_PtPos_t y = y0; y < y1; y++) {
		for (WeUi_Gfx_PtPos_t x = x0; x < x1; x++) {
			WeUi_Gfx_ColorRaw_t raw =
				fromOld ? WeUi_Gfx_Effect_GetOld(x, y) : WeUi_Gfx_Effect_GetNew(x, y);
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
	return WeUi_Gfx_Transform_Color2Bytes(mean);
}

/* Block j of the row is j blocks from the grid's start; the edge ones may fall wholly off */
static void WeUi_Gfx_Effect_PixelateSpan(const WeUi_Gfx_Effect_Ctx_t* pcCtx, u32 j,
										 WeUi_Gfx_PtPos_t* pX0, WeUi_Gfx_PtPos_t* pX1) {
	const WeUi_Gfx_Effect_PixelateGrid_t* pcG = &WeUi_Gfx_Effect_Grid;
	WeUi_Gfx_PtPos_t x0						  = pcG->StartX + (WeUi_Gfx_PtPos_t)j * pcG->Block;
	*pX0									  = GET_MAX(x0, 0);
	*pX1 = GET_MIN(x0 + pcG->Block, (WeUi_Gfx_PtPos_t)pcCtx->Width);
}

static void WeUi_Gfx_Effect_Row_Pixelate(const WeUi_Gfx_Effect_Ctx_t* pcCtx, WeUi_Gfx_PtPos_t y) {
	WeUi_Gfx_Effect_PixelateGrid_t* pG = &WeUi_Gfx_Effect_Grid;
	WeUi_Gfx_PtPos_t width			   = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	WeUi_Gfx_PtPos_t height			   = (WeUi_Gfx_PtPos_t)pcCtx->Height;
	/* Coarser up to halfway, finer after: a tent over the run */
	bool oldSide					   = pcCtx->Progress < 0.5f;
	ASSERT_CHECK(pcCtx->Width <= WEUI_DISPLAY_WIDTH);

	if (y == 0) {
		float tent	   = oldSide ? 2.0f * pcCtx->Progress : 2.0f * (1.0f - pcCtx->Progress);
		float blockMax = (float)height * WEUI_GFX_EFFECT_PIXELATE_BLOCK_SHARE;
		pG->Block	   = (WeUi_Gfx_PtPos_t)(1.0f + (blockMax - 1.0f) * tent + 0.5f);
		/* Blocks are laid from the centre, so the coarsest stay symmetric on the panel */
		pG->StartX	   = (width / 2) % pG->Block - pG->Block;
		pG->StartY	   = (height / 2) % pG->Block - pG->Block;
		pG->MeansRow   = -1;
	}
	/* Sharp again: the incoming frame as drawn */
	if (pG->Block <= 1 && !oldSide) {
		WeUi_Gfx_Effect_RowNew(0, 0, y, width);
		return;
	}

	s32 k				= (y - pG->StartY) / pG->Block;
	WeUi_Gfx_PtPos_t y0 = pG->StartY + (WeUi_Gfx_PtPos_t)k * pG->Block;
	u32 blocks			= (u32)((width - pG->StartX + pG->Block - 1) / pG->Block);
	WeUi_Gfx_PtPos_t x0, x1;
	if (k != pG->MeansRow) {
		WeUi_Gfx_PtPos_t ya = GET_MAX(y0, 0), yb = GET_MIN(y0 + pG->Block, height);
		for (u32 j = 0; j < blocks; j++) {
			WeUi_Gfx_Effect_PixelateSpan(pcCtx, j, &x0, &x1);
			if (x1 > x0)
				pG->Means[j] = WeUi_Gfx_Effect_PixelateMean(x0, ya, x1, yb, oldSide);
		}
		pG->MeansRow = k;
	}
	for (u32 j = 0; j < blocks; j++) {
		WeUi_Gfx_Effect_PixelateSpan(pcCtx, j, &x0, &x1);
		WeUi_Gfx_Effect_RowFill(x0, x1 - x0, pG->Means[j]);
	}
}

RET_STATE_t WeUi_Gfx_Effect_Pixelate(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames) {
	return WeUi_Gfx_Effect_RunRows(WeUi_Gfx_Effect_Row_Pixelate, NULL, frames, NULL,
								   renderIncoming);
}
