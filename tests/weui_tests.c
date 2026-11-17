/**
 * Host tests: weui drawn into a simulated panel (sim/display_sim.c) under the sanitizers.
 *
 * Two kinds of check. Regression tests assert a property - a crop keeps every pixel in
 * place, a transition lands on the incoming picture. Golden runs play every demo on a
 * virtual clock and fold each pushed frame into one hash per run ("hash ..." lines):
 * `make golden` records them, `make check` diffs against the record, so any change in
 * what reaches the panel shows up and has to be explained.
 */

#include "display_m.h"
#include "log.h"
#include "test_png.h"
#include "weui_demo.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_affine.h"
#include "weui_gfx_animation.h"
#include "weui_gfx_base.h"
#include "weui_gfx_colors.h"
#include "weui_gfx_effects.h"
#include "weui_gfx_fill.h"
#include "weui_gfx_fps_control.h"
#include "weui_gfx_image.h"
#include "weui_gfx_path.h"
#include "weui_gfx_scene.h"
#include "weui_gfx_scenemgr.h"
#include "weui_gfx_scenemgr_trace.h"
#include "weui_gfx_stroke.h"
#include "weui_gfx_text.h"
#include "weui_gfx_transform.h"
#include "weui_gfx_widgets.h"
#include "weui_rand.h"

#define TEST_PNG_SCALE		8
#define TEST_DEMO_PNG_EVERY 25

static const char* OutDir = "out";
static u32 Failed;
static u32 Checks;

#define CHECK(cond, ...)                                    \
	do {                                                    \
		Checks++;                                           \
		if (!(cond)) {                                      \
			Failed++;                                       \
			printf("    FAIL %s:%d: ", __func__, __LINE__); \
			printf(__VA_ARGS__);                            \
			printf("\n");                                   \
		}                                                   \
	} while (0)

// =================== Helpers ===================

void Sim_Log(LOG_LVL_t lvl, const char* pcFmt, ...) {
	(void)lvl, (void)pcFmt;
}

void Sim_Panic(const char* pcFile, int line) {
	printf("PANIC at %s:%d\n", pcFile, line);
	abort();
}

static void Sim_Setup(void) {
	DisplaySim_Init(WEUI_DISPLAY_WIDTH, WEUI_DISPLAY_HEIGHT);
	srand(1);
	WeUi_Gfx_Adapter_Init();
	WeUi_Gfx_Text_Init();
	WeUi_Gfx_FpsCtrl_AttachDrawCallback(WeUi_Gfx_Adapter_DrawFrame);
	WeUi_Gfx_FpsCtrl_SetMainFreqToDef();
	WeUi_Gfx_Adapter_ClearAllFrames();
}

static const u32* Shown(void) {
	return DisplaySim_GetDisplayFrame();
}

static u32 PixelAt(u32 x, u32 y) {
	return Shown()[y * WEUI_DISPLAY_WIDTH + x];
}

static u32 CountLit(void) {
	u32 lit = 0;
	for (u32 i = 0; i < (u32)WEUI_DISPLAY_WIDTH * WEUI_DISPLAY_HEIGHT; i++)
		lit += Shown()[i] != 0;
	return lit;
}

static WeUi_Gfx_Color_t Rgb(u8 r, u8 g, u8 b) {
	return WeUi_Gfx_Transform_Rgb2Color(r, g, b);
}

/* What the panel holds for a colour, so one expectation serves a colour and a gray build */
static u32 Stored(WeUi_Gfx_Color_t color) {
#ifdef WEUI_DISPLAY_GRAY
	u8 luma = WeUi_Gfx_Transform_Color2Byte_Luma(color);
	color	= Rgb(luma, luma, luma);
#endif /* WEUI_DISPLAY_GRAY */
	return WeUi_Gfx_Transform_Color2Bytes(color);
}

/* What a buffer holds for a colour: no driver stores it, so a gray panel's level does not
 * apply - only a gray scheme's, kept in R alone */
static u32 Given(WeUi_Gfx_Color_t color) {
#if defined(WEUI_LIB_COLOR_SCHEME__GRAY)
	return WeUi_Gfx_Transform_FillBytesFromChannel(color.R);
#else
	return WeUi_Gfx_Transform_Color2Bytes(color);
#endif /* defined(WEUI_LIB_COLOR_SCHEME__GRAY) */
}

static void SavePng(const char* pcName, const u32* pcFrame) {
	char path[256];
	snprintf(path, sizeof(path), "%s/%s.png", OutDir, pcName);
	Test_SavePng(path, pcFrame, TEST_PNG_SCALE);
}

static u64 RunHash;
static const char* RunName;

static void Hook_Record(const u32* pcFrame, u32 idx) {
	RunHash = (RunHash ^ DisplaySim_Hash(pcFrame)) * 1099511628211ULL;
	if (idx % TEST_DEMO_PNG_EVERY == 0) {
		char name[128];
		snprintf(name, sizeof(name), "%s_%04u", RunName, idx);
		SavePng(name, pcFrame);
	}
}

static void Record_Start(const char* pcName) {
	RunHash = 1469598103934665603ULL;
	RunName = pcName;
	DisplaySim_SetFrameHook(Hook_Record);
}

static void Record_Print(void) {
	printf("hash %-16s frames %5u %016llx\n", RunName, DisplaySim_GetFramesPushed(),
		   (unsigned long long)RunHash);
}

// =================== Primitives ===================

static void Test_RectZeroSize(void) {
	Sim_Setup();
	WeUi_Gfx_Rect_t rect;
	WeUi_Gfx_Rect_InitStruct(&rect);
	rect.PtOne = (WeUi_Gfx_Point_t){10, 10};
	rect.Width = 10;
	rect.Color = Rgb(255, 255, 255);

	WeUi_Gfx_RectEdges_Draw(&rect);
	WeUi_Gfx_Adapter_DrawFrame();
	CHECK(CountLit() == 0, "zero-height rect lit %u px", CountLit());
}

static void Test_RectFillKeepsInput(void) {
	Sim_Setup();
	WeUi_Gfx_Rect_t rect;
	WeUi_Gfx_Rect_InitStruct(&rect);
	rect.PtOne = (WeUi_Gfx_Point_t){20, 15};
	rect.PtTwo = (WeUi_Gfx_Point_t){5, 2};
	rect.Color = Rgb(255, 0, 0);

	WeUi_Gfx_RectVerticesFill_Draw(&rect);
	CHECK(rect.PtOne.X == 20 && rect.PtOne.Y == 15, "input rewritten to (%d,%d)-(%d,%d)",
		  rect.PtOne.X, rect.PtOne.Y, rect.PtTwo.X, rect.PtTwo.Y);
}

static void Test_LineDefaultsVisible(void) {
	Sim_Setup();
	WeUi_Gfx_Line_t line;
	WeUi_Gfx_Line_InitStruct(&line);
	line.Base.PtTwo = (WeUi_Gfx_Point_t){WEUI_DISPLAY_WIDTH - 1, 0};
	line.Base.Color = Rgb(255, 255, 255);

	WeUi_Gfx_Line_Draw(&line);
	WeUi_Gfx_Adapter_DrawFrame();
	CHECK(CountLit() == WEUI_DISPLAY_WIDTH, "InitStruct line lit %u of %u px", CountLit(),
		  WEUI_DISPLAY_WIDTH);
}

static void Test_ColorOverRange(void) {
	WeUi_Gfx_ColorSchemeHsb_t hsb = {.Hue = -30.0f, .Sat = 100.0f, .Bri = 255.0f};
	WeUi_Gfx_Color_t color		  = WeUi_Gfx_Color_FromHsb(&hsb);
	CHECK(color.R == UINT8_MAX, "over-range brightness gave R=%u", color.R);
}

static void Test_FillMatchesPixel(void) {
	Sim_Setup();
	WeUi_Gfx_Color_t color = Rgb(200, 40, 90);
	WeUi_Gfx_Adapter_FillDrawingFrame(color);

	WeUi_Gfx_ColorPoint_t cpt = {.Pt = {1, 0}, .Color = color};
	WeUi_Gfx_Point_Draw(&cpt);
	CHECK(DisplayM_GetPixel(1, 0) == DisplayM_GetPixel(0, 0), "fill 0x%06x, pixel 0x%06x",
		  DisplayM_GetPixel(0, 0), DisplayM_GetPixel(1, 0));
}

// =================== Text ===================

static void Test_TextWidthConsistency(void) {
	Sim_Setup();
	static const char* strs[] = {"Hello", "0123456789", "Привет, мир", "W", "iii", "a b  c"};
	static const u8* fonts[]  = {u8g2_font_8x13_t_cyrillic, u8g2_font_5x7_tf, u8g2_font_fur11_tr};

	for (u32 f = 0; f < NUM_ELEMENTS(fonts); f++) {
		for (u32 s = 0; s < NUM_ELEMENTS(strs); s++) {
			WeUi_Gfx_Text_t text;
			WeUi_Gfx_Text_InitStruct(&text);
			text.Font = fonts[f];
			text.Str  = (char*)strs[s];
			u32 len	  = (u32)strlen(strs[s]);

			u32 full = WeUi_Gfx_Text_GetPixLen(&text);
			u32 byN	 = WeUi_Gfx_Text_GetPixLenN(&text, strs[s], len);
			CHECK(full == byN, "\"%s\" font %u: %u != %u", strs[s], f, full, byN);

			u32 fit = WeUi_Gfx_Text_FitLen(&text, strs[s], len, 20);
			CHECK(fit == 0 || ((u8)strs[s][fit] & 0xC0) != 0x80, "FitLen split UTF-8 at %u", fit);
		}
	}
}

static void Test_Utf8FromCodePoint(void) {
	static const u32 cps[] = {0x41,	 0x7F,	 0x80,	 0x416,	  0x7FF,
							  0x800, 0x20AC, 0xFFFF, 0x10000, 0x10FFFF};
	char buf[WEUI_GFX_TEXT_UTF8_BUF_SIZE];

	for (u32 i = 0; i < NUM_ELEMENTS(cps); i++) {
		u32 len = WeUi_Gfx_Text_Utf8FromCodePoint(cps[i], buf, sizeof(buf));
		CHECK(len == WeUi_Gfx_Text_Utf8CharLen((u8)buf[0]), "U+%X: len %u", cps[i], len);
	}
	CHECK(WeUi_Gfx_Text_Utf8FromCodePoint(0xD800, buf, sizeof(buf)) == 0, "surrogate encoded");
	CHECK(WeUi_Gfx_Text_Utf8FromCodePoint(0x110000, buf, sizeof(buf)) == 0, "past U+10FFFF");
	CHECK(WeUi_Gfx_Text_Utf8FromCodePoint(0x20AC, buf, 3) == 0, "overflowed a short buffer");
}

/* Text_Init leaves no font: nothing may reach u8g2's glyph lookup */
static void Test_TextNoFont(void) {
	Sim_Setup();
	WeUi_Gfx_Text_t text;
	WeUi_Gfx_Text_InitStruct(&text);
	text.Str = "no font";
	CHECK(WeUi_Gfx_Text_GetPixLen(&text) == 0, "measured without a font");
	CHECK(WeUi_Gfx_Text_GetHeight(&text) == 0, "height without a font");

	WeUi_Gfx_Widget_TextBlock_Style_t style;
	WeUi_Gfx_Widget_TextBlock_StyleInit(&style, NULL, NULL);
	CHECK(WeUi_Gfx_Widget_TextBlock_Draw(&style, "no font", 0, 5, 0, 0) == 5, "block moved y");
	WeUi_Gfx_Adapter_DrawFrame();
	CHECK(CountLit() == 0, "block drew without a font");
}

/* RefPt moves the whole screen: the same text, laid out alike, one offset further */
static void TextBlock_DrawShifted(WeUi_Gfx_PtPos_t dx, WeUi_Gfx_PtPos_t dy) {
	const WeUi_Gfx_Point_t refPt = {.X = dx, .Y = dy};
	WeUi_Gfx_Widget_TextBlock_Style_t style;
	WeUi_Gfx_Widget_TextBlock_StyleInit(&style, &refPt, u8g2_font_5x7_tf);
	style.Align = WEUI_TEXT_ALIGN_RIGHT;

	Sim_Setup();
	WeUi_Gfx_Widget_TextBlock_Draw(&style, "Offset wraps alike", 0, 0, 0, 0);
	WeUi_Gfx_Widget_TextBlock_DrawScreenCentered(&refPt, "Mid", u8g2_font_5x7_tf);
	WeUi_Gfx_Adapter_DrawFrame();
}

static void Test_TextBlockRefPtShifts(void) {
	static u32 base[WEUI_DISPLAY_WIDTH * WEUI_DISPLAY_HEIGHT];
	const u32 dx = WEUI_DISPLAY_WIDTH / 8, dy = WEUI_DISPLAY_HEIGHT / 8;

	TextBlock_DrawShifted(0, 0);
	memcpy(base, Shown(), sizeof(base));
	TextBlock_DrawShifted(dx, dy);
	SavePng("text_block_refpt", Shown());

	u32 moved = 0;
	for (u32 y = 0; y + dy < WEUI_DISPLAY_HEIGHT; y++)
		for (u32 x = 0; x + dx < WEUI_DISPLAY_WIDTH; x++)
			moved += PixelAt(x + dx, y + dy) != base[y * WEUI_DISPLAY_WIDTH + x];

	CHECK(CountLit() > 0 && !moved, "%u pixels differ from the unshifted block", moved);
}

static void Test_TextBlockWrap(void) {
	Sim_Setup();
	WeUi_Gfx_Widget_TextBlock_Style_t style;
	WeUi_Gfx_Widget_TextBlock_StyleInit(&style, NULL, u8g2_font_5x7_tf);
	style.Align		 = WEUI_TEXT_ALIGN_CENTER;
	style.IsBordered = true;

	const char* str		  = "Wrapped text block with a verylongwordthatdoesnotfit";
	WeUi_Gfx_ObjSize_t h  = WeUi_Gfx_Widget_TextBlock_GetHeight(&style, str, 60);
	WeUi_Gfx_PtPos_t yEnd = WeUi_Gfx_Widget_TextBlock_Draw(&style, str, 2, 2, 60, 0);
	WeUi_Gfx_Adapter_DrawFrame();
	SavePng("text_block_wrap", Shown());
	CHECK(yEnd - 2 == h, "Draw ended at %d, GetHeight said %u", yEnd - 2, h);
}

// =================== Images and animation (RAM storage, RGB pixels) ===================

#if defined(WEUI_LIB_COLOR_SCHEME__RGB)

/* 4x2: white, black, red, black / black, green, black, blue */
static const u8 ImgPixels[] = {
	255, 255, 255, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 255,
};

static void Image_FromRam(WeUi_Gfx_Image_t* pImg, const u8* pcData, u16 w, u16 h) {
	WeUi_Gfx_Image_InitStruct(pImg);
	pImg->Storage.StorageType = STORAGE_TYPE_RAM;
	pImg->Storage.Addr		  = (WeUi_Gfx_StorageAddr_t)pcData;
	pImg->Storage.Width		  = w;
	pImg->Storage.Height	  = h;
	pImg->Storage.Size		  = (u32)w * h * WEUI_IMG_STORAGE_COLOR_SIZE;
}

/* Transparent pixels must keep what is under them, not run it through the LUT again */
static void Test_ImageTransparentUnderLut(void) {
	Sim_Setup();
	DisplaySim_SetBrightScale(0.5f);

	WeUi_Gfx_Rect_t bg;
	WeUi_Gfx_Rect_InitStruct(&bg);
	bg.PtTwo = (WeUi_Gfx_Point_t){7, 3};
	bg.Color = Rgb(200, 200, 200);
	WeUi_Gfx_RectVerticesFill_Draw(&bg);
	u32 bgStored = DisplayM_GetPixel(1, 0);

	WeUi_Gfx_Image_t img;
	Image_FromRam(&img, ImgPixels, 4, 2);
	img.Transform.Intersect = IMG_INTERSECT_FRONT_OPACITY_ENABLED;
	WeUi_Gfx_Point_t pt		= {0, 0};
	WeUi_Gfx_Image_Draw(&pt, &img);

	CHECK(DisplayM_GetPixel(1, 0) == bgStored, "transparent pixel 0x%06x -> 0x%06x", bgStored,
		  DisplayM_GetPixel(1, 0));
	u32 red = DisplaySim_ApplyBright(Stored(Rgb(255, 0, 0)));
	CHECK(DisplayM_GetPixel(2, 0) == red, "opaque red 0x%06x, want 0x%06x", DisplayM_GetPixel(2, 0),
		  red);
}

