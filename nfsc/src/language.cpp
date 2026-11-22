// nfsc - the game's own language select screen at startup, with every language on the
// disc, and the chosen language remembered in nfsc.language.
//
// default.xex v0.0.0.11 is the "EuropeEng" build (NfsMWCollectorsEuropeEngRelease.pdb). Its
// disc has the text of 13 languages (LANGUAGES\<NAME>_GLOBAL.BIN and so on, in NFS\ZDIR.BIN),
// but the executable was built for English only. Reverse engineered from it; the names in
// addresses_v11.h are in brackets:
//   sub_82457998  game init: calls SetLanguage(0), a constant.
//   sub_8231F5A8  SetLanguage(id) [NFSC_FN_SET_LANGUAGE]: stores the id in kCurrentLanguage
//                 and loads the text files of its entry in kLanguageTable.
//   sub_824DA910  [NFSC_FN_BOOT_STAGES] the Xbox 360 boot flow adds its stages with
//                 AddStage(flow, stage) [NFSC_FN_BOOT_ADD_STAGE]: 5 and 6 (EA logos), 8, 0
//                 (attract video), 9, 3, 1. Stage 7 opens LS_LangSelect.fng, but is not added.
//   sub_826832C8  [NFSC_FN_LANGUAGE_SCREEN_CTOR] that screen's constructor loops over the
//                 language ids 0-18, but the condition left of it is `id == 0`. For English
//                 it formats the label "LANG_ENGLISH_NO_TRANSLATION" (the language's own
//                 name for itself), allocates 152 bytes, constructs the option with
//                 sub_8230EF48(option, text, id, &enabled, &flag)
//                 [NFSC_FN_LANGUAGE_OPTION_CTOR] and adds it to the menu. After the loop it
//                 calls sub_82313918(screen) [NFSC_FN_MENU_FINISH], which selects the first
//                 option with sub_82310208(screen + 40, 0) [NFSC_FN_MENU_SELECT].
//   sub_82322C40  boot flow: the option accepted on the screen; calls SetLanguage(id) and
//                 goes to the next stage. Without a choice the screen goes on by itself
//                 after 10 seconds and the language stays as it is.
//
// The hooks add stage 7 before the others, add the other languages to the screen, start the
// screen on the current language, save an accepted language to nfsc.language and make the
// init use it instead of English.
//
// The disc has the text of those languages, but the speech of English only. The audio code
// names its speech files after the language (nisaudio_it, copspeech_it and UCAPAudio_it for
// Italian) and plays nothing when they are missing: a cutscene in Italian had the idling
// engine and no voices, music or effects. The hook on the choice of the speech files makes
// it English for a language without them.

#include <atomic>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

#include <rex/logging/log_category.h>
#include <rex/ppc/hook.h>

#include "addresses.h"
#include "pad.h"
#include "settings.h"
#include "settings_window.h"

#if NFSC_FEATURE_LANGUAGE_SELECT

REX_EXTERN(NFSC_IMP(NFSC_FN_SET_LANGUAGE));
REX_EXTERN(NFSC_IMP(NFSC_FN_BOOT_STAGES));
REX_EXTERN(NFSC_IMP(NFSC_FN_BOOT_ADD_STAGE));
REX_EXTERN(NFSC_IMP(NFSC_FN_LANGUAGE_SCREEN_CTOR));
REX_EXTERN(NFSC_IMP(NFSC_FN_LANGUAGE_SCREEN_DTOR));
REX_EXTERN(NFSC_IMP(NFSC_FN_LANGUAGE_ISO_CODE));
REX_EXTERN(NFSC_IMP(NFSC_FN_SPEECH_LANGUAGE));
REX_EXTERN(NFSC_IMP(NFSC_FN_MENU_FINISH));
REX_EXTERN(NFSC_IMP(NFSC_FN_MENU_SELECT));
REX_EXTERN(NFSC_IMP(NFSC_FN_LANGUAGE_OPTION_CTOR));
REX_EXTERN(NFSC_IMP(NFSC_FN_STRING_HASH));
REX_EXTERN(NFSC_IMP(NFSC_FN_LABEL_HASH));
REX_EXTERN(NFSC_IMP(NFSC_FN_FILE_EXISTS));
REX_EXTERN(NFSC_IMP(NFSC_FN_ALLOC));
REX_EXTERN(NFSC_IMP(NFSC_FN_MENU_ADD_OPTION));
REX_EXTERN(NFSC_IMP(NFSC_FN_STRING_LOOKUP));

