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
//      (c) Copyright 2016-2021 by Andrettin
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
#include "data_type.h"

namespace wyrmgus {

class magic_domain;
class season_schedule;
class species;
class time_of_day_schedule;

class plane final : public detailed_data_entry, public data_type<plane>, public CDataType
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "plane";
	static constexpr const char *database_folder = "planes";

	explicit plane(const std::string &identifier) : detailed_data_entry(identifier), CDataType(identifier)
	{
	}

	virtual void ProcessConfigData(const CConfigData *config_data) override;

	const wyrmgus::time_of_day_schedule *get_time_of_day_schedule() const
	{
		return this->time_of_day_schedule;
	}

	const wyrmgus::season_schedule *get_season_schedule() const
	{
		return this->season_schedule;
	}

	const std::vector<const species *> &get_native_species() const
	{
		return this->native_species;
	}

	void add_native_species(const species *species)
	{
		this->native_species.push_back(species);
	}

	std::vector<const species *> get_native_sapient_species() const;
	std::vector<const species *> get_native_fauna_species() const;

private:
	wyrmgus::time_of_day_schedule *time_of_day_schedule = nullptr;
	wyrmgus::season_schedule *season_schedule = nullptr;
public:
	std::vector<magic_domain *> EmpoweredMagicDomains; //magic domains empowered in this plane
private:
	std::vector<const species *> native_species; //species in this plane
};

}