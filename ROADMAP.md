# Roadmap

Goal: a recompilation that is by default **visually faithful** to the Xbox 360 version, with an
optional **Remix** profile for modern hardware. Renderer: **Vulkan**.

Dates are day and month, in 2026.

## Profiles

- **Original**: 1280×720, 30 fps, native 2× MSAA, accurate EDRAM emulation
  (fragment shader interlock), original gamma, no extra post-processing.
  Goal: indistinguishable from a real 360.
- **Remix**: higher internal resolution, anisotropic filtering, the monitor's frame rate,
  widescreen/ultrawide as you like. Everything can be set separately. (CAS sharpening was part
  of it until SDK v0.11, which no longer has FidelityFX.)

## Phases

- [x] **1. Vulkan + faithful base**: Vulkan backend in the SDK on and the default, EDRAM through
      interlock, compare with D3D12, fix the "Original" profile.
      *Done (24 Oct). Vulkan and D3D12 picture identical to the eye. Compared (29 Oct) with
      360 recordings (YouTube, "NEED FOR SPEED: CARBON | Xbox 360 Gameplay" by BenedictPlays,
      1080p): Career races with the RX-8, cutscenes, race intro. Same lighting, bloom,
      motion blur, paint colour and HUD layout. The recording is ~10% brighter and the HUD cyan
      is lighter there (R 130 instead of 76): a colour shift of the recording/YouTube, because
      the HUD colours are fixed. No reason to change anything; an exact comparison needs our own
      recordings of the same scene.*
- [x] **2. Remix (existing options)**: resolution scale 2×/3×, anisotropic 16×, FSR/CAS,
      vsync; bundled in the "Remix" profile.
      *Done (25 Oct): Remix = 2×, 16× AF, `fbo`, CAS (`present_effect = cas`, mild default
      sharpness; more with `present_cas_additional_sharpness`). The effect is subtle because 2×
      on 1440p is already native. Build notes below.
      Original note: FSR/CAS needs the experimental build option
      `REXGLUE_ENABLE_FIDELITYFX` with `REXGLUE_FIDELITYFX_BACKEND=vk`, and the FidelityFX
      Vulkan backend needs the LunarG Vulkan SDK (`vulkan-1.lib`). The installation
      (`winget install KhronosGroup.VulkanSDK`, asks for admin) was cancelled on 25 Oct at the
      UAC prompt. After a failed configure, remove `CMAKE_GENERATOR_PLATFORM` from the CMake
      cache. On a 1440p screen 2× is already native, so CAS (sharpening) is the only real gain.*
- [x] **3. 60 fps**: find and patch the game's frame cap; test physics, AI and FMVs.
      *Done (24 Oct). There is no 30 fps cap in the code: the game presents on every vblank
      (60 Hz) and computes with the clock; the 360 simply didn't reach 60. The new SDK cvar
      `guest_vblank_rate` sets the cap: Original 30, Remix 60. Tested in a Quick Race:
      the race clock runs exactly in real time at 30, 60 and 120 fps, the AI races, the car
      drives normally; FMVs stay at their own 30 fps with the audio in sync.*

## Performance (measured 24 Oct, RTX 5080, Quick Race Main Street)

- **The bottleneck is the host GPU in the accurate EDRAM path.** In the race the GPU is at
  ~96% (Vulkan `fsi`) or ~98% (D3D12 `rov`) already at 1× resolution; unlimited ~75–92 fps.
  With `fbo` (host render targets): ~88 fps at ~80%. The cost hardly scales with
  resolution (interlock serialisation, not the number of pixels).
- Remix (2×, 60 fps cap) holds 60 fps in the race, but with `fsi` at 94–97% GPU (dips
  to 56); with `fbo` at ~68%.
- CPU profile (our own sampling profiler): the game threads sleep/yield ~1/3 of the time and
  spend ~22% in the D3D "wait for GPU" loop (`sub_826E0088`); so they mostly wait for the GPU.
- Codegen options: `ctr/xer/cr/reserved/non_volatile_as_local` seem to work, but break the
  videos (FMVs): green/purple blocks (seen 29 Oct; earlier tests skipped the videos). Off;
  `non_argument_as_local` makes the game hang at startup (r11/r12 are used outside the
  calling convention). The gain in the race is small and noisy (−4…+8%) because the GPU was the
  brake; measured again on 29 Oct (+1–2%), but not adopted because of the videos.
- Decision (25 Oct): **Remix uses `fbo`**, Original stays `fsi`. Side by side at 2560×1440
  (menu and race start, same key sequence): car, floor and buildings differ by < 1/255 on
  average; the remaining differences came from animation and random opponents. No
  banding in glow/neon. Remix menu now ~33% GPU instead of ~53%.
