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

#include "database/data_entry.h"

namespace wyrmgus {

class text_processor;

class named_data_entry : public data_entry
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring)

public:
	explicit named_data_entry(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual ~named_data_entry() {}

	virtual void process_text() override;

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

	virtual text_processor create_text_processor() const;

private:
	std::string name;
};

}
