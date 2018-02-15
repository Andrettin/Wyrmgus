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
/**@name ai.cpp - The computer player AI main file. */
//
//      (c) Copyright 2000-2018 by Lutz Sammer, Ludovic Pollet,
//      Jimmy Salmon and Andrettin
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

//----------------------------------------------------------------------------
// Documentation
//----------------------------------------------------------------------------

/**
** @page AiModule Module - AI
**
** @section aibasics What is it?
**
** Stratagus uses a very simple scripted AI. There are no optimizations
** yet. The complete AI was written on one weekend.
** Until no AI specialist joins, I keep this AI.
**
** @subsection aiscripted What is scripted AI?
**
** The AI script tells the engine build 4 workers, than build 3 footman,
** than attack the player, than sleep 100 frames.
**
** @section API The AI API
**
** @subsection aimanage Management calls
**
** Manage the inititialse and cleanup of the AI players.
**
** ::InitAiModule()
**
** Initialise all global varaibles and structures.
** Called before AiInit, or before game loading.
**
** ::AiInit(::Player)
**
** Called for each player, to setup the AI structures
** Player::Aiin the player structure. It can use Player::AiName to
** select different AI's.
**
** ::CleanAi()
**
** Called to release all the memory for all AI structures.
** Must handle self which players contains AI structures.
**
** ::SaveAi(::FILE *)
**
** Save the AI structures of all players to file.
** Must handle self which players contains AI structures.
**
**
** @subsection aipcall Periodic calls
**
** This functions are called regular for all AI players.
**
** ::AiEachCycle(::Player)
**
** Called each game cycle, to handle quick checks, which needs
** less CPU.
**
** ::AiEachSecond(::Player)
**
** Called each second, to handle more CPU intensive things.
**
**
** @subsection aiecall Event call-backs
**
** This functions are called, when some special events happens.
**
** ::AiHelpMe()
**
** Called if a unit owned by the AI is attacked.
**
** ::AiUnitKilled()
**
** Called if a unit owned by the AI is killed.
**
** ::AiNeedMoreSupply()
**
** Called if an trained unit is ready, but not enough food is
** available for it.
**
** ::AiWorkComplete()
**
** Called if a unit has completed its work.
**
** ::AiCanNotBuild()
**
** Called if the AI unit can't build the requested unit-type.
**
** ::AiCanNotReach()
**
** Called if the AI unit can't reach the building place.
**
** ::AiTrainingComplete()
**
** Called if AI unit has completed training a new unit.
**
** ::AiUpgradeToComplete()
**
** Called if AI unit has completed upgrade to new unit-type.
**
** ::AiResearchComplete()
**
** Called if AI unit has completed research of an upgrade or spell.
*/

/*----------------------------------------------------------------------------
-- Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "ai.h"
#include "ai_local.h"

#include "actions.h"
#include "action/action_attack.h"
#include "commands.h"
//Wyrmgus start
#include "depend.h"
#include "editor.h"
//Wyrmgus end
#include "grand_strategy.h"
#include "iolib.h"
//Wyrmgus start
#include "luacallback.h"
//Wyrmgus end
#include "map.h"
//Wyrmgus start
#include "network.h"
//Wyrmgus end
#include "pathfinder.h"
#include "player.h"
//Wyrmgus start
#include "quest.h"
//Wyrmgus end
#include "script.h"
#include "unit.h"
//Wyrmgus start
#include "unit_find.h"
//Wyrmgus end
#include "unit_manager.h"
#include "unittype.h"
#include "upgrade.h"

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

int AiSleepCycles;              /// Ai sleeps # cycles

std::vector<CAiType *> AiTypes; /// List of all AI types.
AiHelper AiHelpers;             /// AI helper variables

PlayerAi *AiPlayer;             /// Current AI player

/*----------------------------------------------------------------------------
-- Lowlevel functions
----------------------------------------------------------------------------*/

/**
**  Execute the AI Script.
*/
static void AiExecuteScript()
{
	if (AiPlayer->Script.empty()) {
		return;
	}
	lua_getglobal(Lua, "_ai_scripts_");
	lua_pushstring(Lua, AiPlayer->Script.c_str());
	lua_rawget(Lua, -2);
	LuaCall(0, 1);
	lua_pop(Lua, 1);
}