- `fsi` vs `fbo` at high speed (29 Oct): Quick Race canyon (Knife's Edge) at full throttle, same
  moments (80–105 mph, motion blur, sparks): equal to the eye, 3/255 difference on average
  (mostly the opponent and sparks). Rain can't be chosen in Quick Race (no weather option).
- Open: optimise the interlock shaders themselves (only relevant for Original now).

### Stretch goals

- [x] **4. Widescreen/ultrawide**: patch aspect ratio + FOV, HUD anchors, enlarge the
      framebuffer. *Works (28 Oct) with `aspect_ratio` (21:9, 32:9, your own W:H), in
      `nfsc/src/widescreen.cpp`:
      - Render size: `sub_8250B0D0` and `sub_82500168` put 1280×720 in 0x82C651E8/EC and
        0x82C651F0/F4; overwritten after both.
      - Layout: `sub_824FFAB0` builds viewport and tile rectangles with x1 = 1280; widened.
        With that, 3D and HUD fill the width, hor+ works by itself, the HUD anchors to the edges.
      - EDRAM: the main scene uses 4× MSAA tiles of 1280×256 (exactly 10 MiB); descriptors
        are registered through `sub_82505998` (table 0x82D5B250, +36 = MSAA). Tiles that don't
        fit at 4× go to 2× → no bands; at most 2560 wide (32:9).
      - Videos: 16:9 material was stretched; SDK patch 0004 (v0.10 series) presented in
        `present_movie_aspect` (16:9) while a `.wmv` was playing.
      - Loading screens (`Loading*.fng`) are drawn by the game stretched over the full width; as
        long as such an FE package is active the app presents at 16:9 (SDK patch 0005 of v0.10),
        like videos. On v0.11 SDK patch 0003 (`gpu.xenos.present_aspect`) does both, see
        DEVELOPMENT.
      Tested: intro videos, title screen, main menu, race intro and Quick Race (also with Remix:
      60 fps, ~80% GPU). Checked at 21:9 (28/29 Oct), all correct (not stretched, HUD at
      the edges, dialogs centred): My Cars, Challenge Series, Quick Race map and options,
      car selection, crew, race HUD with rear-view mirror, pause menu, Options, Statistics,
      Career menu, alias entry, autosave dialogs, controls screen, prologue, safehouse, world
      map, Customization (Performance, shopping cart), free roam with SMS, pause menu and Photo
      Mode, loading screens, canyon race with end screen, Standings and Player Stats (after a
      lost race, 29 Oct; the test route can't win a race without steering). Split screen
      (Quick Race, Game Mode "Split Screen") asks for START on a second controller; not
      tested.*
- [x] **5. 120/240 fps**: decouple the simulation from the frame rate (fixed tick + interpolation).
      *Done (29 Oct): not needed, the simulation already computes with the clock. Original
      reaches ~117 fps on average in races at 120 Hz, Remix (now 120 by default) 105–111; free
      roam a steady 120. Open: the cause of the Xid 13 crashes from before patches 0006–0008 was
      never found (not seen since). Details below.
      Exploration (24 Oct): the simulation is already time based, so interpolation is
      probably not needed. `guest_vblank_rate=120` works: menu 120 fps, race 60–88 fps
      (performance is the limit) with a real-time race clock.
      Investigation 29 Oct (`log_fps` added, RTX 5080, 240 Hz screen):
      - At 240 Hz the race clock runs exactly in real time (12 s per 12 s). Menu ~122 fps: a
        frame of ~7 ms misses a vblank and waits for the next (quantisation). At 1000 Hz menu ~140.
      - The ceiling doesn't depend on the cap, the resolution (1×/2×) or `fsi`/`fbo`: menu ~140,
        race 60–110 (120 Hz cap: 69–77 on average, minimum ~45). Occlusion queries off: no effect.
      - The GPU is at 96% but draws ~100 W of 360 W; GPU timestamps: in the race the GPU
        is busy with our command buffers the whole second (idle ~3 ms/s). So no computing work,
        but serialisation.
      - Cause: the game writes vertex data right before each draw; the SDK uploads those pages
        per draw to the shared memory buffer (~90,000 uploads/s, ~650 per menu frame, 1000
        per race frame), and each upload breaks the render pass and sets 2 barriers. Per frame
        ~585 render passes of 7–13 µs = ~70% of the GPU time; copies and barriers together ~25%.
      - Trial "upload-ahead" (with an upload, also take the following invalid pages,
        256 pages): render passes and barriers halved, GPU time per frame −16%, but
        6× as many pages uploaded (the CPU is still writing them) and more page faults: no net
        gain. Better idea: only take pages that haven't been written since the last ring write
        pointer that was read (a write sequence number per page), or look ahead
        in the PM4 ring at the ranges of coming draws. A deep SDK change.
      - Stability: 4 of ~15 runs at 240/1000 Hz ended in the race or while loading it
        with NVIDIA Xid 13 "SKEDCHECK36_DEPENDENCE_COUNTER_UNDERFLOW" (device lost,
        exit 0xC0000409); 3 runs at 120 Hz and earlier days at 30/60 never. Validation layers:
        no errors. Cause still unknown.
      Continued 29 Oct:
      - Note: the measurements of 28/29 Oct above were made with a release build without
        optimisation (a failed configure had emptied `CMAKE_CXX_FLAGS_RELEASE`;
        the game code ran at `-O0`). Fixed; now `-O3` again.
      - `clear_memory_page_state=false` and barriers without the fragment stage: no effect.
      - SDK patch 0006 "upload hoisting": uploads during a render pass are collected in
        a slot that was recorded before that render pass (one barrier + copies + barrier), unless
        a draw in the pass already read the pages (a bitset per page; then the old route). In the
        race ~95% of the ~90,000 uploads/s moved; GPU 93% → ~50%.
      - Result (`-O3`, Quick Race, varies per race because of the AI): 120 Hz 78–90 on average,
        minimum ~65–71 (without 0006: 72, minimum 53); 240 Hz 95–101 on average, minimum 62–70;
        menu at 240 ~140. Now CPU bound: the two game threads (render and a second one) are at
        ~98%, the GPU thread at ~57%; ~10–15k write faults/s on protected pages.
      - Stability: 5 runs (2× 120, 3× 240) without Xid 13.
      - CPU side (29 Oct, benchmark: Quick Race without throttle at 1000 Hz, menu median stable
        ±1%, race ±7%): codegen options `ctr/xer/cr/reserved/non_volatile_as_local`: menu 172 →
        175, race 110 → 115. Turned off again: they break the videos (the numbers below were
        still measured with the options, so ~1–2% too high).
        Upload-ahead tried again (16/64 pages): menu 3–5% slower, race no better.
      - Profile (`-O3`, PDBs through `-Wl,/DEBUG`): the game thread sleeps ~75%, the render
        thread waits ~57% in the GPU wait loop `sub_826E0088`, the GPU thread (command processor)
        is ~80% busy: ~15% `NtProtectVirtualMemory` (protecting pages after each upload,
        ~90,000/s), ~15% driver, the rest PM4 processing spread out. The command processor is
        now the brake.
      - SDK patch 0007 "hot pages": pages that are uploaded 3 frames in a row are no longer
        protected but compared with a shadow copy at each request (~2000–6500
        pages hot; of ~1 million requested invalid pages/s, ~70–110k are really
        uploaded). Never hot: pages with a watch (texture cache; a new watch makes
        them normal and protected again) and GPU-written pages; changes do go to the
        global watches (scaled resolve). After 120 frames unchanged, normal again.
      - `clear_memory_page_state=false` (SDK default true, a workaround for other games: all
        CPU pages invalid every frame) now in both profiles.
      - Benchmark (menu median, 1000 Hz): 174.7 → 180.7 (clear off) → 194.7 (hot pages) →
        208.5 (both, +19%). Races with throttle: 120 Hz ~117 on average (minimum 81–93), 240 Hz
        124–128 (minimum 86–97), menu at 240 ~174; GPU ~50%. Picture checked (Original
        and Remix: menus, map, intro, race); the white flash at the end of the race intro is
        there without hot pages too. 4 runs at 120/240 without Xid 13.
      - Career (savegame, 120 Hz): the same Career race (Main Street, RX-8, with a crash) played
        almost frame for frame the same with and without hot pages: without 82.3 fps on average
        (minimum 57), with 97.7 (minimum 67), same picture. Free roam itself not driven again:
        the route to it depends on loading times (Space in the Career hub sometimes opens free
        roam, sometimes the world map with a race).
      - Free roam (Career hub → FREE ROAM, driving around Historic Chinatown and a tunnel):
        a steady 120 fps (minimum 117), picture good. Startup videos good again after reverting
        the codegen options.
      - Remix profile now at 120 fps (was 60): races 105–111 on average (minimum 62–82) at
        ~45–50% GPU; with 21:9 117 on average (minimum 78); videos good.
      - Upstream (29 Oct): 0006 already existed as open PR rexglue/rexglue-sdk#536 (same design,
        for MoltenVK); our NVIDIA numbers were posted there as a comment. Hot pages (0007)
        offered: issue #548, PR #549 (measured separately with SDK defaults: menu 163 → 190,
        race 87 → 147). Fix #535 adopted as patch 0008. On 30 Oct also 0001 (GapFill after
        `bctr`, issue #552, PR #554: on `development` 72,088 → 72,847 functions, `sub_824DA838`
        was missing) and 0002 (`guest_vblank_rate`, #553 / #555) offered.
      - Review (30 Oct, review agent) of #549/#554/#555, worked in as a second commit per PR and
        in the local patches: hot pages that are no longer requested go back after 120 frames
        (at first they stayed hot forever and filled the limit), limit in bytes (64 MB),
        protecting per run of pages, `shared_memory_hot_pages` off by default (only tested on
        Windows/Vulkan; our profile turns it on); `guest_vblank_rate` limited to 1–1000 (the
        command line and NaN got around the range) and no burst of vblanks when switching;
        GapFill also discovers the call targets of gap functions and tries each function once
        (NFSC code byte-identical). Tested: canyon race Remix at 120 Hz with hot pages 117–120
        fps, without 97–119; `--guest_vblank_rate=100000` gives ~200 fps in the menu without
        hanging. Only a rexglue maintainer can merge.*

### Deliberately not

- **DLSS**: needs motion vectors, depth and jitter that the engine doesn't provide. Internal
  supersampling through the resolution scale is more than fast enough on modern GPUs and more
  faithful.

## Wishes

- [x] **Menu option "Exit to desktop"** in the main menu (asked 25 Oct, done 25 Oct).
      `nfsc/src/exit_option.cpp`: hooks on the menu setup (`sub_82642DB0`), add-option
      (`sub_82313658`), the language lookup (`sub_8231F7B8`, own label "EXIT TO DESKTOP") and
      option React (`sub_8230EE18`, ID 10 closes the game by the same route as Alt+F4).
      Recognised by its own label: ID 10 is PHOTO MODE in the pause menu (which at first closed
      the game, found and fixed 29 Oct). Not needed in the pause menu: that already has "back
      to the main menu".
- [x] **Remix and options in the game instead of a .bat** (asked 31 Oct, done 31 Oct). The menu
      option "PC SETTINGS" (ID 11, own label; `exit_option.cpp` is now called `main_menu.cpp`)
      and `F1` open our own ImGui window (`settings_window.cpp`):
      profile, fps limit, aspect ratio, fullscreen/window, and under Advanced
      resolution scale, CAS, anisotropic filtering and Vulkan/D3D12. "Profile (...)" = the
      profile's value. Apply puts only the changed keys in the root of `nfsc.toml` (the rest and
      the comments stay, unlike "Save to config" in F4, which rewrites everything), applies
      live options at once and restarts the game for `*` options: the new process starts
      immediately (without the changed `--options` of the old command line) and waits through
      `NFSC_WAIT_FOR_PID` until the old one is gone. `atexit` doesn't work: the SDK exits with
      `std::_Exit(0)` (`ReXApp::OnClosing`).
      Controller/keyboard: the SDK input can't be read from the UI thread at the same time
      (`InputSystem::RefreshDevices` rebuilds the device list on every call),
      so a hook on the game's XInputGetState wrapper (`sub_82941FE8`) takes the
      pad state while the window is open and the game gets a released pad. Buttons that
      are still down when opening or closing don't reach the window or the game.
      `Play (Remix).bat` is out of the release package.
      Tested (keyboard, 1440p): the menu item fits between OPTIONS and EXIT TO DESKTOP, F1 and
      Backspace open/close without the game menu reacting, 60 fps applied live,
      Remix and back to Original with a restart (new process 0.19 s after the old one, the keys
      disappear from `nfsc.toml` again), window/fullscreen live. Text 3x at 1440p,
      1x in a window of 1280x720. Apply is wider than Close, otherwise the focus lands on Close
      from the Advanced bar. Not tested: a real controller (same hook path as the
      keyboard through mnk).

