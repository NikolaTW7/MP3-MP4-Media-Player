# libmpv

Version: `mpv-dev-x86_64-20261018-git-01e2533f31` (libmpv API 2, x64), a 64-bit libmpv dev package from an mpv-winbuild-cmake build. Windows builds of mpv and libmpv are listed on https://mpv.io/installation/.

| Path | From the dev package | In Git |
|---|---|---|
| `include/mpv/*.h` | `include/mpv/` | Yes (ISC licence) |
| `lib/libmpv.dll.a` | `libmpv.dll.a` | Yes |
| `bin/libmpv-2.dll` | `libmpv-2.dll` | No (121 MB) |

## Setup

Copy `libmpv-2.dll` from the dev package into `bin/`. The build copies it next to `playerP2.exe`.

Use the DLL from the same package as the headers and import library. A newer `mpv-dev-x86_64-*` package with client API 2.x should also work; replace all three parts together.

See `docs/THIRD_PARTY_NOTICES.md` for licensing.
