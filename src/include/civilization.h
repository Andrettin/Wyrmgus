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
/**@name civilization.h - The civilization header file. */
//
//      (c) Copyright 2018-2019 by Andrettin
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

#ifndef __CIVILIZATION_H__
#define __CIVILIZATION_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <map>
#include <string>
#include <vector>

#include "player.h" //for certain enums
#include "time/date.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CAiBuildingTemplate;
class CCalendar;
class CCurrency;
class CDeity;
class CForceTemplate;
class CLanguage;
class CQuest;
class CUpgrade;

class CCivilization
{
public:
	~CCivilization();
	
	static CCivilization *GetCivilization(const std::string &ident, const bool should_find = true);
	static CCivilization *GetOrAddCivilization(const std::string &ident);
	static void ClearCivilizations();
	
	static std::vector<CCivilization *> Civilizations;    					/// civilizations
	static std::map<std::string, CCivilization *> CivilizationsByIdent;
	
	int GetUpgradePriority(const CUpgrade *upgrade) const;
	int GetForceTypeWeight(int force_type) const;
	CCalendar *GetCalendar() const;
	CCurrency *GetCurrency() const;
	std::vector<CForceTemplate *> GetForceTemplates(int force_type) const;
	std::vector<CAiBuildingTemplate *> GetAiBuildingTemplates() const;
	std::map<int, std::vector<std::string>> &GetPersonalNames();
	std::vector<std::string> &GetUnitClassNames(int class_id);
	std::vector<std::string> &GetShipNames();
	
	int ID = -1;
	CCivilization *ParentCivilization = nullptr;
	std::string Ident;				/// ident of the civilization
	std::string Description;		/// civilization description
	std::string Quote;				/// civilization quote
	std::string Background;			/// civilization background
	std::string Adjective;			/// adjective pertaining to the civilization
	CUnitSound UnitSounds;			/// sounds for unit events
	CLanguage *Language = nullptr;	/// the language used by the civilization
	CCalendar *Calendar = nullptr;	/// the calendar used by the civilization
	CCurrency *Currency = nullptr;	/// the currency used by the civilization
	std::vector<CQuest *> Quests;	/// quests belonging to this civilization
	std::map<const CUpgrade *, int> UpgradePriorities;		/// Priority for each upgrade
	std::map<int, std::vector<CForceTemplate *>> ForceTemplates;	/// Force templates, mapped to each force type
	std::map<int, int> ForceTypeWeights;	/// Weights for each force type
	std::vector<CAiBuildingTemplate *> AiBuildingTemplates;	/// AI building templates
	std::map<int, std::vector<std::string>> PersonalNames;	/// Personal names for the civilization, mapped to the gender they pertain to (use NoGender for names which should be available for both genders)
	std::map<int, std::vector<std::string>> UnitClassNames;	/// Unit class names for the civilization, mapped to the unit class they pertain to, used for mechanical units, and buildings
	std::vector<std::string> FamilyNames;		/// Family names for the civilization
	std::vector<std::string> ProvinceNames;		/// Province names for the civilization
	std::vector<std::string> ShipNames;			/// Ship names for the civilization
	std::vector<CDeity *> Deities;
	std::vector<CSite *> Sites;					/// Sites used for this civilization if a randomly-generated one is required
	std::string MinisterTitles[MaxCharacterTitles][MaxGenders][MaxGovernmentTypes][MaxFactionTiers]; /// this civilization's minister title for each minister type and government type
	std::map<std::string, std::map<CDate, bool>> HistoricalUpgrades;	/// historical upgrades of the faction, with the date of change
};

//@}

#endif // !__CIVILIZATION_H__