- [x] **Package 1 from the code analysis of 5 Nov** (done 5 Nov).
      - *Frame pacing:* the fps limit is a free-running timer, the present mailbox. At 144 Hz
        120 fps shows every fifth frame twice, and at 30 fps a frame lasts 4 and 5 refreshes in
        turn; not visible on our 240 Hz. New `frame_rate_limit` (`monitor` or a
        number): Remix is on `monitor`, with the exact refresh rate from `QueryDisplayConfig` (a
        whole division above 165 Hz, so 240 → 120). Your own `guest_vblank_rate` still wins
        and disappears as soon as you change the limit in PC SETTINGS.
      - *Follows the screen:* `aspect_ratio` is `auto` by default, also for Original (nothing
        changes on 16:9 screens); Remix picks its resolution scale (1x-3x) from the
        screen height.
      - *Portable (issue #1):* `portable = true` or Save location in PC SETTINGS puts everything
        in `userdata` next to the exe and copies savegames and cache along the first time. The
        SDK reads the paths before `nfsc.toml`; `ConfigureDataPaths` therefore loads the config
        earlier, so that `cache_root` and `user_data_root` work there too. The shader cache moves
        by default from Documents (often OneDrive) to `%LOCALAPPDATA%\nfsc\cache`; an existing
        cache moves along. The stutters from issue #1 probably don't come from the disk (writing
        runs on its own thread) but from new shaders; not measured.
      - *Crash report:* on an unhandled exception or abort, `crash_report.cpp` writes a
        dump (`MiniDumpNormal`: stacks and modules, no guest memory) to `logs` and shows a
        message. `--test_crash=1` (memory fault) or `2` (abort) crashes on purpose. `__fastfail`
        can't be caught.
      - *PC SETTINGS:* a failed save now stays on screen with a message, without a restart.
      Tested (keyboard, 2560x1440, 239.97 Hz): monitor recognised exactly, cache moved (153
      pipelines loaded), Remix picks 2x and 120 fps (240/2), `guest_vblank_rate = 60` keeps
      working and disappears on a change, live 30 and back to Profile, portable on through the
      window (restart, savegame and cache copied, 234 pipelines loaded), crash and abort (dump of
      ~265 KB, message, log), read-only `nfsc.toml` (message, no restart). Not tested: a
      real 144 Hz or ultrawide monitor (the effect on frame pacing and `aspect_ratio = auto`
      outside 16:9), `__fastfail`, a real controller.
      Second screen (5 Nov, 2560x1440, 143.97 Hz, `--monitor=2`): recognised exactly, Monitor
      gives 144 fps in the menu. Race (Quick Race, throttle, 45 s): with Monitor 113.5 on average
      (minimum 76.5), with `frame_rate_limit=120` 117.2 (minimum 84.2): the CPU doesn't reach
      those 144 in a race, and the difference can also be the AI. Effect to the eye not judged yet.
      A monitor number that doesn't exist first fell back to 1080p/60; now the primary screen,
      like the SDK.
