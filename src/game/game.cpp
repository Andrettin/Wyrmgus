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
/**@name game.cpp - The game set-up and creation. */
//
//      (c) Copyright 1998-2021 by Lutz Sammer, Andreas Arens,
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

#include "stratagus.h"

#include "game.h"

#include "actions.h"
#include "age.h"
#include "ai.h"
#include "animation.h"
//Wyrmgus start
#include "character.h"
//Wyrmgus end
#include "civilization.h"
#include "commands.h"
#include "database/database.h"
#include "database/defines.h"
#include "database/sml_data.h"
#include "database/sml_parser.h"
#include "diplomacy_state.h"
#include "editor.h"
#include "faction.h"
#include "faction_type.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "iocompat.h"
#include "iolib.h"
#include "item/persistent_item.h"
#include "literary_text.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tileset.h"
#include "missile.h"
#include "netconnect.h"
#include "network.h"
#include "parameters.h"
#include "pathfinder.h"
#include "player.h"
#include "player_color.h"
//Wyrmgus start
#include "province.h"
//Wyrmgus end
#include "quest/campaign.h"
//Wyrmgus start
#include "quest/quest.h"
//Wyrmgus end
#include "replay.h"
#include "resource.h"
#include "results.h"
//Wyrmgus start
#include "script.h"
//Wyrmgus end
#include "script/condition/condition.h"
#include "script/effect/delayed_effect_instance.h"
#include "script/trigger.h"
#include "settings.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "spell/spell.h"
#include "time/calendar.h"
#include "translate.h"
#include "ui/button.h"
#include "ui/cursor.h"
#include "ui/icon.h"
#include "ui/interface.h"
#include "ui/resource_icon.h"
#include "ui/ui.h"
#include "unit/construction.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "util/exception_util.h"
#include "util/random.h"
#include "util/string_util.h"
//Wyrmgus start
#include "util/util.h"
//Wyrmgus end
#include "util/vector_util.h"
#include "version.h"
#include "video/font.h"
#include "video/video.h"
#include "widgets.h"

extern void CleanGame();

Settings GameSettings;					/// Game Settings
static int LcmPreventRecurse;			/// prevent recursion through LoadGameMap
GameResults GameResult;					/// Outcome of the game

std::string GameName;
std::string FullGameName;
//Wyrmgus start
std::string PlayerFaction;
//Wyrmgus end

unsigned long GameCycle;				/// Game simulation cycle counter
unsigned long FastForwardCycle;			/// Cycle to fastforward to in a replay

bool UseHPForXp = false;				/// true if gain XP by dealing damage, false if by killing.

bool DefiningData = false;

static std::vector<std::unique_ptr<gcn::Container>> Containers;

namespace wyrmgus {

game::game()
{
}

game::~game()
{
}

void game::apply_player_history()
{
	const CDate start_date = this->get_current_campaign()->get_start_date();

	for (CPlayer *player : CPlayer::Players) {
		if (player->Type == PlayerNobody || player->get_civilization() == nullptr || player->get_faction() == nullptr) {
			continue;
		}

		if (start_date.Year == 0) {
			continue;
		}

		player->apply_history(start_date);
	}
}

void game::do_cycle()
{
	if (GameCycle % CYCLES_PER_IN_GAME_HOUR == 0) {
		CDate::CurrentTotalHours++;

		this->current_date = this->current_date.addSecs(1 * 60 * 60 * DEFAULT_DAY_MULTIPLIER_PER_YEAR);

		for (const std::unique_ptr<CMapLayer> &map_layer : CMap::Map.MapLayers) {
			map_layer->DoPerHourLoop();
		}
	}
}

void game::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();

	exception::throw_with_trace(std::runtime_error("Invalid game data property: \"" + key + "\"."));
}

void game::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "player_delayed_effects") {
		scope.for_each_child([&](const sml_data &delayed_effect_data) {
			auto delayed_effect = std::make_unique<delayed_effect_instance<CPlayer>>();
			database::process_sml_data(delayed_effect, delayed_effect_data);
			this->add_delayed_effect(std::move(delayed_effect));
		});
	} else if (tag == "unit_delayed_effects") {
		scope.for_each_child([&](const sml_data &delayed_effect_data) {
			auto delayed_effect = std::make_unique<delayed_effect_instance<CUnit>>();
			database::process_sml_data(delayed_effect, delayed_effect_data);
			this->add_delayed_effect(std::move(delayed_effect));
		});
	} else if (tag == "site_data") {
		scope.for_each_child([&](const sml_data &child_scope) {
			const site *site = site::get(child_scope.get_tag());
			database::process_sml_data(site->get_game_data(), child_scope);
		});
	} else {
		exception::throw_with_trace(std::runtime_error("Invalid game data scope: \"" + scope.get_tag() + "\"."));
	}
}

void game::save(CFile &file) const
{
	sml_data game_data;

	if (!this->player_delayed_effects.empty()) {
		sml_data delayed_effects_data("player_delayed_effects");
		for (const auto &delayed_effect : this->player_delayed_effects) {
			delayed_effects_data.add_child(delayed_effect->to_sml_data());
		}
		game_data.add_child(std::move(delayed_effects_data));
	}

	if (!this->unit_delayed_effects.empty()) {
		sml_data delayed_effects_data("unit_delayed_effects");
		for (const auto &delayed_effect : this->unit_delayed_effects) {
			delayed_effects_data.add_child(delayed_effect->to_sml_data());
		}
		game_data.add_child(std::move(delayed_effects_data));
	}

	sml_data site_game_data("site_data");
	for (const site *site : site::get_all()) {
		if (site->get_game_data() == nullptr) {
			continue;
		}

		sml_data site_data = site->get_game_data()->to_sml_data();

		if (site_data.is_empty()) {
			continue;
		}

		site_game_data.add_child(std::move(site_data));
	}

	if (!site_game_data.is_empty()) {
		game_data.add_child(std::move(site_game_data));
	}

	const std::string str = "load_game_data(\"" + string::escaped(game_data.print_to_string()) + "\")\n";
	file.printf("%s", str.c_str());
}

void game::add_local_trigger(std::unique_ptr<trigger> &&local_trigger)
{
	this->local_triggers.push_back(std::move(local_trigger));
}

void game::remove_local_trigger(trigger *local_trigger)
{
	vector::remove(this->local_triggers, local_trigger);
}

void game::clear_local_triggers()
{
	this->local_triggers.clear();
}

void game::process_delayed_effects()
{
	this->process_delayed_effects(this->player_delayed_effects);
	this->process_delayed_effects(this->unit_delayed_effects);
}

void game::add_delayed_effect(std::unique_ptr<delayed_effect_instance<CPlayer>> &&delayed_effect)
{
	this->player_delayed_effects.push_back(std::move(delayed_effect));
}

void game::add_delayed_effect(std::unique_ptr<delayed_effect_instance<CUnit>> &&delayed_effect)
{
	this->unit_delayed_effects.push_back(std::move(delayed_effect));
}

void game::clear_delayed_effects()
{
	this->player_delayed_effects.clear();
	this->unit_delayed_effects.clear();
}

}

void load_game_data(const std::string &sml_string)
{
	wyrmgus::sml_parser parser;
	wyrmgus::database::process_sml_data(wyrmgus::game::get(), parser.parse(sml_string));
}

/**
**  Save game settings.
**
**  @param file  Save file handle
*/
void SaveGameSettings(CFile &file)
{
	file.printf("\nGameSettings.NetGameType = %d\n", GameSettings.NetGameType);
	for (int i = 0; i < PlayerMax - 1; ++i) {
		file.printf("GameSettings.Presets[%d].AIScript = \"%s\"\n", i, GameSettings.Presets[i].AIScript.c_str());
		file.printf("GameSettings.Presets[%d].Race = %d\n", i, GameSettings.Presets[i].Race);
		file.printf("GameSettings.Presets[%d].Team = %d\n", i, GameSettings.Presets[i].Team);
		file.printf("GameSettings.Presets[%d].Type = %d\n", i, GameSettings.Presets[i].Type);
	}
	file.printf("GameSettings.Resources = %d\n", GameSettings.Resources);
	file.printf("GameSettings.Difficulty = %d\n", GameSettings.Difficulty);
	file.printf("GameSettings.NumUnits = %d\n", GameSettings.NumUnits);
	file.printf("GameSettings.Opponents = %d\n", GameSettings.Opponents);
	file.printf("GameSettings.GameType = %d\n", GameSettings.GameType);
	file.printf("GameSettings.NoFogOfWar = %s\n", GameSettings.NoFogOfWar ? "true" : "false");
	file.printf("GameSettings.RevealMap = %d\n", GameSettings.RevealMap);
	file.printf("GameSettings.MapRichness = %d\n", GameSettings.MapRichness);
	file.printf("GameSettings.Inside = %s\n", GameSettings.Inside ? "true" : "false");
	//Wyrmgus start
	file.printf("GameSettings.NoRandomness = %s\n", GameSettings.NoRandomness ? "true" : "false");
	file.printf("GameSettings.NoTimeOfDay = %s\n", GameSettings.NoTimeOfDay ? "true" : "false");
	//Wyrmgus end
	file.printf("GameSettings.TechLevel = %d\n", GameSettings.TechLevel);
	file.printf("GameSettings.MaxTechLevel = %d\n", GameSettings.MaxTechLevel);
	file.printf("\n");
}

void StartMap(const std::string &filename, bool clean)
{
	try {
		std::string nc, rc;

		gcn::Widget *oldTop = Gui->getTop();

		auto container_unique_ptr = std::make_unique<gcn::Container>();
		gcn::Container *container = container_unique_ptr.get();
		Containers.push_back(std::move(container_unique_ptr));

		container->setDimension(gcn::Rectangle(0, 0, Video.Width, Video.Height));
		container->setOpaque(false);
		Gui->setTop(container);

		NetConnectRunning = 0;
		current_interface_state = interface_state::normal;

		//  Create the game.
		DebugPrint("Creating game with map: %s\n" _C_ filename.c_str());
		if (clean) {
			CleanPlayers();
		}

		//Wyrmgus start
		GameEstablishing = true;
		//Wyrmgus end
		CreateGame(filename, &CMap::Map);

		//Wyrmgus start
	//	UI.StatusLine.Set(NameLine);
		//Wyrmgus end
		//Wyrmgus start
		//commented this out because it seemed superfluous
	//	SetMessage("%s", _("Do it! Do it now!"));
		//Wyrmgus end

		//update the quest pool for all players
		for (size_t i = 0; i < PlayerNumNeutral; ++i) {
			CPlayer::Players[i]->update_quest_pool();
		}

		if (CPlayer::GetThisPlayer()->StartMapLayer < static_cast<int>(CMap::Map.MapLayers.size())) {
			UI.CurrentMapLayer = CMap::Map.MapLayers[CPlayer::GetThisPlayer()->StartMapLayer].get();
		}
		UI.SelectedViewport->Center(CMap::Map.tile_pos_to_scaled_map_pixel_pos_center(CPlayer::GetThisPlayer()->StartPos));

		UI.get_minimap()->Update();

		//  Play the game.
		GameMainLoop();

		//  Clear screen
		Video.ClearScreen();

		CleanGame();
		current_interface_state = interface_state::menu;

		Gui->setTop(oldTop);
		wyrmgus::vector::remove(Containers, container);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error running map."));
	}
}

