#ifndef __WEUI_GFX_IMAGE_H
#define __WEUI_GFX_IMAGE_H

#include "weui_gfx_types.h"

/**
 * @brief Set WeUi_Gfx_ImageTransform_t to defaults: corner pos, normal color, no flip/dither,
 *        neutral color adjust (Exposure=1, Brightness=0, Contrast=1), opacity disabled.
 */
void WeUi_Gfx_Image_InitTransformStruct(WeUi_Gfx_ImageTransform_t* pTransform);

/** @brief Zero-initialize WeUi_Gfx_Image_t and set default transforms */
void WeUi_Gfx_Image_InitStruct(WeUi_Gfx_Image_t* pImgObj);

/**
 * @brief Render an image at the reference point.
 *        Pipeline: load from storage -> [decompress LZ4] -> [Floyd-Steinberg dither]
 *                  -> apply color adjust, flip, invert, intersect blend -> draw pixels.
 *        An image too big for the work buffers is cropped to its top-left
 *        WEUI_DISPLAY_WIDTH x WEUI_DISPLAY_HEIGHT; a compressed one is not drawn then.
 */
void WeUi_Gfx_Image_Draw(WeUi_Gfx_Point_t* pRefPt, WeUi_Gfx_Image_t* pImgObj);

#endif /* __WEUI_GFX_IMAGE_H */
