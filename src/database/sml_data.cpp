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
//      (c) Copyright 2019-2021 by Andrettin
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

#include "database/sml_data.h"

#include "database/sml_operator.h"
#include "database/sml_property_visitor.h"
#include "util/geocoordinate.h"

namespace wyrmgus {

sml_data::sml_data(std::string &&tag)
	: tag(std::move(tag)), scope_operator(sml_operator::assignment)
{
}

void sml_data::add_property(const std::string &key, const std::string &value)
{
	this->elements.push_back(sml_property(key, sml_operator::assignment, value));
}

void sml_data::add_property(std::string &&key, const sml_operator sml_operator, std::string &&value)
{
	this->elements.push_back(sml_property(std::move(key), sml_operator, std::move(value)));
}

geocoordinate sml_data::to_geocoordinate() const
{
	if (this->get_values().size() != 2) {
		throw std::runtime_error("Geocoordinate scopes need to contain exactly two values.");
	}

	geocoordinate::number_type longitude = geocoordinate::number_type(this->get_values()[0]);
	geocoordinate::number_type latitude = geocoordinate::number_type(this->get_values()[1]);
	return geocoordinate(std::move(longitude), std::move(latitude));
}

void sml_data::print(std::ostream &ostream, const size_t indentation, const bool new_line) const
{
	if (new_line) {
		ostream << std::string(indentation, '\t');
	} else {
		ostream << " ";
	}
	if (!this->get_tag().empty()) {
		ostream << this->get_tag() << " ";
		switch (this->get_operator()) {
			case sml_operator::assignment:
				ostream << "=";
				break;
			case sml_operator::addition:
				ostream << "+=";
				break;
			case sml_operator::subtraction:
				ostream << "-=";
				break;
			case sml_operator::none:
				throw std::runtime_error("Cannot print the SML \"none\" operator.");
		}
		ostream << " ";
	}
	ostream << "{";
	if (!this->is_minor()) {
		ostream << "\n";
	}

	this->print_components(ostream, indentation + 1);

	if (!this->is_minor()) {
		ostream << std::string(indentation, '\t');
	} else if (this->is_empty()) {
		//ensure that there is a white space between the braces of an empty SML data
		ostream << " ";
	}
	ostream << "}";
	if (!this->is_minor()) {
		ostream << "\n";
	}
}

}
