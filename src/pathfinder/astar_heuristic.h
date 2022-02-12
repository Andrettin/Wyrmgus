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
//      (c) Copyright 1999-2022 by Lutz Sammer, Fabrice Rossi, Russell Smith,
//      Francois Beerten, Jimmy Salmon and Andrettin
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

#include "map/map.h"
#include "util/number_util.h"

#include <boost/graph/astar_search.hpp>
#include <boost/graph/graph_traits.hpp>

namespace wyrmgus {

template <class graph, class cost>
class astar_heuristic final : public boost::astar_heuristic<graph, cost>
{
public:
	using vertex = typename boost::graph_traits<graph>::vertex_descriptor;

	explicit astar_heuristic(const vertex goal_index, const size_t z) : goal_index(goal_index), z(z)
	{
	}

	cost operator()(const vertex tile_index) const
	{
		const QPoint tile_pos = CMap::get()->get_index_pos(tile_index, this->z);
		const QPoint goal_pos = CMap::get()->get_index_pos(goal_index, this->z);

		const QPoint diff = tile_pos - goal_pos;
		return std::max<int>(number::fast_abs(diff.x()), number::fast_abs(diff.y()));
	}

private:
	vertex goal_index;
	size_t z = 0;
};

}
