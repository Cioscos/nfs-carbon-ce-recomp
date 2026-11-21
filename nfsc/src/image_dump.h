// nfsc - writing the loaded game image to a file (nfsc.dump_image), for the reverse
// engineering tools in tools/re/.

#pragma once

namespace rex::kernel {
class GuestMemory;
}

namespace nfsc {

// With nfsc.dump_image set: writes the PE image at 0x82000000 (SizeOfImage bytes) there.
void DumpImageIfRequested(const rex::kernel::GuestMemory& memory);

}  // namespace nfsc