namespace {

constexpr const char* kSettingName = "nfsc.language";
constexpr uint32_t kLanguageStage = 7;
constexpr uint32_t kEnglish = 0;
// Mexican Spanish calls itself ESPAÑOL, as Spanish does: left off the screen.
constexpr uint32_t kMexican = 10;
// The ids the screen's constructor loops over.
constexpr uint32_t kScreenLanguageIds = 19;
constexpr uint32_t kDefaultStringLabel = 0x881F8EFA;
constexpr uint32_t kOptionSize = 152;
constexpr uint32_t kMenuOptionListOffset = 40;
// Language table entry.
constexpr uint32_t kEntryNameOffset = 4;
constexpr uint32_t kEntryGlobalFileOffset = 8;
// Locals of the screen's constructor, used by its loop the same way.
constexpr uint32_t kCtorFlagOffset = 112;
constexpr uint32_t kCtorEnabledOffset = 113;
constexpr uint32_t kCtorLabelOffset = 144;
constexpr size_t kCtorLabelSize = 64;
// A stack frame of ours for a file name, with the name above the callee's part of it.
constexpr uint32_t kNameFrameSize = 256;
constexpr uint32_t kNameFrameOffset = 112;

// The init's SetLanguage call is the first one.
std::atomic<bool> g_init_done{false};
// The init used the language of nfsc.language.
std::atomic<bool> g_remembered{false};
std::atomic<bool> g_in_screen_ctor{false};

uint32_t LoadU32(uint8_t* base, uint32_t addr) {
  return __builtin_bswap32(*reinterpret_cast<uint32_t*>(base + addr));
}

void StoreU32(uint8_t* base, uint32_t addr, uint32_t value) {
  *reinterpret_cast<uint32_t*>(base + addr) = __builtin_bswap32(value);
}

const char* GuestString(uint8_t* base, uint32_t addr) {
  return reinterpret_cast<const char*>(base + addr);
}

// The language's entry in the game's table, or 0.
uint32_t EntryOf(uint8_t* base, uint32_t id) {
  for (uint32_t i = 0; i < nfsc::addr::kLanguageCount; i++) {
    const uint32_t entry = nfsc::addr::kLanguageTable + i * nfsc::addr::kLanguageEntrySize;
    if (LoadU32(base, entry) == id) {
      return entry;
    }
  }
  return 0;
}

// The table's name of a language in lower case, as nfsc.language has it: "italian".
std::string NameOf(uint8_t* base, uint32_t entry) {
  std::string name = GuestString(base, LoadU32(base, entry + kEntryNameOffset));
  for (char& c : name) {
    c = char(std::tolower(static_cast<unsigned char>(c)));
  }
  return name;
}

uint32_t EntryByName(uint8_t* base, std::string_view name) {
  for (uint32_t i = 0; i < nfsc::addr::kLanguageCount; i++) {
    const uint32_t entry = nfsc::addr::kLanguageTable + i * nfsc::addr::kLanguageEntrySize;
    if (NameOf(base, entry) == name) {
      return entry;
    }
  }
  return 0;
}

// Whether the language's text is on the disc.
bool OnDisc(PPCContext& ctx, uint8_t* base, uint32_t entry) {
  ctx.r3.u64 = LoadU32(base, entry + kEntryGlobalFileOffset);
  NFSC_IMP(NFSC_FN_FILE_EXISTS)(ctx, base);
  return ctx.r3.u32 != 0;
}

// Whether the cutscene speech of the current language is on the disc.
bool SpeechOnDisc(PPCContext& ctx, uint8_t* base) {
  NFSC_IMP(NFSC_FN_LANGUAGE_ISO_CODE)(ctx, base);
  const uint32_t iso_code = ctx.r3.u32;
  if (iso_code == 0) {
    return false;
  }
  const uint32_t caller_frame = ctx.r1.u32;
  const uint32_t frame = caller_frame - kNameFrameSize;
  const uint32_t name = frame + kNameFrameOffset;
  *reinterpret_cast<uint32_t*>(base + frame) = __builtin_bswap32(caller_frame);
  std::snprintf(reinterpret_cast<char*>(base + name), kNameFrameSize - kNameFrameOffset,
                "sound\\streams\\nisaudio_%s.idx", GuestString(base, iso_code));
  ctx.r1.u64 = frame;
  ctx.r3.u64 = name;
  NFSC_IMP(NFSC_FN_FILE_EXISTS)(ctx, base);
  ctx.r1.u64 = caller_frame;
  return ctx.r3.u32 != 0;
}

// Adds a language to the screen the way the constructor's loop adds English.
bool AddLanguageOption(PPCContext& ctx, uint8_t* base, uint32_t screen, uint32_t frame,
                       uint32_t id, uint32_t entry) {
  const uint32_t label = frame + kCtorLabelOffset;
  std::snprintf(reinterpret_cast<char*>(base + label), kCtorLabelSize, "LANG_%s_NO_TRANSLATION",
                GuestString(base, LoadU32(base, entry + kEntryNameOffset)));
  ctx.r3.u64 = label;
  NFSC_IMP(NFSC_FN_STRING_HASH)(ctx, base);
  const uint32_t hash = ctx.r3.u32;

  ctx.r3.u64 = kOptionSize;
  ctx.r4.u64 = 0;
  ctx.r5.u64 = 0;
  ctx.r6.u64 = 0;
  NFSC_IMP(NFSC_FN_ALLOC)(ctx, base);
  const uint32_t option = ctx.r3.u32;
  if (option == 0) {
    return false;
  }

  ctx.r3.u64 = hash;
  NFSC_IMP(NFSC_FN_LABEL_HASH)(ctx, base);
  NFSC_IMP(NFSC_FN_STRING_LOOKUP)(ctx, base);
  if (ctx.r3.u32 == 0) {
    ctx.r3.u64 = kDefaultStringLabel;
    NFSC_IMP(NFSC_FN_STRING_LOOKUP)(ctx, base);
  }
  const uint32_t text = ctx.r3.u32;

  base[frame + kCtorFlagOffset] = 0;
  base[frame + kCtorEnabledOffset] = 1;
  ctx.r3.u64 = option;
  ctx.r4.u64 = text;
  ctx.r5.u64 = id;
  ctx.r6.u64 = frame + kCtorEnabledOffset;
  ctx.r7.u64 = frame + kCtorFlagOffset;
  NFSC_IMP(NFSC_FN_LANGUAGE_OPTION_CTOR)(ctx, base);

  ctx.r3.u64 = screen;
  ctx.r4.u64 = option;
  NFSC_IMP(NFSC_FN_MENU_ADD_OPTION)(ctx, base);
  return true;
}

}  // namespace

