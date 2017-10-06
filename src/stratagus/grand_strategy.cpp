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
/**@name grand_strategy.cpp - The grand strategy mode. */
//
//      (c) Copyright 2015-2016 by Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "grand_strategy.h"

#include "character.h"
#include "game.h"	// for loading screen elements
#include "font.h"	// for grand strategy mode tooltip drawing
#include "interface.h"
#include "iolib.h"
#include "luacallback.h"
#include "menus.h"
#include "player.h"
#include "results.h"
#include "sound_server.h"
#include "ui.h"
#include "unit.h"
#include "unittype.h"
#include "upgrade.h"
#include "util.h"
#include "video.h"

#include <ctype.h>

#include <string>
#include <map>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

bool GrandStrategy = false;				///if the game is in grand strategy mode
bool GrandStrategyGamePaused = false;
bool GrandStrategyGameInitialized = false;
bool GrandStrategyGameLoading = false;
bool GrandStrategyBattleBaseBuilding = false;
int GrandStrategyYear = 0;
int GrandStrategyMonth = 0;
std::string GrandStrategyWorld;
int WorldMapOffsetX;
int WorldMapOffsetY;
int GrandStrategyMapWidthIndent;
int GrandStrategyMapHeightIndent;
int BattalionMultiplier;
int PopulationGrowthThreshold = 1000;
std::string GrandStrategyInterfaceState;
std::string SelectedHero;
CGrandStrategyGame GrandStrategyGame;
std::map<std::string, int> GrandStrategyHeroStringToIndex;
std::vector<CGrandStrategyEvent *> GrandStrategyEvents;
std::map<std::string, CGrandStrategyEvent *> GrandStrategyEventStringToPointer;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CGrandStrategyGame::DrawInterface()
{
	if (this->PlayerFaction != NULL && this->PlayerFaction->OwnedProvinces.size() > 0) { //draw resource bar
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
			UI.Resources[stored_resources[i]].G->DrawFrameClip(0, x, y, true);
			
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
				income_string += std::to_string((long long) income);
			}
			std::string resource_stored_string = std::to_string((long long) quantity_stored) + income_string;
			
			if (resource_stored_string.size() <= 9) {
				CLabel(GetGameFont()).Draw(x + 18, y + 1, resource_stored_string);
			} else {
				CLabel(GetSmallFont()).Draw(x + 18, y + 1 + 2, resource_stored_string);
			}
			
			if (CursorScreenPos.x >= x && CursorScreenPos.x <= (x + UI.Resources[stored_resources[i]].G->getGraphicWidth()) && CursorScreenPos.y >= y && CursorScreenPos.y <= (y + UI.Resources[stored_resources[i]].G->getGraphicHeight())) {
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
			DrawGenericPopup("Gain Research by building town halls, lumber mills, smithies and temples", hovered_research_icon.x, hovered_research_icon.y);
		} else if (hovered_prestige_icon.x != -1 && hovered_prestige_icon.y != -1) {
			DrawGenericPopup("Prestige influences trade priority between nations, and factions with negative prestige cannot declare war", hovered_prestige_icon.x, hovered_prestige_icon.y);
		}
	}
}

