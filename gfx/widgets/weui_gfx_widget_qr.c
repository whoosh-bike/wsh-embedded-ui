#include "weui_gfx_widget_qr.h"
#include "def_macro.h"
#include "log.h"
#include "weui_gfx_adapter.h"
#include "weui_gfx_base.h"
#include "weui_gfx_colors.h"

#if DEBUG_ENABLE
#define LOCAL_DEBUG_PRINT_ENABLE 0
#endif /* DEBUG_ENABLE */

#if LOCAL_DEBUG_PRINT_ENABLE
#warning LOCAL_DEBUG_PRINT_ENABLE
#define LOCAL_DEBUG_PRINT LOG_RAW_DIRECT_NL
#else /* LOCAL_DEBUG_PRINT_ENABLE */
#define LOCAL_DEBUG_PRINT(_f_, ...)
#endif /* LOCAL_DEBUG_PRINT_ENABLE */

#if WEUI_USE_COMPONENT__WIDGET_QR

#include "qrcodegen.h"

/* QR Model 2: a version v symbol is 17 + 4v modules a side */
#define WEUI_GFX_WIDGET_QR_SIDE_BASE 17
#define WEUI_GFX_WIDGET_QR_SIDE_STEP 4

/* A cap on top of the panel's own bound, e.g. to limit the heap a call takes */
#ifndef WEUI_GFX_WIDGET_QR_VERSION_MAX
#define WEUI_GFX_WIDGET_QR_VERSION_MAX qrcodegen_VERSION_MAX
#endif /* WEUI_GFX_WIDGET_QR_VERSION_MAX */

/**
 * @brief The largest symbol the panel shows at one pixel per module, from the panel
 *        itself rather than the buffer-size macros. The buffers are sized for it instead
 *        of for version 40, ~8 KB a call: a text needing more does not fit anyway
 */
static s32 WeUi_Gfx_Widget_QR_VersionMax(void) {
	s32 side	= GET_MIN(WeUi_Gfx_Adapter_GetWidth(), WeUi_Gfx_Adapter_GetHeight());
	s32 version = (side - WEUI_GFX_WIDGET_QR_SIDE_BASE) / WEUI_GFX_WIDGET_QR_SIDE_STEP;

	return GET_MIN(version, WEUI_GFX_WIDGET_QR_VERSION_MAX);
}

/* WEUI_QR_ECC_t is passed straight to qrcodegen, so the two must stay aligned */
static_assert((s32)WEUI_QR_ECC_LOW == (s32)qrcodegen_Ecc_LOW &&
				  (s32)WEUI_QR_ECC_HIGH == (s32)qrcodegen_Ecc_HIGH,
			  "WEUI_QR_ECC_t no longer mirrors enum qrcodegen_Ecc");

void WeUi_Gfx_Widget_QR(WeUi_Gfx_Point_t* pRefPt, u32 scale, WEUI_QR_ECC_t ecc, const char* pcText,
						WeUi_Gfx_ColorSchemeHsb_t hsb) {
	s32 versionMax = WeUi_Gfx_Widget_QR_VersionMax();
	if (!pRefPt || !pcText || versionMax < qrcodegen_VERSION_MIN)
		return;

	u32 buffLen	 = qrcodegen_BUFFER_LEN_FOR_VERSION(versionMax);
	u8* pTmpBuff = WEUI_ALLOC(buffLen, MEM_ALLOC_DEF_TMO);
	if (!pTmpBuff)
		return;

	u8* pQrCode = WEUI_ALLOC(buffLen, MEM_ALLOC_DEF_TMO);
	if (!pQrCode) {
		WEUI_FREE(pTmpBuff);
		return;
	}

	bool qrIsOk =
		qrcodegen_encodeText(pcText, pTmpBuff, pQrCode, (enum qrcodegen_Ecc)ecc,
							 qrcodegen_VERSION_MIN, versionMax, qrcodegen_Mask_AUTO, true);
	if (!qrIsOk) {
		WEUI_FREE(pTmpBuff);
		WEUI_FREE(pQrCode);
		return;
	}

	u32 size				  = qrcodegen_getSize(pQrCode);
	WeUi_Gfx_ColorPoint_t cpt = {
		.Color = WeUi_Gfx_Color_FromHsb(&hsb),
	};

	for (u32 y = 0; y < size; y++) {
		for (u32 x = 0; x < size; x++) {
			bool isColored = qrcodegen_getModule(pQrCode, x, y);
			if (!isColored)
				continue;

			u32 baseX = pRefPt->X + x * scale;
			u32 baseY = pRefPt->Y + y * scale;

			for (u32 dx = 0; dx < scale; dx++) {
				for (u32 dy = 0; dy < scale; dy++) {
					cpt.Pt.X = baseX + dx;
					cpt.Pt.Y = baseY + dy;
					WeUi_Gfx_Point_Draw(&cpt);
				}
			}
		}
	}

	WEUI_FREE(pTmpBuff);
	WEUI_FREE(pQrCode);
}

#else /* WEUI_USE_COMPONENT__WIDGET_QR */

void WeUi_Gfx_Widget_QR(WeUi_Gfx_Point_t* pRefPt, u32 scale, WEUI_QR_ECC_t ecc, const char* pcText,
						WeUi_Gfx_ColorSchemeHsb_t hsb) {
}

#endif /* WEUI_USE_COMPONENT__WIDGET_QR */
