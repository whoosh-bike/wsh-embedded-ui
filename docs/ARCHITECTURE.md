# Architecture

## Overview

weui draws into a frame buffer owned by the display driver and pushes it to the panel. It
has no task, no clock and no hardware knowledge: the panel is behind the `display_m`
contract, and time, delays and critical sections come from the host's `weui_cfg.h`.

```
Application
  scenes, navigation, overlay, the loop
        |
        v
weui graphics
  draw / text / image / anima / widgets / effects / scenemgr
        |
        v
adapter (gfx/core/weui_gfx_adapter)
  cached panel parameters, colour conversion, flip
        |
        v
display_m driver (host BSP)
  pixels, frames, brightness, DMA
        |
        v
panel
```

## Modules

| Folder | Responsibility |
|---|---|
| `gfx/core/` | Config check, types, adapter, colour, transforms, FPS control |
| `gfx/draw/` | Points, lines, rectangles, circles, frame fade, fractal tree |
| `gfx/text/` | u8g2 text rendering |
| `gfx/image/` | Image rendering and Floyd-Steinberg dithering |
| `gfx/anima/` | Frame playback and compressed packets |
| `gfx/widgets/` | QR code and wrapped/aligned text block |
| `gfx/effects/` | Painters, solo effects, transitions |
| `gfx/scenemgr/` | Scene table, message ring, timeouts, overlay, transitions |
| `gfx/sim/` | Game of Life |
| `gfx/vector/` | Affine transforms, paths, antialiased raster, fills, masks, strokes, damage-tracked scenes |
| `gfx/demo/` | Showpieces, one-shot actions |
| `common/` | Colour types, PRNG, FPS helpers |
| `fs/` | Asset storage |
| `ind/` | LED-strip indication (optional) |
| `external/fonts/` | u8g2 |

Headers are included by basename; `weui.mk` puts every library folder on the include path.
`gfx/core/` is the leaf: everything else may depend on it; it depends on the config and the
base types only.

## Render loop

The host owns the loop. `WeUi_Gfx_SceneMgr_Poll()` is one turn:

```
Poll()
  first call only: enter the home scene
  drain the message ring (never blocks)
    GOTO   -> Exit old, clear if requested, Enter new, play transition
    CLEAR  -> switch to the clear scene, then call OnDone
    ACTION -> run a posted function once, scene untouched
    INPUT  -> scene OnInput() or cfg.OnInput(); may return a GoTo()
  scene timeout, if due -> cfg.OnTimeout()
  if a frame is due:
    scene Render()          one frame, no clear and no push
    cfg.Overlay()           optional, on top of the scene
    clear or copy frame, DrawFrame()
    schedule the next frame from the scene's Fps
  return the ms the host may sleep
```

Every drawing call ends in `WeUi_Gfx_Point_Draw()` -> adapter -> `DisplayM_SetPixel()`. A
scene switch is synchronous: `Exit()`, `Enter()` and the transition all run inside
`Poll()`. Transitions block the loop for their duration; keep them bounded, or kick the
watchdog from `WeUi_Gfx_FpsCtrl_AttachAppCallback()`, which effects keep calling.

## Vector core

`gfx/vector/` draws antialiased shapes from float coordinates, independently of the integer
primitive API:

