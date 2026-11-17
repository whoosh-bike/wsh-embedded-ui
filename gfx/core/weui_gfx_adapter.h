#ifndef __WEUI_GFX_ADAPTER_H
#define __WEUI_GFX_ADAPTER_H

#include "weui_gfx_types.h"

/**
 * @brief Cache display parameters from the driver. Must be called once after
 *        display hardware init, before any drawing operations.
 *
 * @return RET_STATE_ERR_PARAM when the panel is larger than WEUI_DISPLAY_WIDTH x
 *         WEUI_DISPLAY_HEIGHT: the library's per-pixel buffers are sized by those,
 *         so drawing on it would run past them. Nothing must be drawn then.
 */
RET_STATE_t WeUi_Gfx_Adapter_Init(void);

/** @brief Display width in pixels */
WeUi_Gfx_ObjSize_t WeUi_Gfx_Adapter_GetWidth(void);

/** @brief Display height in pixels */
WeUi_Gfx_ObjSize_t WeUi_Gfx_Adapter_GetHeight(void);

WeUi_Gfx_PtPos_t WeUi_Gfx_Adapter_GetX_Min(void);
WeUi_Gfx_PtPos_t WeUi_Gfx_Adapter_GetX_Mid(void);
WeUi_Gfx_PtPos_t WeUi_Gfx_Adapter_GetX_Max(void);

WeUi_Gfx_PtPos_t WeUi_Gfx_Adapter_GetY_Min(void);
WeUi_Gfx_PtPos_t WeUi_Gfx_Adapter_GetY_Mid(void);
WeUi_Gfx_PtPos_t WeUi_Gfx_Adapter_GetY_Max(void);

WeUi_Gfx_ColorChannel_t WeUi_Gfx_Adapter_GetColor_Min(void);
WeUi_Gfx_ColorChannel_t WeUi_Gfx_Adapter_GetColor_Mid(void);
WeUi_Gfx_ColorChannel_t WeUi_Gfx_Adapter_GetColor_Max(void);

/**
 * @brief Read pixel from the drawing frame - the frame being built, which is the
 *        last pushed picture only after CopyFrame_DisplayToDrawing (or in a
 *        KeepFrame scene). In GRAY mode, replicates R channel to G and B.
 */
WeUi_Gfx_Color_t WeUi_Gfx_Adapter_GetPixel(const WeUi_Gfx_Point_t* pcPt);

/** @brief Write pixel to the drawing frame. In GRAY mode, converts RGB to luma first. */
void WeUi_Gfx_Adapter_SetPixel(const WeUi_Gfx_ColorPoint_t* pcCpt);

/**
 * @brief Move a drawing frame pixel without changing its value. The Get/SetPixel
 *        pair would rescale it on every pass; these do not.
 */
WeUi_Gfx_ColorRaw_t WeUi_Gfx_Adapter_GetPixelRaw(const WeUi_Gfx_Point_t* pcPt);
void WeUi_Gfx_Adapter_SetPixelRaw(const WeUi_Gfx_Point_t* pcPt, WeUi_Gfx_ColorRaw_t raw);

/** @brief Park the on-panel picture in the aux frame: it survives drawing and pushes. */
void WeUi_Gfx_Adapter_SaveFrame_DisplayToAux(void);

/** @brief Read a pixel of the aux frame. */
WeUi_Gfx_ColorRaw_t WeUi_Gfx_Adapter_GetPixelRaw_Aux(const WeUi_Gfx_Point_t* pcPt);

typedef enum {
	WEUI_GFX_FRAME_DRAWING = 0,
	WEUI_GFX_FRAME_DISPLAY,
	WEUI_GFX_FRAME_AUX,
} WEUI_GFX_FRAME_t;

/** @brief How a frame keeps its pixels in memory */
typedef enum {
	WEUI_GFX_FRAME_PX_RAW = 0,	 /**< a WeUi_Gfx_ColorRaw_t each, as the raw accessors take them */
	WEUI_GFX_FRAME_PX_RGB565,	 /**< 16 bits, red at the top: the raw colour cut to 5-6-5 */
	WEUI_GFX_FRAME_PX_RGB565_BE, /**< the same, high byte first */
} WEUI_GFX_FRAME_PX_t;

typedef struct {
	void* pBuf;
	u32 Stride; /**< pixels from one row to the next */
	WEUI_GFX_FRAME_PX_t Px;
} WeUi_Gfx_FrameMem_t;

/**
 * @brief A frame as memory, for code that moves many pixels - effects - to work on it without
 *        a call a pixel. Only with WEUI_DISPLAY_FRAME_MEM, and not with the panel mirrored here
 *        (WEUI_DISPLAY_FLIP_X/Y): false then, or for a frame the driver does not keep, and the
 *        pixel calls are the way. Valid until the next DrawFrame(): it swaps frames.
 */
