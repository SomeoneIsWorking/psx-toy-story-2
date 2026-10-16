// Frame turn boundary: per-field order, the guest main loop's title operations and the runtime factories, run against
// recorders.

#include "debug/dev_warp.h"
#include "facts/guest_facts.h"
#include "frame/frame_boundary.h"
#include "frame/outer_loop.h"
#include "frame/resident_frame.h"
#include "game.h"
#include "game_runtime.h"
#include "native_fmv.h"
#include "render_mode.h"
#include "runtime/session.h"
#include "state_render_check.h"
#include "testutil.h"
#include "title_game.h"
#include "widescreen/guest_widescreen.h"

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {

class RecordingBoundary final : public ts2::ResidentFrameBoundary {
public:
  int displayFieldQuota() const override {
    return fieldQuota;
  }
  void beginLogicFrame(uint32_t frame) override {
    operations.push_back("begin:" + std::to_string(frame));
  }
  void sampleInput() override {
    operations.emplace_back("input");
  }
  void tickDisplayField() override {
    operations.emplace_back("field");
  }
  void serviceDeferredDisplay() override {
    operations.emplace_back("deferred-display");
  }
  void updateResidentGame() override {
    operations.emplace_back("resident-update");
  }
  void advanceAudio() override {
    operations.emplace_back("audio");
  }
  void present(int guestFields) override {
    operations.push_back("present:" + std::to_string(guestFields));
  }

  std::vector<std::string> operations;
  int fieldQuota = 2;
};

class RecordingOuterLoop final : public ts2::OuterLoopBoundary {
public:
  void initializeFrontEnd() override {
    operations.emplace_back("initialize");
  }
  void restartColdFrontEnd() override {
    operations.emplace_back("restart-cold");
  }
  bool stepIntroMovies() override {
    operations.emplace_back("intro-movies");
    return introMoviesFinished;
  }
  void finishColdFrontEnd() override {
    operations.emplace_back("finish-cold");
  }
  void prepareFrontEnd() override {
    operations.emplace_back("prepare-front-end");
  }
  std::optional<int> pollFrontEndEvent() override {
    operations.emplace_back("poll");
    if (pollFieldsRemaining > 0) {
      --pollFieldsRemaining;
      return std::nullopt;
    }
    return event;
  }
  void acknowledgeResidentEntry() override {
    operations.emplace_back("ack-entry");
  }
  void finishFrontEndPoll() override {
    operations.emplace_back("finish-poll");
  }
  bool playbackMode() const override {
    return playback;
  }
  void setPlaybackMode(bool enabled) override {
    playback = enabled;
    operations.push_back(enabled ? "playback:on" : "playback:off");
  }
  void selectPlaybackLevel() override {
    operations.emplace_back("select-playback");
  }
  void selectWarpLevel(uint16_t level) override {
    operations.push_back("select-warp:" + std::to_string(level));
  }
  bool needsInteractiveSelection() const override {
    return needsInteractive;
  }
  ts2::SelectionProgress stepInteractiveSelection() override {
    operations.emplace_back("interactive-step");
    return selectionProgress;
  }
  ts2::ResidentPreparationProgress prepareResident() override {
    operations.emplace_back("prepare-resident");
    return preparationProgress;
  }
  void showMemoryDialog() override {
    operations.emplace_back("memory-dialog");
  }
  bool memoryScreenDone_ = true;

  bool stepMemoryScreen() override {
    operations.emplace_back("memory-screen-step");
    return memoryScreenDone_;
  }
  void beginMemorySelection() override {
    operations.emplace_back("begin-check-save");
  }
  void beginLoadSaveSelection() override {
    operations.emplace_back("begin-load-save");
  }
  void restartFrontEnd() override {
    operations.emplace_back("restart");
  }
  bool residentActive() const override {
    return residentIsActive;
  }
  void updateResident() override {
    operations.emplace_back("resident-update");
  }
  ts2::PostResidentTransition finishResident() override {
    operations.emplace_back("finish-resident");
    return postResidentTransition;
  }
  std::optional<int> pollLevelTransitionEvent() override {
    operations.emplace_back("level-transition");
    if (levelTransitionFieldsRemaining > 0) {
      --levelTransitionFieldsRemaining;
      return std::nullopt;
    }
    return levelTransitionEvent;
  }
  void shutdown() override {
    operations.emplace_back("shutdown");
  }

  int event = 0;
  int pollFieldsRemaining = 0;            // fields the poll spans before it returns its event
  int levelTransitionFieldsRemaining = 0; // fields the post-level transition spans before it returns
  int levelTransitionEvent = 1;           // 0 = the boot countdown finished, 1 = prepare the next level
  bool introMoviesFinished = true;
  bool playback = true;
  bool needsInteractive = true;
  ts2::SelectionProgress selectionProgress = ts2::SelectionProgress::pending;
  ts2::ResidentPreparationProgress preparationProgress = ts2::ResidentPreparationProgress::ready;
  bool residentIsActive = true;
  ts2::PostResidentTransition postResidentTransition = ts2::PostResidentTransition::residentSetup;
  std::vector<std::string> operations;
};

void test_measured_resident_order_has_one_two_field_present() {
  RecordingBoundary boundary;
  ts2::stepResidentFrame(boundary, 37);

  const std::vector<std::string> expected = {
      "begin:37", "input", "field", "field", "deferred-display", "present:2", "resident-update", "audio"};
  CHECK_EQ(boundary.operations.size(), expected.size());
  for (size_t index = 0; index < expected.size(); ++index) {
    CHECK(boundary.operations[index] == expected[index]);
  }
}

void test_transition_frame_owns_one_field() {
  RecordingBoundary boundary;
  boundary.fieldQuota = 1;
  ts2::stepResidentFrame(boundary, 9);

  const std::vector<std::string> expected = {
      "begin:9", "input", "field", "deferred-display", "present:1", "resident-update", "audio"};
  CHECK(boundary.operations == expected);
}

void test_runtime_supplies_title_frame_driver() {
  ts2::test::TitleGame title;

  CHECK(&title.game().runtime == &title.runtime());
  CHECK(title.game().frameDriver != nullptr);
  CHECK(title.game().sessionState != nullptr);
}

void test_runtime_selects_the_record_path_with_guest_widescreen() {
  ts2::test::TitleGame title;
  const ts2::Runtime &runtime = title.runtime();
  Game *game = &title.game();

  // The picture is the guest's GP0 output replayed from the frame record; producers render the in-between.
  CHECK(runtime.interpolates());
  CHECK(game->core.rsub.mode.path() == RenderPath::Record);

  const GuestWidescreenProjection *policy = runtime.guestWidescreenProjection();
  CHECK(policy != nullptr);
  game->mods.aspect = ASPECT_16_9;
  CHECK(policy->presentationAspect(game->core) == PresentationAspect::Wide16x9);

  // The record canvas holds the margins: the guest keeps its retail 512-wide canvas and centre.
  const GuestProjectionPlan wide = psx::present::guestProjectionPlan({
      .path = RenderPath::Record,
      .requested = policy->presentationAspect(game->core),
      .nativePresentation = {512, 240},
      .nativeProjection = {.extent = {512, 240}, .drawWidth = 512},
      .sink = {960, 720},
      .vramWidth = 1024,
  });
  CHECK(wide.widescreen());
  CHECK_EQ(wide.presentationExtent.width, 684);
  CHECK_EQ(wide.presentationHorizontalMargin, 86);
  CHECK_EQ(wide.guestDrawWidth, 512);
  CHECK_EQ(wide.projectionCenterX, 256);

  game->mods.aspect = ASPECT_4_3;
  const GuestProjectionPlan standard = psx::present::guestProjectionPlan({
      .path = RenderPath::Record,
      .requested = policy->presentationAspect(game->core),
      .nativePresentation = {512, 240},
      .nativeProjection = {.extent = {512, 240}, .drawWidth = 512},
      .sink = {960, 720},
      .vramWidth = 1024,
  });
  CHECK(!standard.widescreen());
  CHECK_EQ(standard.presentationHorizontalMargin, 0);
}

void test_outer_loop_reaches_normal_resident_in_finite_steps() {
  ts2::OuterLoopState state;
  RecordingOuterLoop boundary;

  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::introMovies);
  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::pollFrontEnd);
  boundary.event = 0;
  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::residentSetup);
  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::resident);
  ts2::stepOuterLoop(state, boundary);

  const std::vector<std::string> expected = {"initialize",
                                             "intro-movies",
                                             "finish-cold",
                                             "poll",
                                             "playback:on",
                                             "ack-entry",
                                             "select-playback",
                                             "prepare-resident",
                                             "resident-update"};
  CHECK(boundary.operations == expected);
}

