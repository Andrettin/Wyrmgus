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
//      (c) Copyright 2019-2020 by Andrettin
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

#pragma once

class CConfigData;
class CPlayer;
class CUnitType;

namespace stratagus {

class dialogue;

/// The effect which occurs after triggering a trigger
class effect
{
public:
	virtual void ProcessConfigData(const CConfigData *config_data) = 0;
	virtual void do_effect(CPlayer *player) const = 0;
};

class call_dialogue_effect final : public effect
{
public:
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void do_effect(CPlayer *player) const override;
	
	stratagus::dialogue *Dialogue = nullptr;	/// Dialogue to be called
};

class create_unit_effect final : public effect
{
public:
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void do_effect(CPlayer *player) const override;
	
	int Quantity = 1;				/// Quantity of units created
	CUnitType *UnitType = nullptr;	/// Unit type to be created
};

}
