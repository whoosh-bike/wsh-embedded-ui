#include "weui_demo.h"

#if WEUI_USE_COMPONENT__DEMO

#include "weui_demo_pace.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_base.h"
#include "weui_gfx_colors.h"
#include "weui_gfx_fps_control.h"
#include "weui_gfx_fractal_tree.h"
#include "weui_gfx_life.h"
#include "weui_rand.h"

#define DEMO_TREE_COUNT		   9
#define DEMO_GUEST_COUNT	   5
#define DEMO_TREE_GAP_SHARE	   0.2f /* least distance between two roots, of the panel width */
#define DEMO_TREE_MARGIN_SHARE 8	/* roots keep 1/8 of the width off either edge */
#define DEMO_TREE_PLANT_FRAMES (WEUI_DEMO_LIVE_FRAMES / 2) /* every tree is up by then */
#define DEMO_TREE_LEVEL_FRAMES 3
#define DEMO_TREE_DEPTH_MIN	   7 /* the whole garden is redrawn every frame */
#define DEMO_TREE_DEPTH_MAX	   9
#define DEMO_TREE_SPREAD_MIN   20
#define DEMO_TREE_SPREAD_MAX   40
#define DEMO_TREE_RIGHT		 90.0f /* a lopsided tree turns its other branch by what's left of it */
#define DEMO_TREE_SHRINK	 0.7f
#define DEMO_TREE_BREATH	 8.0f /* spread swing, degrees */
#define DEMO_TREE_SWAY		 8.0f
#define DEMO_TREE_SWAY_TURNS 2.0f
#define DEMO_TREE_HUE_TIPS	 90.0f

/* The passing tree, the old FractalTree scene: grown a level a frame, held, then the next one */
#define DEMO_GUEST_DEPTH_MIN   10
#define DEMO_GUEST_DEPTH_MAX   12
#define DEMO_GUEST_SPREAD_MIN  2 /* spread = steps * STEP + OFFSET, so 25..75 degrees */
#define DEMO_GUEST_SPREAD_MAX  7
#define DEMO_GUEST_SPREAD_STEP 10
#define DEMO_GUEST_SPREAD_OFS  5
#define DEMO_GUEST_HOLD_FRAMES 6

typedef struct {
	WeUi_Gfx_FractalTreeCfg_t Cfg;
	float SpreadCcw;
	float SpreadCw;
	float Phase;
	u32 DepthMax;
	u16 PlantFrame;
} WeUi_DemoTree_t;

typedef struct {
	WeUi_Gfx_FractalTreeCfg_t Cfg;
	u32 DepthMax;
	u16 BornFrame;
} WeUi_DemoGuest_t;

typedef struct {
	s32 Min;
	s32 Span;
	s32 Gap;
} WeUi_DemoTree_Rows_t;

/** @brief The gap shrinks below its share when that many trees would not fit. */
static WeUi_DemoTree_Rows_t WeUi_DemoTree_GetRows(u8 trees) {
	s32 width  = WeUi_Gfx_Adapter_GetWidth();
	s32 margin = width / DEMO_TREE_MARGIN_SHARE;
	s32 span   = width - 2 * margin;
	s32 gap	   = (s32)(DEMO_TREE_GAP_SHARE * width);
	if (trees > 1)
		gap = GET_MIN(gap, span / (trees - 1));

	return (WeUi_DemoTree_Rows_t){.Min = margin, .Span = span, .Gap = gap};
}

static float WeUi_DemoTree_RandLength(void) {
	s32 height = WeUi_Gfx_Adapter_GetHeight();
	return (float)WeUi_Rand_Between(height / 4, height * 2 / 5);
}

