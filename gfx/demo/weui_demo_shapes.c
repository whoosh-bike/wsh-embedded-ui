#include "weui_demo.h"

#if WEUI_USE_COMPONENT__DEMO

#include "weui_demo_pace.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_base.h"
#include "weui_gfx_colors.h"
#include "weui_gfx_fps_control.h"
#include "weui_gfx_transform.h"
#include "weui_rand.h"

static const WeUi_Gfx_ThreeCoord_t CubeNormal[8] = {
	{-1.0f, -1.0f, -1.0f}, {+1.0f, -1.0f, -1.0f}, {+1.0f, +1.0f, -1.0f}, {-1.0f, +1.0f, -1.0f},
	{-1.0f, -1.0f, +1.0f}, {+1.0f, -1.0f, +1.0f}, {+1.0f, +1.0f, +1.0f}, {-1.0f, +1.0f, +1.0f},
};

#define DEMO_CUBE_BRI_LEVELS 3.0f

/* Corner pairs of the twelve edges, corners numbered as in CubeNormal */
static const u8 CubeEdges[][2] = {
	{0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 5}, {2, 6}, {3, 7}, {4, 5}, {5, 6}, {6, 7}, {7, 4},
};

/* Lit by depth: an edge behind the centre is dimmest, one in front brighter, one crossing full */
static float WeUi_DemoCube_GetEdgeBri(float z1, float z2) {
	if (z1 < 0 && z2 < 0)
		return WEUI_COLOR_HSB_MAX_BRI / DEMO_CUBE_BRI_LEVELS;
	if (z1 > 0 && z2 > 0)
		return WEUI_COLOR_HSB_MAX_BRI * (DEMO_CUBE_BRI_LEVELS - 1.0f) / DEMO_CUBE_BRI_LEVELS;
	return WEUI_COLOR_HSB_MAX_BRI;
}

static WeUi_Gfx_Color_t WeUi_DemoCube_GetLineBrightAndColor(float hue, float z1, float z2) {
	hue								   = WEUI_COLOR_CUT_HUE(hue, WEUI_COLOR_HSB_MAX_HUE);
	WeUi_Gfx_ColorSchemeHsb_t colorHsb = WeUi_Color_GetFullHsb(hue);
	colorHsb.Bri					   = WeUi_DemoCube_GetEdgeBri(z1, z2);

	return WeUi_Gfx_Color_FromHsb(&colorHsb);
}

static float WeUi_Gfx_GetHatching(float z1, float z2) {
	if (z1 < 0 && z2 < 0)
		return 0.2f;
	else if (z1 > 0 && z2 > 0)
		return 1.0f;
	else
		return 0.5f;
}

static WeUi_Gfx_Color_t WeUi_Gfx_GetRandColor(void) {
	WeUi_Gfx_ColorSchemeHsb_t hsbColor = WeUi_Color_GetFullHsb(WeUi_Rand_Hue());

	return WeUi_Gfx_Color_FromHsb(&hsbColor);
}

static void WeUi_DemoCube_SetEdge(WeUi_Gfx_Line_t* pLine, const WeUi_Gfx_ThreeCoord_t* pcA,
								  const WeUi_Gfx_ThreeCoord_t* pcB) {
	pLine->Base.PtOne.X = (WeUi_Gfx_PtPos_t)pcA->X;
	pLine->Base.PtOne.Y = (WeUi_Gfx_PtPos_t)pcA->Y;
	pLine->Base.PtTwo.X = (WeUi_Gfx_PtPos_t)pcB->X;
	pLine->Base.PtTwo.Y = (WeUi_Gfx_PtPos_t)pcB->Y;
}

