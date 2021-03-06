﻿cmake_minimum_required(VERSION 3.12)
project(Darwinia)

file(COPY data DESTINATION .)

set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -pedantic -Wextra")

include_directories(code
	contrib/libvorbis-1.1.2/include
	contrib/linux/libogg-1.1/include
	)

add_library(unrar
	contrib/unrar/unrar.cpp
	contrib/unrar/errhnd.cpp
	contrib/unrar/rarresource.cpp
	contrib/unrar/consio.cpp
	contrib/unrar/file.cpp
	contrib/unrar/unicode.cpp
	contrib/unrar/strfn.cpp
	contrib/unrar/pathfn.cpp
	contrib/unrar/filefn.cpp
	contrib/unrar/timefn.cpp
	contrib/unrar/savepos.cpp
	contrib/unrar/find.cpp
	contrib/unrar/archive.cpp
	contrib/unrar/crypt.cpp
	contrib/unrar/crc.cpp
	contrib/unrar/rdwrfn.cpp
	contrib/unrar/system.cpp
	contrib/unrar/options.cpp
	contrib/unrar/rijndael.cpp
	contrib/unrar/volume.cpp
	contrib/unrar/sha1.cpp
	contrib/unrar/smallfn.cpp
	contrib/unrar/recvol.cpp
	contrib/unrar/arcread.cpp
	contrib/unrar/rawread.cpp
	contrib/unrar/unpack.cpp
	contrib/unrar/cmddata.cpp
	contrib/unrar/filestr.cpp
	contrib/unrar/getbits.cpp
	contrib/unrar/rs.cpp
	contrib/unrar/strlist.cpp
	contrib/unrar/extract.cpp
	contrib/unrar/scantree.cpp
	contrib/unrar/encname.cpp
	contrib/unrar/int64.cpp
	contrib/unrar/extinfo.cpp
	contrib/unrar/match.cpp
	contrib/unrar/filcreat.cpp
	contrib/unrar/ulinks.cpp
	contrib/unrar/rarvm.cpp
	contrib/unrar/global.cpp
	)
target_compile_definitions(unrar PUBLIC
	UNRAR
	)

add_subdirectory(contrib/libpng-1.2.59)

add_library(vorbis
	contrib/libvorbis-1.1.2/lib/block.c
	contrib/libogg-1.1.3/src/framing.c
	contrib/libogg-1.1.3/src/bitwise.c
	contrib/libvorbis-1.1.2/lib/vorbisfile.c
	contrib/libvorbis-1.1.2/lib/lpc.c
	contrib/libvorbis-1.1.2/lib/envelope.c
	contrib/libvorbis-1.1.2/lib/psy.c
	contrib/libvorbis-1.1.2/lib/synthesis.c
	contrib/libvorbis-1.1.2/lib/info.c
	contrib/libvorbis-1.1.2/lib/sharedbook.c
	contrib/libvorbis-1.1.2/lib/registry.c
	contrib/libvorbis-1.1.2/lib/floor0.c
	contrib/libvorbis-1.1.2/lib/lsp.c
	contrib/libvorbis-1.1.2/lib/res0.c
	contrib/libvorbis-1.1.2/lib/codebook.c
	contrib/libvorbis-1.1.2/lib/mdct.c
	contrib/libvorbis-1.1.2/lib/bitrate.c
	contrib/libvorbis-1.1.2/lib/window.c
	contrib/libvorbis-1.1.2/lib/smallft.c
	contrib/libvorbis-1.1.2/lib/mapping0.c
	contrib/libvorbis-1.1.2/lib/floor1.c
	contrib/libvorbis-1.1.2/lib/analysis.c
	)

add_library(netlib
	contrib/netlib/net_socket.cpp
	contrib/netlib/net_lib.cpp
	contrib/netlib/net_mutex_linux.cpp
	contrib/netlib/net_udp_packet.cpp
	contrib/netlib/net_socket_listener.cpp
	#contrib/netlib/net_thread.cpp
	)
target_compile_definitions(netlib PUBLIC HAVE_GETHOSTBYNAME)

add_library(eclipse
	contrib/eclipse/eclipse.cpp
	contrib/eclipse/eclwindow.cpp
	contrib/eclipse/eclbutton.cpp
	)

