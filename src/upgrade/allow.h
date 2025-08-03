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
//      (c) Copyright 1999-2025 by Vladi Belperchinov-Shabanski,
//		Jimmy Salmon and Andrettin
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

/**
**  Allow what a player can do. Every #CPlayer has an own allow struct.
**
**  This could allow/disallow units, actions or upgrades.
**
**  Values are:
**    @li `A' -- allowed,
**    @li `F' -- forbidden,
**    @li `R' -- acquired, perhaps other values
**    @li `Q' -- acquired but forbidden (does it make sense?:))
**    @li `E' -- enabled, allowed by level but currently forbidden
**    @li `X' -- fixed, acquired can't be disabled
*/
class CAllow final
{
public:
	CAllow()
	{
		this->Clear();
	}

	void Clear()
	{
		memset(Units, 0, sizeof(Units));
		memset(Upgrades, 0, sizeof(Upgrades));
	}

	int  Units[UnitTypeMax];        /// maximum amount of units allowed
	char Upgrades[UpgradeMax];      /// upgrades allowed/disallowed
};
