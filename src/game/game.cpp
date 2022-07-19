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
//      (c) Copyright 1998-2022 by Lutz Sammer, Andreas Arens,
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

#include "stratagus.h"

#include "game/game.h"

#include "actions.h"
#include "age.h"
#include "ai.h"
#include "animation/animation.h"
//Wyrmgus start
#include "character.h"
//Wyrmgus end
#include "commands.h"
#include "database/database.h"
#include "database/defines.h"
#include "database/gsml_data.h"
#include "database/gsml_parser.h"
#include "dialogue.h"
#include "economy/resource.h"
#include "editor.h"
#include "engine_interface.h"
#include "game/results_info.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "iocompat.h"
#include "iolib.h"
#include "item/persistent_item.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/map_presets.h"
#include "map/map_settings.h"
#include "map/minimap.h"
#include "map/region.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/world.h"
#include "map/world_game_data.h"
#include "missile.h"
#include "network/netconnect.h"
#include "network/network.h"
#include "parameters.h"
#include "pathfinder/pathfinder.h"
#include "player/civilization.h"
#include "player/diplomacy_state.h"
#include "player/faction.h"
#include "player/faction_type.h"
#include "player/player.h"
#include "player/player_color.h"
#include "player/player_type.h"
//Wyrmgus start
#include "province.h"
//Wyrmgus end
#include "quest/campaign.h"
//Wyrmgus start
#include "quest/quest.h"
//Wyrmgus end
#include "replay.h"
#include "results.h"
//Wyrmgus start
#include "script.h"
//Wyrmgus end
#include "script/condition/and_condition.h"
#include "script/condition/condition.h"
#include "script/effect/delayed_effect_instance.h"
#include "script/trigger.h"
#include "settings.h"
#include "sound/music_player.h"
#include "sound/music_type.h"
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
#include "unit/build_restriction/on_top_build_restriction.h"
#include "unit/construction.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "util/assert_util.h"
#include "util/date_util.h"
#include "util/event_loop.h"
#include "util/exception_util.h"
#include "util/log_util.h"
#include "util/path_util.h"
#include "util/random.h"
#include "util/string_conversion_util.h"
#include "util/string_util.h"
#include "util/thread_pool.h"
//Wyrmgus start
#include "util/util.h"
//Wyrmgus end
#include "util/vector_random_util.h"
#include "util/vector_util.h"
#include "version.h"
#include "video/font.h"
#include "video/video.h"
#include "widgets.h"

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

bool GameRunning = false;				/// Current running state
bool GameObserve = false;				/// Observe mode
bool GameEstablishing = false;			/// Game establishing mode

static std::vector<std::unique_ptr<gcn::Container>> Containers;

