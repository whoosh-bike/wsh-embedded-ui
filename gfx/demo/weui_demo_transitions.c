#include "weui_demo.h"

#if WEUI_USE_COMPONENT__DEMO

#include "weui_gfx_adapter.h"
#include "weui_gfx_base.h"
#include "weui_gfx_colors.h"
#include "weui_gfx_effects.h"
#include "weui_gfx_fps_control.h"
#include "weui_gfx_text.h"

/* A second each, and a second held after, so each one can be judged on its own and
 * its name read before the next: the demo runs as long as the step list below is */
#define DEMO_TRANSITION_FPS			   30
#define DEMO_TRANSITION_MS			   1000
#define DEMO_TRANSITION_HOLD_MS		   1000
#define DEMO_TRANSITION_RAIN_COLUMN_PX 1 /* a drizzle */
#define DEMO_TRANSITION_MELT_COLUMN_PX 6 /* chunks, as Doom's screen melt */

#define DEMO_TRANSITION_FRAMES WeUi_Gfx_Effect_Frames(DEMO_TRANSITION_MS, DEMO_TRANSITION_FPS)

/* A bouncy throw, to show what the cfg buys over the defaults: flung well past the
 * edge and left to swing home several times */
static const WeUi_Gfx_Effect_PushCfg_t DemoPushCfg_Bounce = {
	.OvershootShare = 0.25f,
	.OvershootMinPx = 4.0f,
	.OvershootMaxPx = 24.0f,
	.ApproachShare	= 0.3f,
	.BounceKeep		= 0.5f,
	.Bounces		= 3,
};

#define DEMO_TRANSITION_HUE_SPAN 90.0f	/* across the panel */
#define DEMO_TRANSITION_HUE_STEP 137.5f /* golden angle: no two pictures in a row alike */
#define DEMO_TRANSITION_HUE_FLOW 4.0f	/* degrees per frame, so the picture is alive */
#define DEMO_TRANSITION_BRI		 60.0f	/* under the label, which stays readable on it */

/* The transitions call back without arguments, so what to draw next is parked here */
static float DemoTransition_Hue;
static const char* DemoTransition_Label = "";

/** @brief A fresh picture for the next transition, named after it */
static void WeUi_DemoTransition_Next(const char* pcLabel) {
	DemoTransition_Hue	 = WeUi_Color_HueIncDec(DemoTransition_Hue, DEMO_TRANSITION_HUE_STEP);
	DemoTransition_Label = pcLabel;
}

/* Stands in for a scene: clears, draws one animated frame, holds the frame rate,
 * and leaves pushing it to the caller. A flowing gradient, so a slide shows its
 * travel, and the name of the transition playing, so it is clear which is which */
static void WeUi_DemoTransition_RenderIncoming(void) {
	WeUi_Gfx_ObjSize_t width = WeUi_Gfx_Adapter_GetWidth();
	float flow				 = (float)WeUi_Gfx_FpsCtrl_GetRenderCycle() * DEMO_TRANSITION_HUE_FLOW;

	for (WeUi_Gfx_PtPos_t x = 0; x < (WeUi_Gfx_PtPos_t)width; x++) {
		WeUi_Gfx_ColorSchemeHsb_t hsb = {
			.Hue = WeUi_Color_HueIncDec(DemoTransition_Hue,
										DEMO_TRANSITION_HUE_SPAN * x / width + flow),
			.Sat = WEUI_COLOR_HSB_MAX_SAT,
			.Bri = DEMO_TRANSITION_BRI,
		};
		WeUi_Gfx_ColorPoint_t cpt = {
			.Pt.X  = x,
			.Pt.Y  = 0,
			.Color = WeUi_Gfx_Color_FromHsb(&hsb),
		};
		WeUi_Gfx_LineHV_Draw(&cpt, WeUi_Gfx_Adapter_GetHeight(), WEUI_LINE_DIR_VERTICAL);
	}

	WeUi_Gfx_Text_t text;
	WeUi_Gfx_Text_InitStruct(&text);
	text.Str	= (char*)DemoTransition_Label; /* only read, the field just is not const */
	text.Font	= u8g2_font_9x15B_tf;
	text.BgMode = WEUI_TEXT_BG_MODE_TRANSPARENT;
	memset(&text.TextColor, WEUI_COLOR_CHANNEL_MAX, sizeof(WeUi_Gfx_Color_t));

	WeUi_Gfx_Point_t pt = {
		.X = WeUi_Gfx_Adapter_GetX_Mid() - (WeUi_Gfx_PtPos_t)(WeUi_Gfx_Text_GetPixLen(&text) / 2),
		.Y = WeUi_Gfx_Adapter_GetY_Mid() + (WeUi_Gfx_PtPos_t)(WeUi_Gfx_Text_GetHeight(&text) / 2),
	};
	WeUi_Gfx_Text_Draw(&pt, &text);

	WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_TRANSITION_FPS);
}

/** @brief Keep what a transition brought in on the panel, so its name can be read */
static void WeUi_DemoTransition_Hold(void) {
	u16 frames = WeUi_Gfx_Effect_Frames(DEMO_TRANSITION_HOLD_MS, DEMO_TRANSITION_FPS);

	for (u16 frame = 0; frame < frames; frame++)
		WeUi_DemoTransition_RenderIncoming();
}

/* One per step below, each playing just the transition */
static void WeUi_DemoTransition_Dissolve(void) {
	WeUi_Gfx_Effect_Dissolve(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES);
}

static void WeUi_DemoTransition_Dim(void) {
	WeUi_Gfx_Effect_Dim(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES);
}

static void WeUi_DemoTransition_Fade(void) {
	WeUi_Gfx_Effect_Fade(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES);
}

static void WeUi_DemoTransition_Flash(void) {
	WeUi_Gfx_Effect_Flash(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES);
}

