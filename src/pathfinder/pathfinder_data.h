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

#include "pathfinder/pathfinder.h"

#include <boost/graph/adjacency_list.hpp>

namespace wyrmgus {

class pathfinder_data final
{
public:
	using cost = int;
	using graph = boost::adjacency_list<boost::listS, boost::vecS, boost::undirectedS, boost::no_property, boost::property<boost::edge_weight_t, cost>>;
	using edge = graph::edge_descriptor;
	using vertex = graph::vertex_descriptor;

	explicit pathfinder_data(const size_t z) : z(z)
	{
		for (int x = 0; x < CMap::get()->Info->MapWidths[z]; ++x) {
			for (int y = 0; y < CMap::get()->Info->MapHeights[z]; ++y) {
				const QPoint tile_pos(x, y);
				const int tile_index = CMap::get()->get_pos_index(tile_pos, z);

				for (int x_offset = -1; x_offset <= 1; ++x_offset) {
					for (int y_offset = -1; y_offset <= 1; ++y_offset) {
						if (x_offset == 0 && y_offset == 0) {
							continue;
						}

						const QPoint adjacent_tile_pos(tile_pos.x() + x_offset, tile_pos.y() + y_offset);

						if (!CMap::get()->Info->IsPointOnMap(adjacent_tile_pos, z)) {
							continue;
						}

						const int adjacent_tile_index = CMap::get()->get_pos_index(adjacent_tile_pos, z);

						edge e;
						bool inserted = false;
						boost::tie(e, inserted) = boost::add_edge(tile_index, adjacent_tile_index, this->map_graph);
					}
				}
			}
		}
	}

	graph &get_map_graph()
	{
		return this->map_graph;
	}

public:
	size_t z = 0;
	graph map_graph;
};

}
