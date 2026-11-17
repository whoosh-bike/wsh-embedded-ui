#include "weui_demo.h"

#if WEUI_USE_COMPONENT__DEMO

#include "weui_gfx_adapter.h"
#include "weui_gfx_effects.h"
#include "weui_gfx_fps_control.h"
#include "weui_gfx_widget_qr.h"
#include "weui_rand.h"

/* The round, in order: comment a line out to leave that demo out of it */
/* clang-format off */
static const WeUi_Demo_t WeUi_Demos[] = {
	{"static",          WeUi_DrawDemo_Static,          false},
	{"primitives",      WeUi_DrawDemo_Primitives,      false},
	{"cube rotation",   WeUi_DrawDemo_CubeRotation,    false},
	{"affine",          WeUi_DrawDemo_Affine,          false},
	{"paths",           WeUi_DrawDemo_Paths,           false},
	{"fills",           WeUi_DrawDemo_Fills,           false},
	{"gradients",       WeUi_DrawDemo_Gradients,       false},
	{"strokes",         WeUi_DrawDemo_Strokes,         false},
	{"eyes",            WeUi_DrawDemo_Eyes,            false},
	{"scene",           WeUi_DrawDemo_Scene,           false},
	{"fonts",          WeUi_DrawDemo_Fonts,           false},
	{"font transforms", WeUi_DrawDemo_FontTransforms,  false},
	{"ramps",           WeUi_DrawDemo_Ramps,           true},
	{"icons",           WeUi_DrawDemo_Icons,           false},
	{"image",           WeUi_DrawDemo_Image,           false},
#if WEUI_USE_COMPONENT__WIDGET_QR
	{"qr",              WeUi_DrawDemo_Qr,              false},
#endif /* WEUI_USE_COMPONENT__WIDGET_QR */
	{"game of life",    WeUi_DrawDemo_Life,            false},
	{"fractal tree",    WeUi_DrawDemo_FractalTree,     false},
	{"transitions",     WeUi_DrawDemo_Transitions,     false},
	{"effects",         WeUi_DrawDemo_Effects,         false},
	{"sweep",           WeUi_DrawDemo_FieldSweep,      true},
	{"fire",            WeUi_DrawDemo_FieldFire,       true},
	{"ocean",           WeUi_DrawDemo_FieldOcean,      true},
	{"aurora",          WeUi_DrawDemo_FieldAurora,     true},
	{"nebula",          WeUi_DrawDemo_FieldNebula,     true},
	{"spiral",          WeUi_DrawDemo_FieldSpiral,     true},
	{"toxic",           WeUi_DrawDemo_FieldToxic,      true},
};
/* clang-format on */

/* Between two demos the last one is led out into black: a demo paces itself and cannot
 * draw a single frame on demand, so the next one has nothing to bring in. Only the
 * transitions that take no arguments of their own; Fade into black reads as Dim */
typedef RET_STATE_t (*WeUi_DemoRound_Outro_t)(WeUi_Gfx_Effect_RenderCb_t renderIncoming,
											  u16 frames);

static const WeUi_DemoRound_Outro_t WeUi_DemoRound_Outros[] = {
	WeUi_Gfx_Effect_Dissolve, WeUi_Gfx_Effect_Dim,	  WeUi_Gfx_Effect_Flash,
	WeUi_Gfx_Effect_Zoom,	  WeUi_Gfx_Effect_Rotate, WeUi_Gfx_Effect_Pixelate,
	WeUi_Gfx_Effect_Blocks,	  WeUi_Gfx_Effect_Blinds, WeUi_Gfx_Effect_Interlace,
	WeUi_Gfx_Effect_Reload,	  WeUi_Gfx_Effect_Wave,
};

#define DEMO_ROUND_OUTRO_FPS 30
#define DEMO_ROUND_OUTRO_MS	 1000

static void WeUi_DemoRound_RenderBlack(void) {
	WeUi_Gfx_Adapter_ClearDrawingFrame();
	WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_ROUND_OUTRO_FPS);
}

/** @brief A random way out, never the one just played: no fixed cycle to fall in step
 *         with the demos */
static void WeUi_DemoRound_PlayOutro(void) {
	static s32 last = -1;
	s32 count		= (s32)NUM_ELEMENTS(WeUi_DemoRound_Outros);
	bool first		= last < 0;

	/* One fewer to draw from, then the one just played stepped over */
	s32 pick = WeUi_Rand_Between(0, first ? count - 1 : count - 2);
	if (!first && pick >= last)
		pick++;

	last = pick;
	WeUi_DemoRound_Outros[pick](WeUi_DemoRound_RenderBlack,
								WeUi_Gfx_Effect_Frames(DEMO_ROUND_OUTRO_MS, DEMO_ROUND_OUTRO_FPS));
}

u32 WeUi_Demo_Count(void) {
	return NUM_ELEMENTS(WeUi_Demos);
}

const WeUi_Demo_t* WeUi_Demo_Get(u32 idx) {
	return (idx < NUM_ELEMENTS(WeUi_Demos)) ? &WeUi_Demos[idx] : NULL;
}

void WeUi_Demo_PlayRound(void) {
	for (u32 demo = 0; demo < NUM_ELEMENTS(WeUi_Demos); demo++) {
		WeUi_Demos[demo].Play();

		if (!WeUi_Demos[(demo + 1) % NUM_ELEMENTS(WeUi_Demos)].LiveIn)
			WeUi_DemoRound_PlayOutro();
	}
}

#endif /* WEUI_USE_COMPONENT__DEMO */