/** @brief Roots a gap apart at least: the slack the gaps leave is shared out at random. */
static void WeUi_DemoTree_PlaceRoots(float* pRootX, u8 trees) {
	WeUi_DemoTree_Rows_t rows = WeUi_DemoTree_GetRows(trees);
	s32 slack				  = GET_MAX(0, rows.Span - (trees - 1) * rows.Gap);

	s32 cut[WEUI_DEMO_TREE_MAX];
	for (u16 i = 0; i < trees; i++) {
		s32 c = WeUi_Rand_Between(0, slack);
		u16 j = i;
		for (; j > 0 && cut[j - 1] > c; j--)
			cut[j] = cut[j - 1];
		cut[j] = c;
	}

	for (u16 i = 0; i < trees; i++)
		pRootX[i] = (float)(rows.Min + i * rows.Gap + cut[i]);
}

static void WeUi_DemoTree_Plant(WeUi_DemoTree_t* pTree, float rootX, u16 plantFrame) {
	float hue = (float)WeUi_Rand_Hue();
	float ccw = (float)WeUi_Rand_Between(DEMO_TREE_SPREAD_MIN, DEMO_TREE_SPREAD_MAX);

	pTree->SpreadCcw  = ccw;
	pTree->SpreadCw	  = WeUi_Rand_Bool() ? ccw : DEMO_TREE_RIGHT - ccw;
	pTree->Phase	  = D2R((float)WeUi_Rand_Between(0, 359));
	pTree->DepthMax	  = (u32)WeUi_Rand_Between(DEMO_TREE_DEPTH_MIN, DEMO_TREE_DEPTH_MAX);
	pTree->PlantFrame = plantFrame;
	pTree->Cfg		  = (WeUi_Gfx_FractalTreeCfg_t){
		.RootX	= rootX,
		.RootY	= (float)WeUi_Gfx_Adapter_GetY_Max(),
		.Length = WeUi_DemoTree_RandLength(),
		.Angle	= 90.0f,
		.Shrink = DEMO_TREE_SHRINK,
		.Trunk	= WeUi_Color_GetFullHsb(hue),
		.Tips	= WeUi_Color_GetFullHsb(WeUi_Color_HueIncDec(hue, DEMO_TREE_HUE_TIPS)),
	};
	pTree->Cfg.Trunk.Bri = WEUI_COLOR_HSB_MAX_BRI / 3;
}

static void WeUi_DemoTree_Grow(WeUi_DemoTree_t* pTree, u16 frame, float wind) {
	float phase	 = wind + pTree->Phase;
	float breath = DEMO_TREE_BREATH * sinf(2.0f * phase);
	u32 grown	 = 1 + (frame - pTree->PlantFrame) / DEMO_TREE_LEVEL_FRAMES;

	pTree->Cfg.Depth	 = GET_MIN(grown, pTree->DepthMax);
	pTree->Cfg.SpreadCcw = pTree->SpreadCcw + breath;
	pTree->Cfg.SpreadCw	 = pTree->SpreadCw + breath;
	pTree->Cfg.Sway		 = DEMO_TREE_SWAY * sinf(phase);
	WeUi_Gfx_FractalTree_Draw(&pTree->Cfg);
}

/** @brief A new passing tree, a gap off where the last one stood. */
static void WeUi_DemoGuest_Spawn(WeUi_DemoGuest_t* pGuest, u16 frame) {
	WeUi_DemoTree_Rows_t rows = WeUi_DemoTree_GetRows(1);
	s32 lastX				  = (s32)pGuest->Cfg.RootX - rows.Min;
	s32 rootX =
		rows.Min + (lastX + WeUi_Rand_Between(rows.Gap, rows.Span - rows.Gap)) % (rows.Span + 1);
	float spread = (float)(WeUi_Rand_Between(DEMO_GUEST_SPREAD_MIN, DEMO_GUEST_SPREAD_MAX) *
							   DEMO_GUEST_SPREAD_STEP +
						   DEMO_GUEST_SPREAD_OFS);
	WeUi_Gfx_ColorSchemeHsb_t hsb = WeUi_Color_GetFullHsb(WeUi_Rand_Hue());

	pGuest->DepthMax  = (u32)WeUi_Rand_Between(DEMO_GUEST_DEPTH_MIN, DEMO_GUEST_DEPTH_MAX);
	pGuest->BornFrame = frame;
	pGuest->Cfg		  = (WeUi_Gfx_FractalTreeCfg_t){
		.RootX	   = (float)rootX,
		.RootY	   = (float)WeUi_Gfx_Adapter_GetY_Max(),
		.Length	   = WeUi_DemoTree_RandLength(),
		.Angle	   = 90.0f,
		.SpreadCcw = spread,
		.SpreadCw  = DEMO_TREE_RIGHT - spread,
		.Shrink	   = DEMO_TREE_SHRINK,
		.Trunk	   = hsb,
		.Tips	   = hsb,
	};
}

