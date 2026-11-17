#include "weui_demo.h"

#if WEUI_USE_COMPONENT__DEMO

#include "mathlib_common.h"
#include "weui_demo_pace.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_base.h"
#include "weui_gfx_colors.h"
#include "weui_gfx_effects.h"
#include "weui_gfx_fps_control.h"
#include "weui_rand.h"

#define WEUI_DEMO_FIELDS_FPS 30

/* Ways in, by kind: a push travels further than a swipe, and a dissolve reads in less.
 * Which segment comes in which way is the reel below */
#define WEUI_DEMO_FIELDS_DISSOLVE_MS 650
#define WEUI_DEMO_FIELDS_PUSH_MS	 1050
#define WEUI_DEMO_FIELDS_SWIPE_MS	 800
#define WEUI_DEMO_FIELDS_SEGMENT_MS	 (WEUI_DEMO_SEC * 1000) /* the way in included */

/* perlin_3D truncates towards zero, so every sampled coordinate is kept positive */
#define WEUI_DEMO_FIELDS_ORIGIN		 50.0f
#define WEUI_DEMO_FIELDS_FBM_OCTAVES 2

#define WEUI_DEMO_NEBULA_SCALE_X 0.05f
#define WEUI_DEMO_NEBULA_SCALE_Y 0.10f
#define WEUI_DEMO_NEBULA_WARP	 1.6f /* noise units the field is bent by its own noise */
#define WEUI_DEMO_NEBULA_MORPH	 0.30f
#define WEUI_DEMO_NEBULA_DRIFT	 0.40f
#define WEUI_DEMO_NEBULA_GAIN	 1.6f
#define WEUI_DEMO_NEBULA_SEED_X	 5.2f /* the two warp fields must not be the same noise */
#define WEUI_DEMO_NEBULA_SEED_Y	 1.3f

#define WEUI_DEMO_FIRE_SCALE_X 0.16f
#define WEUI_DEMO_FIRE_SCALE_Y 0.20f
#define WEUI_DEMO_FIRE_RISE	   4.0f /* noise units per s, upwards */
#define WEUI_DEMO_FIRE_CHURN   0.9f
#define WEUI_DEMO_FIRE_FEED	   1.5f /* heat gained from the top row to the bottom one */
#define WEUI_DEMO_FIRE_COOL	   0.45f
#define WEUI_DEMO_FIRE_FLICKER 1.1f

#define WEUI_DEMO_OCEAN_SWELL_X		 0.05f
#define WEUI_DEMO_OCEAN_SWELL_Y		 0.10f
#define WEUI_DEMO_OCEAN_SWELL_T		 0.25f
#define WEUI_DEMO_OCEAN_RIPPLE_X	 0.11f
#define WEUI_DEMO_OCEAN_RIPPLE_Y	 0.22f
#define WEUI_DEMO_OCEAN_RIPPLE_T	 0.45f
#define WEUI_DEMO_OCEAN_DRIFT		 0.6f
#define WEUI_DEMO_OCEAN_GAIN		 1.8f
#define WEUI_DEMO_OCEAN_SHARPNESS	 6.0f /* higher draws the caustics thinner */
#define WEUI_DEMO_OCEAN_BASE		 0.30f
#define WEUI_DEMO_OCEAN_SWELL_SPAN	 0.30f
#define WEUI_DEMO_OCEAN_CAUSTIC_SPAN 0.60f

#define WEUI_DEMO_AURORA_EDGE_MID	  0.70f /* of the height, where the curtain hems */
#define WEUI_DEMO_AURORA_EDGE_SWAY	  0.45f
#define WEUI_DEMO_AURORA_EDGE_SCALE	  0.035f
#define WEUI_DEMO_AURORA_EDGE_SPEED	  0.30f
#define WEUI_DEMO_AURORA_RAY_SCALE	  0.25f
#define WEUI_DEMO_AURORA_RAY_DRIFT	  1.2f
#define WEUI_DEMO_AURORA_RAY_FLICKER  0.9f
#define WEUI_DEMO_AURORA_RAY_GAIN	  1.6f
#define WEUI_DEMO_AURORA_RAY_FLOOR	  0.25f
#define WEUI_DEMO_AURORA_RISE		  0.35f /* of the height, the curtain's fade upwards */
#define WEUI_DEMO_AURORA_HEM_PX		  1.5f
#define WEUI_DEMO_AURORA_COLOR_SPAN	  0.8f
#define WEUI_DEMO_AURORA_STAR_RARITY  23u
#define WEUI_DEMO_AURORA_STAR_BRI	  0.7f
#define WEUI_DEMO_AURORA_STAR_TWINKLE 5.0f /* rad per s */