void FreeAllContainers()
{
	Containers.clear();
}

/*----------------------------------------------------------------------------
--  Map loading/saving
----------------------------------------------------------------------------*/

/**
**  Load a Stratagus map.
**
**  @param smpname  smp filename
**  @param mapname  map filename
**  @param map      map loaded
*/
static void LoadStratagusMap(const std::string &smpname, const std::string &mapname)
{
	char mapfull[PATH_MAX];
	CFile file;

	// Try the same directory as the smp file first
	strcpy_s(mapfull, sizeof(mapfull), smpname.c_str());
	char *p = strrchr(mapfull, '/');
	if (!p) {
		p = mapfull;
	} else {
		++p;
	}
	strcpy_s(p, sizeof(mapfull) - (p - mapfull), mapname.c_str());

	if (file.open(mapfull, CL_OPEN_READ) == -1) {
		// Not found, try the root path and the smp's dir
		strcpy_s(mapfull, sizeof(mapfull), wyrmgus::database::get()->get_root_path().string().c_str());
		strcat_s(mapfull, sizeof(mapfull), "/");
		strcat_s(mapfull, sizeof(mapfull), smpname.c_str());
		char *p2 = strrchr(mapfull, '/');
		if (!p2) {
			p2 = mapfull;
		} else {
			++p2;
		}
		strcpy_s(p2, sizeof(mapfull) - (p2 - mapfull), mapname.c_str());
		if (file.open(mapfull, CL_OPEN_READ) == -1) {
			// Not found again, try the root path
			strcpy_s(mapfull, sizeof(mapfull), wyrmgus::database::get()->get_root_path().string().c_str());
			strcat_s(mapfull, sizeof(mapfull), "/");
			strcat_s(mapfull, sizeof(mapfull), mapname.c_str());
			if (file.open(mapfull, CL_OPEN_READ) == -1) {
				// Not found, try mapname by itself as a last resort
				strcpy_s(mapfull, sizeof(mapfull), mapname.c_str());
			} else {
				file.close();
			}
		} else {
			file.close();
		}
	} else {
		file.close();
	}

	if (LcmPreventRecurse) {
		exception::throw_with_trace(std::runtime_error("Recursive use of load map!"));
	}

	InitPlayers();

	LcmPreventRecurse = 1;
	if (LuaLoadFile(mapfull) == -1) {
		exception::throw_with_trace(std::runtime_error("Can't load lua file: " + std::string(mapfull)));
	}
	LcmPreventRecurse = 0;

#if 0
	// Not true if multiplayer levels!
	if (!ThisPlayer) { /// ARI: bomb if nothing was loaded!
		exception::throw_with_trace(std::runtime_error(mapname + ": invalid map"));
	}
#endif
	if (!CMap::Map.Info.MapWidth || !CMap::Map.Info.MapHeight) {
		exception::throw_with_trace(std::runtime_error(mapname + ": invalid map"));
	}
	CMap::Map.Info.Filename = mapname;
}

// Write the map presentation file
//Wyrmgus start
//static int WriteMapPresentation(const std::string &mapname, CMap &map)
static int WriteMapPresentation(const std::string &mapname, CMap &map, bool is_mod)
//Wyrmgus end
{
	static constexpr std::array<const char *, 8> type = {"", "", "neutral", "nobody", "computer", "person", "rescue-passive", "rescue-active" };

	std::unique_ptr<FileWriter> f;

	int numplayers = 0;
	int topplayer = PlayerMax - 2;

	try {
		f = CreateFileWriter(mapname);
		f->printf("-- Stratagus Map Presentation\n");
		f->printf("-- File generated by the " NAME " v" VERSION " built-in map editor.\n\n");
		// MAPTODO Copyright notice in generated file

		//Wyrmgus start
		/*
		f->printf("DefinePlayerTypes(");
		while (topplayer > 0 && map.Info.PlayerType[topplayer] == PlayerNobody) {
			--topplayer;
		}
		for (int i = 0; i <= topplayer; ++i) {
			f->printf("%s\"%s\"", (i ? ", " : ""), type[map.Info.PlayerType[i]]);
			if (map.Info.PlayerType[i] == PlayerPerson) {
				++numplayers;
			}
		}
		f->printf(")\n");

		f->printf("PresentMap(\"%s\", %d, %d, %d, %d)\n",
				  map.Info.Description.c_str(), numplayers, map.Info.MapWidth, map.Info.MapHeight,
				  map.Info.MapUID + 1);
		*/

		if (!is_mod) {
			f->printf("DefinePlayerTypes(");
			while (topplayer > 0 && map.Info.PlayerType[topplayer] == PlayerNobody) {
				--topplayer;
			}
			for (int i = 0; i <= topplayer; ++i) {
				f->printf("%s\"%s\"", (i ? ", " : ""), type[map.Info.PlayerType[i]]);
				if (map.Info.PlayerType[i] == PlayerPerson) {
					++numplayers;
				}
			}
			f->printf(")\n");

			f->printf("PresentMap(\"%s\", %d, %d, %d, %d)\n",
					  map.Info.Description.c_str(), numplayers, map.Info.MapWidth, map.Info.MapHeight,
					  map.Info.MapUID + 1);
		} else {
			f->printf("PresentMap(\"%s\")\n", map.Info.Description.c_str());
		}
		//Wyrmgus end

		//Wyrmgus start
		/*
		if (map.Info.Filename.find(".sms") == std::string::npos && !map.Info.Filename.empty()) {
			f->printf("DefineMapSetup(\"%s\")\n", map.Info.Filename.c_str());
		}
		*/
		//Wyrmgus end
	} catch (const FileException &) {
		fprintf(stderr, "ERROR: cannot write the map presentation\n");
		return -1;
	}

	return 1;
}


