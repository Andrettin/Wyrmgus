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
//      (c) Copyright 2018-2020 by Andrettin
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

#include "stratagus.h"

#include "religion/deity_domain.h"

#include "config.h"
#include "upgrade/upgrade_structs.h"

namespace stratagus {

deity_domain *deity_domain::get_by_upgrade(const CUpgrade *upgrade)
{
	auto find_iterator = deity_domain::domains_by_upgrade.find(upgrade);
	if (find_iterator != deity_domain::domains_by_upgrade.end()) {
		return find_iterator->second;
	}

	throw std::runtime_error("No deity domain found for upgrade \"" + upgrade->get_identifier() + "\".");
}

void deity_domain::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;

		if (key == "name") {
			this->set_name(value);
		} else if (key == "upgrade") {
			CUpgrade *upgrade = CUpgrade::get(value);
			this->Upgrade = upgrade;
			deity_domain::domains_by_upgrade[upgrade] = this;
		} else if (key == "ability") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *ability = CUpgrade::get(value);
			this->Abilities.push_back(ability);
			ability->DeityDomains.push_back(this);
		} else if (key == "description") {
			this->set_description(value);
		} else if (key == "background") {
			this->set_background(value);
		} else if (key == "quote") {
			this->set_quote(value);
		} else {
			fprintf(stderr, "Invalid school of magic property: \"%s\".\n", key.c_str());
		}
	}
}

}
