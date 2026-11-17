#ifndef __WEUI_GFX_EFFECT_MELT_H
#define __WEUI_GFX_EFFECT_MELT_H

#include "weui_gfx_effect_types.h"

/**
 * @brief The old picture melts: its columns sag and drop off, the incoming one above.
 * @param columnPx  width of a column: a few px melt in chunks, as Doom's screen did
 */
RET_STATE_t WeUi_Gfx_Effect_Melt(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
								 u8 columnPx);

#endif /* __WEUI_GFX_EFFECT_MELT_H */
