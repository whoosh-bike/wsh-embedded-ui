#ifndef __WEUI_GFX_MASK_H
#define __WEUI_GFX_MASK_H

#include "weui_gfx_raster.h"

/**
 * @file weui_gfx_mask.h
 * @brief Where other shapes may show: a coverage of 0..255 per pixel over a rectangle, built
 *        from shapes - added, each at an opacity, and cut away - and used by a paint to keep
 *        inside it or out of it (weui_gfx_fill.h). An iris inside the eye's opening, a moon
 *        cut from a disc; several shapes at their opacities make a layer's alpha, as Lottie's
 *        alpha matte.
 *
 * The caller owns the storage: a mask holds Rect's pixels, row by row, and reads 0 outside.
 */

typedef enum {
	WEUI_GFX_MASK_IN = 0, /**< show only inside the mask: the intersection */
	WEUI_GFX_MASK_OUT,	  /**< show only outside it: the mask subtracted */
} WEUI_GFX_MASK_MODE_t;

typedef struct {
	u8* pBuf;
	u32 Size;
	WeUi_Gfx_ClipRect_t Rect; /**< the pixels it holds */
} WeUi_Gfx_Mask_t;

/** @brief A mask as a paint uses it */
typedef struct {
	const WeUi_Gfx_Mask_t* pcMask;
	WEUI_GFX_MASK_MODE_t Mode;
} WeUi_Gfx_MaskUse_t;

/**
 * @brief An empty mask over rect, in the caller's buffer.
 * @return RET_STATE_ERR_PARAM for no buffer or an empty rect, RET_STATE_ERR_MEMORY for a rect
 *         of more pixels than the buffer holds.
 */
RET_STATE_t WeUi_Gfx_Mask_Init(WeUi_Gfx_Mask_t* pMask, u8* pBuf, u32 size,
							   const WeUi_Gfx_ClipRect_t* pcRect);

/**
 * @brief Add the shape at an opacity: what it covers shows from then on, over what showed
 *        already as one layer does over another.
 * @return what WeUi_Gfx_Raster_Fill() returns
 */
RET_STATE_t WeUi_Gfx_Mask_AddPath(WeUi_Gfx_Mask_t* pMask, const WeUi_Gfx_Path_t* pcPath,
								  u8 opacity);

/** @brief WeUi_Gfx_Mask_AddPath() of a shape, which cannot be Broken */
RET_STATE_t WeUi_Gfx_Mask_AddShape(WeUi_Gfx_Mask_t* pMask, const WeUi_Gfx_Shape_t* pcShape,
								   u8 opacity);

/** @brief Cut the shape away, at an opacity. @return as WeUi_Gfx_Mask_AddPath() */
RET_STATE_t WeUi_Gfx_Mask_CutPath(WeUi_Gfx_Mask_t* pMask, const WeUi_Gfx_Path_t* pcPath,
								  u8 opacity);

/** @brief The mask at a pixel, 0 outside its Rect */
u8 WeUi_Gfx_Mask_At(const WeUi_Gfx_Mask_t* pcMask, WeUi_Gfx_PtPos_t x, WeUi_Gfx_PtPos_t y);

#endif /* __WEUI_GFX_MASK_H */
