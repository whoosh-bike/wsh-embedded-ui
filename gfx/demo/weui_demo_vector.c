#include "weui_demo.h"

#if WEUI_USE_COMPONENT__DEMO && WEUI_USE_COMPONENT__VECTOR

#include "log.h"
#include "mathlib_common.h"
#include "weui_demo_pace.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_affine.h"
#include "weui_gfx_base.h"
#include "weui_gfx_colors.h"
#include "weui_gfx_fill.h"
#include "weui_gfx_fps_control.h"
#include "weui_gfx_mask.h"
#include "weui_gfx_path.h"
#include "weui_gfx_scene.h"
#include "weui_gfx_stroke.h"
#include "weui_gfx_transform.h"

#define DEMO_VECTOR_ROUND_OFFSET 0.5f

static WeUi_Gfx_Point_t WeUi_DemoVector_ToPixel(WeUi_Gfx_PointF_t pt) {
	return (WeUi_Gfx_Point_t){(WeUi_Gfx_PtPos_t)floorf(pt.X + DEMO_VECTOR_ROUND_OFFSET),
							  (WeUi_Gfx_PtPos_t)floorf(pt.Y + DEMO_VECTOR_ROUND_OFFSET)};
}

static WeUi_Gfx_Color_t WeUi_DemoVector_Color(float hue, float bri) {
	WeUi_Gfx_ColorSchemeHsb_t hsb =
		WeUi_Color_GetFullHsb(WEUI_COLOR_CUT_HUE(hue, WEUI_COLOR_HSB_MAX_HUE));
	hsb.Bri = bri;
	return WeUi_Gfx_Color_FromHsb(&hsb);
}

static void WeUi_DemoVector_DrawEdge(WeUi_Gfx_PointF_t a, WeUi_Gfx_PointF_t b,
									 WeUi_Gfx_Color_t color) {
	WeUi_Gfx_Line_t line;
	WeUi_Gfx_Line_InitStruct(&line);
	line.Base.Color = color;
	line.Base.PtOne = WeUi_DemoVector_ToPixel(a);
	line.Base.PtTwo = WeUi_DemoVector_ToPixel(b);
	WeUi_Gfx_Line_Draw(&line);
}

/** @brief A demo of the file: `render` draws the frame at t in [0; 1) */
static void WeUi_DemoVector_Play(RET_STATE_t (*render)(float t)) {
	for (u32 frame = 0; frame < WEUI_DEMO_LIVE_FRAMES; frame++) {
		WeUi_Gfx_Adapter_ClearDrawingFrame();
		if (render((float)frame / WEUI_DEMO_LIVE_FRAMES) != RET_STATE_SUCCESS)
			return;
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(WEUI_DEMO_LIVE_FPS);
	}
}

// =================== Affine ===================

/* Sizes are shares of the panel height, orbit radii of the width and height */
#define DEMO_AFFINE_TURNS		1.5f /* planet orbits over the demo */
#define DEMO_AFFINE_BODY_HALF	0.32f
#define DEMO_AFFINE_BODY_SPIN	0.5f /* body turns per orbit */
#define DEMO_AFFINE_BODY_PULSE	0.25f
#define DEMO_AFFINE_PULSES		2.0f /* per orbit */
#define DEMO_AFFINE_PLANET_HALF 0.14f
#define DEMO_AFFINE_PLANET_SPIN -2.0f
#define DEMO_AFFINE_ORBIT_X		0.36f
#define DEMO_AFFINE_ORBIT_Y		0.22f
#define DEMO_AFFINE_MOON_HALF	0.06f
#define DEMO_AFFINE_MOON_ORBIT	0.26f
#define DEMO_AFFINE_MOON_TURNS	3.0f   /* per orbit */
#define DEMO_AFFINE_HUE_DRIFT	120.0f /* degrees over the demo */
#define DEMO_AFFINE_HUE_STEP	120.0f /* between body, planet and moon */
#define DEMO_AFFINE_CORNERS_NUM 4

/** @brief The outline of a square of half side `half` around the origin of pcM */
static void WeUi_DemoAffine_DrawSquare(const WeUi_Gfx_Affine_t* pcM, float half, float hue) {
	static const WeUi_Gfx_PointF_t corners[DEMO_AFFINE_CORNERS_NUM] = {
		{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}};
	WeUi_Gfx_PointF_t pts[DEMO_AFFINE_CORNERS_NUM];

	for (u32 i = 0; i < DEMO_AFFINE_CORNERS_NUM; i++) {
		WeUi_Gfx_PointF_t corner = {corners[i].X * half, corners[i].Y * half};
		pts[i]					 = WeUi_Gfx_Affine_Apply(pcM, corner);
	}

	WeUi_Gfx_Color_t color = WeUi_DemoVector_Color(hue, WEUI_COLOR_HSB_MAX_BRI);
	for (u32 i = 0; i < DEMO_AFFINE_CORNERS_NUM; i++)
		WeUi_DemoVector_DrawEdge(pts[i], pts[(i + 1) % DEMO_AFFINE_CORNERS_NUM], color);
}

/**
 * @brief One frame at `t` in [0; 1): each part is drawn in the coordinates its parent left on
 *        the stack - the body spun and pulsed inside a push, the planet carried along its
 *        orbit, its own spin pushed and popped, the moon circling where the planet is.
 */
static RET_STATE_t WeUi_DemoAffine_Render(float t) {
	float w		= (float)WeUi_Gfx_Adapter_GetWidth();
	float h		= (float)WeUi_Gfx_Adapter_GetHeight();
	float orbit = 2.0f * M_PIf * DEMO_AFFINE_TURNS * t;
	float hue	= DEMO_AFFINE_HUE_DRIFT * t;

	WeUi_Gfx_AffineStack_t stack;
	WeUi_Gfx_AffineStack_Reset(&stack);
	WeUi_Gfx_AffineStack_Translate(&stack, (float)WeUi_Gfx_Adapter_GetX_Mid(),
								   (float)WeUi_Gfx_Adapter_GetY_Mid());

	RETURN_IF_UNSUCCESS(WeUi_Gfx_AffineStack_Push(&stack));
	float pulse = 1.0f + DEMO_AFFINE_BODY_PULSE * sinf(orbit * DEMO_AFFINE_PULSES);
	WeUi_Gfx_AffineStack_Rotate(&stack, orbit * DEMO_AFFINE_BODY_SPIN);
	WeUi_Gfx_AffineStack_Scale(&stack, pulse, pulse);
	WeUi_DemoAffine_DrawSquare(&stack.Top, DEMO_AFFINE_BODY_HALF * h, hue);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_AffineStack_Pop(&stack));

	WeUi_Gfx_AffineStack_Translate(&stack, DEMO_AFFINE_ORBIT_X * w * cosf(orbit),
								   DEMO_AFFINE_ORBIT_Y * h * sinf(orbit));

	RETURN_IF_UNSUCCESS(WeUi_Gfx_AffineStack_Push(&stack));
	WeUi_Gfx_AffineStack_Rotate(&stack, orbit * DEMO_AFFINE_PLANET_SPIN);
	WeUi_DemoAffine_DrawSquare(&stack.Top, DEMO_AFFINE_PLANET_HALF * h, hue + DEMO_AFFINE_HUE_STEP);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_AffineStack_Pop(&stack));

	WeUi_Gfx_AffineStack_Rotate(&stack, orbit * DEMO_AFFINE_MOON_TURNS);
	WeUi_Gfx_AffineStack_Translate(&stack, DEMO_AFFINE_MOON_ORBIT * h, 0.0f);
	WeUi_DemoAffine_DrawSquare(&stack.Top, DEMO_AFFINE_MOON_HALF * h,
							   hue + 2.0f * DEMO_AFFINE_HUE_STEP);
	return RET_STATE_SUCCESS;
}