/**
**  Check if everything is fine, send new requests to resource manager.
*/
static void AiCheckUnits()
{
	//  Count the already made build requests.
	int counter[UnitTypeMax];
	AiGetBuildRequestsCount(*AiPlayer, counter);

	//  Look if some unit-types are missing.
	int n = AiPlayer->UnitTypeRequests.size();
	for (int i = 0; i < n; ++i) {
		const unsigned int t = AiPlayer->UnitTypeRequests[i].Type->Slot;
		const int x = AiPlayer->UnitTypeRequests[i].Count;
		const int unit_class = AiPlayer->UnitTypeRequests[i].Type->Class;

		// Add equivalent units
		int e = AiPlayer->Player->GetUnitTypeAiActiveCount(AiPlayer->UnitTypeRequests[i].Type);
		if (t < AiHelpers.Equiv.size()) {
			for (unsigned int j = 0; j < AiHelpers.Equiv[t].size(); ++j) {
				e += AiPlayer->Player->GetUnitTypeAiActiveCount(AiHelpers.Equiv[t][j]);
			}
		}
		if (unit_class != -1) {
			for (size_t j = 0; j < ClassUnitTypes[unit_class].size(); ++j) {
				const CUnitType *class_unit_type = ClassUnitTypes[unit_class][j];
				if (class_unit_type != AiPlayer->UnitTypeRequests[i].Type) {
					e += AiPlayer->Player->GetUnitTypeAiActiveCount(class_unit_type);
				}
			}
		}
		const int requested = x - e - counter[t];
		if (requested > 0) {  // Request it.
			//Wyrmgus start
//			AiAddUnitTypeRequest(*AiPlayer->UnitTypeRequests[i].Type, requested);
			AiAddUnitTypeRequest(*AiPlayer->UnitTypeRequests[i].Type, requested, AiPlayer->UnitTypeRequests[i].Landmass);
			//Wyrmgus end
			counter[t] += requested;
		}
		counter[t] -= x;
	}

	AiPlayer->Force.CheckUnits(counter);

	//  Look if some upgrade-to are missing.
	n = AiPlayer->UpgradeToRequests.size();
	for (int i = 0; i < n; ++i) {
		const unsigned int t = AiPlayer->UpgradeToRequests[i]->Slot;
		const int x = 1;

		//  Add equivalent units
		int e = AiPlayer->Player->GetUnitTypeAiActiveCount(AiPlayer->UpgradeToRequests[i]);
		if (t < AiHelpers.Equiv.size()) {
			for (unsigned int j = 0; j < AiHelpers.Equiv[t].size(); ++j) {
				e += AiPlayer->Player->GetUnitTypeAiActiveCount(AiHelpers.Equiv[t][j]);
			}
		}

		const int requested = x - e - counter[t];
		if (requested > 0) {  // Request it.
			AiAddUpgradeToRequest(*AiPlayer->UpgradeToRequests[i]);
			counter[t] += requested;
		}
		counter[t] -= x;
	}

	//  Look if some researches are missing.
	n = (int)AiPlayer->ResearchRequests.size();
	for (int i = 0; i < n; ++i) {
		if (UpgradeIdAllowed(*AiPlayer->Player, AiPlayer->ResearchRequests[i]->ID) == 'A') {
			AiAddResearchRequest(AiPlayer->ResearchRequests[i]);
		}
	}
	
	//Wyrmgus start
	//check if can hire any heroes
	if (AiPlayer->Player->Heroes.size() < PlayerHeroMax && AiPlayer->Player->HeroCooldownTimer == 0 && !IsNetworkGame() && CurrentQuest == NULL) {
		for (int i = 0; i < AiPlayer->Player->GetUnitCount(); ++i) {
			CUnit *hero_recruiter = &AiPlayer->Player->GetUnit(i);
			if (!hero_recruiter || !hero_recruiter->IsAliveOnMap() || !hero_recruiter->Type->BoolFlag[RECRUITHEROES_INDEX].value || hero_recruiter->CurrentAction() == UnitActionBuilt) {
				continue;
			}
			
			for (size_t j = 0; j < hero_recruiter->SoldUnits.size(); ++j) {
				int buy_costs[MaxCosts];
				memset(buy_costs, 0, sizeof(buy_costs));
				buy_costs[CopperCost] = hero_recruiter->SoldUnits[j]->GetPrice();
				if (!AiPlayer->Player->CheckCosts(buy_costs) && AiPlayer->Player->CheckLimits(*hero_recruiter->SoldUnits[j]->Type) >= 1) {
					CommandBuy(*hero_recruiter, hero_recruiter->SoldUnits[j], AiPlayer->Player->Index);
					break;
				}
			}
		}
	}
	
	if (AiPlayer->Player->NumTownHalls > 0 && !AiPlayer->Player->HasNeutralFactionType()) {
		//check if can hire any mercenaries
		for (int i = 0; i < PlayerMax; ++i) {
			if (i == AiPlayer->Player->Index) {
				continue;
			}
			if (Players[i].Type != PlayerComputer || !AiPlayer->Player->HasBuildingAccess(Players[i])) {
				continue;
			}
			for (int j = 0; j < Players[i].GetUnitCount(); ++j) {
				CUnit *mercenary_building = &Players[i].GetUnit(j);
				if (!mercenary_building || !mercenary_building->IsAliveOnMap() || !mercenary_building->Type->BoolFlag[BUILDING_INDEX].value || !mercenary_building->IsVisible(*AiPlayer->Player)) {
					continue;
				}

				if (AiPlayer->Player->Heroes.size() < PlayerHeroMax && AiPlayer->Player->HeroCooldownTimer == 0 && mercenary_building->Type->BoolFlag[RECRUITHEROES_INDEX].value && !IsNetworkGame() && CurrentQuest == NULL) { //check if can hire any heroes at the mercenary camp
					for (size_t k = 0; k < mercenary_building->SoldUnits.size(); ++k) {
						int buy_costs[MaxCosts];
						memset(buy_costs, 0, sizeof(buy_costs));
						buy_costs[CopperCost] = mercenary_building->SoldUnits[k]->GetPrice();
						if (!AiPlayer->Player->CheckCosts(buy_costs) && AiPlayer->Player->CheckLimits(*mercenary_building->SoldUnits[k]->Type) >= 1) {
							CommandBuy(*mercenary_building, mercenary_building->SoldUnits[k], AiPlayer->Player->Index);
							break;
						}
					}
				}

				bool mercenary_recruited = false;
				for (std::map<int, int>::iterator iterator = mercenary_building->UnitStock.begin(); iterator != mercenary_building->UnitStock.end(); ++iterator) {
					CUnitType *mercenary_type = UnitTypes[iterator->first];
					if (
						iterator->second
						&& !mercenary_type->BoolFlag[ITEM_INDEX].value
						&& CheckDependByType(Players[i], *mercenary_type)
						&& AiPlayer->Player->CheckLimits(*mercenary_type) >= 1
						&& !AiPlayer->Player->CheckUnitType(*mercenary_type, true)
					) {
						//see if there are any unit type requests for units of the same class as the mercenary
						for (size_t k = 0; k < AiPlayer->UnitTypeBuilt.size(); ++k) {
							AiBuildQueue &queue = AiPlayer->UnitTypeBuilt[k];
							if (
								mercenary_type->Class == queue.Type->Class
								&& queue.Want > queue.Made
								&& (!queue.Landmass || queue.Landmass == Map.GetTileLandmass(mercenary_building->tilePos, mercenary_building->MapLayer))
								&& (!queue.Settlement || queue.Settlement == mercenary_building->Settlement)
							) {
								queue.Made++;
								CommandTrainUnit(*mercenary_building, *mercenary_type, AiPlayer->Player->Index, FlushCommands);
								mercenary_recruited = true;
								break;
							}
						}
					}
					if (mercenary_recruited) {
						break; // only hire one unit per mercenary camp per second
					}
				}
			}
		}
	}
	//Wyrmgus end
	
	//Wyrmgus start
	//check if any factions can be founded, and if so, pick one randomly
	if (AiPlayer->Player->Faction != -1 && AiPlayer->Player->NumTownHalls > 0) {
		std::vector<CFaction *> potential_factions;
		for (size_t i = 0; i < PlayerRaces.Factions[AiPlayer->Player->Faction]->DevelopsTo.size(); ++i) {
			CFaction *possible_faction = PlayerRaces.Factions[AiPlayer->Player->Faction]->DevelopsTo[i];
			
			if (!AiPlayer->Player->CanFoundFaction(possible_faction)) {
				continue;
			}
				
			potential_factions.push_back(possible_faction);
		}
		
		if (potential_factions.size() > 0) {
			AiPlayer->Player->SetFaction(potential_factions[SyncRand(potential_factions.size())]);
		}
		
		if (!AiPlayer->Player->Dynasty) { //if the AI player has no dynasty, pick one if available
			std::vector<CDynasty *> potential_dynasties;
			for (size_t i = 0; i < PlayerRaces.Factions[AiPlayer->Player->Faction]->Dynasties.size(); ++i) {
				CDynasty *possible_dynasty = PlayerRaces.Factions[AiPlayer->Player->Faction]->Dynasties[i];
				
				if (!AiPlayer->Player->CanChooseDynasty(possible_dynasty)) {
					continue;
				}
					
				potential_dynasties.push_back(possible_dynasty);
			}
			
			if (potential_dynasties.size() > 0) {
				AiPlayer->Player->SetDynasty(potential_dynasties[SyncRand(potential_dynasties.size())]);
			}
		}
	}
	//Wyrmgus end
}

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

