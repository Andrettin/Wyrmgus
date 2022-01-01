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
//      (c) Copyright 2021-2022 by Andrettin
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

#include "database/basic_data_entry.h"

class CGraphic;

namespace wyrmgus {

class interface_style;
enum class button_state;

class button_style final : public basic_data_entry
{
	Q_OBJECT

	Q_PROPERTY(std::filesystem::path normal_file MEMBER normal_file WRITE set_normal_file)
	Q_PROPERTY(std::filesystem::path pressed_file MEMBER pressed_file WRITE set_pressed_file)
	Q_PROPERTY(std::filesystem::path grayed_file MEMBER grayed_file WRITE set_grayed_file)

public:
	explicit button_style(const interface_style *interface) : interface(interface)
	{
	}

	void initialize();

	void set_normal_file(const std::filesystem::path &filepath);
	void set_pressed_file(const std::filesystem::path &filepath);
	void set_grayed_file(const std::filesystem::path &filepath);

	const std::shared_ptr<CGraphic> &get_graphics(const button_state state) const;

private:
	const interface_style *interface = nullptr;
	std::filesystem::path normal_file;
	std::filesystem::path pressed_file;
	std::filesystem::path grayed_file;
	std::shared_ptr<CGraphic> normal_graphics;
	std::shared_ptr<CGraphic> pressed_graphics;
	std::shared_ptr<CGraphic> grayed_graphics;
};

}
