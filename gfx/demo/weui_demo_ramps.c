#include "weui_demo.h"

#if WEUI_USE_COMPONENT__DEMO

#include "weui_demo_pace.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_fps_control.h"

/* Repainted at a crawl: the picture never changes, the frames only keep the buffers in step */
#define DEMO_RAMPS_FPS 5

/* The dark ramp tops out here: it spreads the few dimmest levels over the widest strip, and
 * the dimmest levels are the ones that blink slowest on a sub-frame scanned panel */
#define DEMO_RAMPS_DARK_MAX 48

/* Strip heights, in parts of the panel height: white, red, green, blue, then the dark ramp
 * takes the rest. Tuned for 25 rows: 5, 3, 3, 3, 11 */
#define DEMO_RAMPS_WHITE_PARTS 20
#define DEMO_RAMPS_COLOR_PARTS 12
#define DEMO_RAMPS_PARTS_TOTAL 100

typedef struct {
	u8 R;
	u8 G;
	u8 B;
	u8 Max;
} WeUi_DemoRamps_Strip_t;

static void WeUi_DemoRamps_Paint(void) {
	WeUi_Gfx_ObjSize_t width  = WeUi_Gfx_Adapter_GetWidth();
	WeUi_Gfx_ObjSize_t height = WeUi_Gfx_Adapter_GetHeight();

	const WeUi_DemoRamps_Strip_t strips[] = {
		{1, 1, 1, 0xFF},
		{1, 0, 0, 0xFF},
		{0, 1, 0, 0xFF},
		{0, 0, 1, 0xFF},
		{1, 1, 1, DEMO_RAMPS_DARK_MAX},
	};

	u32 rowStart = 0;
	for (u32 strip = 0; strip < NUM_ELEMENTS(strips); strip++) {
		u32 parts	 = (strip == 0) ? DEMO_RAMPS_WHITE_PARTS : DEMO_RAMPS_COLOR_PARTS;
		bool isLast	 = (strip == NUM_ELEMENTS(strips) - 1);
		u32 rowCount = isLast
						   ? height - rowStart
						   : (height * parts + DEMO_RAMPS_PARTS_TOTAL / 2) / DEMO_RAMPS_PARTS_TOTAL;

		for (u32 y = rowStart; y < rowStart + rowCount && y < height; y++) {
			for (u32 x = 0; x < width; x++) {
				u8 val = (u8)(x * strips[strip].Max / (width - 1));

				WeUi_Gfx_ColorPoint_t cpt = {
					.Pt.X	 = (WeUi_Gfx_PtPos_t)x,
					.Pt.Y	 = (WeUi_Gfx_PtPos_t)y,
					.Color.R = strips[strip].R ? val : 0,
#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
					.Color.G = strips[strip].G ? val : 0,
					.Color.B = strips[strip].B ? val : 0,
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */
#if WEUI_LIB_COLOR_ALPHA_CHANNEL
					.Color.A = 0xFF,
#endif /* WEUI_LIB_COLOR_ALPHA_CHANNEL */
				};
				WeUi_Gfx_Adapter_SetPixel(&cpt);
			}
		}

		rowStart += rowCount;
	}
}

void WeUi_DrawDemo_Ramps(void) {
	for (u16 frame = 0; frame < WEUI_DEMO_SEC * DEMO_RAMPS_FPS; frame++) {
		WeUi_DemoRamps_Paint();
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_RAMPS_FPS);
	}
}

#endif /* WEUI_USE_COMPONENT__DEMO */
