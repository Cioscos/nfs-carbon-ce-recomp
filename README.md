# Need for Speed: Carbon Collector's Edition – Recompiled (European edition)

A native PC port of **Need for Speed: Carbon Collector's Edition for the Xbox 360**, made by
static recompilation with the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk). The
game's PowerPC code is translated to C++ ahead of time and compiled for x86-64; graphics run
on Vulkan (or Direct3D 12).

This is a fork of [mkalkhoven-po/nfs-carbon-ce-recomp](https://github.com/mkalkhoven-po/nfs-carbon-ce-recomp),
the original project by Martin Kalkhoven. The fork adds the **European Collector's Edition**,
with its language screen and the twelve languages on the disc, and makes the game run faster
on SDK v0.11. Its releases are for the European edition; for the USA edition, use the releases of
the original project. See [Differences from the original project](#differences-from-the-original-project).

**Version 0.761 – public test build.** The game is playable from start to finish as far as we
know, but only a part of the Career has been tested. Please report what you find (see
[Reporting problems](#reporting-problems)).

> **No game files are included.** You need your own copy of the game: a disc image (`.iso`) of
> the **Collector's Edition** (title ID `454107EC`) or the files extracted from it. The code
> supports two editions: the USA one (`default.xex` version 0.0.0.7) and the European one
> (version 0.0.0.11). Each `nfsc.exe` runs only the edition it was built for. The releases of
> this fork are for the **European edition, 0.0.0.11**; for 0.0.0.7, download the
> [original project's release](https://github.com/mkalkhoven-po/nfs-carbon-ce-recomp/releases)
> or build from source (see [Building from source](#building-from-source)). Other editions
> (the regular release) have a different executable and will not run; the game checks this at
> startup.

## Differences from the original project

What this fork changes, compared with the original project's v0.76:

| | Original project | This fork |
|---|---|---|
| Release download | USA Collector's Edition (0.0.0.7) | European Collector's Edition (0.0.0.11) |
| Editions in the code | USA only | USA and Europe: the build reads the version of `game/default.xex` and uses that edition's addresses and function list (`-DNFSC_GAME_VERSION=v7` or `v11` to insist on one) |
| Languages | English | European edition: the game's own language screen at startup and the twelve languages on the disc. The disc has speech in English only, so voices and cutscene audio stay English with every language (before this fix, cutscenes in other languages had no sound track) |
| Frame rate (Remix, 165 Hz monitor) | 80–120 fps in races, GPU at 40% | 150–157 fps in menus, races and the eight-car start (157 is the NVIDIA driver's own cap with low-latency G-SYNC) |
| `gpu.vsync` | On: every frame waits for a whole screen refresh, so a frame slightly too slow for one refresh waits for two | Off in both profiles; the frame rate limit is kept by the game itself |
| SDK patches (`patches/rexglue-sdk/`) | Widescreen movies and loading screens | Also: vblanks as soon as the game waits for one, GPU registers written as ranges, "hot pages" for textures uploaded every frame. One subject per patch, and the SDK setup stops when a patch doesn't apply instead of skipping it |
| RecompLauncher | – | `recomplauncher.json` in the release: RecompLauncher checks your disc, sets the game up and starts it (see [With RecompLauncher](#with-recomplauncher)) |
| Switching to another window (Alt+Tab) | The pause is the wrong way round: the game goes on in the background, and pauses and ignores the controller when you come back | Fixed in v0.761: the game pauses when you leave its window and goes on when you come back |

Our frame rates were measured on a Ryzen 7 7800X3D with an RTX 4080 at 2560×1440, 165 Hz. The
measurements and the causes are in [ROADMAP.md](ROADMAP.md) (in Dutch). Two of the SDK patches
have also been offered to the ReXGlue SDK itself: the register ranges
([#618](https://github.com/rexglue/rexglue-sdk/pull/618)) and the disc image as game root
([#620](https://github.com/rexglue/rexglue-sdk/pull/620)).

This project is not affiliated with or endorsed by Electronic Arts, Microsoft or Xbox. All
trademarks belong to their respective owners. It does not promote piracy: use a dump of a disc
you own.

## Features

- Runs natively, no emulator: 30 fps like the Xbox 360, or 60/120/240 fps with the race clock,
  physics and AI still in real time.
- Two graphics profiles:
  - **Original** (default): 1280×720, the console's 30 fps, accurate EDRAM emulation,
    nothing added. Meant to look exactly like the Xbox 360.
  - **Remix**: internal resolution and frame rate that follow your monitor (1440p on 1080p
    and 1440p screens, 4K on 4K screens; up to 165 fps) and 16× anisotropic filtering.
- Widescreen and ultrawide, by default matching your monitor (21:9, 32:9 or any ratio wider
  than 16:9): more view to the sides, HUD at the screen edges, videos and loading screens kept
  at 16:9.
- Controller and keyboard at the same time.
- "PC SETTINGS" and "EXIT TO DESKTOP" in the main menu. The online options are gone, because
  they only say that Xbox Live can't be reached.
- A race pauses when you open PC SETTINGS or switch to another window, and goes on when you
  come back.
- With Remix the game skips the logos and intro videos and goes straight to the main menu.
- European edition (0.0.0.11): the game's own language screen at startup, with the twelve
  languages on the disc (English, French, German, Italian, Spanish, Dutch, Swedish, Danish,
  Hungarian, Czech, Polish, Finnish). The original executable is English only. Your choice
  is remembered; without a choice the game goes on after 10 seconds. The videos stay English.

## Requirements

- Windows 10/11, 64-bit.
- A GPU with Vulkan 1.3 (or Direct3D 12). Tested on an NVIDIA RTX 5080; Original needs
  little, Remix at 120 fps wants a recent mid-range card or better.
- The [Microsoft Visual C++ Redistributable 2015–2022 (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe).
- Your Collector's Edition disc image (about 7.3 GB).
- A folder you can write to: not `Program Files`, because settings and logs go next to
  `nfsc.exe`.

## Installing and starting

### With RecompLauncher

[RecompLauncher](https://recomplauncher.com) installs and updates the game from this
repository's releases. Install it there, then drop in your disc image (or pick the folder with
`default.xex`): the launcher checks that it is the Collector's Edition (title ID `454107EC`)
and Play starts the game with it. No file dialog appears.

Started from RecompLauncher, the game keeps its savegames and shader cache in the `recompdata`
folder inside its install folder (not in `Documents\nfsc`), so the launcher can back them up.
Settings (`nfsc.toml`) and logs stay next to `nfsc.exe`. To keep savegames from an earlier
installation, copy the contents of `Documents\nfsc` (or of `userdata` in portable mode) into
`recompdata`. Settings are changed in the game, under PC SETTINGS.

The launcher's check can't tell the European disc from the USA one (they have the same title
ID). With the USA disc the game stops at startup and says so.

### By hand

1. Download the release zip (`...-Europe-Windows-x64.zip`) and extract it anywhere.
2. Start `nfsc.exe`. Choose the Remix profile and other options later under PC SETTINGS
   in the main menu.
3. On the first start a file dialog asks for the game. Choose either
   - the disc image (`.iso`), or
   - `default.xex` in a folder with the extracted disc.

   The choice is remembered (in `Documents\nfsc\game_path.txt`; delete that file to choose
   again). Instead of using the dialog you can also put the `.iso` or a folder named `game`
   next to `nfsc.exe`, or start with `--game_data_root="path to the iso or folder"`.

"Cannot connect to Xbox Live!" at startup is expected. Savegames are stored in
`Documents\nfsc`, the shader cache in `%LOCALAPPDATA%\nfsc\cache`.

**Portable mode:** choose *Save location* > *Next to nfsc.exe* in PC SETTINGS (Advanced), or put
`portable = true` in `nfsc.toml`. Savegames and shader cache then live in a `userdata` folder
next to `nfsc.exe`, so the whole folder can move to another PC; with a `game` folder or `.iso`
next to it, you don't have to choose anything there. The first time, your savegames and cache
are copied there; the originals stay where they are and are not updated any more.

## Controls

An Xbox controller and the keyboard work at the same time.

| Keyboard | |
|---|---|
| `W` `A` `S` `D` | Steer / navigate |
| `E` / `Q` | Throttle / brake |
| `Space` / `Backspace` | A / B (accept / back) |
| `L` / `P` | X / Y |
| `Return` / `Tab` | Start / Back |
| `F11` | Fullscreen ↔ window |
| `F1` | PC settings (also in the main menu) |
| `F4` | All settings, for advanced use |
| `F3` | Performance overlay |
| `` ` `` | Console, for advanced use |
| `Alt+F4` | Quit (or EXIT TO DESKTOP in the main menu) |

## Settings

The main ones are in **PC SETTINGS** (main menu or `F1`): graphics profile, frame rate limit,
aspect ratio, fullscreen or window, the intro videos, keyboard controls, and under Advanced the
resolution, texture filtering, graphics API and save location. Use a controller (A select, B back), the keyboard (WASD or arrows,
`Space`, `Backspace`) or the mouse. "Profile: ..." means the graphics profile's value.
**Apply** saves to `nfsc.toml` next to `nfsc.exe`; settings marked with `*` restart the game.

Every setting can also be set in the advanced settings window (`F4`), in `nfsc.toml`, or on the
command line (`--name=value`). Settings you change yourself always win over the profile.
Setting names have a scope, such as `nfsc.profile` or `gpu.vsync`; the names of earlier
versions without a scope (`profile`, `fullscreen`, ...) still work.

| Setting | Values | |
|---|---|---|
| `nfsc.profile` | `original`, `remix` | Graphics profile (restart) |
| `nfsc.aspect_ratio` | `auto`, `16:9`, `21:9`, `32:9`, `W:H` | Render aspect ratio (restart) |
| `nfsc.frame_rate_limit` | `monitor`, `30`, `60`, `120`, `240` | Frame rate limit (restart) |
| `gpu.resolution_scale_x` / `_y` | `1`, `2`, `3` | Internal resolution: 720p, 1440p, 4K (restart) |
| `ui.fullscreen` | `true`, `false` | |
| `ui.monitor` | `1`, `2`, ... | Monitor to open on; `0` is the main one |
| `gpu.driver` | `vulkan`, `d3d12` | Graphics API (restart) |
| `gpu.vsync` | `false`, `true` | `true` makes the game wait for a whole screen refresh at every frame: no skipped refreshes while the PC keeps up, but a frame that is slightly late waits for the next one (82 instead of 120 fps on a 165 Hz monitor) |
| `nfsc.skip_intro` | `true`, `false` | Skip logos and videos up to the main menu; also the language screen once a language is chosen |
| `nfsc.language` | `english`, `french`, `german`, `italian`, `spanish`, `dutch`, `swedish`, `danish`, `hungarian`, `czech`, `polish`, `finnish` | European edition: the language chosen on the language screen, saved by the game (restart). The disc has speech in English only: voices and cutscene audio stay English |
| `nfsc.pause_in_background` | `true`, `false` | Pause a race and ignore the controller while in the background |
| `nfsc.portable` | `true`, `false` | Savegames and shader cache next to `nfsc.exe` (restart) |
| `app.cache_root` | path | Shader cache folder (restart) |

Example `nfsc.toml` for an ultrawide monitor:

```toml
[nfsc]
profile = "remix"
aspect_ratio = "21:9"
```

## Known issues

- Only tested on NVIDIA (RTX 5080, Windows 11). AMD and Intel GPUs have not been tested.
- Career: tested up to the first races, the Career hub, the world map, the safehouse and free
  roam; the rest of the story (territories, crew, canyon duels) has not been played through.
- Split screen has not been tested (needs a second controller).
- The Vulkan "immediate" present mode shows a black screen on NVIDIA; the profiles turn it
  off.
- Smart App Control can block the executable; it is not signed.
- The European Collector's Edition (0.0.0.11) has all features of the USA one, but has only
  been tested up to the main menu (intro skip, widescreen, PC SETTINGS, EXIT TO DESKTOP).
  The languages other than English have only been tested up to the main menu, in Italian.

## Reporting problems

Open an issue [in this repository](https://github.com/Cioscos/nfs-carbon-ce-recomp/issues)
(problems with the USA release go to the original project) with what you did, what happened,
your GPU, your monitor's refresh rate and the profile and settings you used. The log files are in the `logs` folder next to
`nfsc.exe`; attach the one from the session where it happened. After a crash the game also
writes a dump file (`.dmp`) there and tells you where it is; attach that too.

**Privacy:** nothing is ever sent anywhere. The game only writes log files on your own PC;
you decide whether to share one. `--nfsc.log_fps=true` adds the frame rate to the log once a
second.

## Building from source

The repository contains only our own code: the project (`nfsc/`), the hooks for widescreen,
frame rate and the menu option (`nfsc/src/`), and our changes to the ReXGlue SDK
(`patches/rexglue-sdk/`). The recompiled game code is generated from your own `default.xex`
during the build.

Toolchain (Windows): LLVM/Clang 23, Visual Studio 2022 Build Tools (*Desktop development
with C++*), CMake 4, Ninja, Git, Python 3.12 and the LunarG Vulkan SDK 1.4.313 or newer, with
its *Vulkan Memory Allocator header* component. Put `C:\Program Files\LLVM\bin` first in
`PATH`. The project uses ReXGlue SDK 0.11.0.

The build is for the edition of the `default.xex` in `game/`: CMake reads its version
(`tools/xexid.py`) and uses the matching addresses and function list. To check that you
build for a particular edition, add `-DNFSC_GAME_VERSION=v7` or `v11` to `cmake --preset`;
the configure step stops if `game/default.xex` is another one. The build only needs
`default.xex`; the game reads the rest from the disc image at startup.

```bash
# 1. Extract the disc (or only default.xex: add default.xex at the end)
python tools/xdvdfs.py extract "path/to/your/disc.iso" game

# 2. Get, patch and build the SDK (once)
bash tools/setup-sdk.sh
cd sdk/rexglue-sdk
cmake --preset win-amd64 -DREXGLUE_BUILD_TESTS=OFF
cmake --build out/build/win-amd64 --config Release --target install

# 3. Build the game. The recompilation runs as part of the build; the first build stops
#    after it, so run both commands once more.
cd ../../nfsc
cmake --preset win-amd64
cmake --build --preset win-amd64-release
```

`python tools/package-release.py` puts a build together as a release folder and zip, named
for the edition it was built for (`NFS-Carbon-CE-Recompiled-v0.761-Europe-Windows-x64.zip`). The
European one also gets `recomplauncher.json`, which RecompLauncher reads from the top of the zip.

Development notes (in Dutch): [docs/DEVELOPMENT.nl.md](docs/DEVELOPMENT.nl.md) and
[ROADMAP.md](ROADMAP.md).

## License

Our code is licensed under the BSD 3-Clause License (see [LICENSE](LICENSE)), like the ReXGlue
SDK. The original project is by Martin Kalkhoven; this fork keeps its license and copyright
notice. Release builds include third-party libraries under their own licenses (see the
`licenses` folder in the release). The game itself and everything generated from it remain
the property of Electronic Arts.
