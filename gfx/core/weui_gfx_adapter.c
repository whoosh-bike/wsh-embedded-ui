#include "weui_gfx_adapter.h"
#include "display_m.h"
#include "weui_gfx_transform.h"

/* Mirroring of a panel mounted the other way round. Done here, where every pixel access
 * crosses into the driver, so scenes, effects and the frame buffers all agree on one picture.
 * A panel that can mirror in hardware should do it in its driver instead. */
#ifndef WEUI_DISPLAY_FLIP_X
#define WEUI_DISPLAY_FLIP_X 0
#endif /* WEUI_DISPLAY_FLIP_X */

#ifndef WEUI_DISPLAY_FLIP_Y
#define WEUI_DISPLAY_FLIP_Y 0
#endif /* WEUI_DISPLAY_FLIP_Y */

static DisplayM_Params_t CachedParams;

static u16 WeUi_Gfx_Adapter_MapX(WeUi_Gfx_PtPos_t x) {
	return WEUI_DISPLAY_FLIP_X ? CachedParams.X_Max - (x - CachedParams.X_Min) : x;
}

static u16 WeUi_Gfx_Adapter_MapY(WeUi_Gfx_PtPos_t y) {
	return WEUI_DISPLAY_FLIP_Y ? CachedParams.Y_Max - (y - CachedParams.Y_Min) : y;
}

RET_STATE_t WeUi_Gfx_Adapter_Init(void) {
	CachedParams = DisplayM_GetParams();

	/* A smaller panel is fine - a config may be sized for the largest one a
	 * project supports - but a larger one overruns every buffer sized from it */
	if (CachedParams.Width > WEUI_DISPLAY_WIDTH || CachedParams.Height > WEUI_DISPLAY_HEIGHT)
		return RET_STATE_ERR_PARAM;

	return RET_STATE_SUCCESS;
}

WeUi_Gfx_ObjSize_t WeUi_Gfx_Adapter_GetWidth(void) {
	return CachedParams.Width;
}

WeUi_Gfx_ObjSize_t WeUi_Gfx_Adapter_GetHeight(void) {
	return CachedParams.Height;
}

WeUi_Gfx_PtPos_t WeUi_Gfx_Adapter_GetX_Min(void) {
	return CachedParams.X_Min;
}

WeUi_Gfx_PtPos_t WeUi_Gfx_Adapter_GetX_Mid(void) {
	return CachedParams.MidX;
}

WeUi_Gfx_PtPos_t WeUi_Gfx_Adapter_GetX_Max(void) {
	return CachedParams.X_Max;
}

WeUi_Gfx_PtPos_t WeUi_Gfx_Adapter_GetY_Min(void) {
	return CachedParams.Y_Min;
}

WeUi_Gfx_PtPos_t WeUi_Gfx_Adapter_GetY_Mid(void) {
	return CachedParams.Y_Mid;
}

WeUi_Gfx_PtPos_t WeUi_Gfx_Adapter_GetY_Max(void) {
	return CachedParams.Y_Max;
}

WeUi_Gfx_ColorChannel_t WeUi_Gfx_Adapter_GetColor_Min(void) {
	return CachedParams.ColorMin;
}

WeUi_Gfx_ColorChannel_t WeUi_Gfx_Adapter_GetColor_Mid(void) {
	return CachedParams.ColorMid;
}

WeUi_Gfx_ColorChannel_t WeUi_Gfx_Adapter_GetColor_Max(void) {
	return CachedParams.ColorMax;
}

WeUi_Gfx_Color_t WeUi_Gfx_Adapter_GetPixel(const WeUi_Gfx_Point_t* pcPt) {
	u32 colorBytes =
		DisplayM_GetPixel(WeUi_Gfx_Adapter_MapX(pcPt->X), WeUi_Gfx_Adapter_MapY(pcPt->Y));

#ifdef WEUI_DISPLAY_GRAY
	/**
	 * Create RGB channels only from R channel
	 */
	WeUi_Gfx_Color_t colorRgb = WeUi_Gfx_Transform_Bytes2Color(colorBytes);
	return WeUi_Gfx_Transform_Rgb2Color(colorRgb.R, colorRgb.R, colorRgb.R);
#endif /* WEUI_DISPLAY_GRAY */

	return WeUi_Gfx_Transform_Bytes2Color(colorBytes);
}