void WeUi_DrawDemo_Affine(void) {
	WeUi_DemoVector_Play(WeUi_DemoAffine_Render);
}

// =================== Paths ===================

/* Shapes are built around the origin with a radius of about 1 and scaled up on the stack */
#define DEMO_PATHS_HALF_MAX 0.4f /* share of the panel height */
#define DEMO_PATHS_HALF_SLOT \
	0.45f /* share of a slot's width, on a panel not much wider than high */
#define DEMO_PATHS_SCALE_MIN   0.3f	 /* of the largest size */
#define DEMO_PATHS_BREATHS	   2.0f	 /* grow-and-shrink cycles over the demo */
#define DEMO_PATHS_SWING	   0.45f /* rad either way: turned further, a shape is lost on 25 px */
#define DEMO_PATHS_SWINGS	   3.0f	 /* over the demo */
#define DEMO_PATHS_EDGE_BRI	   (WEUI_COLOR_HSB_MAX_BRI * 0.3f)
#define DEMO_PATHS_HUE_STEP	   75.0f
#define DEMO_PATHS_STAR_POINTS 5
#define DEMO_PATHS_STAR_INNER  0.45f

static RET_STATE_t WeUi_DemoPaths_Circle(WeUi_Gfx_Path_t* pPath) {
	return WeUi_Gfx_Path_Ellipse(pPath, 0.0f, 0.0f, 1.0f, 1.0f);
}

static RET_STATE_t WeUi_DemoPaths_Heart(WeUi_Gfx_Path_t* pPath) {
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_MoveTo(pPath, 0.0f, 0.9f));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_CubicTo(pPath, -0.6f, 0.4f, -1.0f, 0.0f, -1.0f, -0.4f));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_CubicTo(pPath, -1.0f, -1.0f, -0.2f, -1.1f, 0.0f, -0.5f));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_CubicTo(pPath, 0.2f, -1.1f, 1.0f, -1.0f, 1.0f, -0.4f));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_CubicTo(pPath, 1.0f, 0.0f, 0.6f, 0.4f, 0.0f, 0.9f));
	return WeUi_Gfx_Path_Close(pPath);
}

static RET_STATE_t WeUi_DemoPaths_Leaf(WeUi_Gfx_Path_t* pPath) {
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_MoveTo(pPath, 0.0f, -1.0f));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_QuadTo(pPath, 1.0f, 0.0f, 0.0f, 1.0f));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_QuadTo(pPath, -1.0f, 0.0f, 0.0f, -1.0f));
	return WeUi_Gfx_Path_Close(pPath);
}

static RET_STATE_t WeUi_DemoPaths_Star(WeUi_Gfx_Path_t* pPath) {
	u32 corners = 2 * DEMO_PATHS_STAR_POINTS;
	for (u32 i = 0; i < corners; i++) {
		float a = 2.0f * M_PIf * (float)i / (float)corners - M_PIf / 2.0f;
		float r = (i % 2) ? DEMO_PATHS_STAR_INNER : 1.0f;
		RETURN_IF_UNSUCCESS(i == 0 ? WeUi_Gfx_Path_MoveTo(pPath, r * cosf(a), r * sinf(a))
								   : WeUi_Gfx_Path_LineTo(pPath, r * cosf(a), r * sinf(a)));
	}
	return WeUi_Gfx_Path_Close(pPath);
}

typedef RET_STATE_t (*WeUi_DemoPaths_Shape_t)(WeUi_Gfx_Path_t* pPath);

static const WeUi_DemoPaths_Shape_t WeUi_DemoPaths_Shapes[] = {
	WeUi_DemoPaths_Circle,
	WeUi_DemoPaths_Heart,
	WeUi_DemoPaths_Leaf,
	WeUi_DemoPaths_Star,
};

static WeUi_Gfx_Path_t WEUI_DATA_PLACEMENT WeUi_DemoPaths_Path;

/** @brief The flattened outline dim, the points the curves were cut at bright */
static RET_STATE_t WeUi_DemoPaths_Draw(const WeUi_Gfx_Path_t* pcPath, float hue) {
	WeUi_Gfx_Color_t edge = WeUi_DemoVector_Color(hue, DEMO_PATHS_EDGE_BRI);

	for (u32 c = 0; c < pcPath->ContoursNum; c++) {
		const WeUi_Gfx_PathContour_t* pcC = &pcPath->Contours[c];
		const WeUi_Gfx_PointF_t* pcPts	  = &pcPath->Pts[pcC->Start];
		u32 edges						  = pcC->Closed ? pcC->Num : pcC->Num - 1;
		for (u32 i = 0; i < edges; i++)
			WeUi_DemoVector_DrawEdge(pcPts[i], pcPts[(i + 1) % pcC->Num], edge);
	}

	WeUi_Gfx_ColorPoint_t vertex = {.Color = WeUi_DemoVector_Color(hue, WEUI_COLOR_HSB_MAX_BRI)};
	for (u32 i = 0; i < pcPath->PtsNum; i++) {
		vertex.Pt = WeUi_DemoVector_ToPixel(pcPath->Pts[i]);
		WeUi_Gfx_Point_Draw(&vertex);
	}
	return RET_STATE_SUCCESS;
}

typedef RET_STATE_t (*WeUi_DemoPaths_Paint_t)(const WeUi_Gfx_Path_t* pcPath, float hue);

/**
 * @brief One frame at `t` in [0; 1): each shape in its own slot, swinging and growing out of
 *        step with the others, handed to `paint`.
 */
static RET_STATE_t WeUi_DemoPaths_Place(float t, WeUi_DemoPaths_Paint_t paint) {
	u32 count  = NUM_ELEMENTS(WeUi_DemoPaths_Shapes);
	float slot = (float)WeUi_Gfx_Adapter_GetWidth() / (float)count;
	float half = fminf(DEMO_PATHS_HALF_MAX * (float)WeUi_Gfx_Adapter_GetHeight(),
					   DEMO_PATHS_HALF_SLOT * slot);

	for (u32 i = 0; i < count; i++) {
		float phase = (float)i / (float)count;
		float wave	= 0.5f + 0.5f * sinf(2.0f * M_PIf * (DEMO_PATHS_BREATHS * t + phase));
		float scale = half * (DEMO_PATHS_SCALE_MIN + (1.0f - DEMO_PATHS_SCALE_MIN) * wave);

		WeUi_Gfx_AffineStack_t stack;
		WeUi_Gfx_AffineStack_Reset(&stack);
		WeUi_Gfx_AffineStack_Translate(&stack, slot * ((float)i + 0.5f),
									   (float)WeUi_Gfx_Adapter_GetY_Mid());
		WeUi_Gfx_AffineStack_Rotate(
			&stack, DEMO_PATHS_SWING * sinf(2.0f * M_PIf * (DEMO_PATHS_SWINGS * t + phase)));
		WeUi_Gfx_AffineStack_Scale(&stack, scale, scale);

		WeUi_Gfx_Path_Reset(&WeUi_DemoPaths_Path, &stack.Top);
		RETURN_IF_UNSUCCESS(WeUi_DemoPaths_Shapes[i](&WeUi_DemoPaths_Path));
		RETURN_IF_UNSUCCESS(paint(&WeUi_DemoPaths_Path, DEMO_PATHS_HUE_STEP * (float)i));
	}
	return RET_STATE_SUCCESS;
}

/* Curves are cut on the panel, so a shape drawn larger gets more points - the bright ones -
 * and a small one only a few */
static RET_STATE_t WeUi_DemoPaths_Render(float t) {
	return WeUi_DemoPaths_Place(t, WeUi_DemoPaths_Draw);
}

