#include "weui_demo.h"

#if WEUI_USE_COMPONENT__DEMO

#include "log.h"
#include "weui_demo_pace.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_colors.h"
#include "weui_gfx_fps_control.h"
#include "weui_gfx_text.h"

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT LOG
#else /* LOCAL_DEBUG_PRINT_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* LOCAL_DEBUG_PRINT_ENABLE */

#define DEMO_FONTS_BG_BRI 40.0f

/** @brief The same text twice, scrolling together: rainbow on nothing above, white on a
 *         colour cycling under it below */
void WeUi_DrawDemo_Fonts(void) {
	WeUi_Gfx_Text_t clear, solid;
	WeUi_Gfx_Text_InitStruct(&clear);
	clear.Str	 = "0123456789 AaBbXxYyZz АаБбЖжЮюЯя !?@#&";
	clear.Font	 = u8g2_font_6x12_t_cyrillic;
	clear.BgMode = WEUI_TEXT_BG_MODE_TRANSPARENT;

	solid		 = clear;
	solid.BgMode = WEUI_TEXT_BG_MODE_SOLID;
	memset(&solid.TextColor, WEUI_COLOR_CHANNEL_MAX, sizeof(WeUi_Gfx_Color_t));

	u8 rowH					 = WeUi_Gfx_Text_GetHeight(&clear);
	WeUi_Gfx_Point_t ptClear = {.X = WeUi_Gfx_Adapter_GetWidth(), .Y = rowH};
	WeUi_Gfx_Point_t ptSolid = {.X = WeUi_Gfx_Adapter_GetWidth(), .Y = 2 * rowH};
	u32 strLen				 = WeUi_Gfx_Text_GetPixLen(&clear) + WeUi_Gfx_Adapter_GetWidth();

	/* One px a frame, so the scroll stays smooth: the rate is what fits the budget */
	u32 fps = CUT_MIN_MAX(strLen / WEUI_DEMO_SEC, WeUi_Gfx_FpsCtrl_GetMinFreq(),
						  WeUi_Gfx_FpsCtrl_GetMaxFreq());

	for (u32 i = 0; i < strLen; i++) {
		float hue = fmodf((float)i, WEUI_COLOR_HSB_MAX_HUE);

		clear.TextColor = WeUi_Gfx_Color_FromHsb(&(WeUi_Gfx_ColorSchemeHsb_t){
			.Hue = hue,
			.Sat = WEUI_COLOR_HSB_MAX_SAT,
			.Bri = WEUI_COLOR_HSB_MAX_BRI,
		});
		solid.BackColor = WeUi_Gfx_Color_FromHsb(&(WeUi_Gfx_ColorSchemeHsb_t){
			.Hue = hue,
			.Sat = WEUI_COLOR_HSB_MAX_SAT,
			.Bri = DEMO_FONTS_BG_BRI,
		});

		WeUi_Gfx_Adapter_ClearDrawingFrame();
		ptClear.X--;
		ptSolid.X--;
		WeUi_Gfx_Text_Draw(&ptClear, &clear);
		WeUi_Gfx_Text_Draw(&ptSolid, &solid);
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(fps);
	}
}

/* The font transforms are three parts in a third of the budget each, every frame alive */
#define DEMO_FONT_PARTS		  3
#define DEMO_FONT_PART_FRAMES (WEUI_DEMO_LIVE_FRAMES / DEMO_FONT_PARTS)
#define DEMO_FONT_HUE_FLOW	  6.0f /* degrees per frame */

#define DEMO_FONT_SPIN_TEXT		  "WEUI"
#define DEMO_FONT_SPIN_QUARTER_FR 5 /* frames a quarter turn holds */
#define DEMO_FONT_SPIN_TURNS	  (WEUI_TEXT_ROTATION_270 + 1)

#define DEMO_FONT_WAVE_TEXT	   "Hello, мир!"
#define DEMO_FONT_WAVE_FONT	   u8g2_font_6x12_t_cyrillic
#define DEMO_FONT_WAVE_SPREAD  0.7f	 /* radians between neighbour letters */
#define DEMO_FONT_WAVE_SPEED   0.35f /* radians per frame */
#define DEMO_FONT_WAVE_HUE_GAP 30.0f /* degrees between neighbour letters */