void CGrandStrategyGame::DoTurn()
{
	for (size_t i = 0; i < this->Provinces.size(); ++i) {
		if (this->Provinces[i]->Civilization != -1) { // if this province has a culture
			if (this->Provinces[i]->Owner != NULL) {
				if (!this->Provinces[i]->HasFactionClaim(this->Provinces[i]->Owner->Civilization, this->Provinces[i]->Owner->Faction) && SyncRand(100) < 1) { // 1% chance the owner of this province will get a claim on it
					this->Provinces[i]->AddFactionClaim(this->Provinces[i]->Owner->Civilization, this->Provinces[i]->Owner->Faction);
				}
					
				if (SyncRand(1000) == 0) { // 0.1% chance per year that a (randomly generated) literary work will be created in a province
					this->CreateWork(NULL, NULL, this->Provinces[i]);
				}
			}
		}
		this->Provinces[i]->Movement = false; //after processing the turn, always set the movement to false
	}
	
	// check if any literary works should be published this year
	int works_size = this->UnpublishedWorks.size();
	for (int i = (works_size - 1); i >= 0; --i) {
		CGrandStrategyHero *author = NULL;
		if (this->UnpublishedWorks[i]->Author != NULL) {
			author = this->GetHero(this->UnpublishedWorks[i]->Author->GetFullName());
			if (author != NULL && !author->IsAlive()) {
				continue;
			}
		}
			
		if ((author == NULL && SyncRand(200) != 0) || (author != NULL && SyncRand(10) != 0)) { // 0.5% chance per year that a work will be published if no author is preset, and 10% if an author is preset
			continue;
		}
		
		int civilization = this->UnpublishedWorks[i]->Civilization;
		if (
			(author != NULL && author->ProvinceOfOrigin != NULL)
			|| (civilization != -1 && this->CultureProvinces.find(civilization) != this->CultureProvinces.end() && this->CultureProvinces[civilization].size() > 0)
		) {
			bool characters_existed = true;
			for (size_t j = 0; j < this->UnpublishedWorks[i]->Characters.size(); ++j) {
				CGrandStrategyHero *hero = this->GetHero(this->UnpublishedWorks[i]->Characters[j]->GetFullName());
				
				if (hero == NULL || !hero->Existed) {
					characters_existed = false;
					break;
				}
			}
			if (!characters_existed) {
				continue;
			}
			
			if (author != NULL && author->Province != NULL) {
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

	if (!province->Owner->HasTechnologyClass("writing")) { // only factions which have knowledge of writing can produce literary works
		return;
	}

	if (work != NULL) {
		this->UnpublishedWorks.erase(std::remove(this->UnpublishedWorks.begin(), this->UnpublishedWorks.end(), work), this->UnpublishedWorks.end()); // remove work from the vector, so that it doesn't get created again
	}

	std::string work_name;
	if (work != NULL) {
		work_name = work->Name;
	} else {
		work_name = province->GenerateWorkName();
		if (work_name.empty()) {
			return;
		}
	}
	
	if (author == NULL) {
		author = province->GetRandomAuthor();
	}
	
	if (province->Owner == GrandStrategyGame.PlayerFaction || work != NULL) { // only show foreign works that are predefined
		std::string work_creation_message = "if (GenericDialog ~= nil) then GenericDialog(\"" + work_name + "\", \"";
		if (author != NULL) {
			work_creation_message += "The " + FullyDecapitalizeString(author->Type->Name) + " " + author->GetFullName() + " ";
		} else {
			work_creation_message += "A sage ";
		}
		work_creation_message += "has written the literary work \\\"" + work_name + "\\\" in ";
		if (province->Owner != GrandStrategyGame.PlayerFaction) {
			work_creation_message += "the foreign lands of ";
		}
		work_creation_message += province->Name + "!";
		if (work != NULL && !work->Description.empty()) {
			work_creation_message += " " + FindAndReplaceString(FindAndReplaceString(work->Description, "\"", "\\\""), "\n", "\\n");
		}
		if (work != NULL && !work->Quote.empty()) {
			work_creation_message += "\\n\\n" + FindAndReplaceString(FindAndReplaceString(work->Quote, "\"", "\\\""), "\n", "\\n");
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

bool CGrandStrategyGame::IsPointOnMap(int x, int y)
{
	if (x < 0 || x >= this->WorldMapWidth || y < 0 || y >= this->WorldMapHeight || !WorldMapTiles[x][y]) {
		return false;
	}
	return true;
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
		return NULL;
	}
}

void GrandStrategyWorldMapTile::SetResourceProspected(int resource_id, bool discovered)
{
	if (this->ResourceProspected == discovered) { //no change, return
		return;
	}
	
	if (resource_id != -1 && this->Resource == resource_id) {
		this->ResourceProspected = discovered;
		
		if (this->Province != NULL) {
			if (this->ResourceProspected) {
				this->Province->ProductionCapacity[resource_id] += 1;
			} else {
				this->Province->ProductionCapacity[resource_id] -= 1;
			}
		}
	}
}

bool GrandStrategyWorldMapTile::IsWater()
{
	if (this->Terrain != -1) {
		return WorldMapTerrainTypes[this->Terrain]->Water;
	}
	return false;
}

/**
**  Get whether the tile has a resource
*/
bool GrandStrategyWorldMapTile::HasResource(int resource, bool ignore_prospection)
{
	if (resource == this->Resource && (this->ResourceProspected || ignore_prospection)) {
		return true;
	}
	return false;
}

void CGrandStrategyProvince::SetOwner(int civilization_id, int faction_id)
{
	//if new owner is the same as the current owner, return
	if (
		(this->Owner != NULL && this->Owner->Civilization == civilization_id && this->Owner->Faction == faction_id)
		|| (this->Owner == NULL && civilization_id == -1 && faction_id == -1)
	) {
		return;
	}
	
	CGrandStrategyFaction *old_owner = this->Owner;
	
	if (this->Owner != NULL) { //if province has a previous owner, remove it from the owner's province list
		this->Owner->OwnedProvinces.erase(std::remove(this->Owner->OwnedProvinces.begin(), this->Owner->OwnedProvinces.end(), this->ID), this->Owner->OwnedProvinces.end());

		if (GrandStrategyGameInitialized) {
			if (this->Owner->Capital == this) { // if this was the old owner's capital province, set a random one of the provinces it still has remaining as the capital if it still has territory, set the capital to NULL otherwise
				if (this->Owner->OwnedProvinces.size() > 0) {
					this->Owner->SetCapital(this->Owner->GetRandomProvinceWeightedByPopulation());
				} else {
					this->Owner->SetCapital(NULL);
				}
			}
			
			for (int i = 0; i < MaxCharacterTitles; ++i) {
				if (IsMinisterialTitle(i) && this->Owner->Ministers[i] != NULL && this->Owner->Ministers[i]->Province == this) { // if any ministers of the old owner are in this province, move them to another province they own, or kill them off if none are available
					if (this->Owner->OwnedProvinces.size() > 0) {
						this->Owner->GetRandomProvinceWeightedByPopulation()->SetHero(this->Owner->Ministers[i]->GetFullName(), this->Owner->Ministers[i]->State);
					} else {
						this->Owner->Ministers[i]->Die();
					}
				}
			}
		}
		
		//also remove its resource incomes from the owner's incomes, and reset the province's income so it won't be deduced from the new owner's income when recalculating it
		for (int i = 0; i < MaxCosts; ++i) {
			if (this->Income[i] != 0) {
				this->Owner->Income[i] -= this->Income[i];
				this->Income[i] = 0;
			}
		}
	}
	
	for (size_t i = 0; i < UnitTypes.size(); ++i) { //change the province's military score to be appropriate for the new faction's technologies
		if (IsMilitaryUnit(*UnitTypes[i])) {
			int old_owner_military_score_bonus = (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[i] : 0);
			int new_owner_military_score_bonus = (faction_id != -1 ? GrandStrategyGame.Factions[civilization_id][faction_id]->MilitaryScoreBonus[i] : 0);
			if (old_owner_military_score_bonus != new_owner_military_score_bonus) {
				this->MilitaryScore += this->Units[i] * (new_owner_military_score_bonus - old_owner_military_score_bonus);
				this->OffensiveMilitaryScore += this->Units[i] * new_owner_military_score_bonus - old_owner_military_score_bonus;
			}
		} else if (UnitTypes[i]->Class != -1 && UnitTypeClasses[UnitTypes[i]->Class] == "worker") {
			int militia_unit_type = PlayerRaces.GetCivilizationClassUnitType(UnitTypes[i]->Civilization, GetUnitTypeClassIndexByName("militia"));
			if (militia_unit_type != -1) {
				int old_owner_military_score_bonus = (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[militia_unit_type] : 0);
				int new_owner_military_score_bonus = (faction_id != -1 ? GrandStrategyGame.Factions[civilization_id][faction_id]->MilitaryScoreBonus[militia_unit_type] : 0);
				if (old_owner_military_score_bonus != new_owner_military_score_bonus) {
					this->MilitaryScore += this->Units[i] * ((new_owner_military_score_bonus - old_owner_military_score_bonus) / 2);
				}
			}
		}
	}

	if (civilization_id != -1 && faction_id != -1) {
		this->Owner = GrandStrategyGame.Factions[civilization_id][faction_id];
		this->Owner->OwnedProvinces.push_back(this->ID);
		
		if (GrandStrategyGameInitialized && this->Owner->Capital == NULL) { //if new owner has no capital, set this province as the capital
			this->Owner->SetCapital(this);
		}
	} else {
		this->Owner = NULL;
	}
}

void CGrandStrategyProvince::SetSettlementBuilding(int building_id, bool has_settlement_building)
{
	if (building_id == -1) {
		fprintf(stderr, "Tried to set invalid building type for the settlement of \"%s\".\n", this->Name.c_str());
		return;
	}
	
	//if this province has an equivalent building for its civilization/faction, use that instead
	if (UnitTypes[building_id]->Civilization != -1) {
		if (this->GetClassUnitType(UnitTypes[building_id]->Class) != building_id && this->GetClassUnitType(UnitTypes[building_id]->Class) != -1) {
			building_id = this->GetClassUnitType(UnitTypes[building_id]->Class);
		}
	}
				
	if (this->SettlementBuildings[building_id] == has_settlement_building) {
		return;
	}
	
	this->SettlementBuildings[building_id] = has_settlement_building;
	
	int change = has_settlement_building ? 1 : -1;
	for (int i = 0; i < MaxCosts; ++i) {
		if (UnitTypes[building_id]->GrandStrategyProductionEfficiencyModifier[i] != 0) {
			this->ProductionEfficiencyModifier[i] += UnitTypes[building_id]->GrandStrategyProductionEfficiencyModifier[i] * change;
		}
	}
	
	if (UnitTypes[building_id]->Class != -1 && UnitTypeClasses[UnitTypes[building_id]->Class] == "stronghold") { //increase the military score of the province, if this building is a stronghold
		this->MilitaryScore += (100 * 2) * change; // two guard towers if has a stronghold
	}
}

void CGrandStrategyProvince::SetSettlementLocation(int x, int y)
{
	this->SettlementLocation.x = x;
	this->SettlementLocation.y = y;
	CclCommand("if (GetProvinceFromName(\"" + this->Name + "\") ~= nil) then GetProvinceFromName(\"" + this->Name + "\").SettlementLocation = {" + std::to_string((long long) x) + ", " + std::to_string((long long) y) + "} end;");
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

	int change = has_modifier ? 1 : -1;

	for (int i = 0; i < MaxCosts; ++i) {
		if (modifier->GrandStrategyProductionEfficiencyModifier[i] != 0) {
			this->ProductionEfficiencyModifier[i] += modifier->GrandStrategyProductionEfficiencyModifier[i] * change;
		}
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
	
	if (IsMilitaryUnit(*UnitTypes[unit_type_id])) {
		this->MilitaryScore += change * (UnitTypes[unit_type_id]->DefaultStat.Variables[POINTS_INDEX].Value + (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[unit_type_id] : 0));
		this->OffensiveMilitaryScore += change * (UnitTypes[unit_type_id]->DefaultStat.Variables[POINTS_INDEX].Value + (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[unit_type_id] : 0));
	}
	
	if (UnitTypes[unit_type_id]->Class != -1 && UnitTypeClasses[UnitTypes[unit_type_id]->Class] == "worker") {
		this->TotalWorkers += change;
		
		//if this unit's civilization can change workers into militia, add half of the militia's points to the military score (one in every two workers becomes a militia when the province is attacked)
		int militia_unit_type = PlayerRaces.GetCivilizationClassUnitType(UnitTypes[unit_type_id]->Civilization, GetUnitTypeClassIndexByName("militia"));
		if (militia_unit_type != -1) {
			this->MilitaryScore += change * ((UnitTypes[militia_unit_type]->DefaultStat.Variables[POINTS_INDEX].Value + (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[militia_unit_type] : 0)) / 2);
		}
	}
	
	this->Units[unit_type_id] = quantity;
}

void CGrandStrategyProvince::ChangeUnitQuantity(int unit_type_id, int quantity)
{
	this->SetUnitQuantity(unit_type_id, this->Units[unit_type_id] + quantity);
}

void CGrandStrategyProvince::SetAttackingUnitQuantity(int unit_type_id, int quantity)
{
	quantity = std::max(0, quantity);
	
	int change = quantity - this->GetAttackingUnitQuantity(unit_type_id);
	
	if (IsMilitaryUnit(*UnitTypes[unit_type_id])) {
		this->AttackingMilitaryScore += change * (UnitTypes[unit_type_id]->DefaultStat.Variables[POINTS_INDEX].Value + (this->AttackedBy != NULL ? this->AttackedBy->MilitaryScoreBonus[unit_type_id] : 0));
	}

	if (quantity > 0) {
		this->AttackingUnits[unit_type_id] = quantity;
	} else {
		this->AttackingUnits.erase(unit_type_id);
	}
}

void CGrandStrategyProvince::ChangeAttackingUnitQuantity(int unit_type_id, int quantity)
{
	this->SetAttackingUnitQuantity(unit_type_id, this->GetAttackingUnitQuantity(unit_type_id) + quantity);
}

void CGrandStrategyProvince::SetMovingUnitQuantity(int unit_type_id, int quantity)
{
	quantity = std::max(0, quantity);
	
	if (quantity > 0) {
		this->Movement = true;
	}
		
	this->MovingUnits[unit_type_id] = quantity;
}

void CGrandStrategyProvince::ChangeMovingUnitQuantity(int unit_type_id, int quantity)
{
	this->SetMovingUnitQuantity(unit_type_id, this->MovingUnits[unit_type_id] + quantity);
}

void CGrandStrategyProvince::SetPopulation(int quantity)
{
	if (this->Civilization == -1) {
		return;
	}

	int worker_unit_type = this->GetClassUnitType(GetUnitTypeClassIndexByName("worker"));
	
	if (quantity > 0) {
		quantity /= 10000; // each population unit represents 10,000 people
		quantity /= 2; // only (working-age) adults are represented, so around half of the total population
		quantity = std::max(1, quantity);
	}
	
//	quantity -= this->TotalUnits - this->Units[worker_unit_type]; // decrease from the quantity to be set the population that isn't composed of workers
	// don't take military units in consideration for this population count for now (since they don't cost food anyway)
	quantity -= this->TotalWorkers - this->Units[worker_unit_type]; // decrease from the quantity to be set the population that isn't composed of workers
	quantity = std::max(0, quantity);
			
	this->SetUnitQuantity(worker_unit_type, quantity);
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
		if (hero->Province != NULL) {
			if (hero->State == 2) {
				hero->Province->MilitaryScore -= (hero->Type->DefaultStat.Variables[POINTS_INDEX].Value + (hero->Province->Owner != NULL ? hero->Province->Owner->MilitaryScoreBonus[hero->Type->Slot] : 0));
			} else if (hero->State == 3) {
				hero->Province->AttackingMilitaryScore -= (hero->Type->DefaultStat.Variables[POINTS_INDEX].Value + (hero->Province->AttackedBy != NULL ? hero->Province->AttackedBy->MilitaryScoreBonus[hero->Type->Slot] : 0));
			}
		}
		hero->State = value;
			
		if (this != hero->Province || value == 0) { //if the new province is different from the hero's current province
			if (hero->Province != NULL) {
				hero->Province->Heroes.erase(std::remove(hero->Province->Heroes.begin(), hero->Province->Heroes.end(), hero), hero->Province->Heroes.end());  //remove the hero from the previous province
				if (hero->IsActive()) {
					hero->Province->ActiveHeroes.erase(std::remove(hero->Province->ActiveHeroes.begin(), hero->Province->ActiveHeroes.end(), hero), hero->Province->ActiveHeroes.end());
				}
			}
			hero->Province = value != 0 ? const_cast<CGrandStrategyProvince *>(&(*this)) : NULL;
			if (hero->Province != NULL) {
				hero->Province->Heroes.push_back(hero); //add the hero to the new province
				if (hero->IsActive()) {
					hero->Province->ActiveHeroes.push_back(hero); //add the hero to the new province
				}
			}
		}
	} else {
		//if the hero hasn't been defined yet, give an error message
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
		return;
	}
	
	
	if (value == 2) {
		this->MilitaryScore += (hero->Type->DefaultStat.Variables[POINTS_INDEX].Value + (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[hero->Type->Slot] : 0));
	} else if (value == 3) {
		this->AttackingMilitaryScore += (hero->Type->DefaultStat.Variables[POINTS_INDEX].Value + (this->AttackedBy != NULL ? this->AttackedBy->MilitaryScoreBonus[hero->Type->Slot] : 0));
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
	if (this->Civilization == -1 || building_class_name.empty()) {
		return false;
	}
	
	int building_type = this->GetClassUnitType(GetUnitTypeClassIndexByName(building_class_name));
	
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
		if (this->Claims[i]->Civilization == civilization_id && this->Claims[i]->Faction == faction_id) {
			return true;
		}
	}
	return false;
}

bool CGrandStrategyProvince::HasResource(int resource, bool ignore_prospection)
{
	for (size_t i = 0; i < this->Tiles.size(); ++i) {
		int x = this->Tiles[i].x;
		int y = this->Tiles[i].y;
		if (GrandStrategyGame.WorldMapTiles[x][y] && GrandStrategyGame.WorldMapTiles[x][y]->HasResource(resource, ignore_prospection)) {
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
		if (border_province->Owner == NULL) {
			if (border_province->Water && check_through_water) {
				for (size_t j = 0; j < border_province->BorderProvinces.size(); ++j) {
					if (
						border_province->BorderProvinces[j]->Owner != NULL
						&& border_province->BorderProvinces[j]->Owner->Civilization == faction_civilization
						&& border_province->BorderProvinces[j]->Owner->Faction == faction
					) {
						return true;
					}
				}
			}
			continue;
		}
		if (border_province->Owner->Civilization == faction_civilization && border_province->Owner->Faction == faction) {
			return true;
		}
	}
	return false;
}

bool CGrandStrategyProvince::CanAttackProvince(CGrandStrategyProvince *province)
{
	if (
		this->Owner == province->Owner
		|| province->Water
		|| (province->AttackedBy != NULL && province->AttackedBy != this->Owner) // province can only be attacked by one player per turn because of mechanical limitations of the current code
	) {
		return false;
	}
	
	// if is at peace or offering peace, can't attack
	if (
		province->Owner != NULL
		&& (
			this->Owner->GetDiplomacyState(province->Owner) != DiplomacyStateWar
			|| this->Owner->GetDiplomacyState(province->Owner) == DiplomacyStatePeace
		)
	) {
		return false;
	}
	
	if (
		this->BordersProvince(province) == false
		&& (
			province->Coastal == false
			|| this->HasBuildingClass("dock") == false
			|| this->HasSecondaryBorderThroughWaterWith(province) == false
		)
	) {
		return false;
	}
	
	if (this->Owner != GrandStrategyGame.PlayerFaction && !this->Owner->IsConquestDesirable(province)) { // if is AI-controlled, and the conquest isn't desirable, don't attack
		return false;
	}

	return true;
}

int CGrandStrategyProvince::GetAttackingUnitQuantity(int unit_type_id)
{
	if (this->AttackingUnits.find(unit_type_id) != this->AttackingUnits.end()) {
		return this->AttackingUnits[unit_type_id];
	} else {
		return 0;
	}
}

int CGrandStrategyProvince::GetPopulation()
{
	return (this->TotalWorkers * 10000) * 2;
}

int CGrandStrategyProvince::GetResourceDemand(int resource)
{
	int quantity = 0;
	if (resource == WoodCost) {
		quantity = 50;
		if (this->HasBuildingClass("lumber-mill")) {
			quantity += 50; // increase the province's lumber demand if it has a lumber mill built
		}
	} else if (resource == StoneCost) {
		quantity = 25;
	}
	
	if (quantity > 0 && GrandStrategyGame.CommodityPrices[resource] > 0) {
		quantity *= DefaultResourcePrices[resource];
		quantity /= GrandStrategyGame.CommodityPrices[resource];
	}

	return quantity;
}

int CGrandStrategyProvince::GetProductionEfficiencyModifier(int resource)
{
	int modifier = 0;
	
	if (this->Owner != NULL) {
		modifier += this->Owner->GetProductionEfficiencyModifier(resource);
	}
	
	modifier += this->ProductionEfficiencyModifier[resource];

	return modifier;
}

int CGrandStrategyProvince::GetClassUnitType(int class_id)
{
	return PlayerRaces.GetCivilizationClassUnitType(this->Civilization, class_id);
}

int CGrandStrategyProvince::GetLanguage()
{
	return PlayerRaces.GetCivilizationLanguage(this->Civilization);
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
	if (this->Owner == NULL) {
		return "";
	}
	
	std::string work_name;
	
	std::vector<CGrandStrategyHero *> potential_heroes;
	for (size_t i = 0; i < this->Owner->HistoricalMinisters[CharacterTitleHeadOfState].size(); ++i) {
		potential_heroes.push_back(this->Owner->HistoricalMinisters[CharacterTitleHeadOfState][i]);
	}
	
	if (potential_heroes.size() > 0 && SyncRand(10) != 0) { // 9 chances out of 10 that a literary work will use a hero's name as a basis
		work_name += potential_heroes[SyncRand(potential_heroes.size())]->Name;
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
		if (this->Heroes[i]->Type->Class != -1 && UnitTypeClasses[this->Heroes[i]->Type->Class] == "priest") { // first see if there are any heroes in the province from a unit class likely to produce scholarly works
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
		return NULL;
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
	for (size_t z = 0; z < AllUpgrades[upgrade_id]->UpgradeModifiers.size(); ++z) {
		for (size_t i = 0; i < UnitTypes.size(); ++i) {
				
			Assert(AllUpgrades[upgrade_id]->UpgradeModifiers[z]->ApplyTo[i] == '?' || AllUpgrades[upgrade_id]->UpgradeModifiers[z]->ApplyTo[i] == 'X');

			if (AllUpgrades[upgrade_id]->UpgradeModifiers[z]->ApplyTo[i] == 'X') {
				if (AllUpgrades[upgrade_id]->UpgradeModifiers[z]->Modifier.Variables[POINTS_INDEX].Value) {
					this->MilitaryScoreBonus[i] += AllUpgrades[upgrade_id]->UpgradeModifiers[z]->Modifier.Variables[POINTS_INDEX].Value * change;
				}
			}
		}
	}
	
	if (!secondary_setting) { //if this technology is not being set as a result of another technology of the same class being researched
		if (has_technology) { //if value is true, mark technologies from other civilizations that are of the same class as researched too, so that the player doesn't need to research the same type of technology every time
			if (AllUpgrades[upgrade_id]->Class != -1) {
				for (size_t i = 0; i < AllUpgrades.size(); ++i) {
					if (AllUpgrades[upgrade_id]->Class == AllUpgrades[i]->Class) {
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

void CGrandStrategyFaction::SetDiplomacyState(CGrandStrategyFaction *faction, int diplomacy_state_id)
{
	int second_diplomacy_state_id; // usually the second diplomacy state is the same as the first, but there are asymmetrical diplomacy states (such as vassal/sovereign relationships)
	if (diplomacy_state_id == DiplomacyStateVassal) {
		second_diplomacy_state_id = DiplomacyStateOverlord;
	} else if (diplomacy_state_id == DiplomacyStateOverlord) {
		second_diplomacy_state_id = DiplomacyStateVassal;
	} else {
		second_diplomacy_state_id = diplomacy_state_id;
	}
	
	this->DiplomacyStates[faction] = diplomacy_state_id;
	faction->DiplomacyStates[this] = second_diplomacy_state_id;
}

void CGrandStrategyFaction::SetMinister(int title, std::string hero_full_name)
{
	if (this->Ministers[title] != NULL && std::find(this->Ministers[title]->Titles.begin(), this->Ministers[title]->Titles.end(), std::pair<int, CGrandStrategyFaction *>(title, this)) != this->Ministers[title]->Titles.end()) { // remove from the old minister's array
		this->Ministers[title]->Titles.erase(std::remove(this->Ministers[title]->Titles.begin(), this->Ministers[title]->Titles.end(), std::pair<int, CGrandStrategyFaction *>(title, this)), this->Ministers[title]->Titles.end());
	}
			
	if (hero_full_name.empty()) {
		if (this->CanHaveSuccession(title, true) && GrandStrategyGameInitialized) { //if the minister died a violent death, wait until the next turn to replace him
			this->MinisterSuccession(title);
		} else {
			this->Ministers[title] = NULL;
		}
	} else {
		CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
		if (hero) {
			this->Ministers[title] = const_cast<CGrandStrategyHero *>(&(*hero));
			hero->Titles.push_back(std::pair<int, CGrandStrategyFaction *>(title, this));
			
			if (this->IsAlive()) {
				int titles_size = hero->Titles.size();
				for (int i = (titles_size - 1); i >= 0; --i) {
					if (!(hero->Titles[i].first == title && hero->Titles[i].second == this) && hero->Titles[i].first != CharacterTitleHeadOfState) { // a character can only have multiple head of state titles, but not others
						hero->Titles[i].second->SetMinister(hero->Titles[i].first, "");
					}
				}
			}
		} else {
			fprintf(stderr, "Tried to make \"%s\" the \"%s\" of the \"%s\", but the hero doesn't exist.\n", hero_full_name.c_str(), GetCharacterTitleNameById(title).c_str(), this->GetFullName().c_str());
		}
		
		if (this == GrandStrategyGame.PlayerFaction && GrandStrategyGameInitialized) {
			std::string new_minister_message = "if (GenericDialog ~= nil) then GenericDialog(\"";
//			new_minister_message += this->GetCharacterTitle(title, this->Ministers[title]->Gender) + " " + this->Ministers[title]->GetFullName();
			new_minister_message += "\", \"";
//			new_minister_message += "A new " + FullyDecapitalizeString(this->GetCharacterTitle(title, this->Ministers[title]->Gender));
			if (title == CharacterTitleHeadOfState) {
				new_minister_message += " has come to power in our realm, ";
			} else {
				new_minister_message += " has been appointed, ";
			}
			new_minister_message += this->Ministers[title]->GetFullName() + "!\\n\\n";
			new_minister_message += "Type: " + this->Ministers[title]->Type->Name + "\\n" + "Trait: " + this->Ministers[title]->Trait->Name + "\\n" +  + "\\n\\n" + this->Ministers[title]->GetMinisterEffectsString(title);
			new_minister_message += "\") end;";
			CclCommand(new_minister_message);	
		}
		
		if (std::find(this->HistoricalMinisters[title].begin(), this->HistoricalMinisters[title].end(), hero) == this->HistoricalMinisters[title].end()) {
			this->HistoricalMinisters[title].push_back(hero);
		}
		
		if (this->IsAlive() && (hero->Province == NULL || hero->Province->Owner != this)) { // if the hero's province is not owned by this faction, move him to a random province owned by this faction
			this->GetRandomProvinceWeightedByPopulation()->SetHero(hero->GetFullName(), hero->State);
		}
	}
}

void CGrandStrategyFaction::MinisterSuccession(int title)
{
	if (
		this->Ministers[title] != NULL
		&& (PlayerRaces.Factions[this->Faction]->Type == FactionTypeTribe || this->GovernmentType == GovernmentTypeMonarchy)
		&& title == CharacterTitleHeadOfState
	) { //if is a tribe or a monarchical polity, try to perform ruler succession by descent
		for (size_t i = 0; i < this->Ministers[title]->Children.size(); ++i) {
			if (this->Ministers[title]->Children[i]->IsAlive() && this->Ministers[title]->Children[i]->IsVisible() && this->Ministers[title]->Children[i]->Gender == MaleGender) { //historically males have generally been given priority in throne inheritance (if not exclusivity), specially in the cultures currently playable in the game
				this->SetMinister(title, this->Ministers[title]->Children[i]->GetFullName());
				return;
			}
		}
		for (size_t i = 0; i < this->Ministers[title]->Siblings.size(); ++i) { // now check for male siblings of the current ruler
			if (this->Ministers[title]->Siblings[i]->IsAlive() && this->Ministers[title]->Siblings[i]->IsVisible() && this->Ministers[title]->Siblings[i]->Gender == MaleGender) {
				this->SetMinister(title, this->Ministers[title]->Siblings[i]->GetFullName());
				return;
			}
		}		
		for (size_t i = 0; i < this->Ministers[title]->Children.size(); ++i) { //check again for children, but now allow for inheritance regardless of gender
			if (this->Ministers[title]->Children[i]->IsAlive() && this->Ministers[title]->Children[i]->IsVisible()) {
				this->SetMinister(title, this->Ministers[title]->Children[i]->GetFullName());
				return;
			}
		}
		for (size_t i = 0; i < this->Ministers[title]->Siblings.size(); ++i) { //check again for siblings, but now allow for inheritance regardless of gender
			if (this->Ministers[title]->Siblings[i]->IsAlive() && this->Ministers[title]->Siblings[i]->IsVisible()) {
				this->SetMinister(title, this->Ministers[title]->Siblings[i]->GetFullName());
				return;
			}
		}
		
		// if no family successor was found, the title becomes extinct if it is only a titular one (an aristocratic title whose corresponding faction does not actually hold territory)
		if (!this->CanHaveSuccession(title, false) || title != CharacterTitleHeadOfState) {
			this->Ministers[title] = NULL;
			return;
		}
	}
	
	CGrandStrategyHero *best_candidate = NULL;
	int best_score = 0;
			
	for (size_t i = 0; i < GrandStrategyGame.Heroes.size(); ++i) {
		if (
			GrandStrategyGame.Heroes[i]->IsAlive()
			&& GrandStrategyGame.Heroes[i]->IsVisible()
			&& (
				(GrandStrategyGame.Heroes[i]->Province != NULL && GrandStrategyGame.Heroes[i]->Province->Owner == this)
				|| (GrandStrategyGame.Heroes[i]->Province == NULL && GrandStrategyGame.Heroes[i]->ProvinceOfOrigin != NULL && GrandStrategyGame.Heroes[i]->ProvinceOfOrigin->Owner == this)
			)
			&& !GrandStrategyGame.Heroes[i]->Custom
			&& GrandStrategyGame.Heroes[i]->IsEligibleForTitle(title)
		) {
			int score = GrandStrategyGame.Heroes[i]->GetTitleScore(title);
			if (score > best_score) {
				best_candidate = GrandStrategyGame.Heroes[i];
				best_score = score;
			}
		}
	}
	if (best_candidate != NULL) {
		this->SetMinister(title, best_candidate->GetFullName());
		return;
	}

	this->Ministers[title] = NULL;
}

bool CGrandStrategyFaction::IsAlive()
{
	return this->OwnedProvinces.size() > 0;
}

bool CGrandStrategyFaction::HasTechnologyClass(std::string technology_class_name)
{
	if (this->Civilization == -1 || technology_class_name.empty()) {
		return false;
	}
	
	int technology_id = PlayerRaces.GetFactionClassUpgrade(this->Faction, GetUpgradeClassIndexByName(technology_class_name));
	
	if (technology_id != -1 && this->Technologies[technology_id] == true) {
		return true;
	}

	return false;
}

bool CGrandStrategyFaction::CanHaveSuccession(int title, bool family_inheritance)
{
	if (!this->IsAlive() && (title != CharacterTitleHeadOfState || !family_inheritance || PlayerRaces.Factions[this->Faction]->Type == FactionTypeTribe || this->GovernmentType != GovernmentTypeMonarchy)) { // head of state titles can be inherited even if their respective factions have no provinces, but if the line dies out then the title becomes extinct; tribal titles cannot be titular-only
		return false;
	}
	
	return true;
}

bool CGrandStrategyFaction::IsConquestDesirable(CGrandStrategyProvince *province)
{
	if (this->OwnedProvinces.size() == 1 && province->Owner == NULL && PlayerRaces.Factions[this->Faction]->Type == FactionTypeTribe) {
		if (province->GetDesirabilityRating() <= GrandStrategyGame.Provinces[this->OwnedProvinces[0]]->GetDesirabilityRating()) { // if conquering the province would trigger a migration, the conquest is only desirable if the province is worth more
			return false;
		}
	}
	
	return true;
}

int CGrandStrategyFaction::GetProductionEfficiencyModifier(int resource)
{
	int modifier = this->ProductionEfficiencyModifier[resource];
	
	return modifier;
}

int CGrandStrategyFaction::GetTroopCostModifier()
{
	int modifier = 0;
	
	if (this->Ministers[CharacterTitleHeadOfState] != NULL) {
		modifier += this->Ministers[CharacterTitleHeadOfState]->GetTroopCostModifier();
	}
	
	if (this->Ministers[CharacterTitleWarMinister] != NULL) {
		modifier += this->Ministers[CharacterTitleWarMinister]->GetTroopCostModifier();
	}
	
	return modifier;
}

int CGrandStrategyFaction::GetDiplomacyState(CGrandStrategyFaction *faction)
{
	if (this->DiplomacyStates.find(faction) != this->DiplomacyStates.end()) {
		return this->DiplomacyStates[faction];
	} else {
		return DiplomacyStatePeace;
	}
}

int CGrandStrategyFaction::GetDiplomacyStateProposal(CGrandStrategyFaction *faction)
{
	if (this->DiplomacyStateProposals.find(faction) != this->DiplomacyStateProposals.end()) {
		return this->DiplomacyStateProposals[faction];
	} else {
		return -1;
	}
}

std::string CGrandStrategyFaction::GetFullName()
{
	if (PlayerRaces.Factions[this->Faction]->Type == FactionTypeTribe) {
		return PlayerRaces.Factions[this->Faction]->Name;
	} else if (PlayerRaces.Factions[this->Faction]->Type == FactionTypePolity) {
//		return this->GetTitle() + " of " + PlayerRaces.Factions[this->Faction]->Name;
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
		return NULL;
	}
}

GrandStrategyWorldMapTile *CGrandStrategyFaction::GetCapitalSettlement()
{
	if (this->Capital == NULL) {
		return NULL;
	}
	
	return GrandStrategyGame.WorldMapTiles[this->Capital->SettlementLocation.x][this->Capital->SettlementLocation.y];
}

void CGrandStrategyHero::Die()
{
	//show message that the hero has died
	/*
	if (GrandStrategyGameInitialized && this->IsVisible()) {
		if (GrandStrategyGame.PlayerFaction != NULL && GrandStrategyGame.PlayerFaction->Ministers[CharacterTitleHeadOfState] == this) {
			char buf[256];
			snprintf(
				buf, sizeof(buf), "if (GenericDialog ~= nil) then GenericDialog(\"%s\", \"%s\") end;",
				(GrandStrategyGame.PlayerFaction->GetCharacterTitle(CharacterTitleHeadOfState, this->Gender) + " " + this->GetFullName() + " Dies").c_str(),
				("Tragic news spread throughout our realm. Our " + FullyDecapitalizeString(GrandStrategyGame.PlayerFaction->GetCharacterTitle(CharacterTitleHeadOfState, this->Gender)) + ", " + this->GetFullName() + ", has died! May his soul rest in peace.").c_str()
			);
			CclCommand(buf);	
		} else if (this->GetFaction() == GrandStrategyGame.PlayerFaction) {
			char buf[256];
			snprintf(
				buf, sizeof(buf), "if (GenericDialog ~= nil) then GenericDialog(\"%s\", \"%s\") end;",
				(this->GetBestDisplayTitle() + " " + this->GetFullName() + " Dies").c_str(),
				("My lord, the " + FullyDecapitalizeString(this->GetBestDisplayTitle()) + " " + this->GetFullName() + " has died!").c_str()
			);
			CclCommand(buf);	
		}
	}
	*/
	
	if (this->Province != NULL) {
		if (this->State == 2) {
			this->Province->MilitaryScore -= (this->Type->DefaultStat.Variables[POINTS_INDEX].Value + (this->Province->Owner != NULL ? this->Province->Owner->MilitaryScoreBonus[this->Type->Slot] : 0));
		} else if (this->State == 3) {
			this->Province->AttackingMilitaryScore -= (this->Type->DefaultStat.Variables[POINTS_INDEX].Value + (this->Province->AttackedBy != NULL ? this->Province->AttackedBy->MilitaryScoreBonus[this->Type->Slot] : 0));
		}
		
		this->Province->Heroes.erase(std::remove(this->Province->Heroes.begin(), this->Province->Heroes.end(), this), this->Province->Heroes.end());  //remove the hero from its province
		if (this->IsActive()) {
			this->Province->ActiveHeroes.erase(std::remove(this->Province->ActiveHeroes.begin(), this->Province->ActiveHeroes.end(), this), this->Province->ActiveHeroes.end());  //remove the hero from its province
		}
	}
	
	this->State = 0;

	//check if the hero has government positions in a faction, and if so, remove it from that position
	int titles_size = this->Titles.size();
	for (int i = (titles_size - 1); i >= 0; --i) {
		this->Titles[i].second->SetMinister(this->Titles[i].first, "");
	}
	
	this->Province = NULL;
}

void CGrandStrategyHero::SetType(int unit_type_id)
{
	if (this->Province != NULL) {
		if (this->State == 2) {
			this->Province->MilitaryScore -= (this->Type->DefaultStat.Variables[POINTS_INDEX].Value + (this->Province->Owner != NULL ? this->Province->Owner->MilitaryScoreBonus[this->Type->Slot] : 0));
		} else if (this->State == 3) {
			this->Province->AttackingMilitaryScore -= (this->Type->DefaultStat.Variables[POINTS_INDEX].Value + (this->Province->AttackedBy != NULL ? this->Province->AttackedBy->MilitaryScoreBonus[this->Type->Slot] : 0));
		}
	}
	
	//if the hero's unit type changed
	if (unit_type_id != this->Type->Slot) {
		this->Type = UnitTypes[unit_type_id];
	}
	
	this->UpdateAttributes();
	
	if (this->Province != NULL) {
		if (this->State == 2) {
			this->Province->MilitaryScore += (this->Type->DefaultStat.Variables[POINTS_INDEX].Value + (this->Province->Owner != NULL ? this->Province->Owner->MilitaryScoreBonus[this->Type->Slot] : 0));
		} else if (this->State == 3) {
			this->Province->AttackingMilitaryScore += (this->Type->DefaultStat.Variables[POINTS_INDEX].Value + (this->Province->AttackedBy != NULL ? this->Province->AttackedBy->MilitaryScoreBonus[this->Type->Slot] : 0));
		}
	}
}

bool CGrandStrategyHero::IsAlive()
{
	return this->State != 0;
}

bool CGrandStrategyHero::IsVisible()
{
	return this->Type->DefaultStat.Variables[GENDER_INDEX].Value == 0 || this->Gender == this->Type->DefaultStat.Variables[GENDER_INDEX].Value; // hero not visible if their unit type has a set gender which is different from the hero's (this is because of instances where i.e. females have a unit type that only has male portraits)
}

bool CGrandStrategyHero::IsActive()
{
	return this->IsVisible() && IsOffensiveMilitaryUnit(*this->Type);
}

bool CGrandStrategyHero::IsGenerated()
{
	return !this->Custom && GetCharacter(this->GetFullName()) == NULL;
}

bool CGrandStrategyHero::IsEligibleForTitle(int title)
{
	if (this->GetFaction()->GovernmentType == GovernmentTypeMonarchy && title == CharacterTitleHeadOfState && this->Type->Class != -1 && UnitTypeClasses[this->Type->Class] == "worker") { // commoners cannot become monarchs
		return false;
	} else if (this->GetFaction()->GovernmentType == GovernmentTypeTheocracy && title == CharacterTitleHeadOfState && this->Type->Class != -1 && UnitTypeClasses[this->Type->Class] != "priest") { // non-priests cannot rule theocracies
		return false;
	}
	
	for (size_t i = 0; i < this->Titles.size(); ++i) {
		if (this->Titles[i].first == CharacterTitleHeadOfState && this->Titles[i].second->IsAlive() && title != CharacterTitleHeadOfState) { // if it is not a head of state title, and this character is already the head of state of a living faction, return false
			return false;
		} else if (this->Titles[i].first == CharacterTitleHeadOfGovernment && title != CharacterTitleHeadOfState) { // if is already a head of government, don't accept ministerial titles of lower rank (that is, any but the title of head of state)
			return false;
		} else if (this->Titles[i].first != CharacterTitleHeadOfState && this->Titles[i].first != CharacterTitleHeadOfGovernment && title != CharacterTitleHeadOfState && title != CharacterTitleHeadOfGovernment) { // if is already a minister, don't accept another ministerial title of equal rank
			return false;
		}
	}
	
	for (size_t i = 0; i < this->ProvinceTitles.size(); ++i) {
		if (this->ProvinceTitles[i].first != CharacterTitleHeadOfState && this->ProvinceTitles[i].first != CharacterTitleHeadOfGovernment && title != CharacterTitleHeadOfState && title != CharacterTitleHeadOfGovernment) { // if already has a government position, don't accept another ministerial title of equal rank
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

int CGrandStrategyHero::GetLanguage()
{
	return PlayerRaces.GetCivilizationLanguage(this->Civilization);
}

int CGrandStrategyHero::GetTitleScore(int title, CGrandStrategyProvince *province)
{
	int score = 0;
	if (title == CharacterTitleHeadOfState) {
		score = ((this->Attributes[IntelligenceAttribute] + ((this->Attributes[this->GetMartialAttribute()] + this->Attributes[IntelligenceAttribute]) / 2) + this->Attributes[CharismaAttribute]) / 3) + 1;
	} else if (title == CharacterTitleHeadOfGovernment) {
		score = ((this->Attributes[IntelligenceAttribute] + ((this->Attributes[this->GetMartialAttribute()] + this->Attributes[IntelligenceAttribute]) / 2) + this->Attributes[CharismaAttribute]) / 3) + 1;
	} else if (title == CharacterTitleEducationMinister) {
		score = this->Attributes[IntelligenceAttribute];
	} else if (title == CharacterTitleFinanceMinister) {
		score = this->Attributes[IntelligenceAttribute];
	} else if (title == CharacterTitleWarMinister) {
		score = (this->Attributes[this->GetMartialAttribute()] + this->Attributes[IntelligenceAttribute]) / 2;
	} else if (title == CharacterTitleInteriorMinister) {
		score = this->Attributes[IntelligenceAttribute];
	} else if (title == CharacterTitleJusticeMinister) {
		score = this->Attributes[IntelligenceAttribute];
	} else if (title == CharacterTitleForeignMinister) {
		score = (this->Attributes[CharismaAttribute] + this->Attributes[IntelligenceAttribute]) / 2;
	} else if (title == CharacterTitleGovernor) {
		score = ((this->Attributes[IntelligenceAttribute] + ((this->Attributes[this->GetMartialAttribute()] + this->Attributes[IntelligenceAttribute]) / 2) + this->Attributes[CharismaAttribute]) / 3);
		if (province != NULL && (province == this->Province || province == this->ProvinceOfOrigin)) {
			score += 1;
		}
	}
	
	if (this->Civilization != this->GetFaction()->Civilization) {
		score -= 1; //characters of a different culture than the faction they are in have a smaller chance of getting a ministerial or gubernatorial position
	}
	
	return score;
}

std::string CGrandStrategyHero::GetMinisterEffectsString(int title)
{
	std::string minister_effects_string;
	
	bool first = true;
	
	if (title == CharacterTitleHeadOfState || title == CharacterTitleWarMinister) {
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
			minister_effects_string += std::to_string((long long) modifier) + "% Troop Cost";
		}
	}
	
	return minister_effects_string;
}

std::string CGrandStrategyHero::GetBestDisplayTitle()
{
	std::string best_title = this->Type->Name;
	int best_title_type = MaxCharacterTitles;
	/*
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
	if (this->Province != NULL) {
		if (this->State == 3) {
			return this->Province->AttackedBy;
		} else {
			return this->Province->Owner;
		}
	} else {
		return this->ProvinceOfOrigin->Owner;
	}
	
	return NULL;
}

CGrandStrategyEvent::~CGrandStrategyEvent()
{
	delete Conditions;
	
	for (size_t i = 0; i < this->OptionConditions.size(); ++i) {
		delete this->OptionConditions[i];
	}
	
	for (size_t i = 0; i < this->OptionEffects.size(); ++i) {
		delete this->OptionEffects[i];
	}
}

void CGrandStrategyEvent::Trigger(CGrandStrategyFaction *faction)
{
//	fprintf(stderr, "Triggering event \"%s\" for faction %s.\n", this->Name.c_str(), PlayerRaces.Factions[faction->Faction]->Name.c_str());	
	
	CclCommand("EventFaction = GetFactionFromName(\"" + PlayerRaces.Factions[faction->Faction]->Ident + "\");");
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
//	fprintf(stderr, "Checking for triggers for event \"%s\" for faction %s.\n", this->Name.c_str(), PlayerRaces.Factions[faction->Faction]->Name.c_str());	
	
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
**  Get the terrain variation of a world map tile.
*/
int GetWorldMapTileTerrainVariation(int x, int y)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	Assert(GrandStrategyGame.WorldMapTiles[x][y]->Terrain != -1);
	Assert(GrandStrategyGame.WorldMapTiles[x][y]->Variation != -1);
	
	return GrandStrategyGame.WorldMapTiles[x][y]->Variation + 1;
}

std::string GetWorldMapTileProvinceName(int x, int y)
{
	
	clamp(&x, 0, GrandStrategyGame.WorldMapWidth - 1);
	clamp(&y, 0, GrandStrategyGame.WorldMapHeight - 1);

	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	if (GrandStrategyGame.WorldMapTiles[x][y]->Province != NULL) {
		return GrandStrategyGame.WorldMapTiles[x][y]->Province->Name;
	} else {
		return "";
	}
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

/**
**  Set the size of the world map.
*/
void SetWorldMapSize(int width, int height)
{
	Assert(width <= WorldMapWidthMax);
	Assert(height <= WorldMapHeightMax);
	GrandStrategyGame.WorldMapWidth = width;
	GrandStrategyGame.WorldMapHeight = height;
	
	//create new world map tile objects for the size, if necessary
	if (!GrandStrategyGame.WorldMapTiles[width - 1][height - 1]) {
		for (int x = 0; x < GrandStrategyGame.WorldMapWidth; ++x) {
			for (int y = 0; y < GrandStrategyGame.WorldMapHeight; ++y) {
				if (!GrandStrategyGame.WorldMapTiles[x][y]) {
					GrandStrategyWorldMapTile *world_map_tile = new GrandStrategyWorldMapTile;
					GrandStrategyGame.WorldMapTiles[x][y] = world_map_tile;
					GrandStrategyGame.WorldMapTiles[x][y]->Position.x = x;
					GrandStrategyGame.WorldMapTiles[x][y]->Position.y = y;
				}
			}
		}
	}
}

/**
**  Set the terrain type of a world map tile.
*/
void SetWorldMapTileTerrain(int x, int y, int terrain)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	//if tile doesn't exist, create it now
	if (!GrandStrategyGame.WorldMapTiles[x][y]) {
		GrandStrategyWorldMapTile *world_map_tile = new GrandStrategyWorldMapTile;
		GrandStrategyGame.WorldMapTiles[x][y] = world_map_tile;
		GrandStrategyGame.WorldMapTiles[x][y]->Position.x = x;
		GrandStrategyGame.WorldMapTiles[x][y]->Position.y = y;
	}
	
	GrandStrategyGame.WorldMapTiles[x][y]->Terrain = terrain;
	
	if (terrain != -1 && WorldMapTerrainTypes[terrain]) {
		//randomly select a variation for the world map tile
		if (WorldMapTerrainTypes[terrain]->Variations > 0) {
			GrandStrategyGame.WorldMapTiles[x][y]->Variation = SyncRand(WorldMapTerrainTypes[terrain]->Variations);
		} else {
			GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
		}
		
		int base_terrain = WorldMapTerrainTypes[terrain]->BaseTile;
		if (base_terrain != -1 && WorldMapTerrainTypes[base_terrain]) {
			//randomly select a variation for the world map tile
			if (WorldMapTerrainTypes[base_terrain]->Variations > 0) {
				GrandStrategyGame.WorldMapTiles[x][y]->BaseTileVariation = SyncRand(WorldMapTerrainTypes[base_terrain]->Variations);
			} else {
				GrandStrategyGame.WorldMapTiles[x][y]->BaseTileVariation = -1;
			}
		}
	}
}

void SetWorldMapTileProvince(int x, int y, std::string province_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	CGrandStrategyProvince *old_province = GrandStrategyGame.WorldMapTiles[x][y]->Province;
	if (old_province != NULL) { //if the tile is already assigned to a province, remove it from that province's tile arrays
		old_province->Tiles.erase(std::remove(old_province->Tiles.begin(), old_province->Tiles.end(), Vec2i(x, y)), old_province->Tiles.end());
		
		if (GrandStrategyGame.WorldMapTiles[x][y]->Resource != -1) {
			int res = GrandStrategyGame.WorldMapTiles[x][y]->Resource;
			if (GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected) {
				old_province->ProductionCapacity[res] -= 1;
			}
			old_province->ResourceTiles[res].erase(std::remove(old_province->ResourceTiles[res].begin(), old_province->ResourceTiles[res].end(), Vec2i(x, y)), old_province->ResourceTiles[res].end());
		}
	}

	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.WorldMapTiles[x][y]->Province = GrandStrategyGame.Provinces[province_id];
		//now add the tile to the province's tile arrays
		GrandStrategyGame.Provinces[province_id]->Tiles.push_back(Vec2i(x, y));
		
		if (GrandStrategyGame.WorldMapTiles[x][y]->Resource != -1) {
			int res = GrandStrategyGame.WorldMapTiles[x][y]->Resource;
			if (GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected) {
				GrandStrategyGame.Provinces[province_id]->ProductionCapacity[res] += 1;
			}
			GrandStrategyGame.Provinces[province_id]->ResourceTiles[res].push_back(Vec2i(x, y));
		}
	} else {
		GrandStrategyGame.WorldMapTiles[x][y]->Province = NULL;
	}
}

/**
**  Set the name of a world map tile.
*/
void SetWorldMapTileName(int x, int y, std::string name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	GrandStrategyGame.WorldMapTiles[x][y]->Name = name;
}

void SetWorldMapTileCulturalTerrainName(int x, int y, std::string terrain_name, std::string civilization_name, std::string cultural_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	int terrain = GetWorldMapTerrainTypeId(terrain_name);
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	if (terrain != -1 && civilization != -1) {
		GrandStrategyGame.WorldMapTiles[x][y]->CulturalTerrainNames[std::pair<int, int>(terrain, civilization)].push_back(TransliterateText(cultural_name));
	}
}

void SetWorldMapTileFactionCulturalTerrainName(int x, int y, std::string terrain_name, std::string civilization_name, std::string faction_name, std::string cultural_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	if (GrandStrategyGame.WorldMapTiles[x][y]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		int terrain = GetWorldMapTerrainTypeId(terrain_name);
		if (terrain != -1 && civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(faction_name);
			if (faction != -1) {
				GrandStrategyGame.WorldMapTiles[x][y]->FactionCulturalTerrainNames[std::pair<int, CFaction *>(terrain, PlayerRaces.Factions[faction])].push_back(TransliterateText(cultural_name));
			}
		}
	}
}

void SetWorldMapTileCulturalResourceName(int x, int y, std::string resource_name, std::string civilization_name, std::string cultural_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	int resource = GetResourceIdByName(resource_name.c_str());
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	if (resource != -1 && civilization != -1) {
		GrandStrategyGame.WorldMapTiles[x][y]->CulturalResourceNames[std::pair<int, int>(resource, civilization)].push_back(TransliterateText(cultural_name));
	}
}

void SetWorldMapTileFactionCulturalResourceName(int x, int y, std::string resource_name, std::string civilization_name, std::string faction_name, std::string cultural_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	if (GrandStrategyGame.WorldMapTiles[x][y]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		int resource = GetResourceIdByName(resource_name.c_str());
		if (resource != -1 && civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(faction_name);
			if (faction != -1) {
				GrandStrategyGame.WorldMapTiles[x][y]->FactionCulturalResourceNames[std::pair<int, CFaction *>(resource, PlayerRaces.Factions[faction])].push_back(TransliterateText(cultural_name));
			}
		}
	}
}

/**
**  Set the cultural name of a world map tile for a particular civilization.
*/
void SetWorldMapTileCulturalSettlementName(int x, int y, std::string civilization_name, std::string cultural_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	if (civilization != -1) {
		GrandStrategyGame.WorldMapTiles[x][y]->CulturalSettlementNames[civilization].push_back(TransliterateText(cultural_name));
	}
}

void SetWorldMapTileFactionCulturalSettlementName(int x, int y, std::string civilization_name, std::string faction_name, std::string cultural_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	if (GrandStrategyGame.WorldMapTiles[x][y]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(faction_name);
			if (faction != -1) {
				GrandStrategyGame.WorldMapTiles[x][y]->FactionCulturalSettlementNames[PlayerRaces.Factions[faction]].push_back(TransliterateText(cultural_name));
			}
		}
	}
}

void AddWorldMapResource(std::string resource_name, int x, int y, bool discovered)
{
	CGrandStrategyProvince *province = GrandStrategyGame.WorldMapTiles[x][y]->Province;
	if (GrandStrategyGame.WorldMapTiles[x][y]->Resource != -1) { //if tile already has a resource, remove it from the old resource's arrays
		int old_resource = GrandStrategyGame.WorldMapTiles[x][y]->Resource;
		for (int i = 0; i < WorldMapResourceMax; ++i) { // remove it from the world map resources array
			if (GrandStrategyGame.WorldMapResources[old_resource][i].x == x && GrandStrategyGame.WorldMapResources[old_resource][i].y == y) { //if tile was found, push every element of the array after it back one step
				for (int j = i; j < WorldMapResourceMax; ++j) {
					GrandStrategyGame.WorldMapResources[old_resource][j].x = GrandStrategyGame.WorldMapResources[old_resource][j + 1].x;
					GrandStrategyGame.WorldMapResources[old_resource][j].y = GrandStrategyGame.WorldMapResources[old_resource][j + 1].y;
					if (GrandStrategyGame.WorldMapResources[old_resource][j].x == -1 && GrandStrategyGame.WorldMapResources[old_resource][j].y == -1) { // if this is a blank tile slot
						break;
					}
				}
				break;
			}
			if (GrandStrategyGame.WorldMapResources[old_resource][i].x == -1 && GrandStrategyGame.WorldMapResources[old_resource][i].y == -1) { // if this is a blank tile slot
				break;
			}
		}
	
		if (province != NULL) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected) {
				province->ProductionCapacity[old_resource] -= 1;
			}
			province->ResourceTiles[old_resource].erase(std::remove(province->ResourceTiles[old_resource].begin(), province->ResourceTiles[old_resource].end(), Vec2i(x, y)), province->ResourceTiles[old_resource].end());
		}
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource != -1) {
		for (int i = 0; i < WorldMapResourceMax; ++i) {
			if (GrandStrategyGame.WorldMapResources[resource][i].x == -1 && GrandStrategyGame.WorldMapResources[resource][i].y == -1) { //if this spot for a world map resource is blank
				GrandStrategyGame.WorldMapResources[resource][i].x = x;
				GrandStrategyGame.WorldMapResources[resource][i].y = y;
				GrandStrategyGame.WorldMapTiles[x][y]->Resource = resource;
				GrandStrategyGame.WorldMapTiles[x][y]->SetResourceProspected(resource, discovered);
				break;
			}
		}
		if (province != NULL) {
			province->ResourceTiles[resource].push_back(Vec2i(x, y));
		}
	}
}

void SetWorldMapResourceProspected(std::string resource_name, int x, int y, bool discovered)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource != -1) {
		GrandStrategyGame.WorldMapTiles[x][y]->SetResourceProspected(resource, discovered);
	}
}

std::string GetProvinceAttackedBy(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		if (GrandStrategyGame.Provinces[province_id]->AttackedBy != NULL) {
			return PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->AttackedBy->Faction]->Ident;
		}
	}
	
	return "";
}

void SetProvinceName(std::string old_province_name, std::string new_province_name)
{
	int province_id = GetProvinceId(old_province_name);

	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->Name = new_province_name;
	}
}

void SetProvinceWater(std::string province_name, bool water)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->Water = water;
	}
}

void SetProvinceOwner(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	int civilization_id = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction_id = PlayerRaces.GetFactionIndexByName(faction_name);
	
	if (province_id == -1 || !GrandStrategyGame.Provinces[province_id]) {
		return;
	}
	
	GrandStrategyGame.Provinces[province_id]->SetOwner(civilization_id, faction_id);
}

void SetProvinceSettlementLocation(std::string province_name, int x, int y)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->SetSettlementLocation(x, y);
	}
}

void SetProvinceCulturalName(std::string province_name, std::string civilization_name, std::string province_cultural_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			GrandStrategyGame.Provinces[province_id]->CulturalNames[civilization] = TransliterateText(province_cultural_name);
		}
	}
}

void SetProvinceFactionCulturalName(std::string province_name, std::string civilization_name, std::string faction_name, std::string province_cultural_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(faction_name);
			if (faction != -1) {
				GrandStrategyGame.Provinces[province_id]->FactionCulturalNames[PlayerRaces.Factions[faction]] = TransliterateText(province_cultural_name);
			}
		}
	}
}

