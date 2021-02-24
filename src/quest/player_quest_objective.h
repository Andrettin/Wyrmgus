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
//      (c) Copyright 2015-2021 by Andrettin
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

class CPlayer;
class CUnit;

namespace wyrmgus {

class quest_objective;
class resource;

class player_quest_objective final
{
public:
	explicit player_quest_objective(const wyrmgus::quest_objective *quest_objective, const CPlayer *player)
		: quest_objective(quest_objective), player(player)
	{
	}

	const wyrmgus::quest_objective *get_quest_objective() const
	{
		return this->quest_objective;
	}

	const CPlayer *get_player() const
	{
		return this->player;
	}

	int get_counter() const
	{
		return this->counter;
	}
	
	void set_counter(const int value);

	void change_counter(const int change)
	{
		this->set_counter(this->get_counter() + change);
	}

	void increment_counter()
	{
		this->change_counter(1);
	}

	void update_counter();
	void on_unit_built(const CUnit *unit);
	void on_unit_destroyed(const CUnit *unit);
	void on_resource_gathered(const resource *resource, const int quantity);

private:
	const wyrmgus::quest_objective *quest_objective = nullptr;
	const CPlayer *player = nullptr;
	int counter = 0;
};

}
