#ifndef __WEUI_GFX_BASE_H
#define __WEUI_GFX_BASE_H

#include "weui_gfx_types.h"

/** @brief Zero-initialize a WeUi_Gfx_Point_t structure */
void WeUi_Gfx_Point_InitStruct(WeUi_Gfx_Point_t* pPt);

/** @brief Zero-initialize a WeUi_Gfx_ColorPoint_t structure */
void WeUi_Gfx_ColorPoint_InitStruct(WeUi_Gfx_ColorPoint_t* pCpt);

/** @brief Zero-initialize a WeUi_Gfx_Base_t structure */
void WeUi_Gfx_Base_InitStruct(WeUi_Gfx_Base_t* pBase);

/** @brief Initialize WeUi_Gfx_Line_t with defaults: zero coords, Hatching=1.0 (solid) */
void WeUi_Gfx_Line_InitStruct(WeUi_Gfx_Line_t* pLIne);

/** @brief Zero-initialize a WeUi_Gfx_Circle_t structure */
void WeUi_Gfx_Circle_InitStruct(WeUi_Gfx_Circle_t* pCircle);

#define WeUi_Gfx_Rect_InitStruct(a) WeUi_Gfx_Base_InitStruct(a)

/**
 * @brief Read a pixel of the drawing frame into pCpt->Color. No-op if out of bounds.
 *        The value is as stored, after the driver's brightness scaling: drawn back
 *        with Point_Draw it is scaled a second time.
 */
void WeUi_Gfx_Point_Fetch(WeUi_Gfx_ColorPoint_t* pCpt);

/** @brief Write a pixel to the drawing frame. No-op if out of bounds. */
void WeUi_Gfx_Point_Draw(const WeUi_Gfx_ColorPoint_t* pcCpt);

/**
 * @brief Draw a horizontal or vertical line. No Bresenham overhead.
 * @param pcCpt Start point and color
 * @param len   Length in pixels
 * @param dir   WEUI_LINE_DIR_HORIZONTAL or WEUI_LINE_DIR_VERTICAL
 */
void WeUi_Gfx_LineHV_Draw(const WeUi_Gfx_ColorPoint_t* pcCpt, u16 len, WEUI_LINE_DIR_t dir);

/**
 * @brief Draw an arbitrary line using Bresenham's algorithm.
 *        pLine->Hatching: 1.0=solid, 0.5=50% pixel density, 0.0=invisible.
 */
void WeUi_Gfx_Line_Draw(const WeUi_Gfx_Line_t* pcLine);

/**
 * @brief Draw a line from PtOne at given angle (degrees, 0=right, 90=down) and length.
 */
void WeUi_Gfx_LineAngle_Draw(WeUi_Gfx_Line_t* pLine, u32 angle, u32 len);

/** @brief Draw rectangle outline. PtOne=top-left corner, PtTwo=bottom-right corner */
void WeUi_Gfx_RectVertices_Draw(const WeUi_Gfx_Rect_t* pcRect);

/** @brief Draw rectangle outline. PtOne=top-left corner, Width/Height=dimensions */
void WeUi_Gfx_RectEdges_Draw(const WeUi_Gfx_Rect_t* pcRect);

/** @brief Draw filled rectangle. PtOne=top-left corner, PtTwo=bottom-right corner */
void WeUi_Gfx_RectVerticesFill_Draw(const WeUi_Gfx_Rect_t* pcRect);

/** @brief Draw filled rectangle. PtOne=top-left corner, Width/Height=dimensions */
void WeUi_Gfx_RectEdgesFill_Draw(const WeUi_Gfx_Rect_t* pcRect);

/**
 * @brief Draw a circle using the midpoint algorithm. Center=PtOne, radius=pcCircle->Radius.
 * @param isFilled  true=filled with horizontal scanlines, false=outline only
 */
void WeUi_Gfx_Circle_Draw(const WeUi_Gfx_Circle_t* pcCircle, bool isFilled);

/**
 * @brief Scale every pixel of the drawing frame by `keep` ∈ [0.0, 1.0] - one
 *        step of a trail. Works on raw pixels, so driver brightness is not
 *        applied a second time.
 */
void WeUi_Gfx_Frame_Fade(float keep);

#endif /* __WEUI_GFX_BASE_H */
