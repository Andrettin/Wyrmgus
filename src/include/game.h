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
//

#pragma once

#include "util/singleton.h"

class CFile;
class CPlayer;
class CUnit;

namespace wyrmgus {

class campaign;
class sml_data;
class sml_property;
class trigger;

template <typename scope_type>
class delayed_effect_instance;

class game final : public singleton<game>
{
public:
	game();
	~game();

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

	void apply_player_history();

	void do_cycle();

	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);

	void save(CFile &file) const;

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

private:
	campaign *current_campaign = nullptr;
	QDateTime current_date;
	std::vector<std::unique_ptr<trigger>> local_triggers; //triggers "local" to the current game
	std::vector<std::unique_ptr<delayed_effect_instance<CPlayer>>> player_delayed_effects;
	std::vector<std::unique_ptr<delayed_effect_instance<CUnit>>> unit_delayed_effects;
};

}

class CFile;
//Wyrmgus start
class CGraphic;
//Wyrmgus end

extern void load_game_data(const std::string &sml_string);

extern void LoadGame(const std::string &filename); /// Load saved game
extern int SaveGame(const std::string &filename); /// Save game
extern void DeleteSaveGame(const std::string &filename);
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
extern std::shared_ptr<CGraphic> loadingBackground;
extern bool DefiningData;
//Wyrmgus end