/**
**  Write the map setup file.
**
**  @param mapsetup      map filename
**  @param map           map to save
**  @param writeTerrain  write the tiles map in the .sms
*/
//Wyrmgus start
//int WriteMapSetup(const char *mapSetup, CMap &map, int writeTerrain)
int WriteMapSetup(const char *mapSetup, CMap &map, int writeTerrain, bool is_mod)
//Wyrmgus end
{
	std::unique_ptr<FileWriter> f;

	try {
		f = CreateFileWriter(mapSetup);

		f->printf("-- Stratagus Map Setup\n");
		f->printf("-- File generated by the " NAME " v" VERSION " built-in map editor.\n\n");
		// MAPTODO Copyright notice in generated file
		
		//Wyrmgus start
		std::string mod_file(mapSetup);
		mod_file = FindAndReplaceStringBeginning(mod_file, wyrmgus::database::get()->get_root_path().string() + "/", "");
		
		for (const wyrmgus::faction *faction : wyrmgus::faction::get_all()) {
			if (faction->Mod != CMap::Map.Info.Filename) {
				continue;
			}
				
			f->printf("DefineFaction(\"%s\", {\n", faction->get_identifier().c_str());
			f->printf("\tName = \"%s\",\n", faction->get_name().c_str());
			f->printf("\tCivilization = \"%s\",\n", faction->get_civilization()->get_identifier().c_str());
			if (faction->get_type() != wyrmgus::faction_type::none) {
				f->printf("\tType = \"%s\",\n", wyrmgus::faction_type_to_string(faction->get_type()).c_str());
			}
			if (faction->get_parent_faction() != nullptr) {
				f->printf("\tParentFaction = \"%s\",\n", faction->get_parent_faction()->get_identifier().c_str());
			}
			if (faction->get_color() != nullptr) {
				f->printf("\tColor = \"%s\",\n", faction->get_color()->get_identifier().c_str());
			}
			if (!faction->FactionUpgrade.empty()) {
				f->printf("\tFactionUpgrade = \"%s\",\n", faction->FactionUpgrade.c_str());
			}
			f->printf("\tMod = \"%s\"\n", mod_file.c_str());
			f->printf("})\n\n");
		}
		
		for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
			if (unit_type->Mod != CMap::Map.Info.Filename) {
				continue;
			}
			
			f->printf("DefineUnitType(\"%s\", {\n", unit_type->Ident.c_str());
			if (unit_type->Parent) {
				f->printf("\tParent = \"%s\",\n", unit_type->Parent->Ident.c_str());
			}
			if (!unit_type->get_name().empty() && (!unit_type->Parent || unit_type->get_name() != unit_type->Parent->get_name())) {
				f->printf("\tName = \"%s\",\n", unit_type->get_name().c_str());
			}
			if (unit_type->get_civilization() != nullptr) {
				f->printf("\tCivilization = \"%s\",\n", unit_type->get_civilization()->get_identifier().c_str());
			}
			if (unit_type->get_faction() != nullptr) {
				f->printf("\tFaction = \"%s\",\n", unit_type->get_faction()->get_identifier().c_str());
			}
			if (unit_type->get_unit_class() != nullptr) {
				f->printf("\tClass = \"%s\",\n", unit_type->get_unit_class()->get_identifier().c_str());
			}
			if (!unit_type->get_image_file().empty() && (!unit_type->Parent || unit_type->get_image_file() != unit_type->Parent->get_image_file())) {
				f->printf("\tImage = {\"file\", \"%s\", \"size\", {%d, %d}},\n", unit_type->get_image_file().string().c_str(), unit_type->get_frame_width(), unit_type->get_frame_height());
			}
			if (!unit_type->Icon.Name.empty() && (!unit_type->Parent || unit_type->Icon.Name != unit_type->Parent->Icon.Name)) {
				f->printf("\tIcon = \"%s\",\n", unit_type->Icon.Name.c_str());
			}
			if (unit_type->get_animation_set() != nullptr && (!unit_type->Parent || unit_type->get_animation_set() != unit_type->Parent->get_animation_set())) {
				f->printf("\tAnimations = \"%s\",\n", unit_type->get_animation_set()->get_identifier().c_str());
			}
			
			f->printf("\tCosts = {");
			for (unsigned int j = 0; j < MaxCosts; ++j) {
				if (unit_type->DefaultStat.Costs[j] != 0 && (!unit_type->Parent || unit_type->DefaultStat.Costs[j] != unit_type->Parent->DefaultStat.Costs[j])) {
					f->printf("\"%s\", ", DefaultResourceNames[j].c_str());
					f->printf("%d, ", unit_type->DefaultStat.Costs[j]);
				}
			}
			f->printf("},\n");
			f->printf("\tImproveProduction = {");
			for (unsigned int j = 0; j < MaxCosts; ++j) {
				if (unit_type->DefaultStat.ImproveIncomes[j] != 0 && (!unit_type->Parent || unit_type->DefaultStat.ImproveIncomes[j] != unit_type->Parent->DefaultStat.ImproveIncomes[j])) {
					f->printf("\"%s\", ", DefaultResourceNames[j].c_str());
					f->printf("%d, ", unit_type->DefaultStat.ImproveIncomes[j]);
				}
			}
			f->printf("},\n");
			f->printf("\tUnitStock = {");
			for (const auto &kv_pair : unit_type->DefaultStat.UnitStock) {
				const wyrmgus::unit_type *other_unit_type = wyrmgus::unit_type::get_all()[kv_pair.first];
				const int unit_stock = kv_pair.second;

				if (unit_stock != 0 && (!unit_type->Parent || unit_stock != unit_type->Parent->DefaultStat.GetUnitStock(other_unit_type))) {
					f->printf("\"%s\", ", other_unit_type->get_identifier().c_str());
					f->printf("%d, ", unit_stock);
				}
			}
			f->printf("},\n");
			
			for (size_t j = 0; j < UnitTypeVar.GetNumberVariable(); ++j) {
				if (!unit_type->Parent || unit_type->DefaultStat.Variables[j] != unit_type->Parent->DefaultStat.Variables[j]) {
					if (unit_type->DefaultStat.Variables[j].Increase != 0 || unit_type->DefaultStat.Variables[j].Value != unit_type->DefaultStat.Variables[j].Max) {
						f->printf("\t%s = {", UnitTypeVar.VariableNameLookup[j]);
						f->printf("Enable = %s, ", unit_type->DefaultStat.Variables[j].Enable ? "true" : "false");
						f->printf("Max = %d, ", unit_type->DefaultStat.Variables[j].Max);
						f->printf("Value = %d, ", unit_type->DefaultStat.Variables[j].Value);
						f->printf("Increase = %d", unit_type->DefaultStat.Variables[j].Increase);
						f->printf("},\n");
					} else if (unit_type->DefaultStat.Variables[j].Value != 0) {
						f->printf("\t%s = %d,\n", UnitTypeVar.VariableNameLookup[j], unit_type->DefaultStat.Variables[j].Value);
					}
				}
			}
			
			if (unit_type->ButtonPos != 0 && (!unit_type->Parent || unit_type->ButtonPos != unit_type->Parent->ButtonPos)) {
				f->printf("\tButtonPos = %d,\n", unit_type->ButtonPos);
			}
			if (!unit_type->get_button_key().empty() && (!unit_type->Parent || unit_type->get_button_key() != unit_type->Parent->get_button_key())) {
				f->printf("\tButtonKey = \"%s\",\n", unit_type->get_button_key().c_str());
			}
			if (!unit_type->ButtonHint.empty() && (!unit_type->Parent || unit_type->ButtonHint != unit_type->Parent->ButtonHint)) {
				f->printf("\tButtonHint = \"%s\",\n", unit_type->ButtonHint.c_str());
			}
			
			const wyrmgus::unit_sound_set *sound_set = unit_type->get_sound_set();
			if (sound_set != nullptr) {
				f->printf("\tSounds = {\n");

				const wyrmgus::unit_sound_set *parent_sound_set = nullptr;
				if (unit_type->Parent != nullptr) {
					parent_sound_set = unit_type->Parent->get_sound_set();
				}

				if (!sound_set->Selected.Name.empty() && (parent_sound_set == nullptr || sound_set->Selected.Name != parent_sound_set->Selected.Name)) {
					f->printf("\t\t\"selected\", \"%s\",\n", sound_set->Selected.Name.c_str());
				}
				if (!sound_set->Acknowledgement.Name.empty() && (parent_sound_set == nullptr || sound_set->Acknowledgement.Name != parent_sound_set->Acknowledgement.Name)) {
					f->printf("\t\t\"acknowledge\", \"%s\",\n", sound_set->Acknowledgement.Name.c_str());
				}
				if (!sound_set->Attack.Name.empty() && (parent_sound_set == nullptr || sound_set->Attack.Name != parent_sound_set->Attack.Name)) {
					f->printf("\t\t\"attack\", \"%s\",\n", sound_set->Attack.Name.c_str());
				}
				if (!sound_set->Idle.Name.empty() && (parent_sound_set == nullptr || sound_set->Idle.Name != parent_sound_set->Idle.Name)) {
					f->printf("\t\t\"idle\", \"%s\",\n", sound_set->Idle.Name.c_str());
				}
				if (!sound_set->Hit.Name.empty() && (parent_sound_set == nullptr || sound_set->Hit.Name != parent_sound_set->Hit.Name)) {
					f->printf("\t\t\"hit\", \"%s\",\n", sound_set->Hit.Name.c_str());
				}
				if (!sound_set->Miss.Name.empty() && (parent_sound_set == nullptr || sound_set->Miss.Name != parent_sound_set->Miss.Name)) {
					f->printf("\t\t\"miss\", \"%s\",\n", sound_set->Miss.Name.c_str());
				}
				if (!sound_set->FireMissile.Name.empty() && (parent_sound_set == nullptr || sound_set->FireMissile.Name != parent_sound_set->FireMissile.Name)) {
					f->printf("\t\t\"fire-missile\", \"%s\",\n", sound_set->FireMissile.Name.c_str());
				}
				if (!sound_set->Step.Name.empty() && (parent_sound_set == nullptr || sound_set->Step.Name != parent_sound_set->Step.Name)) {
					f->printf("\t\t\"step\", \"%s\",\n", sound_set->Step.Name.c_str());
				}
				if (!sound_set->StepDirt.Name.empty() && (parent_sound_set == nullptr || sound_set->StepDirt.Name != parent_sound_set->StepDirt.Name)) {
					f->printf("\t\t\"step-dirt\", \"%s\",\n", sound_set->StepDirt.Name.c_str());
				}
				if (!sound_set->StepGrass.Name.empty() && (parent_sound_set == nullptr || sound_set->StepGrass.Name != parent_sound_set->StepGrass.Name)) {
					f->printf("\t\t\"step-grass\", \"%s\",\n", sound_set->StepGrass.Name.c_str());
				}
				if (!sound_set->StepGravel.Name.empty() && (parent_sound_set == nullptr || sound_set->StepGravel.Name != parent_sound_set->StepGravel.Name)) {
					f->printf("\t\t\"step-gravel\", \"%s\",\n", sound_set->StepGravel.Name.c_str());
				}
				if (!sound_set->StepMud.Name.empty() && (parent_sound_set == nullptr || sound_set->StepMud.Name != parent_sound_set->StepMud.Name)) {
					f->printf("\t\t\"step-mud\", \"%s\",\n", sound_set->StepMud.Name.c_str());
				}
				if (!sound_set->StepStone.Name.empty() && (parent_sound_set == nullptr || sound_set->StepStone.Name != parent_sound_set->StepStone.Name)) {
					f->printf("\t\t\"step-stone\", \"%s\",\n", sound_set->StepStone.Name.c_str());
				}
				if (!sound_set->Used.Name.empty() && (parent_sound_set == nullptr || sound_set->Used.Name != parent_sound_set->Used.Name)) {
					f->printf("\t\t\"used\", \"%s\",\n", sound_set->Used.Name.c_str());
				}
				if (!sound_set->Build.Name.empty() && (parent_sound_set == nullptr || sound_set->Build.Name != parent_sound_set->Build.Name)) {
					f->printf("\t\t\"build\", \"%s\",\n", sound_set->Build.Name.c_str());
				}
				if (!sound_set->Ready.Name.empty() && (parent_sound_set == nullptr || sound_set->Ready.Name != parent_sound_set->Ready.Name)) {
					f->printf("\t\t\"ready\", \"%s\",\n", sound_set->Ready.Name.c_str());
				}
				if (!sound_set->Repair.Name.empty() && (parent_sound_set == nullptr || sound_set->Repair.Name != parent_sound_set->Repair.Name)) {
					f->printf("\t\t\"repair\", \"%s\",\n", sound_set->Repair.Name.c_str());
				}
				for (unsigned int j = 0; j < MaxCosts; ++j) {
					if (!sound_set->Harvest[j].Name.empty() && (parent_sound_set == nullptr || sound_set->Harvest[j].Name != parent_sound_set->Harvest[j].Name)) {
						f->printf("\t\t\"harvest\", \"%s\", \"%s\",\n", DefaultResourceNames[j].c_str(), sound_set->Harvest[j].Name.c_str());
					}
				}
				if (!sound_set->Help.Name.empty() && (parent_sound_set == nullptr || sound_set->Help.Name != parent_sound_set->Help.Name)) {
					f->printf("\t\t\"help\", \"%s\",\n", sound_set->Help.Name.c_str());
				}
				if (!sound_set->Dead[ANIMATIONS_DEATHTYPES].Name.empty() && (parent_sound_set == nullptr || sound_set->Dead[ANIMATIONS_DEATHTYPES].Name != parent_sound_set->Dead[ANIMATIONS_DEATHTYPES].Name)) {
					f->printf("\t\t\"dead\", \"%s\",\n", sound_set->Dead[ANIMATIONS_DEATHTYPES].Name.c_str());
				}
				int death;
				for (death = 0; death < ANIMATIONS_DEATHTYPES; ++death) {
					if (!sound_set->Dead[death].Name.empty() && (parent_sound_set == nullptr || sound_set->Dead[death].Name != parent_sound_set->Dead[death].Name)) {
						f->printf("\t\t\"dead\", \"%s\", \"%s\",\n", ExtraDeathTypes[death].c_str(), sound_set->Dead[death].Name.c_str());
					}
				}
				f->printf("\t},\n");
			}
			
			f->printf("\tMod = \"%s\"\n", mod_file.c_str());
			f->printf("})\n\n");
		}
		
		//save the definition of trained unit types separately, to avoid issues like a trained unit being defined after the unit that trains it
		for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
			if (unit_type->Mod != CMap::Map.Info.Filename) {
				continue;
			}
			
			if (unit_type->Trains.size() > 0) {
				f->printf("DefineUnitType(\"%s\", {\n", unit_type->Ident.c_str());
				
				f->printf("\tTrains = {");
				for (size_t j = 0; j < unit_type->Trains.size(); ++j) {
					f->printf("\"%s\", ", unit_type->Trains[j]->Ident.c_str());
				}
				f->printf("}\n");
				
				f->printf("})\n\n");
			}
		}		

		for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
			const auto mod_trains_find_iterator = unit_type->ModTrains.find(CMap::Map.Info.Filename);
			if (mod_trains_find_iterator != unit_type->ModTrains.end()) {
				f->printf("SetModTrains(\"%s\", \"%s\", {", mod_file.c_str(), unit_type->Ident.c_str());
				for (size_t j = 0; j < mod_trains_find_iterator->second.size(); ++j) {
					f->printf("\"%s\", ", mod_trains_find_iterator->second[j]->Ident.c_str());
				}
				f->printf("})\n\n");
			}
			
			const auto mod_ai_drops_find_iterator = unit_type->ModAiDrops.find(CMap::Map.Info.Filename);
			if (mod_ai_drops_find_iterator != unit_type->ModAiDrops.end()) {
				f->printf("SetModAiDrops(\"%s\", \"%s\", {", mod_file.c_str(), unit_type->Ident.c_str());
				for (size_t j = 0; j < mod_ai_drops_find_iterator->second.size(); ++j) {
					f->printf("\"%s\", ", mod_ai_drops_find_iterator->second[j]->Ident.c_str());
				}
				f->printf("})\n\n");
			}
		}
		//Wyrmgus end

		//Wyrmgus start
		/*
		f->printf("-- player configuration\n");
		for (int i = 0; i < PlayerMax; ++i) {
			if (Map.Info.PlayerType[i] == PlayerNobody) {
				continue;
			}
			f->printf("SetStartView(%d, %d, %d)\n", i, Players[i].StartPos.x, Players[i].StartPos.y);
			f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
					  i, DefaultResourceNames[WoodCost].c_str(),
					  Players[i].Resources[WoodCost]);
			f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
					  i, DefaultResourceNames[GoldCost].c_str(),
					  Players[i].Resources[GoldCost]);
			f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
					  i, DefaultResourceNames[OilCost].c_str(),
					  Players[i].Resources[OilCost]);
			f->printf("SetPlayerData(%d, \"RaceName\", \"%s\")\n",
					  i, PlayerRaces.Name[Players[i].Race].c_str());
			f->printf("SetAiType(%d, \"%s\")\n",
					  i, Players[i].AiName.c_str());
		}
		f->printf("\n");

		f->printf("-- load tilesets\n");
		f->printf("LoadTileModels(\"%s\")\n\n", map.TileModelsFileName.c_str());
		*/
		if (!is_mod) {
			f->printf("-- player configuration\n");
			for (int i = 0; i < PlayerMax; ++i) {
				if (CMap::Map.Info.PlayerType[i] == PlayerNobody) {
					continue;
				}
				f->printf("SetStartView(%d, %d, %d)\n", i, CPlayer::Players[i]->StartPos.x, CPlayer::Players[i]->StartPos.y);
				f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
						  i, DefaultResourceNames[WoodCost].c_str(),
						  CPlayer::Players[i]->Resources[WoodCost]);
				f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
						  i, DefaultResourceNames[CopperCost].c_str(),
						  CPlayer::Players[i]->Resources[CopperCost]);
				if (CPlayer::Players[i]->Resources[OilCost]) {
					f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
							  i, DefaultResourceNames[OilCost].c_str(),
							  CPlayer::Players[i]->Resources[OilCost]);
				}
				f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
						  i, DefaultResourceNames[StoneCost].c_str(),
						  CPlayer::Players[i]->Resources[StoneCost]);
				f->printf("SetPlayerData(%d, \"RaceName\", \"%s\")\n",
						  i, wyrmgus::civilization::get_all()[CPlayer::Players[i]->Race]->get_identifier().c_str());
				if (CPlayer::Players[i]->Faction != -1) {
					f->printf("SetPlayerData(%d, \"Faction\", \"%s\")\n",
							  i, CPlayer::Players[i]->get_faction()->get_identifier().c_str());
				}
				f->printf("SetAiType(%d, \"%s\")\n",
						  i, CPlayer::Players[i]->AiName.c_str());
			}
			f->printf("\n");

			f->printf("-- load tilesets\n");
			f->printf("LoadTileModels(\"%s\")\n\n", map.TileModelsFileName.c_str());
		}
		//Wyrmgus end

		if (writeTerrain) {
			f->printf("-- Tiles Map\n");
			//Wyrmgus start
			for (const std::unique_ptr<CMapLayer> &map_layer: map.MapLayers) {
				for (int y = 0; y < map_layer->get_height(); ++y) {
					for (int x = 0; x < map_layer->get_width(); ++x) {
						//Wyrmgus start
						const wyrmgus::tile &mf = *map_layer->Field(x, y);
	//					const int tile = mf.getGraphicTile();
	//					const int n = map.Tileset->findTileIndexByTile(tile);
						//Wyrmgus end
						const int value = mf.get_value();
						//Wyrmgus start
	//					f->printf("SetTile(%3d, %d, %d, %d)\n", n, j, i, value);
						f->printf("SetTileTerrain(\"%s\", %d, %d, %d, %d)\n", mf.get_terrain()->get_identifier().c_str(), x, y, 0, map_layer->ID);
						if (mf.get_overlay_terrain() != nullptr) {
							f->printf("SetTileTerrain(\"%s\", %d, %d, %d, %d)\n", mf.get_overlay_terrain()->get_identifier().c_str(), x, y, value, map_layer->ID);
						}
						//Wyrmgus end
					}
				}
			}
			//Wyrmgus end
		}
			
		f->printf("\n-- set map default stat and map sound for unit types\n");
		for (wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
			if (unit_type->ModDefaultStats.find(CMap::Map.Info.Filename) != unit_type->ModDefaultStats.end()) {
				for (unsigned int j = 0; j < MaxCosts; ++j) {
					if (unit_type->ModDefaultStats[CMap::Map.Info.Filename].Costs[j] != 0) {
						f->printf("SetModStat(\"%s\", \"%s\", \"Costs\", %d, \"%s\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModDefaultStats[CMap::Map.Info.Filename].Costs[j], DefaultResourceNames[j].c_str());
					}
				}
				for (unsigned int j = 0; j < MaxCosts; ++j) {
					if (unit_type->ModDefaultStats[CMap::Map.Info.Filename].ImproveIncomes[j] != 0) {
						f->printf("SetModStat(\"%s\", \"%s\", \"ImproveProduction\", %d, \"%s\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModDefaultStats[CMap::Map.Info.Filename].ImproveIncomes[j], DefaultResourceNames[j].c_str());
					}
				}

				for (const auto &kv_pair : unit_type->ModDefaultStats[CMap::Map.Info.Filename].UnitStock) {
					const wyrmgus::unit_type *other_unit_type = wyrmgus::unit_type::get_all()[kv_pair.first];
					const int unit_stock = kv_pair.second;

					if (unit_stock != 0) {
						f->printf("SetModStat(\"%s\", \"%s\", \"UnitStock\", %d, \"%s\")\n", mod_file.c_str(), unit_type->get_identifier().c_str(), unit_stock, other_unit_type->get_identifier().c_str());
					}
				}

				for (size_t j = 0; j < UnitTypeVar.GetNumberVariable(); ++j) {
					if (unit_type->ModDefaultStats[CMap::Map.Info.Filename].Variables[j].Value != 0) {
						f->printf("SetModStat(\"%s\", \"%s\", \"%s\", %d, \"Value\")\n", mod_file.c_str(), unit_type->Ident.c_str(), UnitTypeVar.VariableNameLookup[j], unit_type->ModDefaultStats[CMap::Map.Info.Filename].Variables[j].Value);
					}
					if (unit_type->ModDefaultStats[CMap::Map.Info.Filename].Variables[j].Max != 0) {
						f->printf("SetModStat(\"%s\", \"%s\", \"%s\", %d, \"Max\")\n", mod_file.c_str(), unit_type->Ident.c_str(), UnitTypeVar.VariableNameLookup[j], unit_type->ModDefaultStats[CMap::Map.Info.Filename].Variables[j].Max);
					}
					if (unit_type->ModDefaultStats[CMap::Map.Info.Filename].Variables[j].Enable != 0 && (unit_type->ModDefaultStats[CMap::Map.Info.Filename].Variables[j].Value != 0 || unit_type->ModDefaultStats[CMap::Map.Info.Filename].Variables[j].Max != 0 || unit_type->ModDefaultStats[CMap::Map.Info.Filename].Variables[j].Increase != 0)) {
						f->printf("SetModStat(\"%s\", \"%s\", \"%s\", %d, \"Enable\")\n", mod_file.c_str(), unit_type->Ident.c_str(), UnitTypeVar.VariableNameLookup[j], unit_type->ModDefaultStats[CMap::Map.Info.Filename].Variables[j].Enable);
					}
					if (unit_type->ModDefaultStats[CMap::Map.Info.Filename].Variables[j].Increase != 0) {
						f->printf("SetModStat(\"%s\", \"%s\", \"%s\", %d, \"Increase\")\n", mod_file.c_str(), unit_type->Ident.c_str(), UnitTypeVar.VariableNameLookup[j], unit_type->ModDefaultStats[CMap::Map.Info.Filename].Variables[j].Increase);
					}
				}
			}
			
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].Selected.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"selected\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].Selected.Name.c_str());
			}
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].Acknowledgement.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"acknowledge\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].Acknowledgement.Name.c_str());
			}
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].Attack.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"attack\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].Attack.Name.c_str());
			}
			//Wyrmgus start
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].Idle.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"idle\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].Idle.Name.c_str());
			}
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].Hit.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"hit\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].Hit.Name.c_str());
			}
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].Miss.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"miss\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].Miss.Name.c_str());
			}
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].FireMissile.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"fire-missile\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].FireMissile.Name.c_str());
			}
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].Step.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"step\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].Step.Name.c_str());
			}
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].StepDirt.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"step-dirt\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].StepDirt.Name.c_str());
			}
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].StepGrass.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"step-grass\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].StepGrass.Name.c_str());
			}
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].StepGravel.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"step-gravel\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].StepGravel.Name.c_str());
			}
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].StepMud.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"step-mud\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].StepMud.Name.c_str());
			}
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].StepStone.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"step-stone\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].StepStone.Name.c_str());
			}
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].Used.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"used\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].Used.Name.c_str());
			}
			//Wyrmgus end
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].Build.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"build\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].Build.Name.c_str());
			}
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].Ready.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"ready\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].Ready.Name.c_str());
			}
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].Repair.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"repair\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].Repair.Name.c_str());
			}
			for (unsigned int j = 0; j < MaxCosts; ++j) {
				if (!unit_type->ModSounds[CMap::Map.Info.Filename].Harvest[j].Name.empty()) {
					f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"harvest\", \"%s\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].Harvest[j].Name.c_str(), DefaultResourceNames[j].c_str());
				}
			}
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].Help.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"help\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].Help.Name.c_str());
			}
			if (!unit_type->ModSounds[CMap::Map.Info.Filename].Dead[ANIMATIONS_DEATHTYPES].Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"dead\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].Dead[ANIMATIONS_DEATHTYPES].Name.c_str());
			}
			int death;
			for (death = 0; death < ANIMATIONS_DEATHTYPES; ++death) {
				if (!unit_type->ModSounds[CMap::Map.Info.Filename].Dead[death].Name.empty()) {
					f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"dead\", \"%s\")\n", mod_file.c_str(), unit_type->Ident.c_str(), unit_type->ModSounds[CMap::Map.Info.Filename].Dead[death].Name.c_str(), ExtraDeathTypes[death].c_str());
				}
			}
		}
		
		//Wyrmgus start
		/*
		f->printf("\n-- place units\n");
		f->printf("if (MapUnitsInit ~= nil) then MapUnitsInit() end\n");
		std::vector<CUnit *> teleporters;
		for (auto it = wyrmgus::unit_manager::get()->get_units().begin(); it != wyrmgus::unit_manager::get()->get_units().end(); ++it) {
			const CUnit &unit = **it;
			f->printf("unit = CreateUnit(\"%s\", %d, {%d, %d})\n",
					  unit.Type->Ident.c_str(),
					  unit.Player->Index,
					  unit.tilePos.x, unit.tilePos.y);
			if (unit.Type->get_given_resource() != nullptr) {
				f->printf("SetResourcesHeld(unit, %d)\n", unit.ResourcesHeld);
			}
			if (!unit.Active) { //Active is true by default
				f->printf("SetUnitVariable(unit, \"Active\", false)\n");
			}
			if (unit.Type->BoolFlag[TELEPORTER_INDEX].value && unit.Goal) {
				teleporters.push_back(*it);
			}
		}
		f->printf("\n\n");
		for (std::vector<CUnit *>::iterator it = teleporters.begin(); it != teleporters.end(); ++it) {
			CUnit &unit = **it;
			f->printf("SetTeleportDestination(%d, %d)\n", UnitNumber(unit), UnitNumber(*unit.Goal));
		}
		f->printf("\n\n");
		*/
		if (!is_mod) {
			f->printf("\n-- place units\n");
			f->printf("if (MapUnitsInit ~= nil) then MapUnitsInit() end\n");
			std::vector<const CUnit *> teleporters;
			for (const CUnit *unit : wyrmgus::unit_manager::get()->get_units()) {
				f->printf("unit = CreateUnit(\"%s\", %d, {%d, %d})\n",
						  unit->Type->Ident.c_str(),
						  unit->Player->Index,
						  unit->tilePos.x, unit->tilePos.y);
				if (unit->Type->get_given_resource() != nullptr) {
					f->printf("SetResourcesHeld(unit, %d)\n", unit->ResourcesHeld);
				}
				if (!unit->Active) { //Active is true by default
					f->printf("SetUnitVariable(unit, \"Active\", false)\n");
				}
				if (unit->get_character() != nullptr) {
					if (!unit->get_character()->Custom) {
						f->printf("SetUnitVariable(unit, \"Character\", \"%s\")\n", unit->get_character()->get_identifier().c_str());
					} else {
						f->printf("SetUnitVariable(unit, \"CustomHero\", \"%s\")\n", unit->get_character()->get_identifier().c_str());
					}
				} else {
					if (!unit->Name.empty()) {
						f->printf("SetUnitVariable(unit, \"Name\", \"%s\")\n", unit->Name.c_str());
					}
				}
				if (unit->Trait != nullptr) {
					f->printf("AcquireTrait(unit, \"%s\")\n", unit->Trait->get_identifier().c_str());
				}
				if (unit->Prefix != nullptr) {
					f->printf("SetUnitVariable(unit, \"Prefix\", \"%s\")\n", unit->Prefix->get_identifier().c_str());
				}
				if (unit->Suffix != nullptr) {
					f->printf("SetUnitVariable(unit, \"Suffix\", \"%s\")\n", unit->Suffix->get_identifier().c_str());
				}
				if (unit->Work != nullptr) {
					f->printf("SetUnitVariable(unit, \"Work\", \"%s\")\n", unit->Work->get_identifier().c_str());
				}
				if (unit->Elixir != nullptr) {
					f->printf("SetUnitVariable(unit, \"Elixir\", \"%s\")\n", unit->Elixir->get_identifier().c_str());
				}
				if (unit->Variable[HP_INDEX].Value != unit->GetModifiedVariable(HP_INDEX, VariableAttribute::Max)) {
					f->printf("SetUnitVariable(unit, \"HitPoints\", %d)\n", unit->Variable[HP_INDEX].Value);
				}
				if (unit->Type->BoolFlag[TELEPORTER_INDEX].value && unit->Goal) {
					teleporters.push_back(unit);
				}
			}
			f->printf("\n\n");
			for (const CUnit *unit : teleporters) {
				f->printf("SetTeleportDestination(%d, %d)\n", UnitNumber(*unit), UnitNumber(*unit->Goal));
			}
			f->printf("\n\n");
		}
		//Wyrmgus end
	} catch (const FileException &) {
		fprintf(stderr, "Can't save map setup : '%s' \n", mapSetup);
		return -1;
	}

	return 1;
}



