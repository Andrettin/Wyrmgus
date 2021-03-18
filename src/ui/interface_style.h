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

#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"

class CGraphic;

namespace wyrmgus {

class button_style;
enum class interface_element_type;

class interface_style final : public data_entry, public data_type<interface_style>
{
	Q_OBJECT

	Q_PROPERTY(std::filesystem::path top_bar_file MEMBER top_bar_file WRITE set_top_bar_file)

public:
	static constexpr const char *class_identifier = "interface_style";
	static constexpr const char *database_folder = "interface_styles";

	explicit interface_style(const std::string &identifier);
	~interface_style();

	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;

	void set_top_bar_file(const std::filesystem::path &filepath);

	const std::shared_ptr<CGraphic> &get_interface_element_graphics(const interface_element_type type, const std::string &qualifier) const;

	const button_style *get_button(const interface_element_type type) const;

private:
	std::filesystem::path top_bar_file;
	std::shared_ptr<CGraphic> top_bar_graphics;
	std::unique_ptr<button_style> large_button;
};

}