void test_the_level_transition_spans_fields_and_then_prepares_the_next_level() {
  ts2::OuterLoopState state{ts2::OuterLoopPhase::resident};
  RecordingOuterLoop boundary;
  boundary.residentIsActive = false;
  boundary.postResidentTransition = ts2::PostResidentTransition::levelTransition;
  boundary.levelTransitionFieldsRemaining = 2;

  // The end of the level hands the phase over in one step and does no work of its own.
  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::levelTransition);
  CHECK(boundary.operations == (std::vector<std::string>{"finish-resident"}));

  // Each display field of the transition is one step, and a running transition prepares nothing:
  // the next level is not prepared until the guest's own call has returned.
  for (int field = 0; field < 2; ++field) {
    ts2::stepOuterLoop(state, boundary);
    CHECK(state.phase == ts2::OuterLoopPhase::levelTransition);
  }
  CHECK(boundary.operations == (std::vector<std::string>{"finish-resident", "level-transition", "level-transition"}));
  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::residentSetup);
  CHECK(boundary.operations.back() == "level-transition");
}

void test_a_finished_boot_countdown_at_the_level_transition_ends_the_game() {
  ts2::OuterLoopState state{ts2::OuterLoopPhase::levelTransition};
  RecordingOuterLoop boundary;
  boundary.levelTransitionEvent = 0;
  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::finished);
  CHECK(boundary.operations == (std::vector<std::string>{"level-transition", "shutdown"}));
}

