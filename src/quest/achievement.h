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

#include "data_type.h"
#include "ui/icon_config.h"

#include <core/object.h>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;
class CIcon;
class CPlayerColor;
class CQuest;
class CUnitType;

class CAchievement : public CDataType, public Object
{
	GDCLASS(CAchievement, Object)
	DATA_TYPE_CLASS(CAchievement)
	
public:
	static void CheckAchievements();
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	virtual void Initialize() override;
	
	String GetName() const
	{
		return this->Name.c_str();
	}
	
	String GetDescription() const
	{
		return this->Description.c_str();
	}
	
	CIcon *GetIcon() const
	{
		return this->Icon.Icon;
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
	
	CPlayerColor *GetPlayerColor() const
	{
		return this->PlayerColor;
	}
	
	int GetProgress() const;
	int GetProgressMax() const;
	
public:
	std::string Name;
	std::string Description;		/// Description of the achievement
	CPlayerColor *PlayerColor = nullptr;	/// Player color used for the achievement's icon
	int CharacterLevel = 0;			/// Character level required for the achievement
	int Difficulty = -1;			/// Which difficulty the achievement's requirements need to be done in
	bool Hidden = false;			/// Whether the achievement is hidden
	bool Obtained = false;			/// Whether the achievement has been obtained
	bool Unobtainable = false;		/// Whether this achievement can be obtained by checking for it or not
	IconConfig Icon;				/// Achievement's icon
	const CCharacter *Character = nullptr;	/// Character related to the achievement's requirements
	const CUnitType *CharacterType = nullptr;	/// Unit type required for a character to have for the achievement
	std::vector<const CQuest *> RequiredQuests;	/// Quests required for obtaining this achievement

protected:
	static void _bind_methods();
};

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void SetAchievementObtained(const std::string &achievement_ident, const bool save = true, const bool display = true);

#endif
