#ifndef __WEUI_GFX_WIDGET_QR_H
#define __WEUI_GFX_WIDGET_QR_H

#include "weui_gfx_types.h"

/**
 * QR widget. Off by default: it is the only user of the qrcodegen third-party
 * library, so a host that draws no QR codes does not have to carry it. Set
 * WEUI_USE_COMPONENT__WIDGET_QR to 1 in weui_cfg.h and add qrcodegen to the
 * build to enable it - with it off WeUi_Gfx_Widget_QR stays callable and draws
 * nothing.
 */
#ifndef WEUI_USE_COMPONENT__WIDGET_QR
#define WEUI_USE_COMPONENT__WIDGET_QR 0
#endif /* WEUI_USE_COMPONENT__WIDGET_QR */

/**
 * @brief QR error correction level. Mirrors enum qrcodegen_Ecc so that the
 *        public API stays free of the qrcodegen header
 */
typedef enum {
	WEUI_QR_ECC_LOW = 0,
	WEUI_QR_ECC_MEDIUM,
	WEUI_QR_ECC_QUARTILE,
	WEUI_QR_ECC_HIGH,
} WEUI_QR_ECC_t;

/**
 * @brief Generate and draw a QR code at the reference point. Draws nothing when the
 *        text needs a symbol larger than the panel, or than WEUI_GFX_WIDGET_QR_VERSION_MAX.
 * @param pRefPt   Top-left corner position
 * @param scale    Pixel size per QR module (1=1px per module, 2=2px, etc.)
 * @param ecc      Error correction level (WEUI_QR_ECC_LOW .. WEUI_QR_ECC_HIGH)
 * @param pcText   Text to encode (null-terminated)
 * @param hsb      Module color in HSB. Background is transparent (not drawn).
 */
void WeUi_Gfx_Widget_QR(WeUi_Gfx_Point_t* pRefPt, u32 scale, WEUI_QR_ECC_t ecc, const char* pcText,
						WeUi_Gfx_ColorSchemeHsb_t hsb);

#endif /* __WEUI_GFX_WIDGET_QR_H */