/* Too big for the work buffers: the top-left panel-sized part, every pixel in place */
static void Test_ImageCrop(void) {
	static const struct {
		u16 W, H;
	} sizes[] = {{100, 30}, {10, 300}, {300, 10}};
	static u8 pixels[300 * 30 * 3];

	for (u32 s = 0; s < NUM_ELEMENTS(sizes); s++) {
		Sim_Setup();
		u16 w = sizes[s].W, h = sizes[s].H;
		memset(pixels, 0xEE, sizeof(pixels)); /* whatever lies past the image must not show */
		for (u32 y = 0; y < h; y++) {
			for (u32 x = 0; x < w; x++) {
				u8* pPx = &pixels[(y * w + x) * 3];
				pPx[0] = (u8)x, pPx[1] = (u8)y, pPx[2] = 0x5A;
			}
		}

		WeUi_Gfx_Image_t img;
		Image_FromRam(&img, pixels, w, h);
		WeUi_Gfx_Point_t pt = {0, 0};
		WeUi_Gfx_Image_Draw(&pt, &img);
		WeUi_Gfx_Adapter_DrawFrame();

		u32 bad = 0;
		for (u32 y = 0; y < WEUI_DISPLAY_HEIGHT; y++)
			for (u32 x = 0; x < WEUI_DISPLAY_WIDTH; x++)
				bad += PixelAt(x, y) != ((x < w && y < h) ? Stored(Rgb(x, y, 0x5A)) : 0);
		CHECK(bad == 0, "%ux%u image: %u pixels off", w, h, bad);
	}
}

static void Test_AnimationDraws(void) {
	Sim_Setup();

	/* What InitObject leaves for two 4x1 frames - parsing the header needs jsmn, which
	 * only comes with a storage backend */
	WeUi_Gfx_Animation_t anim;
	WeUi_Gfx_Animation_InitStruct(&anim);
	anim.StorageType		 = STORAGE_TYPE_RAM;
	anim.Storage.StorageType = STORAGE_TYPE_RAM;
	anim.Storage.Addr		 = (WeUi_Gfx_StorageAddr_t)ImgPixels;
	anim.Storage.Width		 = 4;
	anim.Storage.Height		 = 1;
	anim.Storage.Size		 = sizeof(ImgPixels);
	anim.FrameSize			 = 4 * WEUI_IMG_STORAGE_COLOR_SIZE;
	anim.FramesNum			 = anim.Storage.Size / anim.FrameSize;

	WeUi_Gfx_Point_t pt = {0, 0};
	WeUi_Gfx_Animation_DrawFrame(1, &pt, &anim, NULL, false);
	WeUi_Gfx_Adapter_DrawFrame();
	CHECK(CountLit() == 2, "frame 1 lit %u of 2 px", CountLit());

	WeUi_Gfx_Animation_t empty;
	WeUi_Gfx_Animation_InitStruct(&empty);
	WeUi_Gfx_Animation_DrawFrame(0, &pt, &empty, NULL, false);
}

#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */

// =================== Effects ===================

static u32 PaintCalls;
static void Paint_Count(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	if (++PaintCalls > UINT16_MAX) {
		printf("    FAIL: RunSolo kept painting past %u frames (Frame=%u)\n", UINT16_MAX,
			   pcCtx->Frame);
		exit(1);
	}
}

static void Test_EffectMaxFrames(void) {
	Sim_Setup();
	PaintCalls = 0;
	WeUi_Gfx_Effect_RunSolo(Paint_Count, UINT16_MAX, NULL);
	CHECK(PaintCalls == UINT16_MAX, "painted %u frames", PaintCalls);
}

static void Render_Blue(void) {
	WeUi_Gfx_Adapter_FillDrawingFrame(Rgb(0, 0, 255));
}

static void Test_PushRejectsBadCfg(void) {
	Sim_Setup();
	static const WeUi_Gfx_Effect_PushCfg_t bad = {
		.OvershootShare = -2.0f,
		.OvershootMinPx = 2.0f,
		.OvershootMaxPx = 1.0f,
		.ApproachShare	= 0.4f,
		.BounceKeep		= 0.35f,
		.Bounces		= 2,
	};
	RET_STATE_t rs = WeUi_Gfx_Effect_Push(Render_Blue, 8, WEUI_GFX_EFFECT_SLIDE_RIGHT, &bad);
	CHECK(rs == RET_STATE_ERR_PARAM, "negative overshoot accepted, rs %d", rs);
}

static void Test_EffectFrames(void) {
	CHECK(WeUi_Gfx_Effect_Frames(1500, 60) == 90, "1.5 s at 60 Hz: %u",
		  WeUi_Gfx_Effect_Frames(1500, 60));
	CHECK(WeUi_Gfx_Effect_Frames(650, 30) == 20, "rounded: %u", WeUi_Gfx_Effect_Frames(650, 30));
	CHECK(WeUi_Gfx_Effect_Frames(0, 60) == WEUI_GFX_EFFECT_FRAMES_MIN, "a zero length plays: %u",
		  WeUi_Gfx_Effect_Frames(0, 60));
	CHECK(WeUi_Gfx_Effect_Frames(UINT32_MAX, 120) == UINT16_MAX, "clamped: %u",
		  WeUi_Gfx_Effect_Frames(UINT32_MAX, 120));
}

static void Test_PushLandsExactly(void) {
	static const WEUI_GFX_EFFECT_SLIDE_t slides[] = {
		WEUI_GFX_EFFECT_SLIDE_RIGHT, WEUI_GFX_EFFECT_SLIDE_LEFT, WEUI_GFX_EFFECT_SLIDE_DOWN,
		WEUI_GFX_EFFECT_SLIDE_UP};

	for (u32 s = 0; s < NUM_ELEMENTS(slides); s++) {
		Sim_Setup();
		WeUi_Gfx_Adapter_FillDrawingFrame(Rgb(255, 0, 0));
		WeUi_Gfx_Adapter_DrawFrame();
		WeUi_Gfx_Effect_Push(Render_Blue, 12, slides[s], NULL);

		u32 blue = Stored(Rgb(0, 0, 255));
		CHECK(PixelAt(0, 0) == blue &&
				  PixelAt(WEUI_DISPLAY_WIDTH - 1, WEUI_DISPLAY_HEIGHT - 1) == blue,
			  "slide %u did not land on the incoming picture", s);
	}
}

// =================== Scene manager, turned like a host loop minus the sleeping ===================

#define TEST_SCENEMGR_TURNS 400

static u32 OverlayCalls;

static void SceneA_Render(WeUi_Gfx_Point_t* pRefPt) {
	WeUi_Gfx_Circle_t c;
	WeUi_Gfx_Circle_InitStruct(&c);
	c.Base.PtOne = (WeUi_Gfx_Point_t){WeUi_Gfx_Adapter_GetX_Mid(), WeUi_Gfx_Adapter_GetY_Mid()};
	c.Radius	 = 1 + WeUi_Gfx_FpsCtrl_GetRenderCycle() % 5;
	c.Base.Color = Rgb(0, 255, 0);
	WeUi_Gfx_Circle_Draw(&c, false);
}

static void SceneB_Render(WeUi_Gfx_Point_t* pRefPt) {
	WeUi_Gfx_Frame_Fade(0.7f);
	WeUi_Gfx_ColorPoint_t cpt = {
		.Pt	   = {(WeUi_Gfx_PtPos_t)(rand() % WEUI_DISPLAY_WIDTH),
				  (WeUi_Gfx_PtPos_t)(rand() % WEUI_DISPLAY_HEIGHT)},
		.Color = Rgb(255, 255, 0),
	};
	WeUi_Gfx_Point_Draw(&cpt);
}

static void Transition_PushLeft(VoidFunc_t renderIncoming) {
	WeUi_Gfx_Effect_Push(renderIncoming, 16, WEUI_GFX_EFFECT_SLIDE_LEFT, NULL);
}

/* B slides in, A cuts back */
static u32 Scene_OnInput(const void* pcInput) {
	u8 next = *(const u8*)pcInput;

	return (next == 1) ? WeUi_Gfx_SceneMgr_GoTo(next, Transition_PushLeft) : next;
}

static void Overlay_Dot(WeUi_Gfx_Point_t* pRefPt) {
	OverlayCalls++;
	WeUi_Gfx_ColorPoint_t cpt = {.Pt = {0, 0}, .Color = Rgb(255, 0, 255)};
	WeUi_Gfx_Point_Draw(&cpt);
}

static const WeUi_Gfx_SceneVtbl_t SceneA = {
	.Render = SceneA_Render, .OnInput = Scene_OnInput, .Fps = 20};
static const WeUi_Gfx_SceneVtbl_t SceneB = {
	.Render = SceneB_Render, .OnInput = Scene_OnInput, .Fps = 30, .KeepFrame = true};
static const WeUi_Gfx_SceneVtbl_t* const Scenes[] = {&SceneA, &SceneB};

static void Test_SceneMgr(void) {
	Sim_Setup();
	Record_Start("scenemgr");
	OverlayCalls = 0;

	WeUi_Gfx_SceneMgrCfg_t cfg = {
		.Scenes = Scenes, .SceneCount = NUM_ELEMENTS(Scenes), .Overlay = Overlay_Dot};
	WeUi_Gfx_SceneMgr_Init(&cfg);

	for (u32 turn = 0; turn < TEST_SCENEMGR_TURNS; turn++) {
		if (turn == 100 || turn == 250) {
			u8 next = (turn == 100) ? 1 : 0;
			WeUi_Gfx_SceneMgr_PostInput(&next, sizeof(next));
		}
		u32 sleepMs = WeUi_Gfx_SceneMgr_Poll();
		Sim_DelayMs((sleepMs == WEUI_GFX_SCENEMGR_SLEEP_FOREVER || !sleepMs) ? 1 : sleepMs);
	}

	CHECK(OverlayCalls + 1 >= DisplaySim_GetFramesPushed(), "overlay drawn %u times, %u frames",
		  OverlayCalls, DisplaySim_GetFramesPushed());
	CHECK(PixelAt(0, 0) == Stored(Rgb(255, 0, 255)), "overlay missing on the last frame");
	Record_Print();
}

/* Scenes with no OnInput of their own: the inputs go to the cfg's. An input names a
 * target and how to reach it - see Nav_OnInput */
static const WeUi_Gfx_SceneVtbl_t SceneBare			  = {.Render = SceneA_Render, .Fps = 20};
static const WeUi_Gfx_SceneVtbl_t* const BareScenes[] = {&SceneBare, &SceneBare};

static u32 TransitionCalls;
static u32 TransitionScene;

static void Transition_Count(VoidFunc_t renderIncoming) {
	TransitionCalls++;
	TransitionScene = WeUi_Gfx_SceneMgr_GetCurScene();
	renderIncoming();
}

typedef enum {
	NAV_GO_WITH = 0, /* to scene 1, with the transition */
	NAV_GO_BARE,	 /* to scene 1, a bare index */
	NAV_STAY,		 /* a transition chosen, then SELF returned */
} Nav_t;

static u32 Nav_OnInput(const void* pcInput) {
	switch (*(const u8*)pcInput) {
		case NAV_GO_WITH:
			return WeUi_Gfx_SceneMgr_GoTo(1, Transition_Count);
		case NAV_STAY:
			WeUi_Gfx_SceneMgr_GoTo(1, Transition_Count);
			return WEUI_GFX_SCENEMGR_SELF;
		case NAV_GO_BARE:
		default:
			return 1;
	}
}

static void Nav_Post(Nav_t nav) {
	u8 raw = (u8)nav;
	WeUi_Gfx_SceneMgr_PostInput(&raw, sizeof(raw));
	WeUi_Gfx_SceneMgr_Poll();
}

static void Test_SceneMgrTransitions(void) {
	Sim_Setup();
	TransitionCalls = 0;

	WeUi_Gfx_SceneMgrCfg_t cfg = {.Scenes	  = BareScenes,
								  .SceneCount = NUM_ELEMENTS(BareScenes),
								  .OnInput	  = Nav_OnInput,
								  .Intro	  = Transition_Count};
	WeUi_Gfx_SceneMgr_Init(&cfg);

	WeUi_Gfx_SceneMgr_Poll();
	CHECK(TransitionCalls == 1 && TransitionScene == 0, "intro: %u calls, into scene %u",
		  TransitionCalls, TransitionScene);

	Nav_Post(NAV_GO_WITH);
	CHECK(TransitionCalls == 2 && TransitionScene == 1, "GoTo: %u calls, into scene %u",
		  TransitionCalls, TransitionScene);

	/* Chosen, but the switch never came: it must not ride on the next one */
	Nav_Post(NAV_STAY);
	Nav_Post(NAV_GO_BARE);
	CHECK(TransitionCalls == 2, "a dropped GoTo played later, %u calls", TransitionCalls);
	CHECK(WeUi_Gfx_SceneMgr_GetCurScene() == 1, "bare index did not switch");

	WeUi_Gfx_SceneMgr_PlayIntoActive(Transition_Count);
	CHECK(TransitionCalls == 3 && TransitionScene == 1, "PlayIntoActive: %u calls, scene %u",
		  TransitionCalls, TransitionScene);
}

#define TEST_TRACE_MAX 16

static WeUi_Gfx_SceneMgr_Trace_t Traced[TEST_TRACE_MAX];
static u32 TracedNum;

static void Trace_Keep(const WeUi_Gfx_SceneMgr_Trace_t* pcTrace) {
	if (TracedNum < TEST_TRACE_MAX)
		Traced[TracedNum] = *pcTrace;
	TracedNum++;
}

static void Action_Nothing(void) {
}

static bool Traced_Is(u32 idx, WeUi_Gfx_SceneMgr_TraceKind_t kind, u32 scene) {
	return idx < TracedNum && Traced[idx].Kind == kind && Traced[idx].Scene == scene;
}

static void Test_SceneMgrTrace(void) {
	Sim_Setup();
	TracedNum = 0;

	WeUi_Gfx_SceneMgrCfg_t cfg = {
		.Scenes		= BareScenes,
		.SceneCount = NUM_ELEMENTS(BareScenes),
		.OnInput	= Nav_OnInput,
	};
	WeUi_Gfx_SceneMgr_Init(&cfg);
	WeUi_Gfx_SceneMgr_SetTrace(Trace_Keep);

	WeUi_Gfx_SceneMgr_Poll();
	CHECK(TracedNum == 2 && Traced_Is(0, WEUI_GFX_SCENEMGR_TRACE_SWITCH, 0) &&
			  Traced[0].Cause == WEUI_GFX_SCENEMGR_CAUSE_HOME &&
			  Traced_Is(1, WEUI_GFX_SCENEMGR_TRACE_SHOWN, 0),
		  "home: %u steps", TracedNum);

	TracedNum = 0;
	Nav_Post(NAV_GO_WITH);
	CHECK(TracedNum == 4 && Traced_Is(0, WEUI_GFX_SCENEMGR_TRACE_ROUTED, 1) &&
			  Traced_Is(1, WEUI_GFX_SCENEMGR_TRACE_SWITCH, 1) && Traced[1].From == 0 &&
			  Traced[1].Transition == Transition_Count &&
			  Traced_Is(2, WEUI_GFX_SCENEMGR_TRACE_PLAY, 1) &&
			  Traced_Is(3, WEUI_GFX_SCENEMGR_TRACE_SHOWN, 1) &&
			  Traced[3].Cause == WEUI_GFX_SCENEMGR_CAUSE_INPUT,
		  "switch by input: %u steps", TracedNum);

	TracedNum = 0;
	WeUi_Gfx_SceneMgr_PostAction(Action_Nothing);
	WeUi_Gfx_SceneMgr_Poll();
	CHECK(TracedNum == 2 && Traced[0].Kind == WEUI_GFX_SCENEMGR_TRACE_ACTION &&
			  Traced[0].Action == Action_Nothing &&
			  Traced[1].Kind == WEUI_GFX_SCENEMGR_TRACE_ACTION_DONE,
		  "action: %u steps", TracedNum);

	TracedNum = 0;
	WeUi_Gfx_SceneMgr_PlayIntoActive(Transition_Count);
	CHECK(TracedNum == 2 && Traced_Is(0, WEUI_GFX_SCENEMGR_TRACE_PLAY, 1) &&
			  Traced[1].Cause == WEUI_GFX_SCENEMGR_CAUSE_HANDBACK,
		  "hand-back: %u steps", TracedNum);

	WeUi_Gfx_SceneMgr_SetTrace(NULL);
}

/* Scene 0 lasts TEST_TIMEOUT_MS, then OnTimeout sends it to scene 1, which has no
 * timeout. Both draw once, so Poll() sleeps until the timeout and no longer */
#define TEST_TIMEOUT_MS 100

static const WeUi_Gfx_SceneVtbl_t SceneStill				= {.Render = SceneA_Render};
static const WeUi_Gfx_SceneVtbl_t* const StillScenes[]		= {&SceneStill, &SceneStill};
static const u32 StillTimeoutsMs[NUM_ELEMENTS(StillScenes)] = {TEST_TIMEOUT_MS, 0};