namespace wyrmgus {

std::filesystem::path game::save_file_url_string_to_save_filepath(const std::string &file_url_str)
{
	const QUrl file_url = QString::fromStdString(file_url_str);
	QString filepath_qstr = file_url.toLocalFile();

	//correct issue where the QML mistakenly sends us a ".gz" file ending instead of ".sav.gz", or if it sends a file with no extension at all
	if (!filepath_qstr.endsWith(".sav.gz")) {
		if (filepath_qstr.endsWith(".sav")) {
			filepath_qstr += ".gz";
		} else {
			if (filepath_qstr.endsWith(".gz")) {
				filepath_qstr = filepath_qstr.left(filepath_qstr.size() - 3);
			}

			filepath_qstr += ".sav.gz";
		}
	}

	return path::from_qstring(filepath_qstr);
}

game::game()
{
}

game::~game()
{
}

void game::on_started()
{
	if (GameCycle == 0) { //so that this doesn't trigger when loading a saved game
		if (this->get_current_campaign() != nullptr) {
			this->apply_player_history();
		}

		for (const CUpgrade *starting_upgrade : CMap::get()->get_settings()->get_starting_upgrades()) {
			for (const qunique_ptr<CPlayer> &player : CPlayer::Players) {
				if (player->get_type() == player_type::nobody) {
					continue;
				}

				if (UpgradeIdAllowed(*player, starting_upgrade->ID) != 'R') {
					player->acquire_upgrade(starting_upgrade);
				}
			}
		}
	}

	//update the sold units of all units before starting, to make sure they fit the current conditions
	//make a copy of the units list, as updating the sold units can change the list
	const std::vector<CUnit *> units = unit_manager::get()->get_units();
	for (CUnit *unit : units) {
		if (!unit->IsAlive()) {
			continue;
		}

		unit->UpdateSoldUnits();
		unit->update_home_settlement();
	}

	for (size_t z = 0; z < CMap::get()->MapLayers.size(); ++z) {
		UI.get_minimap()->update_exploration(z);
	}
}

void game::set_paused(const bool paused)
{
	if (paused == this->is_paused()) {
		return;
	}

	if (paused && this->is_multiplayer()) {
		//cannot pause the game during multiplayer
		return;
	}

	//Wyrmgus start
	KeyScrollState = MouseScrollState = ScrollNone;
	//Wyrmgus end

	this->paused = paused;

	emit paused_changed();
}

int game::get_cycles_per_year() const
{
	return defines::get()->get_cycles_per_year(this->get_current_year());
}

boost::asio::awaitable<void> game::run_map(const std::filesystem::path &filepath)
{
	engine_interface::get()->set_loading_message("Starting Game...");

	CclCommand("if (LoadedGame == false) then ClearPlayerDataObjectives(); SetDefaultPlayerDataObjectives(); end");

	CleanPlayers();

	while (true) {
		music_player::get()->play_music_type(music_type::loading);

		CclCommand("InitGameVariables();");

		co_await StartMap(filepath, true);

		if (GameResult != GameRestart) {
			break;
		}
	}

	GameSettings.reset();

	this->set_current_campaign(nullptr);
	CurrentQuest = nullptr;
}

void game::run_map_async(const QString &filepath)
{
	event_loop::get()->co_spawn([this, filepath]() -> boost::asio::awaitable<void> {
		co_await this->run_map(path::from_qstring(filepath));
	});
}

void game::run_campaign_async(campaign *campaign)
{
	event_loop::get()->co_spawn([this, campaign]() -> boost::asio::awaitable<void> {
		if (this->get_current_campaign() != nullptr) {
			//already running
			co_return;
		}

		this->set_current_campaign(campaign);
		co_await this->run_map(database::get()->get_campaign_map_filepath());
	});
}

void game::apply_player_history()
{
	const CDate start_date = this->get_current_campaign()->get_start_date();

	for (const qunique_ptr<CPlayer> &player : CPlayer::Players) {
		if (player->get_type() == player_type::nobody || player->get_civilization() == nullptr || player->get_faction() == nullptr) {
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
	try {
		if (GameCycle % this->get_cycles_per_year() == 0) {
			this->increment_current_year();
		}

		if (GameCycle % CYCLES_PER_IN_GAME_HOUR == 0) {
			this->increment_current_total_hours();

			for (const std::unique_ptr<CMapLayer> &map_layer : CMap::get()->MapLayers) {
				map_layer->DoPerHourLoop();
			}

			for (const world *world : world::get_all()) {
				world_game_data *game_data = world->get_game_data();
				if (game_data->is_on_map()) {
					game_data->do_per_in_game_hour_loop();
				}
			}
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error executing the per cycle actions for the game."));
	}
}

void game::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "current_campaign") {
		this->current_campaign = campaign::get(value);
	} else if (key == "current_year") {
		this->current_year = std::stoi(value);
	} else if (key == "cheat") {
		this->cheat = string::to_bool(value);
	} else {
		throw std::runtime_error("Invalid game data property: \"" + key + "\".");
	}
}

void game::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "player_delayed_effects") {
		scope.for_each_child([&](const gsml_data &delayed_effect_data) {
			auto delayed_effect = std::make_unique<delayed_effect_instance<CPlayer>>();
			database::process_gsml_data(delayed_effect, delayed_effect_data);
			this->add_delayed_effect(std::move(delayed_effect));
		});
	} else if (tag == "unit_delayed_effects") {
		scope.for_each_child([&](const gsml_data &delayed_effect_data) {
			auto delayed_effect = std::make_unique<delayed_effect_instance<CUnit>>();
			database::process_gsml_data(delayed_effect, delayed_effect_data);
			this->add_delayed_effect(std::move(delayed_effect));
		});
	} else if (tag == "site_data") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const site *site = site::get(child_scope.get_tag());
			database::process_gsml_data(site->get_game_data(), child_scope);
		});
	} else {
		throw std::runtime_error("Invalid game data scope: \"" + scope.get_tag() + "\".");
	}
}

void game::save(const std::filesystem::path &filepath) const
{
	const std::string filepath_str = path::to_string(filepath);

	CFile file;

	if (file.open(filepath_str.c_str(), CL_WRITE_GZ | CL_OPEN_WRITE) == -1) {
		throw std::runtime_error("Can't save to \"" + filepath_str + "\".");
	}

	time_t now;
	char dateStr[64];

	time(&now);
	const struct tm *timeinfo = localtime(&now);
	strftime(dateStr, sizeof(dateStr), "%c", timeinfo);

	// Load initial level // Without units
	file.printf("local oldCreateUnit = CreateUnit\n");
	file.printf("local oldSetResourcesHeld = SetResourcesHeld\n");
	file.printf("local oldSetTile = SetTile\n");
	file.printf("function CreateUnit() end\n");
	file.printf("function SetResourcesHeld() end\n");
	file.printf("function SetTileTerrain() end\n");
	file.printf("Load(\"%s\")\n", string::escaped(path::to_string(CMap::get()->Info->get_setup_filepath())).c_str());
	file.printf("CreateUnit = oldCreateUnit\n");
	file.printf("SetResourcesHeld = oldSetResourcesHeld\n");
	file.printf("SetTile = oldSetTile\n");
	//
	// Parseable header
	//
	file.printf("SavedGameInfo({\n");
	file.printf("---  \"comment\", \"Generated by %s version %s\",\n", QApplication::applicationName().toStdString().c_str(), QApplication::applicationVersion().toStdString().c_str());
	file.printf("---  \"comment\", \"Visit " HOMEPAGE " for more information\",\n");
	file.printf("---  \"type\",    \"%s\",\n", "single-player");
	file.printf("---  \"date\",    \"%s\",\n", dateStr);
	file.printf("---  \"map\",     \"%s\",\n", CMap::get()->Info->get_name().c_str());
	file.printf("---  \"media-version\", \"%s\"", "Undefined");
	file.printf("---  \"engine\",  {%d, %d, %d},\n",
		StratagusMajorVersion, StratagusMinorVersion, StratagusPatchLevel);
	file.printf("  SyncHash = %d, \n", SyncHash);
	file.printf("  SyncRandSeed = %d, \n", random::get()->get_seed());
	file.printf("  SaveFile = \"%s\"\n", string::escaped(path::to_string(CurrentMapPath)).c_str());
	file.printf("} )\n\n");

	// FIXME: probably not the right place for this
	file.printf("GameCycle = %lu\n", GameCycle);
	file.printf("SetCurrentTotalHours(%" PRIu64 ")\n", this->get_current_total_hours());

	file.printf("SetGodMode(%s)\n", GodMode ? "true" : "false");

	SaveUnitTypes(file);
	SaveUpgrades(file);
	SavePlayers(file);
	CMap::get()->save(file);
	unit_manager::get()->Save(file);
	SaveUserInterface(file);
	SaveAi(file);
	SaveSelections(file);
	SaveGroups(file);
	SaveMissiles(file);
	SaveReplayList(file);
	SaveGameSettings(file);
	// FIXME: find all state information which must be saved.
	const std::string s = SaveGlobal(Lua);
	if (!s.empty()) {
		file.printf("-- Lua state\n\n %s\n", s.c_str());
	}
	SaveTriggers(file); //Triggers are saved in SaveGlobal, so load it after Global

	file.close();
}

