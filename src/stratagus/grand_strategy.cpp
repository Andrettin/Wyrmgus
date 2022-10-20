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
//      (c) Copyright 2015-2022 by Andrettin
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

#include "grand_strategy.h"

#include "character.h"
#include "character_title.h"
#include "database/defines.h"
#include "game/game.h" //for loading screen elements
#include "iolib.h"
#include "luacallback.h"
#include "menus.h"
#include "player/civilization.h"
#include "player/faction.h"
#include "player/faction_type.h"
#include "player/player.h"
#include "results.h"
#include "script.h"
#include "sound/sound_server.h"
#include "ui/cursor.h"
#include "ui/interface.h"
#include "ui/resource_icon.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_class.h"
#include "upgrade/upgrade_modifier.h"
#include "util/gender.h"
#include "util/util.h"
#include "video/font.h"	// for grand strategy mode tooltip drawing
#include "video/video.h"

bool GrandStrategy = false;				///if the game is in grand strategy mode
bool GrandStrategyGameLoading = false;
int GrandStrategyYear = 0;
CGrandStrategyGame GrandStrategyGame;
std::map<std::string, int> GrandStrategyHeroStringToIndex;
std::vector<std::unique_ptr<CGrandStrategyEvent>> GrandStrategyEvents;
std::map<std::string, CGrandStrategyEvent *> GrandStrategyEventStringToPointer;

void CGrandStrategyGame::DrawInterface(std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (this->PlayerFaction != nullptr && this->PlayerFaction->OwnedProvinces.size() > 0) { //draw resource bar
		std::vector<int> stored_resources;
		stored_resources.push_back(CopperCost);
		stored_resources.push_back(WoodCost);
		stored_resources.push_back(StoneCost);
		stored_resources.push_back(ResearchCost);
		stored_resources.push_back(PrestigeCost);

		Vec2i hovered_research_icon(-1, -1);
		Vec2i hovered_prestige_icon(-1, -1);
		for (size_t i = 0; i < stored_resources.size(); ++i) {
			int x = 154 + (100 * i);
			int y = 0;
			const wyrmgus::resource *resource = wyrmgus::resource::get_all()[stored_resources[i]];
			const wyrmgus::resource_icon *icon = resource->get_icon();
			const std::shared_ptr<CGraphic> &icon_graphics = icon->get_graphics();
			icon_graphics->DrawFrameClip(icon->get_frame(), x, y, nullptr, render_commands);
			
			int quantity_stored = this->PlayerFaction->Resources[stored_resources[i]];
			int income = 0;
			if (stored_resources[i] == CopperCost) {
				income = this->PlayerFaction->Income[stored_resources[i]];
			} else if (stored_resources[i] == ResearchCost || stored_resources[i] == LeadershipCost) {
				income = this->PlayerFaction->Income[stored_resources[i]] / this->PlayerFaction->OwnedProvinces.size();
			} else {
				income = this->PlayerFaction->Income[stored_resources[i]];
			}
			std::string income_string;
			if (income != 0) {
				if (income > 0) {
					income_string += "+";
				}
				income_string += std::to_string(income);
			}
			std::string resource_stored_string = std::to_string(quantity_stored) + income_string;
			
			if (resource_stored_string.size() <= 9) {
				CLabel(defines::get()->get_game_font()).Draw(x + 18, y + 1, resource_stored_string, render_commands);
			} else {
				CLabel(defines::get()->get_small_font()).Draw(x + 18, y + 1 + 2, resource_stored_string, render_commands);
			}
			
			if (CursorScreenPos.x >= x && CursorScreenPos.x <= (x + icon_graphics->get_width()) && CursorScreenPos.y >= y && CursorScreenPos.y <= (y + icon_graphics->get_height())) {
				if (stored_resources[i] == ResearchCost) {
					hovered_research_icon.x = x;
					hovered_research_icon.y = y;
				} else if (stored_resources[i] == PrestigeCost) {
					hovered_prestige_icon.x = x;
					hovered_prestige_icon.y = y;
				}
			}
		}
		if (hovered_research_icon.x != -1 && hovered_research_icon.y != -1) {
			DrawGenericPopup("Gain Research by building town halls, lumber mills, smithies and temples", hovered_research_icon.x, hovered_research_icon.y, render_commands);
		} else if (hovered_prestige_icon.x != -1 && hovered_prestige_icon.y != -1) {
			DrawGenericPopup("Prestige influences trade priority between nations, and factions with negative prestige cannot declare war", hovered_prestige_icon.x, hovered_prestige_icon.y, render_commands);
		}
	}
}

void CGrandStrategyGame::DoTurn()
{
	for (size_t i = 0; i < this->Provinces.size(); ++i) {
		if (this->Provinces[i]->civilization != -1) { // if this province has a culture
			if (this->Provinces[i]->Owner != nullptr) {
				if (!this->Provinces[i]->HasFactionClaim(this->Provinces[i]->Owner->civilization, this->Provinces[i]->Owner->Faction) && SyncRand(100) < 1) { // 1% chance the owner of this province will get a claim on it
					this->Provinces[i]->AddFactionClaim(this->Provinces[i]->Owner->civilization, this->Provinces[i]->Owner->Faction);
				}
					
				if (SyncRand(1000) == 0) { // 0.1% chance per year that a (randomly generated) literary work will be created in a province
					this->CreateWork(nullptr, nullptr, this->Provinces[i]);
				}
			}
		}
	}
	
	// check if any literary works should be published this year
	int works_size = this->UnpublishedWorks.size();
	for (int i = (works_size - 1); i >= 0; --i) {
		CGrandStrategyHero *author = nullptr;
		if (this->UnpublishedWorks[i]->Author != nullptr) {
			author = this->GetHero(this->UnpublishedWorks[i]->Author->get_full_name());
			if (author != nullptr && !author->IsAlive()) {
				continue;
			}
		}
			
		if ((author == nullptr && SyncRand(200) != 0) || (author != nullptr && SyncRand(10) != 0)) { // 0.5% chance per year that a work will be published if no author is preset, and 10% if an author is preset
			continue;
		}
		
		int civilization = this->UnpublishedWorks[i]->get_civilization()->ID;
		if (
			(author != nullptr && author->ProvinceOfOrigin != nullptr)
			|| (civilization != -1 && this->CultureProvinces.find(civilization) != this->CultureProvinces.end() && this->CultureProvinces[civilization].size() > 0)
		) {
			bool characters_existed = true;
			for (size_t j = 0; j < this->UnpublishedWorks[i]->Characters.size(); ++j) {
				CGrandStrategyHero *hero = this->GetHero(this->UnpublishedWorks[i]->Characters[j]->get_full_name());
				
				if (hero == nullptr || !hero->Existed) {
					characters_existed = false;
					break;
				}
			}
			if (!characters_existed) {
				continue;
			}
			
			if (author != nullptr && author->Province != nullptr) {
				this->CreateWork(this->UnpublishedWorks[i], author, author->Province);
			} else {
				this->CreateWork(this->UnpublishedWorks[i], author, this->CultureProvinces[civilization][SyncRand(this->CultureProvinces[civilization].size())]);
			}
		}
	}
}

