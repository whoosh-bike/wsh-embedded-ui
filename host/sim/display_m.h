#ifndef __DISPLAY_M_H
#define __DISPLAY_M_H

#include "mathlib_common.h"

/**
 * The display_m contract weui's adapter calls, over host memory. Pixels are u32
 * 0x00BBGGRR. As in the BSP drivers, SetPixel and the fills pass a colour through a
 * brightness LUT while the raw accessors store it as given, and DrawFrame swaps the
 * drawing and display frames the way a DMA-driven driver does.
 */

typedef struct {
	u16 Width;
	u16 Height;
	u16 X_Min;
	u16 Y_Min;
	u16 MidX;
	u16 Y_Mid;
	u16 X_Max;
	u16 Y_Max;
	u32 PixelNum;
	u8 ColorMin;
	u8 ColorMid;
	u8 ColorMax;
} DisplayM_Params_t;

DisplayM_Params_t DisplayM_GetParams(void);
void DisplayM_DrawFrame(void);
u32 DisplayM_GetPixel(u16 x, u16 y);
u32 DisplayM_GetPixelAux(u16 x, u16 y);
void DisplayM_SetPixel(u16 x, u16 y, u32 color);
void DisplayM_SetPixelRaw(u16 x, u16 y, u32 color);
void DisplayM_CopyFrame_DisplayToDrawing(void);
void DisplayM_SaveFrame_DisplayToAux(void);
void DisplayM_FillDrawingFrame(u32 color);
void DisplayM_FillDisplayAndDrawingFrames(u32 color);

/* A frame as memory, for a build with WEUI_DISPLAY_FRAME_MEM on: frame 0 drawing, 1 display,
 * 2 aux; px 0 for a u32 a pixel as the raw accessors take them, 1 for 565, 2 for 565 high byte
 * first. NULL for a frame not kept */
void* DisplayM_GetFrameMem(u8 frame, u32* pStride, u8* pPx);
/* Bands of 565 pixels, for a build with WEUI_DISPLAY_BANDS on */
u16* DisplayM_BandGet(u32* pPixels);
void DisplayM_BandSend(u16 x0, u16 y0, u16 x1, u16 y1, u16* pBuf);

/** @brief Called with every frame DrawFrame pushes */
typedef void (*DisplaySim_FrameHook_t)(const u32* pcFrame, u32 idx);

/** @brief A panel of w x h, cleared, brightness 1.0, no hook */
void DisplaySim_Init(u16 w, u16 h);
void DisplaySim_Deinit(void);
void DisplaySim_SetBrightScale(float scale);
u32 DisplaySim_ApplyBright(u32 color);
void DisplaySim_SetFrameHook(DisplaySim_FrameHook_t hook);
u32 DisplaySim_GetFramesPushed(void);
const u32* DisplaySim_GetDisplayFrame(void);
u64 DisplaySim_Hash(const u32* pcFrame);

#endif /* __DISPLAY_M_H */
