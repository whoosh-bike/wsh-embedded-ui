#ifndef __WEUI_GFX_EFFECTS_H
#define __WEUI_GFX_EFFECTS_H

/**
 * @file weui_gfx_effects.h
 * @brief Umbrella header for the effects module, one effect per file.
 *
 * Three levels, each built on the one below:
 *   painter     one frame - Plasma_Paint, Perlin_Paint, Static_Paint.
 *               A scene calls these directly and paces itself.
 *   solo        a painter looped over frames, the panel to itself - *_Play.
 *               One picture, no partner: nothing to mix, nothing to park.
 *   transition  a compose looped over frames, mixing back an outgoing picture
 *               the runner parks in the aux frame:
 *                 fades    Dim, Fade, Flash
 *                 slides   Push, Swipe, Cover, Curtain, Interlace
 *                 reveals  Dissolve, Blocks, Blinds, Reload, Wave, Rain, Melt, Ripple
 *                 warps    Zoom, Rotate, Pixelate
 *
 * The runner is public as well: a host writes an effect of its own by handing
 * WeUi_Gfx_Effect_RunSolo/RunTransition a frame callback.
 *
 * What each one looks like and what to know about it: EFFECTS.md, beside this file.
 *
 * Every effect is played for a number of frames - WeUi_Gfx_Effect_Frames() turns a
 * length in ms into them. Look and feel is per call, through arguments or a cfg
 * struct, not per build - passing NULL for a cfg takes the library defaults.
 */

#include "weui_gfx_effect_blinds.h"
#include "weui_gfx_effect_blocks.h"
#include "weui_gfx_effect_cover.h"
#include "weui_gfx_effect_curtain.h"
#include "weui_gfx_effect_dim.h"
#include "weui_gfx_effect_dissolve.h"
#include "weui_gfx_effect_fade.h"
#include "weui_gfx_effect_flash.h"
#include "weui_gfx_effect_interlace.h"
#include "weui_gfx_effect_melt.h"
#include "weui_gfx_effect_perlin.h"
#include "weui_gfx_effect_pixelate.h"
#include "weui_gfx_effect_plasma.h"
#include "weui_gfx_effect_push.h"
#include "weui_gfx_effect_rain.h"
#include "weui_gfx_effect_reload.h"
#include "weui_gfx_effect_ripple.h"
#include "weui_gfx_effect_rotate.h"
#include "weui_gfx_effect_runner.h"
#include "weui_gfx_effect_static.h"
#include "weui_gfx_effect_types.h"
#include "weui_gfx_effect_wave.h"
#include "weui_gfx_effect_zoom.h"

#endif /* __WEUI_GFX_EFFECTS_H */
