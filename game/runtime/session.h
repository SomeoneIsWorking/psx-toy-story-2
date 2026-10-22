#pragma once

#include "audio/sound_bank.h"
#include "boot/graphics_sync.h"
#include "boot/guest_main_boot.h"
#include "boot/level_start_presentation.h"
#include "emit_memory.h"
#include "fmv/movie_player.h"
#include "frame/frame_cut.h"
#include "game_runtime.h"
#include "input/pad_owner.h"
#include "overlay/overlay_images.h"
#include "render/actor_incarnation.h"
#include "render/actor_producers.h"
#include "state_producer.h"

class Core;

namespace ts2 {

// Everything one Core owns outside guest RAM, created with its Game.
class Session final : public psx::game::TitleSessionState {
public:
  GuestMainBoot guestMainBoot;
  GraphicsSync graphicsSync;
  LevelStartPresentation levelStartPresentation;
  OverlayImages overlays;
  audio::SoundBankProcessor soundBank;
  PadOwner pad;
  fmv::GuestMoviePlayer moviePlayer;
  FrameCut frameCut;
  ActorIncarnations actorIncarnations;
  ActorProducers actorProducers;
  psx::present::StateProducerTable stateProducers;
  // The host memory a state render runs its body over, kept so renders allocate nothing.
  psx::present::Kept<psx::present::HostMemory> renderMemory;
  // True while a guest call suspended between display fields (the front-end poll) runs; its field
  // barrier then exits the executor at the field boundary so the host can present.
  bool yieldAtFieldBarrier = false;
  // The guest's own field barrier ended the last host step after running the field's deferred display 0x80021028.
  bool displayedAtBarrier = false;
};

// The session of the Game `core` belongs to; aborts when it has none.
Session &session(Core &core);

} // namespace ts2
