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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

namespace wyrmgus {

class font_color final : public data_entry, public data_type<font_color>
{
	Q_OBJECT

	Q_PROPERTY(QVariantList colors READ get_colors_qvariant_list)

public:
	static constexpr const char *class_identifier = "font_color";
	static constexpr const char *database_folder = "font_colors";
	static constexpr int max_colors = 9;

	explicit font_color(const std::string &identifier) : data_entry(identifier)
	{
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
	std::vector<QColor> colors; //the color shades of the font color
};

}
