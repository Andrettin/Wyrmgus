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
//      (c) Copyright 2018-2022 by Andrettin
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

#include "color.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "data_type.h"

namespace wyrmgus {

class resource_icon;

class time_of_day final : public named_data_entry, public data_type<time_of_day>
{
	Q_OBJECT

	Q_PROPERTY(bool dawn MEMBER dawn READ is_dawn)
	Q_PROPERTY(bool day MEMBER day READ is_day)
	Q_PROPERTY(bool dusk MEMBER dusk READ is_dusk)
	Q_PROPERTY(bool night MEMBER night READ is_night)
	Q_PROPERTY(wyrmgus::resource_icon* icon MEMBER icon NOTIFY changed)

public:
	static constexpr const char *class_identifier = "time_of_day";
	static constexpr const char *database_folder = "times_of_day";

	static time_of_day *add(const std::string &identifier, const wyrmgus::data_module *data_module)
	{
		time_of_day *time_of_day = data_type::add(identifier, data_module);
		time_of_day->ID = time_of_day::get_all().size() - 1;
		return time_of_day;
	}

	explicit time_of_day(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	virtual void check() const override
	{
		if (this->get_icon() == nullptr) {
			throw std::runtime_error("Time of day \"" + this->get_identifier() + "\" has no icon.");
		}
	}

	bool is_dawn() const
	{
		return this->dawn;
	}

	bool is_day() const
	{
		return this->day;
	}

	bool is_dusk() const
	{
		return this->dusk;
	}

	bool is_night() const
	{
		return this->night;
	}

	const resource_icon *get_icon() const
	{
		return this->icon;
	}

	bool HasColorModification() const;

signals:
	void changed();

public:
	int ID = -1;								/// The ID of this time of day
private:
	bool dawn = false;							/// Whether this is a dawn time of day
	bool day = false;							/// Whether this is a day time of day
	bool dusk = false;							/// Whether this is a dusk time of day
	bool night = false;							/// Whether this is a night time of day
	resource_icon *icon = nullptr;
public:
	CColor ColorModification;					/// The color modification applied to graphics when the time of day is active
};

}