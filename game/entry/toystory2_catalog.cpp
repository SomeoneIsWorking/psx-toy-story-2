#include "entry/toystory2_catalog.h"

#include "runtime/toystory2_runtime.h"
#include "single_title_catalog.h"

namespace ts2 {
namespace {

// The executable SYSTEM.CNF boots, measured from the retail SLUS_008.93.
constexpr psx::host::TitleIdentity kTitleIdentity{
    .displayName = "Toy Story 2",
    .serial = "SLUS_008.93",
    .slug = "toystory2",
    .fileSize = 598016,
    .sha256 = "32c6752984f27681b4da557374e7bf6cf2a51ecb800aaefd2eb1fbfa11cb9343",
    .entry = 0x80082D60u,
    .globalPointer = 0u,
    .textAddress = 0x80010000u,
    .textSize = 0x91800u,
    .stackAddress = 0x801FFFF0u,
    .stackOffset = 0u,
    .discEnvVar = "PSXPORT_TS2_DISC",
    .discExecutablePath = "SLUS_008.93",
};

} // namespace

std::unique_ptr<psx::host::TitleCatalog> createCatalog() {
  return std::make_unique<psx::host::SingleTitleCatalog>(
      kTitleIdentity, "scratch/bin", std::make_unique<ToyStory2Runtime>());
}

} // namespace ts2
