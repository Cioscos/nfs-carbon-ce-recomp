// nfsc - per-frame work on the guest's present: the frame rate limit, and an optional
// frame rate log.
//
// Reverse engineered from default.xex (title 454107EC, v0.0.0.7):
//   sub_826E3860  NFSC_FN_PRESENT: presents a frame (it calls VdSwap); called once per
//                 game frame.

#include <algorithm>
#include <chrono>
#include <cstdint>

#include <rex/ppc/hook.h>
#include <rex/logging/log_category.h>
#include <rex/platform/thread.h>

#include "addresses.h"
#include "profiles.h"
#include "settings.h"
#include "widescreen.h"

#if NFSC_FEATURE_PRESENT

namespace {

void LogFrameRate() {
  using Clock = std::chrono::steady_clock;
  static Clock::time_point second_start = Clock::now();
  static Clock::time_point last_frame = second_start;
  static uint32_t frames = 0;
  static Clock::duration slowest{};
  const Clock::time_point now = Clock::now();
  slowest = std::max(slowest, now - last_frame);
  last_frame = now;
  ++frames;
  const auto elapsed = now - second_start;
  if (elapsed >= std::chrono::seconds(1)) {
    const double seconds = std::chrono::duration<double>(elapsed).count();
    REXLOG_INFO("nfsc: {:.1f} fps, slowest frame {:.1f} ms", frames / seconds,
                std::chrono::duration<double, std::milli>(slowest).count());
    second_start = now;
    frames = 0;
    slowest = {};
  }
}

// The frame rate limit. With gpu.vsync off the SDK's vertical blanks no longer pace the game
// (a blank at the monitor's rate rounds every frame up to a whole number of refreshes: a
// frame of 6.5 ms on a 165 Hz monitor waited for the second blank, 82 fps), so the limit is
// kept here: each present starts one frame interval after the previous one.
void LimitFrameRate() {
  using Clock = std::chrono::steady_clock;
  static const Clock::duration interval = [] {
    if (nfsc::SettingText("gpu.vsync") == "true") {
      return Clock::duration::zero();  // The vertical blanks pace the game.
    }
    const double rate = nfsc::FrameRateLimit(nfsc::Settings().frame_rate_limit.Get());
    if (rate < 1.0) {
      return Clock::duration::zero();
    }
    return std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / rate));
  }();
  if (interval == Clock::duration::zero()) {
    return;
  }
  static Clock::time_point next = Clock::now();
  next += interval;
  const Clock::time_point now = Clock::now();
  if (now < next) {
    rex::platform::SleepFor(next - now);
  } else if (now - next > interval) {
    next = now;  // Slower than the limit: don't run fast afterwards to catch up.
  }
}

}  // namespace

REX_EXTERN(NFSC_IMP(NFSC_FN_PRESENT));
REX_HOOK_RAW(NFSC_FN_PRESENT) {
  nfsc::UpdatePresentAspect(base);
  LimitFrameRate();
  if (nfsc::Settings().log_fps.Get()) {
    LogFrameRate();
  }
  NFSC_IMP(NFSC_FN_PRESENT)(ctx, base);
}

#endif  // NFSC_FEATURE_PRESENT