/* A gray panel stores luma, so every way a colour reaches the driver converts alike */
static u32 WeUi_Gfx_Adapter_ToBytes(WeUi_Gfx_Color_t color) {
#ifdef WEUI_DISPLAY_GRAY
	WeUi_Gfx_ColorChannel_t gray = WeUi_Gfx_Transform_Color2Byte_Luma(color);
	color						 = WeUi_Gfx_Transform_Rgb2Color(gray, gray, gray);
#endif /* WEUI_DISPLAY_GRAY */

	return WeUi_Gfx_Transform_Color2Bytes(color);
}

void WeUi_Gfx_Adapter_SetPixel(const WeUi_Gfx_ColorPoint_t* pcCpt) {
	DisplayM_SetPixel(WeUi_Gfx_Adapter_MapX(pcCpt->Pt.X), WeUi_Gfx_Adapter_MapY(pcCpt->Pt.Y),
					  WeUi_Gfx_Adapter_ToBytes(pcCpt->Color));
}

WeUi_Gfx_ColorRaw_t WeUi_Gfx_Adapter_GetPixelRaw(const WeUi_Gfx_Point_t* pcPt) {
	return DisplayM_GetPixel(WeUi_Gfx_Adapter_MapX(pcPt->X), WeUi_Gfx_Adapter_MapY(pcPt->Y));
}

void WeUi_Gfx_Adapter_SetPixelRaw(const WeUi_Gfx_Point_t* pcPt, WeUi_Gfx_ColorRaw_t raw) {
	DisplayM_SetPixelRaw(WeUi_Gfx_Adapter_MapX(pcPt->X), WeUi_Gfx_Adapter_MapY(pcPt->Y), raw);
}

void WeUi_Gfx_Adapter_SaveFrame_DisplayToAux(void) {
	DisplayM_SaveFrame_DisplayToAux();
}

WeUi_Gfx_ColorRaw_t WeUi_Gfx_Adapter_GetPixelRaw_Aux(const WeUi_Gfx_Point_t* pcPt) {
	return DisplayM_GetPixelAux(WeUi_Gfx_Adapter_MapX(pcPt->X), WeUi_Gfx_Adapter_MapY(pcPt->Y));
}

bool WeUi_Gfx_Adapter_FrameMem(WEUI_GFX_FRAME_t frame, WeUi_Gfx_FrameMem_t* pMem) {
#if WEUI_DISPLAY_FRAME_MEM && !WEUI_DISPLAY_FLIP_X && !WEUI_DISPLAY_FLIP_Y
	u8 px	   = 0;
	pMem->pBuf = DisplayM_GetFrameMem((u8)frame, &pMem->Stride, &px);
	pMem->Px   = (WEUI_GFX_FRAME_PX_t)px;
	return pMem->pBuf != NULL && px <= WEUI_GFX_FRAME_PX_RGB565_BE;
#else
	(void)frame;
	(void)pMem;
	return false;
#endif
}

void WeUi_Gfx_Adapter_ClearDrawingFrame(void) {
	DisplayM_FillDrawingFrame(WeUi_Gfx_Transform_FillBytesFromChannel(CachedParams.ColorMin));
}

void WeUi_Gfx_Adapter_FillDrawingFrame(WeUi_Gfx_Color_t color) {
	DisplayM_FillDrawingFrame(WeUi_Gfx_Adapter_ToBytes(color));
}

void WeUi_Gfx_Adapter_ClearAllFrames(void) {
	DisplayM_FillDisplayAndDrawingFrames(
		WeUi_Gfx_Transform_FillBytesFromChannel(CachedParams.ColorMin));
}

void WeUi_Gfx_Adapter_DrawFrame(void) {
	DisplayM_DrawFrame();
}

void WeUi_Gfx_Adapter_CopyFrame_DisplayToDrawing(void) {
	DisplayM_CopyFrame_DisplayToDrawing();
}

#if WEUI_DISPLAY_BANDS
u16* WeUi_Gfx_Adapter_BandGet(u32* pPixels) {
	return DisplayM_BandGet(pPixels);
}

void WeUi_Gfx_Adapter_BandSend(u16 x0, u16 y0, u16 x1, u16 y1, u16* pBuf) {
	DisplayM_BandSend(x0, y0, x1, y1, pBuf);
}
#endif /* WEUI_DISPLAY_BANDS */
