# Need for Speed Carbon – Xbox 360 recompilation

Static recompilation of NFS Carbon Collector's Edition (Xbox 360, title ID `454107EC`) with the
[ReXGlue SDK](https://github.com/rexglue/rexglue-sdk). Two editions: US (`default.xex`
v0.0.0.7) and Europe (v0.0.0.11). Each build is for one edition: that of `game/default.xex`.

> Game files (the ISO, `game/`) and everything generated from them
> (`nfsc/generated/default/`) are protected by copyright: never commit or share them.

## Folders

| Folder | Contents |
|---|---|
| `*.iso` | Your own dump of the disc (XGD2) |
| `game/` | The completely extracted disc: `default.xex`, `NFS/ZZDATA*.BIN`, `Movies/` |
| `tools/` | `xdvdfs.py` (extract the disc), `xexinfo.py` (dump the XEX header), `xexid.py` (title, version and digest of the first page, for CMake), `setup-sdk.sh`, `apply-sdk-patches.sh` |
| `tools/re/` | Reverse engineering: `strings.py`, `xrefs.py` (lis/addi references), `callers.py` (who calls a function), `disasm.py` (disassembly from the generated code). They work on a dump of the loaded image: start with `--nfsc.dump_image=image.bin` |
| `sdk/rexglue-sdk/` | ReXGlue SDK v0.11.0 as a git submodule (upstream, tag `v0.11.0`, commit `a3e1d82`) |
| `patches/rexglue-sdk/` | Our changes to the SDK, applied by `setup-sdk.sh` (through `apply-sdk-patches.sh`) |
| `nfsc/` | The recompilation project |
| `nfsc/cmake/editions.cmake` | The known editions: version, digest and name |
| `nfsc/cmake/detect_edition.cmake` | Reads `game/default.xex`, picks the edition (`NFSC_GAME_VERSION`: `auto`, `v7`, `v11`) and makes `nfsc_edition.h` |
| `nfsc/cmake/edition_select.cmake`, `select_functions.cmake` | The choice of the edition (shared), and the build step that copies the `functions.toml` of the current `default.xex` before codegen: otherwise, after another `default.xex`, codegen first runs with that of the previous edition |
| `nfsc/config/<version>/functions.toml` | Manual function boundaries per edition; CMake copies the chosen one to `config/selected/` |
| `nfsc/src/addresses_<version>.h` | Addresses of the hooks per edition, and a flag per feature (`NFSC_FEATURE_*`): with 0 the hooks stay out of the build |

## Toolchain (Windows)

- LLVM/Clang 23 (`winget install LLVM.LLVM`)
- VS 2022 Build Tools with the workload *Desktop development with C++* (MSVC 14.44, Windows SDK 10.0.26100)
- CMake 4.x, Ninja, Git, Python 3.12
- LunarG Vulkan SDK 1.4.313 or newer (`winget install KhronosGroup.VulkanSDK`, asks for admin),
  with the component *Vulkan Memory Allocator header*: SDK v0.11 takes the Vulkan headers and
  VMA from the Vulkan SDK

Put `C:\Program Files\LLVM\bin` at the front of `PATH` before you build.

## Building from scratch

```bash
# 1. Extract the disc
python tools/xdvdfs.py extract "Need for Speed - Carbon - Collector's Edition.iso" game

# 2. Fetch, patch and build the SDK (once). setup-sdk.sh fetches the submodules,
#    replaces git symlinks (Windows checks those out as text files) and applies the patches.
bash tools/setup-sdk.sh
cd sdk/rexglue-sdk
cmake --preset win-amd64 -DREXGLUE_BUILD_TESTS=OFF   # Ninja Multi-Config
cmake --build out/build/win-amd64 --config Release --target install

# 3. Build the project (codegen runs along automatically). The first build stops after
#    codegen, because the generated sources only take part from the next configure: both again.
cd ../../nfsc
cmake --preset win-amd64
cmake --build --preset win-amd64-release
```

## Adding a game version

1. A line in `nfsc/cmake/editions.cmake` (version and digest from `python tools/xexid.py
   game/default.xex`, name) and the version in `NFSC_EDITIONS`.
2. `nfsc/src/addresses_<version>.h` with all `NFSC_FEATURE_*` at 0, and an `#elif` in
   `addresses.h`. With that the game already builds and starts, without our hooks.
