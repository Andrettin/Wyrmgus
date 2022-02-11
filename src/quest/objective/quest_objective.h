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
//      (c) Copyright 2015-2022 by Andrettin
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

class CPlayer;
class CUnit;
class CUpgrade;
class PlayerAi;
struct lua_State;

static int CclDefineQuest(lua_State *l);

namespace wyrmgus {

class faction;
class gsml_data;
class gsml_property;
class player_quest_objective;
class quest;
class resource;
class site;
class unit_class;
class unit_type;
enum class objective_type;

class quest_objective
{
public:
	static std::unique_ptr<quest_objective> try_from_identifier(const std::string &identifier, const quest *quest);

	static std::unique_ptr<quest_objective> from_identifier(const std::string &identifier, const quest *quest)
	{
		std::unique_ptr<quest_objective> objective = quest_objective::try_from_identifier(identifier, quest);
		if (objective == nullptr) {
			throw std::runtime_error("Invalid quest objective type: \"" + identifier + "\".");
		}
		return objective;
	}

	static std::unique_ptr<quest_objective> from_gsml_property(const gsml_property &property, const quest *quest);
	static std::unique_ptr<quest_objective> from_gsml_scope(const gsml_data &scope, const quest *quest);

protected:
	explicit quest_objective(const wyrmgus::quest *quest);

public:
	virtual ~quest_objective()
	{
	}

	virtual void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	virtual void check() const
	{
	}

	virtual objective_type get_objective_type() const = 0;

	const wyrmgus::quest *get_quest() const
	{
		return this->quest;
	}

	int get_index() const
	{
		return this->index;
	}

	virtual int get_quantity() const
	{
		return this->quantity;
	}

	const std::string &get_objective_string() const
	{
		return this->objective_string;
	}

	virtual std::string generate_objective_string(const CPlayer *player) const
	{
		Q_UNUSED(player)

		return std::string();
	}

	std::string get_unit_name_objective_string(const std::string &unit_name, const unit_type *unit_type, bool &first) const;
	std::string get_unit_class_objective_string(const unit_class *unit_class, bool &first) const;
	std::string get_unit_type_objective_string(const unit_type *unit_type, const CPlayer *player, bool &first) const;

	//check whether the objective's quest can be accepted
	virtual bool is_quest_acceptance_allowed(const CPlayer *player) const
	{
		Q_UNUSED(player)

		return true;
	}

	bool overlaps_with(const quest_objective *other_objective) const;

	virtual std::pair<bool, std::string> check_failure(const CPlayer *player) const
	{
		Q_UNUSED(player)

		return std::make_pair(false, std::string());
	}

	virtual void update_counter(player_quest_objective *player_quest_objective) const
	{
		Q_UNUSED(player_quest_objective)
	}

	virtual void on_unit_built(const CUnit *unit, player_quest_objective *player_quest_objective) const
	{
		Q_UNUSED(unit)
		Q_UNUSED(player_quest_objective)
	}

	virtual void on_unit_destroyed(const CUnit *unit, player_quest_objective *player_quest_objective) const
	{
		Q_UNUSED(unit)
		Q_UNUSED(player_quest_objective)
	}

	virtual void on_resource_gathered(const resource *resource, const int quantity, player_quest_objective *player_quest_objective) const
	{
		Q_UNUSED(resource)
		Q_UNUSED(quantity)
		Q_UNUSED(player_quest_objective)
	}

	virtual void check_ai(PlayerAi *ai_player, const player_quest_objective *player_quest_objective) const
	{
		Q_UNUSED(ai_player)
		Q_UNUSED(player_quest_objective)
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

	const wyrmgus::faction *get_faction() const
	{
		return this->faction;
	}

private:
	const wyrmgus::quest *quest = nullptr;
	int index = -1;
	int quantity = 1;
	std::string objective_string;
	std::vector<const unit_class *> unit_classes;
	std::vector<const unit_type *> unit_types;
	const CUpgrade *upgrade = nullptr;
	const wyrmgus::faction *faction = nullptr;

	friend int ::CclDefineQuest(lua_State *l);
};

}