static u32 Timeout_ToOne(void) {
	return 1;
}

/* Any input enters the active scene again */
static u32 Reenter_OnInput(const void* pcInput) {
	return WeUi_Gfx_SceneMgr_GetCurScene();
}

static void Test_SceneMgrTimeouts(void) {
	Sim_Setup();

	WeUi_Gfx_SceneMgrCfg_t cfg = {
		.Scenes		= StillScenes,
		.SceneCount = NUM_ELEMENTS(StillScenes),
		.OnInput	= Reenter_OnInput,
		.TimeoutsMs = StillTimeoutsMs,
		.OnTimeout	= Timeout_ToOne,
	};
	WeUi_Gfx_SceneMgr_Init(&cfg);

	u32 sleepMs = WeUi_Gfx_SceneMgr_Poll();
	CHECK(sleepMs > 0 && sleepMs <= TEST_TIMEOUT_MS, "slept %u ms, not until the timeout", sleepMs);

	/* Halfway, a re-entry starts the time over */
	Sim_DelayMs(TEST_TIMEOUT_MS / 2);
	u8 any = 0;
	WeUi_Gfx_SceneMgr_PostInput(&any, sizeof(any));
	WeUi_Gfx_SceneMgr_Poll();
	Sim_DelayMs(TEST_TIMEOUT_MS * 3 / 4);
	WeUi_Gfx_SceneMgr_Poll();
	CHECK(WeUi_Gfx_SceneMgr_GetCurScene() == 0, "timed out despite the re-entry");

	Sim_DelayMs(TEST_TIMEOUT_MS / 2);
	sleepMs = WeUi_Gfx_SceneMgr_Poll();
	CHECK(WeUi_Gfx_SceneMgr_GetCurScene() == 1, "no switch once the time was up");
	CHECK(sleepMs == WEUI_GFX_SCENEMGR_SLEEP_FOREVER, "scene 1 has no timeout, slept %u ms",
		  sleepMs);

	/* Handing the panel back starts it over as well */
	WeUi_Gfx_SceneMgr_RequestScene(0);
	WeUi_Gfx_SceneMgr_Poll();
	Sim_DelayMs(TEST_TIMEOUT_MS * 3 / 4);
	WeUi_Gfx_SceneMgr_PlayIntoActive(NULL);
	Sim_DelayMs(TEST_TIMEOUT_MS * 3 / 4);
	WeUi_Gfx_SceneMgr_Poll();
	CHECK(WeUi_Gfx_SceneMgr_GetCurScene() == 0, "PlayIntoActive did not start the time over");
}

// =================== QR ===================

#if WEUI_USE_COMPONENT__WIDGET_QR
static void Test_Qr(void) {
	Sim_Setup();
	Record_Start("qr");
	WeUi_Gfx_Point_t pt = {0, 0};
	WeUi_Gfx_Widget_QR(&pt, 1, WEUI_QR_ECC_LOW, "12345678-12345678-12345678",
					   WeUi_Color_GetFullHsb(180.0f));
	WeUi_Gfx_Adapter_DrawFrame();
	SavePng("qr", Shown());
	CHECK(CountLit() > 0, "QR drew nothing");
	Record_Print();

	/* Needs a symbol larger than the panel: drawn clipped it would be unreadable */
	Sim_Setup();
	WeUi_Gfx_Widget_QR(&pt, 1, WEUI_QR_ECC_HIGH,
					   "https://example.com/a/rather/long/link/that/needs/a/big/symbol",
					   WeUi_Color_GetFullHsb(180.0f));
	WeUi_Gfx_Adapter_DrawFrame();
	CHECK(CountLit() == 0, "oversized QR drew %u px", CountLit());
}
#endif /* WEUI_USE_COMPONENT__WIDGET_QR */

// =================== Golden runs ===================

typedef struct {
	const char* Name;
	VoidFunc_t Fn;
} Demo_t;

static void Test_Demos(void) {
	static const Demo_t demos[] = {
		{"primitives", WeUi_DrawDemo_Primitives},
		{"cube", WeUi_DrawDemo_CubeRotation},
		{"fonts", WeUi_DrawDemo_Fonts},
		{"font_transforms", WeUi_DrawDemo_FontTransforms},
		{"icons", WeUi_DrawDemo_Icons},
		{"transitions", WeUi_DrawDemo_Transitions},
		{"trails", WeUi_DrawDemo_Trails},
		{"fractal_tree", WeUi_DrawDemo_FractalTree},
		{"life", WeUi_DrawDemo_Life},
		{"effects", WeUi_DrawDemo_Effects},
		{"fields", WeUi_DrawDemo_Fields},
		{"affine", WeUi_DrawDemo_Affine},
		{"paths", WeUi_DrawDemo_Paths},
		{"fills", WeUi_DrawDemo_Fills},
		{"gradients", WeUi_DrawDemo_Gradients},
		{"strokes", WeUi_DrawDemo_Strokes},
		{"eyes", WeUi_DrawDemo_Eyes},
		{"scene", WeUi_DrawDemo_Scene},
#if WEUI_USE_COMPONENT__WIDGET_QR
		{"qr_demo", WeUi_DrawDemo_Qr},
#endif /* WEUI_USE_COMPONENT__WIDGET_QR */
	};

	for (u32 i = 0; i < NUM_ELEMENTS(demos); i++) {
		Sim_Setup();
		Record_Start(demos[i].Name);
		demos[i].Fn();
		Record_Print();
	}
}

// =================== Vector ===================

#define TEST_AFFINE_TOLERANCE 1e-4f

static bool NearPt(WeUi_Gfx_PointF_t pt, float x, float y) {
	return fabsf(pt.X - x) < TEST_AFFINE_TOLERANCE && fabsf(pt.Y - y) < TEST_AFFINE_TOLERANCE;
}

static bool NearIdentity(const WeUi_Gfx_Affine_t* pcM) {
	const WeUi_Gfx_Affine_t id = WeUi_Gfx_Affine_Identity();
	const float got[]		   = {pcM->A, pcM->B, pcM->C, pcM->D, pcM->E, pcM->F};
	const float want[]		   = {id.A, id.B, id.C, id.D, id.E, id.F};
	for (u32 i = 0; i < NUM_ELEMENTS(got); i++) {
		if (fabsf(got[i] - want[i]) >= TEST_AFFINE_TOLERANCE)
			return false;
	}
	return true;
}

/* Translate, rotate a quarter turn, scale: the last one set acts on the point first */
static WeUi_Gfx_Affine_t ComposedAffine(void) {
	WeUi_Gfx_AffineStack_t stack;
	WeUi_Gfx_AffineStack_Reset(&stack);
	WeUi_Gfx_AffineStack_Translate(&stack, 10.0f, 20.0f);
	WeUi_Gfx_AffineStack_Rotate(&stack, M_PIf / 2.0f);
	WeUi_Gfx_AffineStack_Scale(&stack, 2.0f, 3.0f);
	return stack.Top;
}

static void Test_AffineCompose(void) {
	WeUi_Gfx_Affine_t id = WeUi_Gfx_Affine_Identity();
	WeUi_Gfx_PointF_t pt = WeUi_Gfx_Affine_Apply(&id, (WeUi_Gfx_PointF_t){3.5f, -2.0f});
	CHECK(NearPt(pt, 3.5f, -2.0f), "identity moved the point to %f, %f", pt.X, pt.Y);

	/* y points down, so a positive angle turns x towards y: clockwise on the panel */
	WeUi_Gfx_Affine_t rot = WeUi_Gfx_Affine_Rotate(M_PIf / 2.0f);
	pt					  = WeUi_Gfx_Affine_Apply(&rot, (WeUi_Gfx_PointF_t){1.0f, 0.0f});
	CHECK(NearPt(pt, 0.0f, 1.0f), "quarter turn took (1, 0) to %f, %f", pt.X, pt.Y);

	WeUi_Gfx_Affine_t move	= WeUi_Gfx_Affine_Translate(10.0f, 0.0f);
	WeUi_Gfx_Affine_t twice = WeUi_Gfx_Affine_Scale(2.0f, 2.0f);
	WeUi_Gfx_Affine_t mul	= WeUi_Gfx_Affine_Mul(&move, &twice);
	pt						= WeUi_Gfx_Affine_Apply(&mul, (WeUi_Gfx_PointF_t){1.0f, 1.0f});
	CHECK(NearPt(pt, 12.0f, 2.0f), "move * scale did not scale first: %f, %f", pt.X, pt.Y);

	/* (1, 1) scaled to (2, 3), turned to (-3, 2), moved to (7, 22) */
	WeUi_Gfx_Affine_t m = ComposedAffine();
	pt					= WeUi_Gfx_Affine_Apply(&m, (WeUi_Gfx_PointF_t){1.0f, 1.0f});
	CHECK(NearPt(pt, 7.0f, 22.0f), "composed transform gave %f, %f", pt.X, pt.Y);
}

static void Test_AffineInvert(void) {
	WeUi_Gfx_Affine_t m = ComposedAffine();
	WeUi_Gfx_Affine_t inv;
	CHECK(WeUi_Gfx_Affine_Invert(&m, &inv) == RET_STATE_SUCCESS, "composed transform not inverted");

	WeUi_Gfx_Affine_t both = WeUi_Gfx_Affine_Mul(&m, &inv);
	CHECK(NearIdentity(&both), "m * inverse is not identity");
	both = WeUi_Gfx_Affine_Mul(&inv, &m);
	CHECK(NearIdentity(&both), "inverse * m is not identity");

	static const WeUi_Gfx_PointF_t pts[] = {{0.0f, 0.0f}, {1.0f, 1.0f}, {-45.5f, 233.0f}};
	for (u32 i = 0; i < NUM_ELEMENTS(pts); i++) {
		WeUi_Gfx_PointF_t there = WeUi_Gfx_Affine_Apply(&m, pts[i]);
		WeUi_Gfx_PointF_t back	= WeUi_Gfx_Affine_Apply(&inv, there);
		CHECK(NearPt(back, pts[i].X, pts[i].Y), "(%f, %f) came back as %f, %f", pts[i].X, pts[i].Y,
			  back.X, back.Y);
	}

	WeUi_Gfx_Affine_t flat = WeUi_Gfx_Affine_Scale(0.0f, 1.0f);
	WeUi_Gfx_Affine_t kept = WeUi_Gfx_Affine_Translate(1.0f, 2.0f);
	inv					   = kept;
	CHECK(WeUi_Gfx_Affine_Invert(&flat, &inv) == RET_STATE_ERR_PARAM, "a zero scale inverted");
	CHECK(memcmp(&inv, &kept, sizeof(inv)) == 0, "a failed inversion wrote its output");
}

static void Test_AffineStack(void) {
	WeUi_Gfx_AffineStack_t stack;
	WeUi_Gfx_AffineStack_Reset(&stack);
	const WeUi_Gfx_PointF_t one = {1.0f, 1.0f};

	CHECK(WeUi_Gfx_AffineStack_Push(&stack) == RET_STATE_SUCCESS, "first push refused");
	WeUi_Gfx_AffineStack_Translate(&stack, 5.0f, 5.0f);
	CHECK(WeUi_Gfx_AffineStack_Push(&stack) == RET_STATE_SUCCESS, "second push refused");
	WeUi_Gfx_AffineStack_Scale(&stack, 2.0f, 2.0f);

	WeUi_Gfx_PointF_t pt = WeUi_Gfx_Affine_Apply(&stack.Top, one);
	CHECK(NearPt(pt, 7.0f, 7.0f), "nested transform gave %f, %f", pt.X, pt.Y);

	CHECK(WeUi_Gfx_AffineStack_Pop(&stack) == RET_STATE_SUCCESS, "first pop refused");
	pt = WeUi_Gfx_Affine_Apply(&stack.Top, one);
	CHECK(NearPt(pt, 6.0f, 6.0f), "pop did not drop the scale: %f, %f", pt.X, pt.Y);

	CHECK(WeUi_Gfx_AffineStack_Pop(&stack) == RET_STATE_SUCCESS, "second pop refused");
	CHECK(NearIdentity(&stack.Top), "popped back to a non-identity");
	CHECK(WeUi_Gfx_AffineStack_Pop(&stack) == RET_STATE_ERR_EMPTY, "pop past the bottom");
	CHECK(NearIdentity(&stack.Top), "a refused pop changed the top");

	for (u32 i = 0; i < WEUI_GFX_AFFINE_STACK_DEPTH; i++)
		CHECK(WeUi_Gfx_AffineStack_Push(&stack) == RET_STATE_SUCCESS, "push %u refused", i);
	CHECK(WeUi_Gfx_AffineStack_Push(&stack) == RET_STATE_ERR_OVERFLOW, "push past the depth");
}

/* Float rounding on top of the flattening tolerance */
#define TEST_PATH_SLACK		 1e-3f
/* A circle of four cubics strays from the true one by 0.027% of its radius */
#define TEST_PATH_CIRCLE_R	 100.0f
#define TEST_PATH_CIRCLE_ERR 0.03f
#define TEST_PATH_SAMPLES	 500
#define TEST_PATH_SMALL_R	 10.0f

static WeUi_Gfx_Path_t TestPath;

static float DistToSegment(WeUi_Gfx_PointF_t pt, WeUi_Gfx_PointF_t a, WeUi_Gfx_PointF_t b) {
	float dx = b.X - a.X, dy = b.Y - a.Y;
	float len2 = dx * dx + dy * dy;
	float t	   = len2 > 0.0f ? ((pt.X - a.X) * dx + (pt.Y - a.Y) * dy) / len2 : 0.0f;
	t		   = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
	return hypotf(pt.X - (a.X + t * dx), pt.Y - (a.Y + t * dy));
}

/* How far pt is from the outline of contour c, its closing edge included */
static float DistToContour(const WeUi_Gfx_Path_t* pcPath, u32 c, WeUi_Gfx_PointF_t pt) {
	const WeUi_Gfx_PathContour_t* pcC = &pcPath->Contours[c];
	const WeUi_Gfx_PointF_t* pcPts	  = &pcPath->Pts[pcC->Start];
	u32 edges						  = pcC->Closed ? pcC->Num : pcC->Num - 1;
	float best						  = INFINITY;
	for (u32 i = 0; i < edges; i++)
		best = fminf(best, DistToSegment(pt, pcPts[i], pcPts[(i + 1) % pcC->Num]));
	return best;
}

static WeUi_Gfx_PointF_t CubicAt(const WeUi_Gfx_PointF_t p[4], float t) {
	float u				  = 1.0f - t;
	float w[]			  = {u * u * u, 3.0f * u * u * t, 3.0f * u * t * t, t * t * t};
	WeUi_Gfx_PointF_t out = {0.0f, 0.0f};
	for (u32 i = 0; i < 4; i++) {
		out.X += w[i] * p[i].X;
		out.Y += w[i] * p[i].Y;
	}
	return out;
}

/* The farthest the flattened contour 0 is from points of the true cubic p */
static float CubicStray(const WeUi_Gfx_PointF_t p[4]) {
	float worst = 0.0f;
	for (u32 i = 0; i <= TEST_PATH_SAMPLES; i++) {
		WeUi_Gfx_PointF_t onCurve = CubicAt(p, (float)i / TEST_PATH_SAMPLES);
		worst					  = fmaxf(worst, DistToContour(&TestPath, 0, onCurve));
	}
	return worst;
}

static void Test_PathRect(void) {
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	CHECK(WeUi_Gfx_Path_Rect(&TestPath, 1.0f, 2.0f, 5.0f, 7.0f) == RET_STATE_SUCCESS,
		  "rect refused");
	CHECK(TestPath.ContoursNum == 1 && TestPath.Contours[0].Closed,
		  "rect is not one closed contour");
	CHECK(TestPath.PtsNum == 4, "rect has %u points", TestPath.PtsNum);
	CHECK(NearPt(TestPath.Pts[2], 5.0f, 7.0f), "third corner at %f, %f", TestPath.Pts[2].X,
		  TestPath.Pts[2].Y);

	/* Points are stored transformed, read through the stack's top as it is when added */
	WeUi_Gfx_AffineStack_t stack;
	WeUi_Gfx_AffineStack_Reset(&stack);
	WeUi_Gfx_Path_Reset(&TestPath, &stack.Top);
	WeUi_Gfx_Path_MoveTo(&TestPath, 1.0f, 1.0f);
	WeUi_Gfx_AffineStack_Translate(&stack, 10.0f, 20.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 1.0f, 1.0f);
	CHECK(NearPt(TestPath.Pts[0], 1.0f, 1.0f), "first point moved to %f, %f", TestPath.Pts[0].X,
		  TestPath.Pts[0].Y);
	CHECK(NearPt(TestPath.Pts[1], 11.0f, 21.0f), "second point not transformed: %f, %f",
		  TestPath.Pts[1].X, TestPath.Pts[1].Y);
	CHECK(!TestPath.Broken, "a good path is marked broken");
}

