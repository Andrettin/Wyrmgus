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

#include "unit/unit_image_provider.h"

#include "database/defines.h"
#include "engine_interface.h"
#include "player_color.h"
#include "unit/unit_type.h"
#include "unit/unit_type_variation.h"
#include "util/exception_util.h"
#include "util/image_util.h"
#include "util/string_util.h"
#include "video/video.h"

namespace wyrmgus {

QImage unit_image_provider::requestImage(const QString &id, QSize *size, const QSize &requested_size)
{
	try {
		Q_UNUSED(requested_size)

		const std::string id_str = id.toStdString();
		const std::vector<std::string> id_list = string::split(id_str, '/');

		size_t index = 0;
		const std::string &type_identifier = id_list.at(index);
		const unit_type *unit_type = unit_type::get(type_identifier);
		++index;

		const unit_type_variation *variation = nullptr;
		if ((index + 1) < id_list.size()) {
			const std::string &variation_identifier = id_list.at(index);
			variation = unit_type->GetVariation(variation_identifier);
			if (variation != nullptr) {
				++index;
			}
		}

		const player_color *player_color = nullptr;
		if ((index + 1) < id_list.size()) {
			const std::string &player_color_identifier = id_list.at(index);
			player_color = player_color::try_get(player_color_identifier);
			if (player_color != nullptr) {
				++index;
			}
		}

		const std::string &frame_str = id_list.back();
		const int frame_index = std::stoi(frame_str);

		std::shared_ptr<CPlayerColorGraphic> graphics;

		if (variation != nullptr) {
			graphics = variation->Sprite;
		} else {
			graphics = unit_type->Sprite;
		}

		graphics->get_load_mutex().lock();

		if (!graphics->IsLoaded()) {
			graphics->get_load_mutex().unlock();

			engine_interface::get()->sync([&graphics]() {
				//this has to run in the main Wyrmgus thread, as it performs OpenGL calls
				graphics->Load(false, defines::get()->get_scale_factor());
			});
		} else {
			graphics->get_load_mutex().unlock();
		}

		const QImage &image = graphics->get_or_create_scaled_frame(frame_index, player_color);

		if (image.isNull()) {
			throw std::runtime_error("Unit image for ID \"" + id_str + "\" is null.");
		}

		if (size != nullptr) {
			*size = image.size();
		}

		return image;
	} catch (const std::exception &exception) {
		exception::report(exception);
		return QImage();
	}
}

}
