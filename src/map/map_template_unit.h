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

#pragma once

namespace wyrmgus {

class faction;
class gsml_data;
class gsml_property;
class unit_class;
class unit_type;

class map_template_unit final
{
public:
	explicit map_template_unit(const unit_type *unit_type) : type(unit_type)
	{
	}

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	const unit_type *get_type() const
	{
		return this->type;
	}

	const wyrmgus::unit_class *get_unit_class() const
	{
		return this->unit_class;
	}

	void set_unit_class(const wyrmgus::unit_class *unit_class)
	{
		this->unit_class = unit_class;
	}

	const QPoint &get_pos() const
	{
		return this->pos;
	}

	void set_pos(const QPoint &pos)
	{
		this->pos = pos;
	}

	const wyrmgus::faction *get_faction() const
	{
		return this->faction;
	}

	int get_player_index() const
	{
		return this->player_index;
	}

	void set_player_index(const int index)
	{
		this->player_index = index;
	}

	int get_resource_amount() const
	{
		return this->resource_amount;
	}

	void set_resource_amount(const int amount)
	{
		this->resource_amount = amount;
	}

private:
	const unit_type *type = nullptr;
	const wyrmgus::unit_class *unit_class = nullptr;
	QPoint pos = QPoint(0, 0);
	const wyrmgus::faction *faction = nullptr;
	int player_index = -1;
	int resource_amount = 0;
};

}
