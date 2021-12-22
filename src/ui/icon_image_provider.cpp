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

#include "ui/icon_image_provider.h"

#include "database/preferences.h"
#include "player/player_color.h"
#include "ui/icon.h"
#include "util/exception_util.h"
#include "util/string_util.h"
#include "video/color_modification.h"
#include "video/video.h"

namespace wyrmgus {

QImage icon_image_provider::requestImage(const QString &id, QSize *size, const QSize &requested_size)
{
	Q_UNUSED(requested_size)

	try {
		const std::string id_str = id.toStdString();
		const std::vector<std::string> id_list = string::split(id_str, '/');

		size_t index = 0;
		const std::string &icon_identifier = id_list.at(index);
		const icon *icon = icon::get(icon_identifier);

		++index;

		const player_color *player_color = nullptr;
		if (index < id_list.size()) {
			const std::string &player_color_identifier = id_list.at(index);
			player_color = player_color::get(player_color_identifier);
			++index;
		}

		bool grayscale = false;
		if (index < id_list.size()) {
			if (id_list.at(index) == "grayscale") {
				grayscale = true;
				++index;
			}
		}

		std::shared_ptr<CGraphic> graphics = icon->get_graphics();
		graphics->Load(preferences::get()->get_scale_factor());

		const QImage &image = graphics->get_or_create_frame_image(icon->get_frame(), color_modification(icon->get_hue_rotation(), icon->get_hue_ignored_colors(), player_color), grayscale);

		if (image.isNull()) {
			throw std::runtime_error("Icon image for ID \"" + id_str + "\" is null.");
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
