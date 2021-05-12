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
//      (c) Copyright 1998-2021 by Vladi Shabanski, Lutz Sammer,
//                                 Jimmy Salmon and Andrettin
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

#include "util/qunique_ptr.h"
#include "vec2i.h"

class CMapLayer;
struct lua_State;

static int CclPresentMap(lua_State *l);
static int CclStratagusMap(lua_State *l);

namespace wyrmgus {

class map_info final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring CONSTANT)

public:
	bool IsPointOnMap(const int x, const int y, const int z) const;

	bool IsPointOnMap(const Vec2i &pos, const int z) const;

	bool IsPointOnMap(const int x, const int y, const CMapLayer *map_layer) const;

	bool IsPointOnMap(const Vec2i &pos, const CMapLayer *map_layer) const;

	void Clear();

	qunique_ptr<map_info> duplicate() const;

	const std::string &get_name() const
	{
		return this->name;
	}

	QString get_name_qstring() const
	{
		return QString::fromStdString(this->name);
	}

	void set_name(const std::string &name)
	{
		this->name = name;
	}

private:
	std::string name;
public:
	std::string Filename;       /// Map filename
	int MapWidth;               /// Map width
	int MapHeight;              /// Map height
	//Wyrmgus start
	std::vector<int> MapWidths;	/// Map width for each map layer
	std::vector<int> MapHeights; /// Map height for each map layer
	//Wyrmgus end
	int PlayerType[PlayerMax];  /// Same player->Type
	int PlayerSide[PlayerMax];  /// Same player->Side
	unsigned int MapUID;        /// Unique Map ID (hash)
	std::string MapWorld = "Custom";

	friend int ::CclPresentMap(lua_State *l);
	friend int ::CclStratagusMap(lua_State *l);
};

}