/* Smallest to largest: the parts pick what fits the panel */
static const u8* const DemoFont_Ladder[] = {
	u8g2_font_4x6_tf,	u8g2_font_5x7_tf,	u8g2_font_5x8_tf,	u8g2_font_6x10_tf,
	u8g2_font_6x12_tf,	u8g2_font_7x13_tf,	u8g2_font_8x13_tf,	u8g2_font_9x15_tf,
	u8g2_font_9x18_tf,	u8g2_font_10x20_tf, u8g2_font_fur14_tr, u8g2_font_fur17_tr,
	u8g2_font_fur20_tr, u8g2_font_fur25_tr, u8g2_font_fub25_tr, u8g2_font_fub30_tr,
	u8g2_font_fub35_tr, u8g2_font_fub42_tr,
};

static WeUi_Gfx_Color_t WeUi_DemoFont_Hue(float hue) {
	return WeUi_Gfx_Color_FromHsb(&(WeUi_Gfx_ColorSchemeHsb_t){
		.Hue = fmodf(hue, WEUI_COLOR_HSB_MAX_HUE),
		.Sat = WEUI_COLOR_HSB_MAX_SAT,
		.Bri = WEUI_COLOR_HSB_MAX_BRI,
	});
}

/** @brief Whether the text, set in its font, fits a w x h box upright. A font that
 *         lacks the text's glyphs draws nothing, which is no fit */
static bool WeUi_DemoFont_Fits(WeUi_Gfx_Text_t* pTextObj, u32 w, u32 h) {
	u32 len = WeUi_Gfx_Text_GetPixLen(pTextObj);
	return len && len <= w && WeUi_Gfx_Text_GetHeightAboveBaseline(pTextObj) <= h;
}

/**
 * @brief The baseline start that centres the text on the panel at its rotation. u8g2
 *        turns clockwise about that point: at 90 the text runs down with the letters'
 *        tops to the right, at 180 it runs left upside down, at 270 it runs up.
 */
static WeUi_Gfx_Point_t WeUi_DemoFont_Centre(WeUi_Gfx_Text_t* pTextObj) {
	WeUi_Gfx_PtPos_t cx = WeUi_Gfx_Adapter_GetX_Mid();
	WeUi_Gfx_PtPos_t cy = WeUi_Gfx_Adapter_GetY_Mid();
	WeUi_Gfx_PtPos_t w	= (WeUi_Gfx_PtPos_t)WeUi_Gfx_Text_GetPixLen(pTextObj);
	WeUi_Gfx_PtPos_t a	= (WeUi_Gfx_PtPos_t)WeUi_Gfx_Text_GetHeightAboveBaseline(pTextObj);

	switch (pTextObj->Rotation) {
		case WEUI_TEXT_ROTATION_90:
			return (WeUi_Gfx_Point_t){.X = cx - a / 2, .Y = cy - w / 2};
		case WEUI_TEXT_ROTATION_180:
			return (WeUi_Gfx_Point_t){.X = cx + w / 2, .Y = cy - a / 2};
		case WEUI_TEXT_ROTATION_270:
			return (WeUi_Gfx_Point_t){.X = cx + a / 2, .Y = cy + w / 2};
		default:
			return (WeUi_Gfx_Point_t){.X = cx - w / 2, .Y = cy + a / 2};
	}
}

/** @brief A word spun on the spot a quarter at a time, in the largest font that keeps
 *         it whole at every turn */
