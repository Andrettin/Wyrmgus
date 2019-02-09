#       _________ __                 __
#      /   _____//  |_____________ _/  |______     ____  __ __  ______
#      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
#      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
#     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
#             \/                  \/          \//_____/            \/
#  ______________________                           ______________________
#                        T H E   W A R   B E G I N S
#         Stratagus - A free fantasy real time strategy game engine
#
#    CMakeLists.txt
#    Copyright (C) 2011-2019  Pali Rohár <pali.rohar@gmail.com>, cybermind <cybermindid@gmail.com> and Andrettin
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#

project(stratagus)
cmake_minimum_required(VERSION 2.6)

# Stratagus sources

include_directories(
	src
	src/include
	src/guichan/include
	src/guichan/include/guichan
	${CMAKE_CURRENT_BINARY_DIR}
)

set(beos_SRCS
	src/beos/beos.cpp
)
source_group(beos FILES ${beos_SRCS})

set(win32_SRCS
	src/win32/SetupConsole_win32.cpp
)
source_group(win32 FILES ${win32_SRCS})

set(tolua_FILES
	src/tolua/ai.pkg
	src/tolua/campaign.pkg
	src/tolua/editor.pkg
	src/tolua/font.pkg
	src/tolua/game.pkg
	#Wyrmgus start
	src/tolua/grand_strategy.pkg
	#Wyrmgus end
	src/tolua/map.pkg
	src/tolua/master.pkg
	src/tolua/minimap.pkg
	src/tolua/mod.pkg
	src/tolua/network.pkg
	src/tolua/pathfinder.pkg
	src/tolua/player.pkg
	src/tolua/sound.pkg
	src/tolua/stratagus.pkg
	src/tolua/translate.pkg
	src/tolua/trigger.pkg
	src/tolua/ui.pkg
	src/tolua/unit.pkg
	src/tolua/unittype.pkg
	src/tolua/upgrade.pkg
	src/tolua/video.pkg
	src/tolua/stratagus.lua
)
source_group(tolua FILES ${tolua_FILES})

set(stratagus_SRCS
	${CMAKE_CURRENT_BINARY_DIR}/tolua.cpp
)

# Configuration types
set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo" CACHE STRING "" FORCE)

# Additional platform checks

if(UNIX AND CMAKE_SYSTEM_NAME MATCHES BSD)
	set(BSD true)
endif()

if(UNIX AND CMAKE_SYSTEM_NAME MATCHES Linux)
	set(LINUX true)
endif()

if(WIN32 AND MSVC AND NOT CMAKE_PREFIX_PATH)
	# use a default
	set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies)
endif()

# Find all libraries

if(WIN32)
	option(ENABLE_STATIC "Compile Stratagus as static executable" ON)
else()
	option(ENABLE_STATIC "Compile Stratagus as static executable" OFF)
endif()

if(ENABLE_STATIC)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib;.a")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

find_package(Boost 1.69.0 REQUIRED)

find_package(Lua51 REQUIRED)
find_package(SDL REQUIRED)
find_package(Tolua++ REQUIRED)

find_package(Godot REQUIRED)

find_package(BZip2)
find_package(StackTrace)

find_package(X11)
find_package(OpenGL)
find_package(OpenGLES)
find_package(SDL_gles)

find_package(Sqlite)
find_package(PhysFS)

include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckSymbolExists)

# Windows RC compiler definitions

if(WIN32)
	enable_language(RC)
	include(CMakeDetermineRCCompiler)

	if(MINGW)
		set(CMAKE_RC_COMPILER_INIT windres)
		set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
	endif()
endif()

# Options for compiling

option(ENABLE_METASERVER "Build Stratagus metaserver (requires Sqlite3)" OFF)
option(ENABLE_TOUCHSCREEN "Use touchscreen input" OFF)

option(WITH_BZIP2 "Compile Stratagus with BZip2 compression support" OFF)
option(WITH_PHYSFS "Compile Stratagus with PhysFS support" OFF)
option(WITH_STACKTRACE "Compile Stratagus with StackTrace library" OFF)

option(WITH_X11 "Compile Stratagus with X11 clipboard pasting support" ON)

