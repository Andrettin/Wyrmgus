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

class CPlayer;
class CUpgrade;
struct lua_State;

static int CclDefineQuest(lua_State *l);

namespace wyrmgus {

class character;
class faction;
class quest;
class resource;
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
	void check() const;

	objective_type get_objective_type() const
	{
		return this->objective_type;
	}

	const quest *get_quest() const
	{
		return this->quest;
	}

	int get_quantity() const
	{
		return this->quantity;
	}

	const std::string &get_objective_string() const
	{
		return this->objective_string;
	}

	std::string generate_objective_string(const CPlayer *player) const;
	std::string get_unit_type_objective_string(const unit_type *unit_type, const CPlayer *player, bool &first) const;

	const resource *get_resource() const
	{
		return this->resource;
	}

	const std::vector<const unit_class *> &get_unit_classes() const
	{
		return this->unit_classes;
	}

	const std::vector<const unit_type *> &get_unit_types() const
	{
		return this->unit_types;
	}

	const CUpgrade *get_upgrade() const
	{
		return this->upgrade;
	}

	const character *get_character() const
	{
		return this->character;
	}

	const unique_item *get_unique() const
	{
		return this->unique;
	}

	const site *get_settlement() const
	{
		return this->settlement;
	}

	const faction *get_faction() const
	{
		return this->faction;
	}

private:
	objective_type objective_type;
	const wyrmgus::quest *quest = nullptr;
	int quantity = 0;
	std::string objective_string;
	const wyrmgus::resource *resource = nullptr;
	std::vector<const unit_class *> unit_classes;
	std::vector<const unit_type *> unit_types;
	const CUpgrade *upgrade = nullptr;
	const wyrmgus::character *character = nullptr;
	const unique_item *unique = nullptr;
	const site *settlement = nullptr;
	const wyrmgus::faction *faction = nullptr;

	friend static int ::CclDefineQuest(lua_State *l);
};

class player_quest_objective final
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
