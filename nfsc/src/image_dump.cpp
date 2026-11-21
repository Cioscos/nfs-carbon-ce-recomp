// nfsc - writing the loaded game image to a file (see image_dump.h).

#include "image_dump.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <span>
#include <string>
#include <vector>

#include <rex/kernel/guest_memory.h>
#include <rex/logging/log_category.h>

#include "settings.h"

namespace nfsc {

namespace {

constexpr uint32_t kImageBase = 0x82000000;
constexpr uint32_t kHeaderSize = 0x1000;

uint32_t LoadLE32(const std::vector<std::byte>& data, size_t offset) {
  uint32_t value = 0;
  std::memcpy(&value, data.data() + offset, sizeof(value));
  return value;
}

}  // namespace

void DumpImageIfRequested(const rex::kernel::GuestMemory& memory) {
  const std::string path = Settings().dump_image.Get();
  if (path.empty()) {
    return;
  }
  // The PE header of the loaded image: e_lfanew at +0x3C, SizeOfImage at PE + 0x50.
  std::vector<std::byte> header(kHeaderSize);
  if (!memory.CopyFrom(kImageBase, header) || header[0] != std::byte{'M'} ||
      header[1] != std::byte{'Z'}) {
    REXLOG_WARN("nfsc: no PE image at {:08X}; nothing dumped", kImageBase);
    return;
  }
  const uint32_t pe = LoadLE32(header, 0x3C);
  if (pe + 0x54 > kHeaderSize) {
    REXLOG_WARN("nfsc: unexpected PE header at {:08X}; nothing dumped", kImageBase);
    return;
  }
  const uint32_t size = LoadLE32(header, pe + 0x50);
  std::vector<std::byte> image(size);
  if (!memory.CopyFrom(kImageBase, image)) {
    REXLOG_WARN("nfsc: could not read the {} byte image at {:08X}", size, kImageBase);
    return;
  }
  std::ofstream file(path, std::ios::binary);
  file.write(reinterpret_cast<const char*>(image.data()), std::streamsize(image.size()));
  if (!file) {
    REXLOG_WARN("nfsc: could not write the image to {}", path);
    return;
  }
  REXLOG_INFO("nfsc: dumped image ({} bytes) to {}", size, path);
}

}  // namespace nfsc