/**
**  Save state of player AI.
**
**  @param file   Output file.
**  @param plynr  Player number.
**  @param ai     Player AI.
*/
static void SaveAiPlayer(CFile &file, int plynr, const PlayerAi &ai)
{
	file.printf("DefineAiPlayer(%d,\n", plynr);
	file.printf("  \"ai-type\", \"%s\",\n", ai.AiType->Name.c_str());

	file.printf("  \"script\", \"%s\",\n", ai.Script.c_str());
	file.printf("  \"script-debug\", %s,\n", ai.ScriptDebug ? "true" : "false");
	file.printf("  \"sleep-cycles\", %lu,\n", ai.SleepCycles);

	//Wyrmgus start
	if (ai.Scouting) {
		file.printf("  \"scouting\",\n");
	}
	//Wyrmgus end
	
	//  All forces
	for (size_t i = 0; i < ai.Force.Size(); ++i) {
		file.printf("  \"force\", {%d, %s%s%s", (int) i,
					ai.Force[i].Completed ? "\"complete\"," : "\"recruit\",",
					ai.Force[i].Attacking ? " \"attack\"," : "",
					ai.Force[i].Defending ? " \"defend\"," : "");

		file.printf(" \"role\", ");
		switch (ai.Force[i].Role) {
			case AiForceRoleAttack:
				file.printf("\"attack\",");
				break;
			case AiForceRoleDefend:
				file.printf("\"defend\",");
				break;
			default:
				file.printf("\"unknown-%d\",", ai.Force[i].Role);
				break;
		}

		file.printf("\n    \"types\", { ");
		const size_t unitTypesCounst = ai.Force[i].UnitTypes.size();
		for (size_t j = 0; j != unitTypesCounst; ++j) {
			const AiUnitType &aut = ai.Force[i].UnitTypes[j];
			file.printf("%d, \"%s\", ", aut.Want, aut.Type->Ident.c_str());
		}
		file.printf("},\n    \"units\", {");
		const size_t unitsCount = ai.Force[i].Units.size();
		for (size_t j = 0; j != unitsCount; ++j) {
			const CUnit &aiunit = *ai.Force[i].Units[j];
			file.printf(" %d, \"%s\",", UnitNumber(aiunit),
						aiunit.Type->Ident.c_str());
		}
		file.printf("},\n    \"state\", %d, \"goalx\", %d, \"goaly\", %d,",
					ai.Force[i].State, ai.Force[i].GoalPos.x, ai.Force[i].GoalPos.y);
		file.printf("},\n");
	}

	file.printf("  \"reserve\", {");
	for (int i = 0; i < MaxCosts; ++i) {
		file.printf("\"%s\", %d, ", DefaultResourceNames[i].c_str(), ai.Reserve[i]);
	}
	file.printf("},\n");

	file.printf("  \"used\", {");
	for (int i = 0; i < MaxCosts; ++i) {
		file.printf("\"%s\", %d, ", DefaultResourceNames[i].c_str(), ai.Used[i]);
	}
	file.printf("},\n");

	file.printf("  \"needed\", {");
	for (int i = 0; i < MaxCosts; ++i) {
		file.printf("\"%s\", %d, ", DefaultResourceNames[i].c_str(), ai.Needed[i]);
	}
	file.printf("},\n");

	file.printf("  \"collect\", {");
	for (int i = 0; i < MaxCosts; ++i) {
		file.printf("\"%s\", %d, ", DefaultResourceNames[i].c_str(), ai.Collect[i]);
	}
	file.printf("},\n");

	file.printf("  \"need-mask\", {");
	for (int i = 0; i < MaxCosts; ++i) {
		if (ai.NeededMask & (1 << i)) {
			file.printf("\"%s\", ", DefaultResourceNames[i].c_str());
		}
	}
	file.printf("},\n");
	if (ai.NeedSupply) {
		file.printf("  \"need-supply\",\n");
	}

	//  Requests
	if (!ai.FirstExplorationRequest.empty()) {
		file.printf("  \"exploration\", {");
		const size_t FirstExplorationRequestCount = ai.FirstExplorationRequest.size();
		for (size_t i = 0; i != FirstExplorationRequestCount; ++i) {
			const AiExplorationRequest &ptr = ai.FirstExplorationRequest[i];
			file.printf("{%d, %d, %d}, ", ptr.pos.x, ptr.pos.y, ptr.Mask);
		}
		file.printf("},\n");
	}
	file.printf("  \"last-exploration-cycle\", %lu,\n", ai.LastExplorationGameCycle);
	file.printf("  \"last-can-not-move-cycle\", %lu,\n", ai.LastCanNotMoveGameCycle);
	file.printf("  \"unit-type\", {");
	const size_t unitTypeRequestsCount = ai.UnitTypeRequests.size();
	for (size_t i = 0; i != unitTypeRequestsCount; ++i) {
		file.printf("\"%s\", ", ai.UnitTypeRequests[i].Type->Ident.c_str());
		file.printf("%d, ", ai.UnitTypeRequests[i].Count);
		//Wyrmgus start
		file.printf("%d, ", ai.UnitTypeRequests[i].Landmass);
		//Wyrmgus end
	}
	file.printf("},\n");

	file.printf("  \"upgrade\", {");
	const size_t upgradeToRequestsCount = ai.UpgradeToRequests.size();
	for (size_t i = 0; i != upgradeToRequestsCount; ++i) {
		file.printf("\"%s\", ", ai.UpgradeToRequests[i]->Ident.c_str());
	}
	file.printf("},\n");

	file.printf("  \"research\", {");
	const size_t researchRequestsCount = ai.ResearchRequests.size();
	for (size_t i = 0; i != researchRequestsCount; ++i) {
		file.printf("\"%s\", ", ai.ResearchRequests[i]->Ident.c_str());
	}
	file.printf("},\n");

	//
	//  Building queue
	//
	file.printf("  \"building\", {");
	const size_t UnitTypeBuiltCount = ai.UnitTypeBuilt.size();
	for (size_t i = 0; i != UnitTypeBuiltCount; ++i) {
		const AiBuildQueue &queue = ai.UnitTypeBuilt[i];
		/* rb - for backward compatibility of save format we have to put it first */
		if (queue.Pos.x != -1) {
			file.printf("\"onpos\", %d, %d, ", queue.Pos.x, queue.Pos.y);
		}
		//Wyrmgus start
		if (queue.MapLayer != -1) {
			file.printf("\"map-layer\", %d, ", queue.MapLayer);
		}
		
		if (queue.Landmass != 0) {
			file.printf("\"landmass\", %d, ", queue.Landmass);
		}
		
		if (queue.Settlement != NULL) {
			file.printf("\"settlement\", \"%s\", ", queue.Settlement->Ident.c_str());
		}
		//Wyrmgus end
		/* */

		file.printf("\"%s\", %d, %d", queue.Type->Ident.c_str(), queue.Made, queue.Want);
		if (i < UnitTypeBuiltCount - 1) {
			file.printf(",\n");
		}
	}
	file.printf("},\n");

	//Wyrmgus start
	if (!ai.Scouts.empty()) {
		file.printf("  \"scouts\", {");
		for (size_t i = 0; i != ai.Scouts.size(); ++i) {
			const CUnit &aiunit = *ai.Scouts[i];
			file.printf(" %d, \"%s\",", UnitNumber(aiunit), aiunit.Type->Ident.c_str());
		}
		file.printf("},\n");
	}
	
	if (!ai.Transporters.empty()) {
		file.printf("  \"transporters\", {");
		for (std::map<int, std::vector<CUnit *>>::const_iterator iterator = ai.Transporters.begin(); iterator != ai.Transporters.end(); ++iterator) {
			for (size_t i = 0; i != iterator->second.size(); ++i) {
				const CUnit &aiunit = *iterator->second[i];
				file.printf(" %d, %d,", iterator->first, UnitNumber(aiunit));
			}
		}
		file.printf("},\n");
	}
	//Wyrmgus end
	
	//Wyrmgus start
	file.printf("  \"pathway-construction-building\", %u,\n", ai.LastPathwayConstructionBuilding);
	//Wyrmgus end

	file.printf("  \"repair-building\", %u\n", ai.LastRepairBuilding);
	
	file.printf(")\n\n");
}

/**
**  Save state of AI to file.
**
**  @param file  Output file.
*/
void SaveAi(CFile &file)
{
	file.printf("\n--- -----------------------------------------\n");

	for (int i = 0; i < PlayerMax; ++i) {
		if (Players[i].Ai) {
			SaveAiPlayer(file, i, *Players[i].Ai);
		}
	}

	DebugPrint("FIXME: Saving lua function definition isn't supported\n");
}