void WeUi_DrawDemo_Paths(void) {
	WeUi_DemoVector_Play(WeUi_DemoPaths_Render);
}

// =================== Fills ===================

#define DEMO_FILLS_STRIPE_W	   3 /* px */
/* Gray, so that no shape's hue is lost on them */
#define DEMO_FILLS_STRIPE_DARK 20
#define DEMO_FILLS_STRIPE_LITE 55

/* Stripes under the shapes: an edge blends with them, not just with black */
static RET_STATE_t WeUi_DemoFills_Stripes(void) {
	WeUi_Gfx_ObjSize_t width = WeUi_Gfx_Adapter_GetWidth();
	float h					 = (float)WeUi_Gfx_Adapter_GetHeight();
	u32 n					 = 0;

	for (u32 x = 0; x < width; x += DEMO_FILLS_STRIPE_W, n++) {
		u8 level = (n % 2) ? DEMO_FILLS_STRIPE_LITE : DEMO_FILLS_STRIPE_DARK;
		WeUi_Gfx_Path_Reset(&WeUi_DemoPaths_Path, NULL);
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_Rect(&WeUi_DemoPaths_Path, (float)x, 0.0f,
											   (float)(x + DEMO_FILLS_STRIPE_W), h));
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Fill_Solid(
			&WeUi_DemoPaths_Path, WeUi_Gfx_Transform_FillColorFromChannel(level), NULL));
	}
	return RET_STATE_SUCCESS;
}

static RET_STATE_t WeUi_DemoFills_Paint(const WeUi_Gfx_Path_t* pcPath, float hue) {
	return WeUi_Gfx_Fill_Solid(pcPath, WeUi_DemoVector_Color(hue, WEUI_COLOR_HSB_MAX_BRI), NULL);
}

static RET_STATE_t WeUi_DemoFills_Render(float t) {
	RETURN_IF_UNSUCCESS(WeUi_DemoFills_Stripes());
	return WeUi_DemoPaths_Place(t, WeUi_DemoFills_Paint);
}

void WeUi_DrawDemo_Fills(void) {
	WeUi_DemoVector_Play(WeUi_DemoFills_Render);
}

// =================== Gradients ===================

/* Sizes are shares of the panel height, travels and gaps of its width */
#define DEMO_GRAD_SKY_TURNS		0.5f /* the sky's direction over the demo */
#define DEMO_GRAD_BALL_R		0.38f
#define DEMO_GRAD_BALL_TRAVEL	0.3f /* either way from the middle */
#define DEMO_GRAD_BALL_TRIPS	1.5f
#define DEMO_GRAD_BALL_LIGHT	0.35f /* the highlight's offset up and left, share of the radius */
#define DEMO_GRAD_BALL_REACH	1.3f  /* the radial gradient's radius, share of the ball's */
#define DEMO_GRAD_CHEEK_X		0.78f /* the pair's middle */
#define DEMO_GRAD_CHEEK_GAP		0.1f  /* each cheek from the pair's middle */
#define DEMO_GRAD_CHEEK_RX		0.32f
#define DEMO_GRAD_CHEEK_RY		0.2f
#define DEMO_GRAD_CHEEK_BREATHS 3.0f
#define DEMO_GRAD_CHEEK_OPACITY 220
#define DEMO_GRAD_HEART_HALF	0.36f
#define DEMO_GRAD_HEART_OPACITY 150

/* Dim: on an LED panel a dark colour still glows, and a bright sky drowns what is over it */
static RET_STATE_t WeUi_DemoGradients_Sky(float t, float w, float h) {
	const WeUi_Gfx_PaintStop_t stops[] = {
		{0.0f, WeUi_Gfx_Transform_Rgb2Color(6, 6, 30)},
		{0.55f, WeUi_Gfx_Transform_Rgb2Color(45, 10, 35)},
		{1.0f, WeUi_Gfx_Transform_Rgb2Color(75, 35, 8)},
	};
	/* Across the panel through its middle, the line turning */
	float a				  = 2.0f * M_PIf * DEMO_GRAD_SKY_TURNS * t;
	float half			  = 0.5f * sqrtf(w * w + h * h);
	WeUi_Gfx_PointF_t mid = {0.5f * w, 0.5f * h};
	WeUi_Gfx_PointF_t arm = {half * cosf(a), half * sinf(a)};

	WeUi_Gfx_Paint_t sky = {
		.Type	  = WEUI_GFX_PAINT_LINEAR,
		.pcStops  = stops,
		.StopsNum = NUM_ELEMENTS(stops),
		.P0		  = {mid.X - arm.X, mid.Y - arm.Y},
		.P1		  = {mid.X + arm.X, mid.Y + arm.Y},
		.Opacity  = WEUI_GFX_PAINT_OPAQUE,
	};
	WeUi_Gfx_Path_Reset(&WeUi_DemoPaths_Path, NULL);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_Rect(&WeUi_DemoPaths_Path, 0.0f, 0.0f, w, h));
	return WeUi_Gfx_Fill_Paint(&WeUi_DemoPaths_Path, &sky, NULL);
}

static RET_STATE_t WeUi_DemoGradients_Cheeks(float t, float w, float h) {
	float breath		  = 0.5f + 0.5f * sinf(2.0f * M_PIf * DEMO_GRAD_CHEEK_BREATHS * t);
	WeUi_Gfx_Soft_t cheek = {
		.Rx		 = DEMO_GRAD_CHEEK_RX * h,
		.Ry		 = DEMO_GRAD_CHEEK_RY * h,
		.Color	 = WeUi_Gfx_Transform_Rgb2Color(255, 200, 220),
		.Opacity = (u8)(DEMO_GRAD_CHEEK_OPACITY * breath),
	};
	for (s32 side = -1; side <= 1; side += 2) {
		cheek.Centre =
			(WeUi_Gfx_PointF_t){(DEMO_GRAD_CHEEK_X + side * DEMO_GRAD_CHEEK_GAP) * w, 0.5f * h};
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Fill_Soft(&cheek, NULL));
	}
	return RET_STATE_SUCCESS;
}

/* Lit from the upper left: white where the light hits, the ball's colour, dark at its rim */
static RET_STATE_t WeUi_DemoGradients_Ball(float x, float y, float r) {
	const WeUi_Gfx_PaintStop_t stops[] = {
		{0.0f, WeUi_Gfx_Transform_Rgb2Color(255, 255, 255)},
		{0.3f, WeUi_Gfx_Transform_Rgb2Color(80, 200, 255)},
		{1.0f, WeUi_Gfx_Transform_Rgb2Color(10, 40, 90)},
	};
	WeUi_Gfx_Paint_t lit = {
		.Type	  = WEUI_GFX_PAINT_RADIAL,
		.pcStops  = stops,
		.StopsNum = NUM_ELEMENTS(stops),
		.P0		  = {x - DEMO_GRAD_BALL_LIGHT * r, y - DEMO_GRAD_BALL_LIGHT * r},
		.Radius	  = DEMO_GRAD_BALL_REACH * r,
		.Opacity  = WEUI_GFX_PAINT_OPAQUE,
	};
	WeUi_Gfx_Path_Reset(&WeUi_DemoPaths_Path, NULL);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_Ellipse(&WeUi_DemoPaths_Path, x, y, r, r));
	return WeUi_Gfx_Fill_Paint(&WeUi_DemoPaths_Path, &lit, NULL);
}

