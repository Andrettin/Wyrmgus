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
//      (c) Copyright 1998-2022 by Vladi Shabanski, Lutz Sammer,
//                                 Jimmy Salmon and Andrettin
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
#include "vec2i.h"

class CMapLayer;
struct lua_State;

extern int CclDefinePlayerTypes(lua_State *l);
extern int CclPresentMap(lua_State *l);
extern int CclStratagusMap(lua_State *l);

namespace wyrmgus {

class gsml_data;
class gsml_property;
class map_settings;
enum class player_type;

class map_info final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring NOTIFY changed)
	Q_PROPERTY(std::string description MEMBER description)
	Q_PROPERTY(std::string author MEMBER author)
	Q_PROPERTY(QString presentation_filepath READ get_presentation_filepath_qstring CONSTANT)
	Q_PROPERTY(QString presentation_filename READ get_presentation_filename_qstring CONSTANT)
	Q_PROPERTY(QSize map_size MEMBER map_size READ get_map_size NOTIFY changed)
	Q_PROPERTY(int map_width READ get_map_width CONSTANT)
	Q_PROPERTY(int map_height READ get_map_height CONSTANT)
	Q_PROPERTY(int player_count READ get_player_count CONSTANT)
	Q_PROPERTY(int person_player_count READ get_person_player_count CONSTANT)
	Q_PROPERTY(int person_player_index READ get_person_player_index CONSTANT)
	Q_PROPERTY(bool hidden MEMBER hidden READ is_hidden)
	Q_PROPERTY(QString text READ get_text_qstring CONSTANT)

public:
	map_info();
	~map_info();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	bool IsPointOnMap(const int x, const int y, const int z) const;

	bool IsPointOnMap(const Vec2i &pos, const int z) const;

	bool IsPointOnMap(const int x, const int y, const CMapLayer *map_layer) const;

	bool IsPointOnMap(const Vec2i &pos, const CMapLayer *map_layer) const;

	void reset();

	qunique_ptr<map_info> duplicate() const;

	const std::string &get_name() const
	{
		return this->name;
	}

	QString get_name_qstring() const
	{
		return QString::fromStdString(this->get_name());
	}

	Q_INVOKABLE void set_name(const std::string &name)
	{
		this->name = name;
	}

	const std::filesystem::path &get_presentation_filepath() const
	{
		return this->presentation_filepath;
	}

	std::string get_presentation_filepath_string() const
	{
		return this->get_presentation_filepath().string();
	}

	QString get_presentation_filepath_qstring() const;

	void set_presentation_filepath(const std::filesystem::path &filepath)
	{
		this->presentation_filepath = filepath;
	}

	QString get_presentation_filename_qstring() const;

	std::filesystem::path get_setup_filepath() const
	{
		std::filesystem::path setup_filepath = this->get_presentation_filepath();

		if (!setup_filepath.empty()) {
			setup_filepath.replace_extension(".sms");
		}

		return setup_filepath;
	}

	const QSize &get_map_size() const
	{
		return this->map_size;
	}

	void set_map_size(const QSize &size)
	{
		this->map_size = size;
	}

	int get_map_width() const
	{
		return this->get_map_size().width();
	}

	int get_map_height() const
	{
		return this->get_map_size().height();
	}

	const std::array<player_type, PlayerMax> &get_player_types() const
	{
		return this->player_types;
	}

	int get_player_count() const;
	int get_person_player_count() const;
	int get_person_player_index() const;

	const map_settings *get_settings() const
	{
		return this->settings.get();
	}

	void set_settings(qunique_ptr<map_settings> &&settings);

	bool is_hidden() const
	{
		return this->hidden;
	}

	std::string get_text() const;

	QString get_text_qstring() const
	{
		return QString::fromStdString(this->get_text());
	}

signals:
	void changed();

private:
	std::string name;
	std::string description;
	std::string author;
	std::filesystem::path presentation_filepath;
	QSize map_size = QSize(0, 0);
public:
	//Wyrmgus start
	std::vector<int> MapWidths;	/// Map width for each map layer
	std::vector<int> MapHeights; /// Map height for each map layer
	//Wyrmgus end
private:
	std::array<player_type, PlayerMax> player_types;  /// Same player->Type
public:
	std::array<int, PlayerMax> PlayerSide;  /// Same player->Side
	unsigned int MapUID;        /// Unique Map ID (hash)
	std::string MapWorld = "Custom";
private:
	qunique_ptr<map_settings> settings;
	bool hidden = false;

	friend int ::CclDefinePlayerTypes(lua_State *l);
	friend int ::CclPresentMap(lua_State *l);
	friend int ::CclStratagusMap(lua_State *l);
};

}
