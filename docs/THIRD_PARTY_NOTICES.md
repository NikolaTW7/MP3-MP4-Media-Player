# Third-party notices

playerP2's own code and documentation are Copyright (c) 2026 Aung Khant Thaw and licensed under the MIT License (see `LICENSE`). The MIT License does not apply to the files listed below.

## libmpv client headers (`third_party/mpv/include/mpv/*.h`)

- Licence: ISC. The full notice is at the top of each header and must stay there.
- Copyright (C) the mpv developers.

## libmpv import library (`third_party/mpv/lib/libmpv.dll.a`)

- From the libmpv dev package `mpv-dev-x86_64-20261018-git-01e2533f31` (an mpv-winbuild-cmake build), the same package as the headers.
- Only used to link `playerP2.exe` against `libmpv-2.dll`. It contains no libmpv code.

## libmpv runtime (`libmpv-2.dll`) - not in this repository

The DLL is not stored in Git and is not distributed by this project. You supply it yourself when building (see `docs/BUILDING.md`).

Its licence depends on the build:

- **mpv-winbuild-cmake dev packages** (used during development) statically link FFmpeg and many other libraries, including GPL ones (x264, x265, Rubber Band, libdvdnav) and Apache-2.0 OpenSSL. The DLL as a whole is therefore under the **GPL** (most likely GPLv3).
- **LGPL-only builds** (for example peschuster/libmpv-build) are under the **LGPL 2.1 or later**. One was evaluated with this player; it needs project changes (`mpv.lib`, `mpv-2.dll`) and showed a full-screen flicker issue, so it is not used in this version.

### If you share a built player

The MIT License allows playerP2's code to be combined with either build. A binary package that includes a libmpv DLL must also meet that DLL's licence, for example:

- GPL build: distribute the whole package under GPL-compatible terms, include the GPL text and these notices, and provide the corresponding source of the exact libmpv build (or a written offer).
- LGPL build: include the LGPL text and these notices, keep the DLL replaceable, and provide the corresponding source of the exact libmpv build.

No binary packages are published by this project at the moment.

This is a technical summary, not legal advice.

## .NET Framework 4.7.2

Part of Windows. Not redistributed with playerP2.