- [x] **Package 2 from the code analysis** (done 5 Nov).
      - *Pause:* opening PC SETTINGS and losing focus send the notification that the Xbox guide
        sends (XN_SYS_UI, 0x9): the game shows its own PAUSE screen in races and goes on by
        itself at "off". No reverse engineering needed. During that pause the game doesn't read
        the pad, so the window reads it itself (`pad.cpp`, SDK patch 0010: mutex in
        `InputSystem`). Movies before the race don't pause. `pause_in_background` (on by default)
        also blocks the pad in the background. "Apply and restart" warns about unsaved progress.
      - *Skipping intros:* `skip_intro` (Remix on, Original off) presses Start in pulses until
        the menu setup (`sub_82642DB0`). Main menu in 17 s without a key (was ~30 s by hand).
        New player: "Create a new alias?" stays, Start doesn't choose anything there; any real
        key stops the pulses, and after 60 s they stop anyway.
      - *Online:* CUSTOM MATCH (label BD84501E, id 5) and QUICK MATCH (CA4CF300, id 4) only gave
        "Cannot connect to Xbox Live!" and are no longer added.
      - *Keys:* "Keyboard controls" in PC SETTINGS, for the 14 most used `keybind_*` cvars.
        A new key replaces all earlier ones (also `Semicolon,Space`), double keys get a
        message, "Reset keys" puts them back and the lines disappear from `nfsc.toml` again.

