# v0.76 – European edition, public test build

Need for Speed: Carbon Collector's Edition (Xbox 360), recompiled for PC: the European edition.
This fork of [mkalkhoven-po/nfs-carbon-ce-recomp](https://github.com/mkalkhoven-po/nfs-carbon-ce-recomp)
is built on its v0.76. Requires your own disc image or extracted disc of the **European
Collector's Edition** (title ID 454107EC, `default.xex` version 0.0.0.11); see the README. For
the USA edition (0.0.0.7), use the original project's v0.76.

## New in this fork

- **The European Collector's Edition.** The game's own language screen at startup, with the
  twelve languages on the disc (English, French, German, Italian, Spanish, Dutch, Swedish,
  Danish, Hungarian, Czech, Polish, Finnish). Your choice is remembered; without a choice the
  game goes on after 10 seconds. The disc has speech in English only: voices and cutscene audio
  stay English, with the text in your language.
- **Faster.** 150–157 fps in menus, races and at the start with eight cars, where it was 80–120
  (Remix, RTX 4080, 165 Hz monitor; 157 is the NVIDIA driver's own cap with low-latency G-SYNC).
  Frames no longer wait for a whole screen refresh (`gpu.vsync` is off in both profiles), and the
  GPU thread does less work per frame. `--gpu.vsync=true` gives the old behaviour.
- **RecompLauncher.** The release has a `recomplauncher.json`: RecompLauncher checks your disc,
  starts the game with it and keeps the savegames and shader cache in the game's `recompdata`
  folder.

## Also in v0.76 (from the original project)

- The game runs on version 0.11 of the ReXGlue SDK, with many fixes in the recompiler and the
  emulated Xbox 360 system.
- Setting names have a scope, such as `nfsc.profile` or `ui.fullscreen` (see the README). Older
  `nfsc.toml` files and command lines keep working.
- Remix no longer sharpens the picture with AMD CAS: SDK 0.11 doesn't include it.
- `F4` opens the SDK's own settings window and `` ` `` a console, both for advanced use. Saving in
  the `F4` window rewrites `nfsc.toml` completely (comments are lost); PC SETTINGS (`F1`) only
  changes what you change.

## Not tested yet

- The European edition has been tested less than the USA one, and the languages other than
  English only in Italian.
- The rest of the Career story (territories, crew, canyon duels), split screen.
- AMD and Intel GPUs, a monitor without variable refresh rate, and Direct3D 12 beyond the main
  menu.

## Reporting problems

Open an issue in [this repository](https://github.com/Cioscos/nfs-carbon-ce-recomp/issues)
with what happened, your GPU, your monitor's refresh rate, the profile and settings, and the
log file (and dump file, after a crash) from the `logs` folder next to `nfsc.exe`. Nothing is
sent anywhere automatically.