#define WEUI_DEMO_TOXIC_SCALE_X	  0.08f
#define WEUI_DEMO_TOXIC_SCALE_Y	  0.16f
#define WEUI_DEMO_TOXIC_FLOW_X	  1.2f
#define WEUI_DEMO_TOXIC_FLOW_Y	  0.6f
#define WEUI_DEMO_TOXIC_MORPH	  0.8f
#define WEUI_DEMO_TOXIC_GAIN	  1.4f
#define WEUI_DEMO_TOXIC_SHARPNESS 3.0f /* higher draws the veins thinner */

#define WEUI_DEMO_TOXIC_HUE_SPIN 120.0f /* degrees per s */

#define WEUI_DEMO_SWEEP_TURN	  1.4f	/* rad per s */
#define WEUI_DEMO_SWEEP_SPAN	  70.0f /* px per palette round */
#define WEUI_DEMO_SWEEP_SCROLL	  0.6f	/* palette rounds per s */
#define WEUI_DEMO_SWEEP_Y_STRETCH 2.0f	/* the panel is short: a vertical sweep would barely move */

#define WEUI_DEMO_SPIRAL_ORBIT_X  0.30f /* of the width */
#define WEUI_DEMO_SPIRAL_ORBIT_Y  0.25f
#define WEUI_DEMO_SPIRAL_ORBIT_WX 0.9f /* rad per s */
#define WEUI_DEMO_SPIRAL_ORBIT_WY 1.7f
#define WEUI_DEMO_SPIRAL_RING	  0.08f /* rounds per px of radius */
#define WEUI_DEMO_SPIRAL_ARMS	  3.0f
#define WEUI_DEMO_SPIRAL_SPEED	  1.5f	/* rounds per s */
#define WEUI_DEMO_SPIRAL_HUE_SPIN 90.0f /* degrees per s */
#define WEUI_DEMO_SPIRAL_DEPTH	  0.6f	/* of the height, the dark well at the centre */

typedef struct {
	float Sec;
	float Width;
	float Height;
} WeUi_DemoFields_Frame_t;

typedef WeUi_ColorSchemeRgb_t (*WeUi_DemoFields_PixelFn_t)(float x, float y,
														   const WeUi_DemoFields_Frame_t* pcFrame);

typedef void (*WeUi_DemoFields_PaintFn_t)(float sec);

typedef enum {
	WEUI_DEMO_FIELDS_IN_DISSOLVE = 0,
	WEUI_DEMO_FIELDS_IN_PUSH_LEFT,
	WEUI_DEMO_FIELDS_IN_PUSH_UP,
	WEUI_DEMO_FIELDS_IN_PUSH_DOWN,
	WEUI_DEMO_FIELDS_IN_SWIPE_LEFT,
	WEUI_DEMO_FIELDS_IN_SWIPE_RIGHT,
} WeUi_DemoFields_In_t;

typedef struct {
	WeUi_DemoFields_PaintFn_t Paint;
	u32 Ms; /* the way in included */
	WeUi_DemoFields_In_t In;
} WeUi_DemoFields_Segment_t;

/** @brief Stops spread evenly over [0; 1]; a palette sampled cyclically repeats its first last */
typedef struct {
	const WeUi_ColorSchemeRgb_t* Stops;
	u32 StopsNum;
} WeUi_DemoFields_Palette_t;

#define WEUI_DEMO_FIELDS_PALETTE(stops) {.Stops = (stops), .StopsNum = NUM_ELEMENTS(stops)}

static const WeUi_ColorSchemeRgb_t WeUi_DemoFields_NebulaStops[] = {
	{0.00f, 0.00f, 0.05f}, {0.25f, 0.00f, 0.45f}, {0.85f, 0.10f, 0.55f},
	{1.00f, 0.55f, 0.40f}, {1.00f, 0.90f, 0.80f},
};

static const WeUi_ColorSchemeRgb_t WeUi_DemoFields_ToxicStops[] = {
	{0.00f, 0.02f, 0.02f}, {0.00f, 0.25f, 0.20f}, {0.10f, 0.70f, 0.20f},
	{0.70f, 1.00f, 0.10f}, {1.00f, 1.00f, 0.70f},
};

static const WeUi_ColorSchemeRgb_t WeUi_DemoFields_SunsetStops[] = {
	{0.25f, 0.00f, 0.45f}, {0.90f, 0.05f, 0.50f}, {1.00f, 0.40f, 0.00f},
	{1.00f, 0.85f, 0.20f}, {0.25f, 0.00f, 0.45f},
};