static void Test_PathFlatten(void) {
	const WeUi_Gfx_PointF_t s[4] = {{0.0f, 0.0f}, {80.0f, -60.0f}, {20.0f, 90.0f}, {100.0f, 30.0f}};
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_MoveTo(&TestPath, s[0].X, s[0].Y);
	WeUi_Gfx_Path_CubicTo(&TestPath, s[1].X, s[1].Y, s[2].X, s[2].Y, s[3].X, s[3].Y);
	float stray = CubicStray(s);
	CHECK(stray <= WEUI_GFX_PATH_TOLERANCE + TEST_PATH_SLACK, "cubic strays %f px", stray);

	/* y = x^2 / 100 - x as a quadratic: (0, 0) .. (50, -50) .. (100, 0), apex at (50, -25) */
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_MoveTo(&TestPath, 0.0f, 0.0f);
	WeUi_Gfx_Path_QuadTo(&TestPath, 50.0f, -50.0f, 100.0f, 0.0f);
	float apex = DistToContour(&TestPath, 0, (WeUi_Gfx_PointF_t){50.0f, -25.0f});
	CHECK(apex <= WEUI_GFX_PATH_TOLERANCE + TEST_PATH_SLACK, "quad apex %f px off", apex);

	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_MoveTo(&TestPath, 0.0f, 0.0f);
	WeUi_Gfx_Path_CubicTo(&TestPath, 10.0f, 0.0f, 20.0f, 0.0f, 30.0f, 0.0f);
	CHECK(TestPath.PtsNum == 2, "a straight cubic took %u points", TestPath.PtsNum);

	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Ellipse(&TestPath, 0.0f, 0.0f, TEST_PATH_SMALL_R, TEST_PATH_SMALL_R);
	u16 smallPts = TestPath.PtsNum;

	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Ellipse(&TestPath, 0.0f, 0.0f, TEST_PATH_CIRCLE_R, TEST_PATH_CIRCLE_R);
	CHECK(TestPath.PtsNum > smallPts, "a 10x larger circle got %u points, the small one %u",
		  TestPath.PtsNum, smallPts);

	float worst = 0.0f;
	for (u32 i = 0; i < TEST_PATH_SAMPLES; i++) {
		float a					   = 2.0f * M_PIf * (float)i / TEST_PATH_SAMPLES;
		WeUi_Gfx_PointF_t onCircle = {TEST_PATH_CIRCLE_R * cosf(a), TEST_PATH_CIRCLE_R * sinf(a)};
		worst					   = fmaxf(worst, DistToContour(&TestPath, 0, onCircle));
	}
	CHECK(worst <= WEUI_GFX_PATH_TOLERANCE + TEST_PATH_CIRCLE_ERR, "circle strays %f px", worst);
}

static void Test_PathErrors(void) {
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	CHECK(WeUi_Gfx_Path_LineTo(&TestPath, 1.0f, 1.0f) == RET_STATE_ERR_EMPTY, "line from nowhere");
	CHECK(TestPath.Broken && TestPath.PtsNum == 0, "a refused line left a trace");

	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Rect(&TestPath, 0.0f, 0.0f, 1.0f, 1.0f);
	CHECK(WeUi_Gfx_Path_CubicTo(&TestPath, 1, 1, 2, 2, 3, 3) == RET_STATE_ERR_EMPTY,
		  "a curve went on from a closed contour");

	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_MoveTo(&TestPath, 0.0f, 0.0f);
	for (u32 i = 1; i < WEUI_GFX_PATH_PTS_MAX - 1; i++)
		WeUi_Gfx_Path_LineTo(&TestPath, (float)i, 0.0f);
	CHECK(WeUi_Gfx_Path_CubicTo(&TestPath, 0, 100, 100, 100, 0, 0) == RET_STATE_ERR_OVERFLOW,
		  "a curve past the room");
	CHECK(TestPath.PtsNum == WEUI_GFX_PATH_PTS_MAX - 1, "a refused curve added points");
	CHECK(WeUi_Gfx_Path_LineTo(&TestPath, 0.0f, 1.0f) == RET_STATE_SUCCESS, "the last point");
	CHECK(WeUi_Gfx_Path_LineTo(&TestPath, 0.0f, 2.0f) == RET_STATE_ERR_OVERFLOW, "a point past it");

	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	for (u32 i = 0; i < WEUI_GFX_PATH_CONTOURS_MAX; i++)
		WeUi_Gfx_Path_MoveTo(&TestPath, 0.0f, 0.0f);
	CHECK(WeUi_Gfx_Path_MoveTo(&TestPath, 0.0f, 0.0f) == RET_STATE_ERR_OVERFLOW,
		  "a contour past the room");
}

/* A grid any test variant's rasteriser fits: no wider than the narrowest panel built */
#define TEST_GRID_W			  32
#define TEST_GRID_H			  24
/* Coverage is truncated per pixel and per cell: an edge pixel may be one step off either way */
#define TEST_COVER_STEP		  1.0f
#define TEST_COVER_HALF		  128
#define TEST_COVER_HALF_SLACK 2

static_assert(TEST_GRID_W <= WEUI_GFX_RASTER_WIDTH_MAX, "the test grid outgrows the rasteriser");

static u8 Grid[TEST_GRID_H][TEST_GRID_W];
static u32 GridOutside;
static const WeUi_Gfx_ClipRect_t GridClip = {0, 0, TEST_GRID_W, TEST_GRID_H};

static void Grid_Span(void* pCtx, WeUi_Gfx_PtPos_t y, WeUi_Gfx_PtPos_t x, u16 len, const u8* pcCov,
					  u8 cov) {
	const WeUi_Gfx_ClipRect_t* pcClip = pCtx;
	for (u16 i = 0; i < len; i++) {
		s32 px = x + i;
		if (px < pcClip->X0 || px >= pcClip->X1 || y < pcClip->Y0 || y >= pcClip->Y1) {
			GridOutside++;
			continue;
		}
		Grid[y][px] = pcCov ? pcCov[i] : cov;
	}
}

static RET_STATE_t Grid_FillPath(const WeUi_Gfx_Path_t* pcPath, const WeUi_Gfx_ClipRect_t* pcClip) {
	memset(Grid, 0, sizeof(Grid));
	GridOutside = 0;
	return WeUi_Gfx_Raster_Fill(pcPath, pcClip, Grid_Span, (void*)pcClip);
}

static RET_STATE_t Grid_Fill(const WeUi_Gfx_ClipRect_t* pcClip) {
	return Grid_FillPath(&TestPath, pcClip);
}

static float Grid_Area(void) {
	u32 sum = 0;
	for (u32 y = 0; y < TEST_GRID_H; y++)
		for (u32 x = 0; x < TEST_GRID_W; x++)
			sum += Grid[y][x];
	return (float)sum / WEUI_GFX_RASTER_COVER_FULL;
}

/* Shoelace over every contour: what the coverage of the whole path must add up to */
static float PathArea(void) {
	float twice = 0.0f;
	for (u32 c = 0; c < TestPath.ContoursNum; c++) {
		const WeUi_Gfx_PathContour_t* pcC = &TestPath.Contours[c];
		for (u32 i = 0; i < pcC->Num; i++) {
			WeUi_Gfx_PointF_t a = TestPath.Pts[pcC->Start + i];
			WeUi_Gfx_PointF_t b = TestPath.Pts[pcC->Start + (i + 1) % pcC->Num];
			twice += a.X * b.Y - b.X * a.Y;
		}
	}
	return fabsf(twice) / 2.0f;
}

static u32 EdgePixels(void) {
	u32 n = 0;
	for (u32 y = 0; y < TEST_GRID_H; y++)
		for (u32 x = 0; x < TEST_GRID_W; x++)
			n += Grid[y][x] > 0 && Grid[y][x] < WEUI_GFX_RASTER_COVER_FULL;
	return n;
}

static void Test_RasterRect(void) {
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Rect(&TestPath, 2.0f, 3.0f, 7.0f, 6.0f);
	CHECK(Grid_Fill(&GridClip) == RET_STATE_SUCCESS, "rect refused");

	u32 wrong = 0;
	for (u32 y = 0; y < TEST_GRID_H; y++)
		for (u32 x = 0; x < TEST_GRID_W; x++) {
			bool in = x >= 2 && x < 7 && y >= 3 && y < 6;
			wrong += Grid[y][x] != (in ? WEUI_GFX_RASTER_COVER_FULL : 0);
		}
	CHECK(wrong == 0, "a pixel-aligned rect has %u pixels off", wrong);

	/* With a point mid-side the same rect goes the general way, past the quick one */
	static u8 quick[TEST_GRID_H][TEST_GRID_W];
	memcpy(quick, Grid, sizeof(Grid));
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_MoveTo(&TestPath, 2.0f, 3.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 4.0f, 3.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 7.0f, 3.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 7.0f, 6.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 2.0f, 6.0f);
	WeUi_Gfx_Path_Close(&TestPath);
	Grid_Fill(&GridClip);
	CHECK(memcmp(quick, Grid, sizeof(Grid)) == 0, "the quick way and the general one differ");

	/* Four sides along the axes, folded back on themselves: no area, nothing drawn */
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_MoveTo(&TestPath, 2.0f, 3.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 7.0f, 3.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 7.0f, 6.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 7.0f, 3.0f);
	WeUi_Gfx_Path_Close(&TestPath);
	Grid_Fill(&GridClip);
	CHECK(Grid_Area() == 0.0f, "a folded four-sider covered %.2f px", (double)Grid_Area());

	/* Edges halfway through a column cover half of it */
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Rect(&TestPath, 2.5f, 3.0f, 7.5f, 6.0f);
	Grid_Fill(&GridClip);
	CHECK(abs(Grid[4][2] - TEST_COVER_HALF) <= TEST_COVER_HALF_SLACK, "left half-pixel got %u",
		  Grid[4][2]);
	CHECK(abs(Grid[4][7] - TEST_COVER_HALF) <= TEST_COVER_HALF_SLACK, "right half-pixel got %u",
		  Grid[4][7]);
	CHECK(Grid[4][4] == WEUI_GFX_RASTER_COVER_FULL, "inside got %u", Grid[4][4]);
}

static void Test_RasterArea(void) {
	/* A circle off the pixel grid, and a heart-like mix of curves turned on the stack */
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Ellipse(&TestPath, 15.3f, 11.7f, 9.0f, 7.4f);
	Grid_Fill(&GridClip);
	float want = PathArea(), got = Grid_Area();
	CHECK(fabsf(want - got) <=
			  EdgePixels() * TEST_COVER_STEP / WEUI_GFX_RASTER_COVER_FULL + TEST_PATH_SLACK,
		  "ellipse covers %f px, its polygon is %f", got, want);

	WeUi_Gfx_AffineStack_t stack;
	WeUi_Gfx_AffineStack_Reset(&stack);
	WeUi_Gfx_AffineStack_Translate(&stack, 16.0f, 12.0f);
	WeUi_Gfx_AffineStack_Rotate(&stack, 0.7f);
	WeUi_Gfx_Path_Reset(&TestPath, &stack.Top);
	WeUi_Gfx_Path_MoveTo(&TestPath, 0.0f, 9.0f);
	WeUi_Gfx_Path_CubicTo(&TestPath, -6.0f, 4.0f, -10.0f, 0.0f, -10.0f, -4.0f);
	WeUi_Gfx_Path_CubicTo(&TestPath, -10.0f, -10.0f, -2.0f, -11.0f, 0.0f, -5.0f);
	WeUi_Gfx_Path_QuadTo(&TestPath, 10.0f, -12.0f, 0.0f, 9.0f);
	Grid_Fill(&GridClip);
	want = PathArea();
	got	 = Grid_Area();
	CHECK(fabsf(want - got) <=
			  EdgePixels() * TEST_COVER_STEP / WEUI_GFX_RASTER_COVER_FULL + TEST_PATH_SLACK,
		  "turned shape covers %f px, its polygon is %f", got, want);
	CHECK(GridOutside == 0, "%u pixels came outside the clip", GridOutside);
}

static void Test_RasterNonzero(void) {
	/* Two squares the same way round: the overlap is covered once, not twice */
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Rect(&TestPath, 2.0f, 2.0f, 10.0f, 10.0f);
	WeUi_Gfx_Path_Rect(&TestPath, 6.0f, 6.0f, 14.0f, 14.0f);
	Grid_Fill(&GridClip);
	CHECK(Grid[8][8] == WEUI_GFX_RASTER_COVER_FULL, "overlap got %u", Grid[8][8]);

	/* A square inside, the other way round: a hole */
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Rect(&TestPath, 2.0f, 2.0f, 14.0f, 14.0f);
	WeUi_Gfx_Path_MoveTo(&TestPath, 5.0f, 5.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 5.0f, 11.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 11.0f, 11.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 11.0f, 5.0f);
	Grid_Fill(&GridClip);
	CHECK(Grid[8][8] == 0, "hole got %u", Grid[8][8]);
	CHECK(Grid[3][3] == WEUI_GFX_RASTER_COVER_FULL, "ring got %u", Grid[3][3]);
}

static void Test_RasterClip(void) {
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Ellipse(&TestPath, 12.4f, 11.6f, 10.0f, 9.0f);
	Grid_Fill(&GridClip);
	static u8 whole[TEST_GRID_H][TEST_GRID_W];
	memcpy(whole, Grid, sizeof(Grid));

	/* Cut through the middle of the shape: inside the clip every pixel is as it was */
	const WeUi_Gfx_ClipRect_t clip = {9, 5, 20, 15};
	Grid_Fill(&clip);
	u32 wrong = 0;
	for (s32 y = clip.Y0; y < clip.Y1; y++)
		for (s32 x = clip.X0; x < clip.X1; x++)
			wrong += Grid[y][x] != whole[y][x];
	CHECK(wrong == 0, "%u pixels changed under the clip", wrong);
	CHECK(GridOutside == 0, "%u pixels came outside the clip", GridOutside);

	const WeUi_Gfx_ClipRect_t away = {0, 0, 1, 1};
	CHECK(Grid_Fill(&away) == RET_STATE_SUCCESS && Grid_Area() == 0.0f, "shape off the clip drew");
}

static void Test_RasterErrors(void) {
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_LineTo(&TestPath, 1.0f, 1.0f);
	CHECK(Grid_Fill(&GridClip) == RET_STATE_ERR_PARAM, "a broken path was drawn");

	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Rect(&TestPath, 0.0f, 0.0f, 1.0f, 1.0f);
	const WeUi_Gfx_ClipRect_t wide = {0, 0, WEUI_GFX_RASTER_WIDTH_MAX + 1, 1};
	CHECK(WeUi_Gfx_Raster_Fill(&TestPath, &wide, Grid_Span, (void*)&wide) == RET_STATE_ERR_PARAM,
		  "a clip wider than the rasteriser");
}

/* Through the panel: the inside is the colour as the driver stores it, an edge half of it -
 * with brightness scaled once, not again on the blend */
static void Test_FillSolid(void) {
	static const float brights[] = {1.0f, 0.5f};
	WeUi_Gfx_Color_t red		 = Rgb(200, 0, 0);

	for (u32 i = 0; i < NUM_ELEMENTS(brights); i++) {
		Sim_Setup();
		DisplaySim_SetBrightScale(brights[i]);
		WeUi_Gfx_Path_Reset(&TestPath, NULL);
		WeUi_Gfx_Path_Rect(&TestPath, 2.5f, 1.0f, 6.0f, 4.0f);
		CHECK(WeUi_Gfx_Fill_Solid(&TestPath, red, NULL) == RET_STATE_SUCCESS, "fill refused");
		WeUi_Gfx_Adapter_DrawFrame();

		u32 full = DisplaySim_ApplyBright(Stored(red));
		CHECK(PixelAt(4, 2) == full, "inside %06x, want %06x at %.1f", PixelAt(4, 2), full,
			  brights[i]);
		u32 edge = PixelAt(2, 2) & 0xFF, half = (full & 0xFF) / 2;
		CHECK(abs((s32)edge - (s32)half) <= TEST_COVER_HALF_SLACK, "edge %u, want %u at %.1f", edge,
			  half, brights[i]);
		CHECK(PixelAt(1, 2) == 0 && PixelAt(6, 2) == 0, "fill leaked out at %.1f", brights[i]);
	}
	DisplaySim_SetBrightScale(1.0f);
}

/* A gradient's colour is one of 256 and blended in 8 bits: a level or two either way */
#define TEST_PAINT_SLACK 3

static void PanelRect(void) {
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Rect(&TestPath, 0.0f, 0.0f, WEUI_DISPLAY_WIDTH, WEUI_DISPLAY_HEIGHT);
}

static u32 Level(u32 x, u32 y) {
	return PixelAt(x, y) & 0xFF;
}

