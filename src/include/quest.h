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
//      (c) Copyright 2015 by Andrettin
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

#ifndef __ICONS_H__
#include "icons.h"
#endif

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;

class CQuest
{
public:
	CQuest() :
		Civilization(-1), TechnologyPoints(0), X(-1), Y(-1), PlayerColor(0),
		Hidden(false),
		QuestGiver(NULL)
	{
	}
	
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
	std::string StartSpeech;		/// Speech given by the quest giver when offering the quest
	std::string InProgressSpeech;	/// Speech given by the quest giver while the quest is in progress
	std::string CompletionSpeech;	/// Speech given by the quest giver when the quest is completed
	int Civilization;				/// Which civilization the quest belongs to
	int TechnologyPoints;			/// How many technology points the quest gives as a reward
	int X;							/// X position of the quest in its world's quest screen
	int Y;							/// Y position of the quest in its world's quest screen
	int PlayerColor;				/// Player color used for the quest's icon
	bool Hidden;					/// Whether the quest is hidden
	IconConfig Icon;				/// Quest's icon
	CCharacter *QuestGiver;			/// Quest giver
	std::vector<std::string> Objectives;	/// The objectives of this quest (used for the briefing only)
	std::vector<std::string> BriefingSounds;	/// The briefing sounds of this quest
};

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern std::vector<CQuest *> Quests;

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void CleanQuests();
extern CQuest *GetQuest(std::string quest_name);
extern void QuestCclRegister();

//@}

#endif // !__QUEST_H__
