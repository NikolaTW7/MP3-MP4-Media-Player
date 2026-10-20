# Third-party components

## libmpv client headers (`third_party/mpv/include/mpv/*.h`)

- Licence: ISC. The full notice is at the top of each header and must stay there.
- Copyright (C) 2017-2018 the mpv developers.
- Safe to keep in a private or public repository.

## libmpv runtime (`libmpv-2.dll`) and import library (`libmpv.dll.a`)

- Source: mpv dev package `mpv-dev-x86_64-20261018-git-01e2533f31` (a mpv-winbuild-cmake build).
- The package contains no licence files or source.
- The DLL is not stored in Git.

### What the DLL contains

The DLL statically links FFmpeg and many other libraries. Text found inside the DLL shows, among others:

| Licence | Libraries |
|---|---|
| GPL | x264, x265, Rubber Band, libdvdnav |
| LGPL | FFmpeg core, libbluray, VapourSynth, FriBidi |
| Apache-2.0 | OpenSSL, shaderc, Vulkan loader |
| Permissive (MIT, BSD, ISC, zlib and similar) | libass, dav1d, libplacebo, LuaJIT, zimg, libvpx, libaom, SVT-AV1, Opus, Vorbis, libopenmpt, FreeType, HarfBuzz, libpng, libjpeg, lcms2, libxml2, libarchive, libjxl, soxr, SRT, libssh |

This list comes from inspecting the binary, not from the build recipe, so treat it as indicative.

### Consequences

- **The combined DLL is GPL.** Because it also includes Apache-2.0 OpenSSL, the FFmpeg part is most likely built as GPLv3.
- **Private repository:** no obligations are triggered. Nothing is distributed, and the DLL is not committed.
- **Before sharing binaries** (a zip, a release or a public repository with downloads), a licence review is still needed. It is likely to require:
  - a GPL-compatible licence for playerP2's own source (probably GPLv3);
  - the GPL text and these notices in every release;
  - the corresponding source, or a written offer, for the exact mpv build and its libraries.
- **Alternative:** use an LGPL-only libmpv build (no x264, x265, Rubber Band or libdvdnav). playerP2 does not need those for playback.

This is a technical summary, not legal advice.

## .NET Framework 4.7.2

Part of Windows. It is not redistributed with playerP2.