void CGrandStrategyGame::PerformTrade(CGrandStrategyFaction &importer_faction, CGrandStrategyFaction &exporter_faction, int resource)
{
	if (abs(importer_faction.Trade[resource]) > exporter_faction.Trade[resource]) {
		importer_faction.Resources[resource] += exporter_faction.Trade[resource];
		exporter_faction.Resources[resource] -= exporter_faction.Trade[resource];

		importer_faction.Resources[CopperCost] -= exporter_faction.Trade[resource] * this->CommodityPrices[resource] / 100;
		exporter_faction.Resources[CopperCost] += exporter_faction.Trade[resource] * this->CommodityPrices[resource] / 100;
		
		importer_faction.Trade[resource] += exporter_faction.Trade[resource];
		exporter_faction.Trade[resource] = 0;
	} else {
		importer_faction.Resources[resource] += abs(importer_faction.Trade[resource]);
		exporter_faction.Resources[resource] -= abs(importer_faction.Trade[resource]);

		importer_faction.Resources[CopperCost] -= abs(importer_faction.Trade[resource]) * this->CommodityPrices[resource] / 100;
		exporter_faction.Resources[CopperCost] += abs(importer_faction.Trade[resource]) * this->CommodityPrices[resource] / 100;
		
		exporter_faction.Trade[resource] += importer_faction.Trade[resource];
		importer_faction.Trade[resource] = 0;
	}
}

void CGrandStrategyGame::CreateWork(CUpgrade *work, CGrandStrategyHero *author, CGrandStrategyProvince *province)
{
	if (!province || !province->Owner) {
		return;
	}

	if (!province->Owner->HasTechnologyClass("writing")) {
		//only factions which have knowledge of writing can produce literary works
		return;
	}

	if (work != nullptr) {
		this->UnpublishedWorks.erase(std::remove(this->UnpublishedWorks.begin(), this->UnpublishedWorks.end(), work), this->UnpublishedWorks.end()); // remove work from the vector, so that it doesn't get created again
	}

	std::string work_name;
	if (work != nullptr) {
		work_name = work->get_name();
	} else {
		work_name = province->GenerateWorkName();
		if (work_name.empty()) {
			return;
		}
	}
	
	if (author == nullptr) {
		author = province->GetRandomAuthor();
	}
	
	if (province->Owner == GrandStrategyGame.PlayerFaction || work != nullptr) { // only show foreign works that are predefined
		std::string work_creation_message = "if (GenericDialog ~= nil) then GenericDialog(\"" + work_name + "\", \"";
		if (author != nullptr) {
			work_creation_message += "The " + FullyDecapitalizeString(author->get_unit_type()->get_name()) + " " + author->get_full_name() + " ";
		} else {
			work_creation_message += "A sage ";
		}
		work_creation_message += "has written the literary work \\\"" + work_name + "\\\" in ";
		if (province->Owner != GrandStrategyGame.PlayerFaction) {
			work_creation_message += "the foreign lands of ";
		}
		work_creation_message += province->Name + "!";
		if (work != nullptr && !work->get_description().empty()) {
			work_creation_message += " " + FindAndReplaceString(FindAndReplaceString(work->get_description(), "\"", "\\\""), "\n", "\\n");
		}
		if (work != nullptr && !work->get_quote().empty()) {
			work_creation_message += "\\n\\n" + FindAndReplaceString(FindAndReplaceString(work->get_quote(), "\"", "\\\""), "\n", "\\n");
		}
		work_creation_message += "\"";
		if (province->Owner == GrandStrategyGame.PlayerFaction) {
			work_creation_message += ", \"+1 Prestige\"";
		}
		work_creation_message += ") end;";
		CclCommand(work_creation_message);
	}
	
	province->Owner->Resources[PrestigeCost] += 1;
}

bool CGrandStrategyGame::TradePriority(CGrandStrategyFaction &faction_a, CGrandStrategyFaction &faction_b)
{
	return faction_a.Resources[PrestigeCost] > faction_b.Resources[PrestigeCost];
}

CGrandStrategyHero *CGrandStrategyGame::GetHero(std::string hero_full_name)
{
	if (!hero_full_name.empty() && GrandStrategyHeroStringToIndex.find(hero_full_name) != GrandStrategyHeroStringToIndex.end()) {
		return this->Heroes[GrandStrategyHeroStringToIndex[hero_full_name]];
	} else {
		return nullptr;
	}
}

void CGrandStrategyProvince::SetOwner(int civilization_id, int faction_id)
{
	//if new owner is the same as the current owner, return
	if (
		(this->Owner != nullptr && this->Owner->civilization == civilization_id && this->Owner->Faction == faction_id)
		|| (this->Owner == nullptr && civilization_id == -1 && faction_id == -1)
	) {
		return;
	}
	
	if (this->Owner != nullptr) { //if province has a previous owner, remove it from the owner's province list
		this->Owner->OwnedProvinces.erase(std::remove(this->Owner->OwnedProvinces.begin(), this->Owner->OwnedProvinces.end(), this->ID), this->Owner->OwnedProvinces.end());
	}
	
	for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) { //change the province's military score to be appropriate for the new faction's technologies
		if (unit_type->is_template()) {
			continue;
		}

		if (IsMilitaryUnit(*unit_type)) {
			int old_owner_military_score_bonus = (this->Owner != nullptr ? this->Owner->MilitaryScoreBonus[unit_type->Slot] : 0);
			int new_owner_military_score_bonus = (faction_id != -1 ? GrandStrategyGame.Factions[civilization_id][faction_id]->MilitaryScoreBonus[unit_type->Slot] : 0);
			if (old_owner_military_score_bonus != new_owner_military_score_bonus) {
				this->MilitaryScore += this->Units[unit_type->Slot] * (new_owner_military_score_bonus - old_owner_military_score_bonus);
				this->OffensiveMilitaryScore += this->Units[unit_type->Slot] * new_owner_military_score_bonus - old_owner_military_score_bonus;
			}
		} else if (unit_type->get_unit_class() != nullptr && unit_type->get_unit_class()->get_identifier() == "worker") {
			const wyrmgus::unit_type *militia_unit_type = unit_type->get_civilization()->get_class_unit_type(wyrmgus::unit_class::get("militia"));
			if (militia_unit_type != nullptr) {
				int old_owner_military_score_bonus = (this->Owner != nullptr ? this->Owner->MilitaryScoreBonus[militia_unit_type->Slot] : 0);
				int new_owner_military_score_bonus = (faction_id != -1 ? GrandStrategyGame.Factions[civilization_id][faction_id]->MilitaryScoreBonus[militia_unit_type->Slot] : 0);
				if (old_owner_military_score_bonus != new_owner_military_score_bonus) {
					this->MilitaryScore += this->Units[unit_type->Slot] * ((new_owner_military_score_bonus - old_owner_military_score_bonus) / 2);
				}
			}
		}
	}

	if (civilization_id != -1 && faction_id != -1) {
		this->Owner = GrandStrategyGame.Factions[civilization_id][faction_id];
		this->Owner->OwnedProvinces.push_back(this->ID);
	} else {
		this->Owner = nullptr;
	}
}

