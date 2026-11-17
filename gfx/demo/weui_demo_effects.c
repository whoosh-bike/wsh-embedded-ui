#include "weui_demo.h"

#if WEUI_USE_COMPONENT__DEMO

#include "weui_demo_pace.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_base.h"
#include "weui_gfx_colors.h"
#include "weui_gfx_effects.h"
#include "weui_gfx_fps_control.h"
#include "weui_rand.h"

/* The effects demo is three uses of the effect API in a third of the budget each */
#define DEMO_EFFECTS_PARTS	3
#define DEMO_SOLO_FRAMES	(WEUI_DEMO_SEC * WEUI_GFX_FPS_CTRL_DEF_RENDER_FREQ / DEMO_EFFECTS_PARTS)
#define DEMO_PAINTER_FPS	30
#define DEMO_PAINTER_FRAMES (WEUI_DEMO_SEC * DEMO_PAINTER_FPS / DEMO_EFFECTS_PARTS)

/* Warm, and banded across rather than rippling, to show the cfg reaching the same
 * painter the animation drives */
static const WeUi_Gfx_Effect_PlasmaCfg_t DemoPlasmaCfg_Ember = {
	.WaveX	   = 0.15f,
	.WaveY	   = 1.10f,
	.WaveDiag  = 0.30f,
	.DriftDeg  = 540.0f,
	.DriftY	   = -1.30f,
	.DriftDiag = 0.70f,
	.Bloom	   = 0.30f,
	.HueBase   = 10.0f,
	.HueSpan   = 45.0f,
	.HueSpin   = 15.0f,
	.BriFloor  = 0.20f,
};

/** @brief The ready-made animation: the library owns the loop, blooming from the dark */
static void WeUi_DemoEffects_Solo(void) {
	WeUi_Gfx_Effect_Plasma_Play(DEMO_SOLO_FRAMES, NULL);
}

/**
 * @brief The same plasma as an ordinary scene: the painter draws one frame, the
 *        caller owns the loop and the pacing, and the field cycles rather than
 *        arriving once. Nothing here goes through the effect runner.
 */
static void WeUi_DemoEffects_Painter(void) {
	for (u16 frame = 1; frame <= DEMO_PAINTER_FRAMES; frame++) {
		WeUi_Gfx_Effect_Plasma_Paint((float)frame / (float)DEMO_PAINTER_FRAMES,
									 &DemoPlasmaCfg_Ember);
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_PAINTER_FPS);
	}
}

#define DEMO_FIELD_ORIGIN	   50.0f /* keeps the sampled coordinates positive */
#define DEMO_FIELD_TIME_STEP   0.02f
#define DEMO_FIELD_BREATH_MID  0.3f
#define DEMO_FIELD_BREATH_SPAN 0.4f
#define DEMO_FIELD_BREATHS	   1.0f
#define DEMO_FIELD_WIND		   0.06f /* noise units per frame */
#define DEMO_FIELD_FRAMES	   (WEUI_DEMO_SEC * WEUI_DEMO_LIVE_FPS / DEMO_EFFECTS_PARTS)

/* Teal turning to blue, broader clouds than the default violet */
static const WeUi_Gfx_Effect_PerlinCfg_t DemoPerlinCfg_Reef = {
	.ScaleX	   = 0.12f,
	.ScaleY	   = 0.25f,
	.Drift	   = 3.0f,
	.Gain	   = 1.40f,
	.RevealLow = 0.25f,
	.EdgeSoft  = 0.35f,
	.HueBase   = 110.0f,
	.HueSpan   = 80.0f,
	.HueSpin   = 0.0f,
};
#define DEMO_FIELD_HUE_STEP 1.0f /* degrees per frame */

/**
 * @brief Perlin_PaintField with each part of the field moved on its own: the
 *        threshold breathes, the field drifts on a wind and the hue turns, at
 *        rates no single progress value could tie together.
 */