/**
**  Setup all at start.
**
**  @param player  The player structure pointer.
*/
void AiInit(CPlayer &player)
{
	PlayerAi *pai = new PlayerAi;

	if (!pai) {
		fprintf(stderr, "Out of memory.\n");
		exit(0);
	}

	pai->Player = &player;

	DebugPrint("%d - looking for class %s\n" _C_ player.Index _C_ player.AiName.c_str());
	//MAPTODO print the player name (player->Name) instead of the pointer

	//  Search correct AI type.
	if (AiTypes.empty()) {
		DebugPrint("AI: Got no scripts at all! You need at least one dummy fallback script.\n");
		DebugPrint("AI: Look at the DefineAi() documentation.\n");
		Exit(0);
	}
	size_t i;
	CAiType *ait = NULL;

	for (i = 0; i < AiTypes.size(); ++i) {
		ait = AiTypes[i];
		if (!ait->Race.empty() && ait->Race != PlayerRaces.Name[player.Race]) {
			continue;
		}
		if (!player.AiName.empty() && ait->Name != player.AiName) {
			continue;
		}
		break;
	}
	if (i == AiTypes.size()) {
		DebugPrint("AI: Found no matching ai scripts at all!\n");
		// FIXME: surely we can do something better than exit
		exit(0);
	}
	if (player.AiName.empty()) {
		DebugPrint("AI: not found!!!!!!!!!!\n");
		DebugPrint("AI: Using fallback:\n");
	}
	DebugPrint("AI: %s:%s with %s:%s\n" _C_ PlayerRaces.Name[player.Race].c_str() _C_
			   !ait->Race.empty() ? ait->Race.c_str() : "All" _C_ player.AiName.c_str() _C_ ait->Class.c_str());

	pai->AiType = ait;
	pai->Script = ait->Script;

	//Wyrmgus start
//	pai->Collect[GoldCost] = 50;
//	pai->Collect[WoodCost] = 50;
	pai->Collect[CopperCost] = 45;
	pai->Collect[WoodCost] = 45;
	//Wyrmgus end
	pai->Collect[OilCost] = 0;
	//Wyrmgus start
	pai->Collect[StoneCost] = 10;
	//Wyrmgus end

	player.Ai = pai;
}

/**
**  Initialize global structures of the AI
*/
void InitAiModule()
{
	AiResetUnitTypeEquiv();
}


/**
**  Cleanup the AI in order to enable to restart a game.
*/
void CleanAi()
{
	for (int p = 0; p < PlayerMax; ++p) {
		delete Players[p].Ai;
		Players[p].Ai = NULL;
	}
}


/**
**  Free all AI resources.
*/
void FreeAi()
{
	CleanAi();

	//  Free AiTypes.
	for (unsigned int i = 0; i < AiTypes.size(); ++i) {
		CAiType *aitype = AiTypes[i];

		delete aitype;
	}
	AiTypes.clear();

	//  Free AiHelpers.
	AiHelpers.Train.clear();
	AiHelpers.Build.clear();
	AiHelpers.Upgrade.clear();
	AiHelpers.Research.clear();
	AiHelpers.Repair.clear();
	AiHelpers.UnitLimit.clear();
	AiHelpers.Equiv.clear();
	AiHelpers.Mines.clear();
	AiHelpers.Depots.clear();
	//Wyrmgus start
	AiHelpers.SellMarkets.clear();
	AiHelpers.BuyMarkets.clear();
	AiHelpers.ProducedResources.clear();
	AiHelpers.ResearchedUpgrades.clear();
	AiHelpers.UpgradesTo.clear();
	AiHelpers.ExperienceUpgrades.clear();
	AiHelpers.LearnableAbilities.clear();
	AiHelpers.NavalTransporters.clear();
	//Wyrmgus end

	AiResetUnitTypeEquiv();
}

/*----------------------------------------------------------------------------
-- Support functions
----------------------------------------------------------------------------*/

/**
**  Remove unit-type from build list.
**
**  @param pai   Computer AI player.
**  @param type  Unit-type which is now available.
**  @return      True, if unit-type was found in list.
*/
//Wyrmgus start
//static int AiRemoveFromBuilt2(PlayerAi *pai, const CUnitType &type)
static int AiRemoveFromBuilt2(PlayerAi *pai, const CUnitType &type, int landmass = 0, const CSettlement *settlement = NULL)
//Wyrmgus end
{
	std::vector<AiBuildQueue>::iterator i;

	for (i = pai->UnitTypeBuilt.begin(); i != pai->UnitTypeBuilt.end(); ++i) {
		Assert((*i).Want);
		//Wyrmgus start
//		if (&type == (*i).Type && (*i).Made) {
		if (
			&type == (*i).Type
			&& (*i).Made
			&& (!(*i).Landmass || !landmass || (*i).Landmass == landmass)
			&& (!(*i).Settlement || !settlement || (*i).Settlement == settlement)
		) {
		//Wyrmgus end
			--(*i).Made;
			if (!--(*i).Want) {
				pai->UnitTypeBuilt.erase(i);
			}
			return 1;
		}
	}
	return 0;
}

/**
**  Remove unit-type from build list.
**
**  @param pai   Computer AI player.
**  @param type  Unit-type which is now available.
*/
//Wyrmgus start
//static void AiRemoveFromBuilt(PlayerAi *pai, const CUnitType &type)
static void AiRemoveFromBuilt(PlayerAi *pai, const CUnitType &type, int landmass, const CSettlement *settlement)
//Wyrmgus end
{
	//Wyrmgus start
	if (
		(type.GivesResource && type.BoolFlag[CANHARVEST_INDEX].value && type.GivesResource != TradeCost) //don't reduce refineries from the build request, they should be built dynamically via the resource gathering code without being requested
		|| type.TerrainType // tile units are built without requests
		|| type.BoolFlag[TOWNHALL_INDEX].value // town halls are built without requests
	) {
		return;
	}
	//Wyrmgus end
	
	//Wyrmgus start
//	if (AiRemoveFromBuilt2(pai, type)) {
	if (AiRemoveFromBuilt2(pai, type, landmass, settlement)) {
	//Wyrmgus end
		return;
	}

	//  This could happen if an upgrade is ready, look for equivalent units.
	int equivalents[UnitTypeMax + 1];
	const int equivalentsCount = AiFindUnitTypeEquiv(type, equivalents);
	for (int i = 0; i < equivalentsCount; ++i) {
		//Wyrmgus start
//		if (AiRemoveFromBuilt2(pai, *UnitTypes[equivalents[i]])) {
		if (AiRemoveFromBuilt2(pai, *UnitTypes[equivalents[i]], landmass, settlement)) {
		//Wyrmgus end
			return;
		}
	}
	if (pai->Player == ThisPlayer) {
		DebugPrint("My guess is that you built something under ai me. naughty boy!\n");
		return;
	}
	fprintf(stderr, "Can't reduce %s from build list.\n", type.Ident.c_str());
}

/**
**  Reduce made unit-type from build list.
**
**  @param pai   Computer AI player.
**  @param type  Unit-type which is now available.
**  @return      True if the unit-type could be reduced.
*/
//Wyrmgus start
//static bool AiReduceMadeInBuilt2(PlayerAi &pai, const CUnitType &type)
static bool AiReduceMadeInBuilt2(PlayerAi &pai, const CUnitType &type, int landmass = 0, const CSettlement *settlement = NULL)
//Wyrmgus end
{
	std::vector<AiBuildQueue>::iterator i;

	for (i = pai.UnitTypeBuilt.begin(); i != pai.UnitTypeBuilt.end(); ++i) {
		//Wyrmgus start
//		if (&type == (*i).Type && (*i).Made) {
		if (
			&type == (*i).Type
			&& (*i).Made
			&& (!(*i).Landmass || !landmass || (*i).Landmass == landmass)
			&& (!(*i).Settlement || !settlement || (*i).Settlement == settlement)
		) {
		//Wyrmgus end
			(*i).Made--;
			return true;
		}
	}
	return false;
}

