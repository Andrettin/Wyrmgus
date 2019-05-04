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
/**@name trigger_effect.cpp - The trigger effect source file. */
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

#include "trigger/trigger_effect.h"

#include "config.h"
#include "trigger/call_dialogue_trigger_effect.h"
#include "trigger/change_resource_trigger_effect.h"
#include "trigger/create_unit_trigger_effect.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Create a trigger effect from config data
**
**	@param	config_data	The configuration data
*/
CTriggerEffect *CTriggerEffect::FromConfigData(const CConfigData *config_data)
{
	CTriggerEffect *trigger_effect = nullptr;
	
	if (config_data->Tag == "call_dialogue") {
		trigger_effect = new CCallDialogueTriggerEffect;
	} else if (config_data->Tag == "change_resource") {
		trigger_effect = new CChangeResourceTriggerEffect;
	} else if (config_data->Tag == "create_unit") {
		trigger_effect = new CCreateUnitTriggerEffect;
	} else {
		fprintf(stderr, "Invalid trigger effect type: \"%s\".\n", config_data->Tag.c_str());
	}
	
	trigger_effect->ProcessConfigData(config_data);
	
	return trigger_effect;
}
