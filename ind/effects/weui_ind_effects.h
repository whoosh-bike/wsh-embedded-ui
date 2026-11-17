#ifndef __WEUI_IND_EFFECTS_H
#define __WEUI_IND_EFFECTS_H

#include "weui_ind_gradient.h"
#include "weui_ind_types.h"

void WeUi_Ind_Effect_DistortAndFlash(const WeUi_Ind_VirtMap_t* pVmap);
void WeUi_Ind_Effect_Perlin1D(u32 showCycles, WEUI_IND_GRAD_t grad, float speedScale,
							  float lenScale, const WeUi_Ind_VirtMap_t* pVmap);
void WeUi_Ind_Effect_Perlin3D(u32 showCycles, WEUI_IND_GRAD_t grad, float speedScale,
							  float lenScale, float gradScale, const WeUi_Ind_VirtMap_t* pVmap);

#endif /* __WEUI_IND_EFFECTS_H */
