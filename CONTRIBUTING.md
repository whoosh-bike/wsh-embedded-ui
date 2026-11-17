# Contributing

## Branches

- `main` - stable; applications pin this.
- `dev` - integration branch for the next release.
- `feature/<name>` - short-lived work, merged into `dev` through a merge request.

Create feature branches from `dev`, not from `main`. A release is `dev` merged into `main`
and tagged.

## Before you open a merge request

1. Build and run the host tests:

   ```sh
   cd tests
   make check
   ```

   A sibling `wsh-embedded-libs` checkout is expected; use `EMBLIB=<path>` otherwise. A
   change in what reaches the panel also needs `make golden` in the same commit, with the
   reason in the commit message.

2. Format the C you touched with `clang-format` (`.clang-format` is in the root).
3. Add or update a demo in `gfx/demo/` for a rendering change, and a test in `tests/` for
   behaviour that can be asserted.
4. Update `docs/` and `README.md` if the change makes them wrong. Do not leave stale
   claims behind.

## Conventions

- C23, tabs, 100 columns, pointer on the left: `int* p`.
- CamelCase functions and variables, `UPPER_CASE` macros, `_t` types.
- Public symbols carry the module prefix: `WeUi_Gfx_*`, `WeUi_Ind_*`.
- No magic numbers, no duplicated code, every error handled.
- English in code, comments and documentation.

## Mirror

The canonical repository is on GitLab. GitHub mirrors `main` and `dev` read-only. Open
issues and merge requests on GitLab.

## License

By contributing you agree that your contribution is licensed under the MIT license in
[LICENSE](LICENSE).