static void WeUi_DemoFont_Spin(void) {
	WeUi_Gfx_Text_t textObj;
	WeUi_Gfx_Text_InitStruct(&textObj);
	textObj.Str = DEMO_FONT_SPIN_TEXT;

	u32 side = GET_MIN(WeUi_Gfx_Adapter_GetWidth(), WeUi_Gfx_Adapter_GetHeight());

	textObj.Font = NULL;
	for (u32 i = 0; i < NUM_ELEMENTS(DemoFont_Ladder); i++) {
		WeUi_Gfx_Text_t probe = textObj;
		probe.Font			  = DemoFont_Ladder[i];
		if (WeUi_DemoFont_Fits(&probe, side, side))
			textObj.Font = DemoFont_Ladder[i];
	}

	if (!textObj.Font) {
		LOCAL_DEBUG_PRINT("No font spins \"%s\" on the panel", DEMO_FONT_SPIN_TEXT);
		return;
	}

	for (u16 frame = 0; frame < DEMO_FONT_PART_FRAMES; frame++) {
		textObj.Rotation =
			(WEUI_TEXT_ROTATION_MODE_t)((frame / DEMO_FONT_SPIN_QUARTER_FR) % DEMO_FONT_SPIN_TURNS);
		textObj.TextColor = WeUi_DemoFont_Hue((float)frame * DEMO_FONT_HUE_FLOW);

		WeUi_Gfx_Point_t pt = WeUi_DemoFont_Centre(&textObj);

		WeUi_Gfx_Adapter_ClearDrawingFrame();
		WeUi_Gfx_Text_Draw(&pt, &textObj);
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(WEUI_DEMO_LIVE_FPS);
	}
}

/** @brief The text breathes through every font of the ladder that fits, up and back down */
static void WeUi_DemoFont_Pulse(void) {
	WeUi_Gfx_Text_t textObj;
	WeUi_Gfx_Text_InitStruct(&textObj);

	WeUi_Gfx_ObjSize_t displayW = WeUi_Gfx_Adapter_GetWidth();
	WeUi_Gfx_ObjSize_t displayH = WeUi_Gfx_Adapter_GetHeight();

	const u8* fits[NUM_ELEMENTS(DemoFont_Ladder)];
	u32 fitCount = 0;

	textObj.Str = "Hello";
	for (u32 i = 0; i < NUM_ELEMENTS(DemoFont_Ladder); i++) {
		textObj.Font = DemoFont_Ladder[i];
		if (WeUi_DemoFont_Fits(&textObj, displayW, displayH))
			fits[fitCount++] = DemoFont_Ladder[i];
	}

	if (fitCount < 2) {
		LOCAL_DEBUG_PRINT("Too few fonts of the ladder fit %dx%d", displayW, displayH);
		return;
	}

	/* Up the ladder and down again, once over the part */
	u32 span = 2 * (fitCount - 1);
	for (u16 frame = 0; frame < DEMO_FONT_PART_FRAMES; frame++) {
		u32 step = frame * span / DEMO_FONT_PART_FRAMES;
		u32 idx	 = (step < fitCount) ? step : span - step;

		textObj.Font	  = fits[idx];
		textObj.TextColor = WeUi_DemoFont_Hue((float)frame * DEMO_FONT_HUE_FLOW);

		WeUi_Gfx_Point_t pt = WeUi_DemoFont_Centre(&textObj);

		WeUi_Gfx_Adapter_ClearDrawingFrame();
		WeUi_Gfx_Text_Draw(&pt, &textObj);
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(WEUI_DEMO_LIVE_FPS);
	}
}

