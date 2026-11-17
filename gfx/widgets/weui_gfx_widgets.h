#ifndef __WEUI_GFX_WIDGETS_H
#define __WEUI_GFX_WIDGETS_H

/**
 * @file weui_gfx_widgets.h
 * @brief Umbrella header for the widgets module.
 *
 * A widget composes the drawing primitives into a finished element and takes a
 * style struct rather than build-time settings. The frame and the FPS belong to
 * the scene: a widget neither clears the drawing frame nor pushes it.
 *
 * Each widget carries its own feature toggle, so a host pays only for what it
 * draws - the QR widget is off by default, it being the only user of qrcodegen.
 */

#include "weui_gfx_widget_qr.h"
#include "weui_gfx_widget_text_block.h"

#endif /* __WEUI_GFX_WIDGETS_H */
