// nfsc - writing the loaded game image to a file (see image_dump.h).

#include "image_dump.h"

#include <fstream>
#include <string>
#include <vector>

#include <rex/kernel/guest_memory.h>
#include <rex/logging/log_category.h>

#include "settings.h"

namespace nfsc {

void DumpImageIfRequested(const rex::kernel::GuestMemory& memory, uint32_t image_base,
                          uint32_t image_size) {
  const std::string path = Settings().dump_image.Get();
  if (path.empty()) {
    return;
  }
  std::vector<std::byte> image(image_size);
  if (auto read = memory.CopyFrom(image_base, image); !read) {
    REXLOG_WARN("nfsc: could not read the image at {:08X}: {}", image_base,
                read.error().message);
    return;
  }
  std::ofstream file(path, std::ios::binary);
  file.write(reinterpret_cast<const char*>(image.data()), std::streamsize(image.size()));
  if (!file) {
    REXLOG_WARN("nfsc: could not write the image to {}", path);
    return;
  }
  REXLOG_INFO("nfsc: dumped image {:08X} ({} bytes) to {}", image_base, image_size, path);
}

}  // namespace nfsc
