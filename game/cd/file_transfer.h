// The whole-file read the guest's loader blocks on, owned natively.
#pragma once

#include <cstdint>
#include <string>

class Core;

namespace ts2::cd {

// The CdSearchFile spin and the CdRead/CdSync wait are loading walls, not game state, so both are
// answered from the disc image here.
class FileTransfer {
public:
  struct Outcome {
    bool transferred = false;
    std::uint32_t bytes = 0;
    std::string why;
  };

  struct Request {
    // The guest's normalized spelling: upper case, version suffix stripped by 0x80082508.
    std::uint32_t path = 0;
    std::uint32_t destination = 0;
  };

  Outcome transfer(Core &core, const Request &request) const;

  static void install(Core &core);
};

} // namespace ts2::cd
