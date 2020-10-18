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
//      (c) Copyright 2020 by Andrettin
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