3. `nfsc/config/<version>/functions.toml`, empty at first. If the game doesn't get to the main
   menu, or codegen reports jumps outside a function, the boundaries go here (see the v7).
4. The hooks one by one: find the functions in a dump (`--nfsc.dump_image`) with `tools/re/`,
   starting from what the hook files say about the v7 (strings, constants, imports). Address in
   the header, flag at 1, build, and check with the log line of that hook. If a feature doesn't
   work out, leave the flag at 0 and put it in README and ROADMAP.
   WIDESCREEN needs PRESENT and PAD needs MAIN_MENU; `addresses.h` stops the build otherwise.

For the v0.0.0.11 most functions were at a fixed distance from the v7 (+0x280 for
widescreen, +0x218 for the menu); the globals were the same.

## Local changes to the SDK

They are patches in `patches/rexglue-sdk/` (the submodule points to unchanged upstream).
Ported to SDK v0.11.0 on 17 Nov; of the ten patches for v0.10, three were left. On
22 Nov 0004–0007 were added (performance, see ROADMAP). Each patch holds one subject, so that
a patch that upstream has adopted only affects itself with a new SDK version.

`tools/apply-sdk-patches.sh` (called by `setup-sdk.sh`) first applies the series to a
temporary index of the SDK commit, and only touches the files when the whole series fits:

- a patch that the SDK already has (it reverses cleanly) reports `already in the SDK` and is
  skipped: remove it then;
- a patch that doesn't apply and doesn't reverse either stops the script with an error: update
  it for the new SDK version or take out the part that upstream already has;
- if the patches are already in the working tree, it reports `patches already applied`; other
  local changes in the SDK stop the script, so that it doesn't patch over them.

Tested in `tools/tests/test_apply_sdk_patches.py`. The patches:

- `0001-filesystem-disc-image-game-root.patch` (`file_system`, `title_app`): `title.game_root`
  may also be a disc image (`.iso`); that is then mounted as `kDiscImage` instead of
  as a folder. Successor of v0.10 patch 0009. Proposed upstream: rexglue/rexglue-sdk#620.
- `0002-vulkan-hoist-uploads-no-draw-in-transfer-pass.patch` (Vulkan command processor,
  deferred command buffer, shared memory): the game writes vertex data right before each draw;
  the backend uploads it per draw and ends the render pass for that each time. Now uploads
  during a render pass go to a spot before that render pass, as long as no draw in that pass
  has already read the pages (otherwise the old route). Can be turned off with
  `gpu.vulkan.hoist_shared_memory_uploads = false`. Main menu (Remix, limit 240): 151 fps
  without, 188–196 with (v0.752 on SDK v0.10 with all patches: 167). Also contains the fix from
  upstream PR rexglue/rexglue-sdk#535: with dynamic rendering a draw could run on in an
  ownership transfer pass to the same render target. Successor of 0006 and 0008; 0006 is
  open upstream PR #536.
- `0003-gpu-present-aspect-setting.patch` (command processors): the setting
  `gpu.xenos.present_aspect` (e.g. `16:9`, never saved) presents the picture in that
  ratio instead of that of the video mode. The app sets it while a video
  (`FEAnyMovie.fng`) or loading screen (`Loading*.fng`) is active, because the game draws those
  stretched over the wider render. Successor of 0004/0005, which recognised videos in the
  kernel by open `.wmv` files. Proposed upstream as rexglue/rexglue-sdk#624
  (stricter: each term 1–4095 and at most 16:1, and the ratio itself goes to the presenter as
  the display size).

- `0004-command-processor-register-ranges.patch` (command processor): a type 0 packet
  writes its registers as a range instead of one by one; that is how Carbon sets its
  shader constants, and per register a `switch` ran over all registers (`GetRegisterInfo`,
  only for a warning; now one bit per register). Proposed upstream (slightly different:
  a packet of one register still goes through `WriteRegister` there): rexglue/rexglue-sdk#618.
- `0005-vblank-on-demand.patch` (command processor, graphics driver): without `gpu.vsync`
  a `WAIT_REG_MEM` on memory gets a vblank at once (`DeliverVblankNow`) instead of
  waiting for the next tick of the timer. The timer also runs at the refresh rate without vsync
  (the SDK itself then goes to 1000 Hz) and skips one tick for each early vblank, so that
  the game sees as many vblanks on average as the screen has; the timer and the command
  processor share a mutex, so that one vblank interrupt runs at a time. Belongs to
  `gpu.vsync = false` in our profiles (see ROADMAP, "Performance on SDK v0.11").
