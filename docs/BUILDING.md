# Building playerP2

## Requirements

- Windows 10 or 11, x64
- Visual Studio 2026 with:
  - Desktop development with C++
  - C++/CLI support for the v143 build tools
  - .NET Framework 4.7.2 targeting pack

## One-time setup

1. Clone or copy the repository to any folder. No paths are tied to a particular computer.
2. Copy `libmpv-2.dll` into `third_party/mpv/bin/`.
   It comes from the same mpv dev package as the headers (see `third_party/mpv/README.md`).
   It is not in Git because it is larger than GitHub's 100 MB file limit.

## Build

1. Open `playerP2.sln`.
2. Choose **Release** and **x64**.
3. **Build > Rebuild Solution**.

Output: `build/bin/x64/Release/playerP2.exe`, with `libmpv-2.dll` copied beside it automatically.
Intermediate files go to `build/obj/`.

If `libmpv-2.dll` is missing, the build stops with a message pointing to `third_party/mpv/README.md`.

The linker warning `LNK4248: unresolved typeref token ... for 'mpv_handle'` is expected. `mpv_handle` is an opaque libmpv type and is only used through a pointer.

## Using a different libmpv folder

Pass `MpvDir` (with a trailing backslash):

```
msbuild playerP2.sln /p:Configuration=Release /p:Platform=x64 /p:MpvDir=<path-to-mpv>\
```

The folder needs `include\mpv\`, `lib\libmpv.dll.a` and `bin\libmpv-2.dll`.

## Notes

- Only x64 is supported. The Win32 configurations remain in the project but there is no 32-bit libmpv.
- If you open `MpvPlayerForm.h` in the Windows Forms Designer, avoid saving layout changes from it: sizes in the code are written for 100% scaling.