## Release v0.75 (31 Oct)

- First public test version, without game files: players choose their own ISO or extracted
  folder at the first start (remembered in `Documents\nfsc\game_path.txt`; also a `game` folder
  or `.iso` next to the exe). SDK patch 0009 lets the runtime mount an ISO directly.
- Only the Collector's Edition works (the recompiled code belongs to that one `default.xex`):
  the exe checks title ID 454107EC, version 0.0.0.7 and the first page digest of the XEX
  and otherwise gives a clear message (`--skip_edition_check` for who wants it anyway).
- English README, BSD-3 license, release notes, `tools\package-release.ps1` (zip with exe, DLLs,
  license texts of the SDK and third parties). Logs only locally.
- Tested with the package in an empty folder and an empty user folder: file dialog and cancel,
  `game` folder next to the exe (remembered, edition check, window title, a new player gets
  "Create a new alias?"), and starting from the ISO (videos, menu, 30 fps). Not tested: another
  edition (message), and choosing a path in the dialog itself (text entry through keys didn't
  arrive).

## Release v0.751 (31 Oct)

- PC SETTINGS in the game (see Wishes) instead of `Play (Remix).bat`. Zip of 25.9 MB,
  smoke test with the exe from the package and an empty user folder: title "v0.751", F1 opens
  the window. Release notes in `docs/release-notes-v0.751.md`.

## Release v0.752 (15 Nov)

