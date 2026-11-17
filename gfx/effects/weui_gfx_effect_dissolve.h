#ifndef __WEUI_GFX_EFFECT_DISSOLVE_H
#define __WEUI_GFX_EFFECT_DISSOLVE_H

#include "weui_gfx_effect_types.h"

/**
 * @brief Bleed the incoming scene through the old picture pixel by pixel. Every
 *        pixel turns over once, at its own moment, spread evenly over the run.
 */
RET_STATE_t WeUi_Gfx_Effect_Dissolve(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames);

#endif /* __WEUI_GFX_EFFECT_DISSOLVE_H */
