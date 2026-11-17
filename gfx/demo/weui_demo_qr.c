#include "weui_demo.h"

#if WEUI_USE_COMPONENT__DEMO

#include "def_macro.h"
#include "weui_demo_pace.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_base.h"
#include "weui_gfx_colors.h"
#include "weui_gfx_fps_control.h"
#include "weui_gfx_transform.h"
#include "weui_gfx_widget_qr.h"

/* The trailing digits count re-encodes, so the data modules keep rebuilding */
#define DEMO_QR_TEXT   "weui qr 000"
#define DEMO_QR_DIGITS 3
#define DEMO_QR_SIDE   21 /* version 1: what DEMO_QR_TEXT takes at ECC LOW */

#define DEMO_QR_FPS			  WEUI_DEMO_LIVE_FPS
#define DEMO_QR_RECODE_FRAMES 3
#define DEMO_QR_HOLD_FRAMES	  (DEMO_QR_FPS / 2)
#define DEMO_QR_SPLIT_FRAMES  (WEUI_DEMO_LIVE_FRAMES - DEMO_QR_HOLD_FRAMES)

typedef struct {
	WeUi_Gfx_Point_t Pt;
	s32 Side;
	u8* pMask;
} WeUi_DemoQr_t;

/* The widget can only overwrite, so the symbol is drawn once and read back as a
 * mask: the three channels are then composed from it with additive mixing */
static void WeUi_DemoQr_Capture(WeUi_DemoQr_t* pQr, u32 scale, const char* pcText) {
	WeUi_Gfx_ColorSchemeHsb_t white = {
		.Hue = 0,
		.Sat = 0,
		.Bri = WEUI_COLOR_HSB_MAX_BRI,
	};

	WeUi_Gfx_Adapter_ClearDrawingFrame();
	WeUi_Gfx_Widget_QR(&pQr->Pt, scale, WEUI_QR_ECC_LOW, pcText, white);

	for (s32 y = 0; y < pQr->Side; y++) {
		for (s32 x = 0; x < pQr->Side; x++) {
			WeUi_Gfx_Point_t pt = {
				.X = pQr->Pt.X + x,
				.Y = pQr->Pt.Y + y,
			};
			pQr->pMask[y * pQr->Side + x] = WeUi_Gfx_Adapter_GetPixel(&pt).R != 0;
		}
	}
}

static bool WeUi_DemoQr_IsSet(const WeUi_DemoQr_t* pcQr, s32 x, s32 y) {
	if (x < 0 || x >= pcQr->Side)
		return false;

	return pcQr->pMask[y * pcQr->Side + x];
}

/** @brief Red shifted left, green in place, blue shifted right: white where all three meet */
static void WeUi_DemoQr_DrawSplit(const WeUi_DemoQr_t* pcQr, s32 shift) {
	WeUi_Gfx_Adapter_ClearDrawingFrame();

	for (s32 y = 0; y < pcQr->Side; y++) {
		for (s32 x = -shift; x < pcQr->Side + shift; x++) {
			bool r = WeUi_DemoQr_IsSet(pcQr, x + shift, y);
			bool g = WeUi_DemoQr_IsSet(pcQr, x, y);
			bool b = WeUi_DemoQr_IsSet(pcQr, x - shift, y);
			if (!r && !g && !b)
				continue;

			WeUi_Gfx_ColorPoint_t cpt = {
				.Pt =
					{
						.X = pcQr->Pt.X + x,
						.Y = pcQr->Pt.Y + y,
					},
				.Color = WeUi_Gfx_Transform_Rgb2Color(r ? WEUI_COLOR_CHANNEL_MAX : 0,
													  g ? WEUI_COLOR_CHANNEL_MAX : 0,
													  b ? WEUI_COLOR_CHANNEL_MAX : 0),
			};
			WeUi_Gfx_Point_Draw(&cpt);
		}
	}
}

/** @brief White, then slowly splitting into its R, G and B channels for the rest of the demo */
void WeUi_DrawDemo_Qr(void) {
	/* As large as the shorter side takes: the widget itself draws nothing below 1x */
	u32 minSide = GET_MIN(WeUi_Gfx_Adapter_GetWidth(), WeUi_Gfx_Adapter_GetHeight());
	u32 scale	= GET_MAX(minSide / DEMO_QR_SIDE, 1);
	s32 side	= (s32)(DEMO_QR_SIDE * scale);

	WeUi_DemoQr_t qr = {
		.Pt =
			{
				.X = WeUi_Gfx_Adapter_GetX_Mid() - side / 2,
				.Y = WeUi_Gfx_Adapter_GetY_Mid() - side / 2,
			},
		.Side  = side,
		.pMask = WEUI_ALLOC((u32)(side * side), MEM_ALLOC_DEF_TMO),
	};
	if (!qr.pMask)
		return;

	/* Apart by a module-wide gap, or as far as the panel lets the side copies go */
	s32 shiftMax  = WeUi_Gfx_Adapter_GetX_Mid() - (side + (s32)scale) / 2;
	s32 splitLast = DEMO_QR_SPLIT_FRAMES - 1;

	char text[]	  = DEMO_QR_TEXT;
	char* pDigits = &text[sizeof(text) - 1 - DEMO_QR_DIGITS];

	for (u16 frame = 0; frame < WEUI_DEMO_LIVE_FRAMES; frame++) {
		if (frame % DEMO_QR_RECODE_FRAMES == 0) {
			u16 count = frame / DEMO_QR_RECODE_FRAMES;
			for (s32 i = DEMO_QR_DIGITS - 1; i >= 0; i--, count /= 10)
				pDigits[i] = (char)('0' + count % 10);

			WeUi_DemoQr_Capture(&qr, scale, text);
		}

		s32 shift = 0;
		if (frame >= DEMO_QR_HOLD_FRAMES) {
			/* Ease-out: fast off the white, settling as the channels part */
			s32 left = splitLast - (frame - DEMO_QR_HOLD_FRAMES);
			shift	 = shiftMax * (splitLast * splitLast - left * left) / (splitLast * splitLast);
		}

		WeUi_DemoQr_DrawSplit(&qr, shift);
		WeUi_Gfx_FpsCtrl_DrawAndKeepFPS(DEMO_QR_FPS);
	}

	WEUI_FREE(qr.pMask);
}

#endif /* WEUI_USE_COMPONENT__DEMO */
