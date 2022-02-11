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

#pragma once

#include "database/gsml_data_visitor.h"
#include "database/gsml_element_visitor.h"
#include "database/gsml_property.h"
#include "database/gsml_property_visitor.h"

namespace wyrmgus {

class geocoordinate;
class gsml_parser;

//stratagus markup language data
class gsml_data final
{
public:
	static gsml_data from_point(const QPoint &point, const std::string &tag = std::string())
	{
		gsml_data point_data(tag);
		point_data.add_value(std::to_string(point.x()));
		point_data.add_value(std::to_string(point.y()));
		return point_data;
	}

	static gsml_data from_size(const QSize &point, const std::string &tag = std::string())
	{
		gsml_data point_data(tag);
		point_data.add_value(std::to_string(point.width()));
		point_data.add_value(std::to_string(point.height()));
		return point_data;
	}

	static gsml_data from_rect(const QRect &rect, const std::string &tag = std::string())
	{
		gsml_data rect_data(tag);

		const QPoint top_left = rect.topLeft();
		rect_data.add_value(std::to_string(top_left.x()));
		rect_data.add_value(std::to_string(top_left.y()));

		const QPoint bottom_right = rect.bottomRight();
		rect_data.add_value(std::to_string(bottom_right.x()));
		rect_data.add_value(std::to_string(bottom_right.y()));

		return rect_data;
	}

	explicit gsml_data(std::string &&tag = std::string());

	explicit gsml_data(std::string &&tag, const gsml_operator scope_operator)
		: tag(std::move(tag)), scope_operator(scope_operator)
	{
	}

	explicit gsml_data(const std::string &tag) : gsml_data(std::string(tag))
	{
	}

	explicit gsml_data(const std::string &tag, const gsml_operator scope_operator)
		: gsml_data(std::string(tag), scope_operator)
	{
	}

	const std::string &get_tag() const
	{
		return this->tag;
	}

	gsml_operator get_operator() const
	{
		return this->scope_operator;
	}

	const gsml_data *get_parent() const
	{
		return this->parent;
	}

	bool has_children() const
	{
		for (const auto &element : this->get_elements()) {
			if (std::holds_alternative<gsml_data>(element)) {
				return true;
			}
		}

		return false;
	}

	int get_children_count() const
	{
		int count = 0;

		for (const auto &element : this->get_elements()) {
			if (std::holds_alternative<gsml_data>(element)) {
				++count;
			}
		}

		return count;
	}

	const gsml_data &get_child(const std::string &tag) const
	{
		for (const auto &element : this->get_elements()) {
			if (!std::holds_alternative<gsml_data>(element)) {
				continue;
			}

			const gsml_data &child = std::get<gsml_data>(element);
			if (child.get_tag() == tag) {
				return child;
			}
		}

		throw std::runtime_error("No child with tag \"" + tag + "\" found for GSML data.");
	}

	bool has_child(const std::string &tag) const
	{
		for (const auto &element : this->get_elements()) {
			if (!std::holds_alternative<gsml_data>(element)) {
				continue;
			}

			if (std::get<gsml_data>(element).get_tag() == tag) {
				return true;
			}
		}

		return false;
	}

	gsml_data &add_child(gsml_data &&child)
	{
		this->elements.emplace_back(std::move(child));
		return std::get<gsml_data>(this->elements.back());
	}

	gsml_data &add_child()
	{
		return this->add_child(gsml_data());
	}

	gsml_data &add_child(std::string &&tag, const gsml_operator gsml_operator)
	{
		this->elements.push_back(gsml_data(std::move(tag), gsml_operator));
		return std::get<gsml_data>(this->elements.back());
	}

	void remove_child(const std::string &tag)
	{
		for (size_t i = 0; i < this->elements.size(); ++i) {
			const auto &element = this->elements.at(i);

			if (!std::holds_alternative<gsml_data>(element)) {
				continue;
			}

			const gsml_data &child = std::get<gsml_data>(element);
			if (child.get_tag() == tag) {
				this->elements.erase(this->elements.begin() + i);
				return;
			}
		}

		throw std::runtime_error("No child with tag \"" + tag + "\" found for GSML data.");
	}

	template <typename function_type>
	void for_each_child(const function_type &function) const
	{
		const gsml_data_visitor visitor(function);
		for (const auto &element : this->get_elements()) {
			std::visit(visitor, element);
		}
	}

	std::vector<const gsml_property *> try_get_properties(const std::string &key) const
	{
		std::vector<const gsml_property *> properties;

		this->for_each_property([&](const gsml_property &property) {
			if (property.get_key() == key) {
				properties.push_back(&property);
			}
		});

		return properties;
	}

	const std::string &get_property_value(const std::string &key) const
	{
		for (const auto &element : this->get_elements()) {
			if (!std::holds_alternative<gsml_property>(element)) {
				continue;
			}

			const gsml_property &property = std::get<gsml_property>(element);
			if (property.get_key() == key) {
				return property.get_value();
			}
		}

		throw std::runtime_error("No property with key \"" + key + "\" found for GSML data.");
	}

	void add_property(const std::string &key, const std::string &value);
	void add_property(std::string &&key, const gsml_operator gsml_operator, std::string &&value);

	template <typename function_type>
	void for_each_property(const function_type &function) const
	{
		const gsml_property_visitor visitor(function);
		for (const auto &element : this->get_elements()) {
			std::visit(visitor, element);
		}
	}

	void clear_properties()
	{
		//remove all property elements
		for (size_t i = 0; i < this->elements.size();) {
			const auto &element = this->elements.at(i);

			if (std::holds_alternative<gsml_property>(element)) {
				this->elements.erase(this->elements.begin() + i);
			} else {
				++i;
			}
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

	const std::vector<std::variant<gsml_property, gsml_data>> &get_elements() const
	{
		return this->elements;
	}

	template <typename property_function_type, typename data_function_type>
	void for_each_element(const property_function_type &property_function, const data_function_type &data_function) const
	{
		const gsml_element_visitor visitor(property_function, data_function);
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

	void print_to_file(const std::filesystem::path &filepath) const;

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

		this->for_each_property([&](const gsml_property &property) {
			property.print(ostream, indentation);
		});

		bool new_line = true;
		this->for_each_child([&](const gsml_data &child_data) {
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
		//get whether this is minor GSML data, e.g. just containing a few simple values
		return this->get_tag().empty() && this->get_elements().empty() && this->get_values().size() < 5;
	}

private:
	std::string tag;
	gsml_operator scope_operator;
	gsml_data *parent = nullptr;
	std::vector<std::string> values; //values directly attached to the GSML data scope, used for e.g. name arrays
	std::vector<std::variant<gsml_property, gsml_data>> elements;

	friend gsml_parser;
};

}
