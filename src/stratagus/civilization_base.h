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
//      (c) Copyright 2020-2021 by Andrettin
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

#include "database/detailed_data_entry.h"
#include "unit/unit_class_container.h"

class CUpgrade;

namespace wyrmgus {

class civilization_group;
class civilization_history;
class faction;
class name_generator;
class player_color;
class species;
class unit_class;
class unit_type;
class upgrade_class;
enum class gender;

class civilization_base : public detailed_data_entry
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::species* species MEMBER species)
	Q_PROPERTY(wyrmgus::civilization_group* group MEMBER group)
	Q_PROPERTY(wyrmgus::player_color* default_color MEMBER default_color NOTIFY changed)

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

	const wyrmgus::species *get_species() const
	{
		return this->species;
	}

protected:
	void set_species(wyrmgus::species *species)
	{
		this->species = species;
	}

public:
	const civilization_group *get_group() const
	{
		return this->group;
	}

protected:
	void set_group(civilization_group *group)
	{
		this->group = group;
	}

public:
	bool is_part_of_group(const civilization_group *group) const;

	const player_color *get_default_color() const
	{
		return this->default_color;
	}

	unit_type *get_class_unit_type(const unit_class *unit_class) const;

	void set_class_unit_type(const unit_class *unit_class, unit_type *unit_type)
	{
		if (unit_type == nullptr) {
			this->class_unit_types.erase(unit_class);
			return;
		}

		this->class_unit_types[unit_class] = unit_type;
	}

	void remove_class_unit_type(unit_type *unit_type)
	{
		for (unit_class_map<wyrmgus::unit_type *>::reverse_iterator iterator = this->class_unit_types.rbegin(); iterator != this->class_unit_types.rend(); ++iterator) {
			if (iterator->second == unit_type) {
				this->class_unit_types.erase(iterator->first);
			}
		}
	}

	CUpgrade *get_class_upgrade(const upgrade_class *upgrade_class) const;

	void set_class_upgrade(const upgrade_class *upgrade_class, CUpgrade *upgrade)
	{
		if (upgrade == nullptr) {
			this->class_upgrades.erase(upgrade_class);
			return;
		}

		this->class_upgrades[upgrade_class] = upgrade;
	}

	void remove_class_upgrade(CUpgrade *upgrade)
	{
		for (std::map<const upgrade_class *, CUpgrade *>::reverse_iterator iterator = this->class_upgrades.rbegin(); iterator != this->class_upgrades.rend(); ++iterator) {
			if (iterator->second == upgrade) {
				this->class_upgrades.erase(iterator->first);
			}
		}
	}

	const name_generator *get_personal_name_generator(const gender gender) const;
	void add_personal_name(const gender gender, const std::string &name);

	const name_generator *get_surname_generator() const;
	void add_surname(const std::string &surname);

	const name_generator *get_unit_class_name_generator(const unit_class *unit_class) const;
	void add_unit_class_name(const unit_class *unit_class, const std::string &name);

	void add_ship_name(const std::string &ship_name);

	void add_names_from(const civilization_base *other);
	void add_names_from(const faction *faction);

signals:
	void changed();

private:
	wyrmgus::species *species = nullptr;
	civilization_group *group = nullptr;
	player_color *default_color = nullptr; //the civilization's default player color (used for the encyclopedia, tech tree, etc.)
	unit_class_map<unit_type *> class_unit_types; //the unit type slot of a particular class for the civilization
	std::map<const upgrade_class *, CUpgrade *> class_upgrades; //the upgrade slot of a particular class for the civilization
	std::map<gender, std::unique_ptr<name_generator>> personal_name_generators; //personal name generators for the civilization, mapped to the gender they pertain to (use gender::none for names which should be available for both genders)
	std::unique_ptr<name_generator> surname_generator;
	unit_class_map<std::unique_ptr<name_generator>> unit_class_name_generators; //unit class names for the civilization, mapped to the unit class they pertain to, used for mechanical units, and buildings
	std::unique_ptr<name_generator> ship_name_generator;
	std::unique_ptr<civilization_history> history;
};

}
