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
/**@name achievement.h - The achievement header file. */
//
//      (c) Copyright 2017-2019 by Andrettin
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

#ifndef __ACHIEVEMENT_H__
#define __ACHIEVEMENT_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"
#include "ui/icon_config.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;
class CIcon;
class CPlayerColor;
class CQuest;
class CUnitType;
struct lua_State;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CAchievement : public DataElement, public DataType<CAchievement>
{
	GDCLASS(CAchievement, DataElement)
	
public:
	static constexpr const char *ClassIdentifier = "achievement";
	
	static void CheckAchievements();
	
	virtual bool ProcessConfigDataProperty(const String &key, String value) override;
	virtual void Initialize() override;
	
	const String &GetDescription() const
	{
		return this->Description;
	}
	
	int GetCharacterLevel() const
	{
		return this->CharacterLevel;
	}
	
	int GetDifficulty() const
	{
		return this->Difficulty;
	}
	
	bool IsHidden() const
	{
		return this->Hidden;
	}
	
	bool IsObtained() const
	{
		return this->Obtained;
	}
	
	void Obtain(const bool save = true, const bool display = true);
	bool CanObtain() const;
	
	bool IsUnobtainable() const
	{
		return this->Unobtainable;
	}
	
	const CIcon *GetIcon() const
	{
		return this->Icon.Icon;
	}
	
	const CPlayerColor *GetPrimaryPlayerColor() const
	{
		return this->PrimaryPlayerColor;
	}
	
	const CPlayerColor *GetSecondaryPlayerColor() const
	{
		return this->SecondaryPlayerColor;
	}
	
	int GetProgress() const;
	int GetProgressMax() const;
	
private:
	String Description;		/// description of the achievement
	int CharacterLevel = 0;		/// character level required for the achievement
	int Difficulty = -1;		/// which difficulty the achievement's requirements need to be done in
	bool Hidden = false;		/// whether the achievement is hidden
	bool Obtained = false;		/// whether the achievement has been obtained
	bool Unobtainable = false;	/// whether this achievement can be obtained by checking for it or not
	const CPlayerColor *PrimaryPlayerColor = nullptr;	/// primary player color used for the achievement's icon
	const CPlayerColor *SecondaryPlayerColor = nullptr;	/// secondary player color used for the achievement's icon
	IconConfig Icon;				/// achievement's icon
public:
	const CCharacter *Character = nullptr;	/// character related to the achievement's requirements
	const CUnitType *CharacterType = nullptr;	/// unit type required for a character to have for the achievement
	std::vector<const CQuest *> RequiredQuests;	/// quests required for obtaining this achievement
	
	friend int CclDefineAchievement(lua_State *l);

protected:
	static void _bind_methods();
};

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void SetAchievementObtained(const std::string &achievement_ident, const bool save = true, const bool display = true);

#endif
