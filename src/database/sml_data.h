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

#pragma once

#include "database/sml_data_visitor.h"
#include "database/sml_element_visitor.h"
#include "database/sml_property.h"
#include "database/sml_property_visitor.h"

namespace wyrmgus {

class geocoordinate;
class sml_parser;

//stratagus markup language data
class sml_data final
{
public:
	static sml_data from_point(const QPoint &point, const std::string &tag = std::string())
	{
		sml_data point_data(tag);
		point_data.add_value(std::to_string(point.x()));
		point_data.add_value(std::to_string(point.y()));
		return point_data;
	}

	static sml_data from_rect(const QRect &rect, const std::string &tag = std::string())
	{
		sml_data rect_data(tag);

		const QPoint top_left = rect.topLeft();
		rect_data.add_value(std::to_string(top_left.x()));
		rect_data.add_value(std::to_string(top_left.y()));

		const QPoint bottom_right = rect.bottomRight();
		rect_data.add_value(std::to_string(bottom_right.x()));
		rect_data.add_value(std::to_string(bottom_right.y()));

		return rect_data;
	}

	explicit sml_data(std::string &&tag = std::string());

	explicit sml_data(std::string &&tag, const sml_operator scope_operator)
		: tag(std::move(tag)), scope_operator(scope_operator)
	{
	}

	explicit sml_data(const std::string &tag) : sml_data(std::string(tag))
	{
	}

	explicit sml_data(const std::string &tag, const sml_operator scope_operator)
		: sml_data(std::string(tag), scope_operator)
	{
	}

	const std::string &get_tag() const
	{
		return this->tag;
	}

	sml_operator get_operator() const
	{
		return this->scope_operator;
	}

	const sml_data *get_parent() const
	{
		return this->parent;
	}

	bool has_children() const
	{
		for (const auto &element : this->get_elements()) {
			if (std::holds_alternative<sml_data>(element)) {
				return true;
			}
		}

		return false;
	}

	int get_children_count() const
	{
		int count = 0;

		for (const auto &element : this->get_elements()) {
			if (std::holds_alternative<sml_data>(element)) {
				++count;
			}
		}

		return count;
	}

	const sml_data &get_child(const std::string &tag) const
	{
		for (const auto &element : this->get_elements()) {
			if (!std::holds_alternative<sml_data>(element)) {
				continue;
			}

			const sml_data &child = std::get<sml_data>(element);
			if (child.get_tag() == tag) {
				return child;
			}
		}

		throw std::runtime_error("No child with tag \"" + tag + "\" found for SML data.");
	}

	bool has_child(const std::string &tag) const
	{
		for (const auto &element : this->get_elements()) {
			if (!std::holds_alternative<sml_data>(element)) {
				continue;
			}

			if (std::get<sml_data>(element).get_tag() == tag) {
				return true;
			}
		}

		return false;
	}

	sml_data &add_child()
	{
		this->elements.push_back(sml_data());
		return std::get<sml_data>(this->elements.back());
	}

	void add_child(sml_data &&child)
	{
		this->elements.emplace_back(std::move(child));
	}

	sml_data &add_child(std::string &&tag, const sml_operator sml_operator)
	{
		this->elements.push_back(sml_data(std::move(tag), sml_operator));
		return std::get<sml_data>(this->elements.back());
	}

	template <typename function_type>
	void for_each_child(const function_type &function) const
	{
		const sml_data_visitor visitor(function);
		for (const auto &element : this->get_elements()) {
			std::visit(visitor, element);
		}
	}

	std::vector<const sml_property *> try_get_properties(const std::string &key) const
	{
		std::vector<const sml_property *> properties;

		this->for_each_property([&](const sml_property &property) {
			if (property.get_key() == key) {
				properties.push_back(&property);
			}
		});

		return properties;
	}

	const std::string &get_property_value(const std::string &key) const
	{
		for (const auto &element : this->get_elements()) {
			if (!std::holds_alternative<sml_property>(element)) {
				continue;
			}

			const sml_property &property = std::get<sml_property>(element);
			if (property.get_key() == key) {
				return property.get_value();
			}
		}

		throw std::runtime_error("No property with key \"" + key + "\" found for SML data.");
	}

	void add_property(const std::string &key, const std::string &value);
	void add_property(std::string &&key, const sml_operator sml_operator, std::string &&value);

	template <typename function_type>
	void for_each_property(const function_type &function) const
	{
		const sml_property_visitor visitor(function);
		for (const auto &element : this->get_elements()) {
			std::visit(visitor, element);
		}
	}

