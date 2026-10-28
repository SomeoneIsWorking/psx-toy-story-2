# Toy Story 2 title layer over psxport's native/dynarec runtime.

option(PSXPORT_BUILD_PORT "Build the Toy Story 2 native/dynarec product" ON)

include(${PSXPORT_DIR}/cmake/psxport.cmake)

set(TOYSTORY2_RUNTIME_SOURCES
  game/audio/sound_bank.cpp
  game/boot/guest_main_boot.cpp
  game/boot/graphics_sync.cpp
  game/boot/level_start_presentation.cpp
  game/cd/file_transfer.cpp
  game/debug/debug_menu.cpp
  game/debug/dev_warp.cpp
  game/execution/guest_execution.cpp
  game/frame/field_call.cpp
  game/frame/frame_cut.cpp
  game/frame/frame_boundary.cpp
  game/frame/frame_driver.cpp
  game/frame/outer_loop.cpp
  game/frame/resident_frame.cpp
  game/frame/resident_preparation.cpp
  game/fmv/movie_player.cpp
  game/front_end/memory_screen.cpp
  game/input/pad_owner.cpp
  game/input/recording_phase.cpp
  game/overlay/overlay_images.cpp
  game/overlay/overlay_slot.cpp
  game/render/actor_incarnation.cpp
  game/render/actor_producers.cpp
  game/render/hierarchy_draw_state.cpp
  game/render/hierarchy_drawer.cpp
  game/render/mesh_culling.cpp
  game/render/memory_range.cpp
  game/render/mesh_subdividers.cpp
  game/render/quad_split.cpp
  game/render/quad_split_attributes.cpp
  game/render/split_engine.cpp
  game/render/tri_split.cpp
  game/render/tri_split_attributes.cpp
  game/render/slot_packets.cpp
  game/render/slot_release.cpp
  game/render/mesh_draw_state.cpp
  game/render/mesh_drawers.cpp
  game/render/mesh_format.cpp
  game/render/ordering_tables.cpp
  game/render/part_draw_state.cpp
  game/render/part_face_drawers.cpp
  game/render/rigid_mesh_drawer.cpp
  game/render/static_mesh_drawer.cpp
  game/render/static_mesh_plan.cpp
  game/render/slot_mesh_producers.cpp
  game/runtime/session.cpp
  game/runtime/runtime.cpp
  game/widescreen/guest_widescreen.cpp
  game/widescreen/resident_widescreen.cpp
)

function(toystory2_configure_target target)
  target_include_directories(${target} PRIVATE game)
  target_link_libraries(${target} PRIVATE psxport)
  set_target_properties(${target} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
endfunction()

if(PSXPORT_BUILD_PORT)
  psxport_add_product(
    toystory2_port
    CATALOG ts2::createCatalog
    HEADER entry/catalog.h
    SOURCES game/entry/catalog.cpp ${TOYSTORY2_RUNTIME_SOURCES}
  )
  toystory2_configure_target(toystory2_port)
  if(TARGET gen_gpu_shaders)
    add_dependencies(toystory2_port gen_gpu_shaders)
  endif()
  set_target_properties(toystory2_port PROPERTIES
    ENABLE_EXPORTS ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()
