#include "weui_gfx_transform.h"

WeUi_Gfx_ThreeCoord_t WeUi_Gfx_Transform_Rotate(WeUi_Gfx_ThreeCoord_t* pOldPt, s16 angle,
												WEUI_THREE_AXES_t axe) {
	float rad  = D2R((float)angle);
	float cosa = cosf(rad);
	float sina = sinf(rad);

	WeUi_Gfx_ThreeCoord_t newPt = *pOldPt;
	switch (axe) {
		case AXE_X:
			newPt.X = pOldPt->X;
			newPt.Y = pOldPt->Y * cosa - pOldPt->Z * sina;
			newPt.Z = pOldPt->Z * cosa + pOldPt->Y * sina;
			break;
		case AXE_Y:
			newPt.X = pOldPt->X * cosa - pOldPt->Z * sina;
			newPt.Y = pOldPt->Y;
			newPt.Z = pOldPt->Z * cosa + pOldPt->X * sina;
			break;
		case AXE_Z:
			newPt.X = pOldPt->X * cosa - pOldPt->Y * sina;
			newPt.Y = pOldPt->Y * cosa + pOldPt->X * sina;
			newPt.Z = pOldPt->Z;
			break;
		default:
			break;
	}

	return newPt;
}

WeUi_Gfx_ThreeCoord_t WeUi_Gfx_Transform_Offset(WeUi_Gfx_ThreeCoord_t* pOldPt,
												WeUi_Gfx_ThreeCoord_t* pOffset) {
	WeUi_Gfx_ThreeCoord_t newPt;

	newPt.X = pOldPt->X + pOffset->X;
	newPt.Y = pOldPt->Y + pOffset->Y;
	newPt.Z = pOldPt->Z + pOffset->Z;

	return newPt;
}

WeUi_Gfx_ThreeCoord_t WeUi_Gfx_Transform_Scale(WeUi_Gfx_ThreeCoord_t* pOldPt,
											   WeUi_Gfx_ThreeCoord_t* pScale) {
	WeUi_Gfx_ThreeCoord_t newPt;

	newPt.X = pOldPt->X * pScale->X;
	newPt.Y = pOldPt->Y * pScale->Y;
	newPt.Z = pOldPt->Z * pScale->Z;

	return newPt;
}

void WeUi_Gfx_Transform_InitImageColorAdjustStruct(WeUi_Gfx_ImageTransform_t* pTransform) {
	if (!pTransform)
		return;

	pTransform->Exposure   = WEUI_TRANSFORM_EXPOSURE_DEF;
	pTransform->Brightness = WEUI_TRANSFORM_BRIGHTNESS_DEF;
	pTransform->Contrast   = WEUI_TRANSFORM_CONTRAST_DEF;
}

float WeUi_Gfx_Transform_ColorAdjust(float color, WeUi_Gfx_ImageTransform_t* pTransform) {
	color *= pTransform->Exposure;
	color += pTransform->Brightness;

	float mid = (float)(WEUI_COLOR_CHANNEL_MAX) / 2.0f;
	color	  = ((color - mid) * pTransform->Contrast) + mid;
	color	  = fmaxf(color, (float)(WEUI_COLOR_CHANNEL_MIN));
	color	  = fminf(color, (float)(WEUI_COLOR_CHANNEL_MAX));

	return color;
}

float WeUi_Gfx_Transform_GammaDecode(float color) {
	color /= (float)(WEUI_COLOR_CHANNEL_MAX);
	/* converts a sRGB input (in the range 0.0-1.0) to linear color space */
	if (color <= 0.04045f)
		color /= 12.92f;
	else
		color = powf(((color + 0.055f) / 1.055f), 2.4f);

	return color * (float)(WEUI_COLOR_CHANNEL_MAX);
}

float WeUi_Gfx_Transform_GammaEncode(float color) {
	color /= (float)(WEUI_COLOR_CHANNEL_MAX);
	/* converts a linear color input (in the range 0.0-1.0) to sRGB color space */
	if (color <= 0.0031308f)
		color *= 12.92f;
	else
		color = (1.055f * powf(color, (1.0f / 2.4f))) - 0.055f;

	return color * (float)(WEUI_COLOR_CHANNEL_MAX);
}

