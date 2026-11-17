# AGENTS.md

Guidance for people and coding agents working on weui.

## What this is

weui is a small C graphics library for low-resolution panels on microcontrollers: LED
matrices, small OLED and e-ink displays. It draws primitives, u8g2 text, images and
animations, runs whole-frame effects and scene transitions, and provides a poll-driven
scene manager. `ind/` adds LED-strip indication.

The library is display- and RTOS-agnostic. The panel is behind the `display_m` contract;
time, delays and critical sections come from the host's `weui_cfg.h`.

## Stack

- C23 (`-std=gnu23`) for firmware; the host tests use `-std=gnu2x`
- arm-none-eabi-gcc 15+ for firmware, Clang or GCC with sanitizers for the tests
- GNU Make
- Python 3 and Node.js 20+ for the asset converter and the browser lab

## Development philosophy

`docs/ROADMAP.md` shows the direction, but a feature is added only when a real product
needs it. Before implementing a roadmap item, ask whether there is a concrete use case
today.

## Architectural principles

- Keep solutions as simple as they can be (KISS, YAGNI).
- Avoid duplication and unnecessary abstraction (DRY).
- Prefer an existing solution before writing a new one.
- Treat documentation as part of the codebase.
- Evaluate the impact of a change before implementing it.
- Keep hardware and display abstraction: avoid display-specific features unless they are
  the only reasonable option. Prefer simple, predictable behaviour on low-resolution
  panels.
- A change in graphics rendering comes with a demo or a test that shows it.
- Keep the diagrams and module map in `docs/ARCHITECTURE.md` current.

## Code quality

Code must be clean and warning-free:

- No magic numbers; use constants and configuration.
- No duplicated code.
- Handle every error.
- Use meaningful names.
- Prefer early returns to deep nesting.
- New behaviour gets a working example in `gfx/demo/` (listed in `weui_demo.h`) or a test
  in `tests/`.

## Conventions

- CamelCase for functions and variables.
- UPPER_CASE for macros and constants.
- Module prefix for public symbols (`WeUi_Gfx_*`, `WeUi_Ind_*`).
- `_t` suffix for types.
- Public headers include by basename; `weui.mk` puts every library folder on the include
  path.

## Formatting

- LLVM style, see `.clang-format`.
- Tabs, width 4; line length at most 100.
- Pointer alignment: `int* p`.

## Tests

```sh
cd tests
make check          # run and compare frame hashes with the reference
make                # build and run, write PNG frames to out/
make T=push_lands   # one test
```

The tests need a sibling checkout of `wsh-embedded-libs` (`EMBLIB=<path>` if it is
elsewhere), zlib and a compiler with ASan/UBSan. `make QRGEN=<path>` adds the QR test.

A change that alters what reaches the panel re-records the golden hashes with `make golden`
in the same commit, and the commit message says why.

## Documentation

- `README.md` - what the library is, how to integrate it.
- `docs/ARCHITECTURE.md` - layers, render loop, scene manager, driver contract, config.
- `docs/ROADMAP.md` - what is done and what is planned.
- `gfx/effects/EFFECTS.md` - the effect catalogue.
- `tests/README.md`, `tools/lab/README.md` - the test suite and the browser lab.

Keep documentation in English. Update it with the code; remove stale claims instead of
leaving them for later.

## Branches

- `main` (or `master` in the legacy internal repository) - stable.
- `dev` - integration branch.
- `feature/*` - short-lived work, merged into `dev` through a merge request.

The public GitHub repository is a read-only mirror of the public GitLab repository's `main`
and `dev`. Issues and merge requests belong on GitLab.
