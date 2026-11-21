# The game editions this project can be built for. Each build is for one of them: the game code
# is recompiled from that edition's default.xex, and the hooks use its addresses
# (src/addresses_<edition>.h) and function boundaries (config/<edition>/functions.toml).
#
# VERSION and DIGEST are what tools/xexid.py prints: the version from the execution info and the
# digest of the first page descriptor, which identifies the exact executable. All editions share
# title ID 454107EC.

set(NFSC_TITLE_ID 454107EC)
set(NFSC_EDITIONS v7 v11)

# NfsMWCollectorsRelease.pe
set(NFSC_EDITION_v7_VERSION 00000007)
set(NFSC_EDITION_v7_DIGEST 974b32299ef1c58df946dadaa0cd033a972f9148)
set(NFSC_EDITION_v7_NAME "Collector's Edition (USA) 0.0.0.7")

# NfsMWCollectorsEuropeEngRelease.pe
set(NFSC_EDITION_v11_VERSION 0000000B)
set(NFSC_EDITION_v11_DIGEST 8bcc2f4cdf7888f85ee82c012ed14d570f6e0231)
set(NFSC_EDITION_v11_NAME "Collector's Edition (Europe) 0.0.0.11")
