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
//      (c) Copyright 2020-2022 by Andrettin
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

#include "database/detailed_data_entry.h"

#include "text_processor.h"

namespace wyrmgus {

void detailed_data_entry::process_text()
{
	const std::unique_ptr<text_processor_base> text_processor = this->create_text_processor();

	if (text_processor != nullptr) {
		//process the description text for the detailed data entry
		if (!this->description.empty()) {
			this->description = text_processor->process_text(std::move(this->description), false);
		}

		if (!this->quote.empty()) {
			this->quote = text_processor->process_text(std::move(this->quote), false);
		}

		if (!this->background.empty()) {
			this->background = text_processor->process_text(std::move(this->background), false);
		}
	}

	named_data_entry::process_text();
}

}
