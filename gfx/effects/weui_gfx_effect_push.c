#include "weui_gfx_effect_push.h"
#include "mathlib_common.h"
#include "weui_gfx_effect_compose.h"
#include "weui_gfx_effect_runner.h"

/* Out past the edge and back short of it, then home: on 90 px 11 over and 4 short.
 * On 25 px the floor keeps the peak at 3, and the swing back - a single row - is
 * dropped by WEUI_GFX_EFFECT_PUSH_SWING_MIN_PX */
static const WeUi_Gfx_Effect_PushCfg_t WeUi_Gfx_Effect_PushCfg_Def = {
	.OvershootShare = 0.12f,
	.OvershootMinPx = 3.0f,
	.OvershootMaxPx = 12.0f,
	.ApproachShare	= 0.4f,
	.BounceKeep		= 0.35f,
	.Bounces		= 1,
};

/* A swing back smaller than this is skipped: a single row of the old picture left
 * on the edge reads as a stuck line, not as a bounce. On a short axis that leaves
 * the overshoot and the way home */
#define WEUI_GFX_EFFECT_PUSH_SWING_MIN_PX 2.0f

typedef struct {
	WEUI_GFX_EFFECT_SLIDE_t Slide;
	const WeUi_Gfx_Effect_PushCfg_t* CfgPtr;
} WeUi_Gfx_Effect_PushParams_t;

/** @brief The slide as the compose uses it: travel axis, and whether it runs back
 *         along that axis. Resolved once a frame, not once a pixel */
typedef struct {
	bool Vertical;
	bool Reverse;
} WeUi_Gfx_Effect_SlideAxis_t;

const WeUi_Gfx_Effect_PushCfg_t* WeUi_Gfx_Effect_PushCfg_Default(void) {
	return &WeUi_Gfx_Effect_PushCfg_Def;
}

static WeUi_Gfx_Effect_SlideAxis_t WeUi_Gfx_Effect_SlideAxis(WEUI_GFX_EFFECT_SLIDE_t slide) {
	switch (slide) {
		case WEUI_GFX_EFFECT_SLIDE_LEFT:
			return (WeUi_Gfx_Effect_SlideAxis_t){.Vertical = false, .Reverse = true};

		case WEUI_GFX_EFFECT_SLIDE_DOWN:
			return (WeUi_Gfx_Effect_SlideAxis_t){.Vertical = true, .Reverse = false};

		case WEUI_GFX_EFFECT_SLIDE_UP:
			return (WeUi_Gfx_Effect_SlideAxis_t){.Vertical = true, .Reverse = true};

		case WEUI_GFX_EFFECT_SLIDE_RIGHT:
		default:
			return (WeUi_Gfx_Effect_SlideAxis_t){.Vertical = false, .Reverse = false};
	}
}

/* An approach that never ends, or swings that grow, puts NaN or runaway offsets
 * into the pixel loops - and offsets index the frame buffer */
static bool WeUi_Gfx_Effect_PushCfg_IsValid(const WeUi_Gfx_Effect_PushCfg_t* pcCfg) {
	return (pcCfg->ApproachShare > 0.0f) && (pcCfg->ApproachShare <= 1.0f) &&
		   (pcCfg->BounceKeep >= 0.0f) && (pcCfg->BounceKeep < 1.0f) &&
		   (pcCfg->OvershootShare >= 0.0f) && (pcCfg->OvershootMinPx >= 0.0f) &&
		   (pcCfg->OvershootMaxPx >= pcCfg->OvershootMinPx);
}

/**
 * @brief How far the push has travelled, in px.
 *
 * Swings are keyframed rather than integrated: a damped oscillator spends most
 * of its time under a pixel, which a low-res panel shows as a pause. Here every
 * swing has its share of the frames and its own amplitude, so each one that
 * rounds to a pixel is seen. Progress is normalized, so the shape holds whatever
 * the frame count is, and the last frame lands exactly on the span.
 */
static float WeUi_Gfx_Effect_PushTravel(const WeUi_Gfx_Effect_PushCfg_t* pcCfg, float progress,
										float span) {
	if (progress >= 1.0f)
		return span;

	float overshoot =
		CUT_MIN_MAX(span * pcCfg->OvershootShare, pcCfg->OvershootMinPx, pcCfg->OvershootMaxPx);

	/* A quarter turn of the sine: away at full speed, braking into the peak - the
	 * eye reads that as thrown, where a speed that builds up reads as pulled */
	if (progress < pcCfg->ApproachShare) {
		float phase = (progress / pcCfg->ApproachShare) * (float)D2R(90.0f);
		return (span + overshoot) * sinf(phase);
	}

	u32 bounces = 0;
	for (float amp = overshoot * pcCfg->BounceKeep;
		 bounces < pcCfg->Bounces && amp >= WEUI_GFX_EFFECT_PUSH_SWING_MIN_PX;
		 amp *= pcCfg->BounceKeep)
		bounces++;

	/* Half a cosine per swing: at rest on each extreme, fastest across the edge */
	float swings = (float)bounces + 1.0f;
	float pos	 = (progress - pcCfg->ApproachShare) / (1.0f - pcCfg->ApproachShare) * swings;
	u32 swing	 = GET_MIN((u32)pos, bounces);

	float from = overshoot;
	for (u32 idx = 0; idx < swing; idx++)
		from *= -pcCfg->BounceKeep;

	float to = (swing == bounces) ? 0.0f : from * -pcCfg->BounceKeep;

	return span + cos_interpol(from, to, pos - (float)swing);
}