static bool Near(u32 got, u32 want) {
	return abs((s32)got - (s32)want) <= TEST_PAINT_SLACK;
}

static WeUi_Gfx_PaintStop_t Stops[2];

/* Black to white: the level of a pixel reads its offset */
static WeUi_Gfx_Paint_t Gray(WEUI_GFX_PAINT_t type) {
	Stops[0] = (WeUi_Gfx_PaintStop_t){0.0f, Rgb(0, 0, 0)};
	Stops[1] = (WeUi_Gfx_PaintStop_t){1.0f, Rgb(255, 255, 255)};
	return (WeUi_Gfx_Paint_t){
		.Type	  = type,
		.pcStops  = Stops,
		.StopsNum = NUM_ELEMENTS(Stops),
		.Opacity  = WEUI_GFX_PAINT_OPAQUE,
	};
}

static void Test_PaintLinear(void) {
	Sim_Setup();
	PanelRect();
	WeUi_Gfx_Paint_t paint = Gray(WEUI_GFX_PAINT_LINEAR);
	paint.P1			   = (WeUi_Gfx_PointF_t){WEUI_DISPLAY_WIDTH, 0.0f};
	CHECK(WeUi_Gfx_Fill_Paint(&TestPath, &paint, NULL) == RET_STATE_SUCCESS, "linear refused");
	WeUi_Gfx_Adapter_DrawFrame();

	u32 wrong = 0;
	for (u32 x = 0; x < WEUI_DISPLAY_WIDTH; x++) {
		u32 want = (u32)(((float)x + 0.5f) / WEUI_DISPLAY_WIDTH * 255.0f + 0.5f);
		wrong += !Near(Level(x, 0), want) || !Near(Level(x, WEUI_DISPLAY_HEIGHT - 1), Level(x, 0));
	}
	CHECK(wrong == 0, "%u columns off the ramp", wrong);

	/* The paint's x turned a quarter: on the panel the ramp runs down. cosf(pi / 2) is not quite
	 * 0, so a level may round differently from one side to the other */
	WeUi_Gfx_AffineStack_t stack;
	WeUi_Gfx_AffineStack_Reset(&stack);
	WeUi_Gfx_AffineStack_Rotate(&stack, M_PIf / 2.0f);
	paint.P1   = (WeUi_Gfx_PointF_t){WEUI_DISPLAY_HEIGHT, 0.0f};
	paint.pcXf = &stack.Top;
	WeUi_Gfx_Fill_Paint(&TestPath, &paint, NULL);
	WeUi_Gfx_Adapter_DrawFrame();
	wrong = 0;
	for (u32 y = 0; y < WEUI_DISPLAY_HEIGHT; y++) {
		u32 want = (u32)(((float)y + 0.5f) / WEUI_DISPLAY_HEIGHT * 255.0f + 0.5f);
		wrong += !Near(Level(0, y), want) || !Near(Level(WEUI_DISPLAY_WIDTH - 1, y), Level(0, y));
	}
	CHECK(wrong == 0, "%u rows off the turned ramp", wrong);

#if defined(WEUI_LIB_COLOR_SCHEME__RGB) && defined(WEUI_DISPLAY_COLOR)
	/* Three stops: the middle one rules the middle of the panel */
	const WeUi_Gfx_PaintStop_t rgb[] = {
		{0.0f, Rgb(255, 0, 0)}, {0.5f, Rgb(0, 255, 0)}, {1.0f, Rgb(0, 0, 255)}};
	paint = (WeUi_Gfx_Paint_t){.Type	 = WEUI_GFX_PAINT_LINEAR,
							   .pcStops	 = rgb,
							   .StopsNum = NUM_ELEMENTS(rgb),
							   .P1		 = {WEUI_DISPLAY_WIDTH, 0.0f},
							   .Opacity	 = WEUI_GFX_PAINT_OPAQUE};
	WeUi_Gfx_Fill_Paint(&TestPath, &paint, NULL);
	WeUi_Gfx_Adapter_DrawFrame();
	u32 mid = PixelAt(WEUI_DISPLAY_WIDTH / 2, 0);
	CHECK(((mid >> 8) & 0xFF) > (mid & 0xFF) && ((mid >> 8) & 0xFF) > ((mid >> 16) & 0xFF),
		  "middle of red-green-blue is %06x", mid);
	CHECK(PixelAt(0, 0) & 0xFF, "left of red-green-blue has no red");
#endif
}

static void Test_PaintRadial(void) {
	Sim_Setup();
	PanelRect();
	u32 cx				   = WEUI_DISPLAY_WIDTH / 2;
	u32 cy				   = WEUI_DISPLAY_HEIGHT / 2;
	WeUi_Gfx_Paint_t paint = Gray(WEUI_GFX_PAINT_RADIAL);
	paint.P0			   = (WeUi_Gfx_PointF_t){cx + 0.5f, cy + 0.5f}; /* a pixel's middle */
	paint.Radius		   = WEUI_DISPLAY_HEIGHT / 2.0f;
	CHECK(WeUi_Gfx_Fill_Paint(&TestPath, &paint, NULL) == RET_STATE_SUCCESS, "radial refused");
	WeUi_Gfx_Adapter_DrawFrame();

	CHECK(Level(cx, cy) < 255 / 8, "centre is %u", Level(cx, cy));
	CHECK(Level(0, 0) == 255, "a corner past the radius is %u", Level(0, 0));
	CHECK(Level(cx - 2, cy) < Level(cx - 3, cy) && Level(cx + 2, cy) < Level(cx + 3, cy),
		  "levels do not grow outwards");
}

static void Test_PaintOpacity(void) {
	Sim_Setup();
	PanelRect();
	WeUi_Gfx_Paint_t paint = WeUi_Gfx_Paint_Solid(Rgb(255, 255, 255));
	paint.Opacity		   = 128;
	WeUi_Gfx_Fill_Paint(&TestPath, &paint, NULL);
	WeUi_Gfx_Adapter_DrawFrame();
	CHECK(Near(Level(3, 3), 128), "half-opaque white over black is %u", Level(3, 3));

	paint.Opacity = 0;
	Sim_Setup();
	WeUi_Gfx_Fill_Paint(&TestPath, &paint, NULL);
	WeUi_Gfx_Adapter_DrawFrame();
	CHECK(CountLit() == 0, "a transparent fill lit %u px", CountLit());
}

static void Test_PaintSoft(void) {
	Sim_Setup();
	u32 cx				 = WEUI_DISPLAY_WIDTH / 2;
	u32 cy				 = WEUI_DISPLAY_HEIGHT / 2;
	WeUi_Gfx_Soft_t soft = {
		.Centre	 = {cx + 0.5f, cy + 0.5f}, /* a pixel's middle */
		.Rx		 = 6.0f,
		.Ry		 = 2.0f, /* a 32 x 8 panel holds it, and the row past it */
		.Color	 = Rgb(255, 255, 255),
		.Opacity = WEUI_GFX_PAINT_OPAQUE,
	};
	CHECK(WeUi_Gfx_Fill_Soft(&soft, NULL) == RET_STATE_SUCCESS, "soft refused");
	WeUi_Gfx_Adapter_DrawFrame();

	CHECK(Level(cx, cy) > 240, "soft centre is %u", Level(cx, cy));
	CHECK(Level(cx + 3, cy) < Level(cx, cy) && Level(cx + 3, cy) > 0, "no falloff: %u",
		  Level(cx + 3, cy));
	CHECK(Level(cx - 3, cy) == Level(cx + 3, cy), "lopsided: %u left, %u right", Level(cx - 3, cy),
		  Level(cx + 3, cy));
	CHECK(Level(cx + 7, cy) == 0 && Level(cx, cy + 3) == 0, "soft spot reaches past its radii");
}

static void Test_PaintErrors(void) {
	Sim_Setup();
	PanelRect();
	WeUi_Gfx_Paint_t paint = Gray(WEUI_GFX_PAINT_LINEAR);
	CHECK(WeUi_Gfx_Fill_Paint(&TestPath, &paint, NULL) == RET_STATE_ERR_PARAM, "P0 = P1 drawn");

	paint		   = Gray(WEUI_GFX_PAINT_LINEAR);
	paint.P1	   = (WeUi_Gfx_PointF_t){1.0f, 0.0f};
	paint.StopsNum = 0;
	CHECK(WeUi_Gfx_Fill_Paint(&TestPath, &paint, NULL) == RET_STATE_ERR_PARAM, "no stops drawn");

	paint			= Gray(WEUI_GFX_PAINT_LINEAR);
	paint.P1		= (WeUi_Gfx_PointF_t){1.0f, 0.0f};
	Stops[0].Offset = 0.8f;
	Stops[1].Offset = 0.2f;
	CHECK(WeUi_Gfx_Fill_Paint(&TestPath, &paint, NULL) == RET_STATE_ERR_PARAM, "stops backwards");

	paint = Gray(WEUI_GFX_PAINT_RADIAL);
	CHECK(WeUi_Gfx_Fill_Paint(&TestPath, &paint, NULL) == RET_STATE_ERR_PARAM, "no radius drawn");

	WeUi_Gfx_Affine_t flat = WeUi_Gfx_Affine_Scale(0.0f, 1.0f);
	paint				   = Gray(WEUI_GFX_PAINT_RADIAL);
	paint.Radius		   = 1.0f;
	paint.pcXf			   = &flat;
	CHECK(WeUi_Gfx_Fill_Paint(&TestPath, &paint, NULL) == RET_STATE_ERR_PARAM, "flat xf drawn");

	WeUi_Gfx_Soft_t soft = {.Rx = 0.0f, .Ry = 1.0f, .Opacity = WEUI_GFX_PAINT_OPAQUE};
	CHECK(WeUi_Gfx_Fill_Soft(&soft, NULL) == RET_STATE_ERR_PARAM, "soft with no radius drawn");

	WeUi_Gfx_Adapter_DrawFrame();
	CHECK(CountLit() == 0, "refused paints lit %u px", CountLit());
}

/* With the driver halving brightness a gradient's colours are halved once, edges and all */
static void Test_PaintBright(void) {
	Sim_Setup();
	DisplaySim_SetBrightScale(0.5f);
	PanelRect();
	WeUi_Gfx_Paint_t paint = Gray(WEUI_GFX_PAINT_LINEAR);
	paint.P1			   = (WeUi_Gfx_PointF_t){WEUI_DISPLAY_WIDTH, 0.0f};
	WeUi_Gfx_Fill_Paint(&TestPath, &paint, NULL);
	WeUi_Gfx_Adapter_DrawFrame();

	u32 right = Level(WEUI_DISPLAY_WIDTH - 1, 0);
	u32 want  = DisplaySim_ApplyBright(Stored(Rgb(255, 255, 255))) & 0xFF;
	CHECK(Near(right, want), "right end %u, want about %u", right, want);
	DisplaySim_SetBrightScale(1.0f);
}

/* A stroke's area off its exact value: coverage truncation along its edges and arcs flattened
 * within the tolerance, a little short */
#define TEST_STROKE_SLACK 0.6f

static WeUi_Gfx_Path_t TestStroke;

/* The stroke of TestPath, rasterised into the grid: the area it covers */
static float StrokeArea(const WeUi_Gfx_StrokeStyle_t* pcStyle) {
	CHECK(WeUi_Gfx_Stroke_Build(&TestPath, pcStyle, &TestStroke) == RET_STATE_SUCCESS,
		  "stroke refused");
	CHECK(Grid_FillPath(&TestStroke, &GridClip) == RET_STATE_SUCCESS, "stroke not rasterised");
	return Grid_Area();
}

static bool NearArea(float got, float want) {
	return fabsf(got - want) <= TEST_STROKE_SLACK;
}

/* Round parts are chords within the tolerance of their arcs: short by up to arc x tolerance */
static bool NearArcArea(float got, float want, float arc) {
	return got <= want + TEST_STROKE_SLACK &&
		   want - got <= TEST_STROKE_SLACK + arc * WEUI_GFX_PATH_TOLERANCE;
}

static float Perimeter(const WeUi_Gfx_Path_t* pcPath) {
	float len = 0.0f;
	for (u32 i = 0; i < pcPath->PtsNum; i++) {
		WeUi_Gfx_PointF_t a = pcPath->Pts[i], b = pcPath->Pts[(i + 1) % pcPath->PtsNum];
		len += hypotf(b.X - a.X, b.Y - a.Y);
	}
	return len;
}

static void Test_StrokeCaps(void) {
	WeUi_Gfx_StrokeStyle_t style = WeUi_Gfx_Stroke_Style(4.0f);
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_MoveTo(&TestPath, 5.0f, 10.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 25.0f, 10.0f);

	style.Cap  = WEUI_GFX_CAP_BUTT;
	float butt = StrokeArea(&style);
	CHECK(NearArea(butt, 20.0f * 4.0f), "butt line covers %f", butt);
	CHECK(Grid[8][5] == WEUI_GFX_RASTER_COVER_FULL && Grid[11][24] == WEUI_GFX_RASTER_COVER_FULL &&
			  Grid[7][10] == 0 && Grid[12][10] == 0 && Grid[10][4] == 0 && Grid[10][25] == 0,
		  "butt line is not the rectangle it should be");

	style.Cap	 = WEUI_GFX_CAP_SQUARE;
	float square = StrokeArea(&style);
	CHECK(NearArea(square, 24.0f * 4.0f), "square-capped line covers %f", square);

	style.Cap	= WEUI_GFX_CAP_ROUND;
	float round = StrokeArea(&style);
	CHECK(NearArcArea(round, 20.0f * 4.0f + M_PIf * 4.0f, 2.0f * M_PIf * 2.0f),
		  "round-capped line covers %f", round);

	/* One point: a dot as wide as the stroke */
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_MoveTo(&TestPath, 12.5f, 10.5f);
	float dot = StrokeArea(&style);
	CHECK(NearArcArea(dot, M_PIf * 4.0f, 2.0f * M_PIf * 2.0f), "dot covers %f", dot);
	style.Cap = WEUI_GFX_CAP_BUTT;
	CHECK(StrokeArea(&style) == 0.0f, "a butt-capped point drew");
}

/* Right then down, 15 px each, 4 wide: two 60 px bars sharing 4 px, and the corner's outer
 * square of 2 x 2 - all of it mitered, a quarter disc rounded, half of it bevelled */
static void Test_StrokeJoins(void) {
	WeUi_Gfx_StrokeStyle_t style = WeUi_Gfx_Stroke_Style(4.0f);
	style.Cap					 = WEUI_GFX_CAP_BUTT;
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_MoveTo(&TestPath, 5.0f, 5.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 20.0f, 5.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 20.0f, 20.0f);
	const float body = 60.0f + 60.0f - 4.0f;

	style.Join	= WEUI_GFX_JOIN_MITER;
	float miter = StrokeArea(&style);
	CHECK(NearArea(miter, body + 4.0f), "mitered corner covers %f", miter);
	CHECK(Grid[3][21] == WEUI_GFX_RASTER_COVER_FULL, "miter tip missing");

	style.Join	= WEUI_GFX_JOIN_ROUND;
	float round = StrokeArea(&style);
	CHECK(NearArcArea(round, body + M_PIf, M_PIf), "rounded corner covers %f", round);

	style.Join	= WEUI_GFX_JOIN_BEVEL;
	float bevel = StrokeArea(&style);
	CHECK(NearArea(bevel, body + 2.0f), "bevelled corner covers %f", bevel);

	/* A right angle's miter is sqrt(2) times half the width: a limit under it bevels */
	style.Join		 = WEUI_GFX_JOIN_MITER;
	style.MiterLimit = 1.2f;
	float limited	 = StrokeArea(&style);
	CHECK(NearArea(limited, bevel), "a miter past its limit covers %f", limited);

	/* The same corner the other way round: the inner side, joined through the corner, leaves
	 * no hole there */
	style	   = WeUi_Gfx_Stroke_Style(4.0f);
	style.Cap  = WEUI_GFX_CAP_BUTT;
	style.Join = WEUI_GFX_JOIN_BEVEL;
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_MoveTo(&TestPath, 20.0f, 20.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 20.0f, 5.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 5.0f, 5.0f);
	float back = StrokeArea(&style);
	CHECK(NearArea(back, bevel), "the corner walked back covers %f, %f forth", back, bevel);
	CHECK(Grid[6][19] == WEUI_GFX_RASTER_COVER_FULL, "a hole inside the corner");
}

static void Test_StrokeClosed(void) {
	WeUi_Gfx_StrokeStyle_t style = WeUi_Gfx_Stroke_Style(2.0f);
	style.Join					 = WEUI_GFX_JOIN_MITER;
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Rect(&TestPath, 5.0f, 5.0f, 25.0f, 15.0f);
	float ring = StrokeArea(&style);
	CHECK(NearArea(ring, 22.0f * 12.0f - 18.0f * 8.0f), "rect stroke covers %f", ring);
	CHECK(Grid[10][15] == 0, "rect stroke is not hollow");

	/* A curve: a ring 2 wide round the flattened circle, its perimeter times the width */
	style = WeUi_Gfx_Stroke_Style(2.0f);
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Ellipse(&TestPath, 15.5f, 11.5f, 8.0f, 8.0f);
	float circle = StrokeArea(&style);
	float want	 = 2.0f * Perimeter(&TestPath);
	CHECK(NearArea(circle, want), "circle stroke covers %f, want %f", circle, want);
}