- `0006-per-draw-request-ranges-dump-shaders.patch` (shared memory, Vulkan pipeline cache):
  `RequestRanges` allocated a vector on every call, and `AnalyzeShaderUCode` read a string
  setting on every draw (lock and copy). Upstream already has both on `development`
  (`e3ad982a` for one range, `b4dde823`); the vector with more ranges (texture cache) costs
  too little there to propose. Goes away with the next SDK version.
- `0007-shared-memory-hot-pages.patch` (shared memory, Vulkan): the setting
  `gpu.xenos.shared_memory_hot_pages` (off by default, our profiles turn it on). Pages
  that are uploaded three frames in a row are no longer write-protected; at each request
  they are compared with a shadow copy of what was uploaded last. That saves a
  `VirtualProtect` per upload and an access violation per write of the game. A hot
  page is never valid, never protected and never watched: a watch on such a page makes it
  normal again (`page_watch_count_` per page), like a GPU write, 120 frames
  without a change or 120 frames without a request. Successor of v0.10 patch 0007, written
  anew: the watch lock of v0.11 isn't recursive, so protecting can't happen under the lock.
  A hot page that is watched therefore keeps its shadow copy until `FlushWatchedHotPages`
  protects it (without the lock) and then compares; a write in the meantime still fires the
  watches.

Dropped with the move:

