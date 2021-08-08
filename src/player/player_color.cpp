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

#include "stratagus.h"

#include "player_color.h"

#include "database/defines.h"
#include "util/container_util.h"

namespace wyrmgus {

void player_color::check() const
{
	const player_color *conversible_color = defines::get()->get_conversible_player_color();
	if (this->get_colors().size() != conversible_color->get_colors().size()) {
		throw std::runtime_error("The \"" + this->get_identifier() + "\" player color has a different amount of shades (" + std::to_string(this->get_colors().size()) + ") than the amount of shades (" + std::to_string(conversible_color->get_colors().size()) + ") for the conversible player color (\"" + conversible_color->get_identifier() + "\").");
	}
}

QVariantList player_color::get_colors_qvariant_list() const
{
	return container::to_qvariant_list(this->get_colors());
}

}