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
/**@name faction_type.cpp - The faction type source file. */
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "faction_type.h"

#include "upgrade/upgrade_structs.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void FactionType::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_neutral", "neutral"), [](FactionType *faction_type, const bool neutral){ faction_type->Neutral = neutral; });
	ClassDB::bind_method(D_METHOD("is_neutral"), &FactionType::IsNeutral);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "neutral"), "set_neutral", "is_neutral");

	ClassDB::bind_method(D_METHOD("set_religious", "religious"), [](FactionType *faction_type, const bool religious){ faction_type->Religious = religious; });
	ClassDB::bind_method(D_METHOD("is_religious"), &FactionType::IsReligious);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "religious"), "set_religious", "is_religious");
	
	ClassDB::bind_method(D_METHOD("set_tribal", "tribal"), [](FactionType *faction_type, const bool tribal){ faction_type->Tribal = tribal; });
	ClassDB::bind_method(D_METHOD("is_tribal"), &FactionType::IsTribal);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "tribal"), "set_tribal", "is_tribal");
	
	ClassDB::bind_method(D_METHOD("set_upgrade", "upgrade_ident"), [](FactionType *faction_type, const String &upgrade_ident){ faction_type->Upgrade = CUpgrade::Get(upgrade_ident.utf8().get_data()); });
	ClassDB::bind_method(D_METHOD("get_upgrade"), [](const FactionType *faction_type){ return const_cast<CUpgrade *>(faction_type->GetUpgrade()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "upgrade"), "set_upgrade", "get_upgrade");
}