option(ENABLE_STRIP "Strip all symbols from executables" OFF)
option(ENABLE_USEGAMEDIR "Place all files created by Stratagus(logs, savegames) in game directory(old behavior), otherwise place everything in user directory(new behavior)" ON)
option(ENABLE_MULTIBUILD "Compile Stratagus on all CPU cores simltaneously in MSVC" ON)

if(NOT WITH_RENDERER)
	if(OPENGL_FOUND)
		set(WITH_RENDERER "OpenGL")
	elseif(OPENGLES_FOUND AND SDLGLES_FOUND)
		set(WITH_RENDERER "OpenGLES")
	else()
		set(WITH_RENDERER "NativeSDL")
	endif()
endif()

# Install paths
set(BINDIR "bin" CACHE PATH "Where to install user binaries")
set(SBINDIR "sbin" CACHE PATH "Where to install system binaries")
set(GAMEDIR "games" CACHE PATH "Where to install games binaries")
set(DOCDIR "share/doc/stratagus" CACHE PATH "Sets the doc directory to a non-default location.")
set(MANDIR "share/man/man6" CACHE PATH "Sets the man directory to a non-default location.")
set(PIXMAPSDIR "share/pixmaps" CACHE PATH "Sets the pixmaps directory to a non-default location.")
set(STRATAGUS_HEADERS "include" CACHE PATH "Where to install stratagus headers.")

if(NOT IS_ABSOLUTE "${PIXMAPSDIR}")
	set(PIXMAPSDIRABS "${CMAKE_INSTALL_PREFIX}/${PIXMAPSDIR}")
else()
	set(PIXMAPSDIRABS "${PIXMAPSDIR}")
endif()

# Stratagus definitions

