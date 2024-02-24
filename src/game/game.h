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
//      (c) Copyright 2012-2022 by Joris Dauphin and Andrettin
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

#pragma once

#include "util/qunique_ptr.h"
#include "util/singleton.h"

Q_MOC_INCLUDE("game/results_info.h")
Q_MOC_INCLUDE("quest/campaign.h")

class CFile;
class CPlayer;
class CUnit;

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace wyrmgus {

class age;
class campaign;
class faction;
class results_info;
class trigger;

template <typename scope_type>
class delayed_effect_instance;

class game final : public QObject, public singleton<game>
{
	Q_OBJECT

	Q_PROPERTY(bool running READ is_running NOTIFY running_changed)
	Q_PROPERTY(bool paused READ is_paused WRITE set_paused NOTIFY paused_changed)
	Q_PROPERTY(bool multiplayer READ is_multiplayer NOTIFY multiplayer_changed)
	Q_PROPERTY(wyrmgus::campaign* current_campaign READ get_current_campaign NOTIFY current_campaign_changed)
	Q_PROPERTY(int current_year READ get_current_year NOTIFY current_year_changed)
	Q_PROPERTY(bool console_active READ is_console_active NOTIFY console_active_changed)
	Q_PROPERTY(wyrmgus::results_info* results READ get_results NOTIFY results_changed)

public:
	//100,000 BC; base date from which to calculate the current total hours from the base date
	static inline const QDateTime base_date = QDateTime(QDate(-100000, 1, 1).startOfDay(Qt::UTC));

	static std::filesystem::path save_file_url_string_to_save_filepath(const std::string &file_url_str);

	game();
	~game();

	void clear();

	bool is_running() const
	{
		return this->running;
	}

	void set_running(const bool running)
	{
		if (running == this->is_running()) {
			return;
		}

		this->running = running;

		if (running) {
			emit started();
		} else {
			emit stopped();
		}

		emit running_changed();

		if (running) {
			this->on_started();
		}
	}

	void on_started();

	bool is_paused() const
	{
		return this->paused;
	}

	void set_paused(const bool paused);

	void toggle_paused()
	{
		this->set_paused(!this->is_paused());
	}

	bool is_multiplayer() const
	{
		return this->multiplayer;
	}

	void set_multiplayer(const bool multiplayer)
	{
		if (multiplayer == this->is_multiplayer()) {
			return;
		}

		this->multiplayer = multiplayer;

		emit multiplayer_changed();
	}

	campaign *get_current_campaign() const
	{
		return this->current_campaign;
	}

	void set_current_campaign(campaign *campaign)
	{
		if (campaign == this->get_current_campaign()) {
			return;
		}

		this->current_campaign = campaign;

		emit current_campaign_changed();
	}

	int get_current_year() const
	{
		return this->current_year;
	}

	void set_current_year(const int year)
	{
		if (year == this->get_current_year()) {
			return;
		}

		this->current_year = year;

		emit current_year_changed();
	}

	void increment_current_year()
	{
		int year = this->get_current_year();
		++year;

		if (year == 0) {
			++year;
		}

		this->set_current_year(year);
	}

	int get_cycles_per_year() const;

	uint64_t get_current_total_hours() const
	{
		return this->current_total_hours;
	}

	void set_current_total_hours(const uint64_t hours)
	{
		this->current_total_hours = hours;
	}

	void increment_current_total_hours()
	{
		++this->current_total_hours;
	}

	[[nodiscard]]
	QCoro::Task<void> run_map(const std::filesystem::path filepath);

	Q_INVOKABLE QCoro::QmlTask run_map_async(const QString &filepath);

	Q_INVOKABLE QCoro::QmlTask run_campaign_async(wyrmgus::campaign *campaign)
	{
		return this->run_campaign_coro(campaign);
	}

	[[nodiscard]]
	QCoro::Task<void> run_campaign_coro(campaign *campaign);

	void apply_player_history();

	void do_cycle();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	void save(const std::filesystem::path &filepath) const;
	void save_game_data(CFile &file) const;

	void set_cheat(const bool cheat);

	bool is_persistency_enabled() const;

	void add_local_trigger(std::unique_ptr<trigger> &&local_trigger);
	void remove_local_trigger(trigger *local_trigger);
	void clear_local_triggers();

	void process_delayed_effects();

private:
	template <typename scope_type>
	void process_delayed_effects(std::vector<std::unique_ptr<delayed_effect_instance<scope_type>>> &delayed_effects)
	{
		for (size_t i = 0; i < delayed_effects.size();) {
			const std::unique_ptr<delayed_effect_instance<scope_type>> &delayed_effect = delayed_effects[i];
			delayed_effect->decrement_remaining_cycles();

			if (delayed_effect->get_remaining_cycles() <= 0) {
				delayed_effect->do_effects();
				delayed_effects.erase(delayed_effects.begin() + i);
			} else {
				++i;
			}
		}
	}

public:
	void add_delayed_effect(std::unique_ptr<delayed_effect_instance<CPlayer>> &&delayed_effect);
	void add_delayed_effect(std::unique_ptr<delayed_effect_instance<CUnit>> &&delayed_effect);

	void clear_delayed_effects();

	void update_neutral_faction_presence();
	void do_neutral_faction_expansion(const faction *faction);
	void do_neutral_faction_contraction(const faction *faction);

	results_info *get_results() const
	{
		return this->results.get();
	}

	bool is_console_active() const
	{
		return this->console_active;
	}

	void set_console_active(const bool active)
	{
		if (active == this->is_console_active()) {
			return;
		}

		this->console_active = active;

		emit console_active_changed();
	}

	void set_results(qunique_ptr<results_info> &&results);
	void store_results();
	void clear_results();

	void post_function(std::function<void()> &&function)
	{
		this->posted_functions.push_back(std::move(function));
	}

	void process_functions()
	{
		//process the functions which have been posted to be processed at a specific point in the loop, since they have gameplay effects
		for (const std::function<void()> &function : this->posted_functions) {
			function();
		}

		this->posted_functions.clear();
	}

signals:
	void started();
	void stopped();
	void running_changed();
	void paused_changed();
	void multiplayer_changed();
	void current_campaign_changed();
	void current_year_changed();
	void console_active_changed();
	void results_changed();

private:
	bool running = false;
	bool paused = false;
	bool multiplayer = false;
	campaign *current_campaign = nullptr;
	int current_year = 0;
	uint64_t current_total_hours = 0; //the total in-game hours
	bool cheat = false; //whether a cheat was used in this game
	std::vector<std::unique_ptr<trigger>> local_triggers; //triggers "local" to the current game
	std::vector<std::unique_ptr<delayed_effect_instance<CPlayer>>> player_delayed_effects;
	std::vector<std::unique_ptr<delayed_effect_instance<CUnit>>> unit_delayed_effects;
	bool console_active = false;
	qunique_ptr<results_info> results;
	std::vector<std::function<void()>> posted_functions;
};

}

