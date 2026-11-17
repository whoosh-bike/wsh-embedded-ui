# Effects

Every effect at a glance: what it looks like, and what to know before using it. One
effect a file, `weui_gfx_effect_<name>.c/h`, all of them reached through
`weui_gfx_effects.h`. The demo `WeUi_DrawDemo_Transitions` plays each transition with its
name on it.

## Common to all transitions

- `frames` is the length of the run, paced by whoever draws the incoming scene. Think of
  it in ms: `WeUi_Gfx_Effect_Frames(ms, fps)` turns a length into frames at the rate they
  will be drawn.
- `renderIncoming` draws one live frame of the arriving scene: it must clear the drawing
  frame itself and must not push it. The old picture is whatever is on the panel when the
  call is made.
- The run blocks until it is over and returns `RET_STATE_ERR_BUSY` if another effect is
  playing: they cannot nest.
- No transition takes a frame buffer of its own - the drawing frame and the aux frame are
  all there is, which is why some of them work in a set order.

## Between scenes

A transition between scenes is a function of the app's that plays one effect, or a few in
a row, into the `renderIncoming` it is handed:

```c
#define DISSOLVE_MS 400

static void Dissolve(VoidFunc_t renderIncoming) {
	WeUi_Gfx_Effect_Dissolve(renderIncoming,
							 WeUi_Gfx_Effect_Frames(DISSOLVE_MS, WEUI_GFX_FPS_CTRL_DEF_RENDER_FREQ));
}
```

The scene manager plays it: `return WeUi_Gfx_SceneMgr_GoTo(target, Dissolve);` from an
`OnInput`, `cfg.Intro` on start-up, `WeUi_Gfx_SceneMgr_PlayIntoActive(Dissolve)` to hand
the panel back after a notice. It draws the incoming scene at the main render rate, which
is why the length converts at the default one - and why a rate lowered from the shell
slows every transition down for a closer look.

## Painters and solo effects

| Effect | Looks like | Worth knowing |
| --- | --- | --- |
| `Plasma_Paint` / `Plasma_Play` | Three sine waves beating into a shifting colour field | `Paint` is one frame for a scene to drive, `Play` the whole animation blooming out of the dark. Look per call through `WeUi_Gfx_Effect_PlasmaCfg_t` |
| `Perlin_Paint` / `Perlin_Play` | Noise clouds condensing out of the dark | Same split as plasma |
| `Perlin_PaintField` | The same clouds, every part moved on its own | Time, threshold, hue and offset are the caller's: the breathing, drifting field of `WeUi_DrawDemo_Effects` |
| `Static_Paint` | TV static: every pixel a random hue, or off | One frame, nothing to configure; drive it at the rate the grain should crawl |

## Transitions

### Fades - the whole picture changes at once

| Effect | Looks like | Worth knowing |
| --- | --- | --- |
| `Dim` | The old picture dims to black, the new one comes up out of it | The two never mix |
| `Fade` | The old picture melts into the new one | Mixed per pixel; unlike `Dim` it never goes dark |
| `Flash` | The old picture burns out to white, the new one comes up out of it | |

### Slides - pictures move

| Effect | Looks like | Worth knowing |
| --- | --- | --- |
| `Push` | Both pictures slide together; the new one overshoots and springs back | Shape per call through `WeUi_Gfx_Effect_PushCfg_t`. The bounce is in px, clamped, so a long and a short axis bounce alike; a swing under 2 px is skipped. While past the edge, the trailing lines are blank |
| `Swipe` | `Push` without the bounce: thrown in, braking onto the edge | |
| `Cover` | One picture slides over the other, which stands still | `WEUI_GFX_EFFECT_COVER_IN` brings the new one in over the old; `_OFF` slides the old one off the new |
| `Curtain` | The old picture parts down the middle, halves drawn off to the sides | `WEUI_GFX_EFFECT_AXIS_VERTICAL` parts it up and down instead |
| `Interlace` | The new picture slides in row by row, alternate rows from either side | |

### Reveals - the new picture shows through a moving edge

| Effect | Looks like | Worth knowing |
| --- | --- | --- |
| `Dissolve` | The new picture bleeds through pixel by pixel | Too few frames turn it over in visible clumps |
| `Blocks` | The new picture turns over block by block, in scattered order | A dissolve in 6x5 blocks; the order is the same every run |
| `Blinds` | The new picture opens slat by slat, each slat from its top | Five slats down the panel |
| `Reload` | The old picture is wiped off left to right to black, the new one drawn in the same way | Half the run dark-going, half coming |
| `Wave` | The new picture floods in left to right behind a wavering front | |
| `Rain` | The new picture runs down column by column, each starting when it will | `columnPx` sets the column width: 1 is a drizzle, wider falls in sheets |
| `Melt` | The old picture's columns sag and drop off, the new one above | `columnPx` as for `Rain`; a few px melt in chunks as Doom's screen did |
| `Ripple` | The new picture spreads out of the centre behind a wavering, lit crest | `WEUI_GFX_EFFECT_RIPPLE_DIAMOND` makes it a diamond reaching every corner at once. The costliest: `atan2` and a root per pixel |

### Warps - the picture is reshaped

| Effect | Looks like | Worth knowing |
| --- | --- | --- |
| `Zoom` | The new picture grows out of the centre, scaled whole | Nearest-pixel scaling: small, the picture reads as noise. Shrinks in place, two passes a frame |
| `Rotate` | A card turned on its vertical axis: the old side narrows to an edge, the new opens out | A flip, not a turn in the plane - on a wide panel a turned picture would leave its corners bare |
| `Pixelate` | The old picture breaks into coarse blocks, the new one sharpens out of them | Coarsest block is a share of the panel height |

## Writing one

A transition is a compose callback handed to `WeUi_Gfx_Effect_RunTransition`: per frame
the runner draws the incoming scene, calls the compose with the progress, then pushes.
The compose puts the old picture - parked in the aux frame - back over the new one
wherever the old one still shows. `weui_gfx_effect_compose.h` is its kit, private to
`effects/`: `PutOld`/`PutOldFrame` for the old picture's pixels, `Smooth` for the default
ease, and `Blank`, `Lighten`, `Turn`, `ColumnRun`. Pixels move raw, so the driver's
brightness is not applied twice.

A solo effect is a paint callback handed to `WeUi_Gfx_Effect_RunSolo`.

A new effect gets a file pair of its own, a line in `weui_gfx_effects.h`, a row here and
a step in the transitions demo.
