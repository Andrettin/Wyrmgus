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

#pragma once

#include "network/network.h"
#include "script/condition/condition.h"
#include "time/month.h"
#include "util/locale_util.h"

#include <magic_enum/magic_enum.hpp>

namespace wyrmgus {

template <typename scope_type>
class real_month_condition final : public condition<scope_type>
{
public:
	explicit real_month_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->month = magic_enum::enum_cast<archimedes::month>(value).value();
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "real_month";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);

		if (IsNetworkGame()) {
			//always false in multiplayer games, to prevent desyncs if the real month changes during a game
			return false;
		}

		const QDateTime current_date = QDateTime::currentDateTime();
		const int current_month = current_date.date().month();

		return static_cast<int>(this->month) == current_month;
	}

	virtual std::string get_assignment_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent);
		Q_UNUSED(links_allowed);

		return "The current real month is " + string::highlight(locale::english_locale.standaloneMonthName(static_cast<int>(this->month)).toStdString());
	}

private:
	wyrmgus::month month;
};

}
