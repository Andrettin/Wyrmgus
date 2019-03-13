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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <map>
#include <string>
#include <vector>

#include <core/object.h>

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
class CPlayerColor;
class CQuest;
class CSpecies;
class CUpgrade;

class CCivilization : public Object
{
	GDCLASS(CCivilization, Object)
	
public:
	~CCivilization();
	
	static CCivilization *GetCivilization(const std::string &ident, const bool should_find = true);
	static CCivilization *GetOrAddCivilization(const std::string &ident);
	static void ClearCivilizations();
	
	static std::vector<CCivilization *> Civilizations;    					/// civilizations
	static std::map<std::string, CCivilization *> CivilizationsByIdent;
	
	int GetUpgradePriority(const CUpgrade *upgrade) const;
	int GetForceTypeWeight(const int force_type) const;
	
	/**
	**	@brief	Get the civilization's string identifier
	**
	**	@return	The civilization's string identifier
	*/
	String GetIdent() const
	{
		return this->Ident.c_str();
	}
	
	/**
	**	@brief	Get the civilization's name
	**
	**	@return	The civilization's name
	*/
	String GetName() const
	{
		return this->Name.c_str();
	}
	
	/**
	**	@brief	Get the string identifier for the civilization's interface
	**
	**	@return	The string identifier for the civilization's interface
	*/
	String GetInterface() const
	{
		return this->Interface.c_str();
	}
	
	/**
	**	@brief	Get the civilization's species
	**
	**	@return	The civilization's species
	*/
	CSpecies *GetSpecies() const
	{
		return this->Species;
	}
	
	/**
	**	@brief	Get the civilization's language
	**
	**	@return	The civilization's language
	*/
	CLanguage *GetLanguage() const
	{
		if (this->Language) {
			return this->Language;
		}
		
		if (this->ParentCivilization) {
			return this->ParentCivilization->GetLanguage();
		}
		
		return nullptr;
	}
	
	/**
	**	@brief	Get the calendar for the civilization
	**
	**	@return	The civilization's calendar
	*/
	CCalendar *GetCalendar() const;
	
	/**
	**	@brief	Get the civilization's currency
	**
	**	@return	The civilization's currency
	*/
	CCurrency *GetCurrency() const
	{
		if (this->Currency) {
			return this->Currency;
		}
		
		if (this->ParentCivilization) {
			return this->ParentCivilization->GetCurrency();
		}
		
		return nullptr;
	}
	
	bool IsHidden() const
	{
		return this->Hidden;
	}
	
	bool IsPlayable() const
	{
		return this->Playable;
	}
	
	CPlayerColor *GetDefaultPlayerColor() const
	{
		return this->DefaultPlayerColor;
	}
	
	std::vector<CForceTemplate *> GetForceTemplates(const int force_type) const;
	
	std::vector<CAiBuildingTemplate *> GetAiBuildingTemplates() const
	{
		if (this->AiBuildingTemplates.size() > 0) {
			return this->AiBuildingTemplates;
		}
		
		if (this->ParentCivilization) {
			return this->ParentCivilization->GetAiBuildingTemplates();
		}
		
		return std::vector<CAiBuildingTemplate *>();
	}
	
	const std::map<int, std::vector<std::string>> &GetPersonalNames() const
	{
		if (this->PersonalNames.size() > 0) {
			return this->PersonalNames;
		}
		
		if (this->ParentCivilization) {
			return this->ParentCivilization->GetPersonalNames();
		}
		
		return this->PersonalNames;
	}

	std::vector<std::string> GetUnitClassNames(const int class_id) const
	{
		std::map<int, std::vector<std::string>>::const_iterator find_iterator = this->UnitClassNames.find(class_id);
		
		if (find_iterator != this->UnitClassNames.end() && !find_iterator->second.empty()) {
			return find_iterator->second;
		}
		
		if (this->ParentCivilization != nullptr) {
			return this->ParentCivilization->GetUnitClassNames(class_id);
		}
		
		return std::vector<std::string>();
	}
	
	const std::vector<std::string> &GetShipNames() const
	{
		if (!this->ShipNames.empty()) {
			return this->ShipNames;
		}
		
		if (this->ParentCivilization) {
			return this->ParentCivilization->GetShipNames();
		}
		
		return this->ShipNames;
	}
	
	int ID = -1;
private:
	std::string Ident;				/// ident of the civilization
	std::string Name;				/// name of the civilization
public:
	CCivilization *ParentCivilization = nullptr;
	std::string Description;		/// civilization description
	std::string Quote;				/// civilization quote
	std::string Background;			/// civilization background
	std::string Adjective;			/// adjective pertaining to the civilization
private:
	std::string Interface;			/// the string identifier for the civilization's interface
public:
	CUnitSound UnitSounds;			/// sounds for unit events
private:
	CSpecies *Species = nullptr;	/// the civilization's species (e.g. human)
	CLanguage *Language = nullptr;	/// the language used by the civilization
	CCalendar *Calendar = nullptr;	/// the calendar used by the civilization
	CCurrency *Currency = nullptr;	/// the currency used by the civilization
private:
	bool Hidden = false;			/// whether the civilization is hidden
	bool Playable = true;			/// whether the civilization is playable
public:
	CPlayerColor *DefaultPlayerColor = nullptr;	/// name of the civilization's default color (used for the encyclopedia, tech tree, etc.)
	std::vector<CCivilization *> DevelopsFrom;	/// from which civilizations this civilization develops
	std::vector<CCivilization *> DevelopsTo;	/// to which civilizations this civilization develops
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
	
	friend int CclDefineCivilization(lua_State *l);

protected:
	static void _bind_methods();
};

#endif