/* A ring the width out from the edge and WEUI_GFX_STROKE_OUTLINE_UNDER in, whichever way the
 * contour runs */
static void Test_StrokeOutline(void) {
	WeUi_Gfx_StrokeStyle_t style = WeUi_Gfx_Stroke_Style(2.0f);
	style.Join					 = WEUI_GFX_JOIN_MITER;
	const float u				 = WEUI_GFX_STROKE_OUTLINE_UNDER;
	const float want			 = 24.0f * 14.0f - (20.0f - 2.0f * u) * (10.0f - 2.0f * u);

	for (u32 back = 0; back < 2; back++) {
		WeUi_Gfx_Path_Reset(&TestPath, NULL);
		if (back) {
			WeUi_Gfx_Path_MoveTo(&TestPath, 5.0f, 5.0f);
			WeUi_Gfx_Path_LineTo(&TestPath, 5.0f, 15.0f);
			WeUi_Gfx_Path_LineTo(&TestPath, 25.0f, 15.0f);
			WeUi_Gfx_Path_LineTo(&TestPath, 25.0f, 5.0f);
			WeUi_Gfx_Path_Close(&TestPath);
		} else {
			WeUi_Gfx_Path_Rect(&TestPath, 5.0f, 5.0f, 25.0f, 15.0f);
		}
		CHECK(WeUi_Gfx_Stroke_Outline(&TestPath, &style, &TestStroke) == RET_STATE_SUCCESS,
			  "outline refused");
		Grid_FillPath(&TestStroke, &GridClip);
		float got = Grid_Area();
		CHECK(NearArea(got, want), "outline %s covers %f, want %f", back ? "backwards" : "", got,
			  want);
		CHECK(Grid[4][10] == WEUI_GFX_RASTER_COVER_FULL && Grid[10][15] == 0,
			  "outline %s is not a ring outside", back ? "backwards" : "");
	}
}

static void Test_StrokeScaleErrors(void) {
	/* Width in the style's coordinates: twice the scale, twice as wide */
	WeUi_Gfx_Affine_t twice		 = WeUi_Gfx_Affine_Scale(2.0f, 2.0f);
	WeUi_Gfx_StrokeStyle_t style = WeUi_Gfx_Stroke_Style(2.0f);
	style.Cap					 = WEUI_GFX_CAP_BUTT;
	style.pcXf					 = &twice;
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_MoveTo(&TestPath, 5.0f, 10.0f);
	WeUi_Gfx_Path_LineTo(&TestPath, 25.0f, 10.0f);
	float wide = StrokeArea(&style);
	CHECK(NearArea(wide, 20.0f * 4.0f), "a 2-wide stroke at scale 2 covers %f", wide);

	style = WeUi_Gfx_Stroke_Style(0.0f);
	CHECK(WeUi_Gfx_Stroke_Build(&TestPath, &style, &TestStroke) == RET_STATE_ERR_PARAM &&
			  TestStroke.PtsNum == 0,
		  "a stroke with no width");

	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_LineTo(&TestPath, 1.0f, 1.0f);
	style = WeUi_Gfx_Stroke_Style(1.0f);
	CHECK(WeUi_Gfx_Stroke_Build(&TestPath, &style, &TestStroke) == RET_STATE_ERR_PARAM,
		  "a broken path stroked");

	/* A zigzag stroke takes more than twice its points: past the room it says so */
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_MoveTo(&TestPath, 0.0f, 0.0f);
	for (u32 i = 1; i < WEUI_GFX_PATH_PTS_MAX; i++)
		WeUi_Gfx_Path_LineTo(&TestPath, (float)i, (float)(i % 2) * 5.0f);
	style.Join = WEUI_GFX_JOIN_MITER;
	CHECK(WeUi_Gfx_Stroke_Build(&TestPath, &style, &TestStroke) == RET_STATE_ERR_OVERFLOW,
		  "a stroke past the room");
}

/* Masks fit the narrowest panel a test variant builds, 32 x 8 */
#define TEST_MASK_BYTES (TEST_GRID_W * TEST_GRID_H)
#define TEST_MASK_HALF	128

static u8 MaskBufs[2][TEST_MASK_BYTES];
static WeUi_Gfx_Mask_t Masks[2];

static void Test_RasterBounds(void) {
	WeUi_Gfx_ClipRect_t b;
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Rect(&TestPath, 2.5f, 3.0f, 7.5f, 6.0f);
	CHECK(WeUi_Gfx_Raster_Bounds(&TestPath, &b) == RET_STATE_SUCCESS && b.X0 == 2 && b.Y0 == 3 &&
			  b.X1 == 8 && b.Y1 == 6,
		  "bounds %d %d %d %d", b.X0, b.Y0, b.X1, b.Y1);

	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_LineTo(&TestPath, 1.0f, 1.0f);
	CHECK(WeUi_Gfx_Raster_Bounds(&TestPath, &b) == RET_STATE_ERR_PARAM, "bounds of a broken path");
}

static void Test_MaskBuild(void) {
	const WeUi_Gfx_ClipRect_t rect = {0, 0, 16, 8};
	WeUi_Gfx_Mask_t* pM			   = &Masks[0];

	CHECK(WeUi_Gfx_Mask_Init(pM, NULL, TEST_MASK_BYTES, &rect) == RET_STATE_ERR_PARAM,
		  "a mask with no buffer");
	const WeUi_Gfx_ClipRect_t empty = {4, 4, 4, 8};
	CHECK(WeUi_Gfx_Mask_Init(pM, MaskBufs[0], TEST_MASK_BYTES, &empty) == RET_STATE_ERR_PARAM,
		  "an empty mask");
	CHECK(WeUi_Gfx_Mask_Init(pM, MaskBufs[0], 16 * 8 - 1, &rect) == RET_STATE_ERR_MEMORY,
		  "a mask past its buffer");
	CHECK(WeUi_Gfx_Mask_Init(pM, MaskBufs[0], TEST_MASK_BYTES, &rect) == RET_STATE_SUCCESS,
		  "mask refused");

	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Rect(&TestPath, 2.0f, 2.0f, 10.0f, 6.0f);
	CHECK(WeUi_Gfx_Mask_AddPath(pM, &TestPath, WEUI_GFX_PAINT_OPAQUE) == RET_STATE_SUCCESS,
		  "add refused");
	CHECK(WeUi_Gfx_Mask_At(pM, 4, 4) == WEUI_GFX_RASTER_COVER_FULL &&
			  WeUi_Gfx_Mask_At(pM, 1, 4) == 0,
		  "added rect is not where it was put");
	CHECK(WeUi_Gfx_Mask_At(pM, 40, 4) == 0 && WeUi_Gfx_Mask_At(pM, -1, -1) == 0,
		  "outside the mask is not 0");

	/* Half, then half again over it: three quarters */
	WeUi_Gfx_Mask_Init(pM, MaskBufs[0], TEST_MASK_BYTES, &rect);
	WeUi_Gfx_Mask_AddPath(pM, &TestPath, TEST_MASK_HALF);
	CHECK(Near(WeUi_Gfx_Mask_At(pM, 4, 4), TEST_MASK_HALF), "half added is %u",
		  WeUi_Gfx_Mask_At(pM, 4, 4));
	WeUi_Gfx_Mask_AddPath(pM, &TestPath, TEST_MASK_HALF);
	CHECK(Near(WeUi_Gfx_Mask_At(pM, 4, 4), 192), "half over half is %u",
		  WeUi_Gfx_Mask_At(pM, 4, 4));

	/* Cut its right half away */
	WeUi_Gfx_Mask_AddPath(pM, &TestPath, WEUI_GFX_PAINT_OPAQUE);
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Rect(&TestPath, 6.0f, 0.0f, 16.0f, 8.0f);
	CHECK(WeUi_Gfx_Mask_CutPath(pM, &TestPath, WEUI_GFX_PAINT_OPAQUE) == RET_STATE_SUCCESS,
		  "cut refused");
	CHECK(WeUi_Gfx_Mask_At(pM, 4, 4) == WEUI_GFX_RASTER_COVER_FULL &&
			  WeUi_Gfx_Mask_At(pM, 8, 4) == 0,
		  "cut went wrong: %u left, %u right", WeUi_Gfx_Mask_At(pM, 4, 4),
		  WeUi_Gfx_Mask_At(pM, 8, 4));
}

/* A disc mask over the left of the panel, the panel filled white through it */
static void MaskDisc(WeUi_Gfx_Mask_t* pM, u8* pBuf, float cx) {
	const WeUi_Gfx_ClipRect_t rect = {0, 0, TEST_GRID_W, 8};
	WeUi_Gfx_Mask_Init(pM, pBuf, TEST_MASK_BYTES, &rect);
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Ellipse(&TestPath, cx, 4.0f, 3.5f, 3.5f);
	WeUi_Gfx_Mask_AddPath(pM, &TestPath, WEUI_GFX_PAINT_OPAQUE);
}

static void FillThrough(const WeUi_Gfx_MaskUse_t* pcUses, u8 usesNum) {
	Sim_Setup();
	PanelRect();
	WeUi_Gfx_Paint_t paint = WeUi_Gfx_Paint_Solid(Rgb(255, 255, 255));
	paint.pcMasks		   = pcUses;
	paint.MasksNum		   = usesNum;
	CHECK(WeUi_Gfx_Fill_Paint(&TestPath, &paint, NULL) == RET_STATE_SUCCESS, "masked fill refused");
	WeUi_Gfx_Adapter_DrawFrame();
}

static void Test_MaskFill(void) {
	MaskDisc(&Masks[0], MaskBufs[0], 8.0f);
	MaskDisc(&Masks[1], MaskBufs[1], 11.0f);

	WeUi_Gfx_MaskUse_t in = {&Masks[0], WEUI_GFX_MASK_IN};
	FillThrough(&in, 1);
	CHECK(Level(8, 4) == 255 && Level(20, 4) == 0 && Level(8, 0) < Level(8, 1),
		  "inside a disc: %u at its centre, %u past it", Level(8, 4), Level(20, 4));
	CHECK(Level(5, 4) == WeUi_Gfx_Mask_At(&Masks[0], 5, 4), "an edge pixel is %u, its mask %u",
		  Level(5, 4), WeUi_Gfx_Mask_At(&Masks[0], 5, 4));

	WeUi_Gfx_MaskUse_t out = {&Masks[0], WEUI_GFX_MASK_OUT};
	FillThrough(&out, 1);
	CHECK(Level(8, 4) == 0 && Level(20, 4) == 255, "outside a disc: %u at its centre, %u past it",
		  Level(8, 4), Level(20, 4));

	/* Two discs both in: where they overlap; one in, one out: the moon */
	WeUi_Gfx_MaskUse_t both[] = {{&Masks[0], WEUI_GFX_MASK_IN}, {&Masks[1], WEUI_GFX_MASK_IN}};
	FillThrough(both, NUM_ELEMENTS(both));
	CHECK(Level(9, 4) == 255 && Level(6, 4) == 0 && Level(13, 4) == 0,
		  "the overlap of two discs: %u %u %u", Level(6, 4), Level(9, 4), Level(13, 4));
	WeUi_Gfx_MaskUse_t moon[] = {{&Masks[0], WEUI_GFX_MASK_IN}, {&Masks[1], WEUI_GFX_MASK_OUT}};
	FillThrough(moon, NUM_ELEMENTS(moon));
	CHECK(Level(6, 4) == 255 && Level(9, 4) == 0 && Level(13, 4) == 0,
		  "a disc less another: %u %u %u", Level(6, 4), Level(9, 4), Level(13, 4));

	/* A soft spot through a mask */
	Sim_Setup();
	WeUi_Gfx_Soft_t soft = {
		.Centre	  = {8.5f, 4.5f},
		.Rx		  = 6.0f,
		.Ry		  = 3.0f,
		.Color	  = Rgb(255, 255, 255),
		.Opacity  = WEUI_GFX_PAINT_OPAQUE,
		.pcMasks  = &out,
		.MasksNum = 1,
	};
	WeUi_Gfx_Fill_Soft(&soft, NULL);
	WeUi_Gfx_Adapter_DrawFrame();
	CHECK(Level(8, 4) == 0 && Level(13, 4) > 0, "a soft spot ignored its mask: %u in, %u out",
		  Level(8, 4), Level(13, 4));

	/* A mask that is not there: refused, nothing drawn */
	Sim_Setup();
	WeUi_Gfx_Paint_t paint = WeUi_Gfx_Paint_Solid(Rgb(255, 255, 255));
	paint.MasksNum		   = 1;
	PanelRect();
	CHECK(WeUi_Gfx_Fill_Paint(&TestPath, &paint, NULL) == RET_STATE_ERR_PARAM, "no masks drawn");
	WeUi_Gfx_Adapter_DrawFrame();
	CHECK(CountLit() == 0, "a refused masked fill lit %u px", CountLit());
}

#define TEST_TARGET_SENTINEL 0xABCDu
#define TEST_TARGET_STRIDE	 20
#define TEST_TARGET_ROWS	 8
#define TEST_BAND_ROWS		 7 /* odd, so bands cut the rasteriser's own ones across */
#define TEST_TARGET_LSB		 2 /* 565 steps a buffer pixel may be off the frame's */

static u16 TargetBuf[TEST_TARGET_ROWS * TEST_TARGET_STRIDE];
static u16 TargetBe[TEST_TARGET_ROWS * TEST_TARGET_STRIDE];
static u16 PanelWhole[WEUI_DISPLAY_WIDTH * WEUI_DISPLAY_HEIGHT];
static u16 PanelBands[WEUI_DISPLAY_WIDTH * WEUI_DISPLAY_HEIGHT];

static u16 Px565(u32 rgb) {
	return (u16)(((rgb & 0xF8) << 8) | (((rgb >> 8) & 0xFC) << 3) | (((rgb >> 16) & 0xFF) >> 3));
}

static WeUi_Gfx_Target_t PanelTarget(u16* pBuf, s32 y0, s32 y1) {
	return (WeUi_Gfx_Target_t){
		.Format = WEUI_GFX_TARGET_RGB565,
		.pBuf	= &pBuf[y0 * WEUI_DISPLAY_WIDTH],
		.Stride = WEUI_DISPLAY_WIDTH,
		.Rect	= {0, (WeUi_Gfx_PtPos_t)y0, WEUI_DISPLAY_WIDTH, (WeUi_Gfx_PtPos_t)y1},
	};
}

#define TEST_SCENE_FRAMES 12
#define TEST_SCENE_RECTS  4

static WeUi_Gfx_Scene_t TestScene;
static WeUi_Gfx_ClipRect_t SceneRects[TEST_SCENE_RECTS];
#if !WEUI_DISPLAY_BANDS
static u32 SceneShot[WEUI_DISPLAY_WIDTH * WEUI_DISPLAY_HEIGHT];
#endif /* !WEUI_DISPLAY_BANDS */

static u8 SceneRect(float x0, float y0, float x1, float y1, WeUi_Gfx_Color_t color) {
	u8 idx = 0;
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Rect(&TestPath, x0, y0, x1, y1);
	WeUi_Gfx_Paint_t paint = WeUi_Gfx_Paint_Solid(color);
	CHECK(WeUi_Gfx_Scene_Fill(&TestScene, &TestPath, &paint, &idx) == RET_STATE_SUCCESS,
		  "scene refused a rect");
	return idx;
}

