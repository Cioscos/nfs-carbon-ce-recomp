// nfsc - guest addresses of the game edition this build is for.
//
// Each edition has its own header with the same names (addresses_v7.h, addresses_v11.h, ...):
//   NFSC_FEATURE_<X>  1 if the hooks of a feature are mapped for this edition, else 0; with 0
//                     the feature's hooks are left out and the feature does nothing
//   NFSC_FN_<NAME>    the recompiled function, e.g. sub_826E3860, for REX_HOOK_RAW / REX_EXTERN
//   nfsc::addr::k...  guest data addresses
// NFSC_IMP(NFSC_FN_<NAME>) is the original function a hook calls (__imp__sub_...).
#pragma once

#include <cstdint>

#include "nfsc_edition.h"

#define NFSC_CAT_(a, b) a##b
#define NFSC_CAT(a, b) NFSC_CAT_(a, b)
#define NFSC_IMP(fn) NFSC_CAT(__imp__, fn)

#if defined(NFSC_GAME_V7)
#include "addresses_v7.h"
#elif defined(NFSC_GAME_V11)
#include "addresses_v11.h"
#else
#error "nfsc_edition.h defines no known NFSC_GAME_<edition>"
#endif