static RET_STATE_t WeUi_DemoGradients_Heart(float x, float y, float half) {
	WeUi_Gfx_AffineStack_t stack;
	WeUi_Gfx_AffineStack_Reset(&stack);
	WeUi_Gfx_AffineStack_Translate(&stack, x, y);
	WeUi_Gfx_AffineStack_Scale(&stack, half, half);
	WeUi_Gfx_Path_Reset(&WeUi_DemoPaths_Path, &stack.Top);
	RETURN_IF_UNSUCCESS(WeUi_DemoPaths_Heart(&WeUi_DemoPaths_Path));

	WeUi_Gfx_Paint_t see = WeUi_Gfx_Paint_Solid(WeUi_Gfx_Transform_Rgb2Color(255, 40, 60));
	see.Opacity			 = DEMO_GRAD_HEART_OPACITY;
	return WeUi_Gfx_Fill_Paint(&WeUi_DemoPaths_Path, &see, NULL);
}

/**
 * @brief One frame at `t` in [0; 1): a turning sky, breathing cheeks, a lit ball rolling to
 *        and fro and a see-through heart going the other way over it.
 */
static RET_STATE_t WeUi_DemoGradients_Render(float t) {
	float w	   = (float)WeUi_Gfx_Adapter_GetWidth();
	float h	   = (float)WeUi_Gfx_Adapter_GetHeight();
	float roll = sinf(2.0f * M_PIf * DEMO_GRAD_BALL_TRIPS * t) * DEMO_GRAD_BALL_TRAVEL * w;

	RETURN_IF_UNSUCCESS(WeUi_DemoGradients_Sky(t, w, h));
	RETURN_IF_UNSUCCESS(WeUi_DemoGradients_Cheeks(t, w, h));
	RETURN_IF_UNSUCCESS(WeUi_DemoGradients_Ball(0.5f * w + roll, 0.5f * h, DEMO_GRAD_BALL_R * h));
	return WeUi_DemoGradients_Heart(0.5f * w - roll, 0.5f * h, DEMO_GRAD_HEART_HALF * h);
}

void WeUi_DrawDemo_Gradients(void) {
	WeUi_DemoVector_Play(WeUi_DemoGradients_Render);
}

// =================== Strokes ===================

/* Places are shares of the panel width and height; widths are in panel pixels */
#define DEMO_STROKES_WHISKERS	  3
#define DEMO_STROKES_WHISKER_X0	  0.03f
#define DEMO_STROKES_WHISKER_X1	  0.27f
#define DEMO_STROKES_WHISKER_Y0	  0.25f /* the top one; the others below by the same */
#define DEMO_STROKES_WHISKER_SWAY 0.12f /* of the height, the middle's swing */
#define DEMO_STROKES_WAVES		  2.0f
#define DEMO_STROKES_MOUTH_X	  0.38f
#define DEMO_STROKES_MOUTH_HALF	  0.075f /* of the width, the mouth's half */
#define DEMO_STROKES_MOUTH_SHUT	  0.4f	 /* the dip of each half, of the half width */
#define DEMO_STROKES_MOUTH_OPEN	  0.6f	 /* added at the widest */
#define DEMO_STROKES_MOUTH_EDGE	  -0.3f	 /* the corners and the middle, lifted */
#define DEMO_STROKES_MOUTH_MID	  -0.15f
#define DEMO_STROKES_MOUTH_W	  1.6f
#define DEMO_STROKES_ZIG_X0		  0.52f
#define DEMO_STROKES_ZIG_X1		  0.74f
#define DEMO_STROKES_ZIG_TOP	  0.25f
#define DEMO_STROKES_ZIG_BOTTOM	  0.75f
#define DEMO_STROKES_ZIG_PTS	  5
#define DEMO_STROKES_ZIG_W		  3.5f
#define DEMO_STROKES_HEART_X	  0.88f
#define DEMO_STROKES_HEART_HALF	  0.32f /* of the height */
#define DEMO_STROKES_HEART_PULSE  0.15f
#define DEMO_STROKES_HEART_BEATS  4.0f
#define DEMO_STROKES_OUTLINE_W	  1.5f

static const float WeUi_DemoStrokes_WhiskerW[DEMO_STROKES_WHISKERS] = {0.7f, 1.4f, 2.4f};

static const WEUI_GFX_JOIN_t WeUi_DemoStrokes_Joins[] = {
	WEUI_GFX_JOIN_MITER,
	WEUI_GFX_JOIN_ROUND,
	WEUI_GFX_JOIN_BEVEL,
};

/* Where a stroke is built: the centre line goes in WeUi_DemoPaths_Path */
static WeUi_Gfx_Path_t WEUI_DATA_PLACEMENT WeUi_DemoStrokes_Out;

static RET_STATE_t WeUi_DemoStrokes_Draw(const WeUi_Gfx_StrokeStyle_t* pcStyle,
										 WeUi_Gfx_Color_t color) {
	RETURN_IF_UNSUCCESS(
		WeUi_Gfx_Stroke_Build(&WeUi_DemoPaths_Path, pcStyle, &WeUi_DemoStrokes_Out));
	return WeUi_Gfx_Fill_Solid(&WeUi_DemoStrokes_Out, color, NULL);
}

/* Lines of three widths, swaying */
static RET_STATE_t WeUi_DemoStrokes_Whiskers(float t, float w, float h) {
	for (u32 i = 0; i < DEMO_STROKES_WHISKERS; i++) {
		float y		= DEMO_STROKES_WHISKER_Y0 * (float)(i + 1) * h;
		float phase = 2.0f * M_PIf * (DEMO_STROKES_WAVES * t + (float)i / DEMO_STROKES_WHISKERS);
		float sway	= DEMO_STROKES_WHISKER_SWAY * h * sinf(phase);
		float x0	= DEMO_STROKES_WHISKER_X0 * w;
		float x1	= DEMO_STROKES_WHISKER_X1 * w;

		WeUi_Gfx_Path_Reset(&WeUi_DemoPaths_Path, NULL);
		RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_MoveTo(&WeUi_DemoPaths_Path, x0, y));
		RETURN_IF_UNSUCCESS(
			WeUi_Gfx_Path_QuadTo(&WeUi_DemoPaths_Path, 0.5f * (x0 + x1), y + sway, x1, y - sway));
		WeUi_Gfx_StrokeStyle_t style = WeUi_Gfx_Stroke_Style(WeUi_DemoStrokes_WhiskerW[i]);
		RETURN_IF_UNSUCCESS(
			WeUi_DemoStrokes_Draw(&style, WeUi_Gfx_Transform_Rgb2Color(200, 230, 255)));
	}
	return RET_STATE_SUCCESS;
}

/* The cat's mouth, two curves meeting in the middle, opening and closing */
static RET_STATE_t WeUi_DemoStrokes_Mouth(float t, float w, float h) {
	float open =
		DEMO_STROKES_MOUTH_SHUT +
		DEMO_STROKES_MOUTH_OPEN * (0.5f + 0.5f * sinf(2.0f * M_PIf * DEMO_STROKES_WAVES * t));

	WeUi_Gfx_AffineStack_t stack;
	WeUi_Gfx_AffineStack_Reset(&stack);
	WeUi_Gfx_AffineStack_Translate(&stack, DEMO_STROKES_MOUTH_X * w, 0.5f * h);
	WeUi_Gfx_AffineStack_Scale(&stack, DEMO_STROKES_MOUTH_HALF * w, DEMO_STROKES_MOUTH_HALF * w);

	WeUi_Gfx_Path_t* pPath = &WeUi_DemoPaths_Path;
	WeUi_Gfx_Path_Reset(pPath, &stack.Top);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_MoveTo(pPath, -1.0f, DEMO_STROKES_MOUTH_EDGE));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_QuadTo(pPath, -0.5f, open, 0.0f, DEMO_STROKES_MOUTH_MID));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_QuadTo(pPath, 0.5f, open, 1.0f, DEMO_STROKES_MOUTH_EDGE));

	WeUi_Gfx_StrokeStyle_t style = WeUi_Gfx_Stroke_Style(DEMO_STROKES_MOUTH_W);
	return WeUi_DemoStrokes_Draw(&style, WeUi_Gfx_Transform_Rgb2Color(255, 120, 170));
}

