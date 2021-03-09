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
//      (c) Copyright 2021 by Andrettin
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

#include "map/tile_image_provider.h"

#include "database/defines.h"
#include "engine_interface.h"
#include "map/terrain_type.h"
#include "time/season.h"
#include "util/string_util.h"
#include "video/video.h"

namespace wyrmgus {

QImage tile_image_provider::requestImage(const QString &id, QSize *size, const QSize &requested_size)
{
	Q_UNUSED(requested_size)

	const std::string id_str = id.toStdString();
	const std::vector<std::string> id_list = string::split(id_str, '/');

	const std::string &terrain_identifier = id_list.at(0);
	const terrain_type *terrain = terrain_type::get(terrain_identifier);

	const season *season = nullptr;
	if (id_list.size() > 1) {
		const std::string &season_identifier = id_list.at(1);
		season = season::get(season_identifier);
	}

	const std::shared_ptr<CGraphic> graphics = terrain->get_graphics(season);

	engine_interface::get()->sync([&graphics]() {
		//this has to run in the main Wyrmgus thread, as it performs OpenGL calls
		graphics->set_store_scaled_image(true);
		graphics->Load(false, defines::get()->get_scale_factor());
	});

	const QImage &image = graphics->get_scaled_image();

	if (size != nullptr) {
		*size = image.size();
	}

	return image;
}

}