| Module | Responsibility |
|---|---|
| `weui_gfx_affine` | Float point, affine matrix, transform stack |
| `weui_gfx_path` | Outline builder; curves flattened on the panel (Wang's formula) |
| `weui_gfx_raster` | Exact coverage spans inside a clip rectangle, nonzero fill rule |
| `weui_gfx_fill` | Solid colour, linear/radial gradients, opacity, soft spots |
| `weui_gfx_mask` | Masks built from shapes; a paint keeps inside or outside |
| `weui_gfx_stroke` | Stroke or outline of a path, as another path: joins and caps |
| `weui_gfx_scene` | A frame's items kept for redraw; damage rectangles; `Present()` |

The core draws one shape at a time: the rasteriser accumulator, the stroke contour buffer
and the fill colour tables are static. A scene is the caller's storage and keeps its items,
so only the rectangles that changed since the previous frame are redrawn.

`WeUi_Gfx_Scene_Present()` sends the damaged rectangles to either kind of driver: with a
frame driver it copies the last picture back and draws the rectangles; with a band driver
it borrows a band, draws whole rows into it and sends it back. See "Band output" below.

Unlike the rest of the library, the vector fill blends coverage through the raw pixel
accessors: the stored pixels may already have been through the driver's brightness LUT,
and running the blend through `SetPixel` would scale them a second time.

## Scenes

A scene is a `WeUi_Gfx_SceneVtbl_t` (see `weui_gfx_scenemgr.h`):

- `Render(RefPt)` draws one frame. It must not clear or push the frame.
- `Fps` caps the render rate; `Fps <= 0` renders once on `Enter()` and never self-ticks.
- `Enter()` / `Exit()` set up and tear down per-scene state.
- `OnInput()` handles input where it differs from the app-wide `cfg.OnInput`.
- `FullClearOnEnter` arrives from black; `KeepFrame` builds on the previous frame.

The app supplies the policy: which scenes exist, the home scene, navigation, timeouts and
which transition plays on a switch. The engine owns only the mechanism.

Messages are non-blocking and safe from an interrupt:

```c
WeUi_Gfx_SceneMgr_PostInput(&event, sizeof(event));
WeUi_Gfx_SceneMgr_RequestScene(AppSceneX);
WeUi_Gfx_SceneMgr_PostAction(SomeNotice_Play);
WeUi_Gfx_SceneMgr_RequestClear(OnCleared, ctx);
```

The ring is statically sized by `WEUI_GFX_SCENEMGR_QUEUE_DEPTH` (default 8). A full ring
drops the event.

A transition is an app function `void (*)(VoidFunc_t renderIncoming)` that plays one or
more effects into the callback the engine hands it. `GoTo(target, transition)` returns the
target for `OnInput()`/`OnTimeout()` to return and stores the transition for the switch that
actually happens.

## Effects

Three levels, each a wrapper over the one below:

| Level | Needs | Driver | Examples |
|---|---|---|---|
| Painter | a progress value | the caller | `Plasma_Paint`, `Perlin_Paint`, `Static_Paint` |
| Solo | nothing | `WeUi_Gfx_Effect_RunSolo` | `Plasma_Play`, `Perlin_Play` |
| Transition | aux frame + `renderIncoming` | `WeUi_Gfx_Effect_RunTransition` | fade, push, dissolve, zoom, ... |

A painter draws one frame and returns; a solo effect loops it and pushes frames; a
transition mixes the outgoing picture, parked in the driver's aux frame, with the incoming
scene. Effects cannot nest (`RET_STATE_ERR_BUSY`). Lengths are in frames;
`WeUi_Gfx_Effect_Frames(ms, fps)` converts from milliseconds.

The catalogue, with what each effect looks like, is in
[gfx/effects/EFFECTS.md](../gfx/effects/EFFECTS.md).

## Display driver contract

`gfx/core/weui_gfx_adapter.c` is the only caller of the panel. The host writes
`display_m.h` with these entry points:

```c
typedef struct {
    u16 Width, Height;          /* at most WEUI_DISPLAY_WIDTH x WEUI_DISPLAY_HEIGHT */
    u16 X_Min, Y_Min, MidX, Y_Mid, X_Max, Y_Max;
    u32 PixelNum;
    u8  ColorMin, ColorMid, ColorMax;
} DisplayM_Params_t;

DisplayM_Params_t DisplayM_GetParams(void);
void DisplayM_SetPixel(u16 x, u16 y, u32 color);     /* drawing frame, brightness applied */
u32  DisplayM_GetPixel(u16 x, u16 y);                /* drawing frame, as stored */
void DisplayM_SetPixelRaw(u16 x, u16 y, u32 color);  /* drawing frame, stored as given */
u32  DisplayM_GetPixelAux(u16 x, u16 y);             /* aux frame */
void DisplayM_FillDrawingFrame(u32 color);
void DisplayM_FillDisplayAndDrawingFrames(u32 color);
void DisplayM_DrawFrame(void);                       /* push the drawing frame */
void DisplayM_CopyFrame_DisplayToDrawing(void);
void DisplayM_SaveFrame_DisplayToAux(void);

/* With WEUI_DISPLAY_FRAME_MEM: a frame as memory. frame 0 drawing, 1 display, 2 aux;
 * px 0 = u32 raw, 1 = 565, 2 = 565 high byte first. NULL if the frame is not kept. */
void* DisplayM_GetFrameMem(u8 frame, u32* pStride, u8* pPx);

/* With WEUI_DISPLAY_BANDS: a free band buffer and its size in pixels, and its return. */
u16* DisplayM_BandGet(u32* pPixels);
void DisplayM_BandSend(u16 x0, u16 y0, u16 x1, u16 y1, u16* pBuf);
```

- Pixels are `u32` in `0x00BBGGRR` order, as `WeUi_Gfx_Transform_Color2Bytes()` packs them.
  Coordinates are already inside the panel.
- Apply brightness only in `SetPixel` and the fills. The raw accessors move pixels that were
  scaled already; reading with `GetPixel` and drawing again through `SetPixel` scales twice.
- After `DrawFrame()` the drawing frame may hold anything (DMA drivers swap buffers), so the
  library clears or copies it before drawing.
- The aux frame is the driver's and is written only by `SaveFrame_DisplayToAux()`.
  Transitions park the outgoing picture there.
- `WEUI_DISPLAY_FLIP_X` / `_Y` mirror in the adapter, so the driver always sees its own
  orientation.

### Band output

A large panel may keep no frame in memory. With `WEUI_DISPLAY_BANDS` the driver supplies
`DisplayM_BandGet()` / `DisplayM_BandSend()` instead; the frame calls stay but do nothing.
Only a vector scene is drawn then: `WeUi_Gfx_Scene_Present()` borrows a band, draws whole
rows into it, sends it back, and repeats, so the next band is drawn while one goes out.

A band takes colours as stored, and brightness and mirroring are the panel's own:
`WEUI_DISPLAY_FLIP_X` / `_Y` are refused with bands. `WEUI_DISPLAY_BAND_FORMAT` selects the
pixel format and `WEUI_DISPLAY_BAND_ALIGN` the x alignment of a band.

`host/sim/display_sim.c` is a reference implementation over host memory.

## Configuration

The application provides `weui_cfg.h`. `gfx/core/weui_cfg_check.h` is included right after
it, rejects impossible combinations and fills in defaults, so a minimal file only sets the
panel size, the colour scheme and the platform bindings.

| Group | Main options |
|---|---|
| Colour | `WEUI_LIB_COLOR_SCHEME__RGB` / `__GRAY`, `WEUI_LIB_COLOR_ALPHA_CHANNEL`, `WEUI_IMG_STORAGE_COLOR_SCHEME__*` |
| Display | `WEUI_DISPLAY_WIDTH`, `WEUI_DISPLAY_HEIGHT`, `WEUI_DISPLAY_COLOR` / `_GRAY`, `WEUI_DISPLAY_FLIP_X/Y` |
| Display output | `WEUI_DISPLAY_BANDS` (0), `_BAND_FORMAT`, `_BAND_ALIGN` (1), `WEUI_DISPLAY_FRAME_MEM` (0) |
| Memory placement | `WEUI_DATA_PLACEMENT`, `WEUI_IMAGES_OPS_DATA_PLACEMENT` |
| Components | `WEUI_USE_COMPONENT__IMAGE`, `_IMAGE_DITHER`, `_IMAGE_COMPRESS`, `_WIDGET_QR` |
| Asset storage | `WEUI_FS_USE_INT_STORAGE`, `_EXT_STORAGE`, `_FAT_SYNC`, `WEUI_FS_JSON_*` |
| FPS | `WEUI_FPS_CTRL_MIN/DEF/MAX_RENDER_FREQ` (1 / 60 / 120) |
| Scene manager | `WEUI_GFX_SCENEMGR_QUEUE_DEPTH` (8), `_INPUT_MAX_BYTES` (16), `_PACE_FROM_FRAME_END` (0) |
| Vector core | `WEUI_GFX_AFFINE_STACK_DEPTH` (8), `_PATH_PTS_MAX` (256), `_PATH_CONTOURS_MAX` (16), `_PATH_TOLERANCE` (0.2), `_RASTER_WIDTH_MAX` (panel width), `_RASTER_BAND_ROWS` (16), `_FILL_LUTS` (1), `_SCENE_ITEMS_MAX` (32), `_SCENE_PTS_MAX` (512), `_SCENE_CONTOURS_MAX` (64), `_SCENE_STOPS_MAX` (32), `_SCENE_MASKS_CACHED` (4), `_SCENE_DAMAGE_GAP` (2), `_SCENE_DAMAGE_ALIGN` (1) |
| Platform | `WEUI_USE_RTOS` / `WEUI_USE_BAREMETAL`, `WEUI_USE_CUSTOM_ALLOC`, `WEUI_TICK_GET_MS_CNT()`, `WEUI_DELAY_MS()`, `WEUI_DELAY_UNTIL_MS()`, `WEUI_ENTER/EXIT_CRITICAL()` |
| Build | `WEUI_USE_IND` (1), `WEUI_USE_DEMO` (1) in `weui.mk` |

The authoritative list, with defaults and checks, is `gfx/core/weui_cfg_check.h`.

## Memory

| Buffer | Size | Owner |
|---|---|---|
| Display frame | W x H x pixel size | display driver |
| Drawing frame | W x H x pixel size | display driver |
| Aux frame | W x H x pixel size | display driver |
| Image buffer one | W x H x pixel size | library, `WEUI_IMAGES_OPS_DATA_PLACEMENT` |
| Image buffer two | W x H x pixel size | library, `WEUI_IMAGES_OPS_DATA_PLACEMENT` |

`W` and `H` are `WEUI_DISPLAY_WIDTH` / `HEIGHT`, the largest panel the build supports, not
the panel actually attached. Pixel size is 1 byte (gray), 3 (RGB) or 4 (RGB + alpha).

A driver that keeps its frames in memory may expose them (`WEUI_DISPLAY_FRAME_MEM`,
`DisplayM_GetFrameMem`). Effects then compose on those pixels directly instead of calling
the driver for every pixel; the picture is identical, which `make check FRAME_MEM=1`
checks against the same reference. With `WEUI_DISPLAY_BANDS` there are no frames at all and
only a vector scene is drawn.

`WEUI_DATA_PLACEMENT` places the library's large static buffers in a named section (for
example SRAM2 or CCM): image buffers, Game of Life cells, the plasma and aurora caches, the
vector rasteriser accumulator, fill tables, span rows, stroke contour copy and demo paths.
`WEUI_IMAGES_OPS_DATA_PLACEMENT` overrides the image buffers; it falls back to
`WEUI_DATA_PLACEMENT`.

