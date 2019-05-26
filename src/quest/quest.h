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
/**@name quest.h - The quest header file. */
//
//      (c) Copyright 2015-2019 by Andrettin
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

#ifndef __QUEST_H__
#define __QUEST_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"
#include "detailed_data_element.h"

#include <core/object.h>

#include <tuple>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;
class CDependency;
class CDialogue;
class CFaction;
class CMapTemplate;
class CPlayerColor;
class CQuest;
class CSite;
class CTriggerEffect;
class CUnitType;
class CUpgrade;
class LuaCallback;
class UniqueItem;
class UnitClass;
struct lua_State;

/*----------------------------------------------------------------------------
--  Enumerations
----------------------------------------------------------------------------*/

enum ObjectiveTypes {
	GatherResourceObjectiveType,
	HaveResourceObjectiveType,
	BuildUnitsObjectiveType,
	DestroyUnitsObjectiveType,
	ResearchUpgradeObjectiveType,
	RecruitHeroObjectiveType,
	DestroyHeroObjectiveType,
	HeroMustSurviveObjectiveType,
	DestroyUniqueObjectiveType,
	DestroyFactionObjectiveType,
	
	MaxObjectiveTypes
};

/*----------------------------------------------------------------------------
--  Definitions
----------------------------------------------------------------------------*/

class CQuestObjective
{
public:
	static CQuestObjective *FromConfigData(const CConfigData *config_data);
	
	void ProcessConfigData(const CConfigData *config_data);

	int ObjectiveType = -1;
	int Quantity = 1;
	int Resource = -1;
	std::string ObjectiveString;
	CQuest *Quest = nullptr;
	std::vector<const UnitClass *> UnitClasses;
	std::vector<const CUnitType *> UnitTypes;
	const CUpgrade *Upgrade = nullptr;
	const CCharacter *Character = nullptr;
	const UniqueItem *Unique = nullptr;
	const CSite *Settlement = nullptr;
	const CFaction *Faction = nullptr;
};

class CPlayerQuestObjective : public CQuestObjective
{
public:
	int Counter = 0;
};

class CQuest : public DetailedDataElement, public DataType<CQuest>
{
	DATA_TYPE(CQuest, DetailedDataElement)
	
public:
	~CQuest();
	
	static constexpr const char *ClassIdentifier = "quest";
	static constexpr int MaxQuestsPerPlayer = 4;	/// the maximum amount of quests a player can have
	
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	
	const CPlayerColor *GetPlayerColor() const
	{
		return this->PlayerColor;
	}
	
	const String &GetHint() const
	{
		return this->Hint;
	}
	
	const String &GetRewardsString() const
	{
		return this->RewardsString;
	}
	
	bool IsCompleted() const
	{
		return this->Completed;
	}
	
	bool IsUnobtainable() const
	{
		return this->Unobtainable;
	}
	
	bool IsRepeatable() const
	{
		return this->Repeatable;
	}
	
	const std::vector<CTriggerEffect *> &GetAcceptEffects() const
	{
		return this->AcceptEffects;
	}
	
	const std::vector<CTriggerEffect *> &GetCompletionEffects() const
	{
		return this->CompletionEffects;
	}
	
	const std::vector<CTriggerEffect *> &GetFailEffects() const
	{
		return this->FailEffects;
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
private:
	String RewardsString;			/// description of the quest's rewards
	String Hint;					/// quest hint
public:
	int Civilization = -1;			/// which civilization the quest belongs to
private:
	const CPlayerColor *PlayerColor = nullptr;	/// the player color used for the quest's icon
public:
	int HighestCompletedDifficulty = -1;
private:
	bool Completed = false;				/// Whether the quest has been completed
public:
	bool CurrentCompleted = false;		/// Whether the quest has been completed in the current game
	bool Competitive = false;			/// Whether a player completing the quest causes it to fail for others
private:
	bool Unobtainable = false;			/// Whether the quest can be obtained normally (or only through triggers)
public:
	bool Uncompleteable = false;		/// Whether the quest can be completed normally (or only through triggers)
	bool Unfailable = false;			/// Whether the quest can fail normally
private:
	bool Repeatable = false;			/// whether the quest is repeatable
public:
	CCharacter *QuestGiver = nullptr;	/// Quest giver
	CDialogue *IntroductionDialogue = nullptr;
	LuaCallback *Conditions = nullptr;
	LuaCallback *AcceptEffectsLua = nullptr;
	LuaCallback *CompletionEffectsLua = nullptr;
	LuaCallback *FailEffectsLua = nullptr;
private:
	std::vector<CTriggerEffect *> AcceptEffects;
	std::vector<CTriggerEffect *> CompletionEffects;
	std::vector<CTriggerEffect *> FailEffects;
public:
	std::vector<CQuestObjective *> Objectives;	/// The objectives of this quest
	std::vector<std::string> ObjectiveStrings;	/// The objective strings of this quest
	std::vector<std::string> BriefingSounds;	/// The briefing sounds of this quest
	std::vector<CCharacter *> HeroesMustSurvive;	/// Which heroes must survive or this quest fails
	
public:
	CDependency *Predependency = nullptr;	/// the predependency for the quest
	CDependency *Dependency = nullptr;		/// the dependency for the quest
	
	friend int CclDefineQuest(lua_State *l);
	friend void SetQuestCompleted(const std::string &quest_ident, int difficulty, bool save);

protected:
	static void _bind_methods();
};

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern CQuest *CurrentQuest;

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void SaveQuestCompletion();
std::string GetQuestObjectiveTypeNameById(int objective_type);
extern int GetQuestObjectiveTypeIdByName(const std::string &objective_type);

extern void SetCurrentQuest(const std::string &quest_ident);
extern std::string GetCurrentQuest();
extern void SetQuestCompleted(const std::string &quest_ident, int difficulty = 2, bool save = true);
extern void SetQuestCompleted(const std::string &quest_ident, bool save);

extern void QuestCclRegister();

#endif