static const WeUi_ColorSchemeRgb_t WeUi_DemoFields_FireStops[] = {
	{0.00f, 0.00f, 0.00f}, {0.45f, 0.00f, 0.00f}, {1.00f, 0.20f, 0.00f},
	{1.00f, 0.65f, 0.05f}, {1.00f, 1.00f, 0.60f},
};

static const WeUi_ColorSchemeRgb_t WeUi_DemoFields_OceanStops[] = {
	{0.00f, 0.02f, 0.12f}, {0.00f, 0.12f, 0.40f}, {0.00f, 0.45f, 0.70f},
	{0.30f, 0.85f, 0.90f}, {0.90f, 1.00f, 1.00f},
};

static const WeUi_ColorSchemeRgb_t WeUi_DemoFields_AuroraStops[] = {
	{0.10f, 1.00f, 0.40f},
	{0.00f, 0.80f, 0.70f},
	{0.30f, 0.25f, 0.95f},
	{0.60f, 0.10f, 0.60f},
};

static const WeUi_DemoFields_Palette_t WeUi_DemoFields_Fire =
	WEUI_DEMO_FIELDS_PALETTE(WeUi_DemoFields_FireStops);
static const WeUi_DemoFields_Palette_t WeUi_DemoFields_Ocean =
	WEUI_DEMO_FIELDS_PALETTE(WeUi_DemoFields_OceanStops);
static const WeUi_DemoFields_Palette_t WeUi_DemoFields_Aurora =
	WEUI_DEMO_FIELDS_PALETTE(WeUi_DemoFields_AuroraStops);
static const WeUi_DemoFields_Palette_t WeUi_DemoFields_Sunset =
	WEUI_DEMO_FIELDS_PALETTE(WeUi_DemoFields_SunsetStops);
static const WeUi_DemoFields_Palette_t WeUi_DemoFields_Nebula =
	WEUI_DEMO_FIELDS_PALETTE(WeUi_DemoFields_NebulaStops);
static const WeUi_DemoFields_Palette_t WeUi_DemoFields_Toxic =
	WEUI_DEMO_FIELDS_PALETTE(WeUi_DemoFields_ToxicStops);

static const WeUi_ColorSchemeRgb_t WeUi_DemoFields_NightTop	   = {0.00f, 0.00f, 0.03f};
static const WeUi_ColorSchemeRgb_t WeUi_DemoFields_NightBottom = {0.02f, 0.03f, 0.10f};

static float WeUi_DemoFields_AuroraHem[WEUI_DISPLAY_WIDTH];
static float WeUi_DemoFields_AuroraRay[WEUI_DISPLAY_WIDTH];

static WeUi_DemoFields_PaintFn_t WeUi_DemoFields_Paint;
/* Wall clock, not frames: a heavy field that cannot keep the rate would otherwise
 * run long and in slow motion */
static u32 WeUi_DemoFields_StartMs;

static float WeUi_DemoFields_Frac(float value) {
	return value - floorf(value);
}

static float WeUi_DemoFields_Fbm(float x, float y, float z) {
	float sum  = 0.0f;
	float amp  = 1.0f;
	float norm = 0.0f;

	for (u32 octave = 0; octave < WEUI_DEMO_FIELDS_FBM_OCTAVES; octave++) {
		sum += amp * perlin_3D(x, y, z);
		norm += amp;
		amp *= 0.5f;
		x *= 2.0f;
		y *= 2.0f;
		z *= 2.0f;
	}

	return sum / norm;
}

static WeUi_ColorSchemeRgb_t WeUi_DemoFields_Mix(const WeUi_ColorSchemeRgb_t* pcA,
												 const WeUi_ColorSchemeRgb_t* pcB, float mix) {
	return (WeUi_ColorSchemeRgb_t){
		.Red   = lerp(pcA->Red, pcB->Red, mix),
		.Green = lerp(pcA->Green, pcB->Green, mix),
		.Blue  = lerp(pcA->Blue, pcB->Blue, mix),
	};
}

static WeUi_ColorSchemeRgb_t WeUi_DemoFields_Scale(WeUi_ColorSchemeRgb_t rgb, float scale) {
	return (WeUi_ColorSchemeRgb_t){
		.Red   = rgb.Red * scale,
		.Green = rgb.Green * scale,
		.Blue  = rgb.Blue * scale,
	};
}

