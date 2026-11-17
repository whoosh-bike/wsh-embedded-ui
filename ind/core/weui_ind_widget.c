#include "weui_ind_widget.h"
#include "log.h"
#include "mathlib_common.h"
#include "weui_colors.h"
#include "weui_ind_fps_control.h"
#include "weui_ind_layout.h"
#include "weui_ind_remap.h"
#include "weui_rand.h"

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0

//Update LED each time WeUi_Ind_Widget_SetPixLenCtrl calls
#define LOCAL_DEBUG_UPD_LEDS_ENABLE 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT LOG_RAW
#else /* LOCAL_DEBUG_PRINT_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* LOCAL_DEBUG_PRINT_ENABLE */

static void WeUi_Ind_Widget_SetPixLenCtrl(u16 pixIdx, WeUi_ColorSchemeHsb_t color,
										  const WeUi_Ind_VirtMap_t* pVmap) {
	if (pixIdx < pVmap->Len)
		WeUi_Ind_LedRemap_SetPixel(pVmap->Ptr[pixIdx], color);

#if LOCAL_DEBUG_UPD_LEDS_ENABLE
	WeUi_Ind_FpsCtrl_DrawAndKeepFPS_MainFreq();
#endif /* LOCAL_DEBUG_UPD_LEDS_ENABLE */
}

void WeUi_Ind_Widget_SetPix(u16 pixIdx, WeUi_ColorSchemeHsb_t color,
							const WeUi_Ind_VirtMap_t* pVmap) {
	WeUi_Ind_Widget_SetPixLenCtrl(pixIdx, color, pVmap);
}

static WeUi_ColorSchemeHsb_t WeUi_Ind_Widget_GetPixLenCtrl(u16 pixIdx,
														   const WeUi_Ind_VirtMap_t* pVmap) {
	if (pixIdx < pVmap->Len)
		return WeUi_Ind_LedRemap_GetPixel(pVmap->Ptr[pixIdx]);

	return WeUi_Color_GetEmptyHsb();
}

WeUi_ColorSchemeHsb_t WeUi_Ind_Widget_GetPix(u16 pixIdx, const WeUi_Ind_VirtMap_t* pVmap) {
	return WeUi_Ind_Widget_GetPixLenCtrl(pixIdx, pVmap);
}

void WeUi_Ind_Widget_SetPixRstPrev(u16 pixIdx, WeUi_ColorSchemeHsb_t color,
								   const WeUi_Ind_VirtMap_t* pVmap, bool dir) {
	WeUi_Ind_Widget_SetPixLenCtrl(pixIdx, color, pVmap);

	s32 rstPixIdx = pixIdx;
	if (dir == true && rstPixIdx >= 1)
		WeUi_Ind_Widget_SetPixLenCtrl(rstPixIdx - 1, WeUi_Color_GetEmptyHsb(), pVmap);
	else if (dir == false && rstPixIdx < pVmap->Len - 1)
		WeUi_Ind_Widget_SetPixLenCtrl(rstPixIdx + 1, WeUi_Color_GetEmptyHsb(), pVmap);
}

void WeUi_Ind_Widget_FillColor_FromTo(u16 pixIdxFrom, u16 pixIdxTo, WeUi_ColorSchemeHsb_t color,
									  const WeUi_Ind_VirtMap_t* pVmap) {
	ASSERT_CHECK(pixIdxFrom <= pixIdxTo);

	for (u16 idx = pixIdxFrom; idx < pixIdxTo; idx++) {
		WeUi_Ind_Widget_SetPixLenCtrl(idx, color, pVmap);
	}
}

void WeUi_Ind_Widget_FillColor(WeUi_ColorSchemeHsb_t color, const WeUi_Ind_VirtMap_t* pVmap) {
	WeUi_Ind_Widget_FillColor_FromTo(0, pVmap->Len, color, pVmap);
}

