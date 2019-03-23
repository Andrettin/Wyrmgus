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
/**@name literary_text_page.cpp - The literary text page source file. */
//
//      (c) Copyright 2016-2019 by Andrettin
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

#include "config.h"
#include "literary_text_page.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CLiteraryTextPage::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "text") {
			value = FindAndReplaceString(value, "_", "-");
			this->Text = value.c_str();
		} else if (key == "number") { // override the number given in the constructor
			this->Number = std::stoi(value);
		} else {
			fprintf(stderr, "Invalid literary text property: \"%s\".\n", key.c_str());
		}
	}
}

void CLiteraryTextPage::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_text"), &CLiteraryTextPage::GetText);
	ClassDB::bind_method(D_METHOD("get_number"), &CLiteraryTextPage::GetNumber);
	ClassDB::bind_method(D_METHOD("get_previous_page"), &CLiteraryTextPage::GetPreviousPage);
	ClassDB::bind_method(D_METHOD("get_next_page"), &CLiteraryTextPage::GetNextPage);
}
