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
/**@name quest.h - The quest headerfile. */
//
//      (c) Copyright 2015-2016 by Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <vector>
#include <tuple>

#ifndef __ICONS_H__
#include "icons.h"
#endif

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;
class CDialogue;
class CDialogueNode;
class CFaction;
class CUniqueItem;
class CUnitType;
class CUpgrade;
class LuaCallback;
class CMapTemplate;

class CQuest
{
public:
	CQuest() :
		ID(-1), Civilization(-1), PlayerColor(0), HairColor(0), HighestCompletedDifficulty(-1),
		Hidden(false), Completed(false), CurrentCompleted(false), Competitive(false), Unobtainable(false), Uncompleteable(false), Unfailable(false),
		QuestGiver(NULL), IntroductionDialogue(NULL), Conditions(NULL), AcceptEffects(NULL), CompletionEffects(NULL), FailEffects(NULL)
	{
	}
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
	int ID;
	int Civilization;				/// Which civilization the quest belongs to
	int PlayerColor;				/// Player color used for the quest's icon
	int HairColor;					/// Hair color used for the quest's icon
	int HighestCompletedDifficulty;
	bool Hidden;					/// Whether the quest is hidden
	bool Completed;					/// Whether the quest has been completed
	bool CurrentCompleted;			/// Whether the quest has been completed in the current game
	bool Competitive;				/// Whether a player completing the quest causes it to fail for others
	bool Unobtainable;				/// Whether the quest can be obtained normally (or only through triggers)
	bool Uncompleteable;			/// Whether the quest can be completed normally (or only through triggers)
	bool Unfailable;				/// Whether the quest can fail normally
	IconConfig Icon;				/// Quest's icon
	CCharacter *QuestGiver;			/// Quest giver
	CDialogue *IntroductionDialogue;
	LuaCallback *Conditions;
	LuaCallback *AcceptEffects;
	LuaCallback *CompletionEffects;
	LuaCallback *FailEffects;
	std::vector<std::string> Objectives;	/// The objectives of this quest
	std::vector<std::string> BriefingSounds;	/// The briefing sounds of this quest
	std::vector<std::tuple<CUnitType *, int>> BuildUnits;	/// Build units objective vector, containing unit type and quantity
	std::vector<std::tuple<int, int>> BuildUnitsOfClass;	/// Build units objective vector, containing class id and quantity
	std::vector<CUpgrade *> ResearchUpgrades;
	std::vector<std::tuple<CUnitType *, CFaction *, int>> DestroyUnits;	/// Destroy units objective vector, containing unit type, faction and quantity
	std::vector<CUniqueItem *> DestroyUniques;
	std::vector<CFaction *> DestroyFactions;	/// Destroy factions objective vector
	std::vector<std::tuple<int, int>> GatherResources;	/// Gather resources objective vector, containing resource ID and quantity
	std::vector<CCharacter *> HeroesMustSurvive;	/// Which heroes must survive or this quest fails
};

class CCampaign
{
public:
	CCampaign() :
		ID(-1), Civilization(-1),
		MapSize(256, 256), MapTemplateStartPos(0, 0),
		Hidden(false), Sandbox(false),
		Faction(NULL), StartEffects(NULL), MapTemplate(NULL)
	{
		StartDate.year = 0;
		StartDate.month = 1;
		StartDate.day = 1;
		StartDate.timeline = NULL;
	}
	~CCampaign();
	
	std::string Ident;				/// Ident of the campaign
	std::string Name;				/// Name of the campaign
	std::string Description;		/// Description of the campaign
	int ID;
	int Civilization;				/// Which civilization the campaign belongs to
	CDate StartDate;				/// The starting date of the campaign
	bool Hidden;					/// Whether the campaign is hidden
	bool Sandbox;					/// Whether the campaign is a sandbox one
	Vec2i MapSize;					/// Map size
	Vec2i MapTemplateStartPos;		/// Map template position the map will start on
	CFaction *Faction;				/// Which faction the player plays as in the campaign
	CMapTemplate *MapTemplate;		/// Map template used by the campaign
	LuaCallback *StartEffects;		/// The effects at game start to set up the campaign
};

