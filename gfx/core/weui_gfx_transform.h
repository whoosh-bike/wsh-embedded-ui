#ifndef __WEUI_GFX_TRANSFORM_H
#define __WEUI_GFX_TRANSFORM_H

#include "weui_gfx_types.h"

/** @brief Neutral image color adjust defaults (no change) */
#define WEUI_TRANSFORM_EXPOSURE_DEF	  1.0f
#define WEUI_TRANSFORM_BRIGHTNESS_DEF 0.0f
#define WEUI_TRANSFORM_CONTRAST_DEF	  1.0f

/** @brief Rotate 3D point around axis. Returns new point; does not modify original. */
WeUi_Gfx_ThreeCoord_t WeUi_Gfx_Transform_Rotate(WeUi_Gfx_ThreeCoord_t* pOldPt, s16 angle,
												WEUI_THREE_AXES_t axe);

/** @brief Translate 3D point by offset. Returns new point. */
WeUi_Gfx_ThreeCoord_t WeUi_Gfx_Transform_Offset(WeUi_Gfx_ThreeCoord_t* pOldPt,
												WeUi_Gfx_ThreeCoord_t* pOffset);

/** @brief Scale 3D point per-axis. Returns new point. */
WeUi_Gfx_ThreeCoord_t WeUi_Gfx_Transform_Scale(WeUi_Gfx_ThreeCoord_t* pOldPt,
											   WeUi_Gfx_ThreeCoord_t* pScale);

/** @brief Set image transform color fields to neutral defaults (Exposure=1, Brightness=0, Contrast=1) */
void WeUi_Gfx_Transform_InitImageColorAdjustStruct(WeUi_Gfx_ImageTransform_t* pTransform);

/**
 * @brief Apply exposure/brightness/contrast to a single channel value [0, 255].
 *        Result is clamped to [WEUI_COLOR_CHANNEL_MIN, WEUI_COLOR_CHANNEL_MAX].
 */
float WeUi_Gfx_Transform_ColorAdjust(float color, WeUi_Gfx_ImageTransform_t* pTransform);

/** @brief sRGB -> linear light (gamma decode). Input/output range [0, 255]. */
float WeUi_Gfx_Transform_GammaDecode(float color);

/** @brief Linear light -> sRGB (gamma encode). Input/output range [0, 255]. */
float WeUi_Gfx_Transform_GammaEncode(float color);

/** @brief Pack WeUi_Gfx_Color_t into u32: R | (G<<8) | (B<<16) | (A<<24) */
u32 WeUi_Gfx_Transform_Color2Bytes(WeUi_Gfx_Color_t color);

/** @brief Unpack u32 to WeUi_Gfx_Color_t */
WeUi_Gfx_Color_t WeUi_Gfx_Transform_Bytes2Color(u32 value);

/** @brief Create WeUi_Gfx_Color_t from individual RGBA byte channels */
WeUi_Gfx_Color_t WeUi_Gfx_Transform_Rgba2Color(WeUi_Gfx_ColorChannel_t r, WeUi_Gfx_ColorChannel_t g,
											   WeUi_Gfx_ColorChannel_t b,
											   WeUi_Gfx_ColorChannel_t a);

/** @brief Create WeUi_Gfx_Color_t from RGB byte channels (alpha = WEUI_COLOR_CHANNEL_MAX) */
WeUi_Gfx_Color_t WeUi_Gfx_Transform_Rgb2Color(WeUi_Gfx_ColorChannel_t r, WeUi_Gfx_ColorChannel_t g,
											  WeUi_Gfx_ColorChannel_t b);

/** @brief Create packed u32 color with R=G=B=value */
u32 WeUi_Gfx_Transform_FillBytesFromChannel(WeUi_Gfx_ColorChannel_t value);

/** @brief Create WeUi_Gfx_Color_t with R=G=B=value */
WeUi_Gfx_Color_t WeUi_Gfx_Transform_FillColorFromChannel(WeUi_Gfx_ColorChannel_t value);

/** @brief Bitwise-invert R, G, B channels. Alpha is NOT inverted. */
WeUi_Gfx_Color_t WeUi_Gfx_Transform_InvertColor(WeUi_Gfx_Color_t color);

/** @brief Convert RGB to luminance using ITU-R BT.709: 0.2126*R + 0.7152*G + 0.0722*B */
WeUi_Gfx_ColorChannel_t WeUi_Gfx_Transform_Color2Byte_Luma(WeUi_Gfx_Color_t color);

#endif /* __WEUI_GFX_TRANSFORM_H */
