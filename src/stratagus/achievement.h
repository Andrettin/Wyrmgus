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
//      (c) Copyright 2017-2020 by Andrettin
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

#include "data_type.h"
#include "icons.h"

#include <map>
#include <string>
#include <vector>

class CCharacter;
class CQuest;
class CUnitType;
struct lua_State;

class CAchievement : public CDataType
{
public:
	static CAchievement *GetAchievement(const std::string &ident, const bool should_find = true);
	static CAchievement *GetOrAddAchievement(const std::string &ident);
	static const std::vector<CAchievement *> &GetAchievements();
	static void ClearAchievements();
	static void CheckAchievements();

private:
	static inline std::vector<CAchievement *> Achievements;
	static inline std::map<std::string, CAchievement *> AchievementsByIdent;

public:
	virtual void ProcessConfigData(const CConfigData *config_data) override;

	const std::string &get_name() const
	{
		return this->name;
	}

	const std::string &get_description() const
	{
		return this->description;
	}

	bool is_hidden() const
	{
		return this->hidden;
	}

	bool is_obtained() const
	{
		return this->obtained;
	}

	bool CanObtain() const;
	void Obtain(bool save = true, bool display = true);
	int GetProgress() const;
	int GetProgressMax() const;

	std::string Ident;				/// Ident of the achievement
private:
	std::string name;				/// Name of the achievement
	std::string description;		/// Description of the achievement
public:
	int PlayerColor = 0;			/// Player color used for the achievement's icon
	int CharacterLevel = 0;			/// Character level required for the achievement
	int Difficulty = -1;			/// Which difficulty the achievement's requirements need to be done in
private:
	bool hidden = false;			/// Whether the achievement is hidden
	bool obtained = false;			/// Whether the achievement has been obtained
public:
	bool Unobtainable = false;		/// Whether this achievement can be obtained by checking for it or not
	IconConfig Icon;				/// Achievement's icon
	const CCharacter *Character = nullptr;	/// Character related to the achievement's requirements
	const CUnitType *CharacterType = nullptr;	/// Unit type required for a character to have for the achievement
	std::vector<const CQuest *> RequiredQuests;	/// Quests required for obtaining this achievement

	friend int CclDefineAchievement(lua_State *l);
};

extern void SetAchievementObtained(const std::string &achievement_ident, const bool save = true, const bool display = true);