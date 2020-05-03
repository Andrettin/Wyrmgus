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
//

#pragma once

#include "database/data_type.h"
#include "database/detailed_data_entry.h"
#include "data_type.h"
#include "time/date.h"
#include "vec2i.h"

class LuaCallback;
struct lua_State;

int CclDefineCampaign(lua_State *l);
int CclGetCampaignData(lua_State *l);

namespace stratagus {

class calendar;
class faction;
class map_template;
class quest;
class timeline;

class campaign : public detailed_data_entry, public data_type<campaign>, public CDataType
{
	Q_OBJECT

	Q_PROPERTY(QDateTime start_date MEMBER start_date READ get_start_date)
	Q_PROPERTY(stratagus::calendar* start_date_calendar MEMBER start_date_calendar)
	Q_PROPERTY(stratagus::timeline* timeline MEMBER timeline READ get_timeline)
	Q_PROPERTY(stratagus::faction* faction MEMBER faction READ get_faction)
	Q_PROPERTY(stratagus::quest* completion_quest MEMBER completion_quest READ get_completion_quest)
	Q_PROPERTY(QVariantList map_templates READ get_map_templates_qvariant_list)

public:
	static constexpr const char *class_identifier = "campaign";
	static constexpr const char *database_folder = "campaigns";

	static void initialize_all();

public:
	campaign(const std::string &identifier) : detailed_data_entry(identifier), CDataType(identifier)
	{
	}

	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void initialize() override;
	
	const QDateTime &get_start_date() const
	{
		return this->start_date;
	}

	timeline *get_timeline() const
	{
		return this->timeline;
	}
	
	faction *get_faction() const
	{
		return this->faction;
	}

	quest *get_completion_quest() const
	{
		return this->completion_quest;
	}

	std::string GetSpecies() const;

	bool IsHidden() const
	{
		return this->Hidden;
	}

	bool IsAvailable() const;

	bool contains_timeline_date(const timeline *timeline, const QDateTime &date) const;

	const std::vector<map_template *> &get_map_templates() const
	{
		return this->map_templates;
	}

	QVariantList get_map_templates_qvariant_list() const;

	Q_INVOKABLE void add_map_template(map_template *map_template)
	{
		this->map_templates.push_back(map_template);
	}

	Q_INVOKABLE void remove_map_template(map_template *map_template);

private:
	QDateTime start_date; //the starting date for the campaign
	calendar *start_date_calendar = nullptr; //the calendar for the start date
	timeline *timeline = nullptr; //the timeline in which the campaign is set
	bool Hidden = false;			/// Whether the campaign is hidden
	bool Sandbox = false;			/// Whether the campaign is a sandbox one
	std::vector<quest *> RequiredQuests;		/// Quests required by the campaign
	faction *faction = nullptr;	//which faction the player plays as in the campaign
	quest *completion_quest = nullptr; //the quest which when completed means that the campaign has been completed as well
	std::vector<map_template *> map_templates; //map templates used by the campaign
public:
	std::vector<Vec2i> MapSizes;				/// Map sizes
	std::vector<Vec2i> MapTemplateStartPos;		/// Map template position the map will start on
	
	friend int ::CclDefineCampaign(lua_State *l);
	friend int ::CclGetCampaignData(lua_State *l);
};

}

extern void SetCurrentCampaign(const std::string &campaign_ident);
extern std::string GetCurrentCampaign();