// SetLanguage: the init uses the remembered language; a language accepted on the screen is
// remembered.
REX_HOOK_RAW(NFSC_FN_SET_LANGUAGE) {
  uint32_t id = ctx.r3.u32;
  if (!g_init_done.exchange(true)) {
    const std::string saved = nfsc::Settings().language.Get();
    if (!saved.empty()) {
      const uint32_t entry = EntryByName(base, saved);
      if (entry != 0 && OnDisc(ctx, base, entry)) {
        id = LoadU32(base, entry);
        g_remembered = true;
        REXLOG_INFO("nfsc: language {} ({})", saved, id);
      } else {
        REXLOG_WARN("nfsc: language {} is not on the disc, using English", saved);
      }
    }
    ctx.r3.u64 = id;
    NFSC_IMP(NFSC_FN_SET_LANGUAGE)(ctx, base);
    return;
  }

  NFSC_IMP(NFSC_FN_SET_LANGUAGE)(ctx, base);
  const uint32_t entry = EntryOf(base, id);
  if (entry == 0) {
    return;
  }
  const std::string name = NameOf(base, entry);
  if (name != nfsc::Settings().language.Get()) {
    REXLOG_INFO("nfsc: language {} ({}) chosen", name, id);
    nfsc::SaveSetting(kSettingName, name);
  }
}

