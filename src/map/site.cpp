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
/**@name site.cpp - The site source file. */
//
//      (c) Copyright 2018-2019 by Andrettin
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

#include "map/site.h"

#include "civilization.h"
#include "config.h"
#include "config_operator.h"
#include "faction.h"
#include "item/item.h"
#include "item/unique_item.h"
#include "language/word.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/map_template.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "world/plane.h"
#include "world/province.h" //for regions
#include "world/world.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CSite::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "position_x") {
		this->Position.x = std::stoi(value);
	} else if (key == "position_y") {
		this->Position.y = std::stoi(value);
	} else if (key == "map_template") {
		this->MapTemplate = CMapTemplate::Get(value);
	} else if (key == "core") {
		CFaction *faction = CFaction::Get(value);
		if (faction != nullptr) {
			this->Cores.push_back(faction);
			faction->Cores.push_back(this);
			faction->Sites.push_back(this);
			if (faction->GetCivilization()) {
				faction->GetCivilization()->Sites.push_back(this);
			}
		}
	} else if (key == "region") {
		value = FindAndReplaceString(value, "_", "-");
		
		CRegion *region = GetRegion(value);
		if (region != nullptr) {
			this->Regions.push_back(region);
			region->Sites.push_back(this);
		} else {
			fprintf(stderr, "Invalid region: \"%s\".\n", value.c_str());
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
bool CSite::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "cultural_names") {
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			const CCivilization *civilization = CCivilization::Get(property.Key);
			
			if (civilization) {
				this->CulturalNames[civilization] = property.Value.c_str();
			}
		}
	} else if (section->Tag == "cultural_name_words") {
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			const CCivilization *civilization = CCivilization::Get(property.Key);
			CWord *name_word = CWord::Get(property.Value);
			
			if (civilization != nullptr && name_word != nullptr) {
				this->CulturalNameWords[civilization] = name_word;
				name_word->ChangeSettlementNameWeight(1);
				
				if (this->CulturalNames.find(civilization) == this->CulturalNames.end()) {
					this->CulturalNames[civilization] = name_word->GetAnglicizedName();
				}
			}
		}
	} else if (section->Tag == "historical_owner") {
		CDate date;
		CFaction *owner_faction = nullptr;
		
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			if (property.Key == "date") {
				std::string value = FindAndReplaceString(property.Value, "_", "-");
				date = CDate::FromString(value);
			} else if (property.Key == "faction") {
				owner_faction = CFaction::Get(property.Value);
			} else {
				fprintf(stderr, "Invalid historical owner property: \"%s\".\n", property.Key.c_str());
			}
		}
		
		this->HistoricalOwners[date] = owner_faction;
	} else if (section->Tag == "historical_building") {
		CDate start_date;
		CDate end_date;
		const UnitClass *building_class = nullptr;
		UniqueItem *unique = nullptr;
		CFaction *building_owner_faction = nullptr;
			
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			if (property.Key == "start_date") {
				std::string value = FindAndReplaceString(property.Value, "_", "-");
				start_date = CDate::FromString(value);
			} else if (property.Key == "end_date") {
				std::string value = FindAndReplaceString(property.Value, "_", "-");
				end_date = CDate::FromString(value);
			} else if (property.Key == "building_class") {
				building_class = UnitClass::Get(property.Value);
			} else if (property.Key == "unique") {
				std::string value = FindAndReplaceString(property.Value, "_", "-");
				unique = UniqueItem::Get(value);
				if (unique != nullptr) {
					if (building_class == nullptr) {
						building_class = unique->Type->GetClass();
					}
				} else {
					fprintf(stderr, "Invalid unique: \"%s\".\n", value.c_str());
				}
			} else if (property.Key == "faction") {
				building_owner_faction = CFaction::Get(property.Value);
			} else {
				fprintf(stderr, "Invalid historical building property: \"%s\".\n", property.Key.c_str());
			}
		}
		
		if (building_class == nullptr) {
			fprintf(stderr, "Historical building has no building class.\n");
			return true;
		}
		
		this->HistoricalBuildings.push_back(std::tuple<CDate, CDate, const UnitClass *, UniqueItem *, CFaction *>(start_date, end_date, building_class, unique, building_owner_faction));
	} else if (section->Tag == "historical_population") {
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			std::string key = FindAndReplaceString(property.Key, "_", "-");
			const CDate date = CDate::FromString(key);
			this->HistoricalPopulation[date] = std::stoi(property.Value);
		}
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Initialize the site
*/
void CSite::Initialize()
{
	if (!this->Major && !this->Cores.empty()) { //if the site is a minor one, but has faction cores, remove them
		for (CFaction *core_faction : this->Cores) {
			core_faction->Cores.erase(std::remove(core_faction->Cores.begin(), core_faction->Cores.end(), this), core_faction->Cores.end());
		}
		this->Cores.clear();
	}
	
	if (this->MapTemplate) {
		if (this->Position.x != -1 && this->Position.y != -1) {
			if (this->MapTemplate->SitesByPosition.find(std::pair<int, int>(this->Position.x, this->Position.y)) == this->MapTemplate->SitesByPosition.end()) {
				this->MapTemplate->SitesByPosition[std::pair<int, int>(this->Position.x, this->Position.y)] = this;
			} else {
				fprintf(stderr, "Position (%d, %d) of map template \"%s\" already has a site.", this->Position.x, this->Position.y, this->MapTemplate->Ident.c_str());
			}
		}
		
		this->MapTemplate->Sites.push_back(this);
	}
	
	this->Initialized = true;
}

