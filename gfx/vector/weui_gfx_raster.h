#ifndef __WEUI_GFX_RASTER_H
#define __WEUI_GFX_RASTER_H

#include "weui_gfx_path.h"

/**
 * @file weui_gfx_raster.h
 * @brief A path's coverage of each pixel, antialiased without supersampling: every segment
 *        adds the exact signed area it leaves to its right to the cells of the rows it crosses
 *        (as font rasterisers do, after font-rs), and a running sum along each row gives the
 *        share of every pixel inside the shape. Sums are integers; the fill rule is nonzero.
 *        Winding is summed over a pixel before it is clamped, so a pixel is exact where the
 *        shape winds once or not at all across it; one that a self-overlap crosses only in
 *        part (winding 2 there) comes out heavier than its share.
 *
 * The coverage comes out as spans, row by row, for the caller to paint: runs of 8 pixels no
 * edge crosses share one value and come as one span with it, so the inside of a shape costs
 * a fill, not a pixel loop.
 *
 * The accumulator is static and sized by WEUI_GFX_RASTER_WIDTH_MAX x WEUI_GFX_RASTER_BAND_ROWS:
 * a shape is rasterised a band of rows at a time. One shape at a time - a span callback must
 * not rasterise.
 */

/** @brief The widest clip a shape is rasterised in */
#ifndef WEUI_GFX_RASTER_WIDTH_MAX
#define WEUI_GFX_RASTER_WIDTH_MAX WEUI_DISPLAY_WIDTH
#endif /* WEUI_GFX_RASTER_WIDTH_MAX */

/** @brief Rows rasterised at a time: more is fewer passes over the segments, and more RAM */
#ifndef WEUI_GFX_RASTER_BAND_ROWS
#define WEUI_GFX_RASTER_BAND_ROWS 16
#endif /* WEUI_GFX_RASTER_BAND_ROWS */

/** @brief Coverage of a pixel wholly inside the shape */
#define WEUI_GFX_RASTER_COVER_FULL 255

/**
 * @brief One span: `len` pixels of row y from x. pcCov holds each one's coverage, or is NULL
 *        when all of them have `cov` - an edgeless run, never 0. A span with pcCov may hold
 *        zeros. Rows come top to bottom, spans in a row left to right.
 */
typedef void (*WeUi_Gfx_Raster_SpanCb_t)(void* pCtx, WeUi_Gfx_PtPos_t y, WeUi_Gfx_PtPos_t x,
										 u16 len, const u8* pcCov, u8 cov);

/**
 * @brief The pixels the path's coverage can reach: X0, Y0 included, X1, Y1 not - empty for a
 *        path with no points. For sizing a mask, or a clip, to a shape.
 * @return RET_STATE_ERR_PARAM for a Broken path.
 */
RET_STATE_t WeUi_Gfx_Raster_Bounds(const WeUi_Gfx_Path_t* pcPath, WeUi_Gfx_ClipRect_t* pBounds);

/** @brief WeUi_Gfx_Raster_Bounds() of a shape, which cannot be Broken, from its points - whatever
 *         its pcBounds holds */
RET_STATE_t WeUi_Gfx_Raster_ShapeBounds(const WeUi_Gfx_Shape_t* pcShape,
										WeUi_Gfx_ClipRect_t* pBounds);

/**
 * @brief Rasterise every contour of the path, each closed whether Closed or not, inside the
 *        clip. Parts of the shape left of the clip still count: a pixel at the clip's left
 *        edge is covered as it would be without it.
 * @return RET_STATE_ERR_PARAM for a Broken path or a clip wider than
 *         WEUI_GFX_RASTER_WIDTH_MAX; nothing is called then. A shape outside the clip is a
 *         success with no spans.
 */
RET_STATE_t WeUi_Gfx_Raster_Fill(const WeUi_Gfx_Path_t* pcPath, const WeUi_Gfx_ClipRect_t* pcClip,
								 WeUi_Gfx_Raster_SpanCb_t spanCb, void* pCtx);

/** @brief WeUi_Gfx_Raster_Fill() of a shape, which cannot be Broken */
RET_STATE_t WeUi_Gfx_Raster_FillShape(const WeUi_Gfx_Shape_t* pcShape,
									  const WeUi_Gfx_ClipRect_t* pcClip,
									  WeUi_Gfx_Raster_SpanCb_t spanCb, void* pCtx);

#endif /* __WEUI_GFX_RASTER_H */