static void WeUi_Gfx_DrawCube(float hue, WeUi_Gfx_ThreeCoord_t cube[]) {
	WeUi_Gfx_Line_t line = {
		.Hatching = 1.0f,
	};

	for (u32 i = 0; i < NUM_ELEMENTS(CubeEdges); i++) {
		const WeUi_Gfx_ThreeCoord_t* pcA = &cube[CubeEdges[i][0]];
		const WeUi_Gfx_ThreeCoord_t* pcB = &cube[CubeEdges[i][1]];

		WeUi_DemoCube_SetEdge(&line, pcA, pcB);
		line.Base.Color = WeUi_DemoCube_GetLineBrightAndColor(hue, pcA->Z, pcB->Z);
		line.Hatching	= WeUi_Gfx_GetHatching(pcA->Z, pcB->Z);
		WeUi_Gfx_Line_Draw(&line);
	}
}

static void TransformCube(WeUi_Gfx_ThreeCoord_t out[8], const WeUi_Gfx_ThreeCoord_t in[8],
						  WeUi_Gfx_ThreeCoord_t rotation, WeUi_Gfx_ThreeCoord_t scale,
						  WeUi_Gfx_ThreeCoord_t offset) {
	for (u32 i = 0; i < 8; i++) {
		out[i] = in[i];
		out[i] = WeUi_Gfx_Transform_Rotate(&out[i], (s16)rotation.X, AXE_X);
		out[i] = WeUi_Gfx_Transform_Rotate(&out[i], (s16)rotation.Y, AXE_Y);
		out[i] = WeUi_Gfx_Transform_Rotate(&out[i], (s16)rotation.Z, AXE_Z);
		out[i] = WeUi_Gfx_Transform_Scale(&out[i], &scale);
		out[i] = WeUi_Gfx_Transform_Offset(&out[i], &offset);
	}
}

static void RenderCube(float* pHue, WeUi_Gfx_ThreeCoord_t cube[8]) {
	WeUi_Gfx_Adapter_ClearDrawingFrame();
	WeUi_Gfx_DrawCube((*pHue)++, cube);
	WeUi_Gfx_FpsCtrl_DrawAndKeepFPS_MainFreq();
}

#define DEMO_PRIMITIVES_FPS	   50
#define DEMO_PRIMITIVES_SHAPES 7 /* frames a round: one per kind of primitive */
#define DEMO_PRIMITIVES_ROUNDS (WEUI_DEMO_SEC * DEMO_PRIMITIVES_FPS / DEMO_PRIMITIVES_SHAPES)