/**
**  Reduce made unit-type from build list.
**
**  @param pai   Computer AI player.
**  @param type  Unit-type which is now available.
*/
//Wyrmgus start
//void AiReduceMadeInBuilt(PlayerAi &pai, const CUnitType &type)
void AiReduceMadeInBuilt(PlayerAi &pai, const CUnitType &type, int landmass, const CSettlement *settlement)
//Wyrmgus end
{
	//Wyrmgus start
	if (
		(type.GivesResource && type.BoolFlag[CANHARVEST_INDEX].value && type.GivesResource != TradeCost) //don't reduce refineries from the build request, they should be built dynamically via the resource gathering code without being requested
		|| type.TerrainType // tile units are built without requests
		|| type.BoolFlag[TOWNHALL_INDEX].value // town halls are built without requests
	) {
		return;
	}
	//Wyrmgus end
	
	//Wyrmgus start
//	if (AiReduceMadeInBuilt2(pai, type)) {
	if (AiReduceMadeInBuilt2(pai, type, landmass, settlement)) {
	//Wyrmgus end
		return;
	}
	//  This could happen if an upgrade is ready, look for equivalent units.
	int equivs[UnitTypeMax + 1];
	const unsigned int equivnb = AiFindUnitTypeEquiv(type, equivs);

	for (unsigned int i = 0; i < equivnb; ++i) {
		//Wyrmgus start
//		if (AiReduceMadeInBuilt2(pai, *UnitTypes[equivs[i]])) {
		if (AiReduceMadeInBuilt2(pai, *UnitTypes[equivs[i]], landmass, settlement)) {
		//Wyrmgus end
			return;
		}
	}
	if (pai.Player == ThisPlayer) {
		DebugPrint("My guess is that you built something under ai me. naughty boy!\n");
		return;
	}
	fprintf(stderr, "Can't reduce %s from build list.\n", type.Ident.c_str());
}

/*----------------------------------------------------------------------------
-- Callback Functions
----------------------------------------------------------------------------*/

/**
**  Called if a Unit is Attacked
**
**  @param attacker  Pointer to attacker unit.
**  @param defender  Pointer to unit that is being attacked.
*/
void AiHelpMe(const CUnit *attacker, CUnit &defender)
{
	/* Friendly Fire - typical splash */
	if (!attacker || attacker->Player->Index == defender.Player->Index) {
		//FIXME - try react somehow
		return;
	}

	DebugPrint("%d: %d(%s) attacked at %d,%d\n" _C_
			   defender.Player->Index _C_ UnitNumber(defender) _C_
			   defender.Type->Ident.c_str() _C_ defender.tilePos.x _C_ defender.tilePos.y);

	//  Don't send help to scouts (zeppelin,eye of vision).
	//Wyrmgus start
//	if (!defender.Type->CanAttack && defender.Type->UnitType == UnitTypeFly) {
	if (!defender.CanAttack() && defender.Type->UnitType == UnitTypeFly) {
	//Wyrmgus end
		return;
	}
	// Summoned unit, don't help
	if (defender.Summoned) {
		return;
	}

	PlayerAi &pai = *defender.Player->Ai;
	AiPlayer = &pai;
	
	//Wyrmgus start
	pai.Scouting = false;
	//Wyrmgus end

	//  If unit belongs to an attacking force, check if force members can help.
	//Wyrmgus start
	/*
	if (defender.GroupId) {
		AiForce &aiForce = pai.Force[defender.GroupId - 1];

		//  Unit belongs to an force, check if brothers in arms can help
		for (unsigned int i = 0; i < aiForce.Units.size(); ++i) {
			CUnit &aiunit = *aiForce.Units[i];

			if (&defender == &aiunit) {
				continue;
			}
			
			//Wyrmgus start
			if (aiunit.BoardCount) { //if is transporting a unit, don't go to help, as that may endanger your cargo/passengers
				continue;
			}
			//Wyrmgus end

			// if brother is idle or attack no-agressive target and
			// can attack our attacker then ask for help
			// FIXME ad support for help from Coward type units
			if (aiunit.IsAgressive() && CanTarget(*aiunit.Type, *attacker->Type)
				&& aiunit.CurrentOrder()->GetGoal() != attacker) {
				bool shouldAttack = aiunit.IsIdle() && aiunit.Threshold == 0;

				if (aiunit.CurrentAction() == UnitActionAttack) {
					const COrder_Attack &orderAttack = *static_cast<COrder_Attack *>(aiunit.CurrentOrder());
					const CUnit *oldGoal = orderAttack.GetGoal();

					if (oldGoal == NULL || (ThreatCalculate(defender, *attacker) < ThreatCalculate(defender, *oldGoal)
											//Wyrmgus start
//											&& aiunit.MapDistanceTo(defender) <= aiunit.Stats->Variables[ATTACKRANGE_INDEX].Max)) {
											&& aiunit.MapDistanceTo(defender) <= aiunit.GetModifiedVariable(ATTACKRANGE_INDEX))) {
											//Wyrmgus end
						shouldAttack = true;
					}
				}

				if (shouldAttack) {
					//Wyrmgus start
					const int delay = i; // To avoid lot of CPU consuption, send them with a small time difference.

					aiunit.Wait += delay;
					//Wyrmgus end
					
					//Wyrmgus start
//					CommandAttack(aiunit, attacker->tilePos, const_cast<CUnit *>(attacker), FlushCommands);
//					COrder *savedOrder = COrder::NewActionAttack(aiunit, attacker->tilePos);
					CommandAttack(aiunit, attacker->tilePos, const_cast<CUnit *>(attacker), FlushCommands, attacker->MapLayer);
					//Wyrmgus end

					//Wyrmgus start
//					if (aiunit.CanStoreOrder(savedOrder) == false) {
//						delete savedOrder;
//						savedOrder = NULL;
//					} else {
//						aiunit.SavedOrder = savedOrder;
//					}
					//Wyrmgus end
				}
			}
		}
		if (!aiForce.Defending && aiForce.State > 0) {
			DebugPrint("%d: %d(%s) belong to attacking force, don't defend it\n" _C_
					   defender.Player->Index _C_ UnitNumber(defender) _C_ defender.Type->Ident.c_str());
			// unit belongs to an attacking force,
			// so don't send others force in such case.
			// FIXME: there may be other attacking the same place force who can help
			return;
		}
	}
	*/
	//Wyrmgus end

	//Wyrmgus start
	/*
	// Send defending forces, also send attacking forces if they are home/traning.
	// This is still basic model where we suspect only one base ;(
	const Vec2i &pos = attacker->tilePos;

	for (unsigned int i = 0; i < pai.Force.Size(); ++i) {
		AiForce &aiForce = pai.Force[i];

		if (aiForce.Size() > 0
			&& ((aiForce.Role == AiForceRoleDefend && !aiForce.Attacking)
				//Wyrmgus start
//				|| (aiForce.Role == AiForceRoleAttack && !aiForce.Attacking && !aiForce.State))) {  // none attacking
				|| (aiForce.Role == AiForceRoleAttack && !aiForce.Attacking && !aiForce.State)
				|| (defender.GroupId && i == (defender.GroupId - 1)))) {  // none attacking
				//Wyrmgus end
			//Wyrmgus start
//			aiForce.Defending = true;
			if (!aiForce.Attacking) {
				aiForce.Defending = true;
			}
//			aiForce.Attack(pos);

			//instead of making the force attack the position, make its units that can target the attacker attack it (so that we don't have, for instance, a melee land force try to attack back ships...
			for (unsigned int j = 0; j < aiForce.Units.size(); ++j) {
				CUnit &aiunit = *aiForce.Units[j];

				if (&defender == &aiunit) {
					continue;
				}
				
				if (aiunit.BoardCount) { //if is transporting a unit, don't go to help, as that may endanger your cargo/passengers
					continue;
				}

				// if unit is idle or attacking a non-agressive target and
				// can attack our attacker then ask for help
				// FIXME ad support for help from Coward type units
				if (aiunit.IsAgressive() && CanTarget(*aiunit.Type, *attacker->Type)
					&& aiunit.CurrentOrder()->GetGoal() != attacker) {
					bool shouldAttack = aiunit.IsIdle() && aiunit.Threshold == 0;

					if (aiunit.CurrentAction() == UnitActionAttack) {
						const COrder_Attack &orderAttack = *static_cast<COrder_Attack *>(aiunit.CurrentOrder());
						const CUnit *oldGoal = orderAttack.GetGoal();

						if (oldGoal == NULL || (ThreatCalculate(defender, *attacker) < ThreatCalculate(defender, *oldGoal)
												//Wyrmgus start
	//											&& aiunit.MapDistanceTo(defender) <= aiunit.Stats->Variables[ATTACKRANGE_INDEX].Max)) {
												&& aiunit.MapDistanceTo(defender) <= aiunit.GetModifiedVariable(ATTACKRANGE_INDEX))) {
												//Wyrmgus end
							shouldAttack = true;
						}
					}

					if (shouldAttack) {
						const int delay = j; // To avoid lot of CPU consuption, send them with a small time difference.

						aiunit.Wait += delay;
						
						CommandAttack(aiunit, attacker->tilePos, const_cast<CUnit *>(attacker), FlushCommands, attacker->MapLayer);
					}
				}
			}
			if (!aiForce.Defending && aiForce.State > 0) {
				DebugPrint("%d: %d(%s) belong to attacking force, don't defend it\n" _C_
						   defender.Player->Index _C_ UnitNumber(defender) _C_ defender.Type->Ident.c_str());
				// unit belongs to an attacking force,
				// so don't send others force in such case.
				// FIXME: there may be other attacking the same place force who can help
				continue;
			}
			//Wyrmgus end
		}
	}
	*/
	//Wyrmgus end
	
	//Wyrmgus start
	//check if there are any nearby units with active AI, and send them to help
	for (int max_dist = 16; max_dist <= MaxMapWidth; max_dist *= 2) { //search for units in increasingly greater distances, until a helper is found
		bool has_helper = false;
		std::vector<CUnit *> helper_table;
		SelectAroundUnit(defender, max_dist, helper_table, HasSamePlayerAs(*defender.Player));
		for (size_t i = 0; i < helper_table.size(); ++i) {
			CUnit &aiunit = *helper_table[i];

			if (&defender == &aiunit) {
				continue;
			}
			
			if (!aiunit.IsAliveOnMap()) {
				continue;
			}

			if (!aiunit.CanMove()) {
				continue;
			}

			if (aiunit.BoardCount) { //if is transporting a unit, don't go to help, as that may endanger the cargo/passengers
				continue;
			}

			if (aiunit.Type->BoolFlag[HARVESTER_INDEX].value) { //harvesters shouldn't go to help units being attacked
				continue;
			}
			
			// if brother is idle or attack no-agressive target and
			// can attack our attacker then ask for help
			// FIXME ad support for help from Coward type units
			if (aiunit.Active && aiunit.IsAgressive() && CanTarget(*aiunit.Type, *attacker->Type)
				&& aiunit.CurrentOrder()->GetGoal() != attacker) {
				bool shouldAttack = aiunit.IsIdle() && aiunit.Threshold == 0;

				if (aiunit.CurrentAction() == UnitActionAttack) {
					const COrder_Attack &orderAttack = *static_cast<COrder_Attack *>(aiunit.CurrentOrder());
					const CUnit *oldGoal = orderAttack.GetGoal();

					if (oldGoal == NULL || (ThreatCalculate(defender, *attacker) < ThreatCalculate(defender, *oldGoal)
											&& aiunit.MapDistanceTo(defender) <= aiunit.GetModifiedVariable(ATTACKRANGE_INDEX))) {
						shouldAttack = true;
					}
				}

				if (shouldAttack) {
					CommandAttack(aiunit, attacker->tilePos, const_cast<CUnit *>(attacker), FlushCommands, attacker->MapLayer);
					has_helper = true;
				}
			}
		}
		if (has_helper) {
			break;
		}
	}
	//Wyrmgus end
}

