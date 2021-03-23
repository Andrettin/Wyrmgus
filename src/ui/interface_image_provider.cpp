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

#include "ui/interface_image_provider.h"

#include "database/defines.h"
#include "engine_interface.h"
#include "ui/interface_element_type.h"
#include "ui/interface_style.h"
#include "util/log_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"
#include "video/video.h"

namespace wyrmgus {

QImage interface_image_provider::requestImage(const QString &id, QSize *size, const QSize &requested_size)
{
	Q_UNUSED(requested_size)

	const std::string id_str = id.toStdString();
	const std::vector<std::string> id_list = string::split(id_str, '/');

	const std::string &interface_identifier = id_list.at(0);
	const interface_style *interface = interface_style::get(interface_identifier);

	const std::string &interface_element_str = id_list.at(1);
	const interface_element_type interface_element_type = string_to_interface_element_type(interface_element_str);

	std::vector<std::string> qualifiers;
	if (id_list.size() > 2) {
		qualifiers = vector::subvector(id_list, 2);
	}

	const std::shared_ptr<CGraphic> graphics = interface->get_interface_element_graphics(interface_element_type, qualifiers);

	graphics->get_load_mutex().lock();

	if (!graphics->IsLoaded()) {
		graphics->get_load_mutex().unlock();

		engine_interface::get()->sync([&graphics]() {
			//this has to run in the main Wyrmgus thread, as it performs OpenGL calls
			graphics->Load(defines::get()->get_scale_factor());
		});
	} else {
		graphics->get_load_mutex().unlock();
	}

	const QImage &image = graphics->get_or_create_frame_image(0, nullptr);

	if (image.isNull()) {
		log::log_error("Interface image for ID \"" + id_str + "\" is null.");
	}

	if (size != nullptr) {
		*size = image.size();
	}

	return image;
}

}
