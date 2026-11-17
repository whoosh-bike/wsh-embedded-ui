#ifndef __WEUI_GFX_EFFECT_PIXELATE_H
#define __WEUI_GFX_EFFECT_PIXELATE_H

#include "weui_gfx_effect_types.h"

/**
 * @brief Break the old picture into ever coarser blocks, swap the incoming scene in
 *        under the coarsest, then sharpen it back down to single pixels.
 */
RET_STATE_t WeUi_Gfx_Effect_Pixelate(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames);

#endif /* __WEUI_GFX_EFFECT_PIXELATE_H */
