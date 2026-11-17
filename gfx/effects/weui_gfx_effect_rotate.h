#ifndef __WEUI_GFX_EFFECT_ROTATE_H
#define __WEUI_GFX_EFFECT_ROTATE_H

#include "weui_gfx_effect_types.h"

/**
 * @brief Turn the panel over like a card on its vertical axis: the old picture
 *        narrows to an edge, the incoming scene opens out of it on the far side.
 */
RET_STATE_t WeUi_Gfx_Effect_Rotate(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames);

#endif /* __WEUI_GFX_EFFECT_ROTATE_H */
