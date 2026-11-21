// nfsc - writing the loaded game image to a file (nfsc.dump_image), for the reverse
// engineering tools in tools/re/.

#pragma once

#include <cstdint>

namespace rex::kernel {
class GuestMemory;
}

namespace nfsc {

// With nfsc.dump_image set: writes the guest image (image_base, image_size bytes; from the
// generated REX_IMAGE_BASE / REX_IMAGE_SIZE) to that file.
void DumpImageIfRequested(const rex::kernel::GuestMemory& memory, uint32_t image_base,
                          uint32_t image_size);

}  // namespace nfsc