void WeUi_Ind_Widget_FillNoise(WeUi_ColorSchemeHsb_t color, const WeUi_Ind_VirtMap_t* pVmap) {
	for (u16 idx = 0; idx < pVmap->Len; idx++) {
		WeUi_ColorSchemeHsb_t newColor = WeUi_Rand_Bool() ? color : WeUi_Color_GetEmptyHsb();
		WeUi_Ind_Widget_SetPixLenCtrl(idx, newColor, pVmap);
	}
}

void WeUi_Ind_Widget_FillFadeNoise(u32 cnt, WeUi_ColorSchemeHsb_t color, u32 genSpeed, u32 maxSpeed,
								   const WeUi_Ind_VirtMap_t* pVmap) {
	ASSERT_CHECK(genSpeed > 0);
	ASSERT_CHECK(maxSpeed > 0);
	ASSERT_CHECK(maxSpeed >= genSpeed);

	if (genSpeed > maxSpeed)
		genSpeed = maxSpeed;

	u32 threshold = maxSpeed - genSpeed + 1;
	if (cnt % threshold == 0) {
		u32 randLedIdx = WeUi_Rand_Between(0, pVmap->Len - 1);
		color.Bri	   = WEUI_COLOR_HSB_MAX_BRI;
		WeUi_Ind_Widget_SetPixLenCtrl(randLedIdx, color, pVmap);
	}

	float briSpeed = WEUI_COLOR_HSB_MAX_BRI / (float)maxSpeed;
	for (u32 idx = 0; idx < pVmap->Len; idx++) {
		WeUi_ColorSchemeHsb_t oldColor = WeUi_Ind_Widget_GetPixLenCtrl(idx, pVmap);
		if (oldColor.Bri > WEUI_COLOR_HSB_MIN_BRI) {
			oldColor.Bri -= briSpeed;
			oldColor.Bri =
				CUT_MIN_MAX(oldColor.Bri, WEUI_COLOR_HSB_MIN_BRI, WEUI_COLOR_HSB_MAX_BRI);
			WeUi_Ind_Widget_SetPixLenCtrl(idx, oldColor, pVmap);
		}
	}
}

/**
 * [pixIdxFrom, pixIdxTo)
 * [gradStart, gradEnd]
 */
void WeUi_Ind_Widget_FillGrad_FromToGradRange(u16 pixIdxFrom, u16 pixIdxTo, WEUI_IND_GRAD_t grad,
											  bool dir, float satMult, float gradStart,
											  float gradEnd, const WeUi_Ind_VirtMap_t* pVmap) {
	ASSERT_CHECK(pixIdxFrom <= pixIdxTo);
	ASSERT_CHECK(gradStart >= 0.0f && gradStart <= 1.0f);
	ASSERT_CHECK(gradEnd >= 0.0f && gradEnd <= 1.0f);
	ASSERT_CHECK(gradStart <= gradEnd);
	ASSERT_CHECK(satMult >= 0.0f);

	u16 pixCnt = pixIdxTo - pixIdxFrom;
	if (!pixCnt)
		return;

	if (pixCnt == 1) {
		float gradPos					 = dir ? gradEnd : gradStart;
		WeUi_ColorSchemeHsb_t finalColor = WeUi_Ind_Gradient_GetColorByNorm(grad, gradPos);
		WeUi_Ind_Widget_SetPixLenCtrl(pixIdxFrom, finalColor, pVmap);
		return;
	}

	LOCAL_DEBUG_PRINT("%d, %d, %f, %f:\r\n", pixIdxFrom, pixIdxTo, gradStart, gradEnd);
	for (u16 idx = 0; idx < pixCnt; idx++) {
		float t = (float)idx / (pixCnt - 1);
		if (dir)
			t = 1.0f - t;

		float gradPos = gradStart + t * (gradEnd - gradStart);
		u16 gradIdx	  = WeUi_Ind_Gradient_GetIdxByNorm(grad, gradPos);

		LOCAL_DEBUG_PRINT("\t%d, %f, %f, %d \r\n", idx, t, gradPos, gradIdx);

		WeUi_ColorSchemeHsb_t newColor = WeUi_Ind_Gradient_GetColor(grad, gradIdx);
		newColor.Sat =
			CUT_MIN_MAX(newColor.Sat * satMult, WEUI_COLOR_HSB_MIN_SAT, WEUI_COLOR_HSB_MAX_SAT);

		u16 pixIdx = pixIdxFrom + idx;
		WeUi_Ind_Widget_SetPixLenCtrl(pixIdx, newColor, pVmap);
	}
}