void WeUi_DrawDemo_Primitives(void) {
	WeUi_Gfx_Base_t base;
	WeUi_Gfx_Line_t line;
	WeUi_Gfx_Circle_t circle;
	s32 rngX0 = WeUi_Gfx_Adapter_GetX_Min(), rngX1 = WeUi_Gfx_Adapter_GetX_Max();
	s32 rngY0 = WeUi_Gfx_Adapter_GetY_Min(), rngY1 = WeUi_Gfx_Adapter_GetY_Max();

	WeUi_Gfx_Base_InitStruct(&base);
	WeUi_Gfx_Line_InitStruct(&line);
	WeUi_Gfx_Circle_InitStruct(&circle);

	WeUi_Gfx_Adapter_ClearDrawingFrame();
	for (u32 i = 0; i < DEMO_PRIMITIVES_ROUNDS; i++) {
		line.Base.PtOne.X = WeUi_Rand_Between(rngX0, rngX1);
		line.Base.PtOne.Y = WeUi_Rand_Between(rngY0, rngY1);
		line.Base.Color	  = WeUi_Gfx_GetRandColor();
		line.Hatching	  = (float)WeUi_Rand_Between(0, 10) / 10.0f;
		u32 angle		  = WeUi_Rand_Between(0, 359);
		u32 len			  = WeUi_Rand_Between(5, 50);
		WeUi_Gfx_LineAngle_Draw(&line, angle, len);
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_PRIMITIVES_FPS);

		WeUi_Gfx_Adapter_CopyFrame_DisplayToDrawing();
		base.PtOne.X = WeUi_Rand_Between(rngX0, rngX1);
		base.PtOne.Y = WeUi_Rand_Between(rngY0, rngY1);
		base.PtTwo.X = WeUi_Rand_Between(rngX0, rngX1);
		base.PtTwo.Y = WeUi_Rand_Between(rngY0, rngY1);
		base.Color	 = WeUi_Gfx_GetRandColor();
		WeUi_Gfx_RectVertices_Draw(&base);
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_PRIMITIVES_FPS);

		WeUi_Gfx_Adapter_CopyFrame_DisplayToDrawing();
		base.PtOne.X = WeUi_Rand_Between(rngX0, rngX1);
		base.PtOne.Y = WeUi_Rand_Between(rngY0, rngY1);
		base.PtTwo.X = WeUi_Rand_Between(rngX0, rngX1);
		base.PtTwo.Y = WeUi_Rand_Between(rngY0, rngY1);
		base.Color	 = WeUi_Gfx_GetRandColor();
		WeUi_Gfx_RectVerticesFill_Draw(&base);
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_PRIMITIVES_FPS);

		WeUi_Gfx_Adapter_CopyFrame_DisplayToDrawing();
		circle.Base.PtOne.X = WeUi_Rand_Between(rngX0, rngX1);
		circle.Base.PtOne.Y = WeUi_Rand_Between(rngY0, rngY1);
		circle.Radius		= WeUi_Rand_Between(0, 10);
		circle.Base.Color	= WeUi_Gfx_GetRandColor();
		WeUi_Gfx_Circle_Draw(&circle, false);
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_PRIMITIVES_FPS);

		WeUi_Gfx_Adapter_CopyFrame_DisplayToDrawing();

		circle.Base.PtOne.X = WeUi_Rand_Between(rngX0, rngX1);
		circle.Base.PtOne.Y = WeUi_Rand_Between(rngY0, rngY1);
		circle.Radius		= WeUi_Rand_Between(0, 10);
		circle.Base.Color	= WeUi_Gfx_GetRandColor();
		WeUi_Gfx_Circle_Draw(&circle, true);
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_PRIMITIVES_FPS);

		WeUi_Gfx_Adapter_CopyFrame_DisplayToDrawing();
		base.PtOne.X = WeUi_Rand_Between(rngX0, rngX1);
		base.PtOne.Y = WeUi_Rand_Between(rngY0, rngY1);
		base.Width =
			WeUi_Rand_Between(WeUi_Gfx_Adapter_GetX_Min(), WeUi_Gfx_Adapter_GetX_Max() / 4);
		base.Height =
			WeUi_Rand_Between(WeUi_Gfx_Adapter_GetY_Min(), WeUi_Gfx_Adapter_GetY_Max() / 4);
		base.Color = WeUi_Gfx_GetRandColor();
		WeUi_Gfx_RectEdges_Draw(&base);
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_PRIMITIVES_FPS);

		WeUi_Gfx_Adapter_CopyFrame_DisplayToDrawing();
		base.PtOne.X = WeUi_Rand_Between(rngX0, rngX1);
		base.PtOne.Y = WeUi_Rand_Between(rngY0, rngY1);
		base.Width =
			WeUi_Rand_Between(WeUi_Gfx_Adapter_GetX_Min(), WeUi_Gfx_Adapter_GetX_Max() / 4);
		base.Height =
			WeUi_Rand_Between(WeUi_Gfx_Adapter_GetY_Min(), WeUi_Gfx_Adapter_GetY_Max() / 4);
		base.Color = WeUi_Gfx_GetRandColor();
		WeUi_Gfx_RectEdgesFill_Draw(&base);
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_PRIMITIVES_FPS);

		// WeUi_Gfx_Adapter_ClearDrawingFrame();
	}
}
/* The closing growth swells the cube well past the panel - ten shrink phases'
 * worth of scale - but in two phases' worth of steps: the same size, far less
 * spinning on the spot. The shrink back down keeps that pace */
#define DEMO_CUBE_GROWTH_SCALE	10.0f
#define DEMO_CUBE_GROWTH_PHASES 2.0f