void CGrandStrategyProvince::SetSettlementBuilding(int building_id, bool has_settlement_building)
{
	if (building_id == -1) {
		fprintf(stderr, "Tried to set invalid building type for the settlement of \"%s\".\n", this->Name.c_str());
		return;
	}
	
	if (this->SettlementBuildings[building_id] == has_settlement_building) {
		return;
	}
	
	this->SettlementBuildings[building_id] = has_settlement_building;
}

void CGrandStrategyProvince::SetModifier(CUpgrade *modifier, bool has_modifier)
{
	if (this->HasModifier(modifier) == has_modifier) { // current situation already corresponds to has_modifier setting
		return;
	}

	if (has_modifier) {
		this->Modifiers.push_back(modifier);
	} else {
		this->Modifiers.erase(std::remove(this->Modifiers.begin(), this->Modifiers.end(), modifier), this->Modifiers.end());
	}
}

void CGrandStrategyProvince::SetUnitQuantity(int unit_type_id, int quantity)
{
	if (unit_type_id == -1) {
		return;
	}
	
//	fprintf(stderr, "Setting the quantity of unit type \"%s\" to %d in the %s province.\n", UnitTypes[unit_type_id]->Ident.c_str(), quantity, this->Name.c_str());
	
	quantity = std::max(0, quantity);
	
	int change = quantity - this->Units[unit_type_id];
	
	this->TotalUnits += change;
	
	if (IsMilitaryUnit(*wyrmgus::unit_type::get_all()[unit_type_id])) {
		this->MilitaryScore += change * (wyrmgus::unit_type::get_all()[unit_type_id]->DefaultStat.Variables[POINTS_INDEX].Value + (this->Owner != nullptr ? this->Owner->MilitaryScoreBonus[unit_type_id] : 0));
		this->OffensiveMilitaryScore += change * (wyrmgus::unit_type::get_all()[unit_type_id]->DefaultStat.Variables[POINTS_INDEX].Value + (this->Owner != nullptr ? this->Owner->MilitaryScoreBonus[unit_type_id] : 0));
	}
	
	if (wyrmgus::unit_type::get_all()[unit_type_id]->get_unit_class() != nullptr && wyrmgus::unit_type::get_all()[unit_type_id]->get_unit_class()->get_identifier() == "worker") {
		this->TotalWorkers += change;
		
		//if this unit's civilization can change workers into militia, add half of the militia's points to the military score (one in every two workers becomes a militia when the province is attacked)
		const wyrmgus::unit_type *militia_unit_type = wyrmgus::unit_type::get_all()[unit_type_id]->get_civilization()->get_class_unit_type(wyrmgus::unit_class::get("militia"));
		if (militia_unit_type != nullptr) {
			this->MilitaryScore += change * ((militia_unit_type->DefaultStat.Variables[POINTS_INDEX].Value + (this->Owner != nullptr ? this->Owner->MilitaryScoreBonus[militia_unit_type->Slot] : 0)) / 2);
		}
	}
	
	this->Units[unit_type_id] = quantity;
}

void CGrandStrategyProvince::ChangeUnitQuantity(int unit_type_id, int quantity)
{
	this->SetUnitQuantity(unit_type_id, this->Units[unit_type_id] + quantity);
}

void CGrandStrategyProvince::SetHero(std::string hero_full_name, int value)
{
	if (value == 1) {
		this->Movement = true;
	}
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		//update the hero
		if (value == 0) {
			hero->Die();
			return;
		}
		hero->State = value;
	} else {
		//if the hero hasn't been defined yet, give an error message
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
		return;
	}
}

void CGrandStrategyProvince::AddFactionClaim(int civilization_id, int faction_id)
{
	this->Claims.push_back(GrandStrategyGame.Factions[civilization_id][faction_id]);
	GrandStrategyGame.Factions[civilization_id][faction_id]->Claims.push_back(this);
}

void CGrandStrategyProvince::RemoveFactionClaim(int civilization_id, int faction_id)
{
	this->Claims.erase(std::remove(this->Claims.begin(), this->Claims.end(), GrandStrategyGame.Factions[civilization_id][faction_id]), this->Claims.end());
	GrandStrategyGame.Factions[civilization_id][faction_id]->Claims.erase(std::remove(GrandStrategyGame.Factions[civilization_id][faction_id]->Claims.begin(), GrandStrategyGame.Factions[civilization_id][faction_id]->Claims.end(), this), GrandStrategyGame.Factions[civilization_id][faction_id]->Claims.end());
}

bool CGrandStrategyProvince::HasBuildingClass(std::string building_class_name)
{
	if (this->civilization == -1 || building_class_name.empty()) {
		return false;
	}
	
	int building_type = this->GetClassUnitType(wyrmgus::unit_class::get(building_class_name));
	
	if (building_type == -1 && building_class_name == "mercenary-camp") { //special case for mercenary camps, which are a neutral building
		building_type = UnitTypeIdByIdent("unit-mercenary-camp");
	}
	
	if (building_type != -1 && this->SettlementBuildings[building_type] == true) {
		return true;
	}

	return false;
}

bool CGrandStrategyProvince::HasModifier(CUpgrade *modifier)
{
	return std::find(this->Modifiers.begin(), this->Modifiers.end(), modifier) != this->Modifiers.end();
}

bool CGrandStrategyProvince::BordersModifier(CUpgrade *modifier)
{
	//see if any provinces bordering this one have the modifier (count provinces reachable through water as well)
	
	for (size_t i = 0; i < this->BorderProvinces.size(); ++i) {
		CGrandStrategyProvince *border_province = this->BorderProvinces[i];
		if (border_province->HasModifier(modifier)) {
			return true;
		}
	}
	
	return false;
}

bool CGrandStrategyProvince::HasFactionClaim(int civilization_id, int faction_id)
{
	for (size_t i = 0; i < this->Claims.size(); ++i) {
		if (this->Claims[i]->civilization == civilization_id && this->Claims[i]->Faction == faction_id) {
			return true;
		}
	}
	return false;
}

bool CGrandStrategyProvince::BordersProvince(CGrandStrategyProvince *province)
{
	for (size_t i = 0; i < this->BorderProvinces.size(); ++i) {
		if (this->BorderProvinces[i] == province) {
			return true;
		}
	}
	return false;
}

bool CGrandStrategyProvince::HasSecondaryBorderThroughWaterWith(CGrandStrategyProvince *province)
{
	for (size_t i = 0; i < this->BorderProvinces.size(); ++i) {
		CGrandStrategyProvince *border_province = this->BorderProvinces[i];
		if (border_province->Water) {
			for (size_t j = 0; j < border_province->BorderProvinces.size(); ++j) {
				if (border_province->BorderProvinces[j] == province) {
					return true;
				}
			}
		}
	}
	return false;
}

bool CGrandStrategyProvince::BordersFaction(int faction_civilization, int faction, bool check_through_water)
{
	for (size_t i = 0; i < this->BorderProvinces.size(); ++i) {
		CGrandStrategyProvince *border_province = this->BorderProvinces[i];
		if (border_province->Owner == nullptr) {
			if (border_province->Water && check_through_water) {
				for (size_t j = 0; j < border_province->BorderProvinces.size(); ++j) {
					if (
						border_province->BorderProvinces[j]->Owner != nullptr
						&& border_province->BorderProvinces[j]->Owner->civilization == faction_civilization
						&& border_province->BorderProvinces[j]->Owner->Faction == faction
					) {
						return true;
					}
				}
			}
			continue;
		}
		if (border_province->Owner->civilization == faction_civilization && border_province->Owner->Faction == faction) {
			return true;
		}
	}
	return false;
}