Almost everything else is static. The QR widget allocates two qrcodegen buffers per call
through `WEUI_ALLOC` / `WEUI_FREE`; `WeUi_Fs_ForceSync()` allocates one copy chunk through
`WEUI_FS_ALLOC`.

## Assets

Images and animations are stored as a JSON header followed by pixel data:

```
[JSON header][binary pixels]
```

```json
{
  "name": "logo",
  "type": "img",
  "comp": "false",
  "widt": 32,
  "heig": 8,
  "size": 768,
  "scheme": 1,
  "alpha": 0
}
```

`scheme` is `0` for gray and `1` for RGB; `alpha` is `0` or `1`. Names are at most 31
bytes. `utils/convert-graphics.py` produces this format.

Storage backends:

- `STORAGE_TYPE_RAM` - C arrays linked into the firmware.
- `STORAGE_TYPE_INT_FLASH` - a linker-defined region of the MCU flash.
- `STORAGE_TYPE_EXT_FLASH` - a linear region of external SPI flash.

`WeUi_Fs_Storage_Scan()` discovers assets at start-up, `WeUi_Fs_Storage_Fetch()` returns
metadata by name and `WeUi_Fs_Storage_Read()` loads pixels.

## Dependencies

- **u8g2** - font parsing and rendering, vendored in `external/fonts/`.
- **wsh-embedded-libs** - base types, macros, `mathlib_common`, `str`, `json`, linked list.
  Provided by the host; the tests expect a sibling checkout.
- **qrcodegen** - optional, QR widget.
- **lz4_wrapper** - optional, compressed images.
- **jsmn** - optional, asset metadata.

## Testing

`tests/` runs the library on the PC against `host/sim/display_sim.c`, under ASan and UBSan,
with a virtual clock. Regression tests assert properties of the output; golden runs compare
a hash of every pushed frame with a recorded reference and write PNGs to `out/`.
`make BANDS=1` runs the tests with band output, `make FRAME_MEM=1` with effects composing
on the frames as memory. See [tests/README.md](../tests/README.md).
