#ifndef __WEUI_CFG_CHECK_H
#define __WEUI_CFG_CHECK_H

/**
 * @file weui_cfg_check.h
 * @brief What follows from the host's weui_cfg.h: a config that cannot work is
 *        rejected, what the chosen schemes imply is derived, what was left out is
 *        defaulted. Derived values are #ifndef-guarded, so a weui_cfg.h that still
 *        spells them out keeps building - and is checked against the derivation.
 */

// =================== Required ===================

#if !defined(WEUI_DISPLAY_WIDTH) || !defined(WEUI_DISPLAY_HEIGHT)
#error "weui_cfg.h: WEUI_DISPLAY_WIDTH and WEUI_DISPLAY_HEIGHT must be defined"
#endif

#if defined(WEUI_DISPLAY_GRAY) == defined(WEUI_DISPLAY_COLOR)
#error "weui_cfg.h: define exactly one of WEUI_DISPLAY_GRAY, WEUI_DISPLAY_COLOR"
#endif

#if defined(WEUI_LIB_COLOR_SCHEME__GRAY) == defined(WEUI_LIB_COLOR_SCHEME__RGB)
#error "weui_cfg.h: define exactly one of WEUI_LIB_COLOR_SCHEME__GRAY, WEUI_LIB_COLOR_SCHEME__RGB"
#endif

// =================== Pixel formats ===================

/* Image pixels are copied into library colours as they are, so storage defaults to
 * the library scheme - the one combination that draws right */
#if defined(WEUI_IMG_STORAGE_COLOR_SCHEME__GRAY) && defined(WEUI_IMG_STORAGE_COLOR_SCHEME__RGB)
#error "weui_cfg.h: define at most one of WEUI_IMG_STORAGE_COLOR_SCHEME__GRAY, __RGB"
#elif !defined(WEUI_IMG_STORAGE_COLOR_SCHEME__GRAY) && !defined(WEUI_IMG_STORAGE_COLOR_SCHEME__RGB)
#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
#define WEUI_IMG_STORAGE_COLOR_SCHEME__RGB
#else
#define WEUI_IMG_STORAGE_COLOR_SCHEME__GRAY
#endif
#endif

#ifndef WEUI_LIB_COLOR_ALPHA_CHANNEL
#define WEUI_LIB_COLOR_ALPHA_CHANNEL 0
#endif

#ifndef WEUI_IMG_STORAGE_ALPHA_CHANNEL
#define WEUI_IMG_STORAGE_ALPHA_CHANNEL 0
#endif

#ifndef WEUI_LIB_RGB_BLENDING
#define WEUI_LIB_RGB_BLENDING 0
#endif

#define WEUI_CFG_CHANNELS_GRAY 1
#define WEUI_CFG_CHANNELS_RGB  3

#if defined(WEUI_LIB_COLOR_SCHEME__RGB)
#define WEUI_CFG_LIB_CHANNELS WEUI_CFG_CHANNELS_RGB
#else
#define WEUI_CFG_LIB_CHANNELS WEUI_CFG_CHANNELS_GRAY
#endif

#if defined(WEUI_IMG_STORAGE_COLOR_SCHEME__RGB)
#define WEUI_CFG_IMG_CHANNELS WEUI_CFG_CHANNELS_RGB
#else
#define WEUI_CFG_IMG_CHANNELS WEUI_CFG_CHANNELS_GRAY
#endif

#define WEUI_CFG_LIB_PIXEL_BYTES (WEUI_CFG_LIB_CHANNELS + (WEUI_LIB_COLOR_ALPHA_CHANNEL ? 1 : 0))
#define WEUI_CFG_IMG_PIXEL_BYTES (WEUI_CFG_IMG_CHANNELS + (WEUI_IMG_STORAGE_ALPHA_CHANNEL ? 1 : 0))

