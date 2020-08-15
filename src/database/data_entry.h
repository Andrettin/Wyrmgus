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
class sml_data;
class sml_property;

//a (de)serializable and identifiable entry to the database
class data_entry : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString identifier READ get_identifier_qstring CONSTANT)

public:
	data_entry(const std::string &identifier) : identifier(identifier)
	{
	}

	virtual ~data_entry() {}

	const std::string &get_identifier() const
	{
		return this->identifier;
	}

	QString get_identifier_qstring() const
	{
		return QString::fromStdString(this->get_identifier());
	}

	const std::set<std::string> &get_aliases() const
	{
		return this->aliases;
	}

	void add_alias(const std::string &alias)
	{
		this->aliases.insert(alias);
	}

	virtual void process_sml_property(const sml_property &property);
	virtual void process_sml_scope(const sml_data &scope);
	virtual void process_sml_dated_property(const sml_property &property, const QDateTime &date);
	virtual void process_sml_dated_scope(const sml_data &scope, const QDateTime &date);

	bool is_defined() const
	{
		return this->defined;
	}

	void set_defined(const bool defined)
	{
		this->defined = defined;
	}

	bool is_initialized() const
	{
		return this->initialized;
	}

	virtual void initialize()
	{
		this->initialized = true;
	}

	virtual void check() const {}

	const module *get_module() const
	{
		return this->module;
	}

	void set_module(const module *module)
	{
		if (module == this->get_module()) {
			return;
		}

		this->module = module;
	}

	void load_history();
	void load_date_scope(const sml_data &date_scope, const QDateTime &date);
	virtual void reset_history() {}

private:
	std::string identifier;
	std::set<std::string> aliases;
	bool defined = false; //whether the data entry's definition has been concluded (with its data having been processed)
	bool initialized = false;
	const module *module = nullptr; //the module to which the data entry belongs, if any
	std::vector<sml_data> history_data;
};

}
