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

if (GODOT_DIR)
	set(GODOT_FOUND "YES")
endif()

if (GODOT_FOUND) 
	message(STATUS "Found Godot: ${GODOT_DIR}")
else (GODOT_FOUND)
	message(FATAL_ERROR "Could not find Godot")
endif (GODOT_FOUND)