/* A thick zigzag, its corners joined each way in turn */
static RET_STATE_t WeUi_DemoStrokes_Zigzag(float t, float w, float h) {
	WeUi_Gfx_Path_Reset(&WeUi_DemoPaths_Path, NULL);
	for (u32 i = 0; i < DEMO_STROKES_ZIG_PTS; i++) {
		float x = (DEMO_STROKES_ZIG_X0 + (DEMO_STROKES_ZIG_X1 - DEMO_STROKES_ZIG_X0) * (float)i /
											 (DEMO_STROKES_ZIG_PTS - 1)) *
				  w;
		float y = ((i % 2) ? DEMO_STROKES_ZIG_BOTTOM : DEMO_STROKES_ZIG_TOP) * h;
		RETURN_IF_UNSUCCESS(i == 0 ? WeUi_Gfx_Path_MoveTo(&WeUi_DemoPaths_Path, x, y)
								   : WeUi_Gfx_Path_LineTo(&WeUi_DemoPaths_Path, x, y));
	}

	u32 joins					 = NUM_ELEMENTS(WeUi_DemoStrokes_Joins);
	WeUi_Gfx_StrokeStyle_t style = WeUi_Gfx_Stroke_Style(DEMO_STROKES_ZIG_W);
	style.Join					 = WeUi_DemoStrokes_Joins[GET_MIN((u32)(t * joins), joins - 1u)];
	style.Cap					 = WEUI_GFX_CAP_BUTT;
	return WeUi_DemoStrokes_Draw(&style, WeUi_Gfx_Transform_Rgb2Color(255, 160, 40));
}

/* A beating heart, its outline drawn first and the heart over it */
static RET_STATE_t WeUi_DemoStrokes_Heart(float t, float w, float h) {
	float beat =
		1.0f + DEMO_STROKES_HEART_PULSE * fabsf(sinf(M_PIf * DEMO_STROKES_HEART_BEATS * t));
	float half = DEMO_STROKES_HEART_HALF * h * beat;

	WeUi_Gfx_AffineStack_t stack;
	WeUi_Gfx_AffineStack_Reset(&stack);
	WeUi_Gfx_AffineStack_Translate(&stack, DEMO_STROKES_HEART_X * w, 0.5f * h);
	WeUi_Gfx_AffineStack_Scale(&stack, half, half);
	WeUi_Gfx_Path_Reset(&WeUi_DemoPaths_Path, &stack.Top);
	RETURN_IF_UNSUCCESS(WeUi_DemoPaths_Heart(&WeUi_DemoPaths_Path));

	WeUi_Gfx_StrokeStyle_t style = WeUi_Gfx_Stroke_Style(DEMO_STROKES_OUTLINE_W);
	RETURN_IF_UNSUCCESS(
		WeUi_Gfx_Stroke_Outline(&WeUi_DemoPaths_Path, &style, &WeUi_DemoStrokes_Out));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Fill_Solid(&WeUi_DemoStrokes_Out,
											WeUi_Gfx_Transform_Rgb2Color(255, 230, 120), NULL));
	return WeUi_Gfx_Fill_Solid(&WeUi_DemoPaths_Path, WeUi_Gfx_Transform_Rgb2Color(220, 20, 40),
							   NULL);
}

static RET_STATE_t WeUi_DemoStrokes_Render(float t) {
	float w = (float)WeUi_Gfx_Adapter_GetWidth();
	float h = (float)WeUi_Gfx_Adapter_GetHeight();

	RETURN_IF_UNSUCCESS(WeUi_DemoStrokes_Whiskers(t, w, h));
	RETURN_IF_UNSUCCESS(WeUi_DemoStrokes_Mouth(t, w, h));
	RETURN_IF_UNSUCCESS(WeUi_DemoStrokes_Zigzag(t, w, h));
	return WeUi_DemoStrokes_Heart(t, w, h);
}

void WeUi_DrawDemo_Strokes(void) {
	WeUi_DemoVector_Play(WeUi_DemoStrokes_Render);
}

// =================== Eyes ===================

/* Sizes are shares of the eye's radius, the radius one of the panel height */
#define DEMO_EYES_R			 0.42f
#define DEMO_EYES_X			 0.33f /* of the width, either side of the middle */
#define DEMO_EYES_LID_REACH	 1.36f /* how far the lids' curves bulge: the open eye shows all */
#define DEMO_EYES_LID_ARM	 0.55f /* the lids' control points along the eye */
#define DEMO_EYES_CORNER	 1.0f  /* px the opening reaches past the eye's sides */
#define DEMO_EYES_GAZE		 0.3f
#define DEMO_EYES_LOOKS_X	 1.5f /* side to side over the demo */
#define DEMO_EYES_LOOKS_Y	 0.9f
#define DEMO_EYES_LOOK_Y	 0.4f /* of the side to side reach */
#define DEMO_EYES_PUPIL		 0.55f
#define DEMO_EYES_GLINT		 0.35f /* of the pupil's radius */
#define DEMO_EYES_GLINT_AT	 0.45f /* up and left of the pupil's centre, of its radius */
#define DEMO_EYES_NOTCH_AT	 0.5f  /* the circle cut from the glint, right and down of it */
#define DEMO_EYES_NOTCH		 0.85f /* of the glint's radius */
#define DEMO_EYES_SPARK		 0.12f /* the small glint, of the pupil's radius */
#define DEMO_EYES_SPARK_AT	 0.5f
#define DEMO_EYES_SPARK_SEEN 220
#define DEMO_EYES_LINER_W	 1.2f
#define DEMO_EYES_BLINK_LEN	 0.08f /* of the demo */
#define DEMO_EYES_BLINKS	 2
/* A mask holds at most the eye's square, with a pixel round it */
#define DEMO_EYES_MASK_SIDE	 (WEUI_DISPLAY_HEIGHT + 2)
#define DEMO_EYES_MASK_BYTES (DEMO_EYES_MASK_SIDE * DEMO_EYES_MASK_SIDE)

static const float WeUi_DemoEyes_BlinkAt[DEMO_EYES_BLINKS] = {0.45f, 0.85f};

typedef enum {
	DEMO_EYES_MASK_DISC = 0, /* the eye */
	DEMO_EYES_MASK_OPENING,	 /* between the lids */
	DEMO_EYES_MASK_NOTCH,	 /* cut from the glint, leaving a crescent */

	DEMO_EYES_MASKS_NUM
} DEMO_EYES_MASK_t;

static u8 WEUI_DATA_PLACEMENT WeUi_DemoEyes_Bufs[DEMO_EYES_MASKS_NUM][DEMO_EYES_MASK_BYTES];
static WeUi_Gfx_Mask_t WeUi_DemoEyes_Masks[DEMO_EYES_MASKS_NUM];

/* What the path in WeUi_DemoPaths_Path covers, as a mask over its bounds on the panel */
static RET_STATE_t WeUi_DemoEyes_MaskPath(DEMO_EYES_MASK_t which) {
	WeUi_Gfx_ClipRect_t rect;
	WeUi_Gfx_ClipRect_t panel = WeUi_Gfx_Fill_PanelClip();
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Raster_Bounds(&WeUi_DemoPaths_Path, &rect));
	rect.X0 = GET_MAX(rect.X0, panel.X0);
	rect.Y0 = GET_MAX(rect.Y0, panel.Y0);
	rect.X1 = GET_MIN(rect.X1, panel.X1);
	rect.Y1 = GET_MIN(rect.Y1, panel.Y1);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Mask_Init(&WeUi_DemoEyes_Masks[which], WeUi_DemoEyes_Bufs[which],
										   DEMO_EYES_MASK_BYTES, &rect));
	return WeUi_Gfx_Mask_AddPath(&WeUi_DemoEyes_Masks[which], &WeUi_DemoPaths_Path,
								 WEUI_GFX_PAINT_OPAQUE);
}