void game::save_game_data(CFile &file) const
{
	gsml_data game_data;

	if (this->get_current_campaign() != nullptr) {
		game_data.add_property("current_campaign", this->get_current_campaign()->get_identifier());
	}

	game_data.add_property("current_year", std::to_string(this->get_current_year()));

	if (this->cheat) {
		game_data.add_property("cheat", string::from_bool(this->cheat));
	}

	if (!this->player_delayed_effects.empty()) {
		gsml_data delayed_effects_data("player_delayed_effects");
		for (const auto &delayed_effect : this->player_delayed_effects) {
			delayed_effects_data.add_child(delayed_effect->to_gsml_data());
		}
		game_data.add_child(std::move(delayed_effects_data));
	}

	if (!this->unit_delayed_effects.empty()) {
		gsml_data delayed_effects_data("unit_delayed_effects");
		for (const auto &delayed_effect : this->unit_delayed_effects) {
			delayed_effects_data.add_child(delayed_effect->to_gsml_data());
		}
		game_data.add_child(std::move(delayed_effects_data));
	}

	gsml_data site_game_data("site_data");
	for (const site *site : site::get_all()) {
		if (site->get_game_data() == nullptr) {
			continue;
		}

		gsml_data site_data = site->get_game_data()->to_gsml_data();

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

void game::set_cheat(const bool cheat)
{
	if (cheat == this->cheat) {
		return;
	}

	this->cheat = cheat;

	if (cheat) {
		CPlayer::GetThisPlayer()->Notify("%s", _("Cheat used, persistent data will no longer be updated during this game."));
	}
}

bool game::is_persistency_enabled() const
{
	return !this->is_multiplayer() && !this->cheat;
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

void game::update_neutral_faction_presence()
{
	try {
		if (this->get_current_campaign() == nullptr) {
			//only dynamically generate/remove neutral faction buildings in the campaign mode
			return;
		}

		const std::vector<const faction *> neutral_factions = vector::shuffled(faction::get_neutral_factions());

		for (const faction *faction : neutral_factions) {
			this->do_neutral_faction_contraction(faction);
			this->do_neutral_faction_expansion(faction);
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error updating the neutral faction presence."));
	}
}

void game::do_neutral_faction_expansion(const faction *faction)
{
	assert_throw(faction->has_neutral_type());

	CPlayer *faction_player = GetFactionPlayer(faction);

	if (faction_player != nullptr && faction->get_max_neutral_buildings() != 0 && faction_player->NumBuildings >= faction->get_max_neutral_buildings()) {
		return;
	}

	std::vector<std::pair<const unit_type *, const on_top_build_restriction *>> building_types;

	for (const unit_class *building_class : faction->get_neutral_building_classes()) {
		const unit_type *building_type = faction->get_class_unit_type(building_class);

		if (building_type == nullptr) {
			continue;
		}

		const on_top_build_restriction *ontop = OnTopDetails(*building_type, nullptr);

		if (ontop == nullptr) {
			continue;
		}

		building_types.push_back({ building_type, ontop });
	}

	if (building_types.empty()) {
		return;
	}

	//shuffle the vector, so that we don't privilege a particular building type, if there are multiple ones which can target the same building site
	vector::shuffle(building_types);

	const std::vector<const site *> target_sites = vector::shuffled(faction->get_all_neutral_target_sites());

	int placed_buildings = 0;

	if (faction_player != nullptr) {
		placed_buildings += faction_player->NumBuildings;
	}

	for (const site *target_site : target_sites) {
		const site_game_data *site_game_data = target_site->get_game_data();

		if (site_game_data->get_site_unit() == nullptr) {
			continue;
		}

		for (const auto &[building_type, ontop] : building_types) {
			if (site_game_data->get_site_unit()->Type != ontop->Parent) {
				continue;
			}

			//give some random chance of not creating the neutral building
			if (random::get()->generate(4) != 0) {
				continue;
			}

			if (faction->get_neutral_site_conditions() != nullptr && !faction->get_neutral_site_conditions()->check(site_game_data->get_site_unit(), read_only_context::from_scope(site_game_data->get_site_unit()))) {
				continue;
			}

			if (faction->get_neutral_site_spawn_conditions() != nullptr && !faction->get_neutral_site_spawn_conditions()->check(site_game_data->get_site_unit(), read_only_context::from_scope(site_game_data->get_site_unit()))) {
				continue;
			}

			if (faction_player == nullptr) {
				faction_player = GetOrAddFactionPlayer(faction);
			}

			CreateUnit(site_game_data->get_site_unit()->tilePos, *building_type, faction_player, site_game_data->get_site_unit()->MapLayer->ID, false, nullptr, false);

			++placed_buildings;
			vector::shuffle(building_types);
			break;
		}

		if (faction->get_max_neutral_buildings() != 0 && placed_buildings >= faction->get_max_neutral_buildings()) {
			break;
		}
	}
}

void game::do_neutral_faction_contraction(const faction *faction)
{
	assert_throw(faction->has_neutral_type());

	if (faction->get_neutral_site_conditions() == nullptr) {
		return;
	}

	CPlayer *faction_player = GetFactionPlayer(faction);

	if (faction_player == nullptr) {
		return;
	}

	std::vector<CUnit *> neutral_buildings;

	for (const unit_class *building_class : faction->get_neutral_building_classes()) {
		vector::merge(neutral_buildings, faction_player->get_class_units(building_class));
	}

	vector::shuffle(neutral_buildings);

	for (CUnit *building : neutral_buildings) {
		if (building->get_site() == nullptr) {
			continue;
		}

		if (!faction->get_neutral_site_conditions()->check(building, read_only_context::from_scope(building))) {
			LetUnitDie(*building);
		}
	}
}


void game::set_results(qunique_ptr<results_info> &&results)
{
	this->results = std::move(results);
	emit results_changed();
}

void game::store_results()
{
	std::vector<qunique_ptr<player_results_info>> player_results;

	for (const CPlayer *player : CPlayer::get_non_neutral_players()) {
		if (player->TotalUnits == 0) {
			continue;
		}

		if (player->get_type() == player_type::rescue_passive) {
			continue;
		}

		if (player->get_type() == player_type::rescue_active) {
			continue;
		}

		std::optional<diplomacy_state> diplomacy_state;

		if (player != CPlayer::GetThisPlayer()) {
			if (CPlayer::GetThisPlayer()->is_allied_with(*player)) {
				diplomacy_state = diplomacy_state::allied;
			} else if (CPlayer::GetThisPlayer()->is_enemy_of(*player)) {
				diplomacy_state = diplomacy_state::enemy;
			} else {
				diplomacy_state = diplomacy_state::neutral;
			}
		}

		player_results.push_back(make_qunique<player_results_info>(player->get_name(), diplomacy_state, player->TotalUnits, player->TotalBuildings, player->get_resource_totals(), player->TotalKills, player->TotalRazings));
	}

	auto results = make_qunique<results_info>(GameResult, std::move(player_results));

	if (QApplication::instance()->thread() != QThread::currentThread()) {
		results->moveToThread(QApplication::instance()->thread());
	}

	this->set_results(std::move(results));
}

void game::clear_results()
{
	this->set_results(nullptr);
}

}

void load_game_data(const std::string &gsml_string)
{
	gsml_parser parser;
	database::process_gsml_data(game::get(), parser.parse(gsml_string));
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
		file.printf("GameSettings.Presets[%d].Type = %d\n", i, static_cast<int>(GameSettings.Presets[i].Type));
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
	file.printf("GameSettings.TechLevel = %d\n", GameSettings.TechLevel);
	file.printf("GameSettings.MaxTechLevel = %d\n", GameSettings.MaxTechLevel);
	file.printf("\n");
}

boost::asio::awaitable<void> StartMap(const std::filesystem::path &filepath, const bool clean)
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
		DebugPrint("Creating game with map: %s\n" _C_ path::to_string(filepath).c_str());
		if (clean) {
			CleanPlayers();
		}

		GameEstablishing = true;

		//create the game in another thread, to not block the main one while it is loading
		co_await thread_pool::get()->co_spawn_awaitable([&filepath]() -> boost::asio::awaitable<void> {
			CreateGame(filepath, CMap::get());
			co_return;
		});

		//Wyrmgus start
	//	UI.StatusLine.Set(NameLine);
		//Wyrmgus end

		for (CPlayer *player : CPlayer::get_non_neutral_players()) {
			//update the quest pool for all players
			player->update_quest_pool();

			//recalculate the military score when starting the game
			player->calculate_military_score();
		}

		if (CPlayer::GetThisPlayer()->StartMapLayer < static_cast<int>(CMap::get()->MapLayers.size())) {
			UI.CurrentMapLayer = CMap::get()->MapLayers[CPlayer::GetThisPlayer()->StartMapLayer].get();
		}
		UI.SelectedViewport->Center(CMap::get()->tile_pos_to_scaled_map_pixel_pos_center(CPlayer::GetThisPlayer()->StartPos));

		UI.get_minimap()->Update();

		//  Play the game.
		co_await GameMainLoop();

		//  Clear screen
		Video.ClearScreen();

		CleanGame();
		current_interface_state = interface_state::menu;

		Gui->setTop(oldTop);
		vector::remove(Containers, container);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error running map \"" + path::to_string(filepath) + "\"."));
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
*/
static void LoadStratagusMap(const std::filesystem::path &smp_path, const std::filesystem::path &map_path)
{
	CFile file;

	if (LcmPreventRecurse) {
		throw std::runtime_error("Recursive use of load map!");
	}

	InitPlayers();

	LcmPreventRecurse = 1;
	if (LuaLoadFile(map_path.string().c_str()) == -1) {
		throw std::runtime_error("Can't load lua file: \"" + path::to_string(map_path) + "\".");
	}
	LcmPreventRecurse = 0;

#if 0
	// Not true if multiplayer levels!
	if (!ThisPlayer) { /// ARI: bomb if nothing was loaded!
		throw std::runtime_error(mapname + ": invalid map");
	}
#endif

	if (!CMap::get()->Info->get_map_width() || !CMap::get()->Info->get_map_height()) {
		throw std::runtime_error(path::to_string(map_path) + ": invalid map");
	}

	CMap::get()->Info->set_presentation_filepath(smp_path);
}

// Write the map presentation file
static int WriteMapPresentation(const std::string &mapname, CMap &map)
{
	std::unique_ptr<FileWriter> f;

	int numplayers = 0;
	int topplayer = PlayerMax - 2;

	try {
		f = CreateFileWriter(mapname);
		f->printf("-- Stratagus Map Presentation\n");
		f->printf("-- File generated by the %s v%s built-in map editor.\n\n", QApplication::applicationName().toStdString().c_str(), QApplication::applicationVersion().toStdString().c_str());
		// MAPTODO Copyright notice in generated file

		//Wyrmgus start
		/*
		f->printf("DefinePlayerTypes(");
		while (topplayer > 0 && map.Info.PlayerType[topplayer] == player_type::nobody) {
			--topplayer;
		}
		for (int i = 0; i <= topplayer; ++i) {
			f->printf("%s\"%s\"", (i ? ", " : ""), type[map.Info.PlayerType[i]]);
			if (map.Info.PlayerType[i] == player_type::person) {
				++numplayers;
			}
		}
		f->printf(")\n");

		f->printf("PresentMap(\"%s\", %d, %d, %d, %d)\n",
				  map.Info.Description.c_str(), numplayers, map.Info.MapWidth, map.Info.MapHeight,
				  map.Info.MapUID + 1);
		*/

		f->printf("DefinePlayerTypes(");
		while (topplayer > 0 && CPlayer::Players[topplayer]->get_type() == player_type::nobody) {
			--topplayer;
		}
		for (int i = 0; i <= topplayer; ++i) {
			const player_type player_type = CPlayer::Players[topplayer]->get_type();
			f->printf("%s\"%s\"", (i ? ", " : ""), player_type_to_string(player_type).c_str());
			if (player_type == player_type::person) {
				++numplayers;
			}
		}
		f->printf(")\n");

		f->printf("PresentMap(\"%s\", %d, %d, %d, %d)\n",
			map.Info->get_name().c_str(), numplayers, map.Info->get_map_width(), map.Info->get_map_height(),
			map.Info->MapUID + 1);
	} catch (const FileException &) {
		log::log_error("Cannot write the map presentation.");
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
int WriteMapSetup(const char *mapSetup, CMap &map, const int writeTerrain)
{
	std::unique_ptr<FileWriter> f;

	try {
		f = CreateFileWriter(mapSetup);

		f->printf("-- Stratagus Map Setup\n");
		f->printf("-- File generated by the %s v%s built-in map editor.\n\n", QApplication::applicationName().toStdString().c_str(), QApplication::applicationVersion().toStdString().c_str());
		// MAPTODO Copyright notice in generated file
		
		f->printf("-- player configuration\n");
		for (int i = 0; i < PlayerMax; ++i) {
			if (CPlayer::Players[i]->get_type() == player_type::nobody) {
				continue;
			}
			f->printf("SetStartView(%d, %d, %d)\n", i, CPlayer::Players[i]->StartPos.x, CPlayer::Players[i]->StartPos.y);
			f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
				i, DefaultResourceNames[WoodCost].c_str(),
				CPlayer::Players[i]->get_resource(resource::get_all()[WoodCost]));
			f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
				i, defines::get()->get_wealth_resource()->get_identifier().c_str(),
				CPlayer::Players[i]->get_resource(defines::get()->get_wealth_resource()));
			if (CPlayer::Players[i]->get_resource(resource::get_all()[OilCost]) != 0) {
				f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
					i, DefaultResourceNames[OilCost].c_str(),
					CPlayer::Players[i]->get_resource(resource::get_all()[OilCost]));
			}
			f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
				i, DefaultResourceNames[StoneCost].c_str(),
				CPlayer::Players[i]->get_resource(resource::get_all()[StoneCost]));
			f->printf("SetPlayerData(%d, \"RaceName\", \"%s\")\n",
				i, civilization::get_all()[CPlayer::Players[i]->Race]->get_identifier().c_str());
			if (CPlayer::Players[i]->get_faction() != nullptr) {
				f->printf("SetPlayerData(%d, \"Faction\", \"%s\")\n",
					i, CPlayer::Players[i]->get_faction()->get_identifier().c_str());
			}
			f->printf("SetAiType(%d, \"%s\")\n",
				i, CPlayer::Players[i]->AiName.c_str());
		}
		f->printf("\n");

		f->printf("-- load tilesets\n");
		f->printf("LoadTileModels(\"%s\")\n\n", map.TileModelsFileName.c_str());

		if (writeTerrain) {
			f->printf("-- Tiles Map\n");
			//Wyrmgus start
			for (const std::unique_ptr<CMapLayer> &map_layer: map.MapLayers) {
				for (int y = 0; y < map_layer->get_height(); ++y) {
					for (int x = 0; x < map_layer->get_width(); ++x) {
						const tile &mf = *map_layer->Field(x, y);
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
			
		f->printf("\n-- place units\n");
		f->printf("if (MapUnitsInit ~= nil) then MapUnitsInit() end\n");
		std::vector<const CUnit *> teleporters;
		for (const CUnit *unit : unit_manager::get()->get_units()) {
			f->printf("unit = CreateUnit(\"%s\", %d, {%d, %d})\n",
				unit->Type->get_identifier().c_str(),
				unit->Player->get_index(),
				unit->tilePos.x, unit->tilePos.y);
			if (unit->Type->get_given_resource() != nullptr) {
				f->printf("SetResourcesHeld(unit, %d)\n", unit->ResourcesHeld);
			}
			if (!unit->Active) { //Active is true by default
				f->printf("SetUnitVariable(unit, \"Active\", false)\n");
			}
			if (unit->get_character() != nullptr) {
				if (!unit->get_character()->is_custom()) {
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
	} catch (const FileException &) {
		log::log_error("Can't save map setup: \"" + std::string(mapSetup) + "\".");
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
int SaveStratagusMap(const std::string &mapName, CMap &map, const int writeTerrain)
{
	if (!map.Info->get_map_width() || !map.Info->get_map_height()) {
		throw std::runtime_error(mapName + ": invalid Stratagus map");
	}

	char mapSetup[PATH_MAX];
	strcpy_s(mapSetup, sizeof(mapSetup), mapName.c_str());
	char *setupExtension = strstr(mapSetup, ".smp");
	if (!setupExtension) {
		throw std::runtime_error(mapName + ": invalid Stratagus map filename");
	}

	memcpy(setupExtension, ".sms", 4 * sizeof(char));

	if (WriteMapPresentation(mapName, map) == -1) {
		return -1;
	}

	return WriteMapSetup(mapSetup, map, writeTerrain);
}

/**
**  Load any map.
**
**  @param filename  map filename
**  @param map       map loaded
*/
static void LoadMap(const std::filesystem::path &filepath, CMap &map)
{
	if (filepath.extension() == ".smp"
		|| filepath.extension() == ".wmp"
#ifdef USE_ZLIB
		|| filepath.string().ends_with(".smp.gz")
		|| filepath.string().ends_with(".wmp.gz")
#endif
	) {
		if (map.Info->get_setup_filepath().empty()) {
			// The map info hasn't been loaded yet => do it now
			LoadStratagusMapInfo(filepath);
		}
		assert_throw(!map.Info->get_setup_filepath().empty());
		map.Create();
		LoadStratagusMap(filepath, map.Info->get_setup_filepath());
		return;
	}

	throw std::runtime_error("Unrecognized map format.");
}

/**
**  Set the game paused or unpaused
**
**  @param paused  True to pause game, false to unpause.
*/
void SetGamePaused(bool paused)
{
	game::get()->set_paused(paused);
}

/*----------------------------------------------------------------------------
--  Game types
----------------------------------------------------------------------------*/

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
			
			CommandDiplomacy(i, diplomacy_state::enemy, j);
			CommandDiplomacy(j, diplomacy_state::enemy, i);
		}
	}
}

/**
**  Top vs Bottom
*/
static void GameTypeTopVsBottom()
{
	const int middle = CMap::get()->Info->get_map_height() / 2;

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
				CommandDiplomacy(i, diplomacy_state::allied, j);
				CPlayer::Players[i]->set_shared_vision_with(CPlayer::Players[j].get(), true);
				CommandDiplomacy(j, diplomacy_state::allied, i);
				CPlayer::Players[j]->set_shared_vision_with(CPlayer::Players[i].get(), true);
			} else {
				CommandDiplomacy(i, diplomacy_state::enemy, j);
				CommandDiplomacy(j, diplomacy_state::enemy, i);
			}
		}
	}
}

/**
**  Left vs Right
*/
static void GameTypeLeftVsRight()
{
	const int middle = CMap::get()->Info->get_map_width() / 2;

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
				CommandDiplomacy(i, diplomacy_state::allied, j);
				CPlayer::Players[i]->set_shared_vision_with(CPlayer::Players[j].get(), true);
				CommandDiplomacy(j, diplomacy_state::allied, i);
				CPlayer::Players[j]->set_shared_vision_with(CPlayer::Players[i].get(), true);
			} else {
				CommandDiplomacy(i, diplomacy_state::enemy, j);
				CommandDiplomacy(j, diplomacy_state::enemy, i);
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
		if (CPlayer::Players[i]->get_type() != player_type::person && CPlayer::Players[i]->get_type() != player_type::computer) {
			continue;
		}
		if (CPlayer::Players[i]->has_neutral_faction_type()) {
			continue;
		}

		for (int j = i + 1; j < PlayerMax - 1; ++j) {
			if (CPlayer::Players[j]->get_type() != player_type::person && CPlayer::Players[j]->get_type() != player_type::computer) {
				continue;
			}
			if (CPlayer::Players[j]->has_neutral_faction_type()) {
				continue;
			}

			if (CPlayer::Players[i]->get_type() == CPlayer::Players[j]->get_type()) {
				CommandDiplomacy(i, diplomacy_state::allied, j);
				CPlayer::Players[i]->set_shared_vision_with(CPlayer::Players[j].get(), true);
				CommandDiplomacy(j, diplomacy_state::allied, i);
				CPlayer::Players[j]->set_shared_vision_with(CPlayer::Players[i].get(), true);
			} else {
				CommandDiplomacy(i, diplomacy_state::enemy, j);
				CommandDiplomacy(j, diplomacy_state::enemy, i);
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
		if (CPlayer::Players[i]->get_type() != player_type::person && CPlayer::Players[i]->get_type() != player_type::computer) {
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
				if (CPlayer::Players[i]->get_type() == CPlayer::Players[j]->get_type()) {
					CommandDiplomacy(i, diplomacy_state::allied, j);
					CPlayer::Players[i]->set_shared_vision_with(CPlayer::Players[j].get(), true);
				} else {
					CommandDiplomacy(i, diplomacy_state::enemy, j);
				}
			}
		}
		if (CPlayer::Players[i]->get_type() == player_type::person) {
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

void CalculateItemsToLoad()
{
	ResetItemsToLoad();
}

void IncItemsLoaded()
{
	if (itemsToLoad == 0 || itemsLoaded >= itemsToLoad) {
		return;
	}

	itemsLoaded++;
}

void ResetItemsToLoad()
{
	itemsLoaded = 0;
	itemsToLoad = 0;
}

/*----------------------------------------------------------------------------
--  Settings
----------------------------------------------------------------------------*/

void Settings::reset()
{
	for (int i = 0; i < PlayerMax; ++i) {
		this->Presets[i].AIScript = "ai-passive";
		this->Presets[i].Race = SettingsPresetMapDefault;
		this->Presets[i].Team = SettingsPresetMapDefault;
		this->Presets[i].Type = player_type::none;
	}

	this->Resources = SettingsPresetMapDefault;
	this->NumUnits = SettingsPresetMapDefault;
	this->Opponents = SettingsPresetMapDefault;
	this->Difficulty = DifficultyNormal;
	this->GameType = SettingsPresetMapDefault;
	this->MapRichness = SettingsPresetMapDefault;
	this->NetGameType = SettingsSinglePlayerGame;

	this->NoFogOfWar = false;
	this->Inside = false;
	this->RevealMap = 0;
	this->TechLevel = NoTechLevel;
	this->MaxTechLevel = NoTechLevel;
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
void CreateGame(const std::filesystem::path &filepath, CMap *map)
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
		random::get()->reset_seed(true);
	}
	
	const campaign *current_campaign = game::get()->get_current_campaign();

	QDateTime start_date;

	if (current_campaign != nullptr) {
		start_date = current_campaign->get_start_date();
	} else {
		const int year = 1;
		const int month = random::get()->generate(date::months_per_year) + 1;
		const int day = random::get()->generate(date::get_days_in_month(month, year)) + 1;
		const int hour = random::get()->generate(date::hours_per_day);
		const QDate date(year, month, day);
		start_date = QDateTime(date, QTime(hour, 0));
	}

	game::get()->set_current_year(start_date.date().year());

	uint64_t total_hours = static_cast<uint64_t>(std::abs(game::base_date.date().year() - start_date.date().year())) * DEFAULT_DAYS_PER_YEAR * DEFAULT_HOURS_PER_DAY;
	total_hours += QDate(1, 1, 1).daysTo(QDate(1, start_date.date().month(), start_date.date().day())) * DEFAULT_HOURS_PER_DAY;
	total_hours += start_date.time().hour();

	game::get()->set_current_total_hours(total_hours);
	//Wyrmgus end

	if (CMap::get()->Info->get_presentation_filepath().empty() && !filepath.empty()) {
		const std::string path = LibraryFileName(path::to_string(filepath).c_str());

		if (filepath.extension() == ".smp") {
			LuaLoadFile(path);
		} else if (filepath.extension() == ".wmp") {
			gsml_parser parser;
			database::process_gsml_data(CMap::get()->get_info(), parser.parse(path::from_string(path)));
		}
	}

	if (current_campaign != nullptr && current_campaign->get_map_presets() != nullptr) {
		CMap::get()->get_info()->set_settings(current_campaign->get_map_presets()->get_settings()->duplicate());
	}

	for (int i = 0; i < PlayerMax; ++i) {
		player_type playertype = CMap::get()->Info->get_player_types()[i];
		// Network games only:
		if (GameSettings.Presets[i].Type != player_type::none) {
			playertype = GameSettings.Presets[i].Type;
		}
		CreatePlayer(playertype);
	}
	
	//Wyrmgus start
	CalculateItemsToLoad();
	//Wyrmgus end

	if (!filepath.empty()) {
		if (CurrentMapPath != filepath) {
			CurrentMapPath = filepath;
		}

		//
		// Load the map.
		//
		InitUnitTypes(1);
		LoadMap(filepath, *map);
		//Wyrmgus end
		ApplyUpgrades();
	}

	GameCycle = 0;
	FastForwardCycle = 0;
	SyncHash = 0;
	random::get()->reset_seed(IsNetworkGame());

	if (IsNetworkGame()) { // Prepare network play
		NetworkOnStartGame();
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

	terrain_type::LoadTerrainTypeGraphics();
	resource_icon::load_all();
	icon::load_all();
	InitMissileTypes();
#ifndef DYNAMIC_LOAD
	LoadMissileSprites();
#endif
	LoadConstructions();
	LoadUnitTypes();
	LoadDecorations();

	InitUserInterface();
	UI.Load();

	CMap::get()->Init();
	UI.get_minimap()->Create();

	try {
		PreprocessMap();
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to preprocess map."));
	}

	map->reset_tile_visibility();

	//update the sight of all units, and mark tiles as visible
	for (CUnit *unit : unit_manager::get()->get_units()) {
		if (!unit->Destroyed) {
			UpdateUnitSightRange(*unit);
			MapMarkUnitSight(*unit);
		}
	}

	if (FlagRevealMap) {
		CMap::get()->Reveal();
	}

	//
	// Sound part
	//
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
	trigger::InitActiveTriggers();

#if 0
	if (!UI.SelectedViewport) {
		UI.SelectedViewport = UI.Viewports;
	}
#endif

	if (CPlayer::GetThisPlayer()->StartMapLayer < static_cast<int>(CMap::get()->MapLayers.size())) {
		UI.CurrentMapLayer = CMap::get()->MapLayers[CPlayer::GetThisPlayer()->StartMapLayer].get();
	}
	UI.SelectedViewport->Center(CMap::get()->tile_pos_to_scaled_map_pixel_pos_center(CPlayer::GetThisPlayer()->StartPos));

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
	GameSettings.reset();
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
	trigger::ClearActiveTriggers();
	game::get()->clear();
	CleanAi();
	CleanGroups();
	CleanMissiles();
	CleanUnits();
	CleanSelections();
	CMap::get()->Clean();
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
		std::string path = parameters::get()->GetUserDirectory() + "/" + GameName;
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
	const wyrmgus::resource *res = resource::get(resource);

	CPlayer::Players[player]->set_resource_harvest_speed(res, LuaToNumber(l, 3));
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
	const wyrmgus::resource *res = resource::get(resource);

	CPlayer::Players[player]->set_resource_return_speed(res, LuaToNumber(l, 3));
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
		for (const resource *resource : resource::get_all()) {
			CPlayer::Players[i]->set_resource_harvest_speed(resource, speed);
			CPlayer::Players[i]->set_resource_return_speed(resource, speed);
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
int CclDefineResource(lua_State *l)
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
	resource *resource = resource::get_all()[resource_id];
	
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
			resource->final_resource = resource::get(final_resource_ident);
		} else if (!strcmp(value, "FinalResourceConversionRate")) {
			resource->final_resource_conversion_rate = LuaToNumber(l, -1);
		} else if (!strcmp(value, "LuxuryResource")) {
			resource->luxury = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "BasePrice")) {
			resource->base_price = LuaToNumber(l, -1);
		} else if (!strcmp(value, "DemandElasticity")) {
			resource->demand_elasticity = LuaToNumber(l, -1);
		} else if (!strcmp(value, "InputResource")) {
			const std::string input_resource_ident = LuaToString(l, -1);
			wyrmgus::resource *input_resource = resource::get(input_resource_ident);
			resource->input_resource = input_resource;
		} else if (!strcmp(value, "Hidden")) {
			resource->hidden = LuaToBoolean(l, -1);
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
int CclDefineDefaultResourceNames(lua_State *l)
{
	for (unsigned int i = 0; i < MaxCosts; ++i) {
		DefaultResourceNames[i].clear();
	}

	const unsigned int args = lua_gettop(l);
	for (unsigned int i = 0; i < MaxCosts && i < args; ++i) {
		DefaultResourceNames[i] = LuaToString(l, i + 1);
		
		resource *resource = resource::get_or_add(DefaultResourceNames[i], nullptr);
		resource->index = i;
	}

	resource::sort_instances([](const resource *a, const resource *b) {
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
**  Get Stratagus Version
*/
static int CclGetStratagusVersion(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushstring(l, QApplication::applicationVersion().toStdString().c_str());
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
			std::filesystem::path path = database::get()->get_root_path();
			path /= path::from_string(LuaToString(l, -1));

			LoadStratagusMapInfo(path);
		} else if (!strcmp(value, "SyncHash")) {
			SyncHash = LuaToNumber(l, -1);
		} else if (!strcmp(value, "SyncRandSeed")) {
			random::get()->set_seed(LuaToNumber(l, -1));
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

	lua_register(Lua, "GetStratagusVersion", CclGetStratagusVersion);
	lua_register(Lua, "GetStratagusHomepage", CclGetStratagusHomepage);

	lua_register(Lua, "SavedGameInfo", CclSavedGameInfo);

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
}
