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

#include "stratagus.h"

#include "map/map_presets.h"

#include "map/map_settings.h"

namespace wyrmgus {

map_presets::map_presets(const std::string &identifier) : data_entry(identifier)
{
	this->settings = make_qunique<map_settings>();

	if (QApplication::instance()->thread() != QThread::currentThread()) {
		this->settings->moveToThread(QApplication::instance()->thread());
	}
}

map_presets::~map_presets()
{
}

void map_presets::process_gsml_property(const gsml_property &property)
{
	this->settings->process_gsml_property(property);
}

void map_presets::process_gsml_scope(const gsml_data &scope)
{
	this->settings->process_gsml_scope(scope);
}

std::string map_presets::get_text() const
{
	return this->settings->get_string();
}

}