add_executable(darwinia
	code/app.cpp
	code/main.cpp
	code/location.cpp
	code/user_input.cpp
	code/renderer.cpp
	code/team.cpp
	code/control_help.cpp
	code/script.cpp
	code/sepulveda.cpp
	code/camera.cpp
	code/location_input.cpp
	code/particle_system.cpp
	code/water.cpp
	code/helpsystem.cpp
	code/taskmanager.cpp
	code/explosion.cpp
	code/tutorial.cpp
	code/global_world.cpp
	code/taskmanager_interface_icons.cpp
	code/taskmanager_interface_gestures.cpp
	code/location_editor.cpp
	code/startsequence.cpp
	code/landscape.cpp
	code/game_menu.cpp
	code/demoendsequence.cpp
	code/level_file.cpp
	code/obstruction_grid.cpp
	code/taskmanager_interface.cpp
	code/global_internet.cpp
	code/routing_system.cpp
	code/entity_grid.cpp
	code/unit.cpp
	code/gesture.cpp
	code/gamecursor.cpp
	code/tutorial_demo2.cpp
	code/clouds.cpp
	code/gesture_demo.cpp
	code/3d_sierpinski_gasket.cpp
	code/sepulveda_strings.cpp
	code/landscape_renderer.cpp

	code/loaders/loader.cpp
	code/loaders/speccy_loader.cpp
	code/loaders/matrix_loader.cpp
	code/loaders/fodder_loader.cpp
	code/loaders/raytrace_loader.cpp
	code/loaders/soul_loader.cpp
	code/loaders/gameoflife_loader.cpp
	code/loaders/credits_loader.cpp
	code/loaders/amiga_loader.cpp

	code/lib/input/input.cpp
	code/lib/input/inputdriver.cpp
	code/lib/input/inputdriver_sdl_mouse.cpp
	code/lib/input/inputdriver_sdl_keyboard.cpp
	code/lib/input/inputdriver_conjoin.cpp
	code/lib/input/inputdriver_chord.cpp
	code/lib/input/inputdriver_invert.cpp
	code/lib/input/inputdriver_idle.cpp
	code/lib/input/inputdriver_prefs.cpp
	code/lib/input/inputdriver_alias.cpp
	code/lib/input/file_paths.cpp
	code/lib/input/transform.cpp
	code/lib/input/input_types.cpp
	code/lib/input/control_bindings.cpp
	code/lib/input/inputspec.cpp
	code/lib/input/movement2d.cpp
	code/lib/input/inputdriver_simple.cpp
	code/lib/input/inputdriver_value.cpp
	code/lib/input/keynames.cpp

	code/lib/system_info.cpp
	code/lib/resource.cpp
	code/lib/text_renderer.cpp
	code/lib/filesys_utils.cpp
	code/lib/hi_res_time.cpp
	code/lib/targetcursor.cpp
	code/lib/window_manager_sdl.cpp
	code/lib/preferences.cpp
	code/lib/debug_utils.cpp
	code/lib/language_table.cpp
	code/lib/profiler.cpp
	code/lib/text_stream_readers.cpp
	code/lib/vector2.cpp
	code/lib/vector3.cpp
	code/lib/bitmap.cpp
	code/lib/random.cpp
	code/lib/matrix33.cpp
	code/lib/matrix34.cpp
	code/lib/rgb_colour.cpp
	code/lib/shape.cpp
	code/lib/debug_render.cpp
	code/lib/math_utils.cpp
	code/lib/plane.cpp
	code/lib/file_writer.cpp
	code/lib/invert_matrix.cpp
	code/lib/3d_sprite.cpp
	code/lib/window_manager_sdl.cpp
	code/lib/ogl_extensions.cpp
	code/lib/poster_maker.cpp
	code/lib/persisting_debug_render.cpp
	code/lib/texture_uv.cpp
	code/lib/sphere_renderer.cpp
	code/lib/binary_stream_readers.cpp
	code/lib/string_utils.cpp

	code/sound/soundsystem.cpp
	code/sound/sound_library_2d.cpp
	code/sound/sound_library_3d.cpp
	code/sound/sound_library_3d_software.cpp
	code/sound/sound_stream_decoder.cpp
	code/sound/sound_instance.cpp
	code/sound/sample_cache.cpp
	code/sound/sound_parameter.cpp
	code/sound/sound_filter.cpp

	code/worldobject/entity.cpp
	code/worldobject/worldobject.cpp
	code/worldobject/laserfence.cpp
	code/worldobject/building.cpp
	code/worldobject/incubator.cpp
	code/worldobject/spirit.cpp
	code/worldobject/engineer.cpp
	code/worldobject/controltower.cpp
	code/worldobject/bridge.cpp
	code/worldobject/teleport.cpp
	code/worldobject/researchitem.cpp
	code/worldobject/factory.cpp
	code/worldobject/spiritstore.cpp
	code/worldobject/cave.cpp
	code/worldobject/radardish.cpp
	code/worldobject/gunturret.cpp
	code/worldobject/powerstation.cpp
	code/worldobject/tree.cpp
	code/worldobject/wall.cpp
	code/worldobject/trunkport.cpp
	code/worldobject/library.cpp
	code/worldobject/generator.cpp
	code/worldobject/constructionyard.cpp
	code/worldobject/armour.cpp
	code/worldobject/flag.cpp
	code/worldobject/rocket.cpp
	code/worldobject/darwinian.cpp
	code/worldobject/anthill.cpp
	code/worldobject/armyant.cpp
	code/worldobject/mine.cpp
	code/worldobject/upgrade_port.cpp
	code/worldobject/safearea.cpp
	code/worldobject/triffid.cpp
	code/worldobject/spiritreceiver.cpp
	code/worldobject/spawnpoint.cpp
	code/worldobject/blueprintstore.cpp
	code/worldobject/ai.cpp
	code/worldobject/scripttrigger.cpp
	code/worldobject/spam.cpp
	code/worldobject/goddish.cpp
	code/worldobject/staticshape.cpp
	code/worldobject/switch.cpp
	code/worldobject/generichub.cpp
	code/worldobject/feedingtube.cpp
	code/worldobject/virii.cpp
	code/worldobject/egg.cpp
	code/worldobject/airstrike.cpp
	code/worldobject/weapons.cpp
	code/worldobject/centipede.cpp
	code/worldobject/snow.cpp
	code/worldobject/officer.cpp
	code/worldobject/insertion_squad.cpp
	code/worldobject/sporegenerator.cpp
	code/worldobject/lander.cpp
	code/worldobject/tripod.cpp
	code/worldobject/entity_leg.cpp
	code/worldobject/spider.cpp
	code/worldobject/souldestroyer.cpp
	code/worldobject/lasertrooper.cpp

	code/interface/message_dialog.cpp
	code/interface/mainmenus.cpp
	code/interface/globalworldeditor_window.cpp
	code/interface/darwinia_window.cpp
	code/interface/input_field.cpp
	code/interface/buynow_window.cpp
	code/interface/editor_window.cpp
	code/interface/landscape_window.cpp
	code/interface/lights_window.cpp
	code/interface/buildings_window.cpp
	code/interface/instant_unit_window.cpp
	code/interface/camera_mount_window.cpp
	code/interface/tree_window.cpp
	code/interface/camera_anim_window.cpp
	code/interface/drop_down_menu.cpp
	code/interface/debugmenu.cpp
	code/interface/reallyquit_window.cpp
	code/interface/demoend_window.cpp
	code/interface/profilewindow.cpp
	code/interface/network_window.cpp
	code/interface/mods_window.cpp
	code/interface/userprofile_window.cpp
	code/interface/prefs_screen_window.cpp
	code/interface/prefs_graphics_window.cpp
	code/interface/prefs_sound_window.cpp
	code/interface/prefs_other_window.cpp
	code/interface/prefs_keybindings_window.cpp

	code/network/clienttoserver.cpp
	code/network/servertoclient.cpp
	code/network/server.cpp
	code/network/servertoclientletter.cpp
	code/network/networkupdate.cpp
	code/network/generic.cpp
)

target_compile_definitions(darwinia PUBLIC
	TARGET_OS_LINUX
	DEBUG_RENDER_ENABLED)

target_include_directories(darwinia PUBLIC
	code
	contrib/eclipse
	contrib/unrar
	contrib/netlib
	)

target_link_libraries(darwinia
	eclipse
	netlib
	vorbis
	png
	unrar
	-lSDL
	-lGL
	-lGLU
	-lglut
	)
