#ifndef __WEUI_GFX_EFFECT_RUNNER_H
#define __WEUI_GFX_EFFECT_RUNNER_H

#include "weui_gfx_effect_types.h"

/**
 * @brief True while an effect owns the panel, including inside a transition's own
 *        renderIncoming. The aux frame holds one picture, so effects cannot nest.
 */
bool WeUi_Gfx_Effect_IsPlaying(void);

/**
 * @brief Frames that `ms` takes at `fps`, rounded and never under WEUI_GFX_EFFECT_FRAMES_MIN.
 *
 * A length is thought of in ms and played in frames, at the rate of whatever draws
 * them: the scene manager's transitions and the solo effects run at the main render
 * rate, so their lengths convert at WEUI_GFX_FPS_CTRL_DEF_RENDER_FREQ - and a main
 * rate lowered from the shell slows them down for a closer look.
 */
u16 WeUi_Gfx_Effect_Frames(u32 ms, u32 fps);

/**
 * @brief Play a solo effect - one picture, no partner: no outgoing frame to mix
 *        and no incoming scene. Per frame: clear, paint, push at the main rate.
 *
 * Paces through the FPS control as a scene does, so the host's per-frame app
 * callback keeps running while this blocks.
 *
 * @param frames     clamped to WEUI_GFX_EFFECT_FRAMES_MIN
 * @param pcParams   reaches `paint` through the context, untouched. May be NULL.
 * @return RET_STATE_ERR_BUSY if another effect already owns the panel
 */
RET_STATE_t WeUi_Gfx_Effect_RunSolo(WeUi_Gfx_Effect_FrameCb_t paint, u16 frames,
									const void* pcParams);

/**
 * @brief Play a transition. The outgoing picture is whatever is on the panel on
 *        entry; `renderIncoming` draws the arriving scene live every frame and
 *        `compose` mixes the outgoing one back over it.
 *
 * The frame is half a picture until compose has run, so the FPS control's draw
 * callback is detached for the duration and frames are pushed here instead. The
 * host's app callback runs only if `renderIncoming` paces itself through
 * DrawAndKeepFPS, as the one the scene manager hands its transitions does.
 *
 * @param frames     transition length: more is slower and smoother
 * @param pcParams   reaches `compose` through the context, untouched. May be NULL.
 * @return RET_STATE_ERR_BUSY if another effect already owns the panel
 */
RET_STATE_t WeUi_Gfx_Effect_RunTransition(WeUi_Gfx_Effect_FrameCb_t compose, u16 frames,
										  const void* pcParams,
										  WeUi_Gfx_Effect_RenderCb_t renderIncoming);

#endif /* __WEUI_GFX_EFFECT_RUNNER_H */
