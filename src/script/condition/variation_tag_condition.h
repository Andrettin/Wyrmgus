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
//      (c) Copyright 2022 by Andrettin
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

#pragma once

#include "script/condition/condition.h"
#include "unit/unit.h"
#include "unit/unit_type_variation.h"
#include "unit/variation_tag.h"

namespace wyrmgus {

class variation_tag_condition final : public condition<CUnit>
{
public:
	explicit variation_tag_condition(const std::string &value, const gsml_operator condition_operator)
		: condition(condition_operator)
	{
		this->variation_tag = variation_tag::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "variation_tag";
		return class_identifier;
	}

	virtual bool check_assignment(const CUnit *unit, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		if (unit->GetVariation() == nullptr) {
			return false;
		}

		return unit->GetVariation()->has_tag(this->variation_tag);
	}

	virtual std::string get_assignment_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent);
		Q_UNUSED(links_allowed);

		return string::highlight(this->variation_tag->get_name()) + " variation";
	}

private:
	const wyrmgus::variation_tag *variation_tag = nullptr;
};

}
