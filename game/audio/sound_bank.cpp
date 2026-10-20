#include "audio/sound_bank.h"

#include "core.h"
#include "native_dispatch.h"
#include "r3000.h"
#include "resumable_guest_call.h"

#include <cstdint>

namespace ts2::audio {
namespace {

constexpr std::uint32_t kSoundBankProcessor = 0x8007F108u;

void soundBankProcessorOverride(Core *core) {
  psx::cpu::callOriginalResumingToReturn(
      *core, "guest sound-bank processor", kSoundBankProcessor, core->r[psx::cpu::kRa], psx::cpu::kUnboundedCallTurns);
}

} // namespace

void SoundBankProcessor::install(Core &core) {
  psx::cpu::installNativeOverride(core, kSoundBankProcessor, "sound-bank-processor", soundBankProcessorOverride);
}

} // namespace ts2::audio