int CGrandStrategyProvince::GetClassUnitType(const wyrmgus::unit_class *unit_class)
{
	const wyrmgus::unit_type *unit_type = wyrmgus::civilization::get_all()[this->civilization]->get_class_unit_type(unit_class);

	if (unit_type != nullptr) {
		return unit_type->Slot;
	}

	return -1;
}

int CGrandStrategyProvince::GetDesirabilityRating()
{
	int desirability = 0;
	
	std::vector<int> resources;
	resources.push_back(CopperCost);
	resources.push_back(GoldCost);
	resources.push_back(SilverCost);
	resources.push_back(WoodCost);
	resources.push_back(StoneCost);
	
	for (size_t i = 0; i < resources.size(); ++i) {
		desirability += this->ProductionCapacity[resources[i]] * 100 * GrandStrategyGame.CommodityPrices[resources[i]] / 100;
	}
	
	if (this->Coastal) {
		desirability += 250;
	}
	
	return desirability;
}

std::string CGrandStrategyProvince::GenerateWorkName()
{
	if (this->Owner == nullptr) {
		return "";
	}
	
	std::string work_name;
	
	std::vector<CGrandStrategyHero *> potential_heroes;
	for (size_t i = 0; i < this->Owner->HistoricalMinisters[wyrmgus::character_title::ruler].size(); ++i) {
		potential_heroes.push_back(this->Owner->HistoricalMinisters[wyrmgus::character_title::ruler][i]);
	}
	
	if (potential_heroes.size() > 0 && SyncRand(10) != 0) { // 9 chances out of 10 that a literary work will use a hero's name as a basis
		work_name += potential_heroes[SyncRand(potential_heroes.size())]->get_name();
		if (work_name.substr(work_name.size() - 1, 1) != "s") {
			work_name += "s";
		}
		
		work_name += "mol";
	}
	
	return work_name;
}

CGrandStrategyHero *CGrandStrategyProvince::GetRandomAuthor()
{
	std::vector<CGrandStrategyHero *> potential_authors;
	
	for (size_t i = 0; i < this->Heroes.size(); ++i) {
		if (this->Heroes[i]->get_unit_type()->get_unit_class() != nullptr && this->Heroes[i]->get_unit_type()->get_unit_class()->get_identifier() == "priest") { // first see if there are any heroes in the province from a unit class likely to produce scholarly works
			potential_authors.push_back(this->Heroes[i]);
		}
	}
	
	if (potential_authors.size() == 0) { // if the first loop through the province's heroes failed, try to get a hero from any class
		for (size_t i = 0; i < this->Heroes.size(); ++i) {
			potential_authors.push_back(this->Heroes[i]);
		}
	}
	
	if (potential_authors.size() > 0) { // if a hero was found in the province, return it as the result
		return potential_authors[SyncRand(potential_authors.size())];
	} else { // if no hero was found, generate one
		return nullptr;
	}
}

void CGrandStrategyFaction::SetTechnology(int upgrade_id, bool has_technology, bool secondary_setting)
{
	if (this->Technologies[upgrade_id] == has_technology) {
		return;
	}
	
	this->Technologies[upgrade_id] = has_technology;
	
	int change = has_technology ? 1 : -1;
		
	//add military score bonuses
	for (const auto &modifier : CUpgrade::get_all()[upgrade_id]->get_modifiers()) {
		for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
			if (unit_type->is_template()) {
				continue;
			}

			if (modifier->applies_to(unit_type)) {
				if (modifier->Modifier.Variables[POINTS_INDEX].Value) {
					this->MilitaryScoreBonus[unit_type->Slot] += modifier->Modifier.Variables[POINTS_INDEX].Value * change;
				}
			}
		}
	}
	
	if (!secondary_setting) { //if this technology is not being set as a result of another technology of the same class being researched
		if (has_technology) { //if value is true, mark technologies from other civilizations that are of the same class as researched too, so that the player doesn't need to research the same type of technology every time
			if (CUpgrade::get_all()[upgrade_id]->get_upgrade_class() != nullptr) {
				for (size_t i = 0; i < CUpgrade::get_all().size(); ++i) {
					if (CUpgrade::get_all()[upgrade_id]->get_upgrade_class() == CUpgrade::get_all()[i]->get_upgrade_class()) {
						this->SetTechnology(i, has_technology, true);
					}
				}
			}
		}
	}
}

void CGrandStrategyFaction::SetCapital(CGrandStrategyProvince *province)
{
	this->Capital = province;
}

void CGrandStrategyFaction::SetMinister(const wyrmgus::character_title title, std::string hero_full_name)
{
	if (this->Ministers[title] != nullptr && std::find(this->Ministers[title]->Titles.begin(), this->Ministers[title]->Titles.end(), std::make_pair(title, this)) != this->Ministers[title]->Titles.end()) { // remove from the old minister's array
		this->Ministers[title]->Titles.erase(std::remove(this->Ministers[title]->Titles.begin(), this->Ministers[title]->Titles.end(), std::make_pair(title, this)), this->Ministers[title]->Titles.end());
	}
			
	if (hero_full_name.empty()) {
		if (this->CanHaveSuccession(title, true)) { //if the minister died a violent death, wait until the next turn to replace him
			this->MinisterSuccession(title);
		} else {
			this->Ministers[title] = nullptr;
		}
	} else {
		CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
		if (hero) {
			this->Ministers[title] = hero;
			hero->Titles.push_back(std::make_pair(title, this));
			
			if (this->IsAlive()) {
				int titles_size = hero->Titles.size();
				for (int i = (titles_size - 1); i >= 0; --i) {
					if (!(hero->Titles[i].first == title && hero->Titles[i].second == this) && hero->Titles[i].first != wyrmgus::character_title::ruler) { // a character can only have multiple head of state titles, but not others
						hero->Titles[i].second->SetMinister(hero->Titles[i].first, "");
					}
				}
			}
		} else {
			fprintf(stderr, "Tried to make \"%s\" the \"%s\" of the \"%s\", but the hero doesn't exist.\n", hero_full_name.c_str(), enum_converter<character_title>::to_string(title).c_str(), this->get_full_name().c_str());
		}
		
		if (this == GrandStrategyGame.PlayerFaction) {
			std::string new_minister_message = "if (GenericDialog ~= nil) then GenericDialog(\"";
//			new_minister_message += this->GetCharacterTitle(title, this->Ministers[title]->Gender) + " " + this->Ministers[title]->get_full_name();
			new_minister_message += "\", \"";
//			new_minister_message += "A new " + FullyDecapitalizeString(this->GetCharacterTitle(title, this->Ministers[title]->Gender));
			if (title == wyrmgus::character_title::ruler) {
				new_minister_message += " has come to power in our realm, ";
			} else {
				new_minister_message += " has been appointed, ";
			}
			new_minister_message += this->Ministers[title]->get_full_name() + "!\\n\\n";
			new_minister_message += "Type: " + this->Ministers[title]->get_unit_type()->get_name() + "\\n" + "Trait: " + this->Ministers[title]->get_traits().at(0)->get_name() + "\\n" + +"\\n\\n" + this->Ministers[title]->GetMinisterEffectsString(title);
			new_minister_message += "\") end;";
			CclCommand(new_minister_message);	
		}
		
		if (std::find(this->HistoricalMinisters[title].begin(), this->HistoricalMinisters[title].end(), hero) == this->HistoricalMinisters[title].end()) {
			this->HistoricalMinisters[title].push_back(hero);
		}
		
		if (this->IsAlive() && (hero->Province == nullptr || hero->Province->Owner != this)) { // if the hero's province is not owned by this faction, move him to a random province owned by this faction
			this->GetRandomProvinceWeightedByPopulation()->SetHero(hero->get_full_name(), hero->State);
		}
	}
}