add_definitions(-DPIXMAPS=\"${PIXMAPSDIRABS}\")

if(GODOT_FOUND)
	include_directories(${GODOT_DIR})
	include_directories(${GODOT_DIR}/core)
	include_directories(${GODOT_DIR}/core/math)
	include_directories(${GODOT_DIR}/modules/oamlGodotModule/oaml/include)
	include_directories(${GODOT_DIR}/platform/windows)
	include_directories(${GODOT_DIR}/thirdparty)
	include_directories(${GODOT_DIR}/thirdparty/libpng)
	include_directories(${GODOT_DIR}/thirdparty/libogg)
	include_directories(${GODOT_DIR}/thirdparty/libtheora)
	include_directories(${GODOT_DIR}/thirdparty/libvorbis)
	include_directories(${GODOT_DIR}/thirdparty/zlib)
endif()

include_directories(${LUA_INCLUDE_DIR} ${SDL_INCLUDE_DIR} ${TOLUA++_INCLUDE_DIR})
set(stratagus_LIBS ${stratagus_LIBS} ${LUA_LIBRARIES} ${SDL_LIBRARY} ${TOLUA++_LIBRARY})

if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
endif()

if(WITH_BZIP2 AND BZIP2_FOUND)
	add_definitions(-DUSE_BZ2LIB ${BZIP2_DEFINITIONS})
	include_directories(${BZIP2_INCLUDE_DIR})
	set(stratagus_LIBS ${stratagus_LIBS} ${BZIP2_LIBRARIES})
endif()

if(WITH_PHYSFS AND PHYSFS_FOUND)
	add_definitions(-DUSE_PHYSFS)
	add_definitions(-DPHYSFS_DATAFILE="data.zip")
	include_directories(${PHYSFS_INCLUDE_DIR})
	set(stratagus_LIBS ${stratagus_LIBS} ${PHYSFS_LIBRARY})
endif()

if(WITH_STACKTRACE AND STACKTRACE_FOUND)
	add_definitions(-DUSE_STACKTRACE ${STACKTRACE_DEFINITIONS})
	include_directories(${STACKTRACE_PROJECT_DIR})
	set(stratagus_LIBS ${stratagus_LIBS} ${STACKTRACE_LIBRARY})
endif()

if(WITH_X11 AND X11_FOUND)
	add_definitions(-DUSE_X11)
	include_directories(${X11_INCLUDE_DIR})
	set(stratagus_LIBS ${stratagus_LIBS} ${X11_X11_LIB})
endif()

if(WITH_RENDERER STREQUAL "OpenGL" AND OPENGL_FOUND)
	add_definitions(-DUSE_OPENGL)
	include_directories(${OPENGL_INCLUDE_DIR})
	set(stratagus_LIBS ${stratagus_LIBS} ${OPENGL_LIBRARIES})
	message(STATUS "Using OpenGL")
elseif(WITH_RENDERER STREQUAL "OpenGLES" AND OPENGLES_FOUND AND SDLGLES_FOUND)
	if(SDLGLES_TYPE STREQUAL "Native")
		add_definitions(-DUSE_GLES_NATIVE)
		message(STATUS "Using OpenGLES (SDL Native)")
	elseif(SDLGLES_TYPE STREQUAL "EGL")
		add_definitions(-DUSE_GLES_EGL)
		message(STATUS "Using OpenGLES (SDL EGL)")
	else()
		message(FATAL_ERROR "Unknown SDL gles type")
	endif()
	add_definitions(-DUSE_GLES)
	include_directories(${OPENGLES_INCLUDE_DIR} ${SDLGLES_INCLUDE_DIR})
	set(stratagus_LIBS ${stratagus_LIBS} ${OPENGLES_LIBRARIES} ${SDLGLES_LIBRARY})
elseif(NOT WITH_RENDERER STREQUAL "NativeSDL")
	message(FATAL_ERROR "Specified rendering support was not found in system.\nChoose: NativeSDL OpenGL OpenGLES")
endif()

# Platform definitions

if(WIN32)
	set(stratagus_SRCS ${stratagus_SRCS} ${win32_SRCS})
	set(stratagus_LIBS ${stratagus_LIBS} dsound winmm ws2_32 dbghelp)
endif()

if (WIN32 AND MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE=1 -DNOMINMAX)

#compile with static CRT	
#	foreach(flag_var
#		CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
#		CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
#		if(${flag_var} MATCHES "/MD")
#			string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
#		endif(${flag_var} MATCHES "/MD")
#	endforeach(flag_var)
endif()

if (WIN32 AND MINGW)
	set(stratagus_LIBS ${stratagus_LIBS} dxguid)
endif()

if(APPLE)
	add_definitions(-DUSE_MAC)
	set(SDL_LIBRARY ${SDL_LIBRARY} SDLmain)	
endif()

if(BSD)
	add_definitions(-DUSE_BSD)
endif()

if(LINUX)
	add_definitions(-DUSE_LINUX)
endif()

if(BEOS)
	add_definitions(-DUSE_BEOS)
	set(stratagus_SRCS ${stratagus_SRCS} ${beos_SRCS})
endif()

# Stratagus needs to have char by default signed
# No idea how to tell this to other compilers
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsigned-char")
endif()
if(CMAKE_COMPILER_IS_GNUC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsigned-char")
endif()

#Wyrmgus start
set(WS_CXX11_FLAGS CACHE STRING "Compilation flags to enable C++11 support")

if(MSVC)
    # C++11 compilation flag is activated by default
    # Optimisation flags can be added if deemed useful
else()
    include(CheckCXXCompilerFlag)
    if(NOT WS_CXX11_FLAGS)
        CHECK_CXX_COMPILER_FLAG("-std=c++11" WS_CXX11_FLAG_SUPPORTED)
        # MinGW supports CXX11 flag but do not compile with it. However, it works with gnu++11
        if(WS_CXX11_FLAG_SUPPORTED AND NOT MINGW)
            set(WS_CXX11_FLAGS "-std=c++11" CACHE STRING "Compilation flags to enable C++11 support" FORCE)
        else()
            set(WS_CXX11_FLAGS "-std=gnu++11" CACHE STRING "Compilation flags to enable C++11 support" FORCE)
        endif()
    endif()
endif()

set(CMAKE_CXX_FLAGS "${WS_CXX11_FLAGS} ${CMAKE_CXX_FLAGS} ${CMAKE_CXX11_STANDARD_COMPILE_OPTION}")
#Wyrmgus end

check_type_size(errno_t ERRNOT)
check_function_exists("strcpy_s" HAVE_STRCPYS)
check_function_exists("strncpy_s" HAVE_STRNCPYS)
check_function_exists("strnlen" HAVE_STRNLEN)

# mingw-w64 does not have strcat_s in any include file, but function symbol in library exists
# so rather check if we have strcat_s in string.h file
check_symbol_exists("strcat_s" "string.h" HAVE_STRCATS)

if(HAVE_ERRNOT)
	add_definitions(-DHAVE_ERRNOT)
endif()

if(HAVE_STRCPYS)
	add_definitions(-DHAVE_STRCPYS)
endif()

if(HAVE_STRNCPYS)
	add_definitions(-DHAVE_STRNCPYS)
endif()

if(HAVE_STRCATS)
	add_definitions(-DHAVE_STRCATS)
endif()

if(HAVE_STRNLEN)
	add_definitions(-DHAVE_STRNLEN)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DDEBUG)
endif()

if(ENABLE_STRIP)
	if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUC)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
	endif()
	# TODO: Add support for other compilers