void SetProvinceSettlementBuilding(std::string province_name, std::string settlement_building_ident, bool has_settlement_building)
{
	int province_id = GetProvinceId(province_name);
	int settlement_building = UnitTypeIdByIdent(settlement_building_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && settlement_building != -1) {
		GrandStrategyGame.Provinces[province_id]->SetSettlementBuilding(settlement_building, has_settlement_building);
	}
}

void SetProvincePopulation(std::string province_name, int quantity)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->SetPopulation(quantity);
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

void SetProvinceUnderConstructionUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && unit_type != -1) {
		GrandStrategyGame.Provinces[province_id]->UnderConstructionUnits[unit_type] = std::max(0, quantity);
	}
}

void SetProvinceMovingUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && unit_type != -1) {
		GrandStrategyGame.Provinces[province_id]->SetMovingUnitQuantity(unit_type, quantity);
	}
}

void SetProvinceAttackingUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && unit_type != -1) {
		GrandStrategyGame.Provinces[province_id]->SetAttackingUnitQuantity(unit_type, quantity);
	}
}

void SetProvinceHero(std::string province_name, std::string hero_full_name, int value)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->SetHero(hero_full_name, value);
	}
}

void SetProvinceFood(std::string province_name, int quantity)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->PopulationGrowthProgress = std::max(0, quantity);
	}
}