void CGrandStrategyFaction::MinisterSuccession(const wyrmgus::character_title title)
{
	if (
		this->Ministers[title] != nullptr
		&& (wyrmgus::faction::get_all()[this->Faction]->get_type() == wyrmgus::faction_type::tribe || this->government_type == wyrmgus::government_type::monarchy)
		&& title == wyrmgus::character_title::ruler
	) { //if is a tribe or a monarchical polity, try to perform ruler succession by descent
		for (size_t i = 0; i < this->Ministers[title]->Children.size(); ++i) {
			if (this->Ministers[title]->Children[i]->IsAlive() && this->Ministers[title]->Children[i]->IsVisible() && this->Ministers[title]->Children[i]->get_gender() == wyrmgus::gender::male) { //historically males have generally been given priority in throne inheritance (if not exclusivity), specially in the cultures currently playable in the game
				this->SetMinister(title, this->Ministers[title]->Children[i]->get_full_name());
				return;
			}
		}
		for (size_t i = 0; i < this->Ministers[title]->Siblings.size(); ++i) { // now check for male siblings of the current ruler
			if (this->Ministers[title]->Siblings[i]->IsAlive() && this->Ministers[title]->Siblings[i]->IsVisible() && this->Ministers[title]->Siblings[i]->get_gender() == wyrmgus::gender::male) {
				this->SetMinister(title, this->Ministers[title]->Siblings[i]->get_full_name());
				return;
			}
		}		
		for (size_t i = 0; i < this->Ministers[title]->Children.size(); ++i) { //check again for children, but now allow for inheritance regardless of gender
			if (this->Ministers[title]->Children[i]->IsAlive() && this->Ministers[title]->Children[i]->IsVisible()) {
				this->SetMinister(title, this->Ministers[title]->Children[i]->get_full_name());
				return;
			}
		}
		for (size_t i = 0; i < this->Ministers[title]->Siblings.size(); ++i) { //check again for siblings, but now allow for inheritance regardless of gender
			if (this->Ministers[title]->Siblings[i]->IsAlive() && this->Ministers[title]->Siblings[i]->IsVisible()) {
				this->SetMinister(title, this->Ministers[title]->Siblings[i]->get_full_name());
				return;
			}
		}
		
		// if no family successor was found, the title becomes extinct if it is only a titular one (an aristocratic title whose corresponding faction does not actually hold territory)
		if (!this->CanHaveSuccession(title, false) || title != wyrmgus::character_title::ruler) {
			this->Ministers[title] = nullptr;
			return;
		}
	}
	
	CGrandStrategyHero *best_candidate = nullptr;
	int best_score = 0;
			
	for (size_t i = 0; i < GrandStrategyGame.Heroes.size(); ++i) {
		if (
			GrandStrategyGame.Heroes[i]->IsAlive()
			&& GrandStrategyGame.Heroes[i]->IsVisible()
			&& (
				(GrandStrategyGame.Heroes[i]->Province != nullptr && GrandStrategyGame.Heroes[i]->Province->Owner == this)
				|| (GrandStrategyGame.Heroes[i]->Province == nullptr && GrandStrategyGame.Heroes[i]->ProvinceOfOrigin != nullptr && GrandStrategyGame.Heroes[i]->ProvinceOfOrigin->Owner == this)
			)
			&& !GrandStrategyGame.Heroes[i]->is_custom()
			&& GrandStrategyGame.Heroes[i]->IsEligibleForTitle(title)
		) {
			int score = GrandStrategyGame.Heroes[i]->GetTitleScore(title);
			if (score > best_score) {
				best_candidate = GrandStrategyGame.Heroes[i];
				best_score = score;
			}
		}
	}
	if (best_candidate != nullptr) {
		this->SetMinister(title, best_candidate->get_full_name());
		return;
	}

	this->Ministers[title] = nullptr;
}

bool CGrandStrategyFaction::IsAlive()
{
	return this->OwnedProvinces.size() > 0;
}

bool CGrandStrategyFaction::HasTechnologyClass(std::string technology_class_name)
{
	if (this->civilization == -1 || technology_class_name.empty()) {
		return false;
	}
	
	int technology_id = wyrmgus::faction::get_all()[this->Faction]->get_class_upgrade(wyrmgus::upgrade_class::get(technology_class_name))->ID;
	
	if (technology_id != -1 && this->Technologies[technology_id] == true) {
		return true;
	}

	return false;
}

bool CGrandStrategyFaction::CanHaveSuccession(const wyrmgus::character_title title, bool family_inheritance)
{
	if (!this->IsAlive() && (title != wyrmgus::character_title::ruler || !family_inheritance || wyrmgus::faction::get_all()[this->Faction]->get_type() == wyrmgus::faction_type::tribe || this->government_type != wyrmgus::government_type::monarchy)) { // head of state titles can be inherited even if their respective factions have no provinces, but if the line dies out then the title becomes extinct; tribal titles cannot be titular-only
		return false;
	}
	
	return true;
}

bool CGrandStrategyFaction::IsConquestDesirable(CGrandStrategyProvince *province)
{
	if (this->OwnedProvinces.size() == 1 && province->Owner == nullptr && wyrmgus::faction::get_all()[this->Faction]->get_type() == wyrmgus::faction_type::tribe) {
		if (province->GetDesirabilityRating() <= GrandStrategyGame.Provinces[this->OwnedProvinces[0]]->GetDesirabilityRating()) { // if conquering the province would trigger a migration, the conquest is only desirable if the province is worth more
			return false;
		}
	}
	
	return true;
}

int CGrandStrategyFaction::GetTroopCostModifier()
{
	int modifier = 0;
	
	if (this->Ministers[wyrmgus::character_title::ruler] != nullptr) {
		modifier += this->Ministers[character_title::ruler]->GetTroopCostModifier();
	}
	
	if (this->Ministers[wyrmgus::character_title::marshal] != nullptr) {
		modifier += this->Ministers[character_title::marshal]->GetTroopCostModifier();
	}
	
	return modifier;
}

std::string CGrandStrategyFaction::get_full_name()
{
	if (wyrmgus::faction::get_all()[this->Faction]->get_type() == wyrmgus::faction_type::tribe) {
		return wyrmgus::faction::get_all()[this->Faction]->get_name();
	} else if (wyrmgus::faction::get_all()[this->Faction]->get_type() == wyrmgus::faction_type::polity) {
//		return this->GetTitle() + " of " + wyrmgus::faction::get_all()[this->Faction]->Name;
	}
	
	return "";
}