void WeUi_Ind_Widget_FillGrad_FromTo(u16 pixIdxFrom, u16 pixIdxTo, WEUI_IND_GRAD_t grad, bool dir,
									 float satMult, const WeUi_Ind_VirtMap_t* pVmap) {
	WeUi_Ind_Widget_FillGrad_FromToGradRange(pixIdxFrom, pixIdxTo, grad, dir, satMult, 0.0f, 1.0f,
											 pVmap);
}

void WeUi_Ind_Widget_FillGrad(WEUI_IND_GRAD_t grad, bool dir, float satMult,
							  const WeUi_Ind_VirtMap_t* pVmap) {
	WeUi_Ind_Widget_FillGrad_FromTo(0, pVmap->Len, grad, dir, satMult, pVmap);
}

void WeUi_Ind_Widget_FallToHalfStrip(s32* pFullProgress, s32* pCycleProgress,
									 WeUi_ColorSchemeHsb_t color, const WeUi_Ind_VirtMap_t* pVmap) {
	if (*pFullProgress == -1) {
		WeUi_Ind_Widget_FillColor(WeUi_Color_GetEmptyHsb(), pVmap);
		*pFullProgress	= pVmap->Len;
		*pCycleProgress = 0;
	}

	if (*pFullProgress > 0) {
		if (*pCycleProgress < *pFullProgress) {
			WeUi_Ind_Widget_SetPixRstPrev(*pCycleProgress, color, pVmap, true);
			(*pCycleProgress)++;
		} else {
			(*pFullProgress)--;
			*pCycleProgress = 0;
		}
	} else {
		*pFullProgress = -1;  //-1 means that widget is not initialized
	}
}

void WeUi_Ind_Widget_FadeFlow(u16 startPix, WeUi_ColorSchemeHsb_t color, u16 headLen, u16 fullLen,
							  float k, const WeUi_Ind_VirtMap_t* pVmap) {
	ASSERT_CHECK(headLen <= fullLen);

	u16 tailLen		 = fullLen - headLen;
	float initialBri = color.Bri;

	s32 newIdx = 0;
	for (u16 idx = 0; idx < fullLen; idx++) {
		newIdx = startPix - idx;
		if (newIdx >= 0 && newIdx < pVmap->Len) {
			if (idx >= headLen) {
				float x	  = idx - headLen;
				float bri = exp_curve(x, 0.0f, (float)tailLen, initialBri, 2.0f, k);
				color.Bri = bri;
			}

			WeUi_Ind_Widget_SetPixLenCtrl(newIdx, color, pVmap);
		}
	}

	color.Bri = WEUI_COLOR_HSB_MIN_BRI;
	WeUi_Ind_Widget_SetPixLenCtrl(newIdx, color, pVmap);
}

void WeUi_Ind_Widget_Progress(float progress, WEUI_IND_GRAD_t grad, const WeUi_Ind_VirtMap_t* pVmap,
							  bool dir, float satMult, s32* pFlowPos) {
	ASSERT_CHECK(progress >= 0 && progress <= 1.0f);
	ASSERT_CHECK(satMult >= 0.0f);

	WeUi_Ind_Widget_FillColor(WeUi_Color_GetEmptyHsb(), pVmap);

	WeUi_ColorSchemeHsb_t newColor = WeUi_Ind_Gradient_GetColorByNorm(grad, 1.0f - progress);
	newColor.Sat =
		CUT_MIN_MAX(newColor.Sat * satMult, WEUI_COLOR_HSB_MIN_SAT, WEUI_COLOR_HSB_MAX_SAT);

	if (*pFlowPos > -1) {
		WeUi_Ind_Widget_FadeFlow(*pFlowPos, newColor, 1, pVmap->Len, -0.2f, pVmap);

		if (++(*pFlowPos) > pVmap->Len * 2)
			*pFlowPos = -1;
	}

	u16 progressLen = (u16)floorf(progress * pVmap->Len);
	WeUi_Ind_Widget_FillColor_FromTo(0, progressLen, newColor, pVmap);
	newColor.Bri *= modff(progress * pVmap->Len, &(float){0});
	WeUi_Ind_Widget_SetPixLenCtrl(progressLen, newColor, pVmap);
}