void ChangeProvinceFood(std::string province_name, int quantity)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->PopulationGrowthProgress += quantity;
		GrandStrategyGame.Provinces[province_id]->PopulationGrowthProgress = std::max(0, GrandStrategyGame.Provinces[province_id]->PopulationGrowthProgress);
	}
}

void SetProvinceAttackedBy(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization_id = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		int faction_id = PlayerRaces.GetFactionIndexByName(faction_name);
		if (civilization_id != -1 && faction_id != -1) {
			GrandStrategyGame.Provinces[province_id]->AttackedBy = const_cast<CGrandStrategyFaction *>(&(*GrandStrategyGame.Factions[civilization_id][faction_id]));
		} else {
			GrandStrategyGame.Provinces[province_id]->AttackedBy = NULL;
		}
	}
}

void AddProvinceClaim(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(faction_name);
			if (faction != -1) {
				GrandStrategyGame.Provinces[province_id]->AddFactionClaim(civilization, faction);
			} else {
				fprintf(stderr, "Can't find %s faction (%s) to add claim to province %s.\n", faction_name.c_str(), civilization_name.c_str(), province_name.c_str());
			}
		} else {
			fprintf(stderr, "Can't find %s civilization to add the claim of its %s faction claim to province %s.\n", civilization_name.c_str(), faction_name.c_str(), province_name.c_str());
		}
	} else {
		fprintf(stderr, "Can't find %s province to add %s faction (%s) claim to.\n", province_name.c_str(), faction_name.c_str(), civilization_name.c_str());
	}
}