class CFile;
//Wyrmgus start
class CGraphic;
//Wyrmgus end

extern void load_game_data(const std::string &gsml_string);

extern void LoadGame(const std::filesystem::path &filepath); /// Load saved game
extern int SaveGame(const std::string &file_url_str); /// Save game

[[nodiscard]]
extern QCoro::Task<void> StartSavedGame(const std::filesystem::path &filepath);

[[nodiscard]]
extern QCoro::Task<void> load_game(const std::filesystem::path &filepath);

extern void set_load_game_file(const std::filesystem::path &filepath);
extern std::filesystem::path load_game_file;
extern bool SaveGameLoading;                 /// Save game is in progress of loading

extern void InitModules();              /// Initialize all modules
extern void LuaRegisterModules();       /// Register lua script of each modules
extern void LoadModules();              /// Load all modules
extern void CleanModules();             /// Cleanup all modules

extern void FreeAllContainers();

extern void SaveGameSettings(CFile &file);             /// Save game settings

[[nodiscard]]
extern QCoro::Task<void> StartMap(const std::filesystem::path &filepath, const bool clean);

extern std::string GameName;                /// Name of the game
extern std::string FullGameName;            /// Full Name of the game

extern bool UseHPForXp;                     /// true if gain XP by dealing damage, false if by killing.

//Wyrmgus start
extern bool DefiningData;
//Wyrmgus end

/// Flag telling if the game is running
extern bool GameRunning;
/// Flag telling if the game is in observe mode
extern bool GameObserve;
/// Flag telling if the game is in establishing mode
extern bool GameEstablishing;

extern void CleanGame();