- Packages 1 and 2 (see Wishes), still on SDK v0.10; the move to v0.11.0 follows separately.
  Aimed at issue #1 (portable shader cache) and issue #2 (jerky at 120 fps: Remix's fixed 120
  doesn't fit every screen). The packaging script is now Python (`tools/package-release.py`).

## Release v0.76: SDK v0.11.0 (17 Nov)

- Released as v0.76 (pre-release), with release notes in `docs/release-notes-v0.76.md`.
  Smoke test with the zip in an empty folder (portable): title v0.76, old shader cache cleared
  once, main menu, no error lines.

- Moved from ReXGlue SDK v0.10 to v0.11.0 (1001 commits: new app layer
  `rex::app::TitleApp`, settings registry with scopes and layers, ~20 module DLLs, Direct3D 12
  the default on Windows, no FidelityFX any more). Building needs the VMA header from the
  Vulkan SDK.
- Our code converted: `main.cpp` (app), `settings.cpp` (scope `nfsc`, old names as aliases),
  profiles in the title layer (what the player sets always wins), paths (savegames stay in
  `Documents\nfsc`; v0.11 would take a `user` folder next to the exe), F1/F11 as keys that can
  be set (`ui.bind.pc_settings`, `ui.bind.fullscreen`), crash dumps next to the SDK's crash
  report, restart through `rex::app::RequestRestart`. Codegen finds all functions without a
  manual list.
- SDK patches from 10 to 3 (see `docs/DEVELOPMENT.md`): ISO as game folder, upload hoisting with
  the transfer pass fix, and `gpu.xenos.present_aspect`. The app now recognises videos by the
  FE package `FEAnyMovie.fng`, like loading screens; they stay 16:9 on ultrawide (tested 21:9
  and 32:9). Another fps limit only works after a restart (`ui.video_mode_refresh_rate`).
- Main menu, Remix, limit 240, RTX 5080 (2560x1440): v0.752 167 fps, v0.11 bare 151, v0.11
  with hoisting 188–196. Races: one usable lap per variant (103 against 115 fps without
  hoisting); the timed Quick Race route was too unreliable (stuck menus, crashes
  into the wall, PAUSE after losing focus), so no race numbers with hoisting.
- PC SETTINGS: on opening no item had the focus and Space arrived twice (the key and the
  A button through mnk). Now focus on the first item a frame after opening, and ImGui keyboard
  navigation off while the window is open (the keyboard operates it as a pad).
- Tested (keyboard, test folders): ISO, savegame from v0.10, main menu, Quick Race, F1, F11,
  Apply and restart (profile, old `profile` line cleaned up), intro and loading screens at
  21:9/32:9. Not tested: controller, Career beyond the menu, D3D12, a package on another PC.

## European Collector's Edition v0.0.0.11 (21 Nov)

- Second edition: `default.xex` v0.0.0.11 (`NfsMWCollectorsEuropeEngRelease.pe`), next to the US
  v0.0.0.7. One build per edition: CMake reads `game/default.xex` (`tools/xexid.py`), picks
  `addresses_<version>.h` and `config/<version>/functions.toml`, and the game only starts with
  the edition it was built for (with a message saying which edition the chosen data is). The
  release zip stays v0.0.0.7. The v7 build is unchanged (same hook symbols, tested with
  `tools/tests/test_hook_symbols.py`).
- Addresses found in a dump of the loaded image (`--nfsc.dump_image`): present -0x10B8,
  pad -0x10B0, widescreen +0x280, menu setup -0x10C0, the menu functions +0x218; render size
  globals and FE manager at the same address. No manual function boundaries needed.
- All four features on, nothing disabled. Tested up to the main menu: fps in the log, intro
  skip (up to the alias question), 21:9 (render 1680x720, 17 layout values, 2x MSAA), PC
  SETTINGS and EXIT TO DESKTOP, no online options. Races, frame rate limits, pause on losing
  focus and the controller not yet: a checklist with the user.
- **Language choice (21 Nov, `language.cpp`, `NFSC_FEATURE_LANGUAGE_SELECT`, v11 only).** The
  executable is the "EuropeEng" build and only knows English: the init does `SetLanguage(0)`,
  the Xbox 360 boot flow (`sub_824DA910`) never adds stage 7 (`LS_LangSelect.fng`) and the
  constructor of that screen (`sub_826832C8`) only adds language 0. Region, console language
  (`XGetLanguage`, `XGetGameRegion`) and the savegame play no part. The disc does have the
  texts of 13 languages (`NFS/ZDIR.BIN`); the videos are English only. Hooks: stage 7 before
  the logos, the other languages on the disc in the screen (without Mexican: that is also
  called ESPAÑOL), the screen starts on the current language, a chosen language goes to
  `nfsc.language` in `nfsc.toml` and the init uses it. The game's screen goes on by itself
  after 10 s with the current language. With `skip_intro` the screen only comes as long as no
  language has been chosen.
  Tested with an empty profile and our own `--app.config_file`: choosing Italian (log,
  `nfsc.toml`), starting again (screen on ITALIANO, alias question and main menu Italian), with
  `skip_intro` (no screen, Italian), an unknown language in `nfsc.toml` (warning, English).
  Not tested: races and Career in another language, the other languages beyond the choice
  screen, PC SETTINGS → Apply after the change in saving (`SaveChanges`).
  Known: who chooses in the last second before the 10 s skips the first EA logo (the
  screen then goes to the next stage twice).
