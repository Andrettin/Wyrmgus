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

#include "database/data_entry.h"
#include "database/data_type.h"
#include "util/qunique_ptr.h"

namespace wyrmgus {

class map_settings;

class map_presets final : public data_entry, public data_type<map_presets>
{
	Q_OBJECT

	Q_PROPERTY(QString description READ get_description_qstring CONSTANT)

public:
	static constexpr const char *class_identifier = "map_presets";
	static constexpr const char *database_folder = "map_presets";

	explicit map_presets(const std::string &identifier);
	virtual ~map_presets() override;

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	const map_settings *get_settings() const
	{
		return this->settings.get();
	}

	std::string get_description() const;

	QString get_description_qstring() const
	{
		return QString::fromStdString(this->get_description());
	}

private:
	qunique_ptr<map_settings> settings;
};

}
