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
//      (c) Copyright 2020-2021 by Andrettin
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

#include "database/data_entry.h"

namespace wyrmgus {

class text_processor;
struct text_processing_context;

class named_data_entry : public data_entry
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring NOTIFY changed)
	Q_PROPERTY(QString encyclopedia_text READ get_encyclopedia_text_qstring CONSTANT)

public:
	static void concatenate_encyclopedia_text(std::string &text, std::string &&additional_text)
	{
		if (additional_text.empty()) {
			return;
		}

		if (!text.empty()) {
			text += "\n\n";
		}

		text += std::move(additional_text);
	}

	explicit named_data_entry(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual ~named_data_entry() {}

	virtual void process_text() override;

	virtual std::string get_encyclopedia_text() const
	{
		return std::string();
	}

	QString get_encyclopedia_text_qstring() const
	{
		return QString::fromStdString(this->get_encyclopedia_text());
	}

	const std::string &get_name() const
	{
		return this->name;
	}

	Q_INVOKABLE void set_name(const std::string &name)
	{
		this->name = name;
	}

	QString get_name_qstring() const
	{
		return QString::fromStdString(this->get_name());
	}

	virtual text_processing_context get_text_processing_context() const;
	text_processor create_text_processor() const;

signals:
	void changed();

private:
	std::string name;
};

}