void RemoveProvinceClaim(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(faction_name);
			if (faction != -1) {
				GrandStrategyGame.Provinces[province_id]->RemoveFactionClaim(civilization, faction);
			}
		}
	}
}

/**
**  Clean the grand strategy variables.
*/
void CleanGrandStrategyGame()
{
	for (int x = 0; x < WorldMapWidthMax; ++x) {
		for (int y = 0; y < WorldMapHeightMax; ++y) {
			if (GrandStrategyGame.WorldMapTiles[x][y]) {
				delete GrandStrategyGame.WorldMapTiles[x][y];
				GrandStrategyGame.WorldMapTiles[x][y] = NULL;
			} else {
				break;
			}
		}
	}
		
	for (int i = 0; i < MAX_RACES; ++i) {
		for (size_t j = 0; j < GrandStrategyGame.Factions[i].size(); ++j) {
			delete GrandStrategyGame.Factions[i][j];
		}
		GrandStrategyGame.Factions[i].clear();
	}
	
	for (int i = 0; i < MaxCosts; ++i) {
		GrandStrategyGame.CommodityPrices[i] = 0;
		for (int j = 0; j < WorldMapResourceMax; ++j) {
			if (GrandStrategyGame.WorldMapResources[i][j].x != -1 || GrandStrategyGame.WorldMapResources[i][j].y != -1) {
				GrandStrategyGame.WorldMapResources[i][j].x = -1;
				GrandStrategyGame.WorldMapResources[i][j].y = -1;
			} else {
				break;
			}
		}
	}
	
	for (size_t i = 0; i < GrandStrategyGame.Provinces.size(); ++i) {
		delete GrandStrategyGame.Provinces[i];
	}
	GrandStrategyGame.Provinces.clear();
	GrandStrategyGame.CultureProvinces.clear();

	for (size_t i = 0; i < GrandStrategyGame.Heroes.size(); ++i) {
		delete GrandStrategyGame.Heroes[i];
	}
	GrandStrategyGame.Heroes.clear();
	GrandStrategyHeroStringToIndex.clear();
	
	GrandStrategyGame.UnpublishedWorks.clear();
	GrandStrategyGame.AvailableEvents.clear();
	
	GrandStrategyGame.WorldMapWidth = 0;
	GrandStrategyGame.WorldMapHeight = 0;
	GrandStrategyGame.SelectedUnits.clear();
	GrandStrategyGame.PlayerFaction = NULL;
	
	WorldMapOffsetX = 0;
	WorldMapOffsetY = 0;
	GrandStrategyMapWidthIndent = 0;
	GrandStrategyMapHeightIndent = 0;
	
	GrandStrategyGameInitialized = false;
}