void test_intro_movies_yield_one_field_per_step_until_finished() {
  ts2::OuterLoopState state{ts2::OuterLoopPhase::introMovies};
  RecordingOuterLoop boundary;
  boundary.introMoviesFinished = false;
  for (int step = 0; step < 3; ++step) {
    ts2::stepOuterLoop(state, boundary);
    CHECK(state.phase == ts2::OuterLoopPhase::introMovies);
  }
  CHECK_EQ(boundary.operations.size(), 3u);
  CHECK(boundary.operations.back() == "intro-movies");
  boundary.introMoviesFinished = true;
  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::pollFrontEnd);
  CHECK(boundary.operations[3] == "intro-movies");
  CHECK(boundary.operations[4] == "finish-cold");
  CHECK_EQ(boundary.operations.size(), 5u);
}

void test_backing_out_of_the_selection_screen_reenters_the_front_end_poll() {
  ts2::OuterLoopState state{ts2::OuterLoopPhase::interactiveSelection};
  RecordingOuterLoop boundary;
  boundary.selectionProgress = ts2::SelectionProgress::backToFrontEnd;
  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::pollFrontEnd);
  // Going back prepares nothing: the poll's own re-entry path handles the stored -1 event.
  CHECK(boundary.operations == (std::vector<std::string>{"interactive-step"}));
}