static WeUi_ColorSchemeRgb_t WeUi_DemoFields_Palette_At(const WeUi_DemoFields_Palette_t* pcPal,
														float pos) {
	float span = CUT_NORM_FLOAT(pos) * (float)(pcPal->StopsNum - 1);
	u32 idx	   = (u32)span;

	if (idx >= pcPal->StopsNum - 1)
		return pcPal->Stops[pcPal->StopsNum - 1];

	return WeUi_DemoFields_Mix(&pcPal->Stops[idx], &pcPal->Stops[idx + 1], span - (float)idx);
}

static void WeUi_DemoFields_Fill(WeUi_DemoFields_PixelFn_t pixel, float sec) {
	/* The aurora's column buffers are sized from the config, the panel reports its own size */
	WeUi_Gfx_ObjSize_t width  = GET_MIN(WeUi_Gfx_Adapter_GetWidth(), WEUI_DISPLAY_WIDTH);
	WeUi_Gfx_ObjSize_t height = WeUi_Gfx_Adapter_GetHeight();

	const WeUi_DemoFields_Frame_t frame = {
		.Sec	= sec,
		.Width	= (float)width,
		.Height = (float)height,
	};

	for (WeUi_Gfx_PtPos_t x = 0; x < (WeUi_Gfx_PtPos_t)width; x++) {
		for (WeUi_Gfx_PtPos_t y = 0; y < (WeUi_Gfx_PtPos_t)height; y++) {
			WeUi_ColorSchemeRgb_t rgb = pixel((float)x, (float)y, &frame);

			WeUi_Gfx_ColorPoint_t cpt = {
				.Pt.X  = x,
				.Pt.Y  = y,
				.Color = WeUi_Gfx_Color_FromRgb(&rgb),
			};

			WeUi_Gfx_Point_Draw(&cpt);
		}
	}
}

/* Sampled where two other noise fields push it: the bends read as gas, not as cloud */
static WeUi_ColorSchemeRgb_t WeUi_DemoFields_NebulaPixel(float x, float y,
														 const WeUi_DemoFields_Frame_t* pcFrame) {
	float nx	= x * WEUI_DEMO_NEBULA_SCALE_X + WEUI_DEMO_FIELDS_ORIGIN;
	float ny	= y * WEUI_DEMO_NEBULA_SCALE_Y + WEUI_DEMO_FIELDS_ORIGIN;
	float morph = pcFrame->Sec * WEUI_DEMO_NEBULA_MORPH + WEUI_DEMO_FIELDS_ORIGIN;

	float warpX = WeUi_DemoFields_Fbm(nx, ny, morph);
	float warpY =
		WeUi_DemoFields_Fbm(nx + WEUI_DEMO_NEBULA_SEED_X, ny + WEUI_DEMO_NEBULA_SEED_Y, morph);
	float noise = WeUi_DemoFields_Fbm(nx + WEUI_DEMO_NEBULA_WARP * warpX +
										  pcFrame->Sec * WEUI_DEMO_NEBULA_DRIFT,
									  ny + WEUI_DEMO_NEBULA_WARP * warpY, morph);

	return WeUi_DemoFields_Palette_At(&WeUi_DemoFields_Nebula,
									  SHIFT_AND_NORM(noise * WEUI_DEMO_NEBULA_GAIN));
}

static WeUi_ColorSchemeRgb_t WeUi_DemoFields_FirePixel(float x, float y,
													   const WeUi_DemoFields_Frame_t* pcFrame) {
	float noise = WeUi_DemoFields_Fbm(
		x * WEUI_DEMO_FIRE_SCALE_X + WEUI_DEMO_FIELDS_ORIGIN,
		y * WEUI_DEMO_FIRE_SCALE_Y + pcFrame->Sec * WEUI_DEMO_FIRE_RISE + WEUI_DEMO_FIELDS_ORIGIN,
		pcFrame->Sec * WEUI_DEMO_FIRE_CHURN + WEUI_DEMO_FIELDS_ORIGIN);
	float depth = y / (pcFrame->Height - 1.0f);

	return WeUi_DemoFields_Palette_At(&WeUi_DemoFields_Fire, depth * WEUI_DEMO_FIRE_FEED -
																 WEUI_DEMO_FIRE_COOL +
																 noise * WEUI_DEMO_FIRE_FLICKER);
}

