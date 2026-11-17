#include "weui_gfx_effect_cover.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

typedef struct {
	WEUI_GFX_EFFECT_SLIDE_t Slide;
	bool Cover; /* the incoming scene moves; else the old picture does */
} WeUi_Gfx_Effect_CoverParams_t;

/* Px the moving picture is shifted by along the slide, and whether the slide runs
 * against the axis */
static void WeUi_Gfx_Effect_CoverShift(WEUI_GFX_EFFECT_SLIDE_t slide, WeUi_Gfx_PtPos_t travel,
									   WeUi_Gfx_PtPos_t* pDx, WeUi_Gfx_PtPos_t* pDy) {
	*pDx = 0;
	*pDy = 0;

	switch (slide) {
		case WEUI_GFX_EFFECT_SLIDE_LEFT:
			*pDx = -travel;
			break;
		case WEUI_GFX_EFFECT_SLIDE_DOWN:
			*pDy = travel;
			break;
		case WEUI_GFX_EFFECT_SLIDE_UP:
			*pDy = -travel;
			break;
		case WEUI_GFX_EFFECT_SLIDE_RIGHT:
		default:
			*pDx = travel;
			break;
	}
}

static void WeUi_Gfx_Effect_Compose_Cover(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	const WeUi_Gfx_Effect_CoverParams_t* pcParams = pcCtx->ParamsPtr;
	WeUi_Gfx_PtPos_t width						  = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	WeUi_Gfx_PtPos_t height						  = (WeUi_Gfx_PtPos_t)pcCtx->Height;
	bool vertical								  = pcParams->Slide == WEUI_GFX_EFFECT_SLIDE_DOWN ||
													pcParams->Slide == WEUI_GFX_EFFECT_SLIDE_UP;
	WeUi_Gfx_PtPos_t span						  = vertical ? height : width;

	/* It sets off, then eases onto its place */
	float travel = (float)span * WeUi_Gfx_Effect_Smooth(pcCtx->Progress);

	/* Cover: the incoming one arrives from the far side, span - travel short of home.
	 * Off: the old one leaves, travel past home */
	WeUi_Gfx_PtPos_t offset =
		(WeUi_Gfx_PtPos_t)((pcParams->Cover ? (float)span - travel : travel) + 0.5f);
	WeUi_Gfx_PtPos_t dx, dy;
	WeUi_Gfx_Effect_CoverShift(pcParams->Slide, offset, &dx, &dy);

	/* The incoming frame is drawn in place; covering it has to move, so it is walked
	 * away from the side it reads from, as the push does */
	if (pcParams->Cover) {
		dx		   = -dx;
		dy		   = -dy;
		bool backX = dx < 0, backY = dy < 0;

		for (WeUi_Gfx_PtPos_t iy = 0; iy < height; iy++) {
			WeUi_Gfx_PtPos_t y = backY ? iy : height - 1 - iy;
			for (WeUi_Gfx_PtPos_t ix = 0; ix < width; ix++) {
				WeUi_Gfx_PtPos_t x	= backX ? ix : width - 1 - ix;
				WeUi_Gfx_PtPos_t sx = x - dx, sy = y - dy;

				if (sx < 0 || sx >= width || sy < 0 || sy >= height) {
					WeUi_Gfx_Effect_PutOld(x, y, x, y);
					continue;
				}

				WeUi_Gfx_Effect_MoveNew(x, y, sx, sy);
			}
		}
		return;
	}

	/* Off: the old picture shifted; where it has left, the incoming shows */
	for (WeUi_Gfx_PtPos_t y = 0; y < height; y++) {
		for (WeUi_Gfx_PtPos_t x = 0; x < width; x++) {
			WeUi_Gfx_PtPos_t sx = x - dx, sy = y - dy;
			if (sx < 0 || sx >= width || sy < 0 || sy >= height)
				continue;

			WeUi_Gfx_Effect_PutOld(x, y, sx, sy);
		}
	}
}

/* The same picture a row at a time: the moving picture shifted by (dx, dy), the standing one
 * wherever the moving one is not */
static void WeUi_Gfx_Effect_Row_Cover(const WeUi_Gfx_Effect_Ctx_t* pcCtx, WeUi_Gfx_PtPos_t y) {
	const WeUi_Gfx_Effect_CoverParams_t* pcParams = pcCtx->ParamsPtr;
	WeUi_Gfx_PtPos_t width						  = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	WeUi_Gfx_PtPos_t height						  = (WeUi_Gfx_PtPos_t)pcCtx->Height;
	bool vertical								  = pcParams->Slide == WEUI_GFX_EFFECT_SLIDE_DOWN ||
													pcParams->Slide == WEUI_GFX_EFFECT_SLIDE_UP;
	WeUi_Gfx_PtPos_t span						  = vertical ? height : width;
	float travel = (float)span * WeUi_Gfx_Effect_Smooth(pcCtx->Progress);
	WeUi_Gfx_PtPos_t offset =
		(WeUi_Gfx_PtPos_t)((pcParams->Cover ? (float)span - travel : travel) + 0.5f);
	WeUi_Gfx_PtPos_t dx, dy;
	WeUi_Gfx_Effect_CoverShift(pcParams->Slide, offset, &dx, &dy);
	if (pcParams->Cover) {
		dx = -dx;
		dy = -dy;
	}

	WeUi_Gfx_PtPos_t sy = y - dy;
	WeUi_Gfx_PtPos_t x0 = GET_MAX(dx, 0), x1 = GET_MIN(width + dx, width);
	if (sy < 0 || sy >= height || x1 <= x0) {
		if (pcParams->Cover)
			WeUi_Gfx_Effect_RowOld(0, 0, y, width);
		else
			WeUi_Gfx_Effect_RowNew(0, 0, y, width);
		return;
	}
	if (pcParams->Cover) {
		WeUi_Gfx_Effect_RowOld(0, 0, y, x0);
		WeUi_Gfx_Effect_RowNew(x0, x0 - dx, sy, x1 - x0);
		WeUi_Gfx_Effect_RowOld(x1, x1, y, width - x1);
		return;
	}
	WeUi_Gfx_Effect_RowNew(0, 0, y, x0);
	WeUi_Gfx_Effect_RowOld(x0, x0 - dx, sy, x1 - x0);
	WeUi_Gfx_Effect_RowNew(x1, x1, y, width - x1);
}

RET_STATE_t WeUi_Gfx_Effect_Cover(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
								  WEUI_GFX_EFFECT_SLIDE_t slide, WEUI_GFX_EFFECT_COVER_t mode) {
	WeUi_Gfx_Effect_CoverParams_t params = {.Slide = slide,
											.Cover = mode == WEUI_GFX_EFFECT_COVER_IN};

	return WeUi_Gfx_Effect_RunRows(WeUi_Gfx_Effect_Row_Cover, WeUi_Gfx_Effect_Compose_Cover, frames,
								   &params, renderIncoming);
}
