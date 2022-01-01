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

#include "database/named_data_entry.h"

#include "language/word.h"
#include "text_processor.h"

namespace wyrmgus {

void named_data_entry::initialize()
{
	if (this->get_name_word() != nullptr && this->name.empty()) {
		this->name = this->get_name_word()->get_anglicized_name();
	}

	data_entry::initialize();
}

void named_data_entry::process_text()
{
	//process the name for the named data entry
	if (!this->name.empty()) {
		const text_processor text_processor = this->create_text_processor();
		this->name = text_processor.process_text(std::move(this->name), false);
	}

	data_entry::process_text();
}

text_processing_context named_data_entry::get_text_processing_context() const
{
	return text_processing_context();
}

text_processor named_data_entry::create_text_processor() const
{
	return text_processor(this->get_text_processing_context());
}

}