/**
**	@brief	Get the site's cultural name
**
**	@param	civilization	The civilization for which to get the cultural name
**
**	@return	The cultural name if present, or the default name otherwise
*/
const String &CSite::GetCulturalName(const CCivilization *civilization) const
{
	if (civilization != nullptr) {
		std::map<const CCivilization *, String>::const_iterator find_iterator = this->CulturalNames.find(civilization);
		if (find_iterator != this->CulturalNames.end()) {
			return find_iterator->second;
		}
	}
	
	return this->GetName();
}

/**
**	@brief	Get the position of the site on the current map
**
**	@return	The site's position on the current map (an invalid position is returned if the site isn't on the map)
*/
Vec2i CSite::GetMapPos() const
{
	if (this->SiteUnit != nullptr) {
		return this->SiteUnit->tilePos;
	}
	
	if (this->MapTemplate != nullptr && this->Position.x != -1 && this->Position.y != -1) {
		Vec2i template_start_pos(-1, -1);
		Vec2i template_end_pos(-1, -1);
		
		if (this->MapTemplate->IsSubtemplateArea()) {
			template_start_pos = CMap::Map.GetSubtemplatePos(this->MapTemplate);
			template_end_pos = CMap::Map.GetSubtemplateEndPos(this->MapTemplate);
		} else {
			const CMapLayer *map_layer = CMap::Map.GetMapLayer(this->MapTemplate->GetPlane(), this->MapTemplate->GetWorld(), this->MapTemplate->GetSurfaceLayer());
			if (map_layer != nullptr) {
				template_start_pos = Vec2i(0, 0);
				template_end_pos.x = map_layer->GetWidth() - 1;
				template_end_pos.y = map_layer->GetHeight() - 1;
			}
		}
		
		if (template_start_pos.x != -1 && template_start_pos.y != -1) {
			Vec2i site_map_pos = template_start_pos - this->MapTemplate->CurrentStartPos + this->Position;
			
			if (site_map_pos.x >= template_start_pos.x && site_map_pos.y >= template_start_pos.y && site_map_pos.x <= template_end_pos.x && site_map_pos.y <= template_end_pos.y) {
				return site_map_pos;
			}
		}
	}
	
	return Vec2i(-1, -1);
}

/**
**	@brief	Get the site's map layer on the current map
**
**	@return	The site's map layer on the current map (null is returned if the site isn't on the map)
*/
CMapLayer *CSite::GetMapLayer() const
{
	if (this->SiteUnit != nullptr) {
		return this->SiteUnit->MapLayer;
	}
	
	if (this->MapTemplate != nullptr) {
		if (this->MapTemplate->IsSubtemplateArea()) {
			return CMap::Map.GetSubtemplateMapLayer(this->MapTemplate);
		} else {
			CMapLayer *map_layer = CMap::Map.GetMapLayer(this->MapTemplate->GetPlane(), this->MapTemplate->GetWorld(), this->MapTemplate->GetSurfaceLayer());
			return map_layer;
		}
	}
	
	return nullptr;
}

void CSite::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_name_word", "name_word_ident"), [](CSite *site, const String &name_word_ident){
		CWord *name_word = CWord::Get(name_word_ident);
		if (name_word != nullptr) {
			site->NameWord = name_word;
			name_word->ChangeSettlementNameWeight(1);
			if (site->Name.empty()) {
				site->Name = name_word->GetAnglicizedName();
			}
		}
	});
	ClassDB::bind_method(D_METHOD("get_name_word"), [](const CSite *site){ return const_cast<CWord *>(site->NameWord); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "name_word"), "set_name_word", "get_name_word");

	ClassDB::bind_method(D_METHOD("set_major", "major"), [](CSite *site, const bool major){ site->Major = major; });
	ClassDB::bind_method(D_METHOD("is_major"), &CSite::IsMajor);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "major"), "set_major", "is_major");
}
