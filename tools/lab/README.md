# weui lab

The library's C compiled to WebAssembly and played in the browser: every frame a run
pushes lands on a timeline you can pause, scrub and step, with a loupe on any pixel. It
is the same code as on the device, drawn into the same simulated panel the host tests use
(`host/sim`), so what you see is what the firmware would push.

```sh
cd tools/lab
npm install        # once
npm run dev        # builds the wasm, opens http://localhost:5173
```

Needs Node 20+ and Emscripten (`brew install emscripten`), plus a checkout of
`wsh-embedded-libs` next to this repository (`EMBLIB=<path>` otherwise).

## What it does

- **Runs** a target's actions - the library demos by default - from a fresh start, so a
  run replays bit for bit. Time is virtual: a ten-second animation computes in
  milliseconds and then plays on its own timestamps.
- **Timeline**: play/pause, frame steps, speed 0.1x-4x, A-B loop over one transition.
  Shaded spans are frames pushed while an effect was running. The strips are in virtual
  time, not frames: a scene that holds still for ten seconds takes ten seconds of strip.
- **Scenes**, for a target on the scene manager: a strip of which scene was up when, with
  the transitions and the actions (a popup, a demo) that played over it, named by their C
  functions - `GfxUi_Transition_ScreenOff` reads `screen_off` - and a tick where an input
  or a timeout came in; hover for what it was. Beside the panel the scene at the playhead,
  how long it has been up and how it came; under the panel a graph of the moves the run
  made - each why, with what, how often; a click seeks to one. The console has each step as a SCENE
  line.
- **Pixel loupe**: hover or pin a pixel - its stored value, RGB, HSB in the library's
  units, luma, the value before the brightness LUT; copy it as C. A pinned pixel draws its
  channels over the whole run: the curve a fade or a spring really produced.
- **Layout**: the bars between the parts drag - the side columns, the console, the panel
  against the graph; dragged to the end a part folds away, a double-click resets a bar.
  Kept in the browser.
- **Views**: LED dots or flat pixels, grid, diff against the previous frame, zoom; panel
  size, colour variant (COLOR-RGB, GRAY-GRAY) and driver brightness.
- **Rebuild on save**: a saved `.c`/`.h` of the library or the target rebuilds the wasm
  and reloads the page on the same run and frame. A compile error shows in the page.
- **Console**: `LOG()`, `LOG_LVL()` and `printf` from the C, each line with the frame it
  was drawn on and the virtual time; lines past the playhead are dimmed, a click seeks.
  Filter by text and level.
- **PANIC/ASSERT** in the C stop nothing: the page marks the frame and names file:line.

Keys: Space play, ←/→ frame (Shift x10), Home/End, A/B loop ends, Esc clears the loop,
D diff, L look.

## Targets

A target is the C to look at: a directory with a `target.mk` and a `lab_target.c`.

```make
# target.mk
TARGET_NAME := my-app
TARGET_SRC  := $(TARGET_DIR)/lab_target.c $(APP)/ui/scenes.c   # sources, beside the library
TARGET_INC  := -I$(TARGET_DIR)/stubs -I$(APP)/ui                 # include paths
TARGET_W    ?= 128                                              # the largest panel it runs on
TARGET_H    ?= 64
TARGET_CFLAGS :=                                                # extra defines, e.g. components
TARGET_DEPS   :=                                                # files read into the above, e.g. a config
```

A saved file of `TARGET_DEPS` rebuilds the wasm as a source of the target's does.

```c
// lab_target.c
#include "lab_api.h"

void Lab_Target_Init(void) {          // after the panel and the library are up
	Lab_AddAction("intro", App_PlayIntro);
}
```

A target that runs the scene manager, as an app's UI task does, is **live**:
`Lab_SetPoll(WeUi_Gfx_SceneMgr_Poll)` and its inputs with `Lab_AddInput(name, fn, arg)`.
The lab then plays it like the board - polls, sleeps what the poll asked - while the page
plays, and the inputs become buttons. What was pressed and when is kept, so a rebuild or a
reload replays the session to the same frame; "restart" (R) starts it clean.

- `Lab_AddGroup(name)` puts a heading over the inputs added after it; keys 1-9 press the
  first group's inputs.
- `Lab_AddAction(name, fn)` in a live target is a place to start a session from: fn runs
  before the first poll - put the app in a scene, say - and the session goes on live.
  "power-on" starts it as the board does, with `Lab_Target_Init()` alone.
- `Lab_SetScenes(count, name)` and `Lab_SetInputName(name)` name the scenes and the inputs
  the scene manager traces (`weui_gfx_scenemgr_trace.h`, which the lab hooks itself).

`TARGET_DIR=<dir> npm run dev` runs it. `targets/demos` is the built-in one. A project
keeps its target in its own repository, next to the scenes it builds, and stubs there the
headers its scenes include but the browser has no use for (`platform.h`, board headers).

## Layout

```
c/          lab.c - the wasm side: start, actions, frames out; lab_api.h for targets
targets/    built-in targets
src/        the UI: worker.ts runs the wasm, main.ts the page, panel.ts and charts.ts draw
Makefile    emcc build: the library archived per variant, a target relinks in a second
```