endif()

if(ENABLE_USEGAMEDIR)
	add_definitions(-DUSE_GAME_DIR)
endif()

if(ENABLE_TOUCHSCREEN)
	add_definitions(-DUSE_TOUCHSCREEN)
endif()

if(ENABLE_MULTIBUILD)
	if(WIN32 AND MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
		message(STATUS "Added parallel build arguments to CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
	endif()
endif()

# Print compile info

message("==================================")

macro(log_package PACKAGE_NAME PACKAGE)
	if(NOT WITH_${PACKAGE})
		message("${PACKAGE_NAME}: Disabled (Enable by param -DWITH_${PACKAGE}=ON)")
	elseif(WITH_${PACKAGE} AND ${PACKAGE}_FOUND)
		message("${PACKAGE_NAME}: Found and enabled (Disable by param -DWITH_${PACKAGE}=OFF)")
	elseif(WITH_${PACKAGE} AND ${PACKAGE}_EMBEDDED)
		message("${PACKAGE_NAME}: Building embedded library (Disable by param -DWITH_${PACKAGE}=OFF, or install system library)")
	else()
		message("${PACKAGE_NAME}: Not found")
	endif()
endmacro()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	message("Debug mode: Yes (Disable by param -DCMAKE_BUILD_TYPE=Release)")
else()
	message("Debug mode: No (Enable by param -DCMAKE_BUILD_TYPE=Debug)")
endif()

if(ENABLE_STRIP)
	message("Strip executables: Yes (Disable by param -DENABLE_STRIP=OFF)")
else()
	message("Strip executables: No (Enable by param -DENABLE_STRIP=ON)")
endif()

if(ENABLE_STATIC)
	message("Static linking: Yes (Disable by param -DENABLE_STATIC=OFF)")
else()
	message("Static linking: No (Enable by param -DENABLE_STATIC=ON)")
endif()

if(ENABLE_USEGAMEDIR)
	message("Place game files in: game directory (Place in user directory with -DENABLE_USEGAMEDIR=OFF)")
else()
	message("Place game files in: user directory (Place in game directory with -DENABLE_USEGAMEDIR=ON)")
endif()

if(ENABLE_MULTIBUILD)
	message("Parallel building in MSVC: Yes (Disable by param -DENABLE_MULTIBUILD=OFF)")
else()
	message("Parallel building in MSVC: No (Enable by param -DENABLE_MULTIBUILD=ON)")
endif()

if(WIN32 AND MSVC)
	message("Platform: Windows (MSVC)")
elseif(WIN32 AND MINGW)
	message("Platform: Windows (MinGW)")
elseif(WIN32)
	message("Platform: Windows (Other)")
elseif(APPLE)
	message("Platform: Mac")
elseif(BSD)
	message("Platform: BSD")
elseif(BEOS)
	message("Platform: BEOS")
elseif(LINUX)
	message("Platform: Linux")
else()
	message("Platform: Other/No platform code")
endif()

if(ENABLE_TOUCHSCREEN)
	message("Touchscreen input: Yes (Disable by param -DENABLE_TOUCHSCREEN=OFF)")
else()
	message("Touchscreen input: No (Enable by param -DENABLE_TOUCHSCREEN=ON)")
endif()

if(ENABLE_METASERVER)
	if(SQLITE_FOUND)
		message("Metaserver: Yes (Disable by param -DENABLE_METASERVER=OFF)")
	else()
		message("Metaserver: No (Sqlite3 dependency not found)")
	endif()
else()
	message("Metaserver: No (Enable by param -DENABLE_METASERVER=ON)")
endif()

log_package("Bzip2" "BZIP2")
log_package("PhysFS" "PHYSFS")
log_package("StackTrace" "STACKTRACE")
log_package("X11" "X11")

if(WITH_RENDERER STREQUAL "OpenGL" AND OPENGL_FOUND)
	message("Renderer: OpenGL (Disable by param -DWITH_RENDERER=NativeSDL)")
elseif(WITH_RENDERER STREQUAL "OpenGLES" AND OPENGLES_FOUND AND SDLGLES_FOUND)
	message("Renderer: OpenGLES (Disable by param -DWITH_RENDERER=NativeSDL)")
else()
	message("Renderer: NativeSDL (Enable OpenGL by param -DWITH_RENDERER=OpenGL)")
endif()

message("==================================")

# Compile Stratagus

add_custom_command(OUTPUT tolua.cpp
	COMMAND ${TOLUA++_APP} ARGS -L stratagus.lua -o ${CMAKE_CURRENT_BINARY_DIR}/tolua.cpp stratagus.pkg
	DEPENDS ${tolua_FILES}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/tolua
	VERBATIM
)

add_library(stratagus STATIC ${stratagus_SRCS})
target_link_libraries(stratagus ${stratagus_LIBS})
if(stratagus_LIBS_RELEASE)
	target_link_libraries(stratagus ${stratagus_LIBS_RELEASE})
endif()
if(stratagus_LIBS_DEBUG)
	target_link_libraries(stratagus ${stratagus_LIBS_DEBUG})
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set_target_properties(stratagus PROPERTIES OUTPUT_NAME stratagus-dbg)
endif()

if(WIN32 AND MINGW AND ENABLE_STATIC)
	set_target_properties(stratagus PROPERTIES LINK_FLAGS "${LINK_FLAGS} -static-libgcc -static-libstdc++ -Wl,--stack,10485760")
endif()

if(APPLE)
	set_target_properties(stratagus PROPERTIES LINK_FLAGS "${LINK_FLAGS} -lSDLmain")
endif()

########### next target ###############

set(metaserver_SRCS
	metaserver/cmd.cpp
	metaserver/db.cpp
	metaserver/games.cpp
	metaserver/main.cpp
	metaserver/netdriver.cpp
	src/network/net_lowlevel.cpp
)

set(metaserver_HDRS
	metaserver/cmd.h
	metaserver/db.h
	metaserver/games.h
	metaserver/netdriver.h
)

source_group(metaserver FILES ${metaserver_SRCS} ${metaserver_HDRS})

if(NOT SQLITE_FOUND)
	set(ENABLE_METASERVER OFF)
endif()

if(ENABLE_METASERVER)
	add_executable(metaserver ${metaserver_SRCS} ${metaserver_HDRS})
	target_link_libraries(metaserver ${SDL_LIBRARY} ${SQLITE_LIBRARIES})
	
	if(WIN32)
		target_link_libraries(metaserver winmm ws2_32)
	endif()
	
	if(WIN32 AND MINGW)
		target_link_libraries(metaserver dxguid)
	endif()

	if(WIN32 AND MINGW AND ENABLE_STATIC)
		set_target_properties(metaserver PROPERTIES LINK_FLAGS "${LINK_FLAGS} -static-libgcc -static-libstdc++")
	endif()
endif()

########### next target ###############

set(gameheaders_HDRS
	gameheaders/stratagus-game-installer.nsi
	gameheaders/stratagus-game-launcher.h
)
source_group(gameheaders FILES ${gameheaders_HDRS})

########### install files ###############

install(TARGETS stratagus DESTINATION ${GAMEDIR})

if(ENABLE_METASERVER)
	install(TARGETS metaserver DESTINATION ${SBINDIR})
endif()

########### clean files ###############

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CLEAN_FILES}")
