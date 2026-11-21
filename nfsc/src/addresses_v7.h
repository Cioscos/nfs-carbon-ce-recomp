// nfsc - addresses in default.xex v0.0.0.7 (title 454107EC, USA Collector's Edition,
// NfsMWCollectorsRelease.pe). See addresses.h; how each was found is in the hook files.
#pragma once

#define NFSC_FEATURE_PRESENT 1
#define NFSC_FEATURE_PAD 1
#define NFSC_FEATURE_WIDESCREEN 1
#define NFSC_FEATURE_MAIN_MENU 1

// framerate.cpp: presents a frame (calls VdSwap), once per game frame.
#define NFSC_FN_PRESENT sub_826E3860

// pad.cpp: XInputGetState wrapper (r3 = user index, r4 = XINPUT_STATE).
#define NFSC_FN_PAD_GET_STATE sub_82941FE8

// widescreen.cpp
#define NFSC_FN_RENDER_SIZE sub_8250B0D0          // render size from the video mode
#define NFSC_FN_SET_RESOLUTION_MODE sub_82500168  // SetResolutionMode(state, mode)
#define NFSC_FN_VIEW_LAYOUT sub_824FFAB0          // viewport and tile rectangles
#define NFSC_FN_RT_DESCRIPTOR sub_82505998        // registers a render target descriptor

// main_menu.cpp
#define NFSC_FN_MAIN_MENU_SETUP sub_82642DB0  // builds the main menu options
#define NFSC_FN_MENU_ADD_OPTION sub_82313658  // menu: add option
#define NFSC_FN_ALLOC sub_821A3320            // allocate
#define NFSC_FN_OPTION_CTOR sub_8230EC78      // option constructor
#define NFSC_FN_OPTION_REACT sub_8230EE18     // option React()
#define NFSC_FN_STRING_LOOKUP sub_8231F7B8    // language string lookup

namespace nfsc::addr {

// The render size, stored twice by NFSC_FN_RENDER_SIZE: {width, height} pairs.
inline constexpr uint32_t kRenderSizes[2][2] = {{0x82C651E8, 0x82C651EC},
                                                {0x82C651F0, 0x82C651F4}};
// Pointer to the FE manager (front-end packages).
inline constexpr uint32_t kFeManager = 0x82C64ACC;

}  // namespace nfsc::addr
