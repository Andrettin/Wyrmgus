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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <vector>
#include <tuple>

#include "icons.h"
#include "time/date.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;
class CDialogue;
class CFaction;
class CQuest;
class CSite;
class CUniqueItem;
class CUnitType;
class CUpgrade;
class LuaCallback;
class CMapTemplate;

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
	ObjectiveType ObjectiveType = ObjectiveType::None;
	int Quantity = 1;
	int Resource = -1;
	int UnitClass = -1;
	std::string ObjectiveString;
	CQuest *Quest = nullptr;
	std::vector<const CUnitType *> UnitTypes;
	const CUpgrade *Upgrade = nullptr;
	const CCharacter *Character = nullptr;
	const CUniqueItem *Unique = nullptr;
	const CSite *Settlement = nullptr;
	const CFaction *Faction = nullptr;
};

class CPlayerQuestObjective : public CQuestObjective
{
public:
	int Counter = 0;
};

class CQuest
{
public:
	~CQuest();
	
	std::string Ident;				/// Ident of the quest
	std::string Name;				/// Name of the quest
	std::string Description;		/// Description of the quest
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
	int Civilization = -1;				/// Which civilization the quest belongs to
	int PlayerColor = 0;				/// Player color used for the quest's icon
	int HighestCompletedDifficulty = -1;
	bool Hidden = false;				/// Whether the quest is hidden
	bool Completed = false;				/// Whether the quest has been completed
	bool CurrentCompleted = false;		/// Whether the quest has been completed in the current game
	bool Competitive = false;			/// Whether a player completing the quest causes it to fail for others
	bool Unobtainable = false;			/// Whether the quest can be obtained normally (or only through triggers)
	bool Uncompleteable = false;		/// Whether the quest can be completed normally (or only through triggers)
	bool Unfailable = false;			/// Whether the quest can fail normally
	IconConfig Icon;					/// Quest's icon
	CCharacter *QuestGiver = nullptr;	/// Quest giver
	CDialogue *IntroductionDialogue = nullptr;
	LuaCallback *Conditions = nullptr;
	LuaCallback *AcceptEffects = nullptr;
	LuaCallback *CompletionEffects = nullptr;
	LuaCallback *FailEffects = nullptr;
	std::vector<CQuestObjective *> Objectives;	/// The objectives of this quest
	std::vector<std::string> ObjectiveStrings;	/// The objective strings of this quest
	std::vector<std::string> BriefingSounds;	/// The briefing sounds of this quest
	std::vector<CCharacter *> HeroesMustSurvive;	/// Which heroes must survive or this quest fails
};

class CCampaign
{
public:
	CCampaign() :
		ID(-1),
		Hidden(false), Sandbox(false),
		Faction(nullptr), StartEffects(nullptr)
	{
	}
	~CCampaign();
	
	std::string Ident;				/// Ident of the campaign
	std::string Name;				/// Name of the campaign
	std::string Description;		/// Description of the campaign
	int ID;
	CDate StartDate;				/// The starting date of the campaign
	bool Hidden;					/// Whether the campaign is hidden
	bool Sandbox;					/// Whether the campaign is a sandbox one
	std::vector<CQuest *> RequiredQuests;		/// Quests required by the campaign
	std::vector<Vec2i> MapSizes;				/// Map sizes
	std::vector<Vec2i> MapTemplateStartPos;		/// Map template position the map will start on
	CFaction *Faction;				/// Which faction the player plays as in the campaign
	std::vector<CMapTemplate *> MapTemplates;		/// Map templates used by the campaign
	LuaCallback *StartEffects;		/// The effects at game start to set up the campaign
};

class CAchievement
{
public:
	void Obtain(bool save = true, bool display = true);
	bool CanObtain() const;
	int GetProgress() const;
	int GetProgressMax() const;
	
	std::string Ident;				/// Ident of the achievement
	std::string Name;				/// Name of the achievement
	std::string Description;		/// Description of the achievement
	int PlayerColor = 0;			/// Player color used for the achievement's icon
	int CharacterLevel = 0;			/// Character level required for the achievement
	int Difficulty = -1;			/// Which difficulty the achievement's requirements need to be done in
	bool Hidden = false;			/// Whether the achievement is hidden
	bool Obtained = false;			/// Whether the achievement has been obtained
	bool Unobtainable = false;		/// Whether this achievement can be obtained by checking for it or not
	IconConfig Icon;				/// Achievement's icon
	CCharacter *Character = nullptr;	/// Character related to the achievement's requirements
	CUnitType *CharacterType = nullptr;	/// Unit type required for a character to have for the achievement
	std::vector<CQuest *> RequiredQuests;	/// Quests required for obtaining this achievement
};

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern std::vector<CQuest *> Quests;
extern CQuest *CurrentQuest;
extern std::vector<CCampaign *> Campaigns;
extern CCampaign *CurrentCampaign;
extern std::vector<CAchievement *> Achievements;

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void CleanQuests();
extern void SaveQuestCompletion();
extern void CheckAchievements();
std::string GetQuestObjectiveTypeNameById(const ObjectiveType objective_type);
extern ObjectiveType GetQuestObjectiveTypeIdByName(const std::string &objective_type);
extern CQuest *GetQuest(const std::string &quest_ident);
extern CCampaign *GetCampaign(const std::string &campaign_ident);
extern CAchievement *GetAchievement(const std::string &achievement_ident);

extern void SetCurrentQuest(const std::string &quest_ident);
extern std::string GetCurrentQuest();
extern void SetCurrentCampaign(const std::string &campaign_ident);
extern std::string GetCurrentCampaign();
extern void SetQuestCompleted(const std::string &quest_ident, int difficulty = 2, bool save = true);
extern void SetQuestCompleted(const std::string &quest_ident, bool save);
extern void SetAchievementObtained(const std::string &achievement_ident, bool save = true, bool display = true);

extern void QuestCclRegister();