/**
**  Save a Stratagus map.
**
**  @param mapName   map filename
**  @param map       map to save
**  @param writeTerrain   write the tiles map in the .sms
*/
//Wyrmgus start
//int SaveStratagusMap(const std::string &mapName, CMap &map, int writeTerrain)
int SaveStratagusMap(const std::string &mapName, CMap &map, int writeTerrain, bool is_mod)
//Wyrmgus end
{
	if (!map.Info.MapWidth || !map.Info.MapHeight) {
		exception::throw_with_trace(std::runtime_error(mapName + ": invalid Stratagus map"));
	}

	char mapSetup[PATH_MAX];
	strcpy_s(mapSetup, sizeof(mapSetup), mapName.c_str());
	char *setupExtension = strstr(mapSetup, ".smp");
	if (!setupExtension) {
		exception::throw_with_trace(std::runtime_error(mapName + ": invalid Stratagus map filename"));
	}

	memcpy(setupExtension, ".sms", 4 * sizeof(char));
	//Wyrmgus start
//	if (WriteMapPresentation(mapName, map) == -1) {
	if (WriteMapPresentation(mapName, map, is_mod) == -1) {
	//Wyrmgus end
		return -1;
	}

	//Wyrmgus start
//	return WriteMapSetup(mapSetup, map, writeTerrain);
	return WriteMapSetup(mapSetup, map, writeTerrain, is_mod);
	//Wyrmgus end
}