static WeUi_ColorSchemeRgb_t WeUi_DemoFields_OceanPixel(float x, float y,
														const WeUi_DemoFields_Frame_t* pcFrame) {
	float drift = pcFrame->Sec * WEUI_DEMO_OCEAN_DRIFT + WEUI_DEMO_FIELDS_ORIGIN;

	float swell = perlin_3D(x * WEUI_DEMO_OCEAN_SWELL_X + drift,
							y * WEUI_DEMO_OCEAN_SWELL_Y + WEUI_DEMO_FIELDS_ORIGIN,
							pcFrame->Sec * WEUI_DEMO_OCEAN_SWELL_T + WEUI_DEMO_FIELDS_ORIGIN);
	float ripple =
		WeUi_DemoFields_Fbm(x * WEUI_DEMO_OCEAN_RIPPLE_X + WEUI_DEMO_FIELDS_ORIGIN,
							y * WEUI_DEMO_OCEAN_RIPPLE_Y + drift,
							pcFrame->Sec * WEUI_DEMO_OCEAN_RIPPLE_T + WEUI_DEMO_FIELDS_ORIGIN);

	/* Lit where the ripple crosses zero: a ridge, thin as the power is high */
	float caustic = powf(CUT_NORM_FLOAT(1.0f - fabsf(ripple * WEUI_DEMO_OCEAN_GAIN)),
						 WEUI_DEMO_OCEAN_SHARPNESS);

	return WeUi_DemoFields_Palette_At(&WeUi_DemoFields_Ocean,
									  WEUI_DEMO_OCEAN_BASE + WEUI_DEMO_OCEAN_SWELL_SPAN * swell +
										  WEUI_DEMO_OCEAN_CAUSTIC_SPAN * caustic);
}

static WeUi_ColorSchemeRgb_t WeUi_DemoFields_AuroraPixel(float x, float y,
														 const WeUi_DemoFields_Frame_t* pcFrame) {
	u32 col		= (u32)x;
	float above = (WeUi_DemoFields_AuroraHem[col] - y) / pcFrame->Height;
	float glow	= (above >= 0.0f) ? expf(-above / WEUI_DEMO_AURORA_RISE)
								  : expf(above * pcFrame->Height / WEUI_DEMO_AURORA_HEM_PX);

	glow *= WEUI_DEMO_AURORA_RAY_FLOOR +
			(1.0f - WEUI_DEMO_AURORA_RAY_FLOOR) * WeUi_DemoFields_AuroraRay[col];

	WeUi_ColorSchemeRgb_t sky = WeUi_DemoFields_Mix(
		&WeUi_DemoFields_NightTop, &WeUi_DemoFields_NightBottom, y / (pcFrame->Height - 1.0f));
	WeUi_ColorSchemeRgb_t curtain = WeUi_DemoFields_Scale(
		WeUi_DemoFields_Palette_At(&WeUi_DemoFields_Aurora, above / WEUI_DEMO_AURORA_COLOR_SPAN),
		glow);

	u32 hash = WeUi_Rand_Hash(col, (u32)y, 0);
	if (hash % WEUI_DEMO_AURORA_STAR_RARITY == 0) {
		float twinkle = SHIFT_AND_NORM(
			sinf(pcFrame->Sec * WEUI_DEMO_AURORA_STAR_TWINKLE + (float)(hash >> 16)));
		float star = WEUI_DEMO_AURORA_STAR_BRI * twinkle * (1.0f - CUT_NORM_FLOAT(glow));

		sky.Red += star;
		sky.Green += star;
		sky.Blue += star;
	}

	return (WeUi_ColorSchemeRgb_t){
		.Red   = sky.Red + curtain.Red,
		.Green = sky.Green + curtain.Green,
		.Blue  = sky.Blue + curtain.Blue,
	};
}

/* Per frame, not per pixel: one curtain is a column-wise shape */
static void WeUi_DemoFields_AuroraPrepare(float sec) {
	float height			 = (float)WeUi_Gfx_Adapter_GetHeight();
	WeUi_Gfx_ObjSize_t width = GET_MIN(WeUi_Gfx_Adapter_GetWidth(), WEUI_DISPLAY_WIDTH);

	for (WeUi_Gfx_ObjSize_t x = 0; x < width; x++) {
		float hem = perlin_3D((float)x * WEUI_DEMO_AURORA_EDGE_SCALE + WEUI_DEMO_FIELDS_ORIGIN,
							  sec * WEUI_DEMO_AURORA_EDGE_SPEED + WEUI_DEMO_FIELDS_ORIGIN,
							  WEUI_DEMO_FIELDS_ORIGIN);
		float ray = perlin_3D((float)x * WEUI_DEMO_AURORA_RAY_SCALE +
								  sec * WEUI_DEMO_AURORA_RAY_DRIFT + WEUI_DEMO_FIELDS_ORIGIN,
							  sec * WEUI_DEMO_AURORA_RAY_FLICKER + WEUI_DEMO_FIELDS_ORIGIN,
							  WEUI_DEMO_FIELDS_ORIGIN);

		WeUi_DemoFields_AuroraHem[x] =
			height * (WEUI_DEMO_AURORA_EDGE_MID + WEUI_DEMO_AURORA_EDGE_SWAY * hem);
		WeUi_DemoFields_AuroraRay[x] =
			CUT_NORM_FLOAT(SHIFT_AND_NORM(WEUI_DEMO_AURORA_RAY_GAIN * ray));
	}
}