void InitializeGrandStrategyGame(bool show_loading)
{
	//initialize literary works
	for (size_t i = 0; i < AllUpgrades.size(); ++i) {
		if (AllUpgrades[i]->Work == -1 || AllUpgrades[i]->UniqueOnly) { // literary works that can only appear in unique items wouldn't be publishable
			continue;
		}
		
		GrandStrategyGame.UnpublishedWorks.push_back(AllUpgrades[i]);
	}
}

void FinalizeGrandStrategyInitialization()
{
	CWorld *world = GetWorld(GrandStrategyWorld);
	
	if (world != NULL) { // create tiles which should be randomly placed
		for (size_t i = 0; i < world->Tiles.size(); ++i) {
			if (world->Tiles[i]->Position.x != -1 && world->Tiles[i]->Position.y != -1) {
				continue;
			}
			
			std::vector<GrandStrategyWorldMapTile *> potential_tiles;			
			
			if (world->Tiles[i]->Province != NULL && GetProvinceId(world->Tiles[i]->Province->Name) != -1) {
				CGrandStrategyProvince *province = GrandStrategyGame.Provinces[GetProvinceId(world->Tiles[i]->Province->Name)];
				for (size_t j = 0; j < province->Tiles.size(); ++j) {
					GrandStrategyWorldMapTile *province_tile = GrandStrategyGame.WorldMapTiles[province->Tiles[j].x][province->Tiles[j].y];
					
					if (
						(world->Tiles[i]->Terrain == -1 || world->Tiles[i]->Terrain == province_tile->Terrain)
						&& (world->Tiles[i]->Resource == -1 || world->Tiles[i]->Resource == province_tile->Resource)
						&& world->Tiles[i]->Capital == (province->SettlementLocation == province_tile->Position)
					) {
						potential_tiles.push_back(province_tile);
					}
				}
			} else {
				for (size_t j = 0; j < GrandStrategyGame.Provinces.size(); ++j) {
					CGrandStrategyProvince *province = GrandStrategyGame.Provinces[j];
					for (size_t k = 0; k < province->Tiles.size(); ++k) {
						GrandStrategyWorldMapTile *province_tile = GrandStrategyGame.WorldMapTiles[province->Tiles[k].x][province->Tiles[k].y];
						
						if (
							(world->Tiles[i]->Terrain == -1 || world->Tiles[i]->Terrain == province_tile->Terrain)
							&& (world->Tiles[i]->Resource == -1 || world->Tiles[i]->Resource == province_tile->Resource)
							&& world->Tiles[i]->Capital == (province->SettlementLocation == province_tile->Position)
						) {
							potential_tiles.push_back(province_tile);
						}
					}
				}
			}
			
			if (potential_tiles.size() > 0) {
				GrandStrategyWorldMapTile *tile = potential_tiles[SyncRand(potential_tiles.size())];
				
				tile->CulturalTerrainNames = world->Tiles[i]->CulturalTerrainNames;
				tile->FactionCulturalTerrainNames = world->Tiles[i]->FactionCulturalTerrainNames;
				tile->CulturalResourceNames = world->Tiles[i]->CulturalResourceNames;
				tile->FactionCulturalResourceNames = world->Tiles[i]->FactionCulturalResourceNames;
				tile->CulturalSettlementNames = world->Tiles[i]->CulturalSettlementNames;
				tile->FactionCulturalSettlementNames = world->Tiles[i]->FactionCulturalSettlementNames;
			}
		}
	}
	
	//initialize literary works
	int works_size = GrandStrategyGame.UnpublishedWorks.size();
	for (int i = (works_size - 1); i >= 0; --i) {
		if (GrandStrategyGameLoading == false) {
			if (GrandStrategyGame.UnpublishedWorks[i]->Year != 0 && GrandStrategyYear >= GrandStrategyGame.UnpublishedWorks[i]->Year) { //if the game is starting after the publication date of this literary work, remove it from the work list
				GrandStrategyGame.UnpublishedWorks.erase(std::remove(GrandStrategyGame.UnpublishedWorks.begin(), GrandStrategyGame.UnpublishedWorks.end(), GrandStrategyGame.UnpublishedWorks[i]), GrandStrategyGame.UnpublishedWorks.end());
			}
		}
	}
	
	for (size_t i = 0; i < GrandStrategyGame.Provinces.size(); ++i) {
		CGrandStrategyProvince *province = GrandStrategyGame.Provinces[i];
		CProvince *base_province = GetProvince(province->Name);
		
		if (GrandStrategyGameLoading == false) {
			// add historical population from regions to provinces here, for a lack of a better place (all province's region belongings need to be defined before this takes place, so this can't happen during the province definitions)
			for (size_t j = 0; j < base_province->Regions.size(); ++j) {
				for (std::map<int, int>::iterator iterator = base_province->Regions[j]->HistoricalPopulation.begin(); iterator != base_province->Regions[j]->HistoricalPopulation.end(); ++iterator) {
					if (base_province->HistoricalPopulation.find(iterator->first) == base_province->HistoricalPopulation.end()) { // if the province doesn't have historical population information for a given year but the region does, then use the region's population quantity divided by the number of provinces the region has
						base_province->HistoricalPopulation[iterator->first] = iterator->second / base_province->Regions[j]->Provinces.size();
					}
				}
			}
		
			for (std::map<int, int>::reverse_iterator iterator = base_province->HistoricalPopulation.rbegin(); iterator != base_province->HistoricalPopulation.rend(); ++iterator) {
				if (GrandStrategyYear >= iterator->first) {
					province->SetPopulation(iterator->second);
					break;
				}
			}
				
			for (std::map<int, std::map<int, bool>>::iterator iterator = base_province->HistoricalSettlementBuildings.begin(); iterator != base_province->HistoricalSettlementBuildings.end(); ++iterator) {
				for (std::map<int, bool>::reverse_iterator second_iterator = iterator->second.rbegin(); second_iterator != iterator->second.rend(); ++second_iterator) {
					if (GrandStrategyYear >= second_iterator->first) {
						province->SetSettlementBuilding(iterator->first, second_iterator->second);
						break;
					}
				}
			}
				
			for (std::map<CUpgrade *, std::map<int, bool>>::iterator iterator = base_province->HistoricalModifiers.begin(); iterator != base_province->HistoricalModifiers.end(); ++iterator) {
				for (std::map<int, bool>::reverse_iterator second_iterator = iterator->second.rbegin(); second_iterator != iterator->second.rend(); ++second_iterator) {
					if (GrandStrategyYear >= second_iterator->first) {
						province->SetModifier(iterator->first, second_iterator->second);
						break;
					}
				}
			}
		}
		
		if (province->Civilization != -1 && province->Owner != NULL) {
			if (GrandStrategyGameLoading == false) {
				if (!province->HasBuildingClass("town-hall")) { // if the province has an owner but no town hall building, give it one; in the future we may want to have gameplay for provinces without town halls (for instance, for nomadic tribes), but at least until then, keep this in place
					province->SetSettlementBuilding(province->GetClassUnitType(GetUnitTypeClassIndexByName("town-hall")), true);
				}
				
				if (province->TotalWorkers < 4) { // make every province that has an owner start with at least four workers
					province->SetUnitQuantity(province->GetClassUnitType(GetUnitTypeClassIndexByName("worker")), 4);
				}

				if (province->GetClassUnitType(GetUnitTypeClassIndexByName("infantry")) != -1 && province->Units[province->GetClassUnitType(GetUnitTypeClassIndexByName("infantry"))] < 2) { // make every province that has an owner start with at least two infantry units
					province->SetUnitQuantity(province->GetClassUnitType(GetUnitTypeClassIndexByName("infantry")), 2);
				}
			}
		}
	}
}