static void WeUi_DemoEffects_Field(void) {
	WeUi_Gfx_Effect_PerlinField_t field = {
		.Time	 = 0.0f,
		.OffsetX = DEMO_FIELD_ORIGIN,
		.OffsetY = DEMO_FIELD_ORIGIN,
	};

	for (u16 frame = 0; frame < DEMO_FIELD_FRAMES; frame++) {
		float phase = DEMO_FIELD_BREATHS * 2.0f * WEUI_PI_F * frame / DEMO_FIELD_FRAMES;

		field.Time += DEMO_FIELD_TIME_STEP;
		field.Threshold = DEMO_FIELD_BREATH_MID + DEMO_FIELD_BREATH_SPAN * sinf(phase);
		field.OffsetX += DEMO_FIELD_WIND;
		field.HueShift = WeUi_Color_HueIncDec(field.HueShift, DEMO_FIELD_HUE_STEP);

		WeUi_Gfx_Effect_Perlin_PaintField(&field, &DemoPerlinCfg_Reef);
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(WEUI_DEMO_LIVE_FPS);
	}
}

/**
 * @brief The effect API at its three levels, each in its own look: a ready-made
 *        animation, a painter driven frame by frame as a scene would, and a field
 *        whose every part is moved on its own.
 */
void WeUi_DrawDemo_Effects(void) {
	WeUi_DemoEffects_Solo();
	WeUi_DemoEffects_Painter();
	WeUi_DemoEffects_Field();
}

/** @brief Static_Paint driven as a scene would drive it: a fresh frame each tick */
void WeUi_DrawDemo_Static(void) {
	for (u16 frame = 0; frame < WEUI_DEMO_LIVE_FRAMES / 2; frame++) {
		WeUi_Gfx_Effect_Static_Paint();
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(WEUI_DEMO_LIVE_FPS);
	}
}

#define DEMO_FULL_TURN	  360.0f
#define DEMO_TRAIL_KEEP	  0.8f
#define DEMO_TRAIL_TURNS  3.0f
#define DEMO_TRAIL_RADIUS 0.4f /* of the shorter panel side */

/**
 * @brief Frame_Fade on its own: a dot circles and leaves a fading tail. Each
 *        frame starts from the one on the panel, as a KeepFrame scene does.
 */
void WeUi_DrawDemo_Trails(void) {
	float radius = DEMO_TRAIL_RADIUS *
				   (float)GET_MIN(WeUi_Gfx_Adapter_GetWidth(), WeUi_Gfx_Adapter_GetHeight());
	WeUi_Gfx_ColorSchemeHsb_t hsb = WeUi_Color_GetFullHsb(WeUi_Rand_Hue());

	WeUi_Gfx_Adapter_ClearAllFrames();
	for (u16 frame = 0; frame < WEUI_DEMO_LIVE_FRAMES; frame++) {
		float angle = DEMO_TRAIL_TURNS * DEMO_FULL_TURN * frame / WEUI_DEMO_LIVE_FRAMES;

		WeUi_Gfx_Adapter_CopyFrame_DisplayToDrawing();
		WeUi_Gfx_Frame_Fade(DEMO_TRAIL_KEEP);

		hsb.Hue					  = WeUi_Color_HueIncDec(hsb.Hue, 1.0f);
		WeUi_Gfx_ColorPoint_t cpt = {
			.Pt.X =
				(WeUi_Gfx_PtPos_t)roundf(WeUi_Gfx_Adapter_GetX_Mid() + radius * cosf(D2R(angle))),
			.Pt.Y =
				(WeUi_Gfx_PtPos_t)roundf(WeUi_Gfx_Adapter_GetY_Mid() - radius * sinf(D2R(angle))),
			.Color = WeUi_Gfx_Color_FromHsb(&hsb),
		};
		WeUi_Gfx_Point_Draw(&cpt);
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(WEUI_DEMO_LIVE_FPS);
	}
}

#endif /* WEUI_USE_COMPONENT__DEMO */
