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

class CPlayer;
class CUnit;

namespace wyrmgus {

class sml_data;
class sml_property;
class unit_ref;

//script context for e.g. events
template <bool read_only>
struct context_base
{
	using player_ptr = std::conditional_t<read_only, const CPlayer *, CPlayer *>;

	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);
	sml_data to_sml_data(const std::string &tag) const;

	player_ptr source_player = nullptr;
	player_ptr current_player = nullptr;
	std::shared_ptr<unit_ref> source_unit;
	std::shared_ptr<unit_ref> current_unit;
};

extern template struct context_base<false>;
extern template struct context_base<true>;

struct context final : context_base<false>
{
};

struct read_only_context final : context_base<true>
{
public:
	read_only_context()
	{
	}

	read_only_context(const context &ctx)
	{
		this->source_player = ctx.source_player;
		this->current_player = ctx.current_player;
		this->source_unit = ctx.source_unit;
		this->current_unit = ctx.current_unit;
	}
};

}