/**
**  Load any map.
**
**  @param filename  map filename
**  @param map       map loaded
*/
//Wyrmgus start
//static void LoadMap(const std::string &filename, CMap &map)
static void LoadMap(const std::string &filename, CMap &map, bool is_mod)
//Wyrmgus end
{
	const char *name = filename.c_str();
	const char *tmp = strrchr(name, '.');
	if (tmp) {
#ifdef USE_ZLIB
		if (!strcmp(tmp, ".gz")) {
			while (tmp - 1 > name && *--tmp != '.') {
			}
		}
#endif
#ifdef USE_BZ2LIB
		if (!strcmp(tmp, ".bz2")) {
			while (tmp - 1 > name && *--tmp != '.') {
			}
		}
#endif
		if (!strcmp(tmp, ".smp")
#ifdef USE_ZLIB
			|| !strcmp(tmp, ".smp.gz")
#endif
#ifdef USE_BZ2LIB
			|| !strcmp(tmp, ".smp.bz2")
#endif
		   ) {
			if (map.Info.Filename.empty()) {
				// The map info hasn't been loaded yet => do it now
				LoadStratagusMapInfo(filename);
			}
			Assert(!map.Info.Filename.empty());
			//Wyrmgus start
//			map.Create();
//			LoadStratagusMap(filename, map.Info.Filename);
			if (!is_mod) {
				map.Create();
				LoadStratagusMap(filename, map.Info.Filename);
			} else {
				DisableMod(LibraryFileName(map.Info.Filename.c_str()));
				LuaLoadFile(LibraryFileName(map.Info.Filename.c_str()));
			}
			//Wyrmgus end
			return;
		}
	}

	exception::throw_with_trace(std::runtime_error("Unrecognized map format."));
}

/**
**  Set the game paused or unpaused
**
**  @param paused  True to pause game, false to unpause.
*/
void SetGamePaused(bool paused)
{
	//Wyrmgus start
	KeyScrollState = MouseScrollState = ScrollNone;
	//Wyrmgus end

	GamePaused = paused;
}

/**
**  Get the game paused or unpaused
**
**  @return  True if the game is paused, false otherwise
*/
bool GetGamePaused()
{
	return GamePaused;
}

/**
**  Set the game speed
**
**  @param speed  New game speed.
*/
void SetGameSpeed(int speed)
{
	if (GameCycle == 0 || FastForwardCycle < GameCycle) {
		VideoSyncSpeed = speed * 100 / CYCLES_PER_SECOND;
		SetVideoSync();
	}
}

/**
**  Get the game speed
**
**  @return  Game speed
*/
int GetGameSpeed()
{
	return CYCLES_PER_SECOND * VideoSyncSpeed / 100;
}

/*----------------------------------------------------------------------------
--  Game types
----------------------------------------------------------------------------*/

//Wyrmgus start
/**
**  Melee
*/
static void GameTypeMelee()
{
	for (int i = 0; i < PlayerMax - 1; ++i) {
		if (CPlayer::Players[i]->has_neutral_faction_type()) {
			continue;
		}

		for (int j = i + 1; j < PlayerMax - 1; ++j) {
			if (CPlayer::Players[j]->has_neutral_faction_type()) {
				continue;
			}

			if (CPlayer::Players[i]->Type == PlayerComputer && CPlayer::Players[j]->Type == PlayerComputer) {
				CommandDiplomacy(i, wyrmgus::diplomacy_state::allied, j);
				CPlayer::Players[i]->ShareVisionWith(*CPlayer::Players[j]);
				CommandDiplomacy(j, wyrmgus::diplomacy_state::allied, i);
				CPlayer::Players[j]->ShareVisionWith(*CPlayer::Players[i]);
			} else {
				CommandDiplomacy(i, wyrmgus::diplomacy_state::enemy, j);
				CommandDiplomacy(j, wyrmgus::diplomacy_state::enemy, i);
			}
		}
	}
}
//Wyrmgus end

/**
**  Free for all
*/
static void GameTypeFreeForAll()
{
	for (int i = 0; i < PlayerMax - 1; ++i) {
		if (CPlayer::Players[i]->has_neutral_faction_type()) {
			continue;
		}

		for (int j = i + 1; j < PlayerMax - 1; ++j) {
			if (CPlayer::Players[j]->has_neutral_faction_type()) {
				continue;
			}
			
			CommandDiplomacy(i, wyrmgus::diplomacy_state::enemy, j);
			CommandDiplomacy(j, wyrmgus::diplomacy_state::enemy, i);
		}
	}
}

