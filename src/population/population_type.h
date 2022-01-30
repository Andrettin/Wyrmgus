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
#include "database/detailed_data_entry.h"
#include "economy/resource_container.h"

namespace wyrmgus {

class civilization;
class civilization_group;
class icon;
class population_class;

class population_type final : public detailed_data_entry, public data_type<population_type>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::population_class* population_class MEMBER population_class)
	Q_PROPERTY(wyrmgus::civilization_group* civilization_group MEMBER civilization_group)
	Q_PROPERTY(wyrmgus::civilization* civilization MEMBER civilization)
	Q_PROPERTY(wyrmgus::icon* icon MEMBER icon NOTIFY changed)

public:
	static constexpr const char *class_identifier = "population_type";
	static constexpr const char *database_folder = "population_types";

	explicit population_type(const std::string &identifier) : detailed_data_entry(identifier)
	{
	}

	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;

	virtual void check() const override
	{
		if (this->get_population_class() == nullptr) {
			throw std::runtime_error("Population type \"" + this->get_identifier() + "\" has no population class.");
		}

		if (this->get_icon() == nullptr) {
			throw std::runtime_error("Population type \"" + this->get_identifier() + "\" has no icon.");
		}
	}

	const wyrmgus::population_class *get_population_class() const
	{
		return this->population_class;
	}

	const wyrmgus::icon *get_icon() const
	{
		return this->icon;
	}

	bool is_growable() const;

	int get_production_efficiency(const resource *resource) const;

signals:
	void changed();

private:
	wyrmgus::population_class *population_class = nullptr;
	wyrmgus::civilization_group *civilization_group = nullptr;
	wyrmgus::civilization *civilization = nullptr;
	wyrmgus::icon *icon = nullptr;
	resource_map<int> production_efficiency_map;
};

}
