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
//      (c) Copyright 2019-2020 by Andrettin
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

namespace wyrmgus {

class module;

//the metadata for a data type, including e.g. its initialization function
class data_type_metadata
{
public:
	data_type_metadata(const std::string &class_identifier, const std::set<std::string> &database_dependencies, const std::function<void(const std::filesystem::path &, const module *)> &parsing_function, const std::function<void(bool)> &processing_function, const std::function<void()> &initialization_function, const std::function<void()> &checking_function, const std::function<void()> &clearing_function)
		: class_identifier(class_identifier), database_dependencies(database_dependencies), parsing_function(parsing_function), processing_function(processing_function), initialization_function(initialization_function), checking_function(checking_function), clearing_function(clearing_function)
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

	const std::function<void(const std::filesystem::path &, const module *)> &get_parsing_function() const
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
	std::function<void(const std::filesystem::path &, const module *)> parsing_function;
	std::function<void(bool)> processing_function;
	std::function<void()> initialization_function; //functions to initialize entries
	std::function<void()> checking_function; //functions to check if data entries are valid
	std::function<void()> clearing_function; //functions to clear the data entries
};

}