/**
**  Top vs Bottom
*/
static void GameTypeTopVsBottom()
{
	const int middle = CMap::Map.Info.MapHeight / 2;

	for (int i = 0; i < PlayerMax - 1; ++i) {
		if (CPlayer::Players[i]->has_neutral_faction_type()) {
			continue;
		}

		const bool top_i = CPlayer::Players[i]->StartPos.y <= middle;

		for (int j = i + 1; j < PlayerMax - 1; ++j) {
			if (CPlayer::Players[j]->has_neutral_faction_type()) {
				continue;
			}

			const bool top_j = CPlayer::Players[j]->StartPos.y <= middle;

			if (top_i == top_j) {
				CommandDiplomacy(i, wyrmgus::diplomacy_state::allied, j);
				CPlayer::Players[i]->ShareVisionWith(*CPlayer::Players[j]);
				CommandDiplomacy(j, wyrmgus::diplomacy_state::allied, i);
				CPlayer::Players[j]->ShareVisionWith(*CPlayer::Players[i]);
			} else {
				CommandDiplomacy(i, wyrmgus::diplomacy_state::enemy, j);
				CommandDiplomacy(j, wyrmgus::diplomacy_state::enemy, i);
			}
		}
	}
}

/**
**  Left vs Right
*/
static void GameTypeLeftVsRight()
{
	const int middle = CMap::Map.Info.MapWidth / 2;

	for (int i = 0; i < PlayerMax - 1; ++i) {
		if (CPlayer::Players[i]->has_neutral_faction_type()) {
			continue;
		}

		const bool left_i = CPlayer::Players[i]->StartPos.x <= middle;
		
		for (int j = i + 1; j < PlayerMax - 1; ++j) {
			if (CPlayer::Players[j]->has_neutral_faction_type()) {
				continue;
			}
		
			const bool left_j = CPlayer::Players[j]->StartPos.x <= middle;

			if (left_i == left_j) {
				CommandDiplomacy(i, wyrmgus::diplomacy_state::allied, j);
				CPlayer::Players[i]->ShareVisionWith(*CPlayer::Players[j]);
				CommandDiplomacy(j, wyrmgus::diplomacy_state::allied, i);
				CPlayer::Players[j]->ShareVisionWith(*CPlayer::Players[i]);
			} else {
				CommandDiplomacy(i, wyrmgus::diplomacy_state::enemy, j);
				CommandDiplomacy(j, wyrmgus::diplomacy_state::enemy, i);
			}
		}
	}
}

/**
**  Man vs Machine
*/
static void GameTypeManVsMachine()
{
	for (int i = 0; i < PlayerMax - 1; ++i) {
		if (CPlayer::Players[i]->Type != PlayerPerson && CPlayer::Players[i]->Type != PlayerComputer) {
			continue;
		}
		if (CPlayer::Players[i]->has_neutral_faction_type()) {
			continue;
		}

		for (int j = i + 1; j < PlayerMax - 1; ++j) {
			if (CPlayer::Players[j]->Type != PlayerPerson && CPlayer::Players[j]->Type != PlayerComputer) {
				continue;
			}
			if (CPlayer::Players[j]->has_neutral_faction_type()) {
				continue;
			}

			if (CPlayer::Players[i]->Type == CPlayer::Players[j]->Type) {
				CommandDiplomacy(i, wyrmgus::diplomacy_state::allied, j);
				CPlayer::Players[i]->ShareVisionWith(*CPlayer::Players[j]);
				CommandDiplomacy(j, wyrmgus::diplomacy_state::allied, i);
				CPlayer::Players[j]->ShareVisionWith(*CPlayer::Players[i]);
			} else {
				CommandDiplomacy(i, wyrmgus::diplomacy_state::enemy, j);
				CommandDiplomacy(j, wyrmgus::diplomacy_state::enemy, i);
			}
		}
	}
}

/**
**  Man vs Machine with Humans on a Team
*/
static void GameTypeManTeamVsMachine()
{
	for (int i = 0; i < PlayerMax - 1; ++i) {
		if (CPlayer::Players[i]->Type != PlayerPerson && CPlayer::Players[i]->Type != PlayerComputer) {
			continue;
		}
		if (CPlayer::Players[i]->has_neutral_faction_type()) {
			continue;
		}

		for (int j = 0; j < PlayerMax - 1; ++j) {
			if (CPlayer::Players[j]->has_neutral_faction_type()) {
				continue;
			}

			if (i != j) {
				if (CPlayer::Players[i]->Type == CPlayer::Players[j]->Type) {
					CommandDiplomacy(i, wyrmgus::diplomacy_state::allied, j);
					CPlayer::Players[i]->ShareVisionWith(*CPlayer::Players[j]);
				} else {
					CommandDiplomacy(i, wyrmgus::diplomacy_state::enemy, j);
				}
			}
		}
		if (CPlayer::Players[i]->Type == PlayerPerson) {
			CPlayer::Players[i]->Team = 2;
		} else {
			CPlayer::Players[i]->Team = 1;
		}
	}
}

//
// LoadingBar
// TODO: move this to a new class

static int itemsToLoad;
static int itemsLoaded;
static std::shared_ptr<CGraphic> loadingEmpty;
static std::shared_ptr<CGraphic> loadingFull;
static std::vector<std::string> loadingBackgrounds;
std::shared_ptr<CGraphic> loadingBackground;
static wyrmgus::font *loadingFont = nullptr;
static std::vector<std::string> loadingTips;
static std::vector<std::string> loadingTip;

static int CclLoadingBarSetTips(lua_State *l)
{
	loadingTips.clear();
	
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	const char *str = nullptr;
	int i = 1;
	do {
		lua_pushinteger(l, i++);
		lua_gettable(l, -2);
		str = lua_tostring(l, -1);
		if (str) {
			loadingTips.push_back(str);
		}
		lua_pop(l, 1);
	} while (str != nullptr);

	return 0;
}

static int CclLoadingBarSetBackgrounds(lua_State *l)
{
	loadingBackgrounds.clear();
	
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	const char *str = nullptr;
	int i = 1;
	do {
		lua_pushinteger(l, i++);
		lua_gettable(l, -2);
		str = lua_tostring(l, -1);
		if (str) {
			loadingBackgrounds.push_back(str);
		}
		lua_pop(l, 1);
	} while (str != nullptr);

	return 0;
}

void CalculateItemsToLoad()
{
	ResetItemsToLoad();

	if (loadingBackgrounds.size() > 0) {
		std::string loading_background_string = loadingBackgrounds[rand()%loadingBackgrounds.size()];
		if (CanAccessFile(loading_background_string.c_str())) {
			loadingBackground = CGraphic::New(loading_background_string);
			loadingBackground->Load();
			loadingBackground->Resize(Video.Width, Video.Height);
		}
	}
	
	if (CanAccessFile("ui/loadingEmpty.png") && CanAccessFile("ui/loadingFull.png")) {
		if (CPlayer::GetThisPlayer()) {
			itemsToLoad+= GetCursorsCount();
		}
		itemsToLoad += wyrmgus::icon::get_to_load_count();
		itemsToLoad += GetUnitTypesCount();
		itemsToLoad += GetDecorationsCount();
		itemsToLoad += GetConstructionsCount();
		itemsToLoad += GetMissileSpritesCount();
		
		loadingEmpty = CGraphic::New("ui/loadingEmpty.png");
		loadingEmpty->Load();
		loadingFull = CGraphic::New("ui/loadingFull.png");
		loadingFull->Load();
	}

	if (loadingTips.size() > 0) {
		std::string base_loadingTip = _(loadingTips[rand()%loadingTips.size()].c_str());
		
		int str_width_per_total_width = 1;
		str_width_per_total_width += wyrmgus::defines::get()->get_game_font()->Width(base_loadingTip) / Video.Width;
		
	//	int line_length = Video.Width / GetGameFont().Width(1);
		int line_length = base_loadingTip.size() / str_width_per_total_width;
		
		int begin = 0;
		for (int i = 0; i < str_width_per_total_width; ++i) {
			int end = base_loadingTip.size();
			
			if (i != (str_width_per_total_width - 1)) {
				end = (i + 1) * line_length;
				while (base_loadingTip.substr(end - 1, 1) != " ") {
					end -= 1;
				}
			}
			
			loadingTip.push_back(base_loadingTip.substr(begin, end - begin));
			
			begin = end;
		}
	}
}

void UpdateLoadingBar()
{
	int y = Video.Height/2;
	
	if (loadingBackground != nullptr) {
		loadingBackground->DrawClip(0, 0);
	}
	
	if (itemsToLoad > 0) {
		int x = Video.Width/2 - loadingEmpty->Width/2;
		int pct = (itemsLoaded * 100) / itemsToLoad;
		
		loadingEmpty->DrawClip(x, y - loadingEmpty->Height/2);
		loadingFull->DrawSub(0, 0, (loadingFull->Width * pct) / 100, loadingFull->Height, x, y - loadingEmpty->Height/2);
		y+= loadingEmpty->Height/2;
	}

	if (loadingFont == nullptr) {
		loadingFont = wyrmgus::font::get("game");
	}

	if (loadingFont != nullptr) {
		CLabel label(loadingFont);
		for (size_t i = 0; i < loadingTip.size(); ++i) {
			label.DrawCentered(Video.Width/2, y + 10 + (wyrmgus::defines::get()->get_game_font()->Height() * i), loadingTip[i]);
		}
	}
}

void IncItemsLoaded()
{
	if (itemsToLoad == 0 || itemsLoaded >= itemsToLoad)
		return;

	itemsLoaded++;
}

void ResetItemsToLoad()
{
	itemsLoaded = 0;
	itemsToLoad = 0;
	loadingTip.clear();
	loadingBackground = nullptr;
}

/*----------------------------------------------------------------------------
--  Game creation
----------------------------------------------------------------------------*/