void test_front_end_poll_spans_fields_without_leaving_its_phase_until_it_returns() {
  ts2::OuterLoopState state{ts2::OuterLoopPhase::pollFrontEnd};
  RecordingOuterLoop boundary;
  boundary.event = 1;
  boundary.pollFieldsRemaining = 3;
  for (int field = 0; field < 3; ++field) {
    ts2::stepOuterLoop(state, boundary);
    CHECK(state.phase == ts2::OuterLoopPhase::pollFrontEnd);
  }
  // A running poll acts on nothing: no playback flag, no entry acknowledgement, no mode selection.
  CHECK(boundary.operations == (std::vector<std::string>{"poll", "poll", "poll"}));
  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::interactiveSelection);
  CHECK(boundary.operations.size() > 4u);
  CHECK(boundary.operations[3] == "poll");
  CHECK(boundary.operations[4] == "playback:off");
}

void test_outer_loop_interactive_path_yields_between_selection_iterations() {
  ts2::OuterLoopState state{ts2::OuterLoopPhase::pollFrontEnd};
  RecordingOuterLoop boundary;
  boundary.event = 1;

  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::interactiveSelection);
  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::interactiveSelection);
  boundary.selectionProgress = ts2::SelectionProgress::chosen;
  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::residentSetup);
  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::resident);

  const std::vector<std::string> expected = {
      "poll", "playback:off", "ack-entry", "interactive-step", "interactive-step", "prepare-resident"};
  CHECK(boundary.operations == expected);
}

void test_resident_preparation_yields_and_can_finish() {
  ts2::OuterLoopState state{ts2::OuterLoopPhase::residentSetup};
  RecordingOuterLoop boundary;
  boundary.preparationProgress = ts2::ResidentPreparationProgress::pending;
  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::residentSetup);
  CHECK(boundary.operations == std::vector<std::string>({"prepare-resident"}));

  boundary.operations.clear();
  boundary.preparationProgress = ts2::ResidentPreparationProgress::finished;
  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::finished);
  CHECK(boundary.operations == std::vector<std::string>({"prepare-resident", "shutdown"}));
}

void test_outer_loop_front_end_events_are_finite_and_non_fallthrough() {
  struct EventExpectation {
    int event;
    const char *operation;
  };
  constexpr EventExpectation expectations[] = {{2, "memory-dialog"}, {8, "restart"}};

  for (const auto &expectation : expectations) {
    ts2::OuterLoopState state{ts2::OuterLoopPhase::pollFrontEnd};
    RecordingOuterLoop boundary;
    boundary.event = expectation.event;
    ts2::stepOuterLoop(state, boundary);
    CHECK(state.phase == ts2::OuterLoopPhase::pollFrontEnd);
    const size_t expectedSize = expectation.event == 8 ? 2u : 3u;
    CHECK_EQ(boundary.operations.size(), expectedSize);
    CHECK(boundary.operations[1] == expectation.operation);
    if (expectation.event != 8) {
      CHECK(boundary.operations[2] == "finish-poll");
    }
  }

  // Events 3 and 4 are modal overlay screens: one guest call spanning display fields, so the poll stays open
  // until the screen returns.
  constexpr EventExpectation overlayExpectations[] = {{3, "begin-check-save"}, {4, "begin-load-save"}};

  for (const auto &expectation : overlayExpectations) {
    ts2::OuterLoopState state{ts2::OuterLoopPhase::pollFrontEnd};
    RecordingOuterLoop boundary;
    boundary.event = expectation.event;
    ts2::stepOuterLoop(state, boundary);
    CHECK(state.phase == ts2::OuterLoopPhase::memoryScreen);
    // operations[0] is the poll step itself; the begin is what this event dispatched.
    CHECK_EQ(boundary.operations.size(), 2u);
    CHECK(boundary.operations[1] == expectation.operation);

    // The screen is still running: one display field per step, and the poll stays open.
    boundary.memoryScreenDone_ = false;
    ts2::stepOuterLoop(state, boundary);
    CHECK(state.phase == ts2::OuterLoopPhase::memoryScreen);
    CHECK(boundary.operations[2] == "memory-screen-step");

    // The screen returned: only now is the poll finished and the loop back on the front end.
    boundary.memoryScreenDone_ = true;
    ts2::stepOuterLoop(state, boundary);
    CHECK(state.phase == ts2::OuterLoopPhase::pollFrontEnd);
    CHECK(boundary.operations[3] == "memory-screen-step");
    CHECK(boundary.operations[4] == "finish-poll");
  }

  ts2::OuterLoopState finished{ts2::OuterLoopPhase::pollFrontEnd};
  RecordingOuterLoop boundary;
  boundary.event = 9;
  ts2::stepOuterLoop(finished, boundary);
  CHECK(finished.phase == ts2::OuterLoopPhase::finished);
  CHECK(boundary.operations == std::vector<std::string>({"poll", "shutdown"}));
}

