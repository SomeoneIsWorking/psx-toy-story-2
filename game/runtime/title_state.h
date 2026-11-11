// title_state.h — Toy Story 2's save-state section: it holds nothing, and a restore breaks the replay's pairing.
#pragma once

#include "machine_state.h"

namespace ts2 {

class TitleState final : public psx::state::NativeStatePort {
public:
  // Renaming the section is a format change.
  [[nodiscard]] const char *sectionName() const override {
    return "ts2";
  }
  [[nodiscard]] std::uint32_t version() const override {
    return 1;
  }
  bool save(psx::platform::BlobWriter &out, std::string &error) const override;
  bool load(psx::platform::BlobReader &in, std::string &error) override;
  // The restored frame does not follow the last one drawn.
  void restored(Core &core) override;
};

} // namespace ts2
