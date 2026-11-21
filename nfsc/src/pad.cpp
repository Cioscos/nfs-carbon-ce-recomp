// nfsc - the game's controller reads (see pad.h), through NFSC_FN_PAD_GET_STATE
// (sub_82941FE8 in v0.0.0.7), its XInputGetState wrapper. Keyboard keys arrive as a
// controller too, through the keyboard-to-controller mapping (mnk_mode).
//
// ReadPad polls the input system itself, because the game stops reading the pad
// while it is paused by the system UI notification.

#include "pad.h"

#include <atomic>
#include <chrono>
#include <cstring>

#include <rex/hid/input_system.h>
#include <rex/hid/pad.h>
#include <rex/logging/log_category.h>
#include <rex/ppc/hook.h>

#include "addresses.h"
#include "settings.h"

#if NFSC_FEATURE_PAD
REX_EXTERN(NFSC_IMP(NFSC_FN_PAD_GET_STATE));  // XInputGetState wrapper
#endif

namespace {

using namespace rex::hid;

IInputSystem* g_input = nullptr;

constexpr int kStickThreshold = 16384;
#if NFSC_FEATURE_PAD
// Start is down for this many reads, then up for as many.
constexpr uint32_t kIntroPulseReads = 3;
// A new player stops at a question before the main menu; don't keep pressing.
constexpr auto kIntroSkipTimeLimit = std::chrono::seconds(60);
#endif

std::atomic<bool> g_captured{false};
std::atomic<bool> g_background{false};
std::atomic<bool> g_releasing{false};
std::atomic<bool> g_intro_skip{false};

#if NFSC_FEATURE_PAD
std::chrono::steady_clock::time_point g_intro_skip_start;

uint16_t LoadU16(const uint8_t* p) {
  return uint16_t(p[0] << 8 | p[1]);
}

void StoreU16(uint8_t* p, uint16_t value) {
  p[0] = uint8_t(value >> 8);
  p[1] = uint8_t(value);
}

#endif  // NFSC_FEATURE_PAD

uint32_t StickToDpad(int16_t x, int16_t y) {
  uint32_t dpad = 0;
  if (y > kStickThreshold)
    dpad |= kPadDPadUp;
  if (y < -kStickThreshold)
    dpad |= kPadDPadDown;
  if (x < -kStickThreshold)
    dpad |= kPadDPadLeft;
  if (x > kStickThreshold)
    dpad |= kPadDPadRight;
  return dpad;
}

uint32_t FoldSticks(uint32_t buttons, int16_t lx, int16_t ly, int16_t rx, int16_t ry) {
  return buttons | StickToDpad(lx, ly) | StickToDpad(rx, ry);
}

#if NFSC_FEATURE_PAD

void PulseStart(uint8_t* state) {
  static uint32_t reads = 0;
  const bool down = (reads++ / kIntroPulseReads) % 2 == 0;
  if (std::chrono::steady_clock::now() - g_intro_skip_start > kIntroSkipTimeLimit) {
    g_intro_skip = false;
    return;
  }
  uint8_t* buttons = state + 4;
  // The player has taken over, maybe at the new alias question.
  if (LoadU16(buttons) != 0) {
    g_intro_skip = false;
    return;
  }
  StoreU16(buttons, down ? uint16_t(kPadStart) : uint16_t(0));
  // The packet number says that the state changed.
  state[3] = uint8_t(state[3] + reads);
}
#endif  // NFSC_FEATURE_PAD

}  // namespace

#if NFSC_FEATURE_PAD
// r3 = user index, r4 = XINPUT_STATE.
REX_HOOK_RAW(NFSC_FN_PAD_GET_STATE) {
  const uint32_t user = ctx.r3.u32;
  const uint32_t state = ctx.r4.u32;
  NFSC_IMP(NFSC_FN_PAD_GET_STATE)(ctx, base);
  if (user != 0 || ctx.r3.u32 != 0 || state == 0) {
    return;
  }
  const bool captured = g_captured || g_background;
  if (!captured && !g_releasing) {
    if (g_intro_skip) {
      PulseStart(base + state);
    }
    return;
  }
  // XINPUT_GAMEPAD after dwPacketNumber: buttons, triggers, then the sticks.
  uint8_t* gamepad = base + state + 4;
  const uint32_t pad = FoldSticks(LoadU16(gamepad), int16_t(LoadU16(gamepad + 4)),
                                  int16_t(LoadU16(gamepad + 6)), int16_t(LoadU16(gamepad + 8)),
                                  int16_t(LoadU16(gamepad + 10)));
  std::memset(gamepad, 0, 12);
  if (!captured && pad == 0) {
    g_releasing = false;
  }
}
#endif  // NFSC_FEATURE_PAD

namespace nfsc {

void SetPadCaptured(bool captured) {
  if (!captured) {
    g_releasing = true;
  }
  g_captured = captured;
}

void SetInputSystem(rex::hid::IInputSystem* input) {
  g_input = input;
}

uint32_t ReadPad() {
  if (!g_input) {
    return 0;
  }
  auto reading = g_input->GetState(0);
  if (!reading) {
    return 0;
  }
  const PadState& pad = reading->state;
  return FoldSticks(pad.buttons, pad.thumb_lx, pad.thumb_ly, pad.thumb_rx, pad.thumb_ry);
}

void SetPadBlockedInBackground(bool blocked) {
  if (!blocked) {
    g_releasing = true;
  }
  g_background = blocked;
}

void StartIntroSkip() {
  if (!Settings().skip_intro.Get()) {
    return;
  }
#if NFSC_FEATURE_PAD
  g_intro_skip_start = std::chrono::steady_clock::now();
  g_intro_skip = true;
#else
  // The Start pulses go through the pad hook.
  REXLOG_INFO("nfsc: intro skip not available for this edition");
#endif
}

void StopIntroSkip() {
  g_intro_skip = false;
}

}  // namespace nfsc
