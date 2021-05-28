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
//      (c) Copyright 2017-2021 by Andrettin
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

#include "database/data_type.h"
#include "database/named_data_entry.h"

struct lua_State;

static int CclDefineAchievement(lua_State *l);

namespace wyrmgus {

class character;
class icon;
class player_color;
class quest;
class unit_type;
enum class difficulty;

class achievement final : public named_data_entry, public data_type<achievement>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(QString description READ get_description_qstring CONSTANT)
	Q_PROPERTY(wyrmgus::player_color* player_color MEMBER player_color NOTIFY changed)
	Q_PROPERTY(wyrmgus::difficulty difficulty MEMBER difficulty)
	Q_PROPERTY(bool hidden MEMBER hidden READ is_hidden)
	Q_PROPERTY(bool unobtainable MEMBER unobtainable)
	Q_PROPERTY(wyrmgus::character* character MEMBER character)
	Q_PROPERTY(wyrmgus::unit_type* character_type MEMBER character_type)
	Q_PROPERTY(int character_level MEMBER character_level)
	Q_PROPERTY(bool obtained READ is_obtained NOTIFY changed)
	Q_PROPERTY(int progress READ get_progress NOTIFY changed)
	Q_PROPERTY(int progress_max READ get_progress_max NOTIFY changed)

public:
	static constexpr const char *class_identifier = "achievement";
	static constexpr const char *database_folder = "achievements";

	static void check_achievements();

	explicit achievement(const std::string &identifier);

	virtual void process_sml_property(const sml_property &property) override;
	virtual void process_sml_scope(const sml_data &scope) override;

	virtual void check() const override
	{
		if (this->get_icon() == nullptr) {
			throw std::runtime_error("Achievement \"" + this->get_identifier() + "\" has no icon.");
		}
	}

	const wyrmgus::icon *get_icon() const
	{
		return this->icon;
	}

	const std::string &get_description() const
	{
		return this->description;
	}

	QString get_description_qstring() const
	{
		return QString::fromStdString(this->get_description());
	}

	const wyrmgus::player_color *get_player_color() const
	{
		return this->player_color;
	}

	wyrmgus::difficulty get_difficulty() const
	{
		return this->difficulty;
	}

	bool is_hidden() const
	{
		return this->hidden;
	}

	bool is_obtained() const
	{
		return this->obtained;
	}

	bool can_obtain() const;
	void obtain(bool save = true, bool display = true);
	int get_progress() const;
	int get_progress_max() const;

signals:
	void changed();

private:
	wyrmgus::icon *icon = nullptr;
	std::string description;
	wyrmgus::player_color *player_color = nullptr; //player color used for the achievement's icon
	wyrmgus::difficulty difficulty; //in which difficulty the achievement's requirements need to be done
	bool hidden = false;
	bool unobtainable = false; //whether this achievement can be obtained by checking for it or not
	wyrmgus::character *character = nullptr; //character related to the achievement's requirements
	unit_type *character_type = nullptr; //unit type required for a character to have for the achievement
	int character_level = 0; //character level required for the achievement
public:
	std::vector<const quest *> RequiredQuests;
private:
	bool obtained = false;

	friend int ::CclDefineAchievement(lua_State *l);
};

}

extern void SetAchievementObtained(const std::string &achievement_ident, const bool save = true, const bool display = true);