	const std::vector<std::string> &get_values() const
	{
		return this->values;
	}

	void add_value(const std::string &value)
	{
		this->values.push_back(value);
	}

	void add_value(std::string &&value)
	{
		this->values.push_back(std::move(value));
	}

	bool is_empty() const
	{
		return this->get_elements().empty() && this->get_values().empty();
	}

	const std::vector<std::variant<sml_property, sml_data>> &get_elements() const
	{
		return this->elements;
	}

	template <typename property_function_type, typename data_function_type>
	void for_each_element(const property_function_type &property_function, const data_function_type &data_function) const
	{
		const sml_element_visitor visitor(property_function, data_function);
		for (const auto &element : this->get_elements()) {
			std::visit(visitor, element);
		}
	}

	QColor to_color() const
	{
		if (this->get_values().size() != 3) {
			throw std::runtime_error("Color scopes need to contain exactly three values.");
		}

		const int red = std::stoi(this->values.at(0));
		const int green = std::stoi(this->values.at(1));
		const int blue = std::stoi(this->values.at(2));

		return QColor(red, green, blue);
	}

	QPoint to_point() const
	{
		if (this->get_values().size() != 2) {
			throw std::runtime_error("Point scopes need to contain exactly two values.");
		}

		const int x = std::stoi(this->get_values()[0]);
		const int y = std::stoi(this->get_values()[1]);
		return QPoint(x, y);
	}

	QPointF to_pointf() const
	{
		if (this->get_values().size() != 2) {
			throw std::runtime_error("Point scopes need to contain exactly two values.");
		}

		const double x = std::stod(this->get_values()[0]);
		const double y = std::stod(this->get_values()[1]);
		return QPointF(x, y);
	}

	QSize to_size() const
	{
		if (this->get_values().size() != 2) {
			throw std::runtime_error("Size scopes need to contain exactly two values.");
		}

		const int width = std::stoi(this->get_values()[0]);
		const int height = std::stoi(this->get_values()[1]);
		return QSize(width, height);
	}

	geocoordinate to_geocoordinate() const;

	QRect to_rect() const
	{
		if (this->get_values().size() != 4) {
			throw std::runtime_error("Rect scopes need to contain exactly four values.");
		}

		const int min_x = std::stoi(this->get_values()[0]);
		const int min_y = std::stoi(this->get_values()[1]);
		const int max_x = std::stoi(this->get_values()[2]);
		const int max_y = std::stoi(this->get_values()[3]);
		return QRect(QPoint(min_x, min_y), QPoint(max_x, max_y));
	}

	void print_to_file(const std::filesystem::path &filepath) const
	{
		std::ofstream ofstream(filepath);

		if (!ofstream) {
			throw std::runtime_error("Failed to open file \"" + filepath.string() + "\" for printing SML data to.");
		}

		this->print_components(ofstream);
	}

	void print_to_dir(const std::filesystem::path &directory) const
	{
		const std::filesystem::path filepath = directory / (this->get_tag() + ".txt");
		this->print_to_file(filepath);
	}

	std::string print_to_string() const
	{
		std::ostringstream ostream;
		this->print_components(ostream);
		return ostream.str();
	}

	void print(std::ostream &ostream, const size_t indentation, const bool new_line) const;

	void print_components(std::ostream &ostream, const size_t indentation = 0) const
	{
		if (!this->get_values().empty()) {
			if (this->is_minor()) {
				ostream << " ";
			} else {
				ostream << std::string(indentation, '\t');
			}
		}
		for (const std::string &value : this->get_values()) {
			ostream << value << " ";
		}
		if (!this->get_values().empty()) {
			if (!this->is_minor()) {
				ostream << "\n";
			}
		}

		this->for_each_property([&](const sml_property &property) {
			property.print(ostream, indentation);
		});

		bool new_line = true;
		this->for_each_child([&](const sml_data &child_data) {
			child_data.print(ostream, indentation, new_line);
			if (new_line && child_data.is_minor()) {
				new_line = false;
			}
		});

		//if the last child data was minor and did not print a new line, print one now
		if (!new_line) {
			ostream << "\n";
		}
	}

private:
	bool is_minor() const
	{
		//get whether this is minor SML data, e.g. just containing a few simple values
		return this->get_tag().empty() && this->get_elements().empty() && this->get_values().size() < 5;
	}

private:
	std::string tag;
	sml_operator scope_operator;
	sml_data *parent = nullptr;
	std::vector<std::string> values; //values directly attached to the SML data scope, used for e.g. name arrays
	std::vector<std::variant<sml_property, sml_data>> elements;

	friend sml_parser;
};

}
