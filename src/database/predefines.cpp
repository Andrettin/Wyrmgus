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
//

#include "stratagus.h"

#include "database/predefines.h"

#include "database/database.h"
#include "database/sml_data.h"
#include "database/sml_parser.h"

namespace wyrmgus {

void predefines::load(const std::filesystem::path &data_path)
{
	std::filesystem::path predefines_path(data_path / "predefines.txt");

	if (!std::filesystem::exists(predefines_path)) {
		return;
	}

	sml_parser parser(predefines_path);
	const sml_data data = parser.parse();
	database::process_sml_data(this, data);
}

void predefines::process_sml_property(const sml_property &property)
{
	database::process_sml_property_for_object(this, property);
}

void predefines::process_sml_scope(const sml_data &scope)
{
	database::process_sml_scope_for_object(this, scope);
}

}