CGrandStrategyProvince *CGrandStrategyFaction::GetRandomProvinceWeightedByPopulation()
{
	std::vector<int> potential_provinces;
	for (size_t i = 0; i < this->OwnedProvinces.size(); ++i) {
		int weight = std::max(1, GrandStrategyGame.Provinces[this->OwnedProvinces[i]]->TotalWorkers);
		for (int j = 0; j < weight; ++j) {
			potential_provinces.push_back(this->OwnedProvinces[i]);
		}
	}
	
	if (potential_provinces.size() > 0) {
		return GrandStrategyGame.Provinces[potential_provinces[SyncRand(potential_provinces.size())]];
	} else {
		return nullptr;
	}
}

void CGrandStrategyHero::Die()
{
	//show message that the hero has died
	/*
	if (this->IsVisible()) {
		if (GrandStrategyGame.PlayerFaction != nullptr && GrandStrategyGame.PlayerFaction->Ministers[wyrmgus::character_title::head_of_state] == this) {
			char buf[256];
			snprintf(
				buf, sizeof(buf), "if (GenericDialog ~= nil) then GenericDialog(\"%s\", \"%s\") end;",
				(GrandStrategyGame.PlayerFaction->GetCharacterTitle(wyrmgus::character_title::head_of_state, this->Gender) + " " + this->get_full_name() + " Dies").c_str(),
				("Tragic news spread throughout our realm. Our " + FullyDecapitalizeString(GrandStrategyGame.PlayerFaction->GetCharacterTitle(wyrmgus::character_title::head_of_state, this->Gender)) + ", " + this->get_full_name() + ", has died! May his soul rest in peace.").c_str()
			);
			CclCommand(buf);	
		} else if (this->GetFaction() == GrandStrategyGame.PlayerFaction) {
			char buf[256];
			snprintf(
				buf, sizeof(buf), "if (GenericDialog ~= nil) then GenericDialog(\"%s\", \"%s\") end;",
				(this->GetBestDisplayTitle() + " " + this->get_full_name() + " Dies").c_str(),
				("My lord, the " + FullyDecapitalizeString(this->GetBestDisplayTitle()) + " " + this->get_full_name() + " has died!").c_str()
			);
			CclCommand(buf);	
		}
	}
	*/
	
	this->State = 0;

	//check if the hero has government positions in a faction, and if so, remove it from that position
	int titles_size = this->Titles.size();
	for (int i = (titles_size - 1); i >= 0; --i) {
		this->Titles[i].second->SetMinister(this->Titles[i].first, "");
	}
	
	this->Province = nullptr;
}

bool CGrandStrategyHero::IsAlive()
{
	return this->State != 0;
}

bool CGrandStrategyHero::IsVisible()
{
	return this->get_unit_type()->get_gender() == wyrmgus::gender::none || this->get_gender() == this->get_unit_type()->get_gender(); // hero not visible if their unit type has a set gender which is different from the hero's (this is because of instances where i.e. females have a unit type that only has male portraits)
}

bool CGrandStrategyHero::IsGenerated()
{
	return !this->is_custom() && character::get(this->get_full_name()) == nullptr;
}

bool CGrandStrategyHero::IsEligibleForTitle(const wyrmgus::character_title title)
{
	if (this->GetFaction()->government_type == wyrmgus::government_type::monarchy && title == wyrmgus::character_title::ruler && this->get_unit_type()->get_unit_class() != nullptr && this->get_unit_type()->get_unit_class()->get_identifier() == "worker") { // commoners cannot become monarchs
		return false;
	} else if (this->GetFaction()->government_type == wyrmgus::government_type::theocracy && title == wyrmgus::character_title::ruler && this->get_unit_type()->get_unit_class() != nullptr && this->get_unit_type()->get_unit_class()->get_identifier() != "priest") { // non-priests cannot rule theocracies
		return false;
	}
	
	for (size_t i = 0; i < this->Titles.size(); ++i) {
		if (this->Titles[i].first == wyrmgus::character_title::ruler && this->Titles[i].second->IsAlive() && title != wyrmgus::character_title::ruler) { // if it is not a head of state title, and this character is already the head of state of a living faction, return false
			return false;
		} else if (this->Titles[i].first != wyrmgus::character_title::ruler && title != wyrmgus::character_title::ruler) { // if is already a minister, don't accept another ministerial title of equal rank
			return false;
		}
	}
	
	for (size_t i = 0; i < this->ProvinceTitles.size(); ++i) {
		if (this->ProvinceTitles[i].first != wyrmgus::character_title::ruler && title != wyrmgus::character_title::ruler) { // if already has a government position, don't accept another ministerial title of equal rank
			return false;
		}
	}
	
	return true;
}

int CGrandStrategyHero::GetTroopCostModifier()
{
	int modifier = 0;
	
	modifier += (this->GetAttributeModifier(IntelligenceAttribute) + this->GetAttributeModifier(this->GetMartialAttribute())) / 2 * -5 / 2; //-2.5% troop cost modifier per average of the intelligence modifier and the strength/dexterity one (depending on which one the character uses)
	
	return modifier;
}

int CGrandStrategyHero::GetTitleScore(const wyrmgus::character_title title)
{
	int score = 0;
	if (title == wyrmgus::character_title::ruler) {
		score = ((this->Attributes[IntelligenceAttribute] + ((this->Attributes[this->GetMartialAttribute()] + this->Attributes[IntelligenceAttribute]) / 2) + this->Attributes[CharismaAttribute]) / 3) + 1;
	} else if (title == wyrmgus::character_title::marshal) {
		score = (this->Attributes[this->GetMartialAttribute()] + this->Attributes[IntelligenceAttribute]) / 2;
	} else if (title == wyrmgus::character_title::chancellor) {
		score = (this->Attributes[CharismaAttribute] + this->Attributes[IntelligenceAttribute]) / 2;
	}
	
	if (this->get_civilization()->ID != this->GetFaction()->civilization) {
		score -= 1; //characters of a different culture than the faction they are in have a smaller chance of getting a ministerial or gubernatorial position
	}
	
	return score;
}

std::string CGrandStrategyHero::GetMinisterEffectsString(const wyrmgus::character_title title)
{
	std::string minister_effects_string;
	
	bool first = true;
	
	if (title == wyrmgus::character_title::ruler || title == wyrmgus::character_title::marshal) {
		int modifier = this->GetTroopCostModifier();
		if (modifier != 0) {
			if (!first) {
				minister_effects_string += ", ";
			} else {
				first = false;
			}
			if (modifier > 0) {
				minister_effects_string += "+";
			}
			minister_effects_string += std::to_string(modifier) + "% Troop Cost";
		}
	}
	
	return minister_effects_string;
}

std::string CGrandStrategyHero::GetBestDisplayTitle()
{
	std::string best_title = this->get_unit_type()->get_name();
	/*
	int best_title_type = -1;
	for (size_t i = 0; i < this->Titles.size(); ++i) {
		if (this->Titles[i].second != this->GetFaction()) {
			continue;
		}
		if (this->Titles[i].first < best_title_type) {
			best_title = this->GetFaction()->GetCharacterTitle(this->Titles[i].first, this->Gender);
			best_title_type = this->Titles[i].first;
		}
	}
	for (size_t i = 0; i < this->ProvinceTitles.size(); ++i) {
		if (this->ProvinceTitles[i].first < best_title_type) {
			best_title = this->GetFaction()->GetCharacterTitle(this->ProvinceTitles[i].first, this->Gender);
			best_title_type = this->ProvinceTitles[i].first;
		}
	}
	*/
	return best_title;
}

