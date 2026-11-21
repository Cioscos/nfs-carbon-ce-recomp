// nfsc - addresses in default.xex v0.0.0.11 (title 454107EC, European Collector's Edition,
// NfsMWCollectorsEuropeEngRelease.pe). See addresses.h; what each function does is in the
// hook files (written for v0.0.0.7). Found in a dump of this edition (nfsc.dump_image) with
// tools/re/; the offset from the v0.0.0.7 address is in brackets.
//
// A feature is 1 once its addresses are found and checked in this edition; with 0 its hooks
// are left out of the build.
#pragma once

#define NFSC_FEATURE_PRESENT 1
#define NFSC_FEATURE_PAD 1
#define NFSC_FEATURE_WIDESCREEN 1
#define NFSC_FEATURE_MAIN_MENU 1
#define NFSC_FEATURE_LANGUAGE_SELECT 1

// framerate.cpp: the only caller of the VdSwap import (v0.0.0.7 -0x10B8).
#define NFSC_FN_PRESENT sub_826E27A8

// pad.cpp: `mr r5,r4; li r4,0; b XamInputGetState`, the XInputGetState wrapper
// (v0.0.0.7 -0x10B0).
#define NFSC_FN_PAD_GET_STATE sub_82940F38

// widescreen.cpp (each v0.0.0.7 +0x280)
// Stores 1280/720 and 1024/576 into kRenderSizes.
#define NFSC_FN_RENDER_SIZE sub_8250B350
// Loads 640/480, 1024/576, 1280/720 by mode.
#define NFSC_FN_SET_RESOLUTION_MODE sub_825003E8
// `li 1280` stored at r3+228, +292, +612 ...
#define NFSC_FN_VIEW_LAYOUT sub_824FFD30
// Indexes the table at 0x82D5B250 by 208-byte entries; the descriptor is in r5.
#define NFSC_FN_RT_DESCRIPTOR sub_82505C18

// main_menu.cpp
// Adds OPTIONS (label 0xD128F223, id 7), custom match (0xBD84501E, id 5) and quick match
// (0xCA4CF300, id 4); `li r3,152` before each allocation; flags at r1+80/81
// (v0.0.0.7 -0x10C0).
#define NFSC_FN_MAIN_MENU_SETUP sub_82641CF0
// Called after the constructor in the setup (v0.0.0.7 +0x218).
#define NFSC_FN_MENU_ADD_OPTION sub_82313870
// Called after `li r3,152` in the setup (v0.0.0.7 +0xF8).
#define NFSC_FN_ALLOC sub_821A3418
// Stores r4 at +44, r5 at +48, a byte at +64 (v0.0.0.7 +0x218).
#define NFSC_FN_OPTION_CTOR sub_8230EE90
// Compares r5 with 0x0C407210, then loads +48 (v0.0.0.7 +0x218).
#define NFSC_FN_OPTION_REACT sub_8230F030
// Hash in r3; binary search over three tables (sub_8231F8B0), returns the string or 0
// (v0.0.0.7 +0x1C0).
#define NFSC_FN_STRING_LOOKUP sub_8231F978

// language.cpp (what each does is in that file; no v0.0.0.7 counterparts were looked for)
// Writes the current language (kCurrentLanguage) and loads its text files.
#define NFSC_FN_SET_LANGUAGE sub_8231F5A8
// The Xbox 360 boot flow's list of stages: `li r4,5`, 6, 8, 0, 9, 3, 1, each followed by a
// call through vtable +284.
#define NFSC_FN_BOOT_STAGES sub_824DA910
// That vtable entry: allocates 16 bytes, stores r4 at +12, links it into the list at r3+200.
#define NFSC_FN_BOOT_ADD_STAGE sub_82323298
// The constructor in the screen table entry of "LS_LangSelect.fng" (0x82B77D90) calls it.
#define NFSC_FN_LANGUAGE_SCREEN_CTOR sub_826832C8
// Called by that constructor after its loop; vtable +28 of the screen.
#define NFSC_FN_MENU_FINISH sub_82313918
// Called by NFSC_FN_MENU_FINISH with index 0, on the option list at screen+40.
#define NFSC_FN_MENU_SELECT sub_82310208
// Called in the constructor's loop: copies the text at r4 to +84, stores r5 at +48.
#define NFSC_FN_LANGUAGE_OPTION_CTOR sub_8230EF48
// Called in the constructor's loop on the formatted label, before the allocation.
#define NFSC_FN_STRING_HASH sub_82381750
// Called in the constructor's loop on that hash, before NFSC_FN_STRING_LOOKUP.
#define NFSC_FN_LABEL_HASH sub_8231FA08
// File name in r3; 0 if the file does not exist. The game's own language cycling
// (sub_8231ED18) skips languages with it.
#define NFSC_FN_FILE_EXISTS sub_8245FC70

namespace nfsc::addr {

// Same addresses as v0.0.0.7: NFSC_FN_RENDER_SIZE stores to them.
inline constexpr uint32_t kRenderSizes[2][2] = {{0x82C651E8, 0x82C651EC},
                                                {0x82C651F0, 0x82C651F4}};
// Same as v0.0.0.7: sub_822F6F20 loads it, then *manager and +216.
inline constexpr uint32_t kFeManager = 0x82C64ACC;
// The language table: kLanguageCount entries of kLanguageEntrySize bytes, {id, name,
// global text file, frontend text file, in-game text file, ...}. NFSC_FN_SET_LANGUAGE
// searches it.
inline constexpr uint32_t kLanguageTable = 0x82B782E8;
inline constexpr uint32_t kLanguageCount = 13;
inline constexpr uint32_t kLanguageEntrySize = 28;
// The current language id, right after the table; only NFSC_FN_SET_LANGUAGE writes it.
inline constexpr uint32_t kCurrentLanguage = 0x82B78454;

}  // namespace nfsc::addr
