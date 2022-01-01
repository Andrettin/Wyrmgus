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
//      (c) Copyright 2018-2022 by Andrettin
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

#include "stratagus.h"

#include "magic_domain.h"

#include "config.h"
#include "upgrade/upgrade_structs.h"

namespace wyrmgus {

magic_domain *magic_domain::add(const std::string &identifier, const wyrmgus::data_module *data_module)
{
	magic_domain *domain = data_type::add(identifier, data_module);

	//add an upgrade with an identifier based on that of the magic domain for it
	CUpgrade *upgrade = CUpgrade::add("upgrade_deity_domain_" + identifier, data_module, domain);
	domain->deity_domain_upgrade = upgrade;

	return domain;
}

void magic_domain::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "deity_domain_upgrade") {
		database::process_sml_data(this->deity_domain_upgrade, scope);
	} else {
		data_entry::process_sml_scope(scope);
	}
}

}
