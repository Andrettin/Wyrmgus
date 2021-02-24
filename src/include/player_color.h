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
//      (c) Copyright 2019-2021 by Andrettin
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

class player_color final : public named_data_entry, public data_type<player_color>
{
	Q_OBJECT

	Q_PROPERTY(bool hidden MEMBER hidden READ is_hidden)
	Q_PROPERTY(QVariantList colors READ get_colors_qvariant_list)

public:
	static constexpr const char *class_identifier = "player_color";
	static constexpr const char *database_folder = "player_colors";

	explicit player_color(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	bool is_hidden() const
	{
		return this->hidden;
	}

	const std::vector<QColor> &get_colors() const
	{
		return this->colors;
	}

	QVariantList get_colors_qvariant_list() const;

	Q_INVOKABLE void add_color(const QColor &color)
	{
		this->colors.push_back(color);
	}

private:
	bool hidden = false;
	std::vector<QColor> colors; //the color shades of the player color
};

}
