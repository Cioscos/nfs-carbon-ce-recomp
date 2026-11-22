// nfsc - graphics profiles ("original" / "remix").

#include "profiles.h"

#include <array>
#include <cstdlib>

#include <rex/logging/log_category.h>
#include <rex/settings_registry.h>

#include "display.h"
#include "settings.h"

namespace nfsc {

namespace {

struct ProfileValue {
  std::string_view setting;
  std::string_view original;
  std::string_view remix;
};

// "original": what the Xbox 360 shows. Accurate EDRAM emulation via fragment shader
// interlock (Vulkan) or rasterizer ordered views (D3D12), the game's own texture
// filtering, native 720p with the game's 2x MSAA.
// "remix": host render targets (fbo / rtv) instead of the interlocked EDRAM path, rendered
// at the scale that covers the monitor ("auto": 2x on 1080p and 1440p, 3x on 4K) with 16x
// anisotropic filtering. The interlocked path keeps an RTX 5080 at 94-97% in races at 60
// fps; host render targets need ~68% and looked identical in side-by-side captures.
constexpr std::array kProfileValues = {
    // SDK v0.11 picks Direct3D 12 on Windows by default; we test on Vulkan.
    ProfileValue{"gpu.driver", "vulkan", "vulkan"},
    ProfileValue{"gpu.vulkan.render_target_path", "fsi", "fbo"},
    ProfileValue{"gpu.d3d12.render_target_path", "rov", "rtv"},
    ProfileValue{"gpu.anisotropic_override", "-1", "5"},
    ProfileValue{"gpu.resolution_scale_x", "1", "auto"},
    ProfileValue{"gpu.resolution_scale_y", "1", "auto"},
    ProfileValue{"gpu.xenos.native_2x_msaa", "true", "true"},
    ProfileValue{"gpu.xenos.swap_post_effect", "none", "none"},
    // The game presents on every vblank and times its simulation by the clock, so the
    // vblank rate (the refresh rate the title sees) is the frame cap: 30 fps like the
    // Xbox 360 (which never reached 60), or the monitor's rate, because a limit the monitor
    // can't divide shows some frames longer than others (120 fps on 144 Hz repeats every
    // fifth). Race timers run in real time at 30, 60, 120 and 240.
    ProfileValue{"nfsc.frame_rate_limit", "30", "monitor"},
    // With gpu.vsync on, the SDK delivers the title's vertical blanks at the frame rate limit
    // and the game waits for one at every present, so a frame that takes slightly longer than
    // one interval waits for the second blank: 82 fps (165 / 2) in races on a 165 Hz monitor
    // with the GPU at 40%. Off: blanks come when the game asks, the limit is kept at the
    // game's present (framerate.cpp) and the picture is presented without blocking (mailbox).
    ProfileValue{"gpu.vsync", "false", "false"},
    // The SDK forgets every CPU-uploaded page at the end of each frame by default (a
    // workaround for other games), so all vertex data used in a frame is uploaded again in
    // the next. Carbon doesn't need it; off was ~3% faster on SDK v0.10.
    ProfileValue{"gpu.xenos.clear_memory_page_state", "false", "false"},
    // Carbon writes vertex data right before each draw, about a thousand pages per frame in a
    // race. Protecting every page again after its upload, and the access violation at the next
    // write, took 17% of the GPU thread; these pages are compared with a copy instead (SDK
    // patch 0007). Eight cars on the grid: 122 fps without, 154 with.
    ProfileValue{"gpu.xenos.shared_memory_hot_pages", "true", "true"},
    // Immediate presentation showed only black on NVIDIA (RTX 5080, layered DXGI present)
    // on SDK v0.10; mailbox and FIFO work.
    ProfileValue{"gpu.vulkan.allow_present_mode_immediate", "false", "false"},
    // Not graphics, but a sensible default for both: keyboard controls next to a
    // controller (the input system merges both devices for player 1).
    ProfileValue{"hid.mnk_mode", "true", "true"},
    // The console's start with its logos and videos, or straight to the menu.
    ProfileValue{"nfsc.skip_intro", "false", "true"},
};

std::string g_active_profile = "original";

}  // namespace

std::string ProfileDefault(std::string_view profile, std::string_view setting) {
  for (const auto& value : kProfileValues) {
    if (value.setting == setting) {
      const std::string_view chosen = profile == "remix" ? value.remix : value.original;
      return chosen == "auto" ? std::to_string(MonitorResolutionScale()) : std::string(chosen);
    }
  }
  return {};
}

double FrameRateLimit(std::string_view limit) {
  if (limit == "monitor") {
    return MonitorFrameRate();
  }
  return std::strtod(std::string(limit).c_str(), nullptr);
}

std::string ActiveProfile() {
  return g_active_profile;
}

void ApplyProfile(std::string_view profile) {
  g_active_profile = std::string(profile);
  auto& registry = REX_SETTINGS();
  for (const auto& value : kProfileValues) {
    const std::string text = ProfileDefault(profile, value.setting);
    if (auto result = registry.SetText(rex::SettingLayer::kTitle, value.setting, text); !result) {
      REXLOG_WARN("nfsc: profile value {} = {}: {}", value.setting, text,
                  result.error().GetDescription());
    }
  }
  REXLOG_INFO("nfsc: graphics profile '{}'", profile);
}

}  // namespace nfsc