/**
**  CreateGame.
**
**  Load map, graphics, sounds, etc
**
**  @param filename  map filename
**  @param map       map loaded
**
**  @todo FIXME: use in this function InitModules / LoadModules!!!
*/
//Wyrmgus start
//void CreateGame(const std::string &filename, CMap *map)
void CreateGame(const std::string &filename, CMap *map, bool is_mod)
//Wyrmgus end
{
	if (SaveGameLoading) {
		SaveGameLoading = false;
		// Load game, already created game with Init/LoadModules
		CommandLog(nullptr, NoUnitP, FlushCommands, -1, -1, NoUnitP, nullptr, -1);
		return;
	}

	InitPlayers();

	//Wyrmgus start
	if (IsNetworkGame()) { // if is a network game, it is necessary to reinitialize the syncrand variables before beginning to load the map, due to random map generation
		SyncHash = 0;
		wyrmgus::random::get()->reset_seed(true);
	}
	
	const wyrmgus::campaign *current_campaign = wyrmgus::game::get()->get_current_campaign();
	if (current_campaign) {
		wyrmgus::game::get()->set_current_date(current_campaign->get_start_date());
	} else {
		const int year = 1;
		const int month = SyncRand(CDate::months_per_year) + 1;
		const int day = SyncRand(CDate::calendar.daysInMonth(month - 1)) + 1;
		const int hour = SyncRand(CDate::hours_per_day);
		QDate date(year, month, day);;
		QDateTime date_time(date, QTime(hour, 0));
		game::get()->set_current_date(date_time);
	}
	
	CDate::CurrentTotalHours = CDate(wyrmgus::game::get()->get_current_date()).GetTotalHours();

	wyrmgus::age::current_age = nullptr;
	//Wyrmgus end

	if (CMap::Map.Info.Filename.empty() && !filename.empty()) {
		const std::string path = LibraryFileName(filename.c_str());

		if (strcasestr(filename.c_str(), ".smp")) {
			LuaLoadFile(path);
		}
	}

	for (int i = 0; i < PlayerMax; ++i) {
		int playertype = (PlayerTypes) CMap::Map.Info.PlayerType[i];
		// Network games only:
		if (GameSettings.Presets[i].Type != SettingsPresetMapDefault) {
			playertype = GameSettings.Presets[i].Type;
		}
		CreatePlayer(playertype);
	}
	
	//Wyrmgus start
	CalculateItemsToLoad();
	//Wyrmgus end

	if (!filename.empty()) {
		if (CurrentMapPath != filename) {
			CurrentMapPath = filename;
		}

		//
		// Load the map.
		//
		//Wyrmgus start
//		InitUnitTypes(1);
		if (!is_mod) {
			InitUnitTypes(1);
		}
//		LoadMap(filename, *map);
		LoadMap(filename, *map, is_mod);
		//Wyrmgus end
		ApplyUpgrades();
	}
	CclCommand("if (MapLoaded ~= nil) then MapLoaded() end");

	GameCycle = 0;
	FastForwardCycle = 0;
	SyncHash = 0;
	wyrmgus::random::get()->reset_seed(IsNetworkGame());

	if (IsNetworkGame()) { // Prepare network play
		NetworkOnStartGame();
	//Wyrmgus start
	/*
	} else {
		const std::string &localPlayerName = Parameters::Instance.LocalPlayerName;

		if (!localPlayerName.empty() && localPlayerName != "Anonymous") {
			ThisPlayer->SetName(localPlayerName);
		}
	*/
	//Wyrmgus end
	}

	CallbackMusicOn();

#if 0
	GamePaused = true;
#endif

	if (FlagRevealMap) {
		CMap::Map.Reveal();
	}

	//
	// Setup game types
	//
	// FIXME: implement more game types
	if (GameSettings.GameType != SettingsGameTypeMapDefault) {
		switch (GameSettings.GameType) {
			case SettingsGameTypeMelee:
				break;
			case SettingsGameTypeFreeForAll:
				GameTypeFreeForAll();
				break;
			case SettingsGameTypeTopVsBottom:
				GameTypeTopVsBottom();
				break;
			case SettingsGameTypeLeftVsRight:
				GameTypeLeftVsRight();
				break;
			case SettingsGameTypeManVsMachine:
				GameTypeManVsMachine();
				break;
			case SettingsGameTypeManTeamVsMachine:
				GameTypeManTeamVsMachine();

				// Future game type ideas
#if 0
			case SettingsGameTypeOneOnOne:
				break;
			case SettingsGameTypeCaptureTheFlag:
				break;
			case SettingsGameTypeGreed:
				break;
			case SettingsGameTypeSlaughter:
				break;
			case SettingsGameTypeSuddenDeath:
				break;
			case SettingsGameTypeTeamMelee:
				break;
			case SettingsGameTypeTeamCaptureTheFlag:
				break;
#endif
		}
	}

	//
	// Graphic part
	//
	SetPlayersPalette();

	UnitUnderCursor = NoUnitP;

	//Wyrmgus start
	wyrmgus::terrain_type::LoadTerrainTypeGraphics();
	//Wyrmgus end
	wyrmgus::resource_icon::load_all();
	wyrmgus::icon::load_all();
	InitMissileTypes();
#ifndef DYNAMIC_LOAD
	LoadMissileSprites();
#endif
	LoadConstructions();
	LoadUnitTypes();
	LoadDecorations();

	InitUserInterface();
	UI.Load();

	CMap::Map.Init();
	UI.get_minimap()->Create();

	try {
		PreprocessMap();
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to preprocess map."));
	}

	//Wyrmgus start
	//update the sight of all units
	for (CUnit *unit : wyrmgus::unit_manager::get()->get_units()) {
		if (!unit->Destroyed) {
			MapUnmarkUnitSight(*unit);
			UpdateUnitSightRange(*unit);
			MapMarkUnitSight(*unit);
		}
	}
	//Wyrmgus end

	//
	// Sound part
	//
	MapUnitSounds();
	if (SoundEnabled()) {
		InitSoundClient();
	}

	//
	// Init players?
	//
	PlayersInitAi();

	//
	// Upgrades
	//
	InitUpgrades();

	//
	// Buttons (botpanel)
	//
	InitButtons();

	//
	// Triggers
	//
	wyrmgus::trigger::InitActiveTriggers();
	
#if 0
	if (!UI.SelectedViewport) {
		UI.SelectedViewport = UI.Viewports;
	}
#endif
	if (CPlayer::GetThisPlayer()->StartMapLayer < static_cast<int>(CMap::Map.MapLayers.size())) {
		UI.CurrentMapLayer = CMap::Map.MapLayers[CPlayer::GetThisPlayer()->StartMapLayer].get();
	}
	UI.SelectedViewport->Center(CMap::Map.tile_pos_to_scaled_map_pixel_pos_center(CPlayer::GetThisPlayer()->StartPos));

	//
	// Various hacks which must be done after the map is loaded.
	//
	// FIXME: must be done after map is loaded
	InitPathfinder();

	GameResult = GameNoResult;

	CommandLog(nullptr, NoUnitP, FlushCommands, -1, -1, NoUnitP, nullptr, -1);
	Video.ClearScreen();
	
	//Wyrmgus start
	ResetItemsToLoad();
	//Wyrmgus end
}

/**
**  Init Game Setting to default values
**
**  @todo  FIXME: this should not be executed for restart levels!
*/
void InitSettings()
{
	for (int i = 0; i < PlayerMax; ++i) {
		GameSettings.Presets[i].AIScript = "ai-passive";
		GameSettings.Presets[i].Race = SettingsPresetMapDefault;
		GameSettings.Presets[i].Team = SettingsPresetMapDefault;
		GameSettings.Presets[i].Type = SettingsPresetMapDefault;
	}
	GameSettings.Resources = SettingsPresetMapDefault;
	GameSettings.NumUnits = SettingsPresetMapDefault;
	GameSettings.Opponents = SettingsPresetMapDefault;
	GameSettings.Difficulty = SettingsPresetMapDefault;
	GameSettings.GameType = SettingsPresetMapDefault;
	GameSettings.MapRichness = SettingsPresetMapDefault;
	GameSettings.NetGameType = SettingsSinglePlayerGame;
}

// call the lua function: CleanGame_Lua.
static void CleanGame_Lua()
{
	lua_getglobal(Lua, "CleanGame_Lua");
	if (lua_isfunction(Lua, -1)) {
		LuaCall(0, 1);
	} else {
		lua_pop(Lua, 1);
	}
}

/**
**  Cleanup game.
**
**  Call each module to clean up.
**  Contrary to CleanModules, maps can be restarted
**  without reloading all lua files.
*/
void CleanGame()
{
	EndReplayLog();
	CleanMessages();

	CleanGame_Lua();
	wyrmgus::trigger::ClearActiveTriggers();
	wyrmgus::game::get()->clear_delayed_effects();
	CleanAi();
	CleanGroups();
	CleanMissiles();
	CleanUnits();
	CleanSelections();
	//Wyrmgus start
	DisableMod(CMap::Map.Info.Filename);
	//Wyrmgus end
	CMap::Map.Clean();
	CleanReplayLog();
	FreePathfinder();
	CursorBuilding = nullptr;
	UnitUnderCursor = nullptr;
	GameEstablishing = false;
}

/**
**  Return of game name.
**
**  @param l  Lua state.
*/
static int CclSetGameName(lua_State *l)
{
	const int args = lua_gettop(l);
	if (args > 1 || (args == 1 && (!lua_isnil(l, 1) && !lua_isstring(l, 1)))) {
		LuaError(l, "incorrect argument");
	}
	if (args == 1 && !lua_isnil(l, 1)) {
		GameName = lua_tostring(l, 1);
	}

	if (!GameName.empty()) {
		std::string path = Parameters::Instance.GetUserDirectory() + "/" + GameName;
		makedir(path.c_str(), 0777);
	}
	return 0;
}

static int CclSetFullGameName(lua_State *l)
{
	const int args = lua_gettop(l);
	if (args > 1 || (args == 1 && (!lua_isnil(l, 1) && !lua_isstring(l, 1)))) {
		LuaError(l, "incorrect argument");
	}
	if (args == 1 && !lua_isnil(l, 1)) {
		FullGameName = lua_tostring(l, 1);
	}
	return 0;
}

/**
**  Set God mode.
**
**  @param l  Lua state.
**
**  @return   The old mode.
*/
static int CclSetGodMode(lua_State *l)
{
	LuaCheckArgs(l, 1);
	GodMode = LuaToBoolean(l, 1);
	return 0;
}

/**
**  Get God mode.
**
**  @param l  Lua state.
**
**  @return   God mode.
*/
static int CclGetGodMode(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushboolean(l, GodMode);
	return 1;
}

/**
**  Set resource harvesting speed (deprecated).
**
**  @param l  Lua state.
*/
static int CclSetSpeedResourcesHarvest(lua_State *l)
{
	LuaCheckArgs(l, 3);

	const int player = LuaToNumber(l, 1);
	const std::string resource = LuaToString(l, 2);
	const int resId = GetResourceIdByName(l, resource.c_str());

	CPlayer::Players[player]->SpeedResourcesHarvest[resId] = LuaToNumber(l, 3);
	return 0;
}

/**
**  Set resource returning speed (deprecated).
**
**  @param l  Lua state.
*/
static int CclSetSpeedResourcesReturn(lua_State *l)
{
	LuaCheckArgs(l, 3);

	const int player = LuaToNumber(l, 1);
	const std::string resource = LuaToString(l, 2);
	const int resId = GetResourceIdByName(l, resource.c_str());

	CPlayer::Players[player]->SpeedResourcesReturn[resId] = LuaToNumber(l, 3);
	return 0;
}

/**
**  Set building speed (deprecated).
**
**  @param l  Lua state.
*/
static int CclSetSpeedBuild(lua_State *l)
{
	LuaCheckArgs(l, 2);
	const int player = LuaToNumber(l, 1);
	CPlayer::Players[player]->SpeedBuild = LuaToNumber(l, 2);
	return 0;
}

