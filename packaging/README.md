# Packaging

`make-release.ps1` copies a built Release x64 player into `dist/playerP2-x64/` and zips it.

```
powershell -ExecutionPolicy Bypass -File packaging\make-release.ps1
```

Build **Release | x64** first. Check `docs/THIRD_PARTY_NOTICES.md` before sharing the zip: the libmpv licence review is still open.
