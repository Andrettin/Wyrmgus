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
/**@name campaign.cpp - The campaign source file. */
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "quest/campaign.h"

#include "civilization.h"
#include "config.h"
#include "config_operator.h"
#include "faction.h"
#include "map/map_template.h"
#include "quest/quest.h"
#include "species/species.h"

#include <mutex>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

CCampaign *CCampaign::CurrentCampaign = nullptr;
std::shared_mutex CCampaign::CampaignMutex;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Set the current campaign
*/
void CCampaign::SetCurrentCampaign(CCampaign *campaign)
{
	std::unique_lock<std::shared_mutex> lock(CampaignMutex);
	
	if (campaign == CCampaign::CurrentCampaign) {
		return;
	}
	
	CCampaign::CurrentCampaign = campaign;
}

/**
**	@brief	Get the current campaign
*/
CCampaign *CCampaign::GetCurrentCampaign()
{
	std::shared_lock<std::shared_mutex> lock(CampaignMutex);
	
	return CCampaign::CurrentCampaign;
}

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CCampaign::ProcessConfigDataProperty(const String &key, String value)
{
	if (key == "faction") {
		CFaction *faction = CFaction::Get(value);
		if (faction != nullptr) {
			this->Faction = faction;
		}
	} else if (key == "start_date") {
		value = value.replace("_", "-");
		this->StartDate = CDate::FromString(value.utf8().get_data());
	} else if (key == "required_quest") {
		value = value.replace("_", "-");
		const CQuest *quest = CQuest::Get(value);
		if (quest != nullptr) {
			this->RequiredQuests.push_back(quest);
		}
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CCampaign::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "map_template") {
		CMapTemplate *map_template = nullptr;
		Vec2i start_pos(0, 0);
		Vec2i map_size(0, 0);
		
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				print_error("Wrong operator enumeration index for property \"" + property.Key + "\": " + String::num_int64(static_cast<int>(property.Operator)) + ".");
				continue;
			}
			
			String key = property.Key;
			String value = property.Value;
			
			if (key == "map_template") {
				map_template = CMapTemplate::Get(value);
			} else if (key == "start_x") {
				start_pos.x = value.to_int();
			} else if (key == "start_y") {
				start_pos.y = value.to_int();
			} else if (key == "width") {
				map_size.x = value.to_int();
			} else if (key == "height") {
				map_size.y = value.to_int();
			} else {
				fprintf(stderr, "Invalid image property: \"%s\".\n", key.utf8().get_data());
			}
		}
		
		if (!map_template) {
			fprintf(stderr, "Campaign map template has no map template.\n");
			return true;
		}
		
		this->MapTemplates.push_back(map_template);
		this->MapTemplateStartPos.push_back(start_pos);
		this->MapSizes.push_back(map_size);
	} else {
		return false;
	}
	
	return true;
}
	
/**
**	@brief	Initialize the campaign
*/
void CCampaign::Initialize()
{
	this->Initialized = true;
	
	if (CCampaign::AreAllInitialized()) {
		std::sort(CCampaign::Instances.begin(), CCampaign::Instances.end(), [](CCampaign *a, CCampaign *b) {
			std::string species_ident_a = a->GetSpecies() ? a->GetSpecies()->GetIdent().utf8().get_data() : "";
			std::string species_ident_b = b->GetSpecies() ? b->GetSpecies()->GetIdent().utf8().get_data() : "";
			if (species_ident_a != species_ident_b) {
				return species_ident_a < species_ident_b;
			} else if (a->StartDate != b->StartDate) {
				return a->StartDate < b->StartDate;
			} else {
				return a->Ident < b->Ident;
			}
		});
		CCampaign::UpdateIndexes();
	}
}

CSpecies *CCampaign::GetSpecies() const
{
	if (this->Faction && this->Faction->GetCivilization() != nullptr) {
		return this->Faction->GetCivilization()->GetSpecies();
	}
	
	return nullptr;
}

bool CCampaign::IsAvailable() const
{
	if (this->Hidden) {
		return false;
	}
	
	for (const CQuest *quest : this->RequiredQuests) {
		if (!quest->IsCompleted()) {
			return false;
		}
	}
	
	return true;
}

Vec2i CCampaign::GetMapSize(const int z) const
{
	if (this->MapSizes[z].x != 0 && this->MapSizes[z].y != 0) {
		return this->MapSizes[z];
	}
	
	return Vec2i(this->MapTemplates[z]->GetWidth(), this->MapTemplates[z]->GetHeight());
}

bool CCampaign::HasMapTemplateForLayer(const CPlane *plane, const CWorld *world, const int surface_layer) const
{
	for (const CMapTemplate *map_template : this->MapTemplates) {
		if (map_template->GetPlane() == plane && map_template->GetWorld() == world && map_template->GetSurfaceLayer() == surface_layer) {
			return true;
		}
	}
	
	return false;
}

void CCampaign::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_description", "description"), +[](CCampaign *campaign, const String &description){ campaign->Description = description; });
	ClassDB::bind_method(D_METHOD("get_description"), &CCampaign::GetDescription);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "description"), "set_description", "get_description");
	
	ClassDB::bind_method(D_METHOD("set_hidden", "hidden"), +[](CCampaign *campaign, const bool hidden){ campaign->Hidden = hidden; });
	ClassDB::bind_method(D_METHOD("is_hidden"), &CCampaign::IsHidden);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hidden"), "set_hidden", "is_hidden");
	
	ClassDB::bind_method(D_METHOD("set_sandbox", "sandbox"), +[](CCampaign *campaign, const bool sandbox){ campaign->Sandbox = sandbox; });
	ClassDB::bind_method(D_METHOD("is_sandbox"), &CCampaign::IsSandbox);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sandbox"), "set_sandbox", "is_sandbox");
	
	ClassDB::bind_method(D_METHOD("set_completion_quest", "completion_quest_ident"), +[](CCampaign *campaign, const String &completion_quest_ident){ campaign->CompletionQuest = CQuest::Get(completion_quest_ident); });
	ClassDB::bind_method(D_METHOD("get_completion_quest"), +[](const CCampaign *campaign){ return const_cast<CQuest *>(campaign->GetCompletionQuest()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "completion_quest"), "set_completion_quest", "get_completion_quest");
	
	ClassDB::bind_method(D_METHOD("is_available"), &CCampaign::IsAvailable);
}

/**
**	@brief	Set the current campaign
**
**	@param	campaign_ident	The campaign's string identifier
*/
void SetCurrentCampaign(const std::string &campaign_ident)
{
	if (campaign_ident.empty()) {
		CCampaign::SetCurrentCampaign(nullptr);
		return;
	}
	
	CCampaign *campaign = CCampaign::Get(campaign_ident);
	
	if (!campaign) {
		return;
	}
	
	CCampaign::SetCurrentCampaign(campaign);
}

/**
**	@brief	Get the current campaign
**
**	@return	The string identifier of the current campaign
*/
std::string GetCurrentCampaign()
{
	const CCampaign *current_campaign = CCampaign::GetCurrentCampaign();
	
	if (!current_campaign) {
		return "";
	} else {
		return current_campaign->GetIdent().utf8().get_data();
	}
}
