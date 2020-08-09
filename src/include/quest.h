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
//      (c) Copyright 2015-2020 by Andrettin
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

#include "database/data_type.h"
#include "database/detailed_data_entry.h"
#include "ui/icon.h"

class CUpgrade;
class LuaCallback;
struct lua_State;

int CclDefineQuest(lua_State *l);

namespace stratagus {

class character;
class civilization;
class condition;
class dialogue;
class effect_list;
class faction;
class player_color;
class quest;
class site;
class unique_item;
class unit_class;
class unit_type;
enum class objective_type;

class quest_objective
{
public:
	explicit quest_objective(const objective_type objective_type, const stratagus::quest *quest);

	void process_sml_property(const stratagus::sml_property &property);
	void process_sml_scope(const stratagus::sml_data &scope);

	objective_type get_objective_type() const
	{
		return this->objective_type;
	}

	const stratagus::quest *get_quest() const
	{
		return this->quest;
	}

	int get_index() const
	{
		return this->index;
	}

	int get_quantity() const
	{
		return this->quantity;
	}

	const std::string &get_objective_string() const
	{
		return this->objective_string;
	}

	const std::vector<const stratagus::unit_class *> &get_unit_classes() const
	{
		return this->unit_classes;
	}

	const stratagus::site *get_settlement() const
	{
		return this->settlement;
	}

	const stratagus::faction *get_faction() const
	{
		return this->faction;
	}

	const stratagus::character *get_character() const
	{
		return this->character;
	}

private:
	objective_type objective_type;
	const stratagus::quest *quest = nullptr;
	int index = -1;
	int quantity = 1;
public:
	int Resource = -1;
private:
	std::string objective_string;
	std::vector<const stratagus::unit_class *> unit_classes;
public:
	std::vector<stratagus::unit_type *> UnitTypes;
	const CUpgrade *Upgrade = nullptr;
private:
	const stratagus::character *character = nullptr;
public:
	const unique_item *Unique = nullptr;
private:
	const stratagus::site *settlement = nullptr;
	const stratagus::faction *faction = nullptr;

	friend int ::CclDefineQuest(lua_State *l);
};

class player_quest_objective
{
public:
	player_quest_objective(const quest_objective *quest_objective) : quest_objective(quest_objective)
	{
	}

	const quest_objective *get_quest_objective() const
	{
		return this->quest_objective;
	}

private:
	const quest_objective *quest_objective = nullptr;
public:
	int Counter = 0;
};

class quest final : public detailed_data_entry, public data_type<quest>
{
	Q_OBJECT

	Q_PROPERTY(stratagus::icon* icon MEMBER icon READ get_icon)
	Q_PROPERTY(stratagus::player_color* player_color MEMBER player_color READ get_player_color)
	Q_PROPERTY(bool unobtainable MEMBER unobtainable READ is_unobtainable)
	Q_PROPERTY(bool uncompleteable MEMBER uncompleteable READ is_uncompleteable)
	Q_PROPERTY(bool unfailable MEMBER unfailable READ is_unfailable)

public:
	static constexpr const char *class_identifier = "quest";
	static constexpr const char *database_folder = "quests";

	static quest *add(const std::string &identifier, const stratagus::module *module)
	{
		quest *quest = data_type::add(identifier, module);
		quest->ID = quest::get_all().size() - 1;
		return quest;
	}

	explicit quest(const std::string &identifier);
	~quest();
	
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;

	icon *get_icon() const
	{
		return this->icon;
	}

	player_color *get_player_color() const
	{
		return this->player_color;
	}

	bool is_unobtainable() const
	{
		return this->unobtainable;
	}

	bool is_uncompleteable() const
	{
		return this->uncompleteable;
	}

	bool is_unfailable() const
	{
		return this->unfailable;
	}

	bool IsCompleted() const
	{
		return this->Completed;
	}

	const std::unique_ptr<condition> &get_conditions() const
	{
		return this->conditions;
	}

	const std::unique_ptr<effect_list> &get_accept_effects() const
	{
		return this->accept_effects;
	}

	const std::unique_ptr<effect_list> &get_completion_effects() const
	{
		return this->completion_effects;
	}

	const std::vector<std::unique_ptr<quest_objective>> &get_objectives() const
	{
		return this->objectives;
	}

	const std::vector<std::string> &get_objective_strings() const
	{
		return this->objective_strings;
	}

	std::string get_rewards_string() const;

	std::string World;				/// Which world the quest belongs to
	std::string Map;				/// What map the quest is played on
	std::string Scenario;			/// Which scenario file is to be loaded for the quest
	std::string RequiredQuest;		/// Quest required before this quest becomes available
	std::string RequiredTechnology;	/// Technology required before this quest becomes available
	std::string Area;				/// The area where the quest is set
	std::string Briefing;			/// Briefing text of the quest
	std::string BriefingBackground;	/// Image file for the briefing's background
	std::string BriefingMusic;		/// Music file image to play during the briefing
	std::string LoadingMusic;		/// Music to play during the loading
	std::string MapMusic;			/// Music to play during quest
	std::string StartSpeech;		/// Speech given by the quest giver when offering the quest
	std::string InProgressSpeech;	/// Speech given by the quest giver while the quest is in progress
	std::string CompletionSpeech;	/// Speech given by the quest giver when the quest is completed
private:
	std::string rewards_string;		/// Description of the quest's rewards
public:
	std::string Hint;				/// Quest hint
	int ID = -1;
	civilization *civilization = nullptr; //civilization to which civilization the quest belongs to
private:
	icon *icon = nullptr;
	player_color *player_color = nullptr;		/// Player color used for the quest's icon
public:
	int HighestCompletedDifficulty = -1;
	bool Hidden = false;				/// Whether the quest is hidden
	bool Competitive = false;			/// Whether a player completing the quest causes it to fail for others
private:
	bool unobtainable = false;			/// Whether the quest can be obtained normally (or only through triggers)
	bool uncompleteable = false;		/// Whether the quest can be completed normally (or only through triggers)
	bool unfailable = false;			/// Whether the quest can fail normally
public:
	bool Completed = false;				/// Whether the quest has been completed
	bool CurrentCompleted = false;		/// Whether the quest has been completed in the current game
	stratagus::dialogue *IntroductionDialogue = nullptr;
	LuaCallback *Conditions = nullptr;
	LuaCallback *AcceptEffects = nullptr;
	LuaCallback *CompletionEffects = nullptr;
	LuaCallback *FailEffects = nullptr;
private:
	std::unique_ptr<condition> conditions;
	std::unique_ptr<effect_list> accept_effects;
	std::unique_ptr<effect_list> completion_effects;
	std::vector<std::unique_ptr<quest_objective>> objectives;
	std::vector<std::string> objective_strings; //display-only objective strings for the quest
public:
	std::vector<std::string> BriefingSounds;	/// The briefing sounds of this quest
	std::vector<character *> HeroesMustSurvive;	/// Which heroes must survive or this quest fails

	friend int ::CclDefineQuest(lua_State *l);
};

}

extern stratagus::quest *CurrentQuest;

extern void SaveQuestCompletion();

extern void SetCurrentQuest(const std::string &quest_ident);
extern std::string GetCurrentQuest();
extern void SetQuestCompleted(const std::string &quest_ident, int difficulty = 2, bool save = true);
extern void SetQuestCompleted(const std::string &quest_ident, bool save);

extern void QuestCclRegister();