- On exit through EXIT TO DESKTOP one line `E gpu indirect buffer failed to execute a packet`
  after the title terminate; not compared with the v7.

## Performance on SDK v0.11 (22 Nov)

Reason: 80–120 fps in races on a Ryzen 7 7800X3D with RTX 4080 (2560×1440, 165 Hz,
Remix 2×), with the GPU at 40%. Measured with `nfsc.log_fps` and our own sampling profiler
(`tools/perf/sampler.cpp`), Quick Race Main Street, same key sequence per run.

| | before | after |
|---|---|---|
| Main menu | 106 fps | 157 |
| Race, alone on the starting line | 82.5 | 157 |
| Start with eight cars (countdown) | 55 | 154 (minimum 138) |
| Driving (20 s throttle, with a collision) | not measured | 150–157 |

157 is the limit of the NVIDIA driver on this system (G-SYNC with low latency limits present
at 165 Hz to 157 fps; the driver sleeps in `vkQueuePresentKHR`).

- **Cause 1: frames were rounded to whole vblanks.** With `gpu.vsync` (SDK default) the vblank
  comes at the refresh rate and the game waits for it at each present (`WAIT_REG_MEM` on a
  flag that its vblank handler clears, polled with sleeps of 1 ms). A frame of 6.2 ms at
  165 Hz waits for the second vblank: 82.5 fps; the start with eight cars (13 ms) for the
  third: 55 fps. The game threads themselves need ~1 ms (logic) and ~2 ms (render) per frame
  and were waiting 80–90% of the time; the GPU thread slept 31% of the time in that wait. On
  top of that the present thread held the queue mutex in FIFO (10% of the GPU thread).
  Now: `gpu.vsync` off in both profiles, the limit in our present hook (`framerate.cpp`), and
  SDK patch 0005 gives a vblank as soon as the game asks for one. The present mode is then
  mailbox.
  Split up (22 Nov, start with eight cars, median over 8 s, two runs): `gpu.vsync` on
  82.5 fps; off without 0005 142.6 / 141.2 (the SDK timer then goes to 1000 Hz, the game sees
  1000 vblanks per second); off with 0005 155.6 / 153.1. So vsync off is the largest part;
  0005 gives ~12 fps and keeps the vblanks at the refresh rate. Put to upstream as a
  comment on rexglue/rexglue-sdk#556.
- **Cause 2: registers one by one.** Carbon sets shader constants with type 0 packets; those
  went per register through a virtual `WriteRegister` with a `switch` over all registers.
  19% of the GPU thread, 5% after patch 0004. Proposed upstream as rexglue/rexglue-sdk#618.
- **Cause 3: protecting pages.** `VirtualProtect` after each upload: 17% of the GPU thread at
  the start with eight cars. Hot pages are back as patch 0007: 122 → 154 fps at that spot
  (22 Nov again: off 122.4 / 124.0, on 155.6 / 153.1). Asked upstream on
  rexglue/rexglue-sdk#548 (Martin's design, #549 was closed without comment).
- Small: a vector allocation per `RequestRanges` (9%), a string setting per draw (2%).
- Checked: race clock real time (10.38 s in 10.41 s), videos equal to the old timing (same
  frames at the same moments, Original and Remix), Original exactly 30 fps, D3D12 starts and
  reaches the same 157 in the menu, picture in menus, map, car selection, race intro, start
  and driving without faults.
- Not tested: AMD/Intel (no mailbox on AMD: then FIFO relaxed), a screen without VRR (the
  limit doesn't run in step with the screen there; `gpu.vsync = true` gives the old behaviour),
  Career, split screen, long sessions with hot pages.
- With `nfsc.skip_intro` (the default in Remix) the language screen made a repeating sound for
  ten seconds: the Start pulses of the intro skip (`pad.cpp`) make the menu play its sound at
  every press. They now pause while the screen exists (constructor to destructor,
  `PauseIntroSkip`) and go on after that, with a new time limit.
- Cutscenes without sound (only an idling engine) in a language other than
  English: the disc has text in twelve languages, but speech in English only, and the game
  looks for its speech files by language (`nisaudio_it` in `sound\streams`, `copspeech_it` and
  `UCAPAudio_it` in `Sound\Speech`). If they are missing, the whole sound track of a
  cutscene doesn't play. The hook on the choice of those files (`NFSC_FN_SPEECH_LANGUAGE`,
  `language.cpp`) makes the game choose English if `nisaudio_<language>.idx` isn't on the disc;
  the text stays in the chosen language.
  Measured on the first cutscene of the career: Italian is then equal to English. It had
  nothing to do with the performance changes (same sound with `gpu.vsync` on, off and with
  vblanks at 1000 Hz).
