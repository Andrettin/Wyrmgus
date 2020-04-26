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

class CCharacter;
class CUniqueItem;
class CUnitType;
class CUpgrade;
class LuaCallback;
struct lua_State;

int CclDefineQuest(lua_State *l);

namespace stratagus {
	class civilization;
	class dialogue;
	class faction;
	class quest;
	class site;
	class unit_class;
}

enum class ObjectiveType {
	None = -1,
	GatherResource,
	HaveResource,
	BuildUnits,
	BuildUnitsOfClass,
	DestroyUnits,
	ResearchUpgrade,
	RecruitHero,
	DestroyHero,
	HeroMustSurvive,
	DestroyUnique,
	DestroyFaction
};

class CQuestObjective
{
public:
	CQuestObjective(const int index) : index(index)
	{
	}

	void process_sml_property(const stratagus::sml_property &property);
	void process_sml_scope(const stratagus::sml_data &scope);

	int get_index() const
	{
		return this->index;
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

private:
	int index = -1;
public:
	ObjectiveType ObjectiveType = ObjectiveType::None;
	int Quantity = 1;
	int Resource = -1;
private:
	std::string objective_string;
public:
	stratagus::quest *Quest = nullptr;
private:
	std::vector<const stratagus::unit_class *> unit_classes;
public:
	std::vector<const CUnitType *> UnitTypes;
	const CUpgrade *Upgrade = nullptr;
	const CCharacter *Character = nullptr;
	const CUniqueItem *Unique = nullptr;
private:
	const stratagus::site *settlement = nullptr;
	const stratagus::faction *faction = nullptr;

	friend int CclDefineQuest(lua_State *l);
};

class CPlayerQuestObjective
{
public:
	CPlayerQuestObjective(const CQuestObjective *quest_objective) : quest_objective(quest_objective)
	{
	}

	const CQuestObjective *get_quest_objective() const
	{
		return this->quest_objective;
	}

private:
	const CQuestObjective *quest_objective = nullptr;
public:
	int Counter = 0;
};

namespace stratagus {

class quest final : public detailed_data_entry, public data_type<quest>
{
	Q_OBJECT

	Q_PROPERTY(CIcon* icon MEMBER icon READ get_icon)
	Q_PROPERTY(bool unobtainable MEMBER unobtainable READ is_unobtainable)

public:
	static constexpr const char *class_identifier = "quest";
	static constexpr const char *database_folder = "quests";

	static quest *add(const std::string &identifier, const stratagus::module *module)
	{
		quest *quest = data_type::add(identifier, module);
		quest->ID = quest::get_all().size() - 1;
		return quest;
	}

	quest(const std::string &identifier);
	~quest();
	
	virtual void process_sml_property(const sml_property &property) override;
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;

	CIcon *get_icon() const
	{
		return this->icon;
	}

	bool is_unobtainable() const
	{
		return this->unobtainable;
	}

	bool IsCompleted() const
	{
		return this->Completed;
	}

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
	std::string Rewards;			/// Description of the quest's rewards
	std::string Hint;				/// Quest hint
	int ID = -1;
	civilization *civilization = nullptr; //civilization to which civilization the quest belongs to
private:
	CIcon *icon = nullptr;
public:
	int PlayerColor = 0;				/// Player color used for the quest's icon
	int HighestCompletedDifficulty = -1;
	bool Hidden = false;				/// Whether the quest is hidden
	bool Competitive = false;			/// Whether a player completing the quest causes it to fail for others
private:
	bool unobtainable = false;			/// Whether the quest can be obtained normally (or only through triggers)
public:
	bool Uncompleteable = false;		/// Whether the quest can be completed normally (or only through triggers)
	bool Unfailable = false;			/// Whether the quest can fail normally
	bool Completed = false;				/// Whether the quest has been completed
	bool CurrentCompleted = false;		/// Whether the quest has been completed in the current game
	stratagus::dialogue *IntroductionDialogue = nullptr;
	LuaCallback *Conditions = nullptr;
	LuaCallback *AcceptEffects = nullptr;
	LuaCallback *CompletionEffects = nullptr;
	LuaCallback *FailEffects = nullptr;
	std::vector<CQuestObjective *> Objectives;	/// The objectives of this quest
	std::vector<std::string> ObjectiveStrings;	/// The objective strings of this quest
	std::vector<std::string> BriefingSounds;	/// The briefing sounds of this quest
	std::vector<CCharacter *> HeroesMustSurvive;	/// Which heroes must survive or this quest fails

	friend int ::CclDefineQuest(lua_State *l);
};

}

extern stratagus::quest *CurrentQuest;

extern void SaveQuestCompletion();
std::string GetQuestObjectiveTypeNameById(const ObjectiveType objective_type);
extern ObjectiveType GetQuestObjectiveTypeIdByName(const std::string &objective_type);

extern void SetCurrentQuest(const std::string &quest_ident);
extern std::string GetCurrentQuest();
extern void SetQuestCompleted(const std::string &quest_ident, int difficulty = 2, bool save = true);
extern void SetQuestCompleted(const std::string &quest_ident, bool save);

extern void QuestCclRegister();
