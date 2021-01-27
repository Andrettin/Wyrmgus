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
//      (c) Copyright 2021 by Andrettin
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

#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"

class CPlayer;

namespace wyrmgus {

template <typename scope_type>
class effect_list;

class cheat final : public data_entry, public data_type<cheat>
{
	Q_OBJECT

	Q_PROPERTY(std::string code MEMBER code)

public:
	static constexpr const char *class_identifier = "cheat";
	static constexpr const char *database_folder = "cheats";

	static const cheat *try_get_by_code(const std::string &code)
	{
		const auto find_iterator = cheat::cheats_by_code.find(code);
		if (find_iterator != cheat::cheats_by_code.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	static void clear()
	{
		data_type::clear();
		cheat::cheats_by_code.clear();
	}

private:
	static inline std::map<std::string, const cheat *> cheats_by_code;

public:
	explicit cheat(const std::string &identifier);
	~cheat();

	virtual void process_sml_scope(const sml_data &scope) override;

	virtual void initialize() override
	{
		if (!this->code.empty()) {
			if (cheat::cheats_by_code.contains(this->code)) {
				throw std::runtime_error("Cannot add cheat \"" + this->get_identifier() + "\" for code \"" + code + "\", as another cheat is already assigned to that code.");
			}

			cheat::cheats_by_code[this->code] = this;
		}

		data_entry::initialize();
	}

	virtual void check() const override;

	void do_effects() const;

private:
	std::string code;
	std::unique_ptr<effect_list<CPlayer>> effects;
};

}
