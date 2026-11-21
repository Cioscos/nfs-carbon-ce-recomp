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
#define NFSC_FEATURE_MAIN_MENU 0

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

namespace nfsc::addr {

// Same addresses as v0.0.0.7: NFSC_FN_RENDER_SIZE stores to them.
inline constexpr uint32_t kRenderSizes[2][2] = {{0x82C651E8, 0x82C651EC},
                                                {0x82C651F0, 0x82C651F4}};
// Same as v0.0.0.7: sub_822F6F20 loads it, then *manager and +216.
inline constexpr uint32_t kFeManager = 0x82C64ACC;

}  // namespace nfsc::addr
