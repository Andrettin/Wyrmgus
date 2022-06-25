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

#include "economy/resource_container.h"

namespace wyrmgus {

enum class diplomacy_state;

class player_results_info final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring CONSTANT)
	Q_PROPERTY(bool this_player READ is_this_player CONSTANT)
	Q_PROPERTY(bool ally READ is_ally CONSTANT)
	Q_PROPERTY(bool enemy READ is_enemy CONSTANT)
	Q_PROPERTY(int unit_count MEMBER unit_count CONSTANT)
	Q_PROPERTY(int building_count MEMBER building_count CONSTANT)
	Q_PROPERTY(int kill_count MEMBER kill_count CONSTANT)
	Q_PROPERTY(int razing_count MEMBER razing_count CONSTANT)

public:
	explicit player_results_info(const std::string &name, const std::optional<diplomacy_state> &diplomacy_state, const int unit_count, const int building_count, const resource_map<int> &resource_counts, const int kill_count, const int razing_count)
		: name(name), diplomacy_state(diplomacy_state), unit_count(unit_count), building_count(building_count), resource_counts(resource_counts), kill_count(kill_count), razing_count(razing_count)
	{
	}

	QString get_name_qstring() const
	{
		return QString::fromStdString(this->name);
	}

	bool is_this_player() const
	{
		return !this->diplomacy_state.has_value();
	}

	bool is_ally() const;
	bool is_enemy() const;

	Q_INVOKABLE int get_resource_count(const QString &resource_identifier) const;

private:
	std::string name;
	std::optional<wyrmgus::diplomacy_state> diplomacy_state;
	int unit_count = 0;
	int building_count = 0;
	resource_map<int> resource_counts;
	int kill_count = 0;
	int razing_count = 0;
};

}
