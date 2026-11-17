# weui

Small embedded graphics for low-resolution panels: LED matrices, small OLED and e-ink
displays. It draws primitives, u8g2 text, images and frame animations, plays whole-frame
effects and scene transitions, and runs a poll-driven scene manager. `ind/` adds effects
for LED strips.

The library knows no panel and no RTOS. The panel sits behind the `display_m` driver
contract; time, delays and critical sections come from the host's `weui_cfg.h`.

Repository: `wsh-embedded-ui` · project name: `weui`.

## Features

- **Drawing** - points, lines, rectangles, circles, hatching, frame fade.
- **Text** - u8g2 fonts, rotation, wrapped and aligned text blocks.
- **Images and animation** - assets in RAM, internal flash or external flash, LZ4
  decompression, Floyd-Steinberg dithering.
- **Effects** - 19 scene transitions, plasma/perlin/static painters, solo animations.
- **Scene manager** - app-supplied scene table, message ring, FPS pacing, timeouts,
  an app-wide overlay, transitions on a switch.
- **Colour** - RGB or grayscale; 1, 3 or 4 bytes per pixel.
- **LED strips** - optional `ind/` subsystem, bound to a host `led_strip_m` driver.

## Requirements

- C23 (`-std=gnu23`) for firmware. The reference build uses `arm-none-eabi-gcc` 15+;
  generated assets that rely on `#embed` need GCC 15 as well.
- **wsh-embedded-libs** - base types (`u8`..`u64`, `RET_STATE_t`), macros,
  `mathlib_common` and containers. Provided by the host; the tests expect a sibling
  checkout. The host supplies `wsh_emblib_cfg.h` with `PANIC()` and `ASSERT_CHECK()`.
- Optional: `qrcodegen` for the QR widget, `jsmn` and the linked list from
  wsh-embedded-libs for asset storage, `lz4_wrapper` for compressed images.

## Integration

With Make:

```make
WEUI_DIR := thirdparty/wsh-embedded-ui
include $(WEUI_DIR)/weui.mk
SRC      += $(WEUI_SRC)
CFLAGS   += $(WEUI_INC)
```

Set these before the include when the host does not need a subsystem:

```make
WEUI_USE_IND  := 0   # no LED strip
WEUI_USE_DEMO := 0   # production build without the demo showpieces
```

The host provides:

1. **`weui_cfg.h`** - board and platform configuration. `gfx/core/weui_cfg_check.h`
   validates it and fills in defaults, so the minimum is:

   ```c
   #include "FreeRTOS.h"
   #include "mathlib_common.h"
   #include "task.h"

   #define WEUI_DISPLAY_WIDTH 32 /* the largest panel this build supports */
   #define WEUI_DISPLAY_HEIGHT 8
   #define WEUI_DISPLAY_COLOR
   #define WEUI_LIB_COLOR_SCHEME__RGB

   #define WEUI_TICK_GET_MS_CNT()          xTaskGetTickCount()
   #define WEUI_DELAY_MS(a)                vTaskDelay(a)
   #define WEUI_DELAY_UNTIL_MS(prev, incr) xTaskDelayUntil(prev, incr)

   #define WEUI_CRITICAL_DECL()  UBaseType_t weuiCriticalMask
   #define WEUI_ENTER_CRITICAL() (weuiCriticalMask = portSET_INTERRUPT_MASK_FROM_ISR())
   #define WEUI_EXIT_CRITICAL()  portCLEAR_INTERRUPT_MASK_FROM_ISR(weuiCriticalMask)
   ```

2. **`display_m.h`** and its driver - the pixel, frame and brightness calls listed in the
   [display driver contract](docs/ARCHITECTURE.md#display-driver-contract).
3. **`ind_cfg.h`** and **`led_strip_m.h`** - only when `WEUI_USE_IND` is on.

Start-up: bring the panel up, then `WeUi_Gfx_Adapter_Init()` (it rejects a panel larger than
`WEUI_DISPLAY_WIDTH` x `WEUI_DISPLAY_HEIGHT`), `WeUi_Gfx_Text_Init()` and
`WeUi_Gfx_SceneMgr_Init()` with the app's scenes, navigation and transitions. The host loop
calls `WeUi_Gfx_SceneMgr_Poll()` and sleeps for the interval it returns.

## Repository layout

| Path | Contents |
|---|---|
| `gfx/core/` | types, config check, display adapter, colour, transforms, FPS control |
| `gfx/draw/` | primitives and the fractal tree |
| `gfx/text/` | u8g2 text rendering |
| `gfx/image/` | images and dithering |
| `gfx/anima/` | frame playback and compressed packets |
| `gfx/effects/` | painters, solo effects and transitions (see `EFFECTS.md`) |
| `gfx/widgets/` | QR code and wrapped text block |
| `gfx/scenemgr/` | scene engine, message ring, timeouts, overlay |
| `gfx/sim/` | Game of Life |
| `gfx/demo/` | demo showpieces |
| `ind/` | LED-strip indication |
| `common/` | colour types, PRNG, FPS helpers |
| `fs/` | asset storage (JSON metadata + pixel data) |
| `external/fonts/` | u8g2 |
| `host/sim/` | `display_m` over host memory for tests and the lab |
| `tests/` | host test suite |
| `tools/lab/` | the library as wasm in a browser |
| `utils/` | `convert-graphics.py` asset converter |

## Tests

```sh
cd tests
make check
```

The library runs on the PC against a simulated panel under AddressSanitizer and
UndefinedBehaviorSanitizer. Regression tests assert properties of the output; golden runs
compare hashes of every pushed frame with a recorded reference and leave PNGs to inspect.
A sibling `wsh-embedded-libs` checkout is expected; use `EMBLIB=<path>` for another
location. See [tests/README.md](tests/README.md).

## Browser lab

```sh
cd tools/lab
npm install
npm run dev
```

The library's C compiled to wasm: demos or an app's scenes on a timeline, a pixel loupe and
a rebuild on save. Needs Node.js 20+, Emscripten and a `wsh-embedded-libs` checkout. See
[tools/lab/README.md](tools/lab/README.md).

## Documentation

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - layers, render loop, scene manager,
  driver contract, configuration, memory.
- [docs/ROADMAP.md](docs/ROADMAP.md) - what exists and what is planned.
- [gfx/effects/EFFECTS.md](gfx/effects/EFFECTS.md) - every effect at a glance.
- [AGENTS.md](AGENTS.md) - conventions for contributors and coding agents.
- [CONTRIBUTING.md](CONTRIBUTING.md) - branches, review and tests.

## License

MIT - see [LICENSE](LICENSE). u8g2, the bundled fonts and the lab typefaces keep their own
licenses; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Mirrors

The canonical repository is on GitLab. GitHub mirrors `main` and `dev` read-only; issues and
merge requests belong on GitLab.
