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
//      (c) Copyright 2022 by Andrettin
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

#include "script/context.h"

namespace wyrmgus {

class dialogue;
class dialogue_node;
class icon;
class player_color;

class dialogue_node_instance final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString title READ get_title CONSTANT)
	Q_PROPERTY(QString text READ get_text CONSTANT)
	Q_PROPERTY(wyrmgus::icon* icon READ get_icon CONSTANT)
	Q_PROPERTY(wyrmgus::player_color* player_color READ get_player_color CONSTANT)
	Q_PROPERTY(QStringList options READ get_options CONSTANT)
	Q_PROPERTY(QStringList option_hotkeys READ get_option_hotkeys CONSTANT)
	Q_PROPERTY(QStringList option_tooltips READ get_option_tooltips CONSTANT)
	Q_PROPERTY(int unit_number READ get_unit_number CONSTANT)

public:
	explicit dialogue_node_instance(const dialogue_node *node, const QString &title, const QString &text, const wyrmgus::icon *icon, const wyrmgus::player_color *player_color, const QStringList &options, const QStringList &option_hotkeys, const QStringList &option_tooltips, const int unit_number)
		: node(node), title(title), text(text), icon(const_cast<wyrmgus::icon *>(icon)), player_color(const_cast<wyrmgus::player_color *>(player_color)), options(options), option_hotkeys(option_hotkeys), option_tooltips(option_tooltips), unit_number(unit_number)
	{
	}

	const QString &get_title() const
	{
		return this->title;
	}

	const QString &get_text() const
	{
		return this->text;
	}

	wyrmgus::icon *get_icon() const
	{
		return this->icon;
	}

	wyrmgus::player_color *get_player_color() const
	{
		return this->player_color;
	}

	const QStringList &get_options() const
	{
		return this->options;
	}

	const QStringList &get_option_hotkeys() const
	{
		return this->option_hotkeys;
	}

	const QStringList &get_option_tooltips() const
	{
		return this->option_tooltips;
	}

	int get_unit_number() const
	{
		return this->unit_number;
	}

	Q_INVOKABLE void call_option_effect(const int option_index);

private:
	const dialogue_node *node = nullptr;
	QString title;
	QString text;
	wyrmgus::icon *icon = nullptr;
	wyrmgus::player_color *player_color = nullptr;
	QStringList options;
	QStringList option_hotkeys;
	QStringList option_tooltips;
	int unit_number = -1;
};

}