class CAchievement
{
public:
	CAchievement() :
		PlayerColor(0), HairColor(0), CharacterLevel(0), Difficulty(-1),
		Hidden(false), Obtained(false), Unobtainable(false),
		Character(NULL), CharacterType(NULL)
	{
	}
	
	void Obtain(bool save = true, bool display = true);
	bool CanObtain();
	
	std::string Ident;				/// Ident of the achievement
	std::string Name;				/// Name of the achievement
	std::string Description;		/// Description of the achievement
	int PlayerColor;				/// Player color used for the achievement's icon
	int HairColor;					/// Hair color used for the achievement's icon
	int CharacterLevel;				/// Character level required for the achievement
	int Difficulty;					/// Which difficulty the achievement's requirements need to be done in
	bool Hidden;					/// Whether the achievement is hidden
	bool Obtained;					/// Whether the achievement has been obtained
	bool Unobtainable;				/// Whether this achievement can be obtained by checking for it or not
	IconConfig Icon;				/// Achievement's icon
	CCharacter *Character;			/// Character related to the achievement's requirements
	CUnitType *CharacterType;		/// Unit type required for a character to have for the achievement
	std::vector<CQuest *> RequiredQuests;	/// Quests required for obtaining this achievement
};

class CDialogue
{
public:
	CDialogue() :
		Ident("")
	{
	}
	
	~CDialogue();
	
	void Call(int player);
	
	std::string Ident;				/// Ident of the dialogue
	std::vector<CDialogueNode *> Nodes;	/// The nodes of the dialogue
};

class CDialogueNode
{
public:
	CDialogueNode() :
		ID(-1), Dialogue(NULL), Conditions(NULL), ImmediateEffects(NULL)
	{
	}
	
	~CDialogueNode();
	
	void Call(int player);
	void OptionEffect(int option, int player);
	
	int ID;
	std::string SpeakerType;			/// "character" if the speaker is a character, "unit" if the speaker belongs to a particular unit type, and empty if the Speaker string will be used as the displayed name of the speaker itself
	std::string SpeakerPlayer;			/// name of the player to whom the speaker belongs
	std::string Speaker;
	std::string Text;
	CDialogue *Dialogue;
	LuaCallback *Conditions;
	LuaCallback *ImmediateEffects;
	std::vector<std::string> Options;
	std::vector<LuaCallback *> OptionEffects;
	std::vector<std::string> OptionTooltips;
};

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern std::vector<CQuest *> Quests;
extern CQuest *CurrentQuest;
extern std::vector<CCampaign *> Campaigns;
extern CCampaign *CurrentCampaign;
extern std::vector<CAchievement *> Achievements;
extern std::vector<CDialogue *> Dialogues;

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void CleanQuests();
extern void CleanDialogues();
extern void SaveQuestCompletion();
extern void CheckAchievements();
extern CQuest *GetQuest(std::string quest_ident);
extern CCampaign *GetCampaign(std::string campaign_ident);
extern CAchievement *GetAchievement(std::string achievement_ident);
extern CDialogue *GetDialogue(std::string dialogue_ident);

extern void SetCurrentQuest(std::string quest_ident);
extern std::string GetCurrentQuest();
extern void SetCurrentCampaign(std::string campaign_ident);
extern std::string GetCurrentCampaign();
extern void SetQuestCompleted(std::string quest_ident, int difficulty = 2, bool save = true);
extern void SetQuestCompleted(std::string quest_ident, bool save);
extern void SetAchievementObtained(std::string achievement_ident, bool save = true, bool display = true);

extern void CallDialogue(std::string dialogue_ident, int player);
extern void CallDialogueNode(std::string dialogue_ident, int node, int player);
extern void CallDialogueNodeOptionEffect(std::string dialogue_ident, int node, int option, int player);

extern void QuestCclRegister();

//@}

#endif // !__QUEST_H__
