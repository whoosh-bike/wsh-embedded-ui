#ifndef __WEUI_GFX_EFFECT_ZOOM_H
#define __WEUI_GFX_EFFECT_ZOOM_H

#include "weui_gfx_effect_types.h"

/**
 * @brief Grow the incoming scene out of the panel centre, scaled whole, over the
 *        old picture until it fills the panel.
 */
RET_STATE_t WeUi_Gfx_Effect_Zoom(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames);

#endif /* __WEUI_GFX_EFFECT_ZOOM_H */
