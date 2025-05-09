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

#pragma once

#include "database/data_type.h"
#include "database/detailed_data_entry.h"

class CPlayer;
class LuaCallback;
struct lua_State;

extern int CclDefineQuest(lua_State *l);

namespace wyrmgus {

class character;
class civilization;
class dialogue;
class icon;
class player_color;
class quest_objective;
class site;
enum class difficulty;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class effect_list;

class quest final : public detailed_data_entry, public data_type<quest>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(wyrmgus::player_color* player_color MEMBER player_color NOTIFY changed)
	Q_PROPERTY(QString map READ get_map_qstring)
	Q_PROPERTY(bool hidden MEMBER hidden READ is_hidden)
	Q_PROPERTY(bool competitive MEMBER competitive READ is_competitive)
	Q_PROPERTY(bool unobtainable MEMBER unobtainable READ is_unobtainable)
	Q_PROPERTY(bool uncompleteable MEMBER uncompleteable READ is_uncompleteable)
	Q_PROPERTY(bool unfailable MEMBER unfailable READ is_unfailable)
	Q_PROPERTY(bool completed READ is_completed WRITE set_completed NOTIFY completed_changed)
	Q_PROPERTY(int highest_completed_difficulty_index READ get_highest_completed_difficulty_index NOTIFY highest_completed_difficulty_changed)
	Q_PROPERTY(wyrmgus::civilization* civilization MEMBER civilization NOTIFY changed)
	Q_PROPERTY(wyrmgus::quest* required_quest MEMBER required_quest NOTIFY changed)

public:
	static constexpr const char *class_identifier = "quest";
	static constexpr const char property_class_identifier[] = "wyrmgus::quest*";
	static constexpr const char *database_folder = "quests";

	static quest *add(const std::string &identifier, const wyrmgus::data_module *data_module)
	{
		quest *quest = data_type::add(identifier, data_module);
		quest->index = static_cast<int>(quest::get_all().size()) - 1;
		return quest;
	}

	static std::filesystem::path get_quest_completion_filepath();

	static void load_quest_completion();
	static void load_quest_completion_scope(const gsml_data &scope);
	static void save_quest_completion();

	explicit quest(const std::string &identifier);
	~quest();
	
	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void process_text() override;
	virtual void check() const override;

	virtual std::unique_ptr<text_processor_base> create_text_processor() const override;

	int get_index() const
	{
		return this->index;
	}

	const wyrmgus::icon *get_icon() const;

	const wyrmgus::player_color *get_player_color() const
	{
		return this->player_color;
	}

	const QString get_map_qstring() const
	{
		return QString::fromStdString(this->Map);
	}

	bool is_hidden() const
	{
		return this->hidden;
	}

