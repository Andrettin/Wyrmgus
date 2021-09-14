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
//      (c) Copyright 2012-2021 by Joris Dauphin and Andrettin
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

class CFile;
class CPlayer;
class CUnit;

namespace wyrmgus {

class campaign;
class results_info;
class sml_data;
class sml_property;
class trigger;

template <typename scope_type>
class delayed_effect_instance;

class game final : public QObject, public singleton<game>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::campaign* current_campaign READ get_current_campaign WRITE set_current_campaign)
	Q_PROPERTY(bool running READ is_running NOTIFY running_changed)
	Q_PROPERTY(bool multiplayer READ is_multiplayer NOTIFY multiplayer_changed)
	Q_PROPERTY(bool console_active READ is_console_active_sync NOTIFY console_active_changed)
	Q_PROPERTY(wyrmgus::results_info* results READ get_results NOTIFY results_changed)

public:
	//100,000 BC; base date from which to calculate the current total hours from the base date
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	static inline const QDateTime base_date = QDateTime(QDate(-100000, 1, 1).startOfDay(Qt::UTC));
#else
	static inline const QDateTime base_date = QDateTime(QDate(-100000, 1, 1));
#endif

	game();
	~game();

	void clear()
	{
		this->cheat = false;
		this->clear_delayed_effects();
	}

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
		this->current_campaign = campaign;
	}

	const QDateTime &get_current_date() const
	{
		return this->current_date;
	}

	void set_current_date(const QDateTime &date)
	{
		this->current_date = date;
	}

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

	void apply_player_history();

	void do_cycle();

	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);

	void save(CFile &file) const;

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

	results_info *get_results() const
	{
		return this->results.get();
	}

	bool is_console_active() const
	{
		return this->console_active;
	}

	bool is_console_active_sync() const
	{
		std::shared_lock<std::shared_mutex> lock(this->mutex);

		return this->is_console_active();
	}

	void set_console_active(const bool active)
	{
		if (active == this->is_console_active()) {
			return;
		}

		std::unique_lock<std::shared_mutex> lock(this->mutex);

		this->console_active = active;

		emit console_active_changed();
	}

	void set_results(qunique_ptr<results_info> &&results);
	void store_results();
	void clear_results();

signals:
	void started();
	void stopped();
	void running_changed();
	void multiplayer_changed();
	void console_active_changed();
	void results_changed();

private:
	bool running = false;
	bool multiplayer = false;
	campaign *current_campaign = nullptr;
	QDateTime current_date;
	uint64_t current_total_hours = 0; //the total in-game hours
	bool cheat = false; //whether a cheat was used in this game
	std::vector<std::unique_ptr<trigger>> local_triggers; //triggers "local" to the current game
	std::vector<std::unique_ptr<delayed_effect_instance<CPlayer>>> player_delayed_effects;
	std::vector<std::unique_ptr<delayed_effect_instance<CUnit>>> unit_delayed_effects;
	bool console_active = false;
	qunique_ptr<results_info> results;
	mutable std::shared_mutex mutex;
};

}

class CFile;
//Wyrmgus start
class CGraphic;
//Wyrmgus end

extern void load_game_data(const std::string &sml_string);

extern void LoadGame(const std::string &filename); /// Load saved game
extern int SaveGame(const std::string &filepath_str); /// Save game
extern void StartSavedGame(const std::string &filename);
extern void load_game(const std::string &filename);
extern void set_load_game_file(const std::string &filename);
extern std::string load_game_file;
extern bool SaveGameLoading;                 /// Save game is in progress of loading

extern void InitModules();              /// Initialize all modules
extern void LuaRegisterModules();       /// Register lua script of each modules
extern void LoadModules();              /// Load all modules
extern void CleanModules();             /// Cleanup all modules

extern void FreeAllContainers();

extern void SaveGameSettings(CFile &file);             /// Save game settings

extern std::string GameName;                /// Name of the game
extern std::string FullGameName;            /// Full Name of the game

extern bool UseHPForXp;                     /// true if gain XP by dealing damage, false if by killing.

//Wyrmgus start
extern bool DefiningData;
//Wyrmgus end