static WeUi_ColorSchemeRgb_t WeUi_DemoFields_SweepPixel(float x, float y,
														const WeUi_DemoFields_Frame_t* pcFrame) {
	float angle = pcFrame->Sec * WEUI_DEMO_SWEEP_TURN;
	float along = (x - pcFrame->Width / 2.0f) * cosf(angle) +
				  (y - pcFrame->Height / 2.0f) * WEUI_DEMO_SWEEP_Y_STRETCH * sinf(angle);

	return WeUi_DemoFields_Palette_At(
		&WeUi_DemoFields_Sunset,
		WeUi_DemoFields_Frac(along / WEUI_DEMO_SWEEP_SPAN + pcFrame->Sec * WEUI_DEMO_SWEEP_SCROLL));
}

static WeUi_ColorSchemeRgb_t WeUi_DemoFields_SpiralPixel(float x, float y,
														 const WeUi_DemoFields_Frame_t* pcFrame) {
	float sec = pcFrame->Sec;
	float dx  = x - pcFrame->Width *
						(0.5f + WEUI_DEMO_SPIRAL_ORBIT_X * sinf(sec * WEUI_DEMO_SPIRAL_ORBIT_WX));
	float dy  = y - pcFrame->Height *
						(0.5f + WEUI_DEMO_SPIRAL_ORBIT_Y * sinf(sec * WEUI_DEMO_SPIRAL_ORBIT_WY));
	float radius = sqrtf(dx * dx + dy * dy);

	float round = radius * WEUI_DEMO_SPIRAL_RING - sec * WEUI_DEMO_SPIRAL_SPEED +
				  atan2f(dy, dx) * WEUI_DEMO_SPIRAL_ARMS / (2.0f * WEUI_PI_F);

	WeUi_ColorSchemeHsb_t hsb = {
		.Hue = WeUi_Color_HueIncDec(WeUi_DemoFields_Frac(round) * WEUI_COLOR_HSB_FULL_TURN,
									sec * WEUI_DEMO_SPIRAL_HUE_SPIN),
		.Sat = WEUI_COLOR_HSB_MAX_SAT,
		.Bri = WEUI_COLOR_HSB_MAX_BRI *
			   CUT_NORM_FLOAT(radius / (pcFrame->Height * WEUI_DEMO_SPIRAL_DEPTH)),
	};

	WeUi_ColorSchemeRgb_t rgb;
	WeUi_Color_Hsb2Rgb(&hsb, &rgb);

	return rgb;
}

/* Lit along the noise's zero crossings, both octaves folded: veins, not blobs */
static WeUi_ColorSchemeRgb_t WeUi_DemoFields_ToxicPixel(float x, float y,
														const WeUi_DemoFields_Frame_t* pcFrame) {
	float nx = x * WEUI_DEMO_TOXIC_SCALE_X + pcFrame->Sec * WEUI_DEMO_TOXIC_FLOW_X +
			   WEUI_DEMO_FIELDS_ORIGIN;
	float ny = y * WEUI_DEMO_TOXIC_SCALE_Y + pcFrame->Sec * WEUI_DEMO_TOXIC_FLOW_Y +
			   WEUI_DEMO_FIELDS_ORIGIN;
	float nz = pcFrame->Sec * WEUI_DEMO_TOXIC_MORPH + WEUI_DEMO_FIELDS_ORIGIN;

	float fold =
		(fabsf(perlin_3D(nx, ny, nz)) + 0.5f * fabsf(perlin_3D(2.0f * nx, 2.0f * ny, nz))) / 1.5f;
	float vein =
		powf(CUT_NORM_FLOAT(1.0f - fold * WEUI_DEMO_TOXIC_GAIN), WEUI_DEMO_TOXIC_SHARPNESS);

	WeUi_ColorSchemeRgb_t rgb = WeUi_DemoFields_Palette_At(&WeUi_DemoFields_Toxic, vein);
	WeUi_ColorSchemeHsb_t hsb;

	WeUi_Color_Rgb2Hsb(&rgb, &hsb);
	hsb.Hue = WeUi_Color_HueIncDec(hsb.Hue, pcFrame->Sec * WEUI_DEMO_TOXIC_HUE_SPIN);
	WeUi_Color_Hsb2Rgb(&hsb, &rgb);

	return rgb;
}

