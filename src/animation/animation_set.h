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
//      (c) Copyright 2005-2022 by Jimmy Salmon and Andrettin
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

#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"
#include "economy/resource_container.h"

class CAnimation;
class CFile;
class CUnit;
struct lua_State;

extern int CclDefineAnimations(lua_State *l);

constexpr int ANIMATIONS_DEATHTYPES = 40;

/**
**  Default names for the extra death types.
*/
extern std::string ExtraDeathTypes[ANIMATIONS_DEATHTYPES];

namespace wyrmgus {

class animation_sequence;

class animation_set final : public data_entry, public data_type<animation_set>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "animation_set";
	static constexpr const char *database_folder = "animation_sets";

	static const std::set<std::string> database_dependencies;

	static void clear();

	explicit animation_set(const std::string &identifier) : data_entry(identifier)
	{
		memset(this->Death, 0, sizeof(this->Death));
	}

	static void SaveUnitAnim(CFile &file, const CUnit &unit);
	static void LoadUnitAnim(lua_State *l, CUnit &unit, int luaIndex);
	static void LoadWaitUnitAnim(lua_State *l, CUnit &unit, int luaIndex);

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;

	void set_animations(const std::string &animation_type, const animation_sequence *animation_sequence);

	const resource_map<const CAnimation *> &get_harvest_animations() const
	{
		return this->harvest_animations;
	}

	const CAnimation *get_harvest_animation(const resource *resource) const
	{
		const auto find_iterator = this->harvest_animations.find(resource);

		if (find_iterator != this->harvest_animations.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}
	
public:
	const CAnimation *Attack = nullptr;
	const CAnimation *RangedAttack = nullptr;
	const CAnimation *Build = nullptr;
	const CAnimation *Death[ANIMATIONS_DEATHTYPES + 1];
private:
	resource_map<const CAnimation *> harvest_animations;
public:
	const CAnimation *Move = nullptr;
	const CAnimation *Repair = nullptr;
	const CAnimation *Research = nullptr;
	const CAnimation *SpellCast = nullptr;
	const CAnimation *Start = nullptr;
	const CAnimation *Still = nullptr;
	const CAnimation *Train = nullptr;
	const CAnimation *Upgrade = nullptr;

	friend int ::CclDefineAnimations(lua_State *l);
};

}