void test_resident_mode_selects_both_measured_owners() {
  CHECK_EQ(ts2::residentUpdateAddress(false), 0x8007B254u);
  CHECK_EQ(ts2::residentUpdateAddress(true), 0x8007B850u);
}

void test_post_resident_routes_are_finite_state_transitions() {
  struct ExpectedTransition {
    ts2::PostResidentTransition result;
    ts2::OuterLoopPhase phase;
  };
  constexpr ExpectedTransition transitions[] = {
      {ts2::PostResidentTransition::coldRestart, ts2::OuterLoopPhase::coldRestart},
      {ts2::PostResidentTransition::frontEndSetup, ts2::OuterLoopPhase::frontEndSetup},
      {ts2::PostResidentTransition::residentSetup, ts2::OuterLoopPhase::residentSetup},
      {ts2::PostResidentTransition::levelTransition, ts2::OuterLoopPhase::levelTransition},
      {ts2::PostResidentTransition::finished, ts2::OuterLoopPhase::finished},
  };

  for (const auto &transition : transitions) {
    ts2::OuterLoopState state{ts2::OuterLoopPhase::resident};
    RecordingOuterLoop boundary;
    boundary.residentIsActive = false;
    boundary.postResidentTransition = transition.result;
    ts2::stepOuterLoop(state, boundary);
    CHECK(state.phase == transition.phase);
    CHECK(!boundary.operations.empty());
    CHECK(boundary.operations[0] == "finish-resident");
    const size_t expectedOperations = transition.result == ts2::PostResidentTransition::finished ? 2u : 1u;
    CHECK_EQ(boundary.operations.size(), expectedOperations);
    if (transition.result == ts2::PostResidentTransition::finished) {
      CHECK(boundary.operations[1] == "shutdown");
    }
  }
}

// ── dev warp: armed while a level runs, it ends the level and the outer loop starts the chosen one ──
void test_a_warp_is_armed_only_while_a_level_runs() {
  ts2::test::TitleGame title;
  Core &core = title.core();
  ts2::OuterLoopState state{ts2::OuterLoopPhase::pollFrontEnd};
  ts2::DevWarp warp(core, state);
  CHECK(warp.arm("warp 3").starts_with("refused"));
  state.phase = ts2::OuterLoopPhase::resident;
  core.mem_w16(ts2::facts::kLoopExitReason, 3);
  CHECK(warp.arm("warp 3").starts_with("refused"));
  core.mem_w16(ts2::facts::kLoopExitReason, 0);
  CHECK(warp.arm("warp").starts_with("usage"));
  CHECK(warp.arm("warp x").starts_with("usage"));
  CHECK(warp.arm("warp 15").starts_with("refused"));
  CHECK(warp.arm("warp 3").starts_with("ok"));
}