/**
 * Fade flow table
 *
 * | orient | dir | direction |    path     |
 * |--------|-----|-----------|-------------|
 * |   1    |  0  |   out     | bot-to-top  |
 * |   1    |  1  |   in      | top-to-bot  |
 * |   0    |  0  |   out     | top-to-bot  |
 * |   0    |  1  |   in      | bot-to-top  |
 */

void WeUi_Ind_Widget_ProgressMulticolor(float progress, WEUI_IND_GRAD_t grad,
										const WeUi_Ind_VirtMap_t* pVmap,
										const WeUi_Ind_VirtMap_t* pVmapFlow, bool dir,
										float satMult, s32* pFlowPos) {
	ASSERT_CHECK(pFlowPos != NULL);
	ASSERT_CHECK(progress >= 0 && progress <= 1.0f);
	ASSERT_CHECK(satMult >= 0.0f);

	if (!pFlowPos)
		return;

	float revProgress = 1.0f - progress;

	if (*pFlowPos > -1) {
		WeUi_ColorSchemeHsb_t fadeFlowColor = WeUi_Ind_Gradient_GetColorByNorm(grad, revProgress);
		fadeFlowColor.Sat = CUT_MIN_MAX(fadeFlowColor.Sat * satMult, WEUI_COLOR_HSB_MIN_SAT,
										WEUI_COLOR_HSB_MAX_SAT);

		WeUi_Ind_Widget_FadeFlow(*pFlowPos, fadeFlowColor, 1, pVmapFlow->Len, -0.2f, pVmapFlow);

		if (++(*pFlowPos) > pVmapFlow->Len * 2)
			*pFlowPos = -1;
	}

	u16 progressLen = (u16)ceilf(progress * pVmap->Len);
	WeUi_Ind_Widget_FillGrad_FromToGradRange(0, progressLen, grad, true, satMult, revProgress, 1.0f,
											 pVmap);
	WeUi_ColorSchemeHsb_t oldColor = WeUi_Ind_Widget_GetPix(progressLen - 1, pVmap);
	oldColor.Bri *= modff(progress * pVmap->Len, &(float){0});
	WeUi_Ind_Widget_SetPixLenCtrl(progressLen - 1, oldColor, pVmap);
}

void WeUi_Ind_Widget_Perlin1D_Grad(u32 noise, float speedScale, float lenScale, float gradScale,
								   WEUI_IND_GRAD_t grad, float satMult, bool isReverse,
								   const WeUi_Ind_VirtMap_t* pVmap) {
	ASSERT_CHECK(speedScale > 0.0f);
	ASSERT_CHECK(lenScale > 0.0f);
	ASSERT_CHECK(satMult >= 0.0f);

	u32 currFreq  = WeUi_Ind_FpsCtrl_GetMainFreq();
	float speed	  = (float)noise / ((float)currFreq * speedScale);
	float lenStep = 1.0f / ((float)(pVmap->Len - 1) * lenScale);

	for (u16 idx = 0; idx < pVmap->Len; idx++) {
		float lenIdx = idx;
		if (isReverse)
			lenIdx = pVmap->Len - idx - 1;

		float noiseValue = perlin_1D(speed + lenStep * lenIdx);	 // find info in gradient_gen.py
		noiseValue		 = SHIFT_AND_NORM(noiseValue);			 // norm
		noiseValue		 = noiseValue * gradScale;				 // stretch

		WeUi_ColorSchemeHsb_t color = WeUi_Ind_Gradient_GetWrapColorByCircle(grad, noiseValue);
		color.Sat =
			CUT_MIN_MAX(color.Sat * satMult, WEUI_COLOR_HSB_MIN_SAT, WEUI_COLOR_HSB_MAX_SAT);

		WeUi_Ind_Widget_SetPixLenCtrl(idx, color, pVmap);
	}
}