/**
 * @brief Put a travel coordinate back on the panel.
 *
 * The compose below only knows `along`, which grows with the travel, and `cross`,
 * which runs the other way. Reverse is the forward pass read backwards: mirroring
 * the axis turns the travel around without mirroring what travels, since a
 * reflection maps a translation onto a translation.
 */
static WeUi_Gfx_Point_t WeUi_Gfx_Effect_SlidePt(WeUi_Gfx_Effect_SlideAxis_t axis,
												WeUi_Gfx_PtPos_t along, WeUi_Gfx_PtPos_t cross,
												WeUi_Gfx_PtPos_t span) {
	WeUi_Gfx_PtPos_t pos = axis.Reverse ? (span - 1 - along) : along;

	if (axis.Vertical)
		return (WeUi_Gfx_Point_t){.X = cross, .Y = pos};

	return (WeUi_Gfx_Point_t){.X = pos, .Y = cross};
}

/** @brief Move one pixel of the drawing frame onto itself, along the slide */
static void WeUi_Gfx_Effect_SlideDrawnPixel(WeUi_Gfx_Effect_SlideAxis_t axis,
											WeUi_Gfx_PtPos_t dstAlong, WeUi_Gfx_PtPos_t srcAlong,
											WeUi_Gfx_PtPos_t cross, WeUi_Gfx_PtPos_t span) {
	WeUi_Gfx_Point_t src = WeUi_Gfx_Effect_SlidePt(axis, srcAlong, cross, span);
	WeUi_Gfx_Point_t dst = WeUi_Gfx_Effect_SlidePt(axis, dstAlong, cross, span);

	WeUi_Gfx_Effect_MoveNew(dst.X, dst.Y, src.X, src.Y);
}

static void WeUi_Gfx_Effect_Compose_Push(const WeUi_Gfx_Effect_Ctx_t* pcCtx) {
	const WeUi_Gfx_Effect_PushParams_t* pcParams = pcCtx->ParamsPtr;
	WeUi_Gfx_Effect_SlideAxis_t axis			 = WeUi_Gfx_Effect_SlideAxis(pcParams->Slide);

	WeUi_Gfx_PtPos_t span  = (WeUi_Gfx_PtPos_t)(axis.Vertical ? pcCtx->Height : pcCtx->Width);
	WeUi_Gfx_PtPos_t lines = (WeUi_Gfx_PtPos_t)(axis.Vertical ? pcCtx->Width : pcCtx->Height);

	/* Travel is in pixels, pacing in frames - equal only at one frame per line.
	 * Rounded, not truncated: past the peak the spring moves under a pixel a frame
	 * and truncation would sit it on the wrong side of the whole one */
	float travel = WeUi_Gfx_Effect_PushTravel(pcParams->CfgPtr, pcCtx->Progress, (float)span);
	WeUi_Gfx_PtPos_t offset = (WeUi_Gfx_PtPos_t)(travel + 0.5f);

	/* Overshot, the incoming frame has dragged its own leading edge onto the panel
	 * and left bare lines behind it. That gap is what reads as the bounce - capped
	 * at the span, since an overshoot of a whole panel leaves nothing but gap */
	WeUi_Gfx_PtPos_t shift = offset - span; /* of the incoming frame over itself */
	WeUi_Gfx_PtPos_t gap   = (shift > 0) ? ((shift < span) ? shift : span) : 0;
	WeUi_Gfx_PtPos_t slid  = (shift > 0) ? span : offset;

	WeUi_Gfx_ColorRaw_t blank = WeUi_Gfx_Effect_Blank();

	for (WeUi_Gfx_PtPos_t cross = 0; cross < lines; cross++) {
		/* The incoming frame slides over itself, so the copy has to run away from
		 * the lines it writes - and that order flips with the slide direction */
		if (shift > 0) {
			for (WeUi_Gfx_PtPos_t a = span - 1; a >= gap; a--)
				WeUi_Gfx_Effect_SlideDrawnPixel(axis, a, a - shift, cross, span);
		} else {
			for (WeUi_Gfx_PtPos_t a = 0; a < slid; a++)
				WeUi_Gfx_Effect_SlideDrawnPixel(axis, a, a - shift, cross, span);
		}

		for (WeUi_Gfx_PtPos_t a = slid; a < span; a++) {
			WeUi_Gfx_Point_t src = WeUi_Gfx_Effect_SlidePt(axis, a - offset, cross, span);
			WeUi_Gfx_Point_t dst = WeUi_Gfx_Effect_SlidePt(axis, a, cross, span);
			WeUi_Gfx_Effect_PutOld(dst.X, dst.Y, src.X, src.Y);
		}

		/* Last: the slide reads these lines on its way past, so blanking them any
		 * earlier would drag the gap along with it, twice its width */
		for (WeUi_Gfx_PtPos_t a = 0; a < gap; a++) {
			WeUi_Gfx_Point_t dst = WeUi_Gfx_Effect_SlidePt(axis, a, cross, span);
			WeUi_Gfx_Effect_SetNew(dst.X, dst.Y, blank);
		}
	}
}