	bool is_competitive() const
	{
		return this->competitive;
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

	bool is_completed() const
	{
		return this->completed;
	}

	void set_completed(const bool completed)
	{
		if (completed == this->is_completed()) {
			return;
		}

		this->completed = completed;
		emit completed_changed();
	}

	const and_condition<CPlayer> *get_conditions() const
	{
		return this->conditions.get();
	}

	const std::unique_ptr<effect_list<CPlayer>> &get_accept_effects() const
	{
		return this->accept_effects;
	}

	const std::unique_ptr<effect_list<CPlayer>> &get_completion_effects() const
	{
		return this->completion_effects;
	}

	const std::unique_ptr<effect_list<CPlayer>> &get_failure_effects() const
	{
		return this->failure_effects;
	}

	const std::vector<std::unique_ptr<quest_objective>> &get_objectives() const
	{
		return this->objectives;
	}

	const std::vector<std::string> &get_objective_strings() const
	{
		return this->objective_strings;
	}

	std::string get_rewards_string(const CPlayer *player) const;

	const std::string &get_hint() const
	{
		return this->hint;
	}

	difficulty get_highest_completed_difficulty() const
	{
		return this->highest_completed_difficulty;
	}

	int get_highest_completed_difficulty_index() const
	{
		return static_cast<int>(this->get_highest_completed_difficulty());
	}

	void set_highest_completed_difficulty(const difficulty difficulty)
	{
		if (difficulty == this->get_highest_completed_difficulty()) {
			return;
		}

		this->highest_completed_difficulty = difficulty;
		emit highest_completed_difficulty_changed();
	}

	bool overlaps_with(const quest *other_quest) const;

	void on_completed(const difficulty difficulty);

	bool has_settlement_objective(const site *settlement) const;

signals:
	void completed_changed();
	void highest_completed_difficulty_changed();
	void changed();

private:
	int index = -1;
	wyrmgus::icon *icon = nullptr;
	wyrmgus::player_color *player_color = nullptr; //the player color used for the quest's icon
public:
	std::string World;				/// Which world the quest belongs to
	std::string Map;				/// What map the quest is played on
	std::string Scenario;			/// Which scenario file is to be loaded for the quest
	quest *required_quest = nullptr; //quest required before this quest becomes available
	std::string RequiredTechnology;	/// Technology required before this quest becomes available
	std::string Area;				/// The area where the quest is set
	std::string Briefing;			/// Briefing text of the quest
	std::string BriefingBackground;	/// Image file for the briefing's background
	std::string StartSpeech;		/// Speech given by the quest giver when offering the quest
	std::string InProgressSpeech;	/// Speech given by the quest giver while the quest is in progress
	std::string CompletionSpeech;	/// Speech given by the quest giver when the quest is completed
private:
	std::string rewards_string;		/// Description of the quest's rewards
	std::string hint;				/// Quest hint
public:
	wyrmgus::civilization *civilization = nullptr; //the civilization to which the quest belongs
private:
	difficulty highest_completed_difficulty;
	bool hidden = false;				/// Whether the quest is hidden
	bool competitive = false;			/// Whether a player completing the quest causes it to fail for others
	bool unobtainable = false;			/// Whether the quest can be obtained normally (or only through triggers)
	bool uncompleteable = false;		/// Whether the quest can be completed normally (or only through triggers)
	bool unfailable = false;			/// Whether the quest can fail normally
	bool completed = false;				/// Whether the quest has been completed
public:
	bool CurrentCompleted = false;		/// Whether the quest has been completed in the current game
	wyrmgus::dialogue *IntroductionDialogue = nullptr;
	std::unique_ptr<LuaCallback> Conditions;
	std::unique_ptr<LuaCallback> AcceptEffects;
	std::unique_ptr<LuaCallback> CompletionEffects;
	std::unique_ptr<LuaCallback> FailEffects;
private:
	std::unique_ptr<and_condition<CPlayer>> conditions;
	std::unique_ptr<effect_list<CPlayer>> accept_effects;
	std::unique_ptr<effect_list<CPlayer>> completion_effects;
	std::unique_ptr<effect_list<CPlayer>> failure_effects;
	std::vector<std::unique_ptr<quest_objective>> objectives;
	std::vector<std::string> objective_strings; //display-only objective strings for the quest
public:
	std::vector<std::string> BriefingSounds;	/// The briefing sounds of this quest
	std::vector<character *> HeroesMustSurvive;	/// Which heroes must survive or this quest fails

	friend int ::CclDefineQuest(lua_State *l);
};

}

extern wyrmgus::quest *CurrentQuest;

extern void SaveQuestCompletion();

extern void SetCurrentQuest(const std::string &quest_ident);
extern std::string GetCurrentQuest();
extern void SetQuestCompleted(const std::string &quest_ident, int difficulty, bool save = true);
extern void SetQuestCompleted(const std::string &quest_ident, const std::string &difficulty_str, bool save = true);
extern void SetQuestCompleted(const std::string &quest_ident, bool save = true);

extern void QuestCclRegister();
