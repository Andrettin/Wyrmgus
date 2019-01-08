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
/**@name trigger_effect.h - The trigger effect header file. */
//
//      (c) Copyright 2019 by Andrettin
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

#ifndef __TRIGGER_EFFECT_H__
#define __TRIGGER_EFFECT_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CConfigData;
class CPlayer;
class CUnitType;

/// The effect which occurs after triggering a trigger
class CTriggerEffect
{
public:
	virtual void ProcessConfigData(const CConfigData *config_data) = 0;
	virtual void Do(CPlayer *player) const = 0;			/// Performs the trigger effect
};

/// The create unit trigger effect
class CCreateUnitTriggerEffect : public CTriggerEffect
{
public:
	virtual void ProcessConfigData(const CConfigData *config_data);
	virtual void Do(CPlayer *player) const;				/// Performs the trigger effect
	
	int Quantity = 1;				/// Quantity of units created
	CUnitType *UnitType = nullptr;	/// Unit type to be created
};

#endif
