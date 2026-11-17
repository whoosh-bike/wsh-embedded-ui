# weui host tests

The library drawn into a simulated panel on the PC, under AddressSanitizer and
UndefinedBehaviorSanitizer. Nothing here is built into firmware: `weui.mk` leaves `tests/` out.

```sh
cd tests
make                    # build and run everything, frames as PNG in out/
make T=push_lands       # one test
make W=32 H=8           # another panel size; DISPLAY=GRAY, LIB=GRAY for the gray paths
make BANDS=1            # scenes out in bands, as to a driver with no frame
make FRAME_MEM=1        # effects on the frames as memory: the same frames, the same reference
make BANDS=1 FRAME_MEM=1   # transitions composed in bands, each frame held to the in-place one
make QRGEN=<path to qr-code-generator/c>   # include the QR widget
make check              # run, then compare frame hashes with the reference
make golden             # record the reference for this host
```

Needs a C compiler with sanitizers (clang or gcc), zlib, and a checkout of
`wsh-embedded-libs` - next to this repository by default, `EMBLIB=<path>` otherwise.

## What runs

- `../host/sim/display_sim.c` implements the `display_m` contract the adapter calls: drawing, display
  and aux frames as separate heap blocks, a brightness LUT on `SetPixel`/fills and none on the
  raw accessors, a frame swap on `DrawFrame` - the behaviour of the BSP drivers, so effects and
  `KeepFrame` scenes run as on the panel. Every access is bounds-checked.
- Time is virtual: it moves only when the library sleeps, so a run takes no wall time and is the
  same on every run. `rand()` is replaced by a fixed generator for the same reason.
- `../host/sim/weui_cfg.h` is the smallest config `weui_cfg_check.h` accepts.

## Two kinds of test

Regression tests assert a property of the output: a crop keeps every pixel in place, a slide
lands on the incoming picture, transparent pixels leave the background as it was.

Golden runs play every demo and fold each pushed frame into one hash per run (`hash ...` lines).
A new test that draws goes below them in the runner's table: above, it moves the frame counter
and the clock the demos are coloured and paced by, and every hash with them.
`make check` diffs them against `golden/<host>-<variant>.txt`, so any change in what reaches the
panel shows up and has to be explained - a fix that changes the picture comes with `make golden`
in the same commit. libm differs in the last bits between hosts, so a reference only holds for
the host it was recorded on.

To look at a change, open the PNGs in `out/`: every 25th frame of each run, upscaled 8x with a
gap between pixels, as the LEDs look.