void WeUi_DrawDemo_CubeRotation(void) {
	WeUi_Gfx_ThreeCoord_t cubeRSO[8];
	float hue = WEUI_COLOR_HSB_MIN_HUE;

	const u32 dispW		= WeUi_Gfx_Adapter_GetWidth();
	const u32 halfDispW = WeUi_Gfx_Adapter_GetX_Mid();
	const u32 dispH		= WeUi_Gfx_Adapter_GetHeight();
	const u32 halfDispH = WeUi_Gfx_Adapter_GetY_Mid();

	float baseScale = (dispW < dispH ? dispW : dispH) * 0.25f;
	float minScale	= baseScale / 2.5f;

	WeUi_Gfx_ThreeCoord_t rotation = {0};

	WeUi_Gfx_ThreeCoord_t scale = {
		baseScale,
		baseScale,
		baseScale,
	};
	WeUi_Gfx_ThreeCoord_t offset = {
		.X = halfDispW,
		.Y = halfDispH,
		.Z = 0.0f,
	};

	float maxCnt  = 10;
	float stepCnt = maxCnt / 30.0f;

	for (u32 i = 0; i < (u32)(maxCnt / stepCnt); i++) {
		rotation.X += 1;
		rotation.Y += 2;
		rotation.Z += 3;

		TransformCube(cubeRSO, CubeNormal, rotation, scale, offset);
		RenderCube(&hue, cubeRSO);
	}

	for (u32 i = 0; i < (u32)(maxCnt / stepCnt); i++) {
		float cnt = (float)i * stepCnt;
		rotation.X += 1;
		rotation.Y += 2;
		rotation.Z += 3;

		offset.X = exp_curve(cnt, 0.0f, maxCnt, halfDispW, dispW, 0.3f);

		TransformCube(cubeRSO, CubeNormal, rotation, scale, offset);
		RenderCube(&hue, cubeRSO);
	}

	float scaleStep = (baseScale - minScale) / (maxCnt / stepCnt);

	for (u32 i = 0; i < (u32)(maxCnt / stepCnt); i++) {
		float cnt = (float)i * stepCnt;
		rotation.X -= 3;
		rotation.Y -= 2;
		rotation.Z -= 1;

		scale.X -= scaleStep;
		scale.Y -= scaleStep;
		scale.Z -= scaleStep;

		offset.X = exp_curve(cnt, 0.0f, maxCnt, dispW, 0.0f, -0.07f);

		TransformCube(cubeRSO, CubeNormal, rotation, scale, offset);
		RenderCube(&hue, cubeRSO);
	}

	offset.X		  = 0.0f;
	float xSinMax	  = 6.0f;
	float xSinStep	  = 0.05f;
	float xOffsetStep = halfDispW / (xSinMax / xSinStep);

	for (u32 i = 0; i < (u32)(xSinMax / xSinStep); i++) {
		float xSin = (float)i * xSinStep;
		rotation.X += 1;
		rotation.Y += 2;
		rotation.Z += 3;

		offset.Y = dispH * SHIFT_AND_NORM(shaped_sine(xSin, 3.0f));
		offset.X += xOffsetStep;

		TransformCube(cubeRSO, CubeNormal, rotation, scale, offset);
		RenderCube(&hue, cubeRSO);
	}

	float growStep = scaleStep * DEMO_CUBE_GROWTH_SCALE / DEMO_CUBE_GROWTH_PHASES;

	for (u32 i = 0; i < (u32)(maxCnt * DEMO_CUBE_GROWTH_PHASES / stepCnt); i++) {
		rotation.X -= 3;
		rotation.Y -= 2;
		rotation.Z -= 1;

		scale.X += growStep;
		scale.Y += growStep;
		scale.Z += growStep;

		TransformCube(cubeRSO, CubeNormal, rotation, scale, offset);
		RenderCube(&hue, cubeRSO);
	}

	rotation.X = 0;
	rotation.Y = 0;
	rotation.Z = 0;
	while (scale.X > 2.0f) {
		rotation.X += 1;
		rotation.Y += 2;
		rotation.Z += 3;

		scale.X -= growStep;
		scale.Y -= growStep;
		scale.Z -= growStep;

		TransformCube(cubeRSO, CubeNormal, rotation, scale, offset);
		RenderCube(&hue, cubeRSO);
	}
}

#endif /* WEUI_USE_COMPONENT__DEMO */
