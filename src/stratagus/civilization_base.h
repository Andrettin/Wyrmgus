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
//      (c) Copyright 2020 by Andrettin
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

#include "database/detailed_data_entry.h"
#include "unit/unit_class_container.h"

namespace wyrmgus {

class civilization_group;
class civilization_history;
class species;
class unit_class;
enum class gender;

class civilization_base : public detailed_data_entry
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::species* species MEMBER species)
	Q_PROPERTY(wyrmgus::civilization_group* group MEMBER group)

protected:
	explicit civilization_base(const std::string &identifier);
	virtual ~civilization_base() override;

public:
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;
	virtual data_entry_history *get_history_base() override;

	const civilization_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	const species *get_species() const
	{
		return this->species;
	}

	const civilization_group *get_group() const
	{
		return this->group;
	}

	const std::vector<std::string> &get_personal_names(const gender gender) const;
	void add_personal_name(const gender gender, const std::string &name);

	const std::vector<std::string> &get_surnames() const;
	void add_surname(const std::string &surname);

	const std::vector<std::string> &get_unit_class_names(const unit_class *unit_class) const;
	void add_unit_class_name(const unit_class *unit_class, const std::string &name);

	const std::vector<std::string> &get_ship_names() const;
	void add_ship_name(const std::string &ship_name);

	void add_names_from(const civilization_base *other);

protected:
	void set_species(species *species)
	{
		this->species = species;
	}

	void set_group(civilization_group *group)
	{
		this->group = group;
	}

private:
	species *species = nullptr;
	civilization_group *group = nullptr;
	std::map<gender, std::vector<std::string>> personal_names; //personal names for the civilization, mapped to the gender they pertain to (use gender::none for names which should be available for both genders)
	std::vector<std::string> surnames; //surnames for the civilization
	unit_class_map<std::vector<std::string>> unit_class_names; //unit class names for the civilization, mapped to the unit class they pertain to, used for mechanical units, and buildings
	std::vector<std::string> ship_names;
	std::unique_ptr<civilization_history> history;
};

}
