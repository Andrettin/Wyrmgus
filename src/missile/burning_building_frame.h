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
/**@name burning_building_frame.h - The burning building frame header file. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer and Andrettin
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
//

#ifndef __BURNING_BUILDING_FRAME_H__
#define __BURNING_BUILDING_FRAME_H__

#include <vector>

class MissileType;

class BurningBuildingFrame
{
public:
	int Percent = 0;				/// HP percent
	MissileType *Missile = nullptr;	/// Missile to draw
};

extern std::vector<BurningBuildingFrame *> BurningBuildingFrames;  /// Burning building frames

/// Get the burning building missile based on hp percent
extern MissileType *MissileBurningBuilding(int percent);

extern void FreeBurningBuildingFrames();

#endif
