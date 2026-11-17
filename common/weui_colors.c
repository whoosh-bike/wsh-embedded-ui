#include "weui_colors.h"

WeUi_ColorSchemeHsb_t WeUi_Color_GetEmptyHsb(void) {
	WeUi_ColorSchemeHsb_t color = {
		.Hue = WEUI_COLOR_HSB_MIN_HUE,
		.Sat = WEUI_COLOR_HSB_MIN_SAT,
		.Bri = WEUI_COLOR_HSB_MIN_BRI,
	};

	return color;
}

WeUi_ColorSchemeHsb_t WeUi_Color_GetFullHsb(float hue) {
	WeUi_ColorSchemeHsb_t color = {
		.Hue = hue,
		.Sat = WEUI_COLOR_HSB_MAX_SAT,
		.Bri = WEUI_COLOR_HSB_MAX_BRI,
	};

	return color;
}

void WeUi_Color_Hsb2Rgb(WeUi_ColorSchemeHsb_t* pHsb, WeUi_ColorSchemeRgb_t* pRgb) {
	float s = pHsb->Sat / WEUI_COLOR_HSB_MAX_SAT;
	float b = pHsb->Bri / WEUI_COLOR_HSB_MAX_BRI;

	/* Converting a negative hue to the sector index is undefined, not just wrong */
	float hue = fmodf(pHsb->Hue, WEUI_COLOR_HSB_FULL_TURN);
	if (hue < 0.0f)
		hue += WEUI_COLOR_HSB_FULL_TURN;

	u16 i	= (u16)(hue / 60);
	float f = hue / 60.0f - i;
	float p = b * (1.0f - s);
	float q = b * (1.0f - s * f);
	float t = b * (1.0f - s * (1.0f - f));
	float R, G, B;

	switch (i) {
		case 0:
			R = b, G = t, B = p;
			break;
		case 1:
			R = q, G = b, B = p;
			break;
		case 2:
			R = p, G = b, B = t;
			break;
		case 3:
			R = p, G = q, B = b;
			break;
		case 4:
			R = t, G = p, B = b;
			break;
		case 5:
			R = b, G = p, B = q;
			break;
		default:
			R = G = B = 0;
			break;
	}

	pRgb->Red = R, pRgb->Green = G, pRgb->Blue = B;
}

void WeUi_Color_Rgb2Hsb(WeUi_ColorSchemeRgb_t* pRgb, WeUi_ColorSchemeHsb_t* pHsb) {
	float r = pRgb->Red;
	float g = pRgb->Green;
	float b = pRgb->Blue;

	float max	= fmaxf(r, fmaxf(g, b));
	float min	= fminf(r, fminf(g, b));
	float delta = max - min;

	pHsb->Bri = roundf(max * WEUI_COLOR_HSB_MAX_BRI);
	pHsb->Sat = (max == 0.0f) ? 0.0f : roundf((delta / max) * WEUI_COLOR_HSB_MAX_SAT);

	if (delta == 0.0f) {
		pHsb->Hue = 0.0f;
	} else {
		float hue;
		if (max == r) {
			hue = (g - b) / delta + (g < b ? 6.0f : 0.0f);
		} else if (max == g) {
			hue = (b - r) / delta + 2.0f;
		} else {
			hue = (r - g) / delta + 4.0f;
		}
		pHsb->Hue = roundf(hue * 60.0f);
	}
}

float WeUi_Color_HueIncDec(float hue, float delta) {
	float newHue = fmodf(hue + delta, WEUI_COLOR_HSB_MAX_HUE);
	if (newHue < 0.0f)
		newHue += WEUI_COLOR_HSB_MAX_HUE;

	return newHue;
}

WeUi_ColorSchemeHsb_t WeUi_Color_GenerateGradient(WeUi_ColorSchemeHsb_t color1,
												  WeUi_ColorSchemeHsb_t color2, float ratio) {
	WeUi_ColorSchemeRgb_t rgb1, rgb2;
	WeUi_Color_Hsb2Rgb(&color1, &rgb1);
	WeUi_Color_Hsb2Rgb(&color2, &rgb2);

	WeUi_ColorSchemeRgb_t rgb3 = {
		.Red   = rgb1.Red + (rgb2.Red - rgb1.Red) * ratio,
		.Green = rgb1.Green + (rgb2.Green - rgb1.Green) * ratio,
		.Blue  = rgb1.Blue + (rgb2.Blue - rgb1.Blue) * ratio,
	};

	WeUi_ColorSchemeHsb_t result;
	WeUi_Color_Rgb2Hsb(&rgb3, &result);

	return result;
}