void WeUi_Ind_Widget_Perlin1D_Bright(u32 noise, float hue, float speedScale, float lenScale,
									 float gradScale, float satMult, bool isReverse,
									 const WeUi_Ind_VirtMap_t* pVmap) {
	ASSERT_CHECK(speedScale > 0.0f);
	ASSERT_CHECK(lenScale > 0.0f);
	ASSERT_CHECK(satMult >= 0.0f);

	u32 currFreq  = WeUi_Ind_FpsCtrl_GetMainFreq();
	float speed	  = (float)noise / (currFreq * speedScale);
	float lenStep = 1.0f / ((float)(pVmap->Len - 1) * lenScale);

	for (u16 idx = 0; idx < pVmap->Len; idx++) {
		float lenIdx = idx;
		if (isReverse)
			lenIdx = pVmap->Len - idx - 1;

		float noiseValue = perlin_1D(speed + lenStep * lenIdx);
		noiseValue *= gradScale;
		noiseValue = CUT_NORM_FLOAT(noiseValue);

		WeUi_ColorSchemeHsb_t color = WeUi_Color_GetFullHsb(hue);
		color.Sat =
			CUT_MIN_MAX(color.Sat * satMult, WEUI_COLOR_HSB_MIN_SAT, WEUI_COLOR_HSB_MAX_SAT);
		color.Bri = WEUI_COLOR_HSB_MAX_BRI * noiseValue;

		WeUi_Ind_Widget_SetPixLenCtrl(idx, color, pVmap);
	}
}

void WeUi_Ind_Widget_Perlin3D_Grad(u32 noise, float speedScale, float lenScale, float gradScale,
								   WEUI_IND_GRAD_t grad, float satMult, float briOvrd,
								   const WeUi_Ind_VirtMap_t* pVmap) {
	ASSERT_CHECK(speedScale >= 1.0f);  // Set >= 1 for every scaler, because we multiply by them
	ASSERT_CHECK(lenScale >= 1.0f);
	ASSERT_CHECK(gradScale >= 1.0f);
	ASSERT_CHECK(satMult >= 0.0f);
	ASSERT_CHECK(briOvrd >= -1.0f && briOvrd <= WEUI_COLOR_HSB_MAX_BRI);

	// lenScale *= 3.0f;  // Additionally multiplying by 3, because this is minimum that looks "normal"
	// Additionally multiplying by 1/10 of frame rate, so it's normalized to time and looks "normal"
	// u32 currFreq = WeUi_Ind_FpsCtrl_GetMainFreq();
	// speedScale *= (currFreq / 10.f);

	for (u16 idx = 0; idx < pVmap->Len; idx++) {
		Point2D_t pt	 = WeUi_Ind_LedsLayout_GetPt(idx);
		float noiseValue = perlin_3D(pt.X / lenScale, pt.Y / lenScale, noise / speedScale);
		noiseValue		 = SHIFT_AND_NORM(noiseValue);	// norm

		// Scale value, so it lays in range (0, gradScale) for further wrapping
		noiseValue *= gradScale;

		// Wrap noise to get value for gradient that ranges from 0 to 1, also making every gradient "ring-like"
		WeUi_ColorSchemeHsb_t color = WeUi_Ind_Gradient_GetWrapColorByCircle(grad, noiseValue);
		color.Sat =
			CUT_MIN_MAX(color.Sat * satMult, WEUI_COLOR_HSB_MIN_SAT, WEUI_COLOR_HSB_MAX_SAT);
		if (briOvrd >= 0)
			color.Bri = CUT_MIN_MAX(briOvrd, WEUI_COLOR_HSB_MIN_BRI, WEUI_COLOR_HSB_MAX_BRI);

		WeUi_Ind_Widget_SetPixLenCtrl(idx, color, pVmap);
	}
}
