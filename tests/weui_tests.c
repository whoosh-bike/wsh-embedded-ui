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
#include "weui_gfx_animation.h"
#include "weui_gfx_base.h"
#include "weui_gfx_colors.h"
#include "weui_gfx_effects.h"
#include "weui_gfx_fps_control.h"
#include "weui_gfx_image.h"
#include "weui_gfx_scenemgr.h"
#include "weui_gfx_scenemgr_trace.h"
#include "weui_gfx_text.h"
#include "weui_gfx_transform.h"
#include "weui_gfx_widgets.h"

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

// =================== Runner ===================

typedef struct {
	const char* Name;
	VoidFunc_t Fn;
} Test_t;

static const Test_t Tests[] = {
	{"rect_zero_size", Test_RectZeroSize},
	{"rect_fill_keeps_input", Test_RectFillKeepsInput},
	{"line_defaults_visible", Test_LineDefaultsVisible},
	{"color_over_range", Test_ColorOverRange},
	{"fill_matches_pixel", Test_FillMatchesPixel},
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
