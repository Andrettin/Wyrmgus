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
//      (c) Copyright 2018-2020 by Andrettin
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
//

#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"
#include "data_type.h"

#include <map>
#include <string>
#include <vector>

class CDependency;
class CGraphic;
class CUpgrade;

class CAge : public stratagus::data_entry, public stratagus::data_type<CAge>
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring WRITE set_name_qstring)
	Q_PROPERTY(int priority MEMBER priority)
	Q_PROPERTY(int year_boost MEMBER year_boost)

public:
	static constexpr const char *class_identifier = "age";
	static constexpr const char *database_folder = "ages";

	static void initialize_all();

	static void SetCurrentAge(CAge *age);
	static void CheckCurrentAge();

	static CAge *CurrentAge;

	CAge(const std::string &identifier) : stratagus::data_entry(identifier)
	{
	}

	virtual ~CAge() override;
	
	virtual void process_sml_scope(const stratagus::sml_data &scope) override;

	virtual void check() const override
	{
		if (this->get_graphics() == nullptr) {
			throw std::runtime_error("Age \"" + this->get_identifier() + "\" has no icon.");
		}
	}

	const std::string &get_name() const
	{
		return this->name;
	}

	QString get_name_qstring() const
	{
		return QString::fromStdString(this->get_name());
	}

	void set_name_qstring(const QString &name)
	{
		this->name = name.toStdString();
	}

	CGraphic *get_graphics() const
	{
		return this->graphics;
	}

	CDependency *get_predependency() const
	{
		return this->predependency;
	}

	CDependency *get_dependency() const
	{
		return this->dependency;
	}

private:
	std::string name;
	CGraphic *graphics = nullptr;
	int priority = 0;
	int year_boost = 0;
	CDependency *predependency = nullptr;
	CDependency *dependency = nullptr;
};

extern void SetCurrentAge(const std::string &age_ident);
