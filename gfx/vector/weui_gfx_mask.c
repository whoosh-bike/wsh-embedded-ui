#include "weui_gfx_mask.h"

#define WEUI_GFX_MASK_SHIFT		 8
/* 0..255 onto 0..256, so a full mask or opacity multiplies by exactly 1 */
#define WEUI_GFX_MASK_FULL_ROUND 7

typedef struct {
	WeUi_Gfx_Mask_t* pMask;
	u32 Opacity; /**< 0..256 */
	bool Cut;
} WeUi_Gfx_MaskCtx_t;

static u32 WeUi_Gfx_Mask_ToMul(u32 v) {
	return v + (v >> WEUI_GFX_MASK_FULL_ROUND);
}

static void WeUi_Gfx_Mask_Span(void* pCtx, WeUi_Gfx_PtPos_t y, WeUi_Gfx_PtPos_t x, u16 len,
							   const u8* pcCov, u8 cov) {
	const WeUi_Gfx_MaskCtx_t* pcCtx = pCtx;
	WeUi_Gfx_Mask_t* pMask			= pcCtx->pMask;
	s32 w							= pMask->Rect.X1 - pMask->Rect.X0;
	u8* pRow						= &pMask->pBuf[(y - pMask->Rect.Y0) * w + (x - pMask->Rect.X0)];

	/* Covered whole by an opaque shape: shown, or cut away, whatever was there */
	if (pcCov == NULL && cov == WEUI_GFX_RASTER_COVER_FULL &&
		pcCtx->Opacity == WeUi_Gfx_Mask_ToMul(WEUI_GFX_RASTER_COVER_FULL)) {
		memset(pRow, pcCtx->Cut ? 0 : WEUI_GFX_RASTER_COVER_FULL, len);
		return;
	}

	for (u16 i = 0; i < len; i++) {
		u32 a	= ((u32)(pcCov ? pcCov[i] : cov) * pcCtx->Opacity) >> WEUI_GFX_MASK_SHIFT;
		u32 m	= pRow[i];
		/* Over: the rest of what does not show yet, by a; cut: what does, by a */
		pRow[i] = pcCtx->Cut
					  ? (u8)(m - ((m * WeUi_Gfx_Mask_ToMul(a)) >> WEUI_GFX_MASK_SHIFT))
					  : (u8)(m + (((WEUI_GFX_RASTER_COVER_FULL - m) * WeUi_Gfx_Mask_ToMul(a)) >>
								  WEUI_GFX_MASK_SHIFT));
	}
}

RET_STATE_t WeUi_Gfx_Mask_Init(WeUi_Gfx_Mask_t* pMask, u8* pBuf, u32 size,
							   const WeUi_Gfx_ClipRect_t* pcRect) {
	s32 w = pcRect->X1 - pcRect->X0;
	s32 h = pcRect->Y1 - pcRect->Y0;
	if (pBuf == NULL || w <= 0 || h <= 0)
		return RET_STATE_ERR_PARAM;
	if ((u32)(w * h) > size)
		return RET_STATE_ERR_MEMORY;

	*pMask = (WeUi_Gfx_Mask_t){.pBuf = pBuf, .Size = size, .Rect = *pcRect};
	memset(pBuf, 0, (size_t)(w * h));
	return RET_STATE_SUCCESS;
}

static RET_STATE_t WeUi_Gfx_Mask_Paint(WeUi_Gfx_Mask_t* pMask, const WeUi_Gfx_Shape_t* pcShape,
									   u8 opacity, bool cut) {
	WeUi_Gfx_MaskCtx_t ctx = {.pMask = pMask, .Opacity = WeUi_Gfx_Mask_ToMul(opacity), .Cut = cut};
	return WeUi_Gfx_Raster_FillShape(pcShape, &pMask->Rect, WeUi_Gfx_Mask_Span, &ctx);
}

RET_STATE_t WeUi_Gfx_Mask_AddPath(WeUi_Gfx_Mask_t* pMask, const WeUi_Gfx_Path_t* pcPath,
								  u8 opacity) {
	if (pcPath->Broken)
		return RET_STATE_ERR_PARAM;
	WeUi_Gfx_Shape_t shape = WeUi_Gfx_Path_Shape(pcPath);
	return WeUi_Gfx_Mask_Paint(pMask, &shape, opacity, false);
}

RET_STATE_t WeUi_Gfx_Mask_CutPath(WeUi_Gfx_Mask_t* pMask, const WeUi_Gfx_Path_t* pcPath,
								  u8 opacity) {
	if (pcPath->Broken)
		return RET_STATE_ERR_PARAM;
	WeUi_Gfx_Shape_t shape = WeUi_Gfx_Path_Shape(pcPath);
	return WeUi_Gfx_Mask_Paint(pMask, &shape, opacity, true);
}

RET_STATE_t WeUi_Gfx_Mask_AddShape(WeUi_Gfx_Mask_t* pMask, const WeUi_Gfx_Shape_t* pcShape,
								   u8 opacity) {
	return WeUi_Gfx_Mask_Paint(pMask, pcShape, opacity, false);
}

u8 WeUi_Gfx_Mask_At(const WeUi_Gfx_Mask_t* pcMask, WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y) {
	const WeUi_Gfx_ClipRect_t* pcR = &pcMask->Rect;
	if (x < pcR->X0 || x >= pcR->X1 || y < pcR->Y0 || y >= pcR->Y1)
		return 0;

	return pcMask->pBuf[(y - pcR->Y0) * (pcR->X1 - pcR->X0) + (x - pcR->X0)];
}