bool WeUi_Gfx_Adapter_FrameMem(WEUI_GFX_FRAME_t frame, WeUi_Gfx_FrameMem_t* pMem);

/* A raw colour (0x00BBGGRR) cut to 5-6-5 and back, the bits repeated down: what a 565 frame
 * keeps, and the colour read back from it */
static inline u16 WeUi_Gfx_Adapter_RawTo565(WeUi_Gfx_ColorRaw_t raw) {
	return (u16)(((raw & 0xF8u) << 8) | ((raw >> 5) & 0x7E0u) | ((raw >> 19) & 0x1Fu));
}

static inline WeUi_Gfx_ColorRaw_t WeUi_Gfx_Adapter_RawFrom565(u16 px) {
	u32 r = (px >> 11) & 0x1Fu, g = (px >> 5) & 0x3Fu, b = px & 0x1Fu;
	return ((r << 3) | (r >> 2)) | (((g << 2) | (g >> 4)) << 8) | (((b << 3) | (b >> 2)) << 16);
}

/** @brief Pixel i - y * Stride + x - of a frame as memory, raw as GetPixelRaw() gives it */
static inline WeUi_Gfx_ColorRaw_t WeUi_Gfx_Adapter_FrameGet(const WeUi_Gfx_FrameMem_t* pcMem,
															u32 i) {
	switch (pcMem->Px) {
		case WEUI_GFX_FRAME_PX_RGB565:
			return WeUi_Gfx_Adapter_RawFrom565(((const u16*)pcMem->pBuf)[i]);
		case WEUI_GFX_FRAME_PX_RGB565_BE:
			return WeUi_Gfx_Adapter_RawFrom565(__builtin_bswap16(((const u16*)pcMem->pBuf)[i]));
		default:
			return ((const WeUi_Gfx_ColorRaw_t*)pcMem->pBuf)[i];
	}
}

/** @brief Pixel i of a frame as memory set raw, as SetPixelRaw() does */
static inline void WeUi_Gfx_Adapter_FrameSet(const WeUi_Gfx_FrameMem_t* pcMem, u32 i,
											 WeUi_Gfx_ColorRaw_t raw) {
	switch (pcMem->Px) {
		case WEUI_GFX_FRAME_PX_RGB565:
			((u16*)pcMem->pBuf)[i] = WeUi_Gfx_Adapter_RawTo565(raw);
			break;
		case WEUI_GFX_FRAME_PX_RGB565_BE:
			((u16*)pcMem->pBuf)[i] = __builtin_bswap16(WeUi_Gfx_Adapter_RawTo565(raw));
			break;
		default:
			((WeUi_Gfx_ColorRaw_t*)pcMem->pBuf)[i] = raw;
			break;
	}
}

/** @brief Bytes a pixel of a frame as memory */
static inline u32 WeUi_Gfx_Adapter_FramePxBytes(const WeUi_Gfx_FrameMem_t* pcMem) {
	return pcMem->Px == WEUI_GFX_FRAME_PX_RAW ? sizeof(WeUi_Gfx_ColorRaw_t) : sizeof(u16);
}

/** @brief Fill drawing frame with minimum color (black) */
void WeUi_Gfx_Adapter_ClearDrawingFrame(void);

/** @brief Fill drawing frame with the given color */
void WeUi_Gfx_Adapter_FillDrawingFrame(WeUi_Gfx_Color_t color);

/** @brief Clear both display frame and drawing frame */
void WeUi_Gfx_Adapter_ClearAllFrames(void);

/** @brief Push drawing frame to display hardware (triggers DMA transfer) */
void WeUi_Gfx_Adapter_DrawFrame(void);

/** @brief Copy current display frame content into the drawing frame */
void WeUi_Gfx_Adapter_CopyFrame_DisplayToDrawing(void);

#if WEUI_DISPLAY_BANDS
/**
 * @brief A band buffer of the driver's to draw into, and how many pixels it holds. Waits for
 *        one while every buffer is still going out. Every buffer taken goes back by BandSend().
 */
u16* WeUi_Gfx_Adapter_BandGet(u32* pPixels);

/** @brief Send the band - the panel's pixels x0..x1, y0..y1, x1 and y1 left out - from pBuf */
void WeUi_Gfx_Adapter_BandSend(u16 x0, u16 y0, u16 x1, u16 y1, u16* pBuf);
#endif /* WEUI_DISPLAY_BANDS */

#endif /* __WEUI_GFX_ADAPTER_H */