/** @brief How shut the lids are at t: 0 open, 1 shut, in and out over each blink */
static float WeUi_DemoEyes_Lid(float t) {
	for (u32 i = 0; i < DEMO_EYES_BLINKS; i++) {
		float d = fabsf(t - WeUi_DemoEyes_BlinkAt[i]) / (0.5f * DEMO_EYES_BLINK_LEN);
		if (d < 1.0f)
			return 1.0f - d;
	}
	return 0.0f;
}

/* The opening between the lids, each lid a curve from corner to corner, closing to the middle */
static RET_STATE_t WeUi_DemoEyes_Opening(float r, float lid, bool upperOnly) {
	float reach = DEMO_EYES_LID_REACH * r * (1.0f - lid);
	float side	= r + DEMO_EYES_CORNER;
	float arm	= DEMO_EYES_LID_ARM * r;

	WeUi_Gfx_Path_t* pPath = &WeUi_DemoPaths_Path;
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_MoveTo(pPath, -side, 0.0f));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_CubicTo(pPath, -arm, -reach, arm, -reach, side, 0.0f));
	if (upperOnly)
		return RET_STATE_SUCCESS;
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_CubicTo(pPath, arm, reach, -arm, reach, -side, 0.0f));
	return WeUi_Gfx_Path_Close(pPath);
}

static RET_STATE_t WeUi_DemoEyes_Disc(const WeUi_Gfx_Affine_t* pcXf, float x, float y, float r,
									  const WeUi_Gfx_Paint_t* pcPaint) {
	WeUi_Gfx_Path_Reset(&WeUi_DemoPaths_Path, pcXf);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_Ellipse(&WeUi_DemoPaths_Path, x, y, r, r));
	return WeUi_Gfx_Fill_Paint(&WeUi_DemoPaths_Path, pcPaint, NULL);
}

/**
 * @brief One eye round (cx, cy): the iris, pupil and glints show only inside both the eye and
 *        the opening between the lids; the glint is a crescent, a disc with another one cut out.
 */
static RET_STATE_t WeUi_DemoEyes_Eye(float cx, float cy, float r, float t) {
	float lid = WeUi_DemoEyes_Lid(t);
	float gx  = DEMO_EYES_GAZE * r * sinf(2.0f * M_PIf * DEMO_EYES_LOOKS_X * t);
	float gy  = DEMO_EYES_GAZE * r * DEMO_EYES_LOOK_Y * sinf(2.0f * M_PIf * DEMO_EYES_LOOKS_Y * t);

	WeUi_Gfx_AffineStack_t stack;
	WeUi_Gfx_AffineStack_Reset(&stack);
	WeUi_Gfx_AffineStack_Translate(&stack, cx, cy);
	const WeUi_Gfx_Affine_t* pcXf = &stack.Top;

	WeUi_Gfx_Path_Reset(&WeUi_DemoPaths_Path, pcXf);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_Ellipse(&WeUi_DemoPaths_Path, 0.0f, 0.0f, r, r));
	RETURN_IF_UNSUCCESS(WeUi_DemoEyes_MaskPath(DEMO_EYES_MASK_DISC));
	WeUi_Gfx_Path_Reset(&WeUi_DemoPaths_Path, pcXf);
	RETURN_IF_UNSUCCESS(WeUi_DemoEyes_Opening(r, lid, false));
	RETURN_IF_UNSUCCESS(WeUi_DemoEyes_MaskPath(DEMO_EYES_MASK_OPENING));

	const WeUi_Gfx_MaskUse_t seen[] = {
		{&WeUi_DemoEyes_Masks[DEMO_EYES_MASK_DISC], WEUI_GFX_MASK_IN},
		{&WeUi_DemoEyes_Masks[DEMO_EYES_MASK_OPENING], WEUI_GFX_MASK_IN},
		{&WeUi_DemoEyes_Masks[DEMO_EYES_MASK_NOTCH], WEUI_GFX_MASK_OUT},
	};
	const u8 seenNum = DEMO_EYES_MASK_OPENING + 1; /* the notch only for the crescent */

	const WeUi_Gfx_PaintStop_t iris[] = {
		{0.0f, WeUi_Gfx_Transform_Rgb2Color(170, 230, 60)},
		{1.0f, WeUi_Gfx_Transform_Rgb2Color(20, 110, 40)},
	};
	WeUi_Gfx_Paint_t paint = {
		.Type	  = WEUI_GFX_PAINT_LINEAR,
		.pcStops  = iris,
		.StopsNum = NUM_ELEMENTS(iris),
		.P0		  = {0.0f, -r},
		.P1		  = {0.0f, r},
		.pcXf	  = pcXf,
		.Opacity  = WEUI_GFX_PAINT_OPAQUE,
		.pcMasks  = seen,
		.MasksNum = seenNum,
	};
	WeUi_Gfx_Path_Reset(&WeUi_DemoPaths_Path, pcXf);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_Rect(&WeUi_DemoPaths_Path, -r, -r, r, r));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Fill_Paint(&WeUi_DemoPaths_Path, &paint, NULL));

	float pr	   = DEMO_EYES_PUPIL * r;
	paint		   = WeUi_Gfx_Paint_Solid(WeUi_Gfx_Transform_Rgb2Color(0, 0, 0));
	paint.pcMasks  = seen;
	paint.MasksNum = seenNum;
	RETURN_IF_UNSUCCESS(WeUi_DemoEyes_Disc(pcXf, gx, gy, pr, &paint));

	float glintR = DEMO_EYES_GLINT * pr;
	float glintX = gx - DEMO_EYES_GLINT_AT * pr;
	float glintY = gy - DEMO_EYES_GLINT_AT * pr;
	WeUi_Gfx_Path_Reset(&WeUi_DemoPaths_Path, pcXf);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_Ellipse(
		&WeUi_DemoPaths_Path, glintX + DEMO_EYES_NOTCH_AT * glintR,
		glintY + DEMO_EYES_NOTCH_AT * glintR, DEMO_EYES_NOTCH * glintR, DEMO_EYES_NOTCH * glintR));
	RETURN_IF_UNSUCCESS(WeUi_DemoEyes_MaskPath(DEMO_EYES_MASK_NOTCH));
	paint		   = WeUi_Gfx_Paint_Solid(WeUi_Gfx_Transform_Rgb2Color(255, 255, 255));
	paint.pcMasks  = seen;
	paint.MasksNum = NUM_ELEMENTS(seen);
	RETURN_IF_UNSUCCESS(WeUi_DemoEyes_Disc(pcXf, glintX, glintY, glintR, &paint));

	paint.MasksNum = seenNum;
	paint.Opacity  = DEMO_EYES_SPARK_SEEN;
	RETURN_IF_UNSUCCESS(WeUi_DemoEyes_Disc(pcXf, gx + DEMO_EYES_SPARK_AT * pr,
										   gy + DEMO_EYES_SPARK_AT * pr, DEMO_EYES_SPARK * pr,
										   &paint));

	/* The upper lid's edge, a line along the top of the opening */
	WeUi_Gfx_Path_Reset(&WeUi_DemoPaths_Path, pcXf);
	RETURN_IF_UNSUCCESS(WeUi_DemoEyes_Opening(r, lid, true));
	WeUi_Gfx_StrokeStyle_t liner = WeUi_Gfx_Stroke_Style(DEMO_EYES_LINER_W);
	return WeUi_DemoStrokes_Draw(&liner, WeUi_Gfx_Transform_Rgb2Color(190, 190, 210));
}

