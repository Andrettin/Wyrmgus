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
/**@name create_unit_trigger_effect.cpp - The create unit trigger effect source file. */
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

#include "trigger/create_unit_trigger_effect.h"

#include "character.h"
#include "config.h"
#include "config_operator.h"
#include "faction.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "player.h"
#include "unit/unit.h"
#include "unit/unit_type.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CCreateUnitTriggerEffect::ProcessConfigData(const CConfigData *config_data)
{
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.utf8().get_data(), property.Operator);
			continue;
		}
		
		if (property.Key == "quantity") {
			this->Quantity = property.Value.to_int();
		} else if (property.Key == "unit_type") {
			const CUnitType *unit_type = CUnitType::Get(property.Value);
			if (unit_type != nullptr) {
				this->UnitType = unit_type;
			}
		} else if (property.Key == "faction") {
			const CFaction *faction = CFaction::Get(property.Value);
			if (faction != nullptr) {
				this->Faction = faction;
			}
		} else if (property.Key == "character") {
			CCharacter *character = CCharacter::Get(property.Value);
			if (character != nullptr) {
				this->Character = character;
				this->UnitType = character->UnitType;
			}
		} else if (property.Key == "site") {
			const CSite *site = CSite::Get(property.Value);
			if (site != nullptr) {
				this->Site = site;
			}
		} else {
			fprintf(stderr, "Invalid create unit trigger effect property: \"%s\".\n", property.Key.utf8().get_data());
		}
	}
	
	if (this->UnitType == nullptr) {
		fprintf(stderr, "Create unit trigger effect has no unit type.\n");
	}
	
	if (this->Character != nullptr && this->Quantity > 1) {
		fprintf(stderr, "Create unit trigger effect has a character, and a quantity greater than 1; setting quantity to 1.\n");
		this->Quantity = 1;
	}
}

/**
**	@brief	Do the effect
**
**	@param	player	The player for which to do the effect
*/
void CCreateUnitTriggerEffect::Do(CPlayer *player) const
{
	Vec2i tile_pos = player->StartPos;
	int z = player->StartMapLayer;
	
	CPlayer *unit_player = player;
	
	if (this->Faction != nullptr) {
		unit_player = CPlayer::GetFactionPlayer(this->Faction);
		if (unit_player == nullptr) {
			fprintf(stderr, "Unit faction \"%s\" has no player.\n", this->Faction->GetIdent().utf8().get_data());
			return;
		}
	}
	
	if (this->Site != nullptr) {
		const CUnit *site_unit = this->Site->SiteUnit;
		if (site_unit != nullptr) {
			tile_pos = site_unit->GetTilePos();
			z = site_unit->GetMapLayer()->GetIndex();
		} else {
			fprintf(stderr, "Unit site \"%s\" has no site unit.\n", this->Site->GetIdent().utf8().get_data());
		}
	}
	
	const bool no_bordering_building = this->UnitType->BoolFlag[BUILDING_INDEX].value; //give a bit of space for created buildings
	
	for (int i = 0; i < this->Quantity; ++i) {
		CUnit *unit = CreateUnit(tile_pos, *this->UnitType, unit_player, z, no_bordering_building);
		
		if (this->Character != nullptr) {
			unit->SetCharacter(this->Character->Ident);
		}
	}
}