u32 WeUi_Gfx_Transform_Color2Bytes(WeUi_Gfx_Color_t color) {
	u32 bytesColor = (u32)color.R;

#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
	bytesColor |= ((u32)color.G << 8) | ((u32)color.B << 16);
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */

#if WEUI_LIB_COLOR_ALPHA_CHANNEL
	bytesColor |= ((u32)color.A << 24);
#endif /* WEUI_LIB_COLOR_ALPHA_CHANNEL */

	return bytesColor;
}

WeUi_Gfx_Color_t WeUi_Gfx_Transform_Bytes2Color(u32 value) {
	WeUi_Gfx_Color_t color = {0};
	color.R				   = value & WEUI_COLOR_CHANNEL_MAX;

#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
	color.G = (value >> 8) & WEUI_COLOR_CHANNEL_MAX;
	color.B = (value >> 16) & WEUI_COLOR_CHANNEL_MAX;
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */

#if WEUI_LIB_COLOR_ALPHA_CHANNEL
	color.A = (value >> 24) & WEUI_COLOR_CHANNEL_MAX;
#endif /* WEUI_LIB_COLOR_ALPHA_CHANNEL */

	return color;
}

WeUi_Gfx_Color_t WeUi_Gfx_Transform_Rgba2Color(WeUi_Gfx_ColorChannel_t r, WeUi_Gfx_ColorChannel_t g,
											   WeUi_Gfx_ColorChannel_t b,
											   WeUi_Gfx_ColorChannel_t a) {
	WeUi_Gfx_Color_t color = {0};
	color.R				   = r;

#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
	color.G = g;
	color.B = b;
#else /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */
#if WEUI_LIB_RGB_BLENDING
	color.R |= g | b;
#endif /* WEUI_LIB_RGB_BLENDING */
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */

#if WEUI_LIB_COLOR_ALPHA_CHANNEL
	color.A = a;
#endif /* WEUI_LIB_COLOR_ALPHA_CHANNEL */

	return color;
}

WeUi_Gfx_Color_t WeUi_Gfx_Transform_Rgb2Color(WeUi_Gfx_ColorChannel_t r, WeUi_Gfx_ColorChannel_t g,
											  WeUi_Gfx_ColorChannel_t b) {
	WeUi_Gfx_ColorChannel_t alphaColor = WEUI_COLOR_CHANNEL_MAX;
	return WeUi_Gfx_Transform_Rgba2Color(r, g, b, alphaColor);
}

u32 WeUi_Gfx_Transform_FillBytesFromChannel(WeUi_Gfx_ColorChannel_t value) {
	return WeUi_Gfx_Transform_Color2Bytes(WeUi_Gfx_Transform_Rgb2Color(value, value, value));
}

WeUi_Gfx_Color_t WeUi_Gfx_Transform_FillColorFromChannel(WeUi_Gfx_ColorChannel_t value) {
	return WeUi_Gfx_Transform_Rgb2Color(value, value, value);
}

WeUi_Gfx_Color_t WeUi_Gfx_Transform_InvertColor(WeUi_Gfx_Color_t color) {
	color.R = ~color.R;

#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
	color.G = ~color.G;
	color.B = ~color.B;
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */

#if WEUI_LIB_COLOR_ALPHA_CHANNEL
	// color.A = ~color.A; //Do not invert alpha channel
#endif /* WEUI_LIB_COLOR_ALPHA_CHANNEL */

	return color;
}

WeUi_Gfx_ColorChannel_t WeUi_Gfx_Transform_Color2Byte_Luma(WeUi_Gfx_Color_t color) {
	WeUi_Gfx_ColorChannel_t gray = color.R;

#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
	gray = (WeUi_Gfx_ColorChannel_t)(0.2126f * color.R + 0.7152f * color.G + 0.0722f * color.B);
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */

	return gray;
}
