#ifndef __WEUI_GFX_SCENEMGR_H
#define __WEUI_GFX_SCENEMGR_H

#include "weui_gfx_scene.h"
#include "weui_gfx_types.h"

/**
 * @file weui_gfx_scenemgr.h
 * @brief The scene engine: which scene is up, when it draws, how the switch looks.
 *
 * A switch, whatever asks for it - an input, a timeout, RequestScene(), the first Poll():
 *   1. Exit() of the scene going away
 *   2. the panel wiped, if the new scene sets FullClearOnEnter
 *   3. the new scene made active, its Enter() run
 *   4. the transition chosen for this switch played into it - GoTo() from an
 *      OnInput or OnTimeout, cfg.Intro on the first Poll(); without one the scene cuts in
 *   5. its timeout started, if cfg.TimeoutsMs gives it one
 *   6. its frames drawn by Poll() from then on, at the scene's own Fps
 *
 * A scene draws itself one of two ways. A drawn one (Render) paints a whole frame, which the
 * engine clears before and pushes after. A vector one (Build) gives a vector scene
 * (weui_gfx_scene.h) built afresh, and the engine puts on the panel only what changed since the
 * frame before - the way for a driver with no frame (WEUI_DISPLAY_BANDS), and for any frame that
 * changes in places. A transition into or out of a vector scene draws it whole into the
 * adapter's frames, which a driver with bands keeps only while cfg.FramesOpen() holds them.
 */

/** @brief Max bytes an app may post through PostInput. */
#ifndef WEUI_GFX_SCENEMGR_INPUT_MAX_BYTES
#define WEUI_GFX_SCENEMGR_INPUT_MAX_BYTES 16
#endif /* WEUI_GFX_SCENEMGR_INPUT_MAX_BYTES */

/** @brief Depth of the message ring, which is statically allocated - the engine
 *         owns no OS queue and takes no heap. */
#ifndef WEUI_GFX_SCENEMGR_QUEUE_DEPTH
#define WEUI_GFX_SCENEMGR_QUEUE_DEPTH 8
#endif /* WEUI_GFX_SCENEMGR_QUEUE_DEPTH */

/**
 * @brief Where a frame's period is measured from.
 *
 * 0 - from the start of the render. The requested rate is held as long as a
 *     frame fits in its period; a render that overruns leaves the next frame
 *     already due, so Poll() returns 0 and the host loop stops sleeping until
 *     the panel catches up. Right for a fast panel and smooth animation.
 *
 * 1 - from the end of the render. Every frame is followed by a full period of
 *     idle, so Poll() always hands the host some sleep. A render slower than
 *     the period drags the real rate down instead of saturating the task -
 *     which is what a slow panel (e-ink, where a refresh blocks on BUSY) wants.
 */
#ifndef WEUI_GFX_SCENEMGR_PACE_FROM_FRAME_END
#define WEUI_GFX_SCENEMGR_PACE_FROM_FRAME_END 0
#endif /* WEUI_GFX_SCENEMGR_PACE_FROM_FRAME_END */

/** @brief Rectangles a vector scene's changes are given in at most, more merged into the last */
#ifndef WEUI_GFX_SCENEMGR_DAMAGE_RECTS
#define WEUI_GFX_SCENEMGR_DAMAGE_RECTS 8
#endif /* WEUI_GFX_SCENEMGR_DAMAGE_RECTS */

/** @brief OnInput return value meaning "stay on the current scene". */
#define WEUI_GFX_SCENEMGR_SELF ((u32) - 1)

/** @brief Poll() return meaning no frame is scheduled at all. */
#define WEUI_GFX_SCENEMGR_SLEEP_FOREVER ((u32) - 1)

/**
 * @brief Plays a transition from what is on the panel into the active scene.
 *
 * `renderIncoming` draws one frame of that scene - clears, renders it with the
 * overlay, paces at the main rate, never pushes - which is exactly what an effect
 * takes as its incoming picture:
 *
 *     static void Dissolve(VoidFunc_t renderIncoming) {
 *         WeUi_Gfx_Effect_Dissolve(renderIncoming, frames);
 *     }
 */
typedef void (*WeUi_Gfx_SceneMgr_TransitionFn_t)(VoidFunc_t renderIncoming);