/**
**  Called if a unit is killed.
**
**  @param unit  Pointer to unit.
*/
void AiUnitKilled(CUnit &unit)
{
	DebugPrint("%d: %d(%s) killed\n" _C_
			   unit.Player->Index _C_ UnitNumber(unit) _C_ unit.Type->Ident.c_str());

	Assert(unit.Player->Type != PlayerPerson);

	if (unit.GroupId) {
		AiForce &force = unit.Player->Ai->Force[unit.GroupId - 1];

		force.Remove(unit);
		if (force.Size() == 0) {
			force.Attacking = false;
			if (!force.Defending && force.State > 0) {
				DebugPrint("%d: Attack force #%lu was destroyed, giving up\n"
						   _C_ unit.Player->Index _C_(long unsigned int)(&force  - & (unit.Player->Ai->Force[0])));
				force.Reset(true);
			}
		}
	}

	unit.CurrentOrder()->AiUnitKilled(unit);
}

/**
**  Called if work complete (Buildings).
**
**  @param unit  Pointer to unit that builds the building.
**  @param what  Pointer to unit building that was built.
*/
void AiWorkComplete(CUnit *unit, CUnit &what)
{
	if (unit) {
		DebugPrint("%d: %d(%s) build %s at %d,%d completed\n" _C_
				   what.Player->Index _C_ UnitNumber(*unit) _C_ unit->Type->Ident.c_str() _C_
				   what.Type->Ident.c_str() _C_ unit->tilePos.x _C_ unit->tilePos.y);
	} else {
		DebugPrint("%d: building %s at %d,%d completed\n" _C_
				   what.Player->Index _C_ what.Type->Ident.c_str() _C_ what.tilePos.x _C_ what.tilePos.y);
	}

	Assert(what.Player->Type != PlayerPerson);
	//Wyrmgus start
//	AiRemoveFromBuilt(what.Player->Ai, *what.Type);
	AiRemoveFromBuilt(what.Player->Ai, *what.Type, Map.GetTileLandmass(what.tilePos, what.MapLayer), what.Settlement);
	//Wyrmgus end
}

/**
**  Called if building can't be build.
**
**  @param unit  Pointer to unit what builds the building.
**  @param what  Pointer to unit-type.
*/
//Wyrmgus start
//void AiCanNotBuild(const CUnit &unit, const CUnitType &what)
void AiCanNotBuild(const CUnit &unit, const CUnitType &what, int landmass, CSettlement *settlement)
//Wyrmgus end
{
	DebugPrint("%d: %d(%s) Can't build %s at %d,%d\n" _C_
			   unit.Player->Index _C_ UnitNumber(unit) _C_ unit.Type->Ident.c_str() _C_
			   what.Ident.c_str() _C_ unit.tilePos.x _C_ unit.tilePos.y);

	Assert(unit.Player->Type != PlayerPerson);
	//Wyrmgus start
//	AiReduceMadeInBuilt(*unit.Player->Ai, what);
	AiReduceMadeInBuilt(*unit.Player->Ai, what, landmass, settlement);
	//Wyrmgus end
}

