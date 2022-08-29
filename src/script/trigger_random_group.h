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
//      (c) Copyright 2022 by Andrettin
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

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace wyrmgus {

class trigger;
enum class trigger_type;

class trigger_random_group final : public named_data_entry, public data_type<trigger_random_group>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::trigger_type type MEMBER type READ get_type)

public:
	static const std::vector<const trigger_random_group *> &get_all_of_type(const trigger_type type)
	{
		static std::vector<const trigger_random_group *> empty_vector;

		const auto find_iterator = trigger_random_group::groups_by_type.find(type);
		if (find_iterator != trigger_random_group::groups_by_type.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

private:
	static inline std::map<trigger_type, std::vector<const trigger_random_group *>> groups_by_type;

public:
	static constexpr const char *class_identifier = "trigger_random_group";
	static constexpr const char *database_folder = "trigger_random_groups";

	explicit trigger_random_group(const std::string &identifier);

	virtual void initialize() override;
	virtual void check() const override;

	trigger_type get_type() const
	{
		return this->type;
	}

	void set_type(const trigger_type type)
	{
		this->type = type;
	}

	void add_trigger(const trigger *trigger)
	{
		this->triggers.push_back(trigger);
	}

	const std::vector<const trigger *> &get_active_triggers() const
	{
		return this->active_triggers;
	}

	void add_active_trigger(const trigger *trigger)
	{
		this->active_triggers.push_back(trigger);
	}

	void remove_active_trigger(const trigger *trigger)
	{
		std::erase(this->active_triggers, trigger);
	}

	void clear_active_triggers()
	{
		this->active_triggers.clear();
	}

private:
	trigger_type type;
	std::vector<const trigger *> triggers;
	std::vector<const trigger *> active_triggers;
};

}
