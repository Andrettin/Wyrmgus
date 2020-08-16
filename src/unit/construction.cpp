//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
//      (c) Copyright 1998-2020 by Lutz Sammer, Jimmy Salmon and Andrettin
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//

#include "stratagus.h"

#include "unit/construction.h"

#include "database/defines.h"
#include "script.h"
#include "translate.h"
#include "ui/ui.h"
#include "video/video.h"

namespace wyrmgus {

construction::~construction()
{
	CGraphic::Free(this->graphics);
}

void construction::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "image_file") {
		this->image_file = database::get_graphics_path(this->get_module()) / value;
	} else {
		data_entry::process_sml_property(property);
	}
}

void construction::load()
{
	if (!this->image_file.empty()) {
		UpdateLoadProgress();
		this->graphics = CPlayerColorGraphic::New(this->image_file, this->get_frame_size(), nullptr);
		this->graphics->Load(false, wyrmgus::defines::get()->get_scale_factor());
		IncItemsLoaded();
	}
}

}

/**
**  Return the amount of constructions.
*/
int GetConstructionsCount()
{
	int count = 0;
	for (const wyrmgus::construction *construction : wyrmgus::construction::get_all()) {
		if (!construction->get_image_file().empty()) count++;
	}
	return count;
}

/**
**  Load the graphics for the constructions.
**
**  HELPME: who make this better terrain depended and extendable
**  HELPME: filename construction.
*/
void LoadConstructions()
{
	ShowLoadProgress("%s", _("Loading Construction Graphics"));
		
	for (wyrmgus::construction *construction : wyrmgus::construction::get_all()) {
		construction->load();
	}
}

/**
**  Parse the construction.
**
**  @param l  Lua state.
**
**  @note make this more flexible
*/
static int CclDefineConstruction(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument");
	}

	// Slot identifier
	const std::string str = LuaToString(l, 1);
	wyrmgus::construction *construction = wyrmgus::construction::get_or_add(str, nullptr);

	//  Parse the arguments, in tagged format.
	lua_pushnil(l);
	while (lua_next(l, 2)) {
		const char *value = LuaToString(l, -2);
		bool files = !strcmp(value, "Files");

		if (files) {
			std::string file;
			int w = 0;
			int h = 0;

			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			lua_pushnil(l);
			while (lua_next(l, -2)) {
				const char *value = LuaToString(l, -2);

				if (!strcmp(value, "File")) {
					file = LuaToString(l, -1);
				} else if (!strcmp(value, "Size")) {
					CclGetPos(l, &w, &h);
				} else {
					LuaError(l, "Unsupported tag: %s" _C_ value);
				}
				lua_pop(l, 1);
			}

			construction->image_file = file;
			construction->frame_size = QSize(w, h);
		} else if (!strcmp(value, "Constructions")) {
			const unsigned int subargs = lua_rawlen(l, -1);

			for (unsigned int k = 0; k < subargs; ++k) {
				int percent = 0;
				wyrmgus::construction_image_type image_type = wyrmgus::construction_image_type::construction;
				int frame = 0;

				lua_rawgeti(l, -1, k + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument");
				}
				lua_pushnil(l);
				while (lua_next(l, -2)) {
					const char *value = LuaToString(l, -2);

					if (!strcmp(value, "Percent")) {
						percent = LuaToNumber(l, -1);
					} else if (!strcmp(value, "File")) {
						const char *value = LuaToString(l, -1);

						if (!strcmp(value, "construction")) {
							image_type = wyrmgus::construction_image_type::construction;
						} else if (!strcmp(value, "main")) {
							image_type = wyrmgus::construction_image_type::main;
						} else {
							LuaError(l, "Unsupported tag: %s" _C_ value);
						}
					} else if (!strcmp(value, "Frame")) {
						frame = LuaToNumber(l, -1);
					} else {
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
					lua_pop(l, 1);
				}
				lua_pop(l, 1);
				auto cframe = std::make_unique<wyrmgus::construction_frame>();
				cframe->percent = percent;
				cframe->image_type = image_type;
				cframe->frame = frame;
				cframe->next = nullptr;
				construction->frames.back()->next = cframe.get();
				construction->frames.push_back(std::move(cframe));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
		lua_pop(l, 1);
	}
	return 0;
}

// ----------------------------------------------------------------------------

/**
**  Register CCL features for construction.
*/
void ConstructionCclRegister()
{
	lua_register(Lua, "DefineConstruction", CclDefineConstruction);
}