/** @brief Letter by letter, each on its own hue and riding a sine wave across the text */
static void WeUi_DemoFont_Wave(void) {
	WeUi_Gfx_Text_t textObj;
	WeUi_Gfx_Text_InitStruct(&textObj);
	textObj.Font = DEMO_FONT_WAVE_FONT;

	const char* pcText = DEMO_FONT_WAVE_TEXT;
	u32 textLen		   = strlen(pcText);
	textObj.Str		   = (char*)pcText; /* only read, the field just is not const */

	WeUi_Gfx_PtPos_t a = (WeUi_Gfx_PtPos_t)WeUi_Gfx_Text_GetHeightAboveBaseline(&textObj);
	WeUi_Gfx_PtPos_t x0 =
		WeUi_Gfx_Adapter_GetX_Mid() - (WeUi_Gfx_PtPos_t)(WeUi_Gfx_Text_GetPixLen(&textObj) / 2);
	WeUi_Gfx_PtPos_t y0 = WeUi_Gfx_Adapter_GetY_Mid() + a / 2;
	/* What the panel leaves above and below, but no more than a letter: further apart the
	 * letters stop reading as a word */
	float amp = CUT_MIN_MAX((float)(WeUi_Gfx_Adapter_GetHeight() - a) / 2.0f, 1.0f, (float)a);
	char glyph[WEUI_GFX_TEXT_UTF8_BUF_SIZE];

	for (u16 frame = 0; frame < DEMO_FONT_PART_FRAMES; frame++) {
		WeUi_Gfx_Adapter_ClearDrawingFrame();

		u32 letter = 0;
		for (u32 pos = 0; pos < textLen; letter++) {
			u32 len = GET_MIN(WeUi_Gfx_Text_Utf8CharLen((u8)pcText[pos]), textLen - pos);
			memcpy(glyph, &pcText[pos], len);
			glyph[len] = '\0';

			float phase =
				(float)frame * DEMO_FONT_WAVE_SPEED - (float)letter * DEMO_FONT_WAVE_SPREAD;
			WeUi_Gfx_Point_t pt = {
				.X = x0 + (WeUi_Gfx_PtPos_t)WeUi_Gfx_Text_GetPixLenN(&textObj, pcText, pos),
				.Y = y0 + (WeUi_Gfx_PtPos_t)lroundf(amp * sinf(phase)),
			};

			textObj.Str		  = glyph;
			textObj.TextColor = WeUi_DemoFont_Hue((float)frame * DEMO_FONT_HUE_FLOW +
												  (float)letter * DEMO_FONT_WAVE_HUE_GAP);
			WeUi_Gfx_Text_Draw(&pt, &textObj);

			pos += len;
		}

		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(WEUI_DEMO_LIVE_FPS);
	}
}

/** @brief A word spun on the spot, a text breathing through the font sizes, and letters
 *         riding a wave - every frame moves */
void WeUi_DrawDemo_FontTransforms(void) {
	WeUi_DemoFont_Spin();
	WeUi_DemoFont_Pulse();
	WeUi_DemoFont_Wave();
}

#define DEMO_ICONS_FPS 4 /* 17 glyphs */

void WeUi_DrawDemo_Icons(void) {
	WeUi_Gfx_Text_t textObj;
	WeUi_Gfx_Text_InitStruct(&textObj);

	char iconStr[2] = {0, 0};
	textObj.Str		= iconStr;

	const u8* iconFonts[] = {
		u8g2_font_open_iconic_embedded_1x_t,
		u8g2_font_open_iconic_embedded_2x_t,
		u8g2_font_open_iconic_embedded_4x_t,
		u8g2_font_open_iconic_embedded_6x_t,
	};
	const u32 scaleCount = NUM_ELEMENTS(iconFonts);

	WeUi_Gfx_PtPos_t midY = WeUi_Gfx_Adapter_GetY_Mid();

	/* Embedded icons: glyphs 64..80 (17 icons) */
	for (u8 glyph = 64; glyph <= 80; glyph++) {
		iconStr[0] = (char)glyph;

		float hue		  = (float)(glyph - 64) * WEUI_COLOR_HSB_MAX_HUE / 17.0f;
		textObj.TextColor = WeUi_Gfx_Color_FromHsb(&(WeUi_Gfx_ColorSchemeHsb_t){
			.Hue = hue,
			.Sat = WEUI_COLOR_HSB_MAX_SAT,
			.Bri = WEUI_COLOR_HSB_MAX_BRI,
		});

		WeUi_Gfx_Adapter_ClearDrawingFrame();

		WeUi_Gfx_PtPos_t xPos = 2;
		for (u32 s = 0; s < scaleCount; s++) {
			textObj.Font = iconFonts[s];
			u8 h		 = WeUi_Gfx_Text_GetHeight(&textObj);

			WeUi_Gfx_Point_t pt = {
				.X = xPos,
				.Y = midY + (WeUi_Gfx_PtPos_t)(h / 2),
			};
			WeUi_Gfx_Text_Draw(&pt, &textObj);
			xPos += WeUi_Gfx_Text_GetPixLen(&textObj) + 4;
		}

		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_ICONS_FPS);
	}
}

#endif /* WEUI_USE_COMPONENT__DEMO */