/**
**  Get building speed (deprecated).
**
**  @param l  Lua state.
**
**  @return   Building speed.
*/
static int CclGetSpeedBuild(lua_State *l)
{
	LuaCheckArgs(l, 1);
	const int player = LuaToNumber(l, 1);
	lua_pushnumber(l, CPlayer::Players[player]->SpeedBuild);
	return 1;
}

/**
**  Set training speed (deprecated).
**
**  @param l  Lua state.
*/
static int CclSetSpeedTrain(lua_State *l)
{
	LuaCheckArgs(l, 2);
	const int player = LuaToNumber(l, 1);
	CPlayer::Players[player]->SpeedTrain = LuaToNumber(l, 2);
	return 0;
}

/**
**  Get training speed (deprecated).
**
**  @param l  Lua state.
**
**  @return   Training speed.
*/
static int CclGetSpeedTrain(lua_State *l)
{
	LuaCheckArgs(l, 1);
	const int player = LuaToNumber(l, 1);
	lua_pushnumber(l, CPlayer::Players[player]->SpeedTrain);
	return 1;
}

/**
**  For debug increase upgrading speed (deprecated).
**
**  @param l  Lua state.
*/
static int CclSetSpeedUpgrade(lua_State *l)
{
	LuaCheckArgs(l, 2);
	const int player = LuaToNumber(l, 1);
	CPlayer::Players[player]->SpeedUpgrade = LuaToNumber(l, 2);

	lua_pushnumber(l, CPlayer::Players[player]->SpeedUpgrade);
	return 1;
}

/**
**  For debug increase researching speed (deprecated).
**
**  @param l  Lua state.
*/
static int CclSetSpeedResearch(lua_State *l)
{
	LuaCheckArgs(l, 2);
	const int player = LuaToNumber(l, 1);
	CPlayer::Players[player]->SpeedResearch = LuaToNumber(l, 2);

	lua_pushnumber(l, CPlayer::Players[player]->SpeedResearch);
	return 1;
}

/**
**  For debug increase all speeds (deprecated).
**
**  @param l  Lua state.
*/
static int CclSetSpeeds(lua_State *l)
{
	LuaCheckArgs(l, 1);
	const int speed = LuaToNumber(l, 1);
	for (int i = 0; i < PlayerMax; ++i) {
		for (int j = 0; j < MaxCosts; ++j) {
			CPlayer::Players[i]->SpeedResourcesHarvest[j] = speed;
			CPlayer::Players[i]->SpeedResourcesReturn[j] = speed;
		}
		CPlayer::Players[i]->SpeedBuild = CPlayer::Players[i]->SpeedTrain = CPlayer::Players[i]->SpeedUpgrade = CPlayer::Players[i]->SpeedResearch = speed;
	}

	lua_pushnumber(l, speed);
	return 1;
}

/**
**  Define a resource.
**
**  @param l  Lua state.
*/
static int CclDefineResource(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string resource_ident = LuaToString(l, 1);
	int resource_id = GetResourceIdByName(resource_ident.c_str());
	if (resource_id == -1) {
		LuaError(l, "Resource \"%s\" doesn't exist." _C_ resource_ident.c_str());
	}
	wyrmgus::resource *resource = wyrmgus::resource::get_all()[resource_id];
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			resource->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "DefaultIncome")) {
			resource->default_income = LuaToNumber(l, -1);
		} else if (!strcmp(value, "DefaultAmount")) {
			resource->default_amount = LuaToNumber(l, -1);
		} else if (!strcmp(value, "DefaultMaxAmount")) {
			resource->DefaultMaxAmount = LuaToNumber(l, -1);
		} else if (!strcmp(value, "ActionName")) {
			resource->action_name = LuaToString(l, -1);
		} else if (!strcmp(value, "FinalResource")) {
			const std::string final_resource_ident = LuaToString(l, -1);
			resource->final_resource = wyrmgus::resource::get(final_resource_ident);
		} else if (!strcmp(value, "FinalResourceConversionRate")) {
			resource->final_resource_conversion_rate = LuaToNumber(l, -1);
		} else if (!strcmp(value, "LuxuryResource")) {
			resource->LuxuryResource = LuaToBoolean(l, -1);
			LuxuryResources.push_back(resource_id);
		} else if (!strcmp(value, "BasePrice")) {
			resource->base_price = LuaToNumber(l, -1);
		} else if (!strcmp(value, "DemandElasticity")) {
			resource->DemandElasticity = LuaToNumber(l, -1);
		} else if (!strcmp(value, "InputResource")) {
			std::string input_resource_ident = LuaToString(l, -1);
			int input_resource_id = GetResourceIdByName(input_resource_ident.c_str());
			if (input_resource_id == -1) {
				LuaError(l, "Resource \"%s\" doesn't exist." _C_ input_resource_ident.c_str());
			}
			resource->InputResource = input_resource_id;
		} else if (!strcmp(value, "Hidden")) {
			resource->Hidden = LuaToBoolean(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define default names for the resources.
**
**  @param l  Lua state.
*/
static int CclDefineDefaultResourceNames(lua_State *l)
{
	for (unsigned int i = 0; i < MaxCosts; ++i) {
		DefaultResourceNames[i].clear();
	}

	const unsigned int args = lua_gettop(l);
	for (unsigned int i = 0; i < MaxCosts && i < args; ++i) {
		DefaultResourceNames[i] = LuaToString(l, i + 1);
		
		wyrmgus::resource *resource = wyrmgus::resource::get_or_add(DefaultResourceNames[i], nullptr);
		resource->index = i;
	}

	wyrmgus::resource::sort_instances([](const wyrmgus::resource *a, const wyrmgus::resource *b) {
		return a->get_index() < b->get_index();
	});

	return 0;
}

/**
**  Affect UseHPForXp.
**
**  @param l  Lua state.
**
**  @return 0.
*/
static int ScriptSetUseHPForXp(lua_State *l)
{
	LuaCheckArgs(l, 1);
	UseHPForXp = LuaToBoolean(l, 1);
	return 0;
}

/**
**  Set the local player name
**
**  @param l  Lua state.
*/
static int CclSetLocalPlayerName(lua_State *l)
{
	LuaCheckArgs(l, 1);
	Parameters::Instance.LocalPlayerName = LuaToString(l, 1);
	return 0;
}

/**
**  Get the local player name
**
**  @param l  Lua state.
*/
static int CclGetLocalPlayerName(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushstring(l, Parameters::Instance.LocalPlayerName.c_str());
	return 1;
}

/**
**  Get Stratagus Version
*/
static int CclGetStratagusVersion(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushstring(l, VERSION);
	return 1;
}

/**
**  Get Stratagus Homepage
*/
static int CclGetStratagusHomepage(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushstring(l, HOMEPAGE);
	return 1;
}

static int CclSetMenuRace(lua_State *l)
{
	LuaCheckArgs(l, 1);
	MenuRace = LuaToString(l, 1);
	return 0;
}

/**
**  Load the SavedGameInfo Header
**
**  @param l  Lua state.
*/
static int CclSavedGameInfo(lua_State *l)
{
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}

	for (lua_pushnil(l); lua_next(l, 1); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);

		if (!strcmp(value, "SaveFile")) {
			CurrentMapPath = LuaToString(l, -1);
			std::string buf = wyrmgus::database::get()->get_root_path().string();
			buf += "/";
			buf += LuaToString(l, -1);
			if (LuaLoadFile(buf) == -1) {
				DebugPrint("Load failed: %s\n" _C_ value);
			}
		} else if (!strcmp(value, "SyncHash")) {
			SyncHash = LuaToNumber(l, -1);
		} else if (!strcmp(value, "SyncRandSeed")) {
			wyrmgus::random::get()->set_seed(LuaToNumber(l, -1));
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	return 0;
}

void LuaRegisterModules()
{
	lua_register(Lua, "SetGameName", CclSetGameName);
	lua_register(Lua, "SetFullGameName", CclSetFullGameName);

	lua_register(Lua, "SetGodMode", CclSetGodMode);
	lua_register(Lua, "GetGodMode", CclGetGodMode);

	lua_register(Lua, "SetSpeedResourcesHarvest", CclSetSpeedResourcesHarvest);
	lua_register(Lua, "SetSpeedResourcesReturn", CclSetSpeedResourcesReturn);
	lua_register(Lua, "SetSpeedBuild", CclSetSpeedBuild);
	lua_register(Lua, "GetSpeedBuild", CclGetSpeedBuild);
	lua_register(Lua, "SetSpeedTrain", CclSetSpeedTrain);
	lua_register(Lua, "GetSpeedTrain", CclGetSpeedTrain);
	lua_register(Lua, "SetSpeedUpgrade", CclSetSpeedUpgrade);
	lua_register(Lua, "SetSpeedResearch", CclSetSpeedResearch);
	lua_register(Lua, "SetSpeeds", CclSetSpeeds);

	lua_register(Lua, "DefineResource", CclDefineResource);
	lua_register(Lua, "DefineDefaultResourceNames", CclDefineDefaultResourceNames);

	lua_register(Lua, "SetUseHPForXp", ScriptSetUseHPForXp);
	lua_register(Lua, "SetLocalPlayerName", CclSetLocalPlayerName);
	lua_register(Lua, "GetLocalPlayerName", CclGetLocalPlayerName);

	lua_register(Lua, "SetMenuRace", CclSetMenuRace);

	lua_register(Lua, "GetStratagusVersion", CclGetStratagusVersion);
	lua_register(Lua, "GetStratagusHomepage", CclGetStratagusHomepage);

	lua_register(Lua, "SavedGameInfo", CclSavedGameInfo);

	//Wyrmgus start
	lua_register(Lua, "LoadingBarSetTips", CclLoadingBarSetTips);
	lua_register(Lua, "LoadingBarSetBackgrounds", CclLoadingBarSetBackgrounds);
	//Wyrmgus end

	AiCclRegister();
	AnimationCclRegister();
	CharacterCclRegister();
	DecorationCclRegister();
	DependenciesCclRegister();
	EditorCclRegister();
	//Wyrmgus start
	GrandStrategyCclRegister();
	//Wyrmgus end
	GroupCclRegister();
	ItemCclRegister();
	LiteraryTextCclRegister();
	MapCclRegister();
	MissileCclRegister();
	NetworkCclRegister();
	PathfinderCclRegister();
	PlayerCclRegister();
	//Wyrmgus start
	ProvinceCclRegister();
	QuestCclRegister();
	//Wyrmgus end
	ReplayCclRegister();
	ScriptRegister();
	SelectionCclRegister();
	SoundCclRegister();
	SpellCclRegister();
	TriggerCclRegister();
	UnitCclRegister();
	UnitTypeCclRegister();
	UpgradesCclRegister();
	UserInterfaceCclRegister();
	VideoCclRegister();
}
