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

#endif /* __WEUI_GFX_ADAPTER_H */
