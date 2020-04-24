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
#include "database/named_data_entry.h"
#include "data_type.h"
#include "time/date.h"
#include "vec2i.h"

class CFaction;
class CQuest;
class LuaCallback;
struct lua_State;

int CclDefineCampaign(lua_State *l);
int CclGetCampaignData(lua_State *l);

namespace stratagus {

class map_template;

class campaign : public named_data_entry, public data_type<campaign>, public CDataType
{
public:
	static constexpr const char *class_identifier = "campaign";
	static constexpr const char *database_folder = "campaigns";

	static void initialize_all();
	static void SetCurrentCampaign(campaign *campaign);
	static campaign *GetCurrentCampaign();

private:
	static campaign *CurrentCampaign;
	
public:
	campaign(const std::string &identifier) : named_data_entry(identifier), CDataType(identifier)
	{
	}

	virtual void ProcessConfigData(const CConfigData *config_data) override;
	
	const CDate &GetStartDate() const
	{
		return this->StartDate;
	}
	
	CFaction *GetFaction() const
	{
		return this->Faction;
	}

	std::string GetSpecies() const;

	bool IsHidden() const
	{
		return this->Hidden;
	}

	bool IsAvailable() const;

private:
	std::string Description;		/// Description of the campaign
	CDate StartDate;				/// The starting date of the campaign
	bool Hidden = false;			/// Whether the campaign is hidden
	bool Sandbox = false;			/// Whether the campaign is a sandbox one
	std::vector<CQuest *> RequiredQuests;		/// Quests required by the campaign
	CFaction *Faction = nullptr;	/// Which faction the player plays as in the campaign
public:
	std::vector<map_template *> map_templates; //map templates used by the campaign
	std::vector<Vec2i> MapSizes;				/// Map sizes
	std::vector<Vec2i> MapTemplateStartPos;		/// Map template position the map will start on
	
	friend int ::CclDefineCampaign(lua_State *l);
	friend int ::CclGetCampaignData(lua_State *l);
};

}

extern void SetCurrentCampaign(const std::string &campaign_ident);
extern std::string GetCurrentCampaign();
