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
/**@name campaign.h - The campaign header file. */
//
//      (c) Copyright 2019 by Andrettin
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

#ifndef __CAMPAIGN_H__
#define __CAMPAIGN_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"
#include "time/date.h"
#include "vec2i.h"

#include <core/object.h>

#include <shared_mutex>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CFaction;
class CMapTemplate;
class CQuest;
class CSpecies;
struct lua_State;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CCampaign : public DataElement, public DataType<CCampaign>
{
	DATA_TYPE(CCampaign, DataElement)
	
public:
	static constexpr const char *ClassIdentifier = "campaign";
	
	static void SetCurrentCampaign(CCampaign *campaign);
	static CCampaign *GetCurrentCampaign();

private:
	static CCampaign *CurrentCampaign;
	static std::shared_mutex CampaignMutex;	/// Mutex for campaigns as a whole
	
public:
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	virtual void Initialize() override;
	
	const String &GetDescription() const
	{
		return this->Description;
	}
	
	const CDate &GetStartDate() const
	{
		return this->StartDate;
	}
	
	CFaction *GetFaction() const
	{
		return this->Faction;
	}
	
	CSpecies *GetSpecies() const;
	
	bool IsAvailable() const;

	bool IsHidden() const
	{
		return this->Hidden;
	}

	bool IsSandbox() const
	{
		return this->Sandbox;
	}
	
	const CQuest *GetCompletionQuest() const
	{
		return this->CompletionQuest;
	}
	
	Vec2i GetMapSize(const int z) const;
	
private:
	String Description;		/// description of the campaign
	CDate StartDate;		/// the starting date of the campaign
	bool Hidden = false;	/// whether the campaign is hidden
	bool Sandbox = false;	/// whether the campaign is a sandbox one
private:
	std::vector<CQuest *> RequiredQuests;	/// quests required for the campaign to be available
	const CQuest *CompletionQuest = nullptr;	/// the quest on which the campaign depends for completion, or failure
	CFaction *Faction = nullptr;	/// which faction the player plays as in the campaign
public:
	std::vector<CMapTemplate *> MapTemplates;	/// map templates used by the campaign
private:
	std::vector<Vec2i> MapSizes;				/// map sizes
public:
	std::vector<Vec2i> MapTemplateStartPos;		/// map template position the map will start on
	
	friend int CclDefineCampaign(lua_State *l);
	friend int CclGetCampaignData(lua_State *l);
	friend int CclGetCampaigns(lua_State *l);

protected:
	static void _bind_methods();
};

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void SetCurrentCampaign(const std::string &campaign_ident);
extern std::string GetCurrentCampaign();

#endif
