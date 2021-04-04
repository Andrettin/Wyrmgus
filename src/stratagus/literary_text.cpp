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
//      (c) Copyright 2016-2021 by Andrettin
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

#include "literary_text.h"

namespace wyrmgus {

bool literary_text::compare_encyclopedia_entries(const literary_text *lhs, const literary_text *rhs)
{
	if (lhs->main_text != rhs->main_text) {
		if (lhs->main_text == nullptr || rhs->main_text == nullptr) {
			return lhs->main_text != nullptr;
		}

		return lhs->main_text->get_name() < rhs->main_text->get_name();
	} else if (lhs->main_text != nullptr) {
		return lhs->chapter_index < rhs->chapter_index;
	}

	return named_data_entry::compare_encyclopedia_entries(lhs, rhs);
}

void literary_text::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "chapters") {
		for (const std::string &value : values) {
			literary_text *chapter = literary_text::get(value);
			chapter->main_text = this;
			chapter->chapter_index = this->chapters.size();
			this->chapters.push_back(chapter);
		}
	} else {
		data_entry::process_sml_scope(scope);
	}
}

}
