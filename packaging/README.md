# Packaging

`make-release.ps1` copies a built Release x64 player into `dist/playerP2-x64/` and zips it, for local testing.

```
powershell -ExecutionPolicy Bypass -File packaging\make-release.ps1
```

Build **Release | x64** first.

The zip contains `libmpv-2.dll`, which is under the GPL when it comes from an mpv-winbuild-cmake dev package. Do not publish the zip until its licence obligations are met: see `docs/THIRD_PARTY_NOTICES.md`. No public download is planned for this version.