void SetGrandStrategyWorld(std::string world)
{
	GrandStrategyWorld = world;
}

void DoGrandStrategyTurn()
{
	GrandStrategyGame.DoTurn();
}

void CalculateProvinceBorders()
{
	for (size_t i = 0; i < GrandStrategyGame.Provinces.size(); ++i) {
		for (size_t j = 0; j < GrandStrategyGame.Provinces[i]->Tiles.size(); ++j) {
			GrandStrategyGame.WorldMapTiles[GrandStrategyGame.Provinces[i]->Tiles[j].x][GrandStrategyGame.Provinces[i]->Tiles[j].y]->Province = GrandStrategyGame.Provinces[i]; //tell the tile it belongs to this province
		}
			
		GrandStrategyGame.Provinces[i]->BorderProvinces.clear();
			
		//calculate which of the province's tiles are border tiles, and which provinces it borders; also whether the province borders water (is coastal) or not
		for (size_t j = 0; j < GrandStrategyGame.Provinces[i]->Tiles.size(); ++j) {
			int x = GrandStrategyGame.Provinces[i]->Tiles[j].x;
			int y = GrandStrategyGame.Provinces[i]->Tiles[j].y;
			for (int sub_x = -1; sub_x <= 1; ++sub_x) {
				if ((x + sub_x) < 0 || (x + sub_x) >= GrandStrategyGame.WorldMapWidth) {
					continue;
				}
							
				for (int sub_y = -1; sub_y <= 1; ++sub_y) {
					if ((y + sub_y) < 0 || (y + sub_y) >= GrandStrategyGame.WorldMapHeight) {
						continue;
					}
							
					CGrandStrategyProvince *second_province = GrandStrategyGame.WorldMapTiles[x + sub_x][y + sub_y]->Province;
					if (!(sub_x == 0 && sub_y == 0) && second_province != GrandStrategyGame.Provinces[i] && GrandStrategyGame.WorldMapTiles[x + sub_x][y + sub_y]->Terrain != -1) {
						if (second_province == NULL || GrandStrategyGame.Provinces[i]->Water == second_province->Water) {
							int direction = DirectionToHeading(Vec2i(x + sub_x, y + sub_y) - Vec2i(x, y)) + (32 / 2);
							if (direction % 32 != 0) {
								direction = direction - (direction % 32);
							}
							direction = direction / 32;
								
							GrandStrategyGame.WorldMapTiles[x][y]->Borders[direction] = true;
						}
								
						if (second_province != NULL && !GrandStrategyGame.Provinces[i]->BordersProvince(second_province)) { //if isn't added yet to the border provinces, do so now
							GrandStrategyGame.Provinces[i]->BorderProvinces.push_back(second_province);
						}
					}
				}
			}
		}
	}
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
	int civilization = PlayerRaces.GetRaceIndexByName(faction_civilization_name.c_str());
	int faction = PlayerRaces.GetFactionIndexByName(faction_name);
	
	if (civilization == -1 || faction == -1) {
		return false;
	}
	
	return GrandStrategyGame.Provinces[province]->BordersFaction(civilization, faction);
}

bool ProvinceHasBuildingClass(std::string province_name, std::string building_class)
{
	int province_id = GetProvinceId(province_name);
	
	return GrandStrategyGame.Provinces[province_id]->HasBuildingClass(building_class);
}

bool ProvinceHasClaim(std::string province_name, std::string faction_civilization_name, std::string faction_name)
{
	int province = GetProvinceId(province_name);
	int civilization = PlayerRaces.GetRaceIndexByName(faction_civilization_name.c_str());
	int faction = PlayerRaces.GetFactionIndexByName(faction_name);
	
	if (civilization == -1 || faction == -1) {
		return false;
	}
	
	return GrandStrategyGame.Provinces[province]->HasFactionClaim(civilization, faction);
}

bool ProvinceHasResource(std::string province_name, std::string resource_name, bool ignore_prospection)
{
	int province_id = GetProvinceId(province_name);
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return false;
	}
	
	return GrandStrategyGame.Provinces[province_id]->HasResource(resource, ignore_prospection);
}

bool IsGrandStrategyBuilding(const CUnitType &type)
{
	if (type.BoolFlag[BUILDING_INDEX].value && type.Class != -1 && UnitTypeClasses[type.Class] != "farm" && UnitTypeClasses[type.Class] != "watch-tower" && UnitTypeClasses[type.Class] != "guard-tower") {
		return true;
	}
	return false;
}

std::string GetProvinceCivilization(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (GrandStrategyGame.Provinces[province_id]->Civilization != -1) {
		return PlayerRaces.Name[GrandStrategyGame.Provinces[province_id]->Civilization];
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

int GetProvinceUnderConstructionUnitQuantity(std::string province_name, std::string unit_type_ident)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	return GrandStrategyGame.Provinces[province_id]->UnderConstructionUnits[unit_type];
}

int GetProvinceMovingUnitQuantity(std::string province_name, std::string unit_type_ident)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	return GrandStrategyGame.Provinces[province_id]->MovingUnits[unit_type];
}

int GetProvinceAttackingUnitQuantity(std::string province_name, std::string unit_type_ident)
{
	int province_id = GetProvinceId(province_name);
	int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
	
	return GrandStrategyGame.Provinces[province_id]->GetAttackingUnitQuantity(unit_type_id);
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
		if (hero->Province != NULL && hero->Province->ID == province_id) {
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
	
	if (province_id == -1 || GrandStrategyGame.Provinces[province_id]->Owner == NULL) {
		return "";
	}
	
	return PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->Owner->Faction]->Ident;
}

void SetPlayerFaction(std::string civilization_name, std::string faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(faction_name);
	}
	
	if (faction == -1) {
		return;
	}
	
	GrandStrategyGame.PlayerFaction = const_cast<CGrandStrategyFaction *>(&(*GrandStrategyGame.Factions[civilization][faction]));
	UI.Load();
}

std::string GetPlayerFactionName()
{
	if (GrandStrategyGame.PlayerFaction != NULL) {
		return PlayerRaces.Factions[GrandStrategyGame.PlayerFaction->Faction]->Ident;
	} else {
		return "";
	}
}

void SetFactionResource(std::string civilization_name, std::string faction_name, std::string resource_name, int resource_quantity)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization][faction]->Resources[resource] = resource_quantity;
}

void ChangeFactionResource(std::string civilization_name, std::string faction_name, std::string resource_name, int resource_quantity)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization][faction]->Resources[resource] += resource_quantity;
}

int GetFactionResource(std::string civilization_name, std::string faction_name, std::string resource_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return 0;
	}
	
	return GrandStrategyGame.Factions[civilization][faction]->Resources[resource];
}

void SetFactionTechnology(std::string civilization_name, std::string faction_name, std::string upgrade_ident, bool has_technology)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int upgrade_id = UpgradeIdByIdent(upgrade_ident);
	if (civilization != -1 && upgrade_id != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(faction_name);
		if (faction != -1) {
			GrandStrategyGame.Factions[civilization][faction]->SetTechnology(upgrade_id, has_technology);
		}
	}
}

bool GetFactionTechnology(std::string civilization_name, std::string faction_name, std::string upgrade_ident)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int upgrade_id = UpgradeIdByIdent(upgrade_ident);
	if (civilization != -1 && upgrade_id != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(faction_name);
		if (faction != -1) {
			return GrandStrategyGame.Factions[civilization][faction]->Technologies[upgrade_id];
		}
	}
	
	return false;
}

void SetFactionGovernmentType(std::string civilization_name, std::string faction_name, std::string government_type_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	
	int government_type_id = GetGovernmentTypeIdByName(government_type_name);
	
	if (government_type_id == -1) {
		return;
	}

	if (civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(faction_name);
		if (faction != -1) {
			GrandStrategyGame.Factions[civilization][faction]->GovernmentType = government_type_id;
		}
	}
}

void SetFactionDiplomacyState(std::string civilization_name, std::string faction_name, std::string second_civilization_name, std::string second_faction_name, std::string diplomacy_state_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int second_civilization = PlayerRaces.GetRaceIndexByName(second_civilization_name.c_str());
	
	int diplomacy_state_id = GetDiplomacyStateIdByName(diplomacy_state_name);
	
	if (diplomacy_state_id == -1) {
		return;
	}

	if (civilization != -1 && second_civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(faction_name);
		int second_faction = PlayerRaces.GetFactionIndexByName(second_faction_name);
		if (faction != -1 && second_faction != -1) {
			GrandStrategyGame.Factions[civilization][faction]->SetDiplomacyState(GrandStrategyGame.Factions[second_civilization][second_faction],diplomacy_state_id);
		}
	}
}

std::string GetFactionDiplomacyState(std::string civilization_name, std::string faction_name, std::string second_civilization_name, std::string second_faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int second_civilization = PlayerRaces.GetRaceIndexByName(second_civilization_name.c_str());
	
	if (civilization != -1 && second_civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(faction_name);
		int second_faction = PlayerRaces.GetFactionIndexByName(second_faction_name);
		if (faction != -1 && second_faction != -1) {
			return GetDiplomacyStateNameById(GrandStrategyGame.Factions[civilization][faction]->GetDiplomacyState(GrandStrategyGame.Factions[second_civilization][second_faction]));
		}
	}
	
	return "";
}

void SetFactionDiplomacyStateProposal(std::string civilization_name, std::string faction_name, std::string second_civilization_name, std::string second_faction_name, std::string diplomacy_state_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int second_civilization = PlayerRaces.GetRaceIndexByName(second_civilization_name.c_str());
	
	int diplomacy_state_id = GetDiplomacyStateIdByName(diplomacy_state_name);
	
	if (civilization != -1 && second_civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(faction_name);
		int second_faction = PlayerRaces.GetFactionIndexByName(second_faction_name);
		if (faction != -1 && second_faction != -1) {
			GrandStrategyGame.Factions[civilization][faction]->DiplomacyStateProposals[GrandStrategyGame.Factions[second_civilization][second_faction]] = diplomacy_state_id;
		}
	}
}