- 0001 (codegen: claiming code after `bctr` tail calls): solved upstream (`a86428b04`, from our
  PR #554); codegen now finds all functions without a manual list.
- 0002 (`guest_vblank_rate`): the vblank timer now follows `ui.video_mode_refresh_rate` (24–240,
  after a restart). `nfsc.frame_rate_limit` sets it.
- 0003 (`log_swap_rate`): no longer needed; `nfsc.log_fps` logs the fps.
- 0007 (hot pages): the first port made the menu on v0.11 drop to ~30 fps with
  frames of 550 ms. Written anew as 0007 (see above).
- 0010 (mutex in `InputSystem`): the input system of v0.11 has its own locks.
- AMD FidelityFX (CAS, FSR) is no longer in the SDK; Remix now presents bilinearly.

## Playing

```bash
cd nfsc/out/build/win-amd64/Release
./nfsc.exe --game_data_root="E:/Need for Speed Carbon - Recomp/game"
```

The build puts all SDK modules (`rex*.dll`, `SDL3.dll`) next to `nfsc.exe` itself. On
Windows the SDK picks Direct3D 12 by default; both profiles set `gpu.driver = "vulkan"`.

### Graphics profiles

`--nfsc.profile=original` (default) or `--nfsc.profile=remix`, or `profile = "remix"` under
`[nfsc]` in `nfsc.toml` next to the exe (the old name `profile` without a scope works too). A
profile writes its values in the title layer of the SDK; what you set yourself (toml, command
line, F1, F4) always wins.

| Setting | original | remix |
|---|---|---|
| EDRAM emulation (`gpu.vulkan.render_target_path`) | `fsi` (interlock, accurate) | `fbo` (host render targets, fast) |
| Internal resolution (`gpu.resolution_scale_x/y`) | 1 (1280×720) | 2 or 3, by the monitor |
| Anisotropic filtering (`gpu.anisotropic_override`) | -1 (what the game asks for) | 5 (16×) |
| Native 2× MSAA | on | on |
| Frame rate (`nfsc.frame_rate_limit`) | 30 fps (like the 360) | `monitor` |
| The SDK's waiting for vblanks (`gpu.vsync`) | off | off |
| Hot pages (`gpu.xenos.shared_memory_hot_pages`) | on | on |
| Skip the intro (`nfsc.skip_intro`) | off | on |

In side-by-side comparisons (menu, race start) `fbo` couldn't be told from `fsi`; high
speed with motion effects hasn't been compared yet. Who wants the accurate path in
Remix anyway: `gpu.vulkan.render_target_path = "fsi"` (under `[gpu.vulkan]`: `render_target_path`).

### Higher frame rate

`--nfsc.frame_rate_limit=120` (or 240) raises the cap (after a restart); the simulation
computes with the clock, so the race clock and physics stay in real time. The limit is in our
present hook (`framerate.cpp`): each present starts one frame interval after the previous one.
The profiles turn `gpu.vsync` off; with that the SDK gives the game a vblank as soon as it asks
for one, instead of at the refresh rate of `ui.video_mode_refresh_rate` (which we still set,
for what the game reads). With `gpu.vsync = true` the game waits for a whole vblank at every
frame again and our limit is off: a frame that is just too late then waits for the next one
(165 Hz: 82.5 fps at 6.5 ms per frame, 55 fps at 13 ms), and the FIFO present blocks the GPU
thread on the queue mutex.

On a 7800X3D with RTX 4080 (2560×1440, 165 Hz, Remix 2×): main menu, race and the start with
eight cars 150–157 fps (see ROADMAP for the measurements before and after). 157 is the limit of
the NVIDIA driver itself there (G-SYNC with low latency: refresh − refresh²/3600). The
measurements on an RTX 5080 with SDK v0.10: race at 120 ~117 fps on average (minimum ~80–90),
at 240 ~125 (menu ~175). `--nfsc.log_fps=true` logs the fps and the slowest frame every second.

Profiling: `tools/perf/sampler.cpp` is a sampling profiler without admin rights
(`sampler <pid> <seconds> <interval_ms> <out.txt> [threads] [function,...]`). For
function names, link the SDK and the game with PDBs: `-DCMAKE_SHARED_LINKER_FLAGS_RELEASE` and
`-DCMAKE_EXE_LINKER_FLAGS_RELEASE` at `-Wl,/DEBUG,/OPT:REF,/OPT:ICF` (same code, only a
PDB added). GDI screenshots are black with present mode mailbox; Desktop Duplication works
(`ffmpeg -f lavfi -i ddagrab`).
Earlier a GPU fault occurred a few times in the race at 240 and higher (NVIDIA Xid 13, see
ROADMAP); not seen since SDK patch 0006 (5 runs at 240, ~17 at 1000).

### Ultrawide

`--nfsc.aspect_ratio=21:9` (or `32:9`, or your own `W:H`; default `auto`) makes the game render
in that ratio: the 3D picture (more view to the sides, not stretched) and the HUD at the edges.
Videos and loading screens stay 16:9 (with black bars on a wide screen). On a 16:9 screen you
see the picture in letterbox. Renders 720 × W/H wide (21:9 = 1680, 32:9 = 2560, the maximum);
the main scene then goes from 4× to 2× MSAA to fit in the EDRAM. Narrower than 16:9 isn't
possible. Remix (2×) with 21:9 reaches ~117 fps on average in races at 120 (minimum ~78),
RTX 5080.

Vulkan present mode *immediate* gives a black picture on NVIDIA; the profiles set
`gpu.vulkan.allow_present_mode_immediate = false` (mailbox/FIFO work). Smart App Control has to
be off, otherwise Windows blocks every newly built `.exe`.

The generated game code is compiled with `-O2` in Debug too: at `-O0` the
videos only reached ~11 fps instead of 30 and fell further and further behind the audio.

## Status

24 Oct 2026: starts, plays the logos, intro videos and the opening cinematic and gets through
the title screen to the **main menu** (the 3D garage renders correctly). Navigating works with
controller and keyboard (among others opening MY CARS and going back). Stable and without
errors in the log. "Cannot connect to Xbox Live!" at startup is expected. Career/races not
tested yet.

### Controls and shortcut keys

An Xbox controller and the keyboard work at the same time (the keyboard is on by default
through `hid.mnk_mode`). Keyboard: Start = `Return`, A = `Space`, B = `Backspace`, back = `Tab`,
steering = `WASD`, throttle = `E`, brake = `Q`.

| Key | Function |
|---|---|
| `Alt+F4` | Exit (or **EXIT TO DESKTOP**, the last option in the main menu) |
| `F11` | Switch between fullscreen and a window (with a close button) |
| `F1` | PC SETTINGS (`ui.bind.pc_settings`) |
| `F3` | Debug overlay |
| `F4` | All settings of the SDK. Note: *Save to config* there rewrites `nfsc.toml` completely, without comments, and also fixes the values of the profile |
| `` ` `` | Console |

Savegames go to `Documents\nfsc`.

Solved: the audio ran out of sync with the videos in the Debug build (video decoding too slow
at `-O0`; measured 11 vs. 30 fps, audio always exactly 187.5 frames/s = 48 kHz / 256).

Running codegen separately:
`sdk/rexglue-sdk/out/install/win-amd64/bin/rexglue.exe codegen nfsc/nfsc_manifest.toml`