/* Frame f of a scene with a little of everything, each part moving at its own pace */
static void SceneFrame(u32 f) {
	const float w = WEUI_DISPLAY_WIDTH, h = WEUI_DISPLAY_HEIGHT;
	WeUi_Gfx_Scene_Begin(&TestScene);
	SceneRect(0.0f, 0.0f, w, h, Rgb(10, 10, 30));
	SceneRect(0.05f * w, 0.2f * h, 0.15f * w, 0.8f * h, Rgb(200, 200, 40));

	/* A ball with a radial gradient, rolling */
	const WeUi_Gfx_PaintStop_t stops[] = {{0.0f, Rgb(255, 255, 255)}, {1.0f, Rgb(20, 60, 160)}};
	float bx						   = 0.25f * w + 0.6f * (float)f;
	WeUi_Gfx_Paint_t ball			   = {.Type		= WEUI_GFX_PAINT_RADIAL,
										  .pcStops	= stops,
										  .StopsNum = NUM_ELEMENTS(stops),
										  .P0		= {bx - 1.0f, 0.4f * h},
										  .Radius	= 0.35f * h,
										  .Opacity	= WEUI_GFX_PAINT_OPAQUE};
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Ellipse(&TestPath, bx, 0.5f * h, 0.3f * h, 0.3f * h);
	CHECK(WeUi_Gfx_Scene_Fill(&TestScene, &TestPath, &ball, NULL) == RET_STATE_SUCCESS,
		  "scene refused a gradient");

	/* An eye: an iris inside a disc, a pupil looking about, a crescent cut from a glint */
	u8 disc, notch, iris, pupil, glint;
	float ex = 0.7f * w, ey = 0.5f * h, er = 0.4f * h;
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Ellipse(&TestPath, ex, ey, er, er);
	WeUi_Gfx_Scene_Mask(&TestScene, &TestPath, &disc);
	iris = SceneRect(ex - er, ey - er, ex + er, ey + er, Rgb(60, 200, 80));
	WeUi_Gfx_Scene_Clip(&TestScene, iris, disc, WEUI_GFX_MASK_IN);
	float px = ex + 0.5f * er * sinf(0.7f * (float)f);
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Ellipse(&TestPath, px, ey, 0.5f * er, 0.5f * er);
	WeUi_Gfx_Paint_t dark = WeUi_Gfx_Paint_Solid(Rgb(0, 0, 0));
	WeUi_Gfx_Scene_Fill(&TestScene, &TestPath, &dark, &pupil);
	WeUi_Gfx_Scene_Clip(&TestScene, pupil, disc, WEUI_GFX_MASK_IN);
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Ellipse(&TestPath, px - 1.0f, ey - 1.0f, 0.25f * er, 0.25f * er);
	WeUi_Gfx_Scene_Mask(&TestScene, &TestPath, &notch);
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Ellipse(&TestPath, px - 1.5f, ey - 1.5f, 0.25f * er, 0.25f * er);
	WeUi_Gfx_Paint_t white = WeUi_Gfx_Paint_Solid(Rgb(255, 255, 255));
	WeUi_Gfx_Scene_Fill(&TestScene, &TestPath, &white, &glint);
	WeUi_Gfx_Scene_Clip(&TestScene, glint, disc, WEUI_GFX_MASK_IN);
	WeUi_Gfx_Scene_Clip(&TestScene, glint, notch, WEUI_GFX_MASK_OUT);

	/* A soft spot fading in and out, and a stroke */
	WeUi_Gfx_Soft_t soft = {.Centre	 = {0.45f * w, 0.7f * h},
							.Rx		 = 0.2f * w,
							.Ry		 = 0.2f * h,
							.Color	 = Rgb(255, 120, 160),
							.Opacity = (u8)(f % 4 * 60)};
	CHECK(WeUi_Gfx_Scene_Soft(&TestScene, &soft, NULL) == RET_STATE_SUCCESS,
		  "scene refused a spot");
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_MoveTo(&TestPath, 0.1f * w, 0.9f * h);
	WeUi_Gfx_Path_QuadTo(&TestPath, 0.3f * w, (f % 2 ? 0.6f : 0.95f) * h, 0.5f * w, 0.9f * h);
	WeUi_Gfx_StrokeStyle_t style = WeUi_Gfx_Stroke_Style(1.5f);
	WeUi_Gfx_Stroke_Build(&TestPath, &style, &TestStroke);
	WeUi_Gfx_Scene_Fill(&TestScene, &TestStroke, &white, NULL);
	CHECK(!TestScene.Overflow, "the test scene overflowed");
}

static u32 RectArea(const WeUi_Gfx_ClipRect_t* pcR) {
	return (u32)((pcR->X1 - pcR->X0) * (pcR->Y1 - pcR->Y0));
}

/* Redrawn only where it changed, every frame matches the same frame drawn whole: in the frame,
 * or - out in bands - in a buffer of the panel's pixels */
static void Test_SceneRedraw(void) {
	Sim_Setup();
	WeUi_Gfx_Scene_Init(&TestScene);
	u32 panel = WEUI_DISPLAY_WIDTH * WEUI_DISPLAY_HEIGHT, partial = 0, wrongFrames = 0;

	for (u32 f = 0; f < TEST_SCENE_FRAMES; f++) {
		SceneFrame(f);
		u8 n	 = WeUi_Gfx_Scene_Damage(&TestScene, SceneRects, TEST_SCENE_RECTS);
		u32 area = 0;
		for (u8 i = 0; i < n; i++)
			area += RectArea(&SceneRects[i]);
		partial += f > 0 && area < panel;
		CHECK(WeUi_Gfx_Scene_Present(&TestScene, SceneRects, n) == RET_STATE_SUCCESS,
			  "scene refused to present");

#if WEUI_DISPLAY_BANDS
		WeUi_Gfx_Target_t whole = PanelTarget(PanelWhole, 0, WEUI_DISPLAY_HEIGHT);
		WeUi_Gfx_Scene_DrawTo(&TestScene, &whole);
		u32 off = 0;
		for (u32 i = 0; i < panel; i++)
			off += Px565(Shown()[i]) != PanelWhole[i];
		wrongFrames += off != 0;
#else
		WeUi_Gfx_Adapter_DrawFrame();
		memcpy(SceneShot, Shown(), sizeof(SceneShot));
		WeUi_Gfx_Adapter_ClearDrawingFrame();
		WeUi_Gfx_Scene_Draw(&TestScene, NULL);
		WeUi_Gfx_Adapter_DrawFrame();
		wrongFrames += memcmp(SceneShot, Shown(), sizeof(SceneShot)) != 0;
#endif /* WEUI_DISPLAY_BANDS */
	}
	CHECK(wrongFrames == 0, "%u frames redrawn in part differ from whole ones", wrongFrames);
	CHECK(partial > 0, "no frame was redrawn in part");
}

static void Test_SceneDamage(void) {
	Sim_Setup();
	const float w = WEUI_DISPLAY_WIDTH, h = WEUI_DISPLAY_HEIGHT;
	WeUi_Gfx_ClipRect_t panel = WeUi_Gfx_Fill_PanelClip();
	WeUi_Gfx_Scene_Init(&TestScene);

	WeUi_Gfx_Scene_Begin(&TestScene);
	SceneRect(0.0f, 0.0f, w, h, Rgb(0, 0, 0));
	SceneRect(2.0f, 1.0f, 6.0f, 5.0f, Rgb(255, 0, 0));
	u8 n = WeUi_Gfx_Scene_Damage(&TestScene, SceneRects, TEST_SCENE_RECTS);
	CHECK(n == 1 && memcmp(&SceneRects[0], &panel, sizeof(panel)) == 0,
		  "the first frame is not the whole panel");

	WeUi_Gfx_Scene_Begin(&TestScene);
	SceneRect(0.0f, 0.0f, w, h, Rgb(0, 0, 0));
	SceneRect(2.0f, 1.0f, 6.0f, 5.0f, Rgb(255, 0, 0));
	CHECK(WeUi_Gfx_Scene_Damage(&TestScene, SceneRects, TEST_SCENE_RECTS) == 0,
		  "the same frame again changed something");

	/* The same rect from another corner: the same fingerprint */
	WeUi_Gfx_Scene_Begin(&TestScene);
	SceneRect(0.0f, 0.0f, w, h, Rgb(0, 0, 0));
	SceneRect(6.0f, 5.0f, 2.0f, 1.0f, Rgb(255, 0, 0));
	CHECK(WeUi_Gfx_Scene_Damage(&TestScene, SceneRects, TEST_SCENE_RECTS) == 0,
		  "a rect drawn from another corner counts as changed");

	/* Moved by 3: one rect over where it was and where it is */
	WeUi_Gfx_Scene_Begin(&TestScene);
	SceneRect(0.0f, 0.0f, w, h, Rgb(0, 0, 0));
	SceneRect(5.0f, 1.0f, 9.0f, 5.0f, Rgb(255, 0, 0));
	n = WeUi_Gfx_Scene_Damage(&TestScene, SceneRects, TEST_SCENE_RECTS);
	CHECK(n == 1 && SceneRects[0].X0 == 2 && SceneRects[0].X1 == 9 && SceneRects[0].Y0 == 1 &&
			  SceneRects[0].Y1 == 5,
		  "a moved rect damaged %u rects, the first %d %d %d %d", n, SceneRects[0].X0,
		  SceneRects[0].Y0, SceneRects[0].X1, SceneRects[0].Y1);

	/* Aligned to 2: odd edges go out to even ones */
	TestScene.DamageAlign = 2;
	WeUi_Gfx_Scene_Begin(&TestScene);
	SceneRect(0.0f, 0.0f, w, h, Rgb(0, 0, 0));
	SceneRect(5.0f, 1.0f, 9.0f, 3.0f, Rgb(255, 0, 0));
	n = WeUi_Gfx_Scene_Damage(&TestScene, SceneRects, TEST_SCENE_RECTS);
	CHECK(n == 1 && SceneRects[0].X0 == 4 && SceneRects[0].X1 == 10 && SceneRects[0].Y0 == 0 &&
			  SceneRects[0].Y1 == 6,
		  "an aligned rect is %d %d %d %d", SceneRects[0].X0, SceneRects[0].Y0, SceneRects[0].X1,
		  SceneRects[0].Y1);

	/* One more item: the whole panel */
	WeUi_Gfx_Scene_Begin(&TestScene);
	SceneRect(0.0f, 0.0f, w, h, Rgb(0, 0, 0));
	SceneRect(5.0f, 1.0f, 9.0f, 3.0f, Rgb(255, 0, 0));
	SceneRect(1.0f, 1.0f, 2.0f, 2.0f, Rgb(0, 255, 0));
	n = WeUi_Gfx_Scene_Damage(&TestScene, SceneRects, TEST_SCENE_RECTS);
	CHECK(n == 1 && RectArea(&SceneRects[0]) == RectArea(&panel),
		  "a new item is not the whole panel");
}

/* An eye's opening closing: its lids move by the same each way, which a weak fingerprint misses
 * where the moves cancel out. Every step, wherever the eye is, has to count as a change */
#define TEST_FP_STEPS 20
#define TEST_FP_EYES  9

static void Test_SceneFingerprint(void) {
	Sim_Setup();
	u32 missed = 0;
	for (u32 e = 0; e < TEST_FP_EYES; e++) {
		float cx = 6.0f + 2.37f * (float)e, cy = 4.0f + 0.5f * (float)(e % 4), r = 3.0f;
		WeUi_Gfx_Scene_Init(&TestScene);
		for (u32 step = 0; step <= TEST_FP_STEPS; step++) {
			float reach = 1.36f * r * (1.0f - (float)step / TEST_FP_STEPS);
			WeUi_Gfx_Scene_Begin(&TestScene);
			WeUi_Gfx_Path_Reset(&TestPath, NULL);
			WeUi_Gfx_Path_MoveTo(&TestPath, cx - r - 1.0f, cy);
			WeUi_Gfx_Path_CubicTo(&TestPath, cx - 0.55f * r, cy - reach, cx + 0.55f * r, cy - reach,
								  cx + r + 1.0f, cy);
			WeUi_Gfx_Path_CubicTo(&TestPath, cx + 0.55f * r, cy + reach, cx - 0.55f * r, cy + reach,
								  cx - r - 1.0f, cy);
			WeUi_Gfx_Path_Close(&TestPath);
			WeUi_Gfx_Scene_Mask(&TestScene, &TestPath, NULL);
			u8 n = WeUi_Gfx_Scene_Damage(&TestScene, SceneRects, TEST_SCENE_RECTS);
			missed += step > 0 && n == 0;
		}
	}
	CHECK(missed == 0, "%u steps of an eye closing went unnoticed", missed);
}

static void Test_SceneErrors(void) {
	Sim_Setup();
	WeUi_Gfx_Scene_Init(&TestScene);
	WeUi_Gfx_Scene_Begin(&TestScene);
	u8 rect = SceneRect(1.0f, 1.0f, 4.0f, 4.0f, Rgb(255, 0, 0));
	u8 mask;
	WeUi_Gfx_Scene_Mask(&TestScene, &TestPath, &mask);
	CHECK(WeUi_Gfx_Scene_Clip(&TestScene, rect, rect, WEUI_GFX_MASK_IN) == RET_STATE_ERR_PARAM,
		  "clipped by a fill");
	CHECK(WeUi_Gfx_Scene_Clip(&TestScene, mask, mask, WEUI_GFX_MASK_IN) == RET_STATE_ERR_PARAM,
		  "a mask clipped");
	CHECK(WeUi_Gfx_Scene_Clip(&TestScene, rect, 99, WEUI_GFX_MASK_IN) == RET_STATE_ERR_PARAM,
		  "clipped by no item");
	for (u32 i = 0; i < WEUI_GFX_SCENE_ITEM_MASKS; i++)
		WeUi_Gfx_Scene_Clip(&TestScene, rect, mask, WEUI_GFX_MASK_IN);
	CHECK(WeUi_Gfx_Scene_Clip(&TestScene, rect, mask, WEUI_GFX_MASK_IN) == RET_STATE_ERR_OVERFLOW,
		  "clipped past the room");

	WeUi_Gfx_MaskUse_t use = {NULL, WEUI_GFX_MASK_IN};
	WeUi_Gfx_Paint_t paint = WeUi_Gfx_Paint_Solid(Rgb(255, 0, 0));
	paint.pcMasks		   = &use;
	paint.MasksNum		   = 1;
	CHECK(WeUi_Gfx_Scene_Fill(&TestScene, &TestPath, &paint, NULL) == RET_STATE_ERR_PARAM,
		  "a paint with masks of its own");

	paint = WeUi_Gfx_Paint_Solid(Rgb(255, 0, 0));
	for (u32 i = TestScene.ItemsNum; i < WEUI_GFX_SCENE_ITEMS_MAX; i++)
		WeUi_Gfx_Scene_Fill(&TestScene, &TestPath, &paint, NULL);
	CHECK(WeUi_Gfx_Scene_Fill(&TestScene, &TestPath, &paint, NULL) == RET_STATE_ERR_OVERFLOW &&
			  TestScene.Overflow,
		  "an item past the room");
	u8 n					  = WeUi_Gfx_Scene_Damage(&TestScene, SceneRects, TEST_SCENE_RECTS);
	WeUi_Gfx_ClipRect_t panel = WeUi_Gfx_Fill_PanelClip();
	CHECK(n == 1 && RectArea(&SceneRects[0]) == RectArea(&panel),
		  "an overflowed frame is not the whole panel");
}

static void FillTarget(u16* pBuf, WEUI_GFX_TARGET_t format) {
	for (u32 i = 0; i < NUM_ELEMENTS(TargetBuf); i++)
		pBuf[i] = TEST_TARGET_SENTINEL;
	const WeUi_Gfx_Target_t target = {
		.Format = format, .pBuf = pBuf, .Stride = TEST_TARGET_STRIDE, .Rect = {4, 2, 20, 10}};
	WeUi_Gfx_Path_Reset(&TestPath, NULL);
	WeUi_Gfx_Path_Rect(&TestPath, 6.0f, 4.0f, 30.0f, 7.0f); /* reaches past the target */
	WeUi_Gfx_Shape_t shape = WeUi_Gfx_Path_Shape(&TestPath);
	WeUi_Gfx_Paint_t red   = WeUi_Gfx_Paint_Solid(Rgb(200, 0, 0));
	CHECK(WeUi_Gfx_Fill_PaintShapeTo(&shape, &red, NULL, &target) == RET_STATE_SUCCESS,
		  "fill into a buffer refused");
}

static void Test_TargetFill(void) {
	FillTarget(TargetBuf, WEUI_GFX_TARGET_RGB565);
	/* (6, 4) is the buffer's (2, 2): row 2, column 2 */
	u16 red = Px565(Given(Rgb(200, 0, 0)));
	CHECK(TargetBuf[2 * TEST_TARGET_STRIDE + 2] == red, "inside is %04x, want %04x",
		  TargetBuf[2 * TEST_TARGET_STRIDE + 2], red);
	CHECK(TargetBuf[2 * TEST_TARGET_STRIDE + 15] == red, "the target's last column is %04x",
		  TargetBuf[2 * TEST_TARGET_STRIDE + 15]);
	CHECK(TargetBuf[2 * TEST_TARGET_STRIDE + 16] == TEST_TARGET_SENTINEL &&
			  TargetBuf[1 * TEST_TARGET_STRIDE + 2] == TEST_TARGET_SENTINEL &&
			  TargetBuf[2 * TEST_TARGET_STRIDE + 1] == TEST_TARGET_SENTINEL,
		  "a fill reached past the shape or the target");

	FillTarget(TargetBe, WEUI_GFX_TARGET_RGB565_BE);
	u32 wrong = 0;
	for (u32 i = 0; i < NUM_ELEMENTS(TargetBuf); i++) {
		bool untouched = TargetBuf[i] == TEST_TARGET_SENTINEL;
		wrong += untouched ? TargetBe[i] != TEST_TARGET_SENTINEL
						   : TargetBe[i] != __builtin_bswap16(TargetBuf[i]);
	}
	CHECK(wrong == 0, "%u big-endian pixels are not the others swapped", wrong);

	const WeUi_Gfx_Target_t noBuf  = {.Stride = 4, .Rect = {0, 0, 4, 4}};
	const WeUi_Gfx_Target_t narrow = {.pBuf = TargetBuf, .Stride = 3, .Rect = {0, 0, 4, 4}};
	WeUi_Gfx_Shape_t shape		   = WeUi_Gfx_Path_Shape(&TestPath);
	WeUi_Gfx_Paint_t redPaint	   = WeUi_Gfx_Paint_Solid(Rgb(200, 0, 0));
	CHECK(WeUi_Gfx_Fill_PaintShapeTo(&shape, &redPaint, NULL, &noBuf) == RET_STATE_ERR_PARAM,
		  "a target with no buffer");
	CHECK(WeUi_Gfx_Fill_PaintShapeTo(&shape, &redPaint, NULL, &narrow) == RET_STATE_ERR_PARAM,
		  "a target with rows shorter than it");
}