std::string GetFactionDiplomacyStateProposal(std::string civilization_name, std::string faction_name, std::string second_civilization_name, std::string second_faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int second_civilization = PlayerRaces.GetRaceIndexByName(second_civilization_name.c_str());
	
	if (civilization != -1 && second_civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(faction_name);
		int second_faction = PlayerRaces.GetFactionIndexByName(second_faction_name);
		if (faction != -1 && second_faction != -1) {
			return GetDiplomacyStateNameById(GrandStrategyGame.Factions[civilization][faction]->GetDiplomacyStateProposal(GrandStrategyGame.Factions[second_civilization][second_faction]));
		}
	}
	
	return "";
}

void SetFactionTier(std::string civilization_name, std::string faction_name, std::string faction_tier_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	
	int faction_tier_id = GetFactionTierIdByName(faction_tier_name);
	
	if (faction_tier_id == -1) {
		return;
	}

	if (civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(faction_name);
		if (faction != -1) {
			GrandStrategyGame.Factions[civilization][faction]->FactionTier = faction_tier_id;
		}
	}
}

std::string GetFactionTier(std::string civilization_name, std::string faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	if (civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(faction_name);
		if (faction != -1) {
			return GetFactionTierNameById(GrandStrategyGame.Factions[civilization][faction]->FactionTier);
		}
	}
	
	return "";
}

void SetFactionCurrentResearch(std::string civilization_name, std::string faction_name, std::string upgrade_ident)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int upgrade_id;
	if (!upgrade_ident.empty()) {
		upgrade_id = UpgradeIdByIdent(upgrade_ident);
	} else {
		upgrade_id = -1;
	}
	if (civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(faction_name);
		if (faction != -1) {
			GrandStrategyGame.Factions[civilization][faction]->CurrentResearch = upgrade_id;
		}
	}
}

std::string GetFactionCurrentResearch(std::string civilization_name, std::string faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	if (civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(faction_name);
		if (faction != -1) {
			if (GrandStrategyGame.Factions[civilization][faction]->CurrentResearch != -1) {
				return AllUpgrades[GrandStrategyGame.Factions[civilization][faction]->CurrentResearch]->Ident;
			}
		}
	}
	
	return "";
}

std::string GetFactionFullName(std::string civilization_name, std::string faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	if (civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(faction_name);
		if (faction != -1) {
			return GrandStrategyGame.Factions[civilization][faction]->GetFullName();
		}
	}
	
	return "";
}

bool IsGrandStrategyUnit(const CUnitType &type)
{
	if (!type.BoolFlag[BUILDING_INDEX].value && type.DefaultStat.Variables[DEMAND_INDEX].Value > 0 && type.Class != -1 && UnitTypeClasses[type.Class] != "caravan") {
		return true;
	}
	return false;
}

bool IsMilitaryUnit(const CUnitType &type)
{
	if (IsGrandStrategyUnit(type) && type.Class != -1 && UnitTypeClasses[type.Class] != "worker") {
		return true;
	}
	return false;
}

bool IsOffensiveMilitaryUnit(const CUnitType &type)
{
	if (IsMilitaryUnit(type) && type.Class != -1 && UnitTypeClasses[type.Class] != "militia") {
		return true;
	}
	return false;
}

void SetFactionCommodityTrade(std::string civilization_name, std::string faction_name, std::string resource_name, int quantity)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization][faction]->Trade[resource] = quantity;
}

void ChangeFactionCommodityTrade(std::string civilization_name, std::string faction_name, std::string resource_name, int quantity)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization][faction]->Trade[resource] += quantity;
}

int GetFactionCommodityTrade(std::string civilization_name, std::string faction_name, std::string resource_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return 0;
	}
	
	return GrandStrategyGame.Factions[civilization][faction]->Trade[resource];
}

void SetFactionMinister(std::string civilization_name, std::string faction_name, std::string title_name, std::string hero_full_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(faction_name);
	}
	int title = GetCharacterTitleIdByName(title_name);
	
	if (faction == -1 || title == -1) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization][faction]->SetMinister(title, TransliterateText(hero_full_name));
}

std::string GetFactionMinister(std::string civilization_name, std::string faction_name, std::string title_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(faction_name);
	}
	int title = GetCharacterTitleIdByName(title_name);
	
	if (faction == -1 || title == -1) {
		return "";
	}
	
	if (GrandStrategyGame.Factions[civilization][faction]->Ministers[title] != NULL) {
		return GrandStrategyGame.Factions[civilization][faction]->Ministers[title]->GetFullName();
	} else {
		return "";
	}
}

int GetFactionUnitCost(std::string civilization_name, std::string faction_name, std::string unit_type_ident, std::string resource_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(faction_name);
	}
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (unit_type == -1 || resource == -1) {
		return 0;
	}
	
	int cost = UnitTypes[unit_type]->DefaultStat.Costs[resource];
	if (faction != -1) {
		if (IsMilitaryUnit(*UnitTypes[unit_type])) {
			cost *= 100 + GrandStrategyGame.Factions[civilization][faction]->GetTroopCostModifier();
			cost /= 100;
		}
	}
	return cost;
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

void SetGrandStrategyHeroUnitType(std::string hero_full_name, std::string unit_type_ident)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
		if (unit_type_id != -1) {
			hero->SetType(unit_type_id);
		}
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
}

std::string GetGrandStrategyHeroUnitType(std::string hero_full_name)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		if (hero->Type != NULL) {
			return hero->Type->Ident;
		}
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	return "";
}

std::string GetGrandStrategyHeroIcon(std::string hero_full_name)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		if (hero->Type != NULL) {
			return hero->GetIcon().Name;
		}
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	return "";
}

std::string GetGrandStrategyHeroBestDisplayTitle(std::string hero_full_name)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		if (hero->Type != NULL) {
			return hero->GetBestDisplayTitle();
		}
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	return "";
}

std::string GetGrandStrategyHeroTooltip(std::string hero_full_name)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		if (hero->Type != NULL) {
			std::string hero_tooltip = hero->GetBestDisplayTitle() + " " + hero->GetFullName();
			
			hero_tooltip += "\nTrait: " + hero->Trait->Name;
			
			for (size_t i = 0; i < hero->Titles.size(); ++i) {
//				hero_tooltip += "\n" + hero->Titles[i].second->GetCharacterTitle(hero->Titles[i].first, hero->Gender) + " of ";
				if (PlayerRaces.Factions[hero->Titles[i].second->Faction]->Type == FactionTypeTribe) {
					hero_tooltip += "the ";
				}
				hero_tooltip += PlayerRaces.Factions[hero->Titles[i].second->Faction]->Name;
			}

			for (size_t i = 0; i < hero->Titles.size(); ++i) {
				if (hero->GetFaction() == hero->Titles[i].second && !hero->GetMinisterEffectsString(hero->Titles[i].first).empty()) {
					hero_tooltip += "\n" + FindAndReplaceString(hero->GetMinisterEffectsString(hero->Titles[i].first), ", ", "\n");
				}
			}
			
			for (size_t i = 0; i < hero->ProvinceTitles.size(); ++i) {
				if (hero->GetFaction() == hero->ProvinceTitles[i].second->Owner && !hero->GetMinisterEffectsString(hero->ProvinceTitles[i].first).empty()) {
					hero_tooltip += "\n" + FindAndReplaceString(hero->GetMinisterEffectsString(hero->ProvinceTitles[i].first), ", ", "\n");
				}
			}
			
			return hero_tooltip;
		}
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	return "";
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

bool GrandStrategyHeroIsVisible(std::string hero_full_name)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		return hero->IsVisible();
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	return false;
}

bool GrandStrategyHeroIsActive(std::string hero_full_name)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		return hero->IsActive();
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	return false;
}

bool GrandStrategyHeroIsCustom(std::string hero_full_name)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		return hero->Custom;
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	return false;
}

void GrandStrategyWorkCreated(std::string work_ident)
{
	CUpgrade *work = CUpgrade::Get(work_ident);
	if (work) {
		GrandStrategyGame.UnpublishedWorks.erase(std::remove(GrandStrategyGame.UnpublishedWorks.begin(), GrandStrategyGame.UnpublishedWorks.end(), work), GrandStrategyGame.UnpublishedWorks.end()); // remove work from the vector, so that it doesn't get created again
	} else {
		fprintf(stderr, "Work \"%s\" doesn't exist.\n", work_ident.c_str());
	}
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

void SetGrandStrategySelectedUnits(std::string unit_type_ident, int quantity)
{
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (unit_type != -1) {
		if (quantity > 0) {
			GrandStrategyGame.SelectedUnits[unit_type] = quantity;
		} else {
			GrandStrategyGame.SelectedUnits.erase(unit_type);
		}
	}
}

int GetGrandStrategySelectedUnits(std::string unit_type_ident)
{
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (unit_type == -1 || GrandStrategyGame.SelectedUnits.find(unit_type) == GrandStrategyGame.SelectedUnits.end()) {
		return 0;
	}
	
	return GrandStrategyGame.SelectedUnits[unit_type];
}
void SetCommodityPrice(std::string resource_name, int price)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return;
	}
	
	GrandStrategyGame.CommodityPrices[resource] = price;
}

int GetCommodityPrice(std::string resource_name)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return 0;
	}
	
	return GrandStrategyGame.CommodityPrices[resource];
}

void SetResourceBasePrice(std::string resource_name, int price)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return;
	}
	
	DefaultResourcePrices[resource] = price;
}

void CleanGrandStrategyEvents()
{
	for (size_t i = 0; i < GrandStrategyEvents.size(); ++i) {
		delete GrandStrategyEvents[i];
	}
	GrandStrategyEvents.clear();
	GrandStrategyEventStringToPointer.clear();
}

CGrandStrategyEvent *GetGrandStrategyEvent(std::string event_name)
{
	if (GrandStrategyEventStringToPointer.find(event_name) != GrandStrategyEventStringToPointer.end()) {
		return GrandStrategyEventStringToPointer[event_name];
	}
	
	return NULL;
}

std::string GetDiplomacyStateNameById(int diplomacy_state)
{
	if (diplomacy_state == DiplomacyStatePeace) {
		return "peace";
	} else if (diplomacy_state == DiplomacyStateWar) {
		return "war";
	} else if (diplomacy_state == DiplomacyStateAlliance) {
		return "alliance";
	} else if (diplomacy_state == DiplomacyStateVassal) {
		return "vassal";
	} else if (diplomacy_state == DiplomacyStateOverlord) {
		return "overlord";
	} else if (diplomacy_state == -1) {
		return "";
	}

	return "";
}

int GetDiplomacyStateIdByName(std::string diplomacy_state)
{
	if (diplomacy_state == "peace") {
		return DiplomacyStatePeace;
	} else if (diplomacy_state == "war") {
		return DiplomacyStateWar;
	} else if (diplomacy_state == "alliance") {
		return DiplomacyStateAlliance;
	} else if (diplomacy_state == "vassal") {
		return DiplomacyStateVassal;
	} else if (diplomacy_state == "overlord") {
		return DiplomacyStateOverlord;
	}

	return -1;
}

std::string GetFactionTierNameById(int faction_tier)
{
	if (faction_tier == FactionTierNoFactionTier) {
		return "no-faction-tier";
	} else if (faction_tier == FactionTierBarony) {
		return "barony";
	} else if (faction_tier == FactionTierCounty) {
		return "county";
	} else if (faction_tier == FactionTierDuchy) {
		return "duchy";
	} else if (faction_tier == FactionTierGrandDuchy) {
		return "grand-duchy";
	} else if (faction_tier == FactionTierKingdom) {
		return "kingdom";
	} else if (faction_tier == FactionTierEmpire) {
		return "empire";
	}

	return "";
}

int GetFactionTierIdByName(std::string faction_tier)
{
	if (faction_tier == "no-faction-tier") {
		return FactionTierNoFactionTier;
	} else if (faction_tier == "barony") {
		return FactionTierBarony;
	} else if (faction_tier == "county") {
		return FactionTierCounty;
	} else if (faction_tier == "duchy") {
		return FactionTierDuchy;
	} else if (faction_tier == "grand-duchy") {
		return FactionTierGrandDuchy;
	} else if (faction_tier == "kingdom") {
		return FactionTierKingdom;
	} else if (faction_tier == "empire") {
		return FactionTierEmpire;
	}

	return -1;
}

//@}
