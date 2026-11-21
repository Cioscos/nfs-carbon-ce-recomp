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
#define NFSC_FEATURE_WIDESCREEN 0
#define NFSC_FEATURE_MAIN_MENU 0

// framerate.cpp: the only caller of the VdSwap import (v0.0.0.7 -0x10B8).
#define NFSC_FN_PRESENT sub_826E27A8

// pad.cpp: `mr r5,r4; li r4,0; b XamInputGetState`, the XInputGetState wrapper
// (v0.0.0.7 -0x10B0).
#define NFSC_FN_PAD_GET_STATE sub_82940F38