CGrandStrategyFaction *CGrandStrategyHero::GetFaction()
{
	if (this->Province != nullptr) {
		return this->Province->Owner;
	} else if (this->ProvinceOfOrigin != nullptr) {
		return this->ProvinceOfOrigin->Owner;
	}
	
	return nullptr;
}

CGrandStrategyEvent::CGrandStrategyEvent()
{
}

CGrandStrategyEvent::~CGrandStrategyEvent()
{
}

void CGrandStrategyEvent::Trigger(CGrandStrategyFaction *faction)
{
//	fprintf(stderr, "Triggering event \"%s\" for faction %s.\n", this->Name.c_str(), wyrmgus::faction::get_all()[faction->Faction]->Name.c_str());	
	
	CclCommand("EventFaction = GetFactionFromName(\"" + wyrmgus::faction::get_all()[faction->Faction]->get_identifier() + "\");");
	CclCommand("GrandStrategyEvent(EventFaction, \"" + this->Name + "\");");
	CclCommand("EventFaction = nil;");
	CclCommand("EventProvince = nil;");
	CclCommand("SecondEventProvince = nil;");
	
	if (!this->Persistent) {
		GrandStrategyGame.AvailableEvents.erase(std::remove(GrandStrategyGame.AvailableEvents.begin(), GrandStrategyGame.AvailableEvents.end(), this), GrandStrategyGame.AvailableEvents.end());
	}
}

bool CGrandStrategyEvent::CanTrigger(CGrandStrategyFaction *faction)
{
	Q_UNUSED(faction)

	if (this->MinYear && GrandStrategyYear < this->MinYear) {
		return false;
	}
	
	if (this->MaxYear && GrandStrategyYear > this->MaxYear) {
		return false;
	}
	
	if (this->Conditions) {
		this->Conditions->pushPreamble();
		this->Conditions->run(1);
		if (this->Conditions->popBoolean() == false) {
			return false;
		}
	}
	
	return true;
}

/**
**  Get the ID of a province
*/
int GetProvinceId(std::string province_name)
{
	if (!province_name.empty()) {
		for (size_t i = 0; i < GrandStrategyGame.Provinces.size(); ++i) {
			if (GrandStrategyGame.Provinces[i]->Name == province_name) {
				return i;
			}
		}
	}
	
	return -1;
}

void SetProvinceOwner(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	wyrmgus::civilization *civilization = wyrmgus::civilization::get(civilization_name);
	int faction_id = wyrmgus::faction::get(faction_name)->ID;
	
	if (!civilization || province_id == -1 || !GrandStrategyGame.Provinces[province_id]) {
		return;
	}
	
	GrandStrategyGame.Provinces[province_id]->SetOwner(civilization->ID, faction_id);
}

void SetProvinceSettlementBuilding(std::string province_name, std::string settlement_building_ident, bool has_settlement_building)
{
	int province_id = GetProvinceId(province_name);
	int settlement_building = UnitTypeIdByIdent(settlement_building_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && settlement_building != -1) {
		GrandStrategyGame.Provinces[province_id]->SetSettlementBuilding(settlement_building, has_settlement_building);
	}
}

void SetProvinceUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && unit_type != -1) {
		GrandStrategyGame.Provinces[province_id]->SetUnitQuantity(unit_type, quantity);
	}
}

void ChangeProvinceUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && unit_type != -1) {
		GrandStrategyGame.Provinces[province_id]->ChangeUnitQuantity(unit_type, quantity);
	}
}

void SetProvinceHero(std::string province_name, std::string hero_full_name, int value)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->SetHero(hero_full_name, value);
	}
}

void AddProvinceClaim(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		wyrmgus::civilization *civilization = wyrmgus::civilization::get(civilization_name);
		int faction = wyrmgus::faction::get(faction_name)->ID;
		if (faction != -1) {
			GrandStrategyGame.Provinces[province_id]->AddFactionClaim(civilization->ID, faction);
		} else {
			fprintf(stderr, "Can't find %s faction (%s) to add claim to province %s.\n", faction_name.c_str(), civilization_name.c_str(), province_name.c_str());
		}
	} else {
		fprintf(stderr, "Can't find %s province to add %s faction (%s) claim to.\n", province_name.c_str(), faction_name.c_str(), civilization_name.c_str());
	}
}

void RemoveProvinceClaim(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		wyrmgus::civilization *civilization = wyrmgus::civilization::get(civilization_name);
		int faction = wyrmgus::faction::get(faction_name)->ID;
		GrandStrategyGame.Provinces[province_id]->RemoveFactionClaim(civilization->ID, faction);
	}
}

void InitializeGrandStrategyGame()
{
	//initialize literary works
	for (CUpgrade *upgrade : CUpgrade::get_all()) {
		if (upgrade->Work == wyrmgus::item_class::none || upgrade->UniqueOnly) { // literary works that can only appear in unique items wouldn't be publishable
			continue;
		}
		
		GrandStrategyGame.UnpublishedWorks.push_back(upgrade);
	}
}

void FinalizeGrandStrategyInitialization()
{
	//initialize literary works
	int works_size = GrandStrategyGame.UnpublishedWorks.size();
	for (int i = (works_size - 1); i >= 0; --i) {
		if (GrandStrategyGame.UnpublishedWorks[i]->Year != 0 && GrandStrategyYear >= GrandStrategyGame.UnpublishedWorks[i]->Year) { //if the game is starting after the publication date of this literary work, remove it from the work list
			GrandStrategyGame.UnpublishedWorks.erase(std::remove(GrandStrategyGame.UnpublishedWorks.begin(), GrandStrategyGame.UnpublishedWorks.end(), GrandStrategyGame.UnpublishedWorks[i]), GrandStrategyGame.UnpublishedWorks.end());
		}
	}
	
	for (size_t i = 0; i < GrandStrategyGame.Provinces.size(); ++i) {
		CGrandStrategyProvince *province = GrandStrategyGame.Provinces[i];
		CProvince *base_province = GetProvince(province->Name);
		
		for (std::map<CUpgrade *, std::map<int, bool>>::iterator iterator = base_province->HistoricalModifiers.begin(); iterator != base_province->HistoricalModifiers.end(); ++iterator) {
			for (std::map<int, bool>::reverse_iterator second_iterator = iterator->second.rbegin(); second_iterator != iterator->second.rend(); ++second_iterator) {
				if (GrandStrategyYear >= second_iterator->first) {
					province->SetModifier(iterator->first, second_iterator->second);
					break;
				}
			}
		}
	}
}

void DoGrandStrategyTurn()
{
	GrandStrategyGame.DoTurn();
}

bool ProvinceBordersProvince(std::string province_name, std::string second_province_name)
{
	int province = GetProvinceId(province_name);
	int second_province = GetProvinceId(second_province_name);
	
	return GrandStrategyGame.Provinces[province]->BordersProvince(GrandStrategyGame.Provinces[second_province]);
}

