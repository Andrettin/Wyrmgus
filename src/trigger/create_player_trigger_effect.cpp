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
/**@name create_player_trigger_effect.cpp - The create player trigger effect source file. */
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

#include "trigger/create_player_trigger_effect.h"

#include "config.h"
#include "config_operator.h"
#include "faction.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "player.h"
#include "unit/unit.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CCreatePlayerTriggerEffect::ProcessConfigData(const CConfigData *config_data)
{
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			print_error("Wrong operator enumeration index for property \"" + property.Key + "\": " + String::num_int64(static_cast<int>(property.Operator)) + ".");
			continue;
		}
		
		if (property.Key == "faction") {
			const CFaction *faction = CFaction::Get(property.Value);
			if (faction != nullptr) {
				this->Faction = faction;
			}
		} else if (property.Key == "start_site") {
			const CSite *site = CSite::Get(property.Value);
			if (site != nullptr) {
				this->StartSite = site;
			}
		} else {
			fprintf(stderr, "Invalid create player trigger effect property: \"%s\".\n", property.Key.utf8().get_data());
		}
	}
	
	if (this->Faction == nullptr) {
		fprintf(stderr, "Create player trigger effect has no faction.\n");
	}
}

/**
**	@brief	Do the effect
**
**	@param	player	The player for which to do the effect
*/
void CCreatePlayerTriggerEffect::Do(CPlayer *player) const
{
	CPlayer *new_player = CPlayer::GetOrAddFactionPlayer(this->Faction);
	
	if (new_player == nullptr) {
		fprintf(stderr, "Failed to create player for faction \"%s\".\n", this->Faction->GetIdent().utf8().get_data());
		return;
	}
	
	if (this->StartSite != nullptr) {
		const CUnit *site_unit = this->StartSite->SiteUnit;
		if (site_unit != nullptr) {
			new_player->SetStartView(site_unit->GetTilePos(), site_unit->GetMapLayer()->GetIndex());
		} else {
			fprintf(stderr, "Player start site \"%s\" has no site unit.\n", this->StartSite->GetIdent().utf8().get_data());
		}
	}
}
