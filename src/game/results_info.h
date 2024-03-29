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
//      (c) Copyright 2021-2022 by Andrettin
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

#include "util/qunique_ptr.h"

enum GameResults : int;

namespace wyrmgus {

class player_results_info;

class results_info final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(bool victory READ is_victory CONSTANT)
	Q_PROPERTY(bool defeat READ is_defeat CONSTANT)
	Q_PROPERTY(bool draw READ is_draw CONSTANT)
	Q_PROPERTY(QVariantList player_results READ get_player_results_qvariant_list CONSTANT)

public:
	explicit results_info(const GameResults result, std::vector<qunique_ptr<player_results_info>> &&player_results);
	results_info(const results_info &other) = delete;
	~results_info();

	bool is_victory() const;
	bool is_defeat() const;
	bool is_draw() const;

	QVariantList get_player_results_qvariant_list() const;

	results_info &operator =(const results_info &other) = delete;

private:
	GameResults result;
	std::vector<qunique_ptr<player_results_info>> player_results;
};

}
