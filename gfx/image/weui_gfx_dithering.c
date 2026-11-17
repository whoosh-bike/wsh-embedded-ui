#include "weui_gfx_dithering.h"
#include "log.h"

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT LOG
#else /* DEBUG_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* DEBUG_ENABLE */

#if WEUI_USE_COMPONENT__IMAGE_DITHER

#include "weui_gfx_transform.h"

static float WEUI_IMAGES_OPS_DATA_PLACEMENT DithBuff[WEUI_DISPLAY_WIDTH * WEUI_DISPLAY_HEIGHT];

bool WeUi_Gfx_Dither_FloydSteinberg(WeUi_Gfx_Image_t* pImgObj) {
	for (u32 i = 0; i < pImgObj->Width * pImgObj->Height; i++) {
		WeUi_Gfx_Color_t colorRgb = pImgObj->ImageOrig[i];

#ifdef WEUI_DISPLAY_GRAY
		WeUi_Gfx_ColorChannel_t grayLuma = WeUi_Gfx_Transform_Color2Byte_Luma(colorRgb);
		DithBuff[i]						 = WeUi_Gfx_Transform_GammaDecode((float)grayLuma);
#else  /* WEUI_DISPLAY_GRAY */
		DithBuff[i] = WeUi_Gfx_Transform_GammaDecode((float)colorRgb.R);
#endif /* WEUI_DISPLAY_GRAY */
	}

	for (u32 y = 0; y < pImgObj->Height; y++) {
		for (u32 x = 0; x < pImgObj->Width; x++) {
			float oldPixel = DithBuff[y * pImgObj->Width + x];
			oldPixel	   = WeUi_Gfx_Transform_ColorAdjust(oldPixel, &pImgObj->Transform);
			float newPixel = oldPixel > 127.0f ? 255.0f : 0.0f;

			WeUi_Gfx_ColorChannel_t colorChannel = (WeUi_Gfx_ColorChannel_t)newPixel;
			WeUi_Gfx_Color_t colorNew =
				WeUi_Gfx_Transform_Rgb2Color(colorChannel, colorChannel, colorChannel);
			pImgObj->ImageAux[y * pImgObj->Width + x] = colorNew;
			float error								  = oldPixel - newPixel;

			u32 i1;
			float p1, k1;
			if (x + 1 < pImgObj->Width) {
				i1			 = y * pImgObj->Width + (x + 1);
				p1			 = DithBuff[i1];
				k1			 = (error * 7.0f) / 16.0f;
				DithBuff[i1] = p1 + k1;
			}
			if (x > 0 && y + 1 < pImgObj->Height) {
				i1			 = (y + 1) * pImgObj->Width + (x - 1);
				p1			 = DithBuff[i1];
				k1			 = (error * 3.0f) / 16.0f;
				DithBuff[i1] = p1 + k1;
			}
			if (y + 1 < pImgObj->Height) {
				i1			 = (y + 1) * pImgObj->Width + x;
				p1			 = DithBuff[i1];
				k1			 = (error * 5.0f) / 16.0f;
				DithBuff[i1] = p1 + k1;
			}
			if (x + 1 < pImgObj->Width && y + 1 < pImgObj->Height) {
				i1			 = (y + 1) * pImgObj->Width + (x + 1);
				p1			 = DithBuff[i1];
				k1			 = (error * 1.0f) / 16.0f;
				DithBuff[i1] = p1 + k1;
			}
		}
	}

	return true;
}

#else  /* WEUI_USE_COMPONENT__IMAGE_DITHER */
bool WeUi_Gfx_Dither_FloydSteinberg(WeUi_Gfx_Image_t* pImgObj) {
	return false;
}
#endif /* WEUI_USE_COMPONENT__IMAGE_DITHER */