/** @brief The trunk brightens as the tree grows, the tips stay the dimmest. */
static void WeUi_DemoGuest_Grow(WeUi_DemoGuest_t* pGuest, u16 frame) {
	if (frame < pGuest->BornFrame)
		return;

	u32 age = frame - pGuest->BornFrame;
	if (age >= pGuest->DepthMax + DEMO_GUEST_HOLD_FRAMES) {
		WeUi_DemoGuest_Spawn(pGuest, frame);
		age = 0;
	}

	float levelBri		  = WEUI_COLOR_HSB_MAX_BRI / (float)pGuest->DepthMax;
	pGuest->Cfg.Depth	  = GET_MIN(age + 1, pGuest->DepthMax);
	pGuest->Cfg.Trunk.Bri = (float)pGuest->Cfg.Depth * levelBri;
	pGuest->Cfg.Tips.Bri  = levelBri;
	WeUi_Gfx_FractalTree_Draw(&pGuest->Cfg);
}

/**
 * @brief FractalTree_Draw a frame at a time: a garden of random trees, symmetric
 *        and lopsided, a fifth of the width apart at least, planted in random order
 *        and grown a level at a time, each swaying in the wind at its own phase.
 *        Behind them lopsided trees come and go, each somewhere new.
 */
void WeUi_DrawDemo_FractalGarden(u8 trees, u8 guests) {
	trees  = GET_MIN(trees, WEUI_DEMO_TREE_MAX);
	guests = GET_MIN(guests, WEUI_DEMO_GUEST_MAX);

	float rootX[WEUI_DEMO_TREE_MAX];
	WeUi_DemoTree_PlaceRoots(rootX, trees);

	u16 order[WEUI_DEMO_TREE_MAX];
	for (u16 i = 0; i < trees; i++) {
		u16 j	 = (u16)WeUi_Rand_Between(0, i);
		order[i] = (j == i) ? i : order[j];
		order[j] = i;
	}

	WeUi_DemoTree_t garden[WEUI_DEMO_TREE_MAX];
	for (u16 i = 0; i < trees; i++)
		WeUi_DemoTree_Plant(&garden[i], rootX[i], order[i] * DEMO_TREE_PLANT_FRAMES / trees);

	/* Staggered over one lifetime, so the passing trees don't grow in step */
	WeUi_DemoGuest_t guest[WEUI_DEMO_GUEST_MAX];
	for (u16 i = 0; i < guests; i++) {
		guest[i] = (WeUi_DemoGuest_t){.Cfg.RootX = (float)WeUi_DemoTree_GetRows(1).Min};
		WeUi_DemoGuest_Spawn(&guest[i],
							 i * (DEMO_GUEST_DEPTH_MIN + DEMO_GUEST_HOLD_FRAMES) / guests);
	}

	for (u16 frame = 0; frame < WEUI_DEMO_LIVE_FRAMES; frame++) {
		float wind = DEMO_TREE_SWAY_TURNS * 2.0f * WEUI_PI_F * frame / WEUI_DEMO_LIVE_FRAMES;

		WeUi_Gfx_Adapter_ClearDrawingFrame();
		for (u16 i = 0; i < guests; i++)
			WeUi_DemoGuest_Grow(&guest[i], frame);
		for (u16 i = 0; i < trees; i++) {
			if (frame >= garden[i].PlantFrame)
				WeUi_DemoTree_Grow(&garden[i], frame, wind);
		}
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(WEUI_DEMO_LIVE_FPS);
	}
}