/* A scene into a buffer: band by band it is the same as whole, and it is the frame's picture
 * but for the last bits of where see-through layers meet - each blended in rounds to 565, in
 * the frame to 8 bits, and a few layers deep that adds up */
static void Test_SceneToBuffer(void) {
	Sim_Setup();
	WeUi_Gfx_Scene_Init(&TestScene);
	SceneFrame(3);

	WeUi_Gfx_Target_t whole = PanelTarget(PanelWhole, 0, WEUI_DISPLAY_HEIGHT);
	CHECK(WeUi_Gfx_Scene_DrawTo(&TestScene, &whole) == RET_STATE_SUCCESS, "scene into a buffer");

	/* Bottom band first: no band depends on another */
	for (s32 y1 = WEUI_DISPLAY_HEIGHT; y1 > 0; y1 -= TEST_BAND_ROWS) {
		WeUi_Gfx_Target_t band = PanelTarget(PanelBands, GET_MAX(y1 - TEST_BAND_ROWS, 0), y1);
		WeUi_Gfx_Scene_DrawTo(&TestScene, &band);
	}
	CHECK(memcmp(PanelWhole, PanelBands, sizeof(PanelWhole)) == 0,
		  "a scene drawn in bands differs from it drawn whole");

#if defined(WEUI_DISPLAY_COLOR)
	WeUi_Gfx_Adapter_ClearDrawingFrame();
	WeUi_Gfx_Scene_Draw(&TestScene, NULL);
	WeUi_Gfx_Adapter_DrawFrame();
	u32 off = 0;
	for (u32 i = 0; i < NUM_ELEMENTS(PanelWhole); i++) {
		u16 a = PanelWhole[i], b = Px565(Shown()[i]);
		s32 dr = (a >> 11) - (b >> 11), dg = ((a >> 5) & 0x3F) - ((b >> 5) & 0x3F);
		s32 db = (a & 0x1F) - (b & 0x1F);
		off += abs(dr) > TEST_TARGET_LSB || abs(dg) > TEST_TARGET_LSB || abs(db) > TEST_TARGET_LSB;
	}
	CHECK(off == 0, "%u pixels of the buffer are off the frame's by more than %d bits", off,
		  TEST_TARGET_LSB);
#endif

	WeUi_Gfx_Target_t wide = PanelTarget(PanelWhole, 0, 1);
	wide.Rect.X1		   = WEUI_GFX_RASTER_WIDTH_MAX + 1;
	wide.Stride			   = WEUI_GFX_RASTER_WIDTH_MAX + 1;
	CHECK(WeUi_Gfx_Scene_DrawTo(&TestScene, &wide) == RET_STATE_ERR_PARAM,
		  "a target wider than the rasteriser");
}

// =================== Runner ===================

typedef struct {
	const char* Name;
	VoidFunc_t Fn;
} Test_t;

#if WEUI_DISPLAY_BANDS && WEUI_DISPLAY_FRAME_MEM
// =================== Transitions in bands ===================

#define TEST_BANDS_FRAMES	 9
#define TEST_BANDS_OLD_SALT	 1u
#define TEST_BANDS_NEW_SALT	 2u
#define TEST_BANDS_COLUMN_PX 3u
#define TEST_BANDS_RGB		 0x00FFFFFFu

/* Noise: every pixel its own, so a pixel taken from the wrong place shows */
static WeUi_Gfx_ColorRaw_t BandsPattern(u32 x, u32 y, u32 salt) {
	return WeUi_Rand_Hash(x, y, salt) & TEST_BANDS_RGB;
}

static void BandsPaint(u32 salt) {
	WeUi_Gfx_Point_t pt;
	for (pt.Y = 0; pt.Y < WEUI_DISPLAY_HEIGHT; pt.Y++)
		for (pt.X = 0; pt.X < WEUI_DISPLAY_WIDTH; pt.X++)
			WeUi_Gfx_Adapter_SetPixelRaw(&pt, BandsPattern((u32)pt.X, (u32)pt.Y, salt));
}

/* The incoming scene holds still, so drawn once or every frame it is the same */
static void Render_BandsNew(void) {
	BandsPaint(TEST_BANDS_NEW_SALT);
}

static u64 BandsSeen[TEST_BANDS_FRAMES];
static u32 BandsSeenNum;
static u32 BandsVariant;

/* The panel as a band of 565 brings it, whichever way the frame got there */
static void BandsSee(const u32* pcFrame) {
	static u32 cut[WEUI_DISPLAY_WIDTH * WEUI_DISPLAY_HEIGHT];
	for (u32 i = 0; i < NUM_ELEMENTS(cut); i++)
		cut[i] = WeUi_Gfx_Adapter_RawFrom565(WeUi_Gfx_Adapter_RawTo565(pcFrame[i]));
	if (BandsSeenNum < TEST_BANDS_FRAMES)
		BandsSeen[BandsSeenNum] = DisplaySim_Hash(cut);
	BandsSeenNum++;
}

static void BandsSeePushed(const u32* pcFrame, u32 idx) {
	BandsSee(pcFrame);
}

static void BandsSeeBands(void) {
	BandsSee(Shown());
}

static RET_STATE_t BandsFade(void) {
	return WeUi_Gfx_Effect_Fade(Render_BandsNew, TEST_BANDS_FRAMES);
}
static RET_STATE_t BandsDissolve(void) {
	return WeUi_Gfx_Effect_Dissolve(Render_BandsNew, TEST_BANDS_FRAMES);
}
static RET_STATE_t BandsFlash(void) {
	return WeUi_Gfx_Effect_Flash(Render_BandsNew, TEST_BANDS_FRAMES);
}
static RET_STATE_t BandsDim(void) {
	return WeUi_Gfx_Effect_Dim(Render_BandsNew, TEST_BANDS_FRAMES);
}
static RET_STATE_t BandsPush(void) {
	return WeUi_Gfx_Effect_Push(Render_BandsNew, TEST_BANDS_FRAMES,
								(WEUI_GFX_EFFECT_SLIDE_t)BandsVariant, NULL);
}
static RET_STATE_t BandsCover(void) {
	return WeUi_Gfx_Effect_Cover(Render_BandsNew, TEST_BANDS_FRAMES,
								 (WEUI_GFX_EFFECT_SLIDE_t)(BandsVariant / 2),
								 (WEUI_GFX_EFFECT_COVER_t)(BandsVariant % 2));
}
static RET_STATE_t BandsCurtain(void) {
	return WeUi_Gfx_Effect_Curtain(Render_BandsNew, TEST_BANDS_FRAMES,
								   (WEUI_GFX_EFFECT_AXIS_t)BandsVariant);
}
static RET_STATE_t BandsInterlace(void) {
	return WeUi_Gfx_Effect_Interlace(Render_BandsNew, TEST_BANDS_FRAMES);
}
static RET_STATE_t BandsBlinds(void) {
	return WeUi_Gfx_Effect_Blinds(Render_BandsNew, TEST_BANDS_FRAMES);
}
static RET_STATE_t BandsBlocks(void) {
	return WeUi_Gfx_Effect_Blocks(Render_BandsNew, TEST_BANDS_FRAMES);
}
static RET_STATE_t BandsReload(void) {
	return WeUi_Gfx_Effect_Reload(Render_BandsNew, TEST_BANDS_FRAMES);
}
static RET_STATE_t BandsWave(void) {
	return WeUi_Gfx_Effect_Wave(Render_BandsNew, TEST_BANDS_FRAMES);
}
static RET_STATE_t BandsRain(void) {
	return WeUi_Gfx_Effect_Rain(Render_BandsNew, TEST_BANDS_FRAMES, TEST_BANDS_COLUMN_PX);
}
static RET_STATE_t BandsMelt(void) {
	return WeUi_Gfx_Effect_Melt(Render_BandsNew, TEST_BANDS_FRAMES, TEST_BANDS_COLUMN_PX);
}
static RET_STATE_t BandsZoom(void) {
	return WeUi_Gfx_Effect_Zoom(Render_BandsNew, TEST_BANDS_FRAMES);
}
static RET_STATE_t BandsRotate(void) {
	return WeUi_Gfx_Effect_Rotate(Render_BandsNew, TEST_BANDS_FRAMES);
}
static RET_STATE_t BandsPixelate(void) {
	return WeUi_Gfx_Effect_Pixelate(Render_BandsNew, TEST_BANDS_FRAMES);
}

/* What reaches the panel, frame by frame, played one way or the other */
static u32 BandsPlay(RET_STATE_t (*play)(void), bool bands, u64* pSeen) {
	Sim_Setup();
	BandsPaint(TEST_BANDS_OLD_SALT);
	WeUi_Gfx_Adapter_DrawFrame();

	WeUi_Gfx_Effect_SetBands(bands);
	BandsSeenNum = 0;
	DisplaySim_SetFrameHook(BandsSeePushed);
	WeUi_Gfx_FpsCtrl_AttachAppCallback(BandsSeeBands);
	RET_STATE_t rs = play();
	WeUi_Gfx_FpsCtrl_AttachAppCallback(NULL);
	DisplaySim_SetFrameHook(NULL);
	WeUi_Gfx_Effect_SetBands(true);

	CHECK(rs == RET_STATE_SUCCESS, "a transition refused to play: %d", rs);
	memcpy(pSeen, BandsSeen, sizeof(BandsSeen));
	return BandsSeenNum;
}

/* Composed band by band from both pictures, every frame is the one composed in place */
static void Test_TransitionsInBands(void) {
	static const struct {
		const char* pcName;
		RET_STATE_t (*Play)(void);
		u32 Variants;
	} effects[] = {
		{"fade", BandsFade, 1},			  {"dissolve", BandsDissolve, 1}, {"flash", BandsFlash, 1},
		{"dim", BandsDim, 1},			  {"push", BandsPush, 4},		  {"cover", BandsCover, 8},
		{"curtain", BandsCurtain, 2},	  {"interlace", BandsInterlace, 1},
		{"blinds", BandsBlinds, 1},		  {"blocks", BandsBlocks, 1},	  {"reload", BandsReload, 1},
		{"wave", BandsWave, 1},			  {"rain", BandsRain, 1},		  {"melt", BandsMelt, 1},
		{"zoom", BandsZoom, 1},			  {"rotate", BandsRotate, 1},
		{"pixelate", BandsPixelate, 1},
	};
	u64 inPlace[TEST_BANDS_FRAMES], inBands[TEST_BANDS_FRAMES];

	for (u32 e = 0; e < NUM_ELEMENTS(effects); e++) {
		for (BandsVariant = 0; BandsVariant < effects[e].Variants; BandsVariant++) {
			u32 n	   = BandsPlay(effects[e].Play, false, inPlace);
			u32 nBands = BandsPlay(effects[e].Play, true, inBands);
			CHECK(n == TEST_BANDS_FRAMES && nBands == n, "%s %u: %u frames in place, %u in bands",
				  effects[e].pcName, BandsVariant, n, nBands);
			u32 frame = 0;
			while (frame < TEST_BANDS_FRAMES && inPlace[frame] == inBands[frame])
				frame++;
			CHECK(frame == TEST_BANDS_FRAMES, "%s %u: frame %u differs in bands", effects[e].pcName,
				  BandsVariant, frame + 1);
		}
	}
}
#endif /* WEUI_DISPLAY_BANDS && WEUI_DISPLAY_FRAME_MEM */

static const Test_t Tests[] = {
	{"rect_zero_size", Test_RectZeroSize},
	{"rect_fill_keeps_input", Test_RectFillKeepsInput},
	{"line_defaults_visible", Test_LineDefaultsVisible},
	{"color_over_range", Test_ColorOverRange},
	{"fill_matches_pixel", Test_FillMatchesPixel},
	{"affine_compose", Test_AffineCompose},
	{"affine_invert", Test_AffineInvert},
	{"affine_stack", Test_AffineStack},
	{"path_rect", Test_PathRect},
	{"path_flatten", Test_PathFlatten},
	{"path_errors", Test_PathErrors},
	{"raster_rect", Test_RasterRect},
	{"raster_area", Test_RasterArea},
	{"raster_nonzero", Test_RasterNonzero},
	{"raster_clip", Test_RasterClip},
	{"raster_errors", Test_RasterErrors},
	{"fill_solid", Test_FillSolid},
	{"paint_linear", Test_PaintLinear},
	{"paint_radial", Test_PaintRadial},
	{"paint_opacity", Test_PaintOpacity},
	{"paint_soft", Test_PaintSoft},
	{"paint_errors", Test_PaintErrors},
	{"paint_bright", Test_PaintBright},
	{"stroke_caps", Test_StrokeCaps},
	{"stroke_joins", Test_StrokeJoins},
	{"stroke_closed", Test_StrokeClosed},
	{"stroke_outline", Test_StrokeOutline},
	{"stroke_scale_errors", Test_StrokeScaleErrors},
	{"raster_bounds", Test_RasterBounds},
	{"mask_build", Test_MaskBuild},
	{"mask_fill", Test_MaskFill},
	{"scene_damage", Test_SceneDamage},
	{"scene_errors", Test_SceneErrors},
	{"scene_fingerprint", Test_SceneFingerprint},
	{"scene_redraw", Test_SceneRedraw},
	{"target_fill", Test_TargetFill},
	{"scene_to_buffer", Test_SceneToBuffer},
	{"text_width", Test_TextWidthConsistency},
	{"utf8", Test_Utf8FromCodePoint},
	{"text_no_font", Test_TextNoFont},
	{"text_block_refpt", Test_TextBlockRefPtShifts},
	{"text_block_wrap", Test_TextBlockWrap},
#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
	{"image_transparent_lut", Test_ImageTransparentUnderLut},
	{"image_crop", Test_ImageCrop},
	{"animation", Test_AnimationDraws},
#endif /* defined(WEUI_LIB_COLOR_SCHEME__RGB) */
	{"effect_max_frames", Test_EffectMaxFrames},
	{"effect_frames", Test_EffectFrames},
	{"push_bad_cfg", Test_PushRejectsBadCfg},
	{"push_lands", Test_PushLandsExactly},
	{"scenemgr", Test_SceneMgr},
#if WEUI_USE_COMPONENT__WIDGET_QR
	{"qr", Test_Qr},
#endif /* WEUI_USE_COMPONENT__WIDGET_QR */
	{"demos", Test_Demos},
	/* A test that draws goes below the golden runs: above them it moves the frame counter
	 * and the clock the demos are coloured and paced by, and every hash with them */
	{"scenemgr_transitions", Test_SceneMgrTransitions},
	{"scenemgr_timeouts", Test_SceneMgrTimeouts},
	{"scenemgr_trace", Test_SceneMgrTrace},
#if WEUI_DISPLAY_BANDS && WEUI_DISPLAY_FRAME_MEM
	{"transitions_in_bands", Test_TransitionsInBands},
#endif /* WEUI_DISPLAY_BANDS && WEUI_DISPLAY_FRAME_MEM */
};

int main(int argc, char** argv) {
	if (argc > 1)
		OutDir = argv[1];
	const char* pcOnly = (argc > 2 && argv[2][0]) ? argv[2] : NULL;

	for (u32 i = 0; i < NUM_ELEMENTS(Tests); i++) {
		if (pcOnly && strcmp(pcOnly, Tests[i].Name) != 0)
			continue;

		u32 failedBefore = Failed;
		Tests[i].Fn();
		printf("%-24s %s\n", Tests[i].Name, Failed == failedBefore ? "ok" : "FAILED");
		fflush(stdout);
	}

	printf("%u checks, %u failed\n", Checks, Failed);
	DisplaySim_Deinit();
	return Failed ? 1 : 0;
}