// The choice of the speech files: English's when the disc has none for the current language.
// The function reads the current language itself, so it sees English for the call; the
// game's text stays in the chosen language.
REX_HOOK_RAW(NFSC_FN_SPEECH_LANGUAGE) {
  const uint32_t current = LoadU32(base, nfsc::addr::kCurrentLanguage);
  if (current == kEnglish) {
    NFSC_IMP(NFSC_FN_SPEECH_LANGUAGE)(ctx, base);
    return;
  }
  // The language last looked for on the disc, plus one, and whether its speech was there.
  static std::atomic<uint32_t> checked{0};
  static std::atomic<bool> checked_on_disc{false};
  if (checked != current + 1) {
    const PPCContext entry = ctx;
    checked_on_disc = SpeechOnDisc(ctx, base);
    ctx = entry;
    checked = current + 1;
    if (!checked_on_disc) {
      REXLOG_INFO("nfsc: no speech for language {} on the disc, using English speech", current);
    }
  }
  if (checked_on_disc) {
    NFSC_IMP(NFSC_FN_SPEECH_LANGUAGE)(ctx, base);
    return;
  }
  StoreU32(base, nfsc::addr::kCurrentLanguage, kEnglish);
  NFSC_IMP(NFSC_FN_SPEECH_LANGUAGE)(ctx, base);
  StoreU32(base, nfsc::addr::kCurrentLanguage, current);
}

// Boot stages: the language screen first. With the intro skipped it only comes up until a
// language is chosen.
REX_HOOK_RAW(NFSC_FN_BOOT_STAGES) {
  const uint32_t flow = ctx.r3.u32;
  if (!(nfsc::Settings().skip_intro.Get() && g_remembered)) {
    ctx.r4.u64 = kLanguageStage;
    NFSC_IMP(NFSC_FN_BOOT_ADD_STAGE)(ctx, base);
    ctx.r3.u64 = flow;
  }
  NFSC_IMP(NFSC_FN_BOOT_STAGES)(ctx, base);
}

// Language screen constructor: mark it, for the hook below. The intro skip's Start presses
// stop until the screen is gone: Start does nothing on it but play the menu's sound, at
// every press.
REX_HOOK_RAW(NFSC_FN_LANGUAGE_SCREEN_CTOR) {
  nfsc::PauseIntroSkip(true);
  g_in_screen_ctor = true;
  NFSC_IMP(NFSC_FN_LANGUAGE_SCREEN_CTOR)(ctx, base);
  g_in_screen_ctor = false;
}

REX_HOOK_RAW(NFSC_FN_LANGUAGE_SCREEN_DTOR) {
  NFSC_IMP(NFSC_FN_LANGUAGE_SCREEN_DTOR)(ctx, base);
  REXLOG_INFO("nfsc: language screen closed");
  nfsc::PauseIntroSkip(false);
}

// End of a menu's options: on the language screen, first add the other languages on the
// disc, in the order of the constructor's loop, then select the current one.
REX_HOOK_RAW(NFSC_FN_MENU_FINISH) {
  if (!g_in_screen_ctor) {
    NFSC_IMP(NFSC_FN_MENU_FINISH)(ctx, base);
    return;
  }
  const uint32_t screen = ctx.r3.u32;
  const uint32_t frame = ctx.r1.u32;  // The constructor's.
  const uint32_t current = LoadU32(base, nfsc::addr::kCurrentLanguage);
  uint32_t options = 1;  // English.
  uint32_t selected = 0;
  for (uint32_t id = kEnglish + 1; id < kScreenLanguageIds; id++) {
    if (id == kMexican) {
      continue;
    }
    const uint32_t entry = EntryOf(base, id);
    if (entry == 0 || !OnDisc(ctx, base, entry)) {
      continue;
    }
    if (!AddLanguageOption(ctx, base, screen, frame, id, entry)) {
      REXLOG_WARN("nfsc: could not allocate a language option");
      break;
    }
    if (id == current) {
      selected = options;
    }
    options++;
  }
  REXLOG_INFO("nfsc: language screen with {} languages", options);

  ctx.r3.u64 = screen;
  NFSC_IMP(NFSC_FN_MENU_FINISH)(ctx, base);
  if (selected != 0) {
    ctx.r3.u64 = screen + kMenuOptionListOffset;
    ctx.r4.u64 = selected;
    NFSC_IMP(NFSC_FN_MENU_SELECT)(ctx, base);
  }
}

#endif  // NFSC_FEATURE_LANGUAGE_SELECT
