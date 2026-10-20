# MP3-MP4 Media Player (playerP2)

A Windows desktop media player for MP3 and MP4 files, written in C++/CLI (Windows Forms, .NET Framework 4.7.2) on top of [libmpv](https://mpv.io/).

Author: Aung Khant Thaw

## Features

- Playlist with Previous / Next, autoplay, shuffle and repeat (Off / All / One)
- Vocal reduction for karaoke (centre-channel cut with FFmpeg's `stereotools` filter)
- Full-screen mode with auto-hiding floating controls and pointer
- Keyboard shortcuts in both normal and full-screen mode: Space, Left/Right, N, P, F, Esc
- Sharp at high DPI (system DPI aware)

## Technology

- C++/CLI, Windows Forms, .NET Framework 4.7.2, Visual Studio 2026 (v143 tools)
- libmpv client API for decoding and rendering; video is drawn into a WinForms panel
- libmpv events are polled on the UI thread and matched by playlist entry id, so old events never skip a track

## Building

Windows 10/11 x64 and Visual Studio 2026 with C++/CLI support are required.

1. Get a 64-bit libmpv dev package (`mpv-dev-x86_64-*.7z`) and copy its `libmpv-2.dll` into `third_party/mpv/bin/`. The DLL is not in Git; see [third_party/mpv/README.md](third_party/mpv/README.md).
2. Open `playerP2.sln` and build **Release | x64**.
3. Run `build/bin/x64/Release/playerP2.exe`.

Details: [docs/BUILDING.md](docs/BUILDING.md).

No ready-to-run download is published yet.

## Documentation

- Design diagrams (draw.io sources and PDF exports): [docs/diagrams](docs/diagrams)
- Project reports (overview, requirements, specifications, testing, user and developer guide): [docs/reports](docs/reports)
- Manual test checklist: [tests/MANUAL_TEST_CHECKLIST.md](tests/MANUAL_TEST_CHECKLIST.md)

## Repository layout

| Folder | Contents |
|---|---|
| `src/playerP2/Source` | C++/CLI source (`.cpp`) |
| `src/playerP2/Include` | Headers, plus the form's `.resx` (kept beside its form) |
| `src/playerP2/Resources` | Application manifest |
| `third_party/mpv` | libmpv headers and import library (runtime DLL added locally) |
| `docs` | Build guide, third-party notices, diagrams and reports |
| `tests` | Manual test checklist |
| `packaging` | Local packaging script (not used for public releases yet) |
| `build` | Build output (not in Git) |

## Licence

Copyright (c) 2026 Aung Khant Thaw. playerP2's own source code and documentation are released under the MIT License: see [LICENSE](LICENSE).

Files in `third_party/` keep their own licences. libmpv itself is not part of this repository; its licence depends on the build you use. See [docs/THIRD_PARTY_NOTICES.md](docs/THIRD_PARTY_NOTICES.md), especially before sharing any built binaries.
