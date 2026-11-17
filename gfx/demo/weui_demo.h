#ifndef __WEUI_DEMO_H
#define __WEUI_DEMO_H

#include "weui_gfx_types.h"

/**
 * @file weui_demo.h
 * @brief The library's showpieces, one call each, about five seconds apiece.
 *
 * A demo paces and pushes its own frames and returns when it is over - the opposite
 * of a scene's Render(), which draws one frame and gets out. Play one as a one-shot
 * action, never as a Render(). Declared whatever WEUI_USE_COMPONENT__DEMO says: only
 * the .c files are gated.
 */

/* weui_demo_shapes.c */
void WeUi_DrawDemo_Primitives(void);
void WeUi_DrawDemo_CubeRotation(void);

/* weui_demo_text.c */
void WeUi_DrawDemo_Fonts(void);
void WeUi_DrawDemo_FontTransforms(void);
void WeUi_DrawDemo_Icons(void);

/* weui_demo_image.c - draws the assets the storage holds, so it is blank without them */
void WeUi_DrawDemo_Image(void);

/* weui_demo_qr.c - "Hello, world!" to scan off the panel; blank without
 * WEUI_USE_COMPONENT__WIDGET_QR, and the round leaves it out then */
void WeUi_DrawDemo_Qr(void);

/* weui_demo_transitions.c - every transition in turn, named on the picture it brings in */
void WeUi_DrawDemo_Transitions(void);

/* weui_demo_effects.c */
void WeUi_DrawDemo_Effects(void);
void WeUi_DrawDemo_Trails(void);
void WeUi_DrawDemo_Static(void);

/* weui_demo_sim.c */
#define WEUI_DEMO_TREE_MAX	2
#define WEUI_DEMO_GUEST_MAX 2

/**
 * @brief A garden of `trees` swaying trees, at most WEUI_DEMO_TREE_MAX, with `guests`
 *        trees at a time, at most WEUI_DEMO_GUEST_MAX, growing and going behind them.
 *        Every tree is redrawn each frame, so the cost grows with both counts.
 */
void WeUi_DrawDemo_FractalGarden(u8 trees, u8 guests);
void WeUi_DrawDemo_FractalTree(void); /* FractalGarden(4, 1) */
void WeUi_DrawDemo_Life(void);

/* weui_demo_fields.c - each field comes in over whatever is on the panel by itself */
void WeUi_DrawDemo_Fields(void);
void WeUi_DrawDemo_FieldSweep(void);
void WeUi_DrawDemo_FieldFire(void);
void WeUi_DrawDemo_FieldOcean(void);
void WeUi_DrawDemo_FieldAurora(void);
void WeUi_DrawDemo_FieldNebula(void);
void WeUi_DrawDemo_FieldSpiral(void);
void WeUi_DrawDemo_FieldToxic(void);

/**
 * weui_demo_ramps.c - a still test card: white, red, green and blue ramps over the full range
 * for the steps between levels, and a wide dark ramp for the blinking of the dimmest ones.
 * The same picture every time, so a panel's power draw can be compared on it.
 */
void WeUi_DrawDemo_Ramps(void);

/* weui_demo_round.c - the table of the demos a round plays, in its order */
typedef struct {
	const char* Name;
	VoidFunc_t Play;
	bool LiveIn; /* comes in over the panel by itself, as a field does */
} WeUi_Demo_t;

u32 WeUi_Demo_Count(void);

/** @return NULL past the table */
const WeUi_Demo_t* WeUi_Demo_Get(u32 idx);

/**
 * @brief Every demo of the table back to back, each led out into black by a random
 *        transition unless the next one comes in by itself. A board with a new panel
 *        calls it in a loop and sees all of the library.
 */
void WeUi_Demo_PlayRound(void);

#endif /* __WEUI_DEMO_H */
