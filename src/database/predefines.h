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
//      (c) Copyright 2020-2022 by Andrettin
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

#include "util/singleton.h"

namespace wyrmgus {

class sml_data;
class sml_property;

class predefines final : public QObject, public singleton<predefines>
{
	Q_OBJECT

	Q_PROPERTY(bool documents_modules_loading_enabled MEMBER documents_modules_loading_enabled READ is_documents_modules_loading_enabled)

public:
	void load(const std::filesystem::path &base_path);
	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);

	bool is_documents_modules_loading_enabled() const
	{
		return this->documents_modules_loading_enabled;
	}

private:
	bool documents_modules_loading_enabled = true;
};

}