void WeUi_DrawDemo_FractalTree(void) {
	WeUi_DrawDemo_FractalGarden(DEMO_TREE_COUNT, DEMO_GUEST_COUNT);
}

#define DEMO_LIFE_SEED_SHARE   0.5f /* side of the random seed, of the shorter panel side */
#define DEMO_LIFE_SPRINKLE	   8	/* frames between the small patches that keep it going */
#define DEMO_LIFE_SPRINKLE_MIN 2
#define DEMO_LIFE_SPRINKLE_MAX 4
#define DEMO_LIFE_TRAIL_KEEP   0.6f
#define DEMO_LIFE_HUE_STABLE   120.0f

static const WeUi_Gfx_Point_t DemoLife_Glider[] = {
	{.X = 1, .Y = 0}, {.X = 2, .Y = 1}, {.X = 0, .Y = 2}, {.X = 1, .Y = 2}, {.X = 2, .Y = 2},
};

static void WeUi_DemoLife_Sprinkle(void) {
	s32 size = WeUi_Rand_Between(DEMO_LIFE_SPRINKLE_MIN, DEMO_LIFE_SPRINKLE_MAX);

	WeUi_Gfx_Life_SeedRandom(WeUi_Rand_Between(0, WeUi_Gfx_Adapter_GetWidth() - size),
							 WeUi_Rand_Between(0, WeUi_Gfx_Adapter_GetHeight() - size),
							 (WeUi_Gfx_ObjSize_t)size);
}

/**
 * @brief The sim/life field stepped once a frame, edges hard: a glider that
 *        breaks up against the border and a random patch, with small patches
 *        sprinkled in to keep it going. Dead cells fade as a trail.
 */
void WeUi_DrawDemo_Life(void) {
	WeUi_Gfx_ObjSize_t side =
		(WeUi_Gfx_ObjSize_t)(DEMO_LIFE_SEED_SHARE *
							 GET_MIN(WeUi_Gfx_Adapter_GetWidth(), WeUi_Gfx_Adapter_GetHeight()));
	WeUi_Gfx_ColorSchemeHsb_t aliveHsb	= WeUi_Color_GetFullHsb(WeUi_Rand_Hue());
	WeUi_Gfx_ColorSchemeHsb_t stableHsb = aliveHsb;
	stableHsb.Hue						= WeUi_Color_HueIncDec(aliveHsb.Hue, DEMO_LIFE_HUE_STABLE);
	stableHsb.Bri						= WEUI_COLOR_HSB_MAX_BRI / 3;

	WeUi_Gfx_Life_Init(false);
	for (u32 i = 0; i < NUM_ELEMENTS(DemoLife_Glider); i++)
		WeUi_Gfx_Life_SetCell(DemoLife_Glider[i].X, DemoLife_Glider[i].Y, true);
	WeUi_Gfx_Life_SeedRandom(WeUi_Gfx_Adapter_GetX_Mid(), WeUi_Gfx_Adapter_GetY_Mid(), side);

	WeUi_Gfx_Adapter_ClearAllFrames();
	for (u16 frame = 0; frame < WEUI_DEMO_LIVE_FRAMES; frame++) {
		if (frame > 0 && frame % DEMO_LIFE_SPRINKLE == 0)
			WeUi_DemoLife_Sprinkle();
		WeUi_Gfx_Life_Step();

		WeUi_Gfx_Adapter_CopyFrame_DisplayToDrawing();
		WeUi_Gfx_Frame_Fade(DEMO_LIFE_TRAIL_KEEP);
		WeUi_Gfx_Point_t refPt = {.X = 0, .Y = 0};
		WeUi_Gfx_Life_Draw(&refPt, WeUi_Gfx_Color_FromHsb(&aliveHsb),
						   WeUi_Gfx_Color_FromHsb(&stableHsb));
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(WEUI_DEMO_LIVE_FPS);
	}
}

#endif /* WEUI_USE_COMPONENT__DEMO */