static RET_STATE_t WeUi_DemoEyes_Render(float t) {
	float w = (float)WeUi_Gfx_Adapter_GetWidth();
	float h = (float)WeUi_Gfx_Adapter_GetHeight();
	float r = DEMO_EYES_R * h;

	for (s32 side = -1; side <= 1; side += 2)
		RETURN_IF_UNSUCCESS(
			WeUi_DemoEyes_Eye((0.5f + side * DEMO_EYES_X * 0.5f) * w, 0.5f * h, r, t));
	return RET_STATE_SUCCESS;
}

void WeUi_DrawDemo_Eyes(void) {
	WeUi_DemoVector_Play(WeUi_DemoEyes_Render);
}

// =================== Scene ===================

/* Sizes are shares of the unit: the scene's height, or a share of its width on a panel not much
 * wider than high, so that the face keeps its shape on a strip and on a round screen alike.
 * The eyes are the eyes demo's */
#define DEMO_SCENE_UNIT_W	   0.3f
#define DEMO_SCENE_EYES_APART  0.6f /* each eye from the middle */
#define DEMO_SCENE_MOUTH_Y	   0.32f
#define DEMO_SCENE_MOUTH_HALF  0.18f
#define DEMO_SCENE_MOUTH_DIP   0.8f /* of the half width */
#define DEMO_SCENE_MOUTH_W	   0.05f
#define DEMO_SCENE_FLY_Y	   -0.36f
#define DEMO_SCENE_FLY_R	   0.12f
#define DEMO_SCENE_FLY_OPACITY 200
#define DEMO_SCENE_BAR_H	   0.02f /* under the face: how much of the panel was redrawn */
#define DEMO_SCENE_BAR_MIN	   1.0f	 /* px */
#define DEMO_SCENE_RECTS	   4
#define DEMO_SCENE_GAZES	   5
#define DEMO_SCENE_PERCENT	   100u
#define DEMO_SCENE_TENTHS	   10u

/* Where the eyes look, in turns: still in between, so that mostly nothing changes */
static const WeUi_Gfx_PointF_t WeUi_DemoScene_Gazes[DEMO_SCENE_GAZES] = {
	{0.0f, 0.0f}, {0.9f, 0.2f}, {-0.7f, -0.3f}, {0.3f, 0.5f}, {-0.9f, 0.0f},
};

typedef struct {
	float W;
	float H;
	float Unit;
	float Bar; /* the bar's height */
	WeUi_Gfx_PointF_t Face;
} WeUi_DemoScene_Layout_t;

static WeUi_Gfx_Scene_t WEUI_DATA_PLACEMENT WeUi_DemoScene_Scene;
static WeUi_Gfx_ClipRect_t WeUi_DemoScene_Rects[DEMO_SCENE_RECTS];

static RET_STATE_t WeUi_DemoScene_Fill(const WeUi_Gfx_Paint_t* pcPaint, u8* pIdx) {
	return WeUi_Gfx_Scene_Fill(&WeUi_DemoScene_Scene, &WeUi_DemoPaths_Path, pcPaint, pIdx);
}

static RET_STATE_t WeUi_DemoScene_ClipBoth(u8 item, u8 first, u8 second,
										   WEUI_GFX_MASK_MODE_t mode) {
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Scene_Clip(&WeUi_DemoScene_Scene, item, first, WEUI_GFX_MASK_IN));
	return WeUi_Gfx_Scene_Clip(&WeUi_DemoScene_Scene, item, second, mode);
}

/** @brief The eyes demo's eye as scene items: masks are items, and items are cut by them */
static RET_STATE_t WeUi_DemoScene_Eye(float cx, float cy, float r, WeUi_Gfx_PointF_t gaze,
									  float lid) {
	WeUi_Gfx_Scene_t* pScene = &WeUi_DemoScene_Scene;
	WeUi_Gfx_Path_t* pPath	 = &WeUi_DemoPaths_Path;
	WeUi_Gfx_AffineStack_t stack;
	WeUi_Gfx_AffineStack_Reset(&stack);
	WeUi_Gfx_AffineStack_Translate(&stack, cx, cy);
	const WeUi_Gfx_Affine_t* pcXf = &stack.Top;
	u8 disc, opening, notch, item;

	WeUi_Gfx_Path_Reset(pPath, pcXf);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_Ellipse(pPath, 0.0f, 0.0f, r, r));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Scene_Mask(pScene, pPath, &disc));
	WeUi_Gfx_Path_Reset(pPath, pcXf);
	RETURN_IF_UNSUCCESS(WeUi_DemoEyes_Opening(r, lid, false));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Scene_Mask(pScene, pPath, &opening));

	const WeUi_Gfx_PaintStop_t iris[] = {
		{0.0f, WeUi_Gfx_Transform_Rgb2Color(170, 230, 60)},
		{1.0f, WeUi_Gfx_Transform_Rgb2Color(20, 110, 40)},
	};
	WeUi_Gfx_Paint_t paint = {
		.Type	  = WEUI_GFX_PAINT_LINEAR,
		.pcStops  = iris,
		.StopsNum = NUM_ELEMENTS(iris),
		.P0		  = {0.0f, -r},
		.P1		  = {0.0f, r},
		.pcXf	  = pcXf,
		.Opacity  = WEUI_GFX_PAINT_OPAQUE,
	};
	WeUi_Gfx_Path_Reset(pPath, pcXf);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_Rect(pPath, -r, -r, r, r));
	RETURN_IF_UNSUCCESS(WeUi_DemoScene_Fill(&paint, &item));
	RETURN_IF_UNSUCCESS(WeUi_DemoScene_ClipBoth(item, disc, opening, WEUI_GFX_MASK_IN));

	float pr = DEMO_EYES_PUPIL * r;
	float gx = DEMO_EYES_GAZE * r * gaze.X;
	float gy = DEMO_EYES_GAZE * r * gaze.Y;
	paint	 = WeUi_Gfx_Paint_Solid(WeUi_Gfx_Transform_Rgb2Color(0, 0, 0));
	WeUi_Gfx_Path_Reset(pPath, pcXf);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_Ellipse(pPath, gx, gy, pr, pr));
	RETURN_IF_UNSUCCESS(WeUi_DemoScene_Fill(&paint, &item));
	RETURN_IF_UNSUCCESS(WeUi_DemoScene_ClipBoth(item, disc, opening, WEUI_GFX_MASK_IN));

	/* The crescent: inside the opening, outside the notch - an item takes two masks */
	float glintR = DEMO_EYES_GLINT * pr;
	float glintX = gx - DEMO_EYES_GLINT_AT * pr;
	float glintY = gy - DEMO_EYES_GLINT_AT * pr;
	WeUi_Gfx_Path_Reset(pPath, pcXf);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_Ellipse(pPath, glintX + DEMO_EYES_NOTCH_AT * glintR,
											  glintY + DEMO_EYES_NOTCH_AT * glintR,
											  DEMO_EYES_NOTCH * glintR, DEMO_EYES_NOTCH * glintR));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Scene_Mask(pScene, pPath, &notch));
	paint = WeUi_Gfx_Paint_Solid(WeUi_Gfx_Transform_Rgb2Color(255, 255, 255));
	WeUi_Gfx_Path_Reset(pPath, pcXf);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_Ellipse(pPath, glintX, glintY, glintR, glintR));
	RETURN_IF_UNSUCCESS(WeUi_DemoScene_Fill(&paint, &item));
	RETURN_IF_UNSUCCESS(WeUi_DemoScene_ClipBoth(item, opening, notch, WEUI_GFX_MASK_OUT));

	WeUi_Gfx_Path_Reset(pPath, pcXf);
	RETURN_IF_UNSUCCESS(WeUi_DemoEyes_Opening(r, lid, true));
	WeUi_Gfx_StrokeStyle_t liner = WeUi_Gfx_Stroke_Style(DEMO_EYES_LINER_W);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Build(pPath, &liner, &WeUi_DemoStrokes_Out));
	paint = WeUi_Gfx_Paint_Solid(WeUi_Gfx_Transform_Rgb2Color(190, 190, 210));
	return WeUi_Gfx_Scene_Fill(pScene, &WeUi_DemoStrokes_Out, &paint, NULL);
}