/* The same picture a row at a time. Along the slide, from its leading side: the gap left by an
 * overshoot, blank; the incoming picture from `shift` back; the old one from `offset` back */
static void WeUi_Gfx_Effect_Row_Push(const WeUi_Gfx_Effect_Ctx_t* pcCtx, WeUi_Gfx_PtPos_t y) {
	const WeUi_Gfx_Effect_PushParams_t* pcParams = pcCtx->ParamsPtr;
	WeUi_Gfx_Effect_SlideAxis_t axis			 = WeUi_Gfx_Effect_SlideAxis(pcParams->Slide);
	WeUi_Gfx_PtPos_t width						 = (WeUi_Gfx_PtPos_t)pcCtx->Width;
	WeUi_Gfx_PtPos_t span = (WeUi_Gfx_PtPos_t)(axis.Vertical ? pcCtx->Height : pcCtx->Width);

	float travel = WeUi_Gfx_Effect_PushTravel(pcParams->CfgPtr, pcCtx->Progress, (float)span);
	WeUi_Gfx_PtPos_t offset = (WeUi_Gfx_PtPos_t)(travel + 0.5f);
	WeUi_Gfx_PtPos_t shift	= offset - span;
	WeUi_Gfx_PtPos_t gap	= (shift > 0) ? ((shift < span) ? shift : span) : 0;
	WeUi_Gfx_PtPos_t slid	= (shift > 0) ? span : offset;
	WeUi_Gfx_ColorRaw_t blank = WeUi_Gfx_Effect_Blank();

	if (axis.Vertical) {
		WeUi_Gfx_PtPos_t a = axis.Reverse ? span - 1 - y : y;
		if (a < gap) {
			WeUi_Gfx_Effect_RowFill(0, width, blank);
			return;
		}
		bool next			 = a < slid;
		WeUi_Gfx_PtPos_t src = a - (next ? shift : offset);
		WeUi_Gfx_PtPos_t sy	 = axis.Reverse ? span - 1 - src : src;
		if (next)
			WeUi_Gfx_Effect_RowNew(0, 0, sy, width);
		else
			WeUi_Gfx_Effect_RowOld(0, 0, sy, width);
		return;
	}

	/* A stretch [a0; a1) along, read from `back` behind: reversed, it runs the other way along
	 * the row and reads `back` ahead */
	const struct {
		WeUi_Gfx_PtPos_t A0, A1, Back;
		u8 What; /* 0 blank, 1 incoming, 2 old */
	} stretches[] = {{0, gap, 0, 0}, {gap, slid, shift, 1}, {slid, span, offset, 2}};
	for (u32 i = 0; i < NUM_ELEMENTS(stretches); i++) {
		WeUi_Gfx_PtPos_t n = stretches[i].A1 - stretches[i].A0;
		if (n <= 0)
			continue;
		WeUi_Gfx_PtPos_t x	= axis.Reverse ? span - stretches[i].A1 : stretches[i].A0;
		WeUi_Gfx_PtPos_t sx = axis.Reverse ? x + stretches[i].Back : x - stretches[i].Back;
		if (stretches[i].What == 0)
			WeUi_Gfx_Effect_RowFill(x, n, blank);
		else if (stretches[i].What == 1)
			WeUi_Gfx_Effect_RowNew(x, sx, y, n);
		else
			WeUi_Gfx_Effect_RowOld(x, sx, y, n);
	}
}

/* The whole run on the approach, nothing past the edge: the swings are never reached */
static const WeUi_Gfx_Effect_PushCfg_t WeUi_Gfx_Effect_SwipeCfg = {
	.ApproachShare = 1.0f,
};

RET_STATE_t WeUi_Gfx_Effect_Swipe(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
								  WEUI_GFX_EFFECT_SLIDE_t slide) {
	return WeUi_Gfx_Effect_Push(renderIncoming, frames, slide, &WeUi_Gfx_Effect_SwipeCfg);
}

RET_STATE_t WeUi_Gfx_Effect_Push(WeUi_Gfx_Effect_RenderCb_t renderIncoming, u16 frames,
								 WEUI_GFX_EFFECT_SLIDE_t slide,
								 const WeUi_Gfx_Effect_PushCfg_t* pcCfg) {
	if (!pcCfg)
		pcCfg = &WeUi_Gfx_Effect_PushCfg_Def;

	if (!WeUi_Gfx_Effect_PushCfg_IsValid(pcCfg))
		return RET_STATE_ERR_PARAM;

	WeUi_Gfx_Effect_PushParams_t params = {.Slide = slide, .CfgPtr = pcCfg};

	return WeUi_Gfx_Effect_RunRows(WeUi_Gfx_Effect_Row_Push, WeUi_Gfx_Effect_Compose_Push, frames,
								   &params, renderIncoming);
}