/**
**  Called if building place can't be reached.
**
**  @param unit  Pointer to unit what builds the building.
**  @param what  Pointer to unit-type.
*/
//Wyrmgus start
//void AiCanNotReach(CUnit &unit, const CUnitType &what)
void AiCanNotReach(CUnit &unit, const CUnitType &what, int landmass, CSettlement *settlement)
//Wyrmgus end
{
	Assert(unit.Player->Type != PlayerPerson);
	//Wyrmgus start
//	AiReduceMadeInBuilt(*unit.Player->Ai, what);
	AiReduceMadeInBuilt(*unit.Player->Ai, what, landmass, settlement);
	//Wyrmgus end
}

/**
**  Try to move a unit that's in the way
*/
static void AiMoveUnitInTheWay(CUnit &unit)
{
	static Vec2i dirs[8] = {Vec2i(-1, -1), Vec2i(-1, 0), Vec2i(-1, 1), Vec2i(0, 1), Vec2i(1, 1), Vec2i(1, 0), Vec2i(1, -1), Vec2i(0, -1)};
	CUnit *movableunits[16];
	Vec2i movablepos[16];
	int movablenb;

	AiPlayer = unit.Player->Ai;

	// No more than 1 move per 10 cycle ( avoid stressing the pathfinder )
	if (GameCycle <= AiPlayer->LastCanNotMoveGameCycle + 10) {
		return;
	}

	const CUnitType &unittype = *unit.Type;
	const Vec2i u0 = unit.tilePos;
	const Vec2i u1(u0.x + unittype.TileWidth - 1, u0.y + unittype.TileHeight - 1);

	movablenb = 0;

	// Try to make some unit moves around it
	//Wyrmgus start
	std::vector<CUnit *> table;
	SelectAroundUnit(unit, 1, table);
//	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
	for (size_t i = 0; i != table.size(); ++i) {
	//Wyrmgus end
		//Wyrmgus start
//		CUnit &blocker = **it;
		CUnit &blocker = *table[i];
		//Wyrmgus end

		if (blocker.IsUnusable()) {
			continue;
		}
		//Wyrmgus start
		if (!blocker.IsIdle()) { // don't move units that aren't idle, as that will stop their current orders
			continue;
		}
		//Wyrmgus end
		if (!blocker.CanMove() || blocker.Moving) {
			continue;
		}
		if (blocker.Player != unit.Player && blocker.Player->IsAllied(*unit.Player) == false) {
			continue;
		}
		//Wyrmgus start
		if (!blocker.Player->AiEnabled) {
			continue;
		}
		
		int blocker_force = blocker.Player->Ai->Force.GetForce(blocker);
		if (blocker_force != -1 && blocker.Player->Ai->Force[blocker_force].Attacking) { //don't try to move the blocker if it is part of a force that is attacking
			if (unit.CurrentAction() != UnitActionBoard) { //unless the unit is trying to board a ship, in which case the blocker should be moved
				continue;
			}
		}
		//Wyrmgus end
		const CUnitType &blockertype = *blocker.Type;

		if (blockertype.UnitType != unittype.UnitType) {
			continue;
		}

		const Vec2i b0 = blocker.tilePos;
		const Vec2i b1(b0.x + blockertype.TileWidth - 1, b0.y + blockertype.TileHeight - 1);

		if (&unit == &blocker) {
			continue;
		}
		// Check for collision
		if (unit.MapDistanceTo(blocker) >= unit.Type->TileWidth + 1) {
			continue;
		}

		// Move blocker in a rand dir
		int r = SyncRand() & 7;
		int trycount = 8;
		while (trycount > 0) {
			r = (r + 1) & 7;
			--trycount;

			const Vec2i pos = blocker.tilePos + blocker.Type->TileWidth * dirs[r];

			// Out of the map => no !
			//Wyrmgus start
//			if (!Map.Info.IsPointOnMap(pos)) {
			if (!Map.Info.IsPointOnMap(pos, unit.MapLayer)) {
			//Wyrmgus end
				continue;
			}
			// move to blocker ? => no !
			if (pos == u0) {
				continue;
			}
			//Wyrmgus start
//			if (Map.Field(pos)->UnitCache.size() > 0) {
			if (Map.Field(pos, unit.MapLayer)->UnitCache.size() > 0) {
			//Wyrmgus end
				continue;
			}

			movableunits[movablenb] = &blocker;
			movablepos[movablenb] = pos;

			++movablenb;
			trycount = 0;
		}
		if (movablenb >= 16) {
			break;
		}
	}

	// Don't move more than 1 unit.
	if (movablenb) {
		const int index = SyncRand() % movablenb;
		COrder *savedOrder = NULL;
		if (movableunits[index]->IsIdle() == false) {
			if (unit.CanStoreOrder(unit.CurrentOrder())) {
				savedOrder = unit.CurrentOrder()->Clone();
			}
		}
		//Wyrmgus start
//		CommandMove(*movableunits[index], movablepos[index], FlushCommands);
		CommandMove(*movableunits[index], movablepos[index], FlushCommands, movableunits[index]->MapLayer);
		//Wyrmgus end
		if (savedOrder != NULL) {
			unit.SavedOrder = savedOrder;
		}
		AiPlayer->LastCanNotMoveGameCycle = GameCycle;
	}
}

/**
**  Called if a unit can't move. Try to move unit in the way
**
**  @param unit  Pointer to unit what builds the building.
*/
void AiCanNotMove(CUnit &unit)
{
	const Vec2i &goalPos = unit.pathFinderData->input.GetGoalPos();
	const int gw = unit.pathFinderData->input.GetGoalSize().x;
	const int gh = unit.pathFinderData->input.GetGoalSize().y;

	AiPlayer = unit.Player->Ai;
	//Wyrmgus start
//	if (PlaceReachable(unit, goalPos, gw, gh, 0, 255)) {
	if (PlaceReachable(unit, goalPos, gw, gh, 0, 511, 0, unit.MapLayer)) {
	//Wyrmgus end
		// Path probably closed by unit here
		AiMoveUnitInTheWay(unit);
		//Wyrmgus start
		if (!unit.Type->BoolFlag[HARVESTER_INDEX].value) {
			unit.Wait = CYCLES_PER_SECOND * 10; // wait a bit before trying to move the unit again; this is so when units are attacking an enemy they won't clog performance if one blocks the other; not for workers since otherwise they will spend too much time doing nothing if blocked briefly by another worker during gathering
		}
		//Wyrmgus end
	}
}

/**
**  Called if the AI needs more farms.
**
**  @param player  player which need supply.
*/
void AiNeedMoreSupply(const CPlayer &player)
{
	Assert(player.Type != PlayerPerson);
	player.Ai->NeedSupply = true;
}