void test_an_applied_warp_ends_the_running_level_through_its_exit_fade() {
  ts2::test::TitleGame title;
  Core &core = title.core();
  title.activateResident();
  constexpr uint32_t kStoreA3 = 0xAC071000u; // sw a3, 0x1000(zero)
  core.mem_w32(ts2::DevWarp::kFadeTo, kStoreA3);
  core.mem_w32(ts2::DevWarp::kFadeTo + 4u, 0x03E00008u);
  core.mem_w32(ts2::DevWarp::kFadeTo + 8u, 0u);
  core.r[29] = 0x801FFF00u;
  ts2::OuterLoopState state{ts2::OuterLoopPhase::resident};
  ts2::DevWarp warp(core, state);
  warp.applyArmed(10);
  CHECK(!state.warpLevel);
  CHECK_EQ(core.mem_r16(ts2::facts::kLoopExitReason), 0u);

  CHECK(warp.arm("warp 3").starts_with("ok"));
  warp.applyArmed(11);
  CHECK(state.warpLevel == std::optional<uint16_t>{3});
  CHECK_EQ(core.mem_r16(ts2::facts::kLoopExitReason), ts2::DevWarp::kLeaveToFrontEnd);
  CHECK_EQ(core.mem_r16(ts2::facts::kExitCountdown), ts2::DevWarp::kExitFadeFields);
  CHECK_EQ(core.mem_r32(0x80001000u), ts2::DevWarp::kFadeSteps);
  CHECK(!warp.warpAllowed());
}

void test_a_taken_warp_starts_its_level_in_place_of_the_games_route() {
  ts2::OuterLoopState state{ts2::OuterLoopPhase::resident};
  state.warpLevel = 3;
  RecordingOuterLoop boundary;
  boundary.residentIsActive = true;
  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::resident); // the level runs out its exit fade first
  boundary.residentIsActive = false;
  boundary.postResidentTransition = ts2::PostResidentTransition::frontEndSetup;
  ts2::stepOuterLoop(state, boundary);
  CHECK(state.phase == ts2::OuterLoopPhase::residentSetup);
  CHECK(!state.warpLevel);
  CHECK(boundary.operations == std::vector<std::string>({"resident-update", "finish-resident", "select-warp:3"}));

  // The game ending wins over a warp.
  ts2::OuterLoopState ending{ts2::OuterLoopPhase::resident};
  ending.warpLevel = 3;
  RecordingOuterLoop endingBoundary;
  endingBoundary.residentIsActive = false;
  endingBoundary.postResidentTransition = ts2::PostResidentTransition::finished;
  ts2::stepOuterLoop(ending, endingBoundary);
  CHECK(ending.phase == ts2::OuterLoopPhase::finished);
  CHECK(!ending.warpLevel);
}

// ── one deferred display per field: the guest barrier that ended the step already ran it ──
void test_a_field_displayed_at_the_guest_barrier_is_not_displayed_again() {
  ts2::test::TitleGame title;
  title.registerOverrides();
  Core &core = title.core();
  constexpr uint32_t kDeferredDisplay = 0x80021028u;
  constexpr uint32_t kDisplays = 0x80001000u;
  // Acknowledges the request at 0x800A10F8 and counts the call.
  const std::array<uint32_t, 8> body{0x3C01800Au, // lui at, 0x800A
                                     0xAC2010F8u, // sw zero, 0x10F8(at)
                                     0x8C081000u, // lw t0, 0x1000(zero)
                                     0u,
                                     0x25080001u, // addiu t0, t0, 1
                                     0xAC081000u, // sw t0, 0x1000(zero)
                                     0x03E00008u, // jr ra
                                     0u};
  for (size_t i = 0; i < body.size(); ++i) {
    core.mem_w32(kDeferredDisplay + static_cast<uint32_t>(i) * 4u, body[i]);
  }
  ts2::FrameCallState state(core);
  state.outerLoop().phase = ts2::OuterLoopPhase::resident;
  ts2::CoreFrameBoundary boundary(core, state);

  boundary.serviceDeferredDisplay();
  CHECK_EQ(core.mem_r32(kDisplays), 1u);

  // The play loop's entry block parks in its field barrier, which displays and ends the host step.
  ts2::session(core).yieldAtFieldBarrier = true;
  core.r[4] = 2;
  ts2::test::call(core, 0x8003FA68u);
  ts2::session(core).yieldAtFieldBarrier = false;
  CHECK_EQ(core.mem_r32(kDisplays), 2u);
  boundary.serviceDeferredDisplay();
  CHECK_EQ(core.mem_r32(kDisplays), 2u);
  boundary.serviceDeferredDisplay();
  CHECK_EQ(core.mem_r32(kDisplays), 3u);
}