/**
 * @brief A scene's lifecycle as one const table, dispatched by index. All
 *        fields may be NULL/0 except one of Render and Build.
 */
typedef struct {
	/** @brief One frame of the scene's content, nothing else: the engine clears the
	 *         frame before it (see KeepFrame) and pushes it after. */
	void (*Render)(WeUi_Gfx_Point_t* pRefPt);

	/**
	 * @brief A vector scene, in place of Render: the frame dt seconds after the one before,
	 *        built from scratch into the scene's own WeUi_Gfx_Scene_t - its time stands still
	 *        between frames, so dt is 0 for the first one and spans whatever held the engine,
	 *        a transition or an action. NULL when it cannot be built: the frame is skipped.
	 *        The engine finds what changed and presents that, through cfg.Present. Nothing is
	 *        cleared under it, as under no vector scene: its first item is its background.
	 */
	WeUi_Gfx_Scene_t* (*Build)(float dt);

	/**
	 * @brief Return a target scene index, or WEUI_GFX_SCENEMGR_SELF to stay put.
	 *        NULL leaves the scene's inputs to cfg.OnInput.
	 *
	 * Returning the scene's own index is a re-entry - Exit() and Enter() run
	 * again - so an input that leads to a fixed scene needs no special case for
	 * "already there". Only OnInput and OnTimeout may do this; RequestScene ignores
	 * a switch to the active scene. A target returned through GoTo() comes with a
	 * transition.
	 */
	u32 (*OnInput)(const void* pcInput);

	/** @brief Runs once on activation: set the scene's state up. The arrival
	 *         transition, if there is one, plays right after it. */
	void (*Enter)(void);
	void (*Exit)(void);

	/** @brief Render rate. <= 0 renders once on Enter and never re-ticks.
	 *         Only capped from above, by the main frequency - a scene wanting a
	 *         frame every few seconds (0.2 for e-ink) says so and gets it. */
	float Fps;

	/** @brief Wipe the panel before Enter: a transition into the scene then comes
	 *         in from black rather than from the scene before. */
	bool FullClearOnEnter;

	/** @brief Start each Render from the picture on the panel instead of a
	 *         cleared frame - for trails and anything else built on the last
	 *         frame. A transition's incoming frames still start cleared: they are
	 *         composed, and feeding them back would smear the effect. */
	bool KeepFrame;
} WeUi_Gfx_SceneVtbl_t;

typedef struct {
	const WeUi_Gfx_SceneVtbl_t* const* Scenes;
	u32 SceneCount;

	/** @brief Entered on the first Poll(), so its Enter() runs with the display
	 *         already up rather than at Init() time. */
	u32 HomeScene;

	/** @brief Scene RequestClear switches to. */
	u32 ClearScene;

	/** @brief Draw-offset for every Render call, also given by GetRefPt(). */
	WeUi_Gfx_Point_t RefPt;

	/** @brief Optional, drawn over every frame of every scene right after its
	 *         Render, transitions included - for a status the app shows no
	 *         matter which scene is up (a mode number, a link icon). */
	void (*Overlay)(WeUi_Gfx_Point_t* pRefPt);

	/** @brief Optional, the vector scenes' Overlay: items added to every frame of every vector
	 *         scene right after its Build, transitions included. Their changes are found with
	 *         the scene's, so they cost only when they move. */
	void (*OverlayItems)(WeUi_Gfx_Scene_t* pScene);

	/** @brief Optional: how a vector scene's changed rectangles reach the panel - a driver's
	 *         own way, synchronised with its refresh, say. It pushes what it draws itself.
	 *         NULL: WeUi_Gfx_Scene_Present() and the frame push. */
	RET_STATE_t (*Present)(const WeUi_Gfx_Scene_t* pcScene, const WeUi_Gfx_ClipRect_t* pcRects,
						   u8 num);

	/** @brief Optional, for a driver that keeps frames only for a while: they are taken before
	 *         a transition into or out of a vector scene and given back after it. false: no
	 *         frames this time, the scene cuts in. */
	bool (*FramesOpen)(void);
	void (*FramesClose)(void);

	/** @brief Optional: the inputs of every scene that has no OnInput of its own,
	 *         same contract - the app's navigation in one place. */
	u32 (*OnInput)(const void* pcInput);

	/** @brief Optional: played into HomeScene on the first Poll(). NULL cuts in. */
	WeUi_Gfx_SceneMgr_TransitionFn_t Intro;

	/** @brief Optional, one per scene as in Scenes: how long the scene stays up before
	 *         OnTimeout is asked where to go, 0 for as long as it likes. The time runs
	 *         from the end of the arrival transition; a switch, a re-entry and
	 *         PlayIntoActive() start it over. */
	const u32* TimeoutsMs;

	/** @brief Asked when the active scene's time is up - OnInput's contract, with no
	 *         input. SELF, or no OnTimeout, keeps the scene, which then has no timeout
	 *         until it is entered again. */
	u32 (*OnTimeout)(void);
} WeUi_Gfx_SceneMgrCfg_t;

