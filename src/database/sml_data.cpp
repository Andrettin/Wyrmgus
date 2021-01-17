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
	}
	ostream << "}";
	if (!this->is_minor()) {
		ostream << "\n";
	}
}

}