#ifndef WEUI_LIB_COLOR_TOTAL_SIZE
#define WEUI_LIB_COLOR_TOTAL_SIZE WEUI_CFG_LIB_PIXEL_BYTES
#elif WEUI_LIB_COLOR_TOTAL_SIZE != WEUI_CFG_LIB_PIXEL_BYTES
#error "weui_cfg.h: WEUI_LIB_COLOR_TOTAL_SIZE disagrees with the library colour scheme"
#endif

#ifndef WEUI_IMG_STORAGE_COLOR_SIZE
#define WEUI_IMG_STORAGE_COLOR_SIZE WEUI_CFG_IMG_PIXEL_BYTES
#elif WEUI_IMG_STORAGE_COLOR_SIZE != WEUI_CFG_IMG_PIXEL_BYTES
#error "weui_cfg.h: WEUI_IMG_STORAGE_COLOR_SIZE disagrees with the image storage scheme"
#endif

// =================== Components ===================

#ifndef WEUI_USE_COMPONENT__IMAGE
#define WEUI_USE_COMPONENT__IMAGE 0
#endif

#ifndef WEUI_USE_COMPONENT__IMAGE_DITHER
#define WEUI_USE_COMPONENT__IMAGE_DITHER 0
#endif

#ifndef WEUI_USE_COMPONENT__IMAGE_COMPRESS
#define WEUI_USE_COMPONENT__IMAGE_COMPRESS 0
#endif

/* gfx/demo: showpieces for bring-up and the lab, flash a product can drop */
#ifndef WEUI_USE_COMPONENT__DEMO
#define WEUI_USE_COMPONENT__DEMO 1
#endif

#if WEUI_USE_COMPONENT__IMAGE_DITHER && !defined(WEUI_LIB_COLOR_SCHEME__GRAY)
#error "weui_cfg.h: WEUI_USE_COMPONENT__IMAGE_DITHER requires WEUI_LIB_COLOR_SCHEME__GRAY"
#endif

#ifndef WEUI_IMAGES_OPS_DATA_PLACEMENT
#define WEUI_IMAGES_OPS_DATA_PLACEMENT
#endif

#ifndef WEUI_HATCHING_PIXELS_SIZE_MAX
#define WEUI_HATCHING_PIXELS_SIZE_MAX 10
#endif

#ifndef WEUI_ALLOC
#define WEUI_ALLOC(size, tmo) malloc(size)
#define WEUI_FREE(ptr)		  free(ptr)
#endif

// =================== Frame rate ===================

#ifndef WEUI_GFX_FPS_CTRL_MIN_RENDER_FREQ
#define WEUI_GFX_FPS_CTRL_MIN_RENDER_FREQ 1
#endif

#ifndef WEUI_GFX_FPS_CTRL_DEF_RENDER_FREQ
#define WEUI_GFX_FPS_CTRL_DEF_RENDER_FREQ 60
#endif

#ifndef WEUI_GFX_FPS_CTRL_MAX_RENDER_FREQ
#define WEUI_GFX_FPS_CTRL_MAX_RENDER_FREQ 120
#endif

#ifndef WEUI_GFX_FPS_CTRL_MIN_DELAY
#define WEUI_GFX_FPS_CTRL_MIN_DELAY 1
#endif

/* A frame period is 1000 / rate: a zero floor is a division by zero in DrawAndKeepFPS */
#if WEUI_GFX_FPS_CTRL_MIN_RENDER_FREQ < 1
#error "weui_cfg.h: WEUI_GFX_FPS_CTRL_MIN_RENDER_FREQ must be at least 1"
#endif

#if WEUI_GFX_FPS_CTRL_MIN_RENDER_FREQ > WEUI_GFX_FPS_CTRL_DEF_RENDER_FREQ || \
	WEUI_GFX_FPS_CTRL_DEF_RENDER_FREQ > WEUI_GFX_FPS_CTRL_MAX_RENDER_FREQ
#error "weui_cfg.h: the render frequencies must hold MIN <= DEF <= MAX"
#endif

#endif /* __WEUI_CFG_CHECK_H */
