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
//      (c) Copyright 2020-2022 by Andrettin
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

class CGraphic;

namespace wyrmgus {

class icon_base : public data_entry
{
	Q_OBJECT

	Q_PROPERTY(std::filesystem::path file MEMBER file WRITE set_file)
	Q_PROPERTY(int frame MEMBER frame READ get_frame)

public:
	explicit icon_base(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual ~icon_base();

	virtual void initialize() override;

	virtual void check() const override
	{
		if (this->get_file().empty()) {
			throw std::runtime_error("Icon \"" + this->get_identifier() + "\" has no image file associated with it.");
		}
	}

	bool is_loaded() const;
	void load() const;

	virtual const QSize &get_size() const = 0;

	const std::filesystem::path &get_file() const
	{
		return this->file;
	}

	void set_file(const std::filesystem::path &filepath);

	const std::shared_ptr<CGraphic> &get_graphics() const
	{
		return this->graphics;
	}

protected:
	void set_graphics(const std::shared_ptr<CGraphic> &graphics)
	{
		this->graphics = graphics;
	}

public:
	int get_frame() const
	{
		return this->frame;
	}

protected:
	void set_frame(const int frame)
	{
		this->frame = frame;
	}

protected:
	std::filesystem::path file;
private:
	std::shared_ptr<CGraphic> graphics;
	int frame = 0;
};

}
