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

#include "database/basic_data_entry.h"

namespace wyrmgus {

class data_entry_history;
class data_module;
class sml_data;
class sml_property;

//a (de)serializable and identifiable entry to the database
class data_entry : public basic_data_entry
{
	Q_OBJECT

	Q_PROPERTY(QString identifier READ get_identifier_qstring CONSTANT)

public:
	explicit data_entry(const std::string &identifier);
	virtual ~data_entry();

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

	virtual void process_sml_property(const sml_property &property) override;
	virtual void process_sml_scope(const sml_data &scope) override;
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

	virtual void initialize();

	virtual void process_text()
	{
	}

	virtual void check() const
	{
	}

	const wyrmgus::data_module *get_module() const
	{
		return this->data_module;
	}

	void set_module(const wyrmgus::data_module *data_module)
	{
		if (data_module == this->get_module()) {
			return;
		}

		this->data_module = data_module;
	}

	virtual data_entry_history *get_history_base()
	{
		return nullptr;
	}

	void load_history();
	void load_date_scope(const sml_data &date_scope, const QDateTime &date);

	virtual void reset_history()
	{
	}

	virtual bool has_encyclopedia_entry() const
	{
		return false;
	}

	virtual std::string get_link_name() const
	{
		return this->get_identifier();
	}

signals:
	void changed();

private:
	std::string identifier;
	std::set<std::string> aliases;
	bool defined = false; //whether the data entry's definition has been concluded (with its data having been processed)
	bool initialized = false;
	const wyrmgus::data_module *data_module = nullptr; //the module to which the data entry belongs, if any
	std::vector<sml_data> history_data;
};

}
