#.rst:
# FindGodot
# -------
#
# Find the Godot includes and library
#
# It defines the following variables
#
# ``GODOT_DIR``
#   where to find Godot
# ``GODOT_LIBRARIES_STATIC``
#   the libraries to link against to use Godot.
# ``GODOT_LIBRARIES_STATIC_DEBUG``
#   the debug libraries to link against to use Godot.
# ``GODOT_FOUND``
#   If true, successfully found Godot.
#

set(GODOT_FOUND "NO")

find_path(GODOT_DIR modules/register_module_types.h)
find_library(GODOT_DRIVERS_LIBRARY_STATIC NAMES drivers.windows.opt.32 drivers.x11.opt.32)
find_library(GODOT_DRIVERS_LIBRARY_STATIC_DEBUG NAMES drivers.windows.debug.32 drivers.x11.debug.32)
find_library(GODOT_CORE_LIBRARY_STATIC NAMES core.windows.opt.32 core.x11.opt.32)
find_library(GODOT_CORE_LIBRARY_STATIC_DEBUG NAMES core.windows.debug.32 core.x11.debug.32)
find_library(GODOT_MODULES_LIBRARY_STATIC NAMES modules.windows.opt.32 modules.x11.opt.32)
find_library(GODOT_MODULES_LIBRARY_STATIC_DEBUG NAMES modules.windows.debug.32 modules.x11.debug.32)
find_library(GODOT_SCENE_LIBRARY_STATIC NAMES scene.windows.opt.32 scene.x11.opt.32)
find_library(GODOT_SCENE_LIBRARY_STATIC_DEBUG NAMES scene.windows.debug.32 scene.x11.debug.32)

if (GODOT_DIR AND GODOT_DRIVERS_LIBRARY_STATIC AND GODOT_CORE_LIBRARY_STATIC AND GODOT_MODULES_LIBRARY_STATIC AND GODOT_SCENE_LIBRARY_STATIC)
	set(GODOT_LIBRARIES_STATIC optimized ${GODOT_DRIVERS_LIBRARY_STATIC} optimized ${GODOT_CORE_LIBRARY_STATIC} optimized ${GODOT_MODULES_LIBRARY_STATIC} optimized ${GODOT_SCENE_LIBRARY_STATIC})
	set(GODOT_FOUND "YES")
endif()

if (GODOT_DIR AND GODOT_DRIVERS_LIBRARY_STATIC_DEBUG AND GODOT_CORE_LIBRARY_STATIC_DEBUG AND GODOT_MODULES_LIBRARY_STATIC_DEBUG AND GODOT_SCENE_LIBRARY_STATIC_DEBUG)
	set(GODOT_LIBRARIES_STATIC_DEBUG debug ${GODOT_DRIVERS_LIBRARY_STATIC_DEBUG} debug ${GODOT_CORE_LIBRARY_STATIC_DEBUG} debug ${GODOT_MODULES_LIBRARY_STATIC_DEBUG} debug ${GODOT_SCENE_LIBRARY_STATIC_DEBUG})
	set(GODOT_FOUND "YES")
endif()

if (GODOT_FOUND) 
	message(STATUS "Found Godot: ${GODOT_DIR}")
else (GODOT_FOUND)
	message(FATAL_ERROR "Could not find Godot")
endif (GODOT_FOUND)