// ── the SPU advances one display field per delivered field ──
void test_a_two_field_frame_advances_the_spu_two_fields() {
  ts2::test::TitleGame title;
  Core &core = title.core();
  ts2::FrameCallState state(core);
  state.outerLoop().phase = ts2::OuterLoopPhase::resident;
  ts2::CoreFrameBoundary boundary(core, state);
  title.game().spu_audio.clearFieldReports();
  boundary.beginLogicFrame(1);
  boundary.tickDisplayField();
  boundary.tickDisplayField();
  boundary.advanceAudio();
  CHECK_EQ(title.game().spu_audio.fieldReports().size(), 2u);
}

// ── a movie owns the picture while it plays ──
class BlankSectors final : public psx::movie::SectorSource {
public:
  bool readRaw(uint32_t, uint8_t *out) override {
    std::fill_n(out, 2352, uint8_t{0});
    return true;
  }
};

void test_a_frame_under_a_movie_presents_nothing_of_its_own() {
  ts2::test::TitleGame title;
  Core &core = title.core();
  BlankSectors sectors;
  CHECK(title.game().fmv.begin(sectors, 0, 16u * 2048u));
  CHECK(title.game().fmv.presenting());
  ts2::FrameCallState state(core);
  state.outerLoop().phase = ts2::OuterLoopPhase::introMovies;
  ts2::CoreFrameBoundary boundary(core, state);
  const uint64_t fence = title.game().presentation.fence();
  const int presents = title.game().display.frame();
  const uint64_t ticks = title.game().timing.emulatedCpuTicks();
  boundary.present(1);
  CHECK_EQ(title.game().presentation.fence(), fence + 1u);
  CHECK_EQ(title.game().display.frame(), presents);
  CHECK(title.game().timing.emulatedCpuTicks() > ticks);
}

} // namespace

int main() {
  RUN(measured_resident_order_has_one_two_field_present);
  RUN(transition_frame_owns_one_field);
  RUN(runtime_supplies_title_frame_driver);
  RUN(runtime_selects_the_record_path_with_guest_widescreen);
  RUN(outer_loop_reaches_normal_resident_in_finite_steps);
  RUN(intro_movies_yield_one_field_per_step_until_finished);
  RUN(the_level_transition_spans_fields_and_then_prepares_the_next_level);
  RUN(a_finished_boot_countdown_at_the_level_transition_ends_the_game);
  RUN(backing_out_of_the_selection_screen_reenters_the_front_end_poll);
  RUN(front_end_poll_spans_fields_without_leaving_its_phase_until_it_returns);
  RUN(outer_loop_interactive_path_yields_between_selection_iterations);
  RUN(resident_preparation_yields_and_can_finish);
  RUN(outer_loop_front_end_events_are_finite_and_non_fallthrough);
  RUN(resident_mode_selects_both_measured_owners);
  RUN(post_resident_routes_are_finite_state_transitions);
  RUN(a_warp_is_armed_only_while_a_level_runs);
  RUN(an_applied_warp_ends_the_running_level_through_its_exit_fade);
  RUN(a_taken_warp_starts_its_level_in_place_of_the_games_route);
  RUN(a_field_displayed_at_the_guest_barrier_is_not_displayed_again);
  RUN(a_two_field_frame_advances_the_spu_two_fields);
  RUN(a_frame_under_a_movie_presents_nothing_of_its_own);
  return psx::test::summary();
}
