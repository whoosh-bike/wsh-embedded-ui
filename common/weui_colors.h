#ifndef __WEUI_COLORS_H
#define __WEUI_COLORS_H

#include "weui_common_types.h"

/** @brief Return black HSB (Hue=0, Sat=0, Bri=0) */
WeUi_ColorSchemeHsb_t WeUi_Color_GetEmptyHsb(void);

/**
 * @brief Return fully saturated, full-brightness HSB at the given hue.
 * @param hue  Hue angle in degrees [0, 360)
 */
WeUi_ColorSchemeHsb_t WeUi_Color_GetFullHsb(float hue);

/** @brief Convert HSB to RGB float [0.0, 1.0] */
void WeUi_Color_Hsb2Rgb(WeUi_ColorSchemeHsb_t* pHsb, WeUi_ColorSchemeRgb_t* pRgb);

/** @brief Convert RGB float [0.0, 1.0] to HSB */
void WeUi_Color_Rgb2Hsb(WeUi_ColorSchemeRgb_t* pRgb, WeUi_ColorSchemeHsb_t* pHsb);

/**
 * @brief Rotate hue by delta degrees, wrapping within [0, 360).
 * @param delta  Positive or negative degrees
 */
float WeUi_Color_HueIncDec(float hue, float delta);

/**
 * @brief Interpolate between two HSB colors via RGB space.
 * @param ratio  Blend factor ∈ [0.0, 1.0] — 0 = color1, 1 = color2
 */
WeUi_ColorSchemeHsb_t WeUi_Color_GenerateGradient(WeUi_ColorSchemeHsb_t color1,
												  WeUi_ColorSchemeHsb_t color2, float ratio);

#endif /* __WEUI_COLORS_H */
