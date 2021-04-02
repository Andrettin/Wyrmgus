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

#include "database/named_data_entry.h"

namespace wyrmgus {

//a data entry with description, quote and background, i.e. a data entry that can be shown in the encyclopedia
class detailed_data_entry : public named_data_entry
{
	Q_OBJECT

	Q_PROPERTY(QString notes READ get_notes_qstring)
	Q_PROPERTY(QString description READ get_description_qstring)
	Q_PROPERTY(QString quote READ get_quote_qstring)
	Q_PROPERTY(QString background READ get_background_qstring)

public:
	explicit detailed_data_entry(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual ~detailed_data_entry() {}

	virtual void process_text() override;

	virtual bool has_encyclopedia_entry() const override
	{
		if (this->get_notes().empty() && this->get_description().empty() && this->get_background().empty() && this->get_quote().empty()) {
			return false;
		}

		return true;
	}

	virtual std::string get_encyclopedia_text() const override
	{
		std::string text;

		if (!this->get_description().empty()) {
			named_data_entry::concatenate_encyclopedia_text(text, "Description: " + this->get_description());
		}

		if (!this->get_quote().empty()) {
			named_data_entry::concatenate_encyclopedia_text(text, "Quote: " + this->get_quote());
		}

		if (!this->get_background().empty()) {
			named_data_entry::concatenate_encyclopedia_text(text, "Background: " + this->get_background());
		}

		if (!this->get_notes().empty()) {
			named_data_entry::concatenate_encyclopedia_text(text, "Notes: " + this->get_notes());
		}

		return text;
	}

	const std::string &get_notes() const
	{
		return this->notes;
	}

	Q_INVOKABLE void set_notes(const std::string &notes)
	{
		this->notes = notes;
	}

	QString get_notes_qstring() const
	{
		return QString::fromStdString(this->get_notes());
	}

	const std::string &get_description() const
	{
		return this->description;
	}

	Q_INVOKABLE void set_description(const std::string &description)
	{
		this->description = description;
	}

	QString get_description_qstring() const
	{
		return QString::fromStdString(this->get_description());
	}

	const std::string &get_quote() const
	{
		return this->quote;
	}

	Q_INVOKABLE void set_quote(const std::string &quote)
	{
		this->quote = quote;
	}

	QString get_quote_qstring() const
	{
		return QString::fromStdString(this->get_quote());
	}

	const std::string &get_background() const
	{
		return this->background;
	}

	Q_INVOKABLE void set_background(const std::string &background)
	{
		this->background = background;
	}

	QString get_background_qstring() const
	{
		return QString::fromStdString(this->get_background());
	}

private:
	std::string notes; //gameplay-related notes about the data entry
	std::string description; //description from an in-universe perspective
	std::string quote; //quote related to the data entry
	std::string background; //encyclopedia text from the perspective of outside the game universe
};

}
