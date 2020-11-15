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
//      (c) Copyright 2020 by Andrettin
//
//      Permission is hereby granted, free of charge, to any person obtaining a
//      copy of this software and associated documentation files (the
//      "Software"), to deal in the Software without restriction, including
//      without limitation the rights to use, copy, modify, merge, publish,
//      distribute, sublicense, and/or sell copies of the Software, and to
//      permit persons to whom the Software is furnished to do so, subject to
//      the following conditions:
//
//      The above copyright notice and this permission notice shall be included
//      in all copies or substantial portions of the Software.
//
//      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "database/data_entry_history.h"

namespace wyrmgus {

class faction;
class historical_location;

class historical_unit_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(bool active MEMBER active READ is_active)
	Q_PROPERTY(wyrmgus::faction* faction MEMBER faction)

public:
	historical_unit_history();
	~historical_unit_history();

	void process_sml_property(const sml_property &property) override;
	void process_sml_scope(const sml_data &scope) override;

	bool is_active() const
	{
		return this->active;
	}

	const faction *get_faction() const
	{
		return this->faction;
	}

	const historical_location *get_location() const
	{
		return this->location.get();
	}

private:
	bool active = false; //whether the unit is active, i.e. should be applied to the map
	wyrmgus::faction *faction = nullptr;
	std::unique_ptr<historical_location> location;
};

}
