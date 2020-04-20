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

#include "util/singleton.h"

namespace stratagus {

class sml_data;
class sml_property;
class time_of_day;

class defines final : public QObject, public singleton<defines>
{
	Q_OBJECT

	Q_PROPERTY(QSize tile_size MEMBER tile_size READ get_tile_size)
	Q_PROPERTY(QSize icon_size MEMBER icon_size READ get_icon_size)
	Q_PROPERTY(int scale_factor MEMBER scale_factor READ get_scale_factor)
	Q_PROPERTY(stratagus::time_of_day* underground_time_of_day MEMBER underground_time_of_day READ get_underground_time_of_day)

public:
	void load(const std::filesystem::path &base_path);
	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);

	const QSize &get_tile_size() const
	{
		return this->tile_size;
	}

	int get_tile_width() const
	{
		return this->get_tile_size().width();
	}

	int get_tile_height() const
	{
		return this->get_tile_size().height();
	}

	const QSize &get_icon_size() const
	{
		return this->icon_size;
	}

	time_of_day *get_underground_time_of_day() const
	{
		return this->underground_time_of_day;
	}

	int get_scale_factor() const
	{
		return this->scale_factor;
	}

	QSize get_scaled_tile_size() const
	{
		return this->get_tile_size() * this->get_scale_factor();
	}

	int get_scaled_tile_width() const
	{
		return this->get_tile_size().width() * this->get_scale_factor();
	}

	int get_scaled_tile_height() const
	{
		return this->get_tile_size().height() * this->get_scale_factor();
	}


private:
	QSize tile_size;
	QSize icon_size;
	time_of_day *underground_time_of_day = nullptr;
	int scale_factor = 1;
};

}