static void WeUi_DemoFields_PaintSweep(float sec) {
	WeUi_DemoFields_Fill(WeUi_DemoFields_SweepPixel, sec);
}

static void WeUi_DemoFields_PaintNebula(float sec) {
	WeUi_DemoFields_Fill(WeUi_DemoFields_NebulaPixel, sec);
}

static void WeUi_DemoFields_PaintFire(float sec) {
	WeUi_DemoFields_Fill(WeUi_DemoFields_FirePixel, sec);
}

static void WeUi_DemoFields_PaintOcean(float sec) {
	WeUi_DemoFields_Fill(WeUi_DemoFields_OceanPixel, sec);
}

static void WeUi_DemoFields_PaintAurora(float sec) {
	WeUi_DemoFields_AuroraPrepare(sec);
	WeUi_DemoFields_Fill(WeUi_DemoFields_AuroraPixel, sec);
}

static void WeUi_DemoFields_PaintSpiral(float sec) {
	WeUi_DemoFields_Fill(WeUi_DemoFields_SpiralPixel, sec);
}

static void WeUi_DemoFields_PaintToxic(float sec) {
	WeUi_DemoFields_Fill(WeUi_DemoFields_ToxicPixel, sec);
}

/* Speeds, palettes and ways in alternate, so no two neighbours read alike */
typedef enum {
	WEUI_DEMO_FIELD_SWEEP,
	WEUI_DEMO_FIELD_FIRE,
	WEUI_DEMO_FIELD_OCEAN,
	WEUI_DEMO_FIELD_AURORA,
	WEUI_DEMO_FIELD_NEBULA,
	WEUI_DEMO_FIELD_SPIRAL,
	WEUI_DEMO_FIELD_TOXIC,

	WEUI_DEMO_FIELD_ENUM_SIZE
} WeUi_DemoField_t;

static const WeUi_DemoFields_Segment_t WeUi_DemoFields_Reel[WEUI_DEMO_FIELD_ENUM_SIZE] = {
	[WEUI_DEMO_FIELD_SWEEP]	 = {WeUi_DemoFields_PaintSweep, WEUI_DEMO_FIELDS_SEGMENT_MS,
								WEUI_DEMO_FIELDS_IN_DISSOLVE},
	[WEUI_DEMO_FIELD_FIRE]	 = {WeUi_DemoFields_PaintFire, WEUI_DEMO_FIELDS_SEGMENT_MS,
								WEUI_DEMO_FIELDS_IN_PUSH_UP},
	[WEUI_DEMO_FIELD_OCEAN]	 = {WeUi_DemoFields_PaintOcean, WEUI_DEMO_FIELDS_SEGMENT_MS,
								WEUI_DEMO_FIELDS_IN_SWIPE_RIGHT},
	[WEUI_DEMO_FIELD_AURORA] = {WeUi_DemoFields_PaintAurora, WEUI_DEMO_FIELDS_SEGMENT_MS,
								WEUI_DEMO_FIELDS_IN_PUSH_DOWN},
	[WEUI_DEMO_FIELD_NEBULA] = {WeUi_DemoFields_PaintNebula, WEUI_DEMO_FIELDS_SEGMENT_MS,
								WEUI_DEMO_FIELDS_IN_PUSH_LEFT},
	[WEUI_DEMO_FIELD_SPIRAL] = {WeUi_DemoFields_PaintSpiral, WEUI_DEMO_FIELDS_SEGMENT_MS,
								WEUI_DEMO_FIELDS_IN_DISSOLVE},
	[WEUI_DEMO_FIELD_TOXIC]	 = {WeUi_DemoFields_PaintToxic, WEUI_DEMO_FIELDS_SEGMENT_MS,
								WEUI_DEMO_FIELDS_IN_SWIPE_LEFT},
};

/* Both the way in and the hold after it: a transition's incoming scene paces itself,
 * and only out of one does the push reach the panel */
static u32 WeUi_DemoFields_ElapsedMs(void) {
	return WeUi_Gfx_FpsCtrl_GetTickCnt() - WeUi_DemoFields_StartMs;
}

static void WeUi_DemoFields_Step(void) {
	WeUi_DemoFields_Paint((float)WeUi_DemoFields_ElapsedMs() / 1000.0f);
	WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(WEUI_DEMO_FIELDS_FPS);
}

static u16 WeUi_DemoFields_Frames(u32 ms) {
	return WeUi_Gfx_Effect_Frames(ms, WEUI_DEMO_FIELDS_FPS);
}

