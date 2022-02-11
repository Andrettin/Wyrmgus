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

#include "player/dynasty.h"

#include "character.h"
#include "player/faction.h"
#include "script/condition/and_condition.h"
#include "upgrade/upgrade_structs.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

dynasty::dynasty(const std::string &identifier) : detailed_data_entry(identifier)
{
}

dynasty::~dynasty()
{
}

void dynasty::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "preconditions") {
		this->preconditions = std::make_unique<and_condition>();
		database::process_gsml_data(this->preconditions, scope);
	} else if (tag == "conditions") {
		this->conditions = std::make_unique<and_condition>();
		database::process_gsml_data(this->conditions, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void dynasty::check() const
{
	if (this->get_preconditions() != nullptr) {
		this->get_preconditions()->check_validity();
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}

std::string dynasty::get_encyclopedia_text() const
{
	std::string text;

	std::vector<const faction *> factions = this->get_factions();
	std::sort(factions.begin(), factions.end(), named_data_entry::compare_encyclopedia_entries);

	if (!factions.empty()) {
		std::string factions_text = "Factions: ";
		for (size_t i = 0; i < factions.size(); ++i) {
			if (i > 0) {
				factions_text += ", ";
			}

			const faction *faction = factions.at(i);
			factions_text += faction->get_link_string();
		}

		named_data_entry::concatenate_encyclopedia_text(text, std::move(factions_text));
	}

	if (this->get_upgrade() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, "Effects: " + this->get_upgrade()->get_effects_string());
	}

	if (this->get_conditions() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, "Conditions:\n" + this->get_conditions()->get_conditions_string(1, true));
	}

	named_data_entry::concatenate_encyclopedia_text(text, detailed_data_entry::get_encyclopedia_text());

	return text;
}

void dynasty::set_upgrade(CUpgrade *upgrade)
{
	if (upgrade == this->get_upgrade()) {
		return;
	}

	this->upgrade = upgrade;
	upgrade->set_dynasty(this);
}

QVariantList dynasty::get_factions_qvariant_list() const
{
	return container::to_qvariant_list(this->get_factions());
}

void dynasty::add_faction(faction *faction)
{
	this->factions.push_back(faction);
	faction->add_dynasty(this);
}

void dynasty::remove_faction(faction *faction)
{
	vector::remove(this->factions, faction);
}

QVariantList dynasty::get_dynastic_tree_characters() const
{
	std::vector<const character *> tree_characters;

	for (const character *character : this->get_characters()) {
		if (character->is_hidden_in_tree()) {
			continue;
		}

		tree_characters.push_back(character);
	}

	return container::to_qvariant_list(tree_characters);
}

}