/**
**  Called if training of a unit is completed.
**
**  @param unit  Pointer to unit making.
**  @param what  Pointer to new ready trained unit.
*/
void AiTrainingComplete(CUnit &unit, CUnit &what)
{
	DebugPrint("%d: %d(%s) training %s at %d,%d completed\n" _C_
			   //Wyrmgus start
//			   unit.Player->Index _C_ UnitNumber(unit) _C_ unit.Type->Ident.c_str() _C_
			   what.Player->Index _C_ UnitNumber(unit) _C_ unit.Type->Ident.c_str() _C_
			   //Wyrmgus end
			   what.Type->Ident.c_str() _C_ unit.tilePos.x _C_ unit.tilePos.y);

	Assert(what.Player->Type != PlayerPerson);

	//Wyrmgus start
//	AiRemoveFromBuilt(unit.Player->Ai, *what.Type);
	if (unit.Player == what.Player) {
		AiRemoveFromBuilt(what.Player->Ai, *what.Type, Map.GetTileLandmass(what.tilePos, what.MapLayer), what.Settlement);
	} else { //remove the request of the unit the mercenary is substituting
		int requested_unit_type_id = PlayerRaces.GetFactionClassUnitType(what.Player->Faction, what.Type->Class);
		if (requested_unit_type_id != -1) {
			AiRemoveFromBuilt(what.Player->Ai, *UnitTypes[requested_unit_type_id], Map.GetTileLandmass(what.tilePos, what.MapLayer), what.Settlement);
		}
	}
	//Wyrmgus end

	//Wyrmgus start
	what.Player->Ai->Force.RemoveDeadUnit();
	what.Player->Ai->Force.Assign(what, -1);
	
	if (what.Player->Ai->Force.GetForce(what) == -1) { // if the unit hasn't been assigned to a force, see if it is a transporter, and assign it accordingly
		if (what.Type->CanTransport() && what.CanMove() && (what.Type->UnitType == UnitTypeNaval || what.Type->UnitType == UnitTypeFly || what.Type->UnitType == UnitTypeFlyLow)) {
			int landmass = Map.GetTileLandmass(what.tilePos, what.MapLayer);
			
			what.Player->Ai->Transporters[landmass].push_back(&what);
		}
	}
	//Wyrmgus end
}

/**
**  Called if upgrading of an unit is completed.
**
**  @param unit Pointer to unit working.
**  @param what Pointer to the new unit-type.
*/
void AiUpgradeToComplete(CUnit &unit, const CUnitType &what)
{
	DebugPrint("%d: %d(%s) upgrade-to %s at %d,%d completed\n" _C_
			   unit.Player->Index _C_ UnitNumber(unit) _C_ unit.Type->Ident.c_str() _C_
			   what.Ident.c_str() _C_ unit.tilePos.x _C_ unit.tilePos.y);

	Assert(unit.Player->Type != PlayerPerson);
}

/**
**  Called if reseaching of an unit is completed.
**
**  @param unit  Pointer to unit working.
**  @param what  Pointer to the new upgrade.
*/
void AiResearchComplete(CUnit &unit, const CUpgrade *what)
{
	DebugPrint("%d: %d(%s) research %s at %d,%d completed\n" _C_
			   unit.Player->Index _C_ UnitNumber(unit) _C_ unit.Type->Ident.c_str() _C_
			   what->Ident.c_str() _C_ unit.tilePos.x _C_ unit.tilePos.y);

	Assert(unit.Player->Type != PlayerPerson);

	// FIXME: upgrading knights -> paladins, must rebuild lists!
}

/**
**  This is called for each player, each game cycle.
**
**  @param player  The player structure pointer.
*/
void AiEachCycle(CPlayer &player)
{
	AiPlayer = player.Ai;
}

/**
**  This is called for each player each second.
**
**  @param player  The player structure pointer.
*/
void AiEachSecond(CPlayer &player)
{
	AiPlayer = player.Ai;
#ifdef DEBUG
	if (!AiPlayer) {
		return;
	}
#endif

	//Wyrmgus start
	//if doesn't have a faction, set a random one for the AI
	if (player.Faction == -1) {
		player.SetRandomFaction();
	}
	//Wyrmgus end

	//  Advance script
	AiExecuteScript();
	
	//Wyrmgus start
	if (player.GetUnitCount() == 0) {
		return;
	}
	//Wyrmgus end
	
	AiPlayer->NeededMask = 0;

	//  Look if everything is fine.
	AiCheckUnits();

	//  Handle the resource manager.
	AiResourceManager();

	//  Handle the force manager.
	AiForceManager();

	//  Check for magic actions.
	AiCheckMagic();

	// At most 1 explorer each 5 seconds
	if (GameCycle > AiPlayer->LastExplorationGameCycle + 5 * CYCLES_PER_SECOND) {
		AiSendExplorers();
	}
}

/**
**  This is called for each player each half minute.
**
**  @param player  The player structure pointer.
*/
void AiEachHalfMinute(CPlayer &player)
{
	AiPlayer = player.Ai;
#ifdef DEBUG
	if (!AiPlayer) {
		return;
	}
#endif

	if (AiPlayer->Scouting) { //check periodically if has found new enemies
		AiPlayer->Scouting = false;
	}
	
	AiCheckWorkers();
	AiCheckUpgrades();
	AiCheckBuildings();
	
	AiForceManagerEachHalfMinute();
}

/**
**  This is called for each player each minute.
**
**  @param player  The player structure pointer.
*/
void AiEachMinute(CPlayer &player)
{
	AiPlayer = player.Ai;
#ifdef DEBUG
	if (!AiPlayer) {
		return;
	}
#endif

	AiCheckSettlementConstruction();
	AiCheckTransporters();
	AiCheckDockConstruction();
	
	AiForceManagerEachMinute();
}

int AiGetUnitTypeCount(const PlayerAi &pai, const CUnitType *type, const int landmass, const bool include_requests, const bool include_upgrades)
{
	int count = 0;
	
	//check for units in the landmass if it has been designated; otherwise look for all units of the type
	if (landmass) {
		std::vector<CUnit *> table;
		FindPlayerUnitsByType(*pai.Player, *type, table, true);
		
		for (size_t i = 0; i < table.size(); ++i) {
			CUnit &unit = *table[i];
					
			if (Map.GetTileLandmass(unit.tilePos, unit.MapLayer) == landmass) {
				count++;
			}
		}
	} else {
		count += pai.Player->GetUnitTypeAiActiveCount(type);
	}
		
	if (include_requests) {
		count += AiGetUnitTypeRequestedCount(pai, type, landmass);
	}
	
	if (include_upgrades) {
		if (type->Slot < (int) AiHelpers.UpgradesTo.size()) {
			for (size_t i = 0; i != AiHelpers.UpgradesTo[type->Slot].size(); ++i) {
				count += AiGetUnitTypeCount(pai, AiHelpers.UpgradesTo[type->Slot][i], landmass, include_requests, include_upgrades);
			}
		}
	}
	
	return count;
}

int AiGetUnitTypeRequestedCount(const PlayerAi &pai, const CUnitType *type, const int landmass, const CSettlement *settlement)
{
	int count = 0;
	
	for (size_t i = 0; i < pai.UnitTypeBuilt.size(); ++i) {
		const AiBuildQueue &queue = pai.UnitTypeBuilt[i];
		if (
			queue.Type == type
			&& (!landmass || queue.Landmass == landmass)
			&& (!settlement || queue.Settlement == settlement)
		) {
			count += queue.Want;
		}
	}
		
	return count;
}

bool AiHasUpgrade(const PlayerAi &pai, const CUpgrade *upgrade, bool include_requests)
{
	if (UpgradeIdAllowed(*pai.Player, upgrade->ID) == 'R') {
		return true;
	}

	if (include_requests) {
		if (pai.Player->UpgradeTimers.Upgrades[upgrade->ID] > 0) { //already researching
			return true;
		}
		
		if (std::find(pai.ResearchRequests.begin(), pai.ResearchRequests.end(), upgrade) != pai.ResearchRequests.end()) {
			return true;
		}
	}
	
	return false;
}

//@}