static void WeUi_DemoFields_Push(WEUI_GFX_EFFECT_SLIDE_t slide) {
	WeUi_Gfx_Effect_Push(WeUi_DemoFields_Step, WeUi_DemoFields_Frames(WEUI_DEMO_FIELDS_PUSH_MS),
						 slide, NULL);
}

static void WeUi_DemoFields_Swipe(WEUI_GFX_EFFECT_SLIDE_t slide) {
	WeUi_Gfx_Effect_Swipe(WeUi_DemoFields_Step, WeUi_DemoFields_Frames(WEUI_DEMO_FIELDS_SWIPE_MS),
						  slide);
}

static void WeUi_DemoFields_PlayIn(WeUi_DemoFields_In_t in) {
	switch (in) {
		case WEUI_DEMO_FIELDS_IN_PUSH_LEFT:
			WeUi_DemoFields_Push(WEUI_GFX_EFFECT_SLIDE_LEFT);
			break;

		case WEUI_DEMO_FIELDS_IN_PUSH_UP:
			WeUi_DemoFields_Push(WEUI_GFX_EFFECT_SLIDE_UP);
			break;

		case WEUI_DEMO_FIELDS_IN_PUSH_DOWN:
			WeUi_DemoFields_Push(WEUI_GFX_EFFECT_SLIDE_DOWN);
			break;

		case WEUI_DEMO_FIELDS_IN_SWIPE_LEFT:
			WeUi_DemoFields_Swipe(WEUI_GFX_EFFECT_SLIDE_LEFT);
			break;

		case WEUI_DEMO_FIELDS_IN_SWIPE_RIGHT:
			WeUi_DemoFields_Swipe(WEUI_GFX_EFFECT_SLIDE_RIGHT);
			break;

		case WEUI_DEMO_FIELDS_IN_DISSOLVE:
		default:
			WeUi_Gfx_Effect_Dissolve(WeUi_DemoFields_Step,
									 WeUi_DemoFields_Frames(WEUI_DEMO_FIELDS_DISSOLVE_MS));
			break;
	}
}

/* Comes in over whatever is on the panel, then plays out its time */
static void WeUi_DemoFields_PlaySegment(const WeUi_DemoFields_Segment_t* pcSeg) {
	WeUi_DemoFields_Paint	= pcSeg->Paint;
	WeUi_DemoFields_StartMs = WeUi_Gfx_FpsCtrl_GetTickCnt();

	WeUi_DemoFields_PlayIn(pcSeg->In);
	while (WeUi_DemoFields_ElapsedMs() < pcSeg->Ms)
		WeUi_DemoFields_Step();
}

void WeUi_DrawDemo_FieldSweep(void) {
	WeUi_DemoFields_PlaySegment(&WeUi_DemoFields_Reel[WEUI_DEMO_FIELD_SWEEP]);
}

void WeUi_DrawDemo_FieldFire(void) {
	WeUi_DemoFields_PlaySegment(&WeUi_DemoFields_Reel[WEUI_DEMO_FIELD_FIRE]);
}

void WeUi_DrawDemo_FieldOcean(void) {
	WeUi_DemoFields_PlaySegment(&WeUi_DemoFields_Reel[WEUI_DEMO_FIELD_OCEAN]);
}

void WeUi_DrawDemo_FieldAurora(void) {
	WeUi_DemoFields_PlaySegment(&WeUi_DemoFields_Reel[WEUI_DEMO_FIELD_AURORA]);
}

void WeUi_DrawDemo_FieldNebula(void) {
	WeUi_DemoFields_PlaySegment(&WeUi_DemoFields_Reel[WEUI_DEMO_FIELD_NEBULA]);
}

void WeUi_DrawDemo_FieldSpiral(void) {
	WeUi_DemoFields_PlaySegment(&WeUi_DemoFields_Reel[WEUI_DEMO_FIELD_SPIRAL]);
}

void WeUi_DrawDemo_FieldToxic(void) {
	WeUi_DemoFields_PlaySegment(&WeUi_DemoFields_Reel[WEUI_DEMO_FIELD_TOXIC]);
}

/**
 * @brief Every field above in a row, each coming in over the last one.
 */
void WeUi_DrawDemo_Fields(void) {
	for (u32 idx = 0; idx < NUM_ELEMENTS(WeUi_DemoFields_Reel); idx++)
		WeUi_DemoFields_PlaySegment(&WeUi_DemoFields_Reel[idx]);
}

#endif /* WEUI_USE_COMPONENT__DEMO */