static WeUi_DemoScene_Layout_t WeUi_DemoScene_Layout(void) {
	WeUi_DemoScene_Layout_t l = {.W = (float)WeUi_Gfx_Adapter_GetWidth(),
								 .H = (float)WeUi_Gfx_Adapter_GetHeight()};
	l.Bar					  = fmaxf(DEMO_SCENE_BAR_MIN, floorf(DEMO_SCENE_BAR_H * l.H));
	l.Unit					  = fminf(l.H - l.Bar, DEMO_SCENE_UNIT_W * l.W);
	l.Face					  = (WeUi_Gfx_PointF_t){0.5f * l.W, 0.5f * (l.H - l.Bar)};
	return l;
}

/** @brief The frame at t: a still face but for its eyes now and then, a firefly, and the bar
 *         as long as the share of the panel `redrawn` in the frame before */
static RET_STATE_t WeUi_DemoScene_Build(float t, const WeUi_DemoScene_Layout_t* pcL,
										float redrawn) {
	WeUi_Gfx_Scene_t* pScene = &WeUi_DemoScene_Scene;
	WeUi_Gfx_Path_t* pPath	 = &WeUi_DemoPaths_Path;
	float u					 = pcL->Unit;
	float cx				 = pcL->Face.X;
	float cy				 = pcL->Face.Y;
	WeUi_Gfx_Scene_Begin(pScene);

	/* Black: a dim gradient comes apart into a few bands on an LED panel. It is an item all the
	 * same - whatever rectangle is redrawn, it paints every pixel first */
	WeUi_Gfx_Paint_t paint = WeUi_Gfx_Paint_Solid(WeUi_Gfx_Transform_Rgb2Color(0, 0, 0));
	WeUi_Gfx_Path_Reset(pPath, NULL);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_Rect(pPath, 0.0f, 0.0f, pcL->W, pcL->H));
	RETURN_IF_UNSUCCESS(WeUi_DemoScene_Fill(&paint, NULL));

	WeUi_Gfx_Soft_t fly = {
		.Centre	 = {t * pcL->W, cy + DEMO_SCENE_FLY_Y * u},
		.Rx		 = DEMO_SCENE_FLY_R * u,
		.Ry		 = DEMO_SCENE_FLY_R * u,
		.Color	 = WeUi_Gfx_Transform_Rgb2Color(255, 240, 120),
		.Opacity = DEMO_SCENE_FLY_OPACITY,
	};
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Scene_Soft(pScene, &fly, NULL));

	u32 gaze = GET_MIN((u32)(t * DEMO_SCENE_GAZES), DEMO_SCENE_GAZES - 1u);
	for (s32 side = -1; side <= 1; side += 2) {
		RETURN_IF_UNSUCCESS(WeUi_DemoScene_Eye(cx + side * DEMO_SCENE_EYES_APART * u, cy,
											   DEMO_EYES_R * u, WeUi_DemoScene_Gazes[gaze],
											   WeUi_DemoEyes_Lid(t)));
	}

	float half = DEMO_SCENE_MOUTH_HALF * u;
	float my   = cy + DEMO_SCENE_MOUTH_Y * u;
	float dip  = my + DEMO_SCENE_MOUTH_DIP * half;
	WeUi_Gfx_Path_Reset(pPath, NULL);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_MoveTo(pPath, cx - half, my));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_QuadTo(pPath, cx - 0.5f * half, dip, cx, my));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_QuadTo(pPath, cx + 0.5f * half, dip, cx + half, my));
	WeUi_Gfx_StrokeStyle_t style = WeUi_Gfx_Stroke_Style(DEMO_SCENE_MOUTH_W * u);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Stroke_Build(pPath, &style, &WeUi_DemoStrokes_Out));
	paint = WeUi_Gfx_Paint_Solid(WeUi_Gfx_Transform_Rgb2Color(255, 120, 170));
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Scene_Fill(pScene, &WeUi_DemoStrokes_Out, &paint, NULL));

	if (redrawn <= 0.0f)
		return RET_STATE_SUCCESS;
	float barY = cy + 0.5f * u;
	paint	   = WeUi_Gfx_Paint_Solid(WeUi_Gfx_Transform_Rgb2Color(60, 255, 60));
	WeUi_Gfx_Path_Reset(pPath, NULL);
	RETURN_IF_UNSUCCESS(WeUi_Gfx_Path_Rect(pPath, 0.0f, barY, redrawn * pcL->W, barY + pcL->Bar));
	return WeUi_DemoScene_Fill(&paint, NULL);
}

static u32 WeUi_DemoScene_Area(const WeUi_Gfx_ClipRect_t* pcRects, u8 num) {
	u32 area = 0;
	for (u8 i = 0; i < num; i++)
		area += (u32)((pcRects[i].X1 - pcRects[i].X0) * (pcRects[i].Y1 - pcRects[i].Y0));
	return area;
}

/**
 * Each frame redraws only what changed, however the panel takes it - into the frame, or out in
 * bands. The bar under the face shows how much of the panel that was; the log has the average,
 * the time a frame takes to build and draw, and the first frame's - the whole panel.
 */
void WeUi_DrawDemo_Scene(void) {
	WeUi_DemoScene_Layout_t l = WeUi_DemoScene_Layout();
	float panel				  = l.W * l.H;
	u32 redrawn = 0, total = 0, busyMs = 0, wholeMs = 0;
	WeUi_Gfx_Scene_Init(&WeUi_DemoScene_Scene);

	for (u32 frame = 0; frame < WEUI_DEMO_LIVE_FRAMES; frame++) {
		u32 start = WeUi_Gfx_FpsCtrl_GetTickCnt();
		if (WeUi_DemoScene_Build((float)frame / WEUI_DEMO_LIVE_FRAMES, &l,
								 (float)redrawn / panel) != RET_STATE_SUCCESS)
			return;
		u8 n = WeUi_Gfx_Scene_Damage(&WeUi_DemoScene_Scene, WeUi_DemoScene_Rects, DEMO_SCENE_RECTS);
		redrawn = WeUi_DemoScene_Area(WeUi_DemoScene_Rects, n);
		if (WeUi_Gfx_Scene_Present(&WeUi_DemoScene_Scene, WeUi_DemoScene_Rects, n) !=
			RET_STATE_SUCCESS)
			return;

		u32 took = WeUi_Gfx_FpsCtrl_GetTickCnt() - start;
		busyMs += took;
		wholeMs = frame == 0 ? took : wholeMs;
		total += redrawn;
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(WEUI_DEMO_LIVE_FPS);
	}
	LOG("scene: %u%% of the panel redrawn a frame, %u.%u ms a frame, the whole panel %u ms",
		(unsigned)(DEMO_SCENE_PERCENT * total / (WEUI_DEMO_LIVE_FRAMES * (u32)panel)),
		(unsigned)(busyMs / WEUI_DEMO_LIVE_FRAMES),
		(unsigned)(DEMO_SCENE_TENTHS * busyMs / WEUI_DEMO_LIVE_FRAMES % DEMO_SCENE_TENTHS),
		(unsigned)wholeMs);
}

#endif /* WEUI_USE_COMPONENT__DEMO */
