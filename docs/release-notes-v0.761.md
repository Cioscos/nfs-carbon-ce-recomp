# v0.761 – European edition, public test build

Need for Speed: Carbon Collector's Edition (Xbox 360), recompiled for PC: the European edition.
This fork of [mkalkhoven-po/nfs-carbon-ce-recomp](https://github.com/mkalkhoven-po/nfs-carbon-ce-recomp)
is built on its v0.76. Requires your own disc image or extracted disc of the **European
Collector's Edition** (title ID 454107EC, `default.xex` version 0.0.0.11); see the README. For
the USA edition (0.0.0.7), use the original project's releases.

## Fixed in this version

- **Switching to another window (Alt+Tab).** In v0.76 the pause was the wrong way round: the game
  went on while its window was in the background, and paused and ignored the controller when you
  came back to it. It looked like a frozen picture, or a game that no longer reacts. Now the game
  pauses when you leave its window and goes on when you come back.
- The log says when the window goes to the background and when it has the focus again, which
  helps with reports about switching windows.

Everything else is as in [v0.76](release-notes-v0.76.md).

## Upgrading

With RecompLauncher, update the game there. Otherwise extract over the old folder or into a new
one. Your savegames and settings stay where they were.

## Reporting problems

Open an issue in [this repository](https://github.com/Cioscos/nfs-carbon-ce-recomp/issues)
with what happened, your GPU, your monitor's refresh rate, the profile and settings, and the
log file (and dump file, after a crash) from the `logs` folder next to `nfsc.exe`. Nothing is
sent anywhere automatically.
