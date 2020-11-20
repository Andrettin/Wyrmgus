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
//      (c) Copyright 2015-2020 by Andrettin
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

#pragma once

class CUpgrade;
struct lua_State;

static int CclDefineQuest(lua_State *l);

namespace wyrmgus {

class character;
class faction;
class quest;
class site;
class sml_data;
class sml_property;
class unique_item;
class unit_class;
class unit_type;
enum class objective_type;

class quest_objective final
{
public:
	explicit quest_objective(const objective_type objective_type, const wyrmgus::quest *quest);

	void process_sml_property(const wyrmgus::sml_property &property);
	void process_sml_scope(const wyrmgus::sml_data &scope);

	objective_type get_objective_type() const
	{
		return this->objective_type;
	}

	const wyrmgus::quest *get_quest() const
	{
		return this->quest;
	}

	int get_index() const
	{
		return this->index;
	}

	int get_quantity() const
	{
		return this->quantity;
	}

	const std::string &get_objective_string() const
	{
		return this->objective_string;
	}

	const std::vector<const wyrmgus::unit_class *> &get_unit_classes() const
	{
		return this->unit_classes;
	}

	const wyrmgus::site *get_settlement() const
	{
		return this->settlement;
	}

	const wyrmgus::faction *get_faction() const
	{
		return this->faction;
	}

	const wyrmgus::character *get_character() const
	{
		return this->character;
	}

private:
	objective_type objective_type;
	const wyrmgus::quest *quest = nullptr;
	int index = -1;
	int quantity = 1;
public:
	int Resource = -1;
private:
	std::string objective_string;
	std::vector<const wyrmgus::unit_class *> unit_classes;
public:
	std::vector<wyrmgus::unit_type *> UnitTypes;
	const CUpgrade *Upgrade = nullptr;
private:
	const wyrmgus::character *character = nullptr;
public:
	const unique_item *Unique = nullptr;
private:
	const wyrmgus::site *settlement = nullptr;
	const wyrmgus::faction *faction = nullptr;

	friend static int ::CclDefineQuest(lua_State *l);
};

class player_quest_objective
{
public:
	explicit player_quest_objective(const quest_objective *quest_objective) : quest_objective(quest_objective)
	{
	}

	const quest_objective *get_quest_objective() const
	{
		return this->quest_objective;
	}

private:
	const quest_objective *quest_objective = nullptr;
public:
	int Counter = 0;
};

}
