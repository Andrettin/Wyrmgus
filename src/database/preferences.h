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

#pragma once

#include "util/singleton.h"

namespace wyrmgus {

class campaign;
class sml_data;
class sml_property;

class preferences final : public QObject, public singleton<preferences>
{
	Q_OBJECT

	Q_PROPERTY(int scale_factor READ get_scale_factor WRITE set_scale_factor)
	Q_PROPERTY(wyrmgus::campaign* selected_campaign READ get_selected_campaign WRITE set_selected_campaign)
	Q_PROPERTY(bool fullscreen MEMBER fullscreen READ is_fullscreen)

public:
	static std::filesystem::path get_path();
	static std::filesystem::path get_fallback_path();

	void load();
	Q_INVOKABLE void save() const;
	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);

	int get_scale_factor() const
	{
		return this->scale_factor;
	}

	void set_scale_factor(const int factor);

	campaign *get_selected_campaign() const
	{
		return this->selected_campaign;
	}

	void set_selected_campaign(campaign *campaign)
	{
		this->selected_campaign = campaign;
	}

	bool is_fullscreen() const
	{
		return this->fullscreen;
	}

private:
	int scale_factor = 1;
	campaign *selected_campaign = nullptr;
	bool fullscreen = false;
};

}