/** @brief Take the scene table. Draws nothing - the first Poll() does that. */
void WeUi_Gfx_SceneMgr_Init(const WeUi_Gfx_SceneMgrCfg_t* pcCfg);

/**
 * @brief One turn of the engine: drain the pending messages, follow the active
 *        scene's timeout if it is up, then render a frame if one is due. The host
 *        owns the loop, the sleeping and the waking - the engine neither blocks nor
 *        knows what an RTOS is.
 *
 * @return ms the caller may sleep before calling again - until the next frame or
 *         the timeout, whichever is sooner - or
 *         WEUI_GFX_SCENEMGR_SLEEP_FOREVER when nothing is scheduled. A message
 *         posted meanwhile is not seen until the next call, so a host that
 *         wants instant input wakes its own loop from its own post sites.
 */
u32 WeUi_Gfx_SceneMgr_Poll(void);

/**
 * @brief Post an input event, delivered to the active scene's OnInput.
 *
 * Safe from an interrupt: the ring is guarded by WEUI_ENTER_CRITICAL, which
 * holds in either context. Never blocks, so a full ring is a dropped event.
 */
bool WeUi_Gfx_SceneMgr_PostInput(const void* pcInput, u32 size);

/** @brief Request a scene switch from outside the engine. The scene cuts in. */
bool WeUi_Gfx_SceneMgr_RequestScene(u32 sceneIdx);

/** @brief Run Fn() once on the polling context; the active scene is untouched. */
bool WeUi_Gfx_SceneMgr_PostAction(VoidFunc_t Fn);

/**
 * @brief Switch to cfg.ClearScene, then call OnDone(pCtx) from the polling
 *        context. Blocking until the display is actually clear is the host's
 *        business - it is the only side that knows how a task is parked.
 */
bool WeUi_Gfx_SceneMgr_RequestClear(void (*OnDone)(void* pCtx), void* pCtx);

/**
 * @brief What OnInput returns to switch with a transition:
 *
 *     return WeUi_Gfx_SceneMgr_GoTo(next, PushRight);
 *
 * Nothing switches here - the engine does, once OnInput has returned, and plays
 * `transition` right after the target's Enter(). It is dropped unless that switch
 * happens. NULL, like a bare index, cuts.
 *
 * @return target
 */
u32 WeUi_Gfx_SceneMgr_GoTo(u32 target, WeUi_Gfx_SceneMgr_TransitionFn_t transition);

/**
 * @brief Play `transition` from what is on the panel into the active scene: for a
 *        PostAction that borrowed the panel - a notice, a demo - to hand it back.
 *        The scene's timeout starts over, as it is back on the panel.
 */
void WeUi_Gfx_SceneMgr_PlayIntoActive(WeUi_Gfx_SceneMgr_TransitionFn_t transition);

/** @brief RefPt from Init, for callbacks that don't get it as an arg. */
WeUi_Gfx_Point_t WeUi_Gfx_SceneMgr_GetRefPt(void);

/**
 * @brief Index of the active scene. Meaningful from a scene's own Enter(),
 *        Render() or OnInput(); before the first Poll() there is no scene and
 *        the return is WEUI_GFX_SCENEMGR_SELF, which no scene index can be.
 */
u32 WeUi_Gfx_SceneMgr_GetCurScene(void);

#endif /* __WEUI_GFX_SCENEMGR_H */
