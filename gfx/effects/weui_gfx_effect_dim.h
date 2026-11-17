#ifndef __WEUI_GFX_EFFECT_DIM_H
#define __WEUI_GFX_EFFECT_DIM_H

#include "weui_gfx_effect_types.h"

/**
 * @brief Dim the old picture down to black, then bring the incoming scene up out
 *        of it - the two never mix, the dark is the seam.
 */
RET_STATE_t WeUi_Gfx_Effect_Dim(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames);

#endif /* __WEUI_GFX_EFFECT_DIM_H */
