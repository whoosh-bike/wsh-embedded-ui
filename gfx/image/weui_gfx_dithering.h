#ifndef __WEUI_GFX_DITHERING_H
#define __WEUI_GFX_DITHERING_H

#include "weui_gfx_types.h"

/**
 * @brief Apply Floyd-Steinberg error-diffusion dithering to pImgObj->ImageOrig.
 *        Result is written into pImgObj->ImageAux. Includes sRGB gamma decode/encode
 *        for perceptually correct error diffusion.
 *        Requires WEUI_USE_COMPONENT__IMAGE_DITHER=1 and WEUI_LIB_COLOR_SCHEME__GRAY.
 * @return true if dithering was applied (caller should swap ImageOrig/ImageAux), false otherwise
 */
bool WeUi_Gfx_Dither_FloydSteinberg(WeUi_Gfx_Image_t* pImgObj);

#endif /* __WEUI_GFX_DITHERING_H */
