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
//      (c) Copyright 1999-2025 by Lutz Sammer, Andreas Arens,
//                                 Jimmy Salmon, Pali Rohár and Andrettin
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

#include "stratagus.h"

#include "ui/filler.h"

#include "database/preferences.h"
#include "video/video.h"

CFiller &CFiller::operator =(const CFiller &other_filler)
{
	if (other_filler.G == nullptr) {
		throw std::runtime_error("Tried to create a copy of a UI filler which has no graphics.");
	}

	this->G = other_filler.G;
	this->X = other_filler.X;
	this->Y = other_filler.Y;

	return *this;
}

void CFiller::Load()
{
	if (this->G != nullptr) {
		this->G->Load(preferences::get()->get_scale_factor());
	}

	this->X = (this->X * preferences::get()->get_scale_factor()).to_int();
	this->Y = (this->Y * preferences::get()->get_scale_factor()).to_int();

	if (this->X < 0) {
		this->X = Video.Width + this->X;
	}

	if (this->Y < 0) {
		this->Y = Video.Height + this->Y;
	}

	this->loaded = true;
}

bool CFiller::OnGraphic(int x, int y) const
{
	x -= X;
	y -= Y;
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();
	if (x >= 0 && y >= 0 && x < this->G->get_width() && y < this->G->get_height()) {
		return this->G->get_image().pixelColor((x / scale_factor).to_int(), (y / scale_factor).to_int()).alpha() != 0;
	}
	return false;
}
