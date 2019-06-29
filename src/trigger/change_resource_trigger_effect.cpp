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
/**@name change_resource_trigger_effect.cpp - The change resource trigger effect source file. */
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

#include "trigger/change_resource_trigger_effect.h"

#include "config.h"
#include "config_operator.h"
#include "economy/resource.h"
#include "player.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CChangeResourceTriggerEffect::ProcessConfigData(const CConfigData *config_data)
{
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			print_error("Wrong operator enumeration index for property \"" + property.Key + "\": " + String::num_int64(static_cast<int>(property.Operator)) + ".");
			continue;
		}
		
		if (property.Key == "quantity") {
			this->Quantity = property.Value.to_int();
		} else if (property.Key == "resource") {
			const CResource *resource = CResource::Get(property.Value);
			if (resource != nullptr) {
				this->Resource = resource;
			}
		} else {
			fprintf(stderr, "Invalid change resource trigger effect property: \"%s\".\n", property.Key.utf8().get_data());
		}
	}
	
	if (!this->Resource) {
		fprintf(stderr, "Change resource trigger effect has no resource.\n");
	}
}

/**
**	@brief	Do the effect
**
**	@param	player	The player for which to do the effect
*/
void CChangeResourceTriggerEffect::Do(CPlayer *player) const
{
	player->ChangeResource(this->Resource->GetIndex(), this->Quantity, true);
}