bool ProvinceBordersFaction(std::string province_name, std::string faction_civilization_name, std::string faction_name)
{
	int province = GetProvinceId(province_name);
	wyrmgus::civilization *civilization = wyrmgus::civilization::get(faction_civilization_name);
	int faction = wyrmgus::faction::get(faction_name)->ID;
	
	if (!civilization || faction == -1) {
		return false;
	}
	
	return GrandStrategyGame.Provinces[province]->BordersFaction(civilization->ID, faction);
}

bool ProvinceHasBuildingClass(std::string province_name, std::string building_class)
{
	int province_id = GetProvinceId(province_name);
	
	return GrandStrategyGame.Provinces[province_id]->HasBuildingClass(building_class);
}

std::string GetProvinceCivilization(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (GrandStrategyGame.Provinces[province_id]->civilization != -1) {
		return wyrmgus::civilization::get_all()[GrandStrategyGame.Provinces[province_id]->civilization]->get_identifier();
	} else {
		return "";
	}
}

bool GetProvinceSettlementBuilding(std::string province_name, std::string building_ident)
{
	int province_id = GetProvinceId(province_name);
	int building_id = UnitTypeIdByIdent(building_ident);
	
	return GrandStrategyGame.Provinces[province_id]->SettlementBuildings[building_id];
}

int GetProvinceUnitQuantity(std::string province_name, std::string unit_type_ident)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	return GrandStrategyGame.Provinces[province_id]->Units[unit_type];
}

int GetProvinceHero(std::string province_name, std::string hero_full_name)
{
	int province_id = GetProvinceId(province_name);

	if (province_id == -1) {
		fprintf(stderr, "Can't find %s province.\n", province_name.c_str());
		return 0;
	}
	
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		if (hero->Province != nullptr && hero->Province->ID == province_id) {
			return hero->State;
		}
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	
	return 0;
}

int GetProvinceMilitaryScore(std::string province_name, bool attacker, bool count_defenders)
{
	int province_id = GetProvinceId(province_name);
	
	int military_score = 0;
	if (province_id != -1) {
		if (attacker) {
			military_score = GrandStrategyGame.Provinces[province_id]->AttackingMilitaryScore;
		} else if (count_defenders) {
			military_score = GrandStrategyGame.Provinces[province_id]->MilitaryScore;
		} else {
			military_score = GrandStrategyGame.Provinces[province_id]->OffensiveMilitaryScore;
		}
	}
	
	return std::max(1, military_score); // military score must be at least one, since it is a divider in some instances, and we don't want to divide by 0
}

std::string GetProvinceOwner(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id == -1 || GrandStrategyGame.Provinces[province_id]->Owner == nullptr) {
		return "";
	}
	
	return wyrmgus::faction::get_all()[GrandStrategyGame.Provinces[province_id]->Owner->Faction]->get_identifier();
}

void SetFactionGovernmentType(std::string civilization_name, std::string faction_name, std::string government_type_name)
{
}

void SetFactionDiplomacyStateProposal(std::string civilization_name, std::string faction_name, std::string second_civilization_name, std::string second_faction_name, std::string diplomacy_state_name)
{
}

std::string GetFactionDiplomacyStateProposal(std::string civilization_name, std::string faction_name, std::string second_civilization_name, std::string second_faction_name)
{
	return "";
}

bool IsGrandStrategyUnit(const wyrmgus::unit_type &type)
{
	if (!type.BoolFlag[BUILDING_INDEX].value && type.DefaultStat.Variables[DEMAND_INDEX].Value > 0 && type.get_unit_class() != nullptr && type.get_unit_class()->get_identifier() != "caravan") {
		return true;
	}
	return false;
}

bool IsMilitaryUnit(const wyrmgus::unit_type &type)
{
	if (IsGrandStrategyUnit(type) && type.get_unit_class() != nullptr && type.get_unit_class()->get_identifier() != "worker") {
		return true;
	}
	return false;
}

void SetFactionMinister(std::string civilization_name, std::string faction_name, std::string title_name, std::string hero_full_name)
{
	wyrmgus::civilization *civilization = wyrmgus::civilization::get(civilization_name);
	int faction = -1;
	if (civilization) {
		faction = wyrmgus::faction::get(faction_name)->ID;
	}
	const wyrmgus::character_title title = enum_converter<character_title>::to_enum(title_name);
	
	if (faction == -1 || title == wyrmgus::character_title::none) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization->ID][faction]->SetMinister(title, TransliterateText(hero_full_name));
}

std::string GetFactionMinister(std::string civilization_name, std::string faction_name, std::string title_name)
{
	wyrmgus::civilization *civilization = wyrmgus::civilization::get(civilization_name);
	int faction = -1;
	if (civilization) {
		faction = wyrmgus::faction::get(faction_name)->ID;
	}
	const wyrmgus::character_title title = enum_converter<character_title>::to_enum(title_name);
	
	if (faction == -1 || title == wyrmgus::character_title::none) {
		return "";
	}
	
	if (GrandStrategyGame.Factions[civilization->ID][faction]->Ministers[title] != nullptr) {
		return GrandStrategyGame.Factions[civilization->ID][faction]->Ministers[title]->get_full_name();
	} else {
		return "";
	}
}

void KillGrandStrategyHero(std::string hero_full_name)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		hero->Die();
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
}

void GrandStrategyHeroExisted(std::string hero_full_name)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		hero->Existed = true;
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
}

bool GrandStrategyHeroIsAlive(std::string hero_full_name)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		return hero->IsAlive();
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	return false;
}

void GrandStrategyWorkCreated(std::string work_ident)
{
	CUpgrade *work = CUpgrade::get(work_ident);
	GrandStrategyGame.UnpublishedWorks.erase(std::remove(GrandStrategyGame.UnpublishedWorks.begin(), GrandStrategyGame.UnpublishedWorks.end(), work), GrandStrategyGame.UnpublishedWorks.end()); // remove work from the vector, so that it doesn't get created again
}

void MakeGrandStrategyEventAvailable(std::string event_name)
{
	CGrandStrategyEvent *event = GetGrandStrategyEvent(event_name);
	if (event) {
		GrandStrategyGame.AvailableEvents.push_back(event);
	} else {
		fprintf(stderr, "Grand strategy event \"%s\" doesn't exist.\n", event_name.c_str());
	}
}

bool GetGrandStrategyEventTriggered(std::string event_name)
{
	CGrandStrategyEvent *event = GetGrandStrategyEvent(event_name);
	if (event) {
		return std::find(GrandStrategyGame.AvailableEvents.begin(), GrandStrategyGame.AvailableEvents.end(), event) == GrandStrategyGame.AvailableEvents.end();
	} else {
		fprintf(stderr, "Grand strategy event \"%s\" doesn't exist.\n", event_name.c_str());
		return false;
	}
}

void CleanGrandStrategyEvents()
{
	GrandStrategyEvents.clear();
	GrandStrategyEventStringToPointer.clear();
}

CGrandStrategyEvent *GetGrandStrategyEvent(std::string event_name)
{
	if (GrandStrategyEventStringToPointer.find(event_name) != GrandStrategyEventStringToPointer.end()) {
		return GrandStrategyEventStringToPointer[event_name];
	}
	
	return nullptr;
}
