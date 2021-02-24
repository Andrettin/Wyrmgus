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

#include "database/data_type.h"
#include "ui/icon_base.h"

namespace wyrmgus {

class resource_icon final : public icon_base, public data_type<resource_icon>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "resource_icon";
	static constexpr const char *database_folder = "resource_icons";
	static constexpr QSize size = QSize(14, 14);

	static void load_all()
	{
		for (resource_icon *resource_icon : resource_icon::get_all()) {
			if (!resource_icon->is_loaded()) {
				resource_icon->load();
			}
		}
	}

	explicit resource_icon(const std::string &identifier) : icon_base(identifier)
	{
	}

	virtual const QSize &get_size() const override
	{
		return resource_icon::size;
	}
};

}
