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
