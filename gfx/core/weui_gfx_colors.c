#include "weui_gfx_colors.h"
#include "def_macro.h"
#include "weui_gfx_transform.h"

/* Clamped first: an over-range HSB (Bri 255 instead of 100) is a float->u8 conversion
 * out of range, which is undefined rather than saturating */
static WeUi_Gfx_ColorChannel_t WeUi_Gfx_Color_ToChannel(float value) {
	return (WeUi_Gfx_ColorChannel_t)(CUT_NORM_FLOAT(value) * (float)WEUI_COLOR_CHANNEL_MAX);
}

WeUi_Gfx_Color_t WeUi_Gfx_Color_FromRgb(WeUi_ColorSchemeRgb_t* pRgb) {
	return WeUi_Gfx_Transform_Rgb2Color(WeUi_Gfx_Color_ToChannel(pRgb->Red),
										WeUi_Gfx_Color_ToChannel(pRgb->Green),
										WeUi_Gfx_Color_ToChannel(pRgb->Blue));
}

WeUi_Gfx_Color_t WeUi_Gfx_Color_FromHsb(WeUi_ColorSchemeHsb_t* pHsb) {
	WeUi_ColorSchemeRgb_t rgb;
	WeUi_Color_Hsb2Rgb(pHsb, &rgb);

	return WeUi_Gfx_Color_FromRgb(&rgb);
}