static void WeUi_DemoTransition_Zoom(void) {
	WeUi_Gfx_Effect_Zoom(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES);
}

static void WeUi_DemoTransition_Rotate(void) {
	WeUi_Gfx_Effect_Rotate(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES);
}

static void WeUi_DemoTransition_Pixelate(void) {
	WeUi_Gfx_Effect_Pixelate(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES);
}

static void WeUi_DemoTransition_Blocks(void) {
	WeUi_Gfx_Effect_Blocks(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES);
}

static void WeUi_DemoTransition_Curtain(void) {
	WeUi_Gfx_Effect_Curtain(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES,
							WEUI_GFX_EFFECT_AXIS_HORIZONTAL);
}

static void WeUi_DemoTransition_CurtainVertical(void) {
	WeUi_Gfx_Effect_Curtain(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES,
							WEUI_GFX_EFFECT_AXIS_VERTICAL);
}

static void WeUi_DemoTransition_Ripple(void) {
	WeUi_Gfx_Effect_Ripple(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES,
						   WEUI_GFX_EFFECT_RIPPLE_CIRCLE);
}

static void WeUi_DemoTransition_RippleDiamond(void) {
	WeUi_Gfx_Effect_Ripple(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES,
						   WEUI_GFX_EFFECT_RIPPLE_DIAMOND);
}

static void WeUi_DemoTransition_PushRight(void) {
	WeUi_Gfx_Effect_Push(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES,
						 WEUI_GFX_EFFECT_SLIDE_RIGHT, NULL);
}

static void WeUi_DemoTransition_PushDown(void) {
	WeUi_Gfx_Effect_Push(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES,
						 WEUI_GFX_EFFECT_SLIDE_DOWN, NULL);
}

static void WeUi_DemoTransition_Bounce(void) {
	WeUi_Gfx_Effect_Push(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES,
						 WEUI_GFX_EFFECT_SLIDE_RIGHT, &DemoPushCfg_Bounce);
}

static void WeUi_DemoTransition_Swipe(void) {
	WeUi_Gfx_Effect_Swipe(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES,
						  WEUI_GFX_EFFECT_SLIDE_RIGHT);
}

static void WeUi_DemoTransition_Cover(void) {
	WeUi_Gfx_Effect_Cover(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES,
						  WEUI_GFX_EFFECT_SLIDE_LEFT, WEUI_GFX_EFFECT_COVER_IN);
}

static void WeUi_DemoTransition_Blinds(void) {
	WeUi_Gfx_Effect_Blinds(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES);
}

static void WeUi_DemoTransition_Interlace(void) {
	WeUi_Gfx_Effect_Interlace(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES);
}

static void WeUi_DemoTransition_Reload(void) {
	WeUi_Gfx_Effect_Reload(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES);
}

static void WeUi_DemoTransition_Wave(void) {
	WeUi_Gfx_Effect_Wave(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES);
}

static void WeUi_DemoTransition_Rain(void) {
	WeUi_Gfx_Effect_Rain(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES,
						 DEMO_TRANSITION_RAIN_COLUMN_PX);
}

static void WeUi_DemoTransition_Melt(void) {
	WeUi_Gfx_Effect_Melt(WeUi_DemoTransition_RenderIncoming, DEMO_TRANSITION_FRAMES,
						 DEMO_TRANSITION_MELT_COLUMN_PX);
}

typedef struct {
	const char* Label; /* drawn on the picture the transition brings in */
	VoidFunc_t Play;
} WeUi_DemoTransition_Step_t;

/* Played in this order. One a line, so commenting one out leaves it out */
static const WeUi_DemoTransition_Step_t DemoTransition_Steps[] = {
	{"dissolve", WeUi_DemoTransition_Dissolve},
	{"dim", WeUi_DemoTransition_Dim},
	{"fade", WeUi_DemoTransition_Fade},
	{"flash", WeUi_DemoTransition_Flash},
	{"zoom", WeUi_DemoTransition_Zoom},
	{"rotate", WeUi_DemoTransition_Rotate},
	{"pixelate", WeUi_DemoTransition_Pixelate},
	{"blocks", WeUi_DemoTransition_Blocks},
	{"curtain", WeUi_DemoTransition_Curtain},
	{"curtain v", WeUi_DemoTransition_CurtainVertical},
	{"ripple", WeUi_DemoTransition_Ripple},
	{"diamond", WeUi_DemoTransition_RippleDiamond},
	{"push >", WeUi_DemoTransition_PushRight},
	{"push v", WeUi_DemoTransition_PushDown},
	{"bounce", WeUi_DemoTransition_Bounce},
	{"swipe", WeUi_DemoTransition_Swipe},
	{"cover", WeUi_DemoTransition_Cover},
	{"blinds", WeUi_DemoTransition_Blinds},
	{"interlace", WeUi_DemoTransition_Interlace},
	{"reload", WeUi_DemoTransition_Reload},
	{"wave", WeUi_DemoTransition_Wave},
	{"rain", WeUi_DemoTransition_Rain},
	{"melt", WeUi_DemoTransition_Melt},
};

void WeUi_DrawDemo_Transitions(void) {
	/* Give the first transition something to push out */
	WeUi_DemoTransition_Next("ready");
	WeUi_DemoTransition_RenderIncoming();
	WeUi_Gfx_Adapter_DrawFrame();

	for (u32 idx = 0; idx < NUM_ELEMENTS(DemoTransition_Steps); idx++) {
		WeUi_DemoTransition_Next(DemoTransition_Steps[idx].Label);
		DemoTransition_Steps[idx].Play();
		WeUi_DemoTransition_Hold();
	}
}

#endif /* WEUI_USE_COMPONENT__DEMO */
