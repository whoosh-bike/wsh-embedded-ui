# Roadmap

Direction, not a promise. Features are added when a real product needs them; see
[AGENTS.md](../AGENTS.md).

## Available now

- Drawing: points, lines, rectangles, circles, hatching, frame fade.
- Text: u8g2 rendering, rotation, wrapped and aligned text block.
- Images and animation: RAM/internal/external flash, LZ4, dithering.
- Colour: RGB or gray, optional alpha channel.
- Effects: 19 transitions, plasma/perlin/static painters, solo animations.
- Scene manager: scene table, message ring, FPS pacing, timeouts, overlay, transitions.
- Config validation (`weui_cfg_check.h`), host tests and the browser lab.
- LED-strip indication (`ind/`) as a separate subsystem.

## Candidates

- Clipping regions, alpha blending, rounded rectangles, arcs, gradient fills, anti-aliased
  lines.
- A widget layer: common interface, layout, hit-testing, progress/button/icon/list.
- Transitions that do not block the host loop.
- Reentrant text rendering and string formatting helpers.
- Asset pipeline: resize, better compression, font subsetting.
- A widget development guide and a reference board project.
- Optional ambilight hook between `gfx/` and `ind/`.

## Known limitations

- One effect at a time; a transition blocks `WeUi_Gfx_SceneMgr_Poll()` for its duration.
- Text state is global and non-reentrant.
- Widgets are free functions with a style struct, not a common base type.
- The golden frame hashes are host-specific; record them on the machine that runs the tests.
- `external/fonts/u8g2_fonts.c` carries the full u8g2 font set, with per-font licenses; trim
  it if a product needs a smaller or single-license distribution.
