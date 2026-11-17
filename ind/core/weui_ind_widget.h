#ifndef __WEUI_IND_WIDGET_H
#define __WEUI_IND_WIDGET_H

#include "weui_ind_gradient.h"
#include "weui_ind_types.h"

void WeUi_Ind_Widget_SetPix(u16 pixIdx, WeUi_ColorSchemeHsb_t color,
							const WeUi_Ind_VirtMap_t* pVmap);
WeUi_ColorSchemeHsb_t WeUi_Ind_Widget_GetPix(u16 pixIdx, const WeUi_Ind_VirtMap_t* pVmap);
void WeUi_Ind_Widget_SetPixRstPrev(u16 pixIdx, WeUi_ColorSchemeHsb_t color,
								   const WeUi_Ind_VirtMap_t* pVmap, bool dir);
void WeUi_Ind_Widget_FillColor_FromTo(u16 pixIdxFrom, u16 pixIdxTo, WeUi_ColorSchemeHsb_t color,
									  const WeUi_Ind_VirtMap_t* pVmap);
void WeUi_Ind_Widget_FillColor(WeUi_ColorSchemeHsb_t color, const WeUi_Ind_VirtMap_t* pVmap);
void WeUi_Ind_Widget_FillNoise(WeUi_ColorSchemeHsb_t color, const WeUi_Ind_VirtMap_t* pVmap);
void WeUi_Ind_Widget_FillFadeNoise(u32 cnt, WeUi_ColorSchemeHsb_t color, u32 genSpeed, u32 maxSpeed,
								   const WeUi_Ind_VirtMap_t* pVmap);
void WeUi_Ind_Widget_FillGrad_FromToGradRange(u16 pixIdxFrom, u16 pixIdxTo, WEUI_IND_GRAD_t grad,
											  bool dir, float satMult, float gradStart,
											  float gradEnd, const WeUi_Ind_VirtMap_t* pVmap);
void WeUi_Ind_Widget_FillGrad_FromTo(u16 pixIdxFrom, u16 pixIdxTo, WEUI_IND_GRAD_t grad, bool dir,
									 float satMult, const WeUi_Ind_VirtMap_t* pVmap);
void WeUi_Ind_Widget_FillGrad(WEUI_IND_GRAD_t grad, bool dir, float satMult,
							  const WeUi_Ind_VirtMap_t* pVmap);
void WeUi_Ind_Widget_FallToHalfStrip(s32* pFullProgress, s32* pCycleProgress,
									 WeUi_ColorSchemeHsb_t color, const WeUi_Ind_VirtMap_t* pVmap);
void WeUi_Ind_Widget_FadeFlow(u16 startPix, WeUi_ColorSchemeHsb_t color, u16 headLen, u16 fullLen,
							  float k, const WeUi_Ind_VirtMap_t* pVmap);
void WeUi_Ind_Widget_Progress(float progress, WEUI_IND_GRAD_t grad, const WeUi_Ind_VirtMap_t* pVmap,
							  bool dir, float satMult, s32* pFlowPos);
void WeUi_Ind_Widget_ProgressMulticolor(float progress, WEUI_IND_GRAD_t grad,
										const WeUi_Ind_VirtMap_t* pVmap,
										const WeUi_Ind_VirtMap_t* pVmapFlow, bool dir,
										float satMult, s32* pFlowPos);
void WeUi_Ind_Widget_Perlin1D_Grad(u32 noise, float speedScale, float lenScale, float gradScale,
								   WEUI_IND_GRAD_t grad, float satMult, bool isReverse,
								   const WeUi_Ind_VirtMap_t* pVmap);
void WeUi_Ind_Widget_Perlin1D_Bright(u32 noise, float hue, float speedScale, float lenScale,
									 float gradScale, float satMult, bool isReverse,
									 const WeUi_Ind_VirtMap_t* pVmap);
void WeUi_Ind_Widget_Perlin3D_Grad(u32 noise, float speedScale, float lenScale, float gradScale,
								   WEUI_IND_GRAD_t grad, float satMult, float briOvrd,
								   const WeUi_Ind_VirtMap_t* pVmap);

#endif /* __WEUI_IND_WIDGET_H */
