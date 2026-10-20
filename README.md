# playerP2

A Windows MP3/MP4 player written in C++/CLI (Windows Forms, .NET Framework 4.7.2) on top of [libmpv](https://mpv.io/).

## Features

- Playlist with Previous / Next, autoplay, shuffle and repeat (Off / All / One)
- Vocal reduction for karaoke (centre-channel cut)
- Full-screen mode with an auto-hiding control bar
- Keyboard shortcuts: Space, Left/Right, N, P, F, Esc
- Sharp at high DPI (system DPI aware)

## Building

Windows 10/11 x64 and Visual Studio 2026 with C++/CLI support are required.

1. Copy `libmpv-2.dll` into `third_party/mpv/bin/` (it is not in Git; see [third_party/mpv/README.md](third_party/mpv/README.md)).
2. Open `playerP2.sln` and build **Release | x64**.
3. Run `build/bin/x64/Release/playerP2.exe`.

Details: [docs/BUILDING.md](docs/BUILDING.md).

## Repository layout

| Folder | Contents |
|---|---|
| `src/playerP2/Source` | C++/CLI source (`.cpp`) |
| `src/playerP2/Include` | Headers, plus the form's `.resx` (kept beside its form) |
| `src/playerP2/Resources` | Application manifest |
| `third_party/mpv` | libmpv headers and import library (runtime DLL added locally) |
| `docs` | Build guide and third-party notices |
| `docs/diagrams` | Architecture and UML diagrams (`.drawio` sources with PDF exports) |
| `docs/reports` | Project reports (PDF) |
| `tests` | Manual test checklist |
| `packaging` | Release packaging script |
| `build` | Build output (not in Git) |

## Licence

No licence has been chosen yet, so all rights are reserved. The libmpv build used here includes GPL components: read [docs/THIRD_PARTY_NOTICES.md](docs/THIRD_PARTY_NOTICES.md) before sharing any binaries.