- Measuring pitfall: the race intro (NIS) has random variants, so comparing the sound of two
  runs says nothing about a setting. The videos at startup are the same every time;
  their sound is the same with `gpu.vsync` on and off.
- Turned out not to be a fault of the game: "savegame not loaded" in the benchmarks came from
  the test script (wrong `app.user_root`).
- Open: the two game threads each run ~95% in wait loops (`KeDelayExecutionThread(0)` and
  D3D's fence loop); `DeferredCommandBuffer::Execute` (15%) could go to a thread of its own.

## Release v0.761 (24 Nov)

- The fork's fix for the pause in the background, which was the wrong way round since the move
  to SDK v0.11: the game went on in the background, and paused and ignored the pad once its
  window had the focus again. Two players reported it as a frozen picture after Alt+Tab. The
  SDK's UI system calls its listeners before it updates its own focus state, so `IsFocused()`
  still had the old value inside `OnUIEvent`; `main.cpp` now reads it in a posted task.
  Reported upstream as rexglue/rexglue-sdk#629. This is also the "PAUSE after losing focus" of
  the timed race route under v0.76.
- Unnoticed because the test scripts start the game with `--nfsc.pause_in_background=false`.
- Tested (keyboard, fullscreen, default settings, EU ISO, main menu): Alt+Tab away pauses,
  Alt+Tab back resumes, the menu takes keys again. Not tested: exclusive fullscreen, Alt+Tab
  during a race.

## Investigated

- **"Specks" on the end screen** (29 Oct, solved 30 Oct): not a render fault but a fault in our
  screenshots. GDI+ (`Graphics.CopyFromScreen`) uses the colour 0x0D0B0C internally as a marker
  and makes pixels of exactly that colour black (in a 32-bit ARGB bitmap also transparent, so
  white specks in a viewer with a white background). The dark end screen has many pixels of
  about that colour, hence the edges. Proven with a bare GDI `BitBlt` at the same moment:
  that simply has RGB (13,11,12). Checked on the way with a temporary draw log and
  resolve dumps: the game's front buffer and the swap texture are clean. Screenshots from now on
  with bare GDI (`BitBlt` into a DIB).
- **"Texture fetch constant has invalid type"** (~500 warnings per race intro, 25 Oct):
  during the intro cutscene the game samples a texture slot without a texture (type invalid,
  base address 0, size 0). Xenia's default (ignore the slot) gives no visible fault;
  `gpu_allow_invalid_fetch_constants=true` suppresses the warnings but would read memory at
  address 0 as a texture. Left at the default; harmless.

## Backlog

- [ ] **Test controllers and split screen** (put on the backlog 15 Nov). Xbox and PS5 controller
      at the same time (SDL: first connected = player 1, the keyboard belongs to player 1). Split
      screen (Quick Race, Game Mode "Split Screen") asks for START on the second controller;
      Martin has to press that. To check: steering, rumble, buttons, widescreen and HUD with two
      views, fps. Known: `pad.cpp` only blocks player 1 during PC SETTINGS and in the
      background, and PC SETTINGS only listens to player 1.

- [x] **Hot pages on SDK v0.11** (17 Nov; done 22 Nov as SDK patch 0007, see "Performance on
      SDK v0.11"). The ported version of v0.10 patch 0007 made
      the menu slow (~30 fps, frames of 550 ms); left out. Find out why (possibly the
      global watches per page) and whether it still gains anything next to hoisting.
- [ ] **Reliable race measurement.** A route that doesn't wait for time but for the picture or
      the log, or a measurement by a person, to compare v0.11 with hoisting in races.
- [ ] **Sharpening in Remix.** FidelityFX (CAS) is no longer in SDK v0.11. Decide whether the
      CAS shader has to come back as a patch of our own; first look at Remix without CAS.

## Open problems

- Career: tested with a savegame up to the first Career race (Main Street, RX-8), the Career
  hub, world map, safehouse and free roam. Further in the story (conquering territories, crew,
  canyon duels) not yet: races have to be won for that, which the test routes (keys
  without steering) can't do. Savegames work (alias and autosave in `Documents\nfsc`).
