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
//      (c) Copyright 2019-2022 by Andrettin
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

#include "database/gsml_data.h"

#include "database/gsml_operator.h"
#include "database/gsml_property_visitor.h"
#include "util/geocoordinate.h"
#include "util/path_util.h"

namespace wyrmgus {

gsml_data::gsml_data(std::string &&tag)
	: tag(std::move(tag)), scope_operator(gsml_operator::assignment)
{
}

void gsml_data::add_property(const std::string &key, const std::string &value)
{
	this->elements.push_back(gsml_property(key, gsml_operator::assignment, value));
}

void gsml_data::add_property(std::string &&key, const gsml_operator gsml_operator, std::string &&value)
{
	this->elements.push_back(gsml_property(std::move(key), gsml_operator, std::move(value)));
}

geocoordinate gsml_data::to_geocoordinate() const
{
	if (this->get_values().size() != 2) {
		throw std::runtime_error("Geocoordinate scopes need to contain exactly two values.");
	}

	geocoordinate::number_type longitude = geocoordinate::number_type(this->get_values()[0]);
	geocoordinate::number_type latitude = geocoordinate::number_type(this->get_values()[1]);
	return geocoordinate(std::move(longitude), std::move(latitude));
}

void gsml_data::print_to_file(const std::filesystem::path &filepath) const
{
	std::ofstream ofstream(filepath);

	if (!ofstream) {
		throw std::runtime_error("Failed to open file \"" + filepath.string() + "\" for printing GSML data to.");
	}

	this->print_components(ofstream);
}

void gsml_data::print(std::ostream &ostream, const size_t indentation, const bool new_line) const
{
	if (new_line) {
		ostream << std::string(indentation, '\t');
	} else {
		ostream << " ";
	}
	if (!this->get_tag().empty()) {
		ostream << this->get_tag() << " ";
		switch (this->get_operator()) {
			case gsml_operator::assignment:
				ostream << "=";
				break;
			case gsml_operator::addition:
				ostream << "+=";
				break;
			case gsml_operator::subtraction:
				ostream << "-=";
				break;
			case gsml_operator::none:
				throw std::runtime_error("Cannot print the GSML \"none\" operator.");
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
		//ensure that there is a white space between the braces of an empty GSML data
		ostream << " ";
	}
	ostream << "}";
	if (!this->is_minor()) {
		ostream << "\n";
	}
}

}
