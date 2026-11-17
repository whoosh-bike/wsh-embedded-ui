#ifndef __WEUI_GFX_COLOR_H
#define __WEUI_GFX_COLOR_H

#include "weui_colors.h"
#include "weui_gfx_types.h"

/** @brief Convert RGB float [0.0, 1.0] to WeUi_Gfx_Color_t pixel color [0, 255] */
WeUi_Gfx_Color_t WeUi_Gfx_Color_FromRgb(WeUi_ColorSchemeRgb_t* pRgb);

/** @brief Convert HSB directly to WeUi_Gfx_Color_t pixel color */
WeUi_Gfx_Color_t WeUi_Gfx_Color_FromHsb(WeUi_ColorSchemeHsb_t* pHsb);

#endif /* __WEUI_GFX_COLOR_H */
