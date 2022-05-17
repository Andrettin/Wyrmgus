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

namespace wyrmgus {

class data_module;

//the metadata for a data type, including e.g. its initialization function
class data_type_metadata final
{
public:
	using parsing_function_type = std::function<boost::asio::awaitable<void>(const std::filesystem::path &, const data_module *)>;

	explicit data_type_metadata(const std::string &class_identifier, const std::set<std::string> &database_dependencies, const parsing_function_type &parsing_function, const std::function<void(bool)> &processing_function, const std::function<void()> &initialization_function, const std::function<void()> &text_processing_function, const std::function<void()> &checking_function, const std::function<void()> &clearing_function)
		: class_identifier(class_identifier), database_dependencies(database_dependencies), parsing_function(parsing_function), processing_function(processing_function), initialization_function(initialization_function), text_processing_function(text_processing_function), checking_function(checking_function), clearing_function(clearing_function)
	{
	}

	const std::string &get_class_identifier() const
	{
		return this->class_identifier;
	}

	bool has_database_dependency_on(const std::string &class_identifier) const
	{
		return this->database_dependencies.find(class_identifier) != this->database_dependencies.end();
	}

	bool has_database_dependency_on(const std::unique_ptr<data_type_metadata> &metadata) const
	{
		return this->has_database_dependency_on(metadata->get_class_identifier());
	}

	size_t get_database_dependency_count() const
	{
		return this->database_dependencies.size();
	}

	const parsing_function_type &get_parsing_function() const
	{
		return this->parsing_function;
	}

	const std::function<void(bool)> &get_processing_function() const
	{
		return this->processing_function;
	}

	const std::function<void()> &get_initialization_function() const
	{
		return this->initialization_function;
	}

	const std::function<void()> &get_text_processing_function() const
	{
		return this->text_processing_function;
	}

	const std::function<void()> &get_checking_function() const
	{
		return this->checking_function;
	}

	const std::function<void()> &get_clearing_function() const
	{
		return this->clearing_function;
	}

private:
	std::string class_identifier;
	const std::set<std::string> &database_dependencies;
	parsing_function_type parsing_function;
	std::function<void(bool)> processing_function;
	std::function<void()> initialization_function; //functions to initialize entries
	std::function<void()> text_processing_function; //functions to process text for entries
	std::function<void()> checking_function; //functions to check if data entries are valid
	std::function<void()> clearing_function; //functions to clear the data entries
};

}
