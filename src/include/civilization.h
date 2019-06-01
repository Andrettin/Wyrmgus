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

#include "character.h" // because of "MaxCharacterTitles"
#include "data_type.h"
#include "detailed_data_element.h"
#include "faction.h" //for certain enums
#include "time/date.h"
#include "ui/icon_config.h"
#include "ui/ui.h" // for the UI fillers

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CAiBuildingTemplate;
class CCalendar;
class Currency;
class CDeity;
class CForceTemplate;
class CGender;
class CLanguage;
class CPlayerColor;
class CQuest;
class CSpecies;
class CUpgrade;
class UnitClass;
class UpgradeClass;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CCivilization : public DetailedDataElement, public DataType<CCivilization>
{
	DATA_TYPE(CCivilization, DetailedDataElement)
	
public:
	~CCivilization();
	
	static constexpr const char *ClassIdentifier = "civilization";
	
	static const CUnitType *GetCivilizationClassUnitType(const CCivilization *civilization, const UnitClass *unit_class);
	static const CUpgrade *GetCivilizationClassUpgrade(const CCivilization *civilization, const UpgradeClass *upgrade_class);
	static std::vector<CFiller> GetCivilizationUIFillers(const CCivilization *civilization);
	
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	virtual void Initialize() override;
	
	int GetUpgradePriority(const CUpgrade *upgrade) const;
	int GetForceTypeWeight(const int force_type) const;
	
	/**
	**	@brief	Get the string identifier for the civilization's interface
	**
	**	@return	The string identifier for the civilization's interface
	*/
	const String &GetInterface() const
	{
		if (!this->Interface.empty()) {
			return this->Interface;
		}
		
		if (this->ParentCivilization != nullptr) {
			return this->ParentCivilization->GetInterface();
		}
		
		return this->Interface;
	}
	
	/**
	**	@brief	Get the civilization's species
	**
	**	@return	The civilization's species
	*/
	CSpecies *GetSpecies() const
	{
		if (this->Species != nullptr) {
			return this->Species;
		}
		
		if (this->ParentCivilization != nullptr) {
			return this->ParentCivilization->GetSpecies();
		}
		
		return nullptr;
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
		
		if (this->ParentCivilization != nullptr) {
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
	Currency *GetCurrency() const
	{
		if (this->Currency) {
			return this->Currency;
		}
		
		if (this->ParentCivilization != nullptr) {
			return this->ParentCivilization->GetCurrency();
		}
		
		return nullptr;
	}
	
	/**
	**	@brief	Get the civilization's upgrade
	**
	**	@return	The civilization's upgrade
	*/
	const CUpgrade *GetUpgrade() const;
	
	bool IsPlayable() const
	{
		return this->Playable;
	}
	
	CPlayerColor *GetDefaultPlayerColor() const
	{
		return this->DefaultPlayerColor;
	}
	
	/**
	**	@brief	Get the civilization's victory background file
	**
	**	@return	The civilization's victory background file
	*/
	const String &GetVictoryBackgroundFile() const
	{
		if (!this->VictoryBackgroundFile.empty()) {
			return this->VictoryBackgroundFile;
		}
		
		if (this->ParentCivilization != nullptr) {
			return this->ParentCivilization->GetVictoryBackgroundFile();
		}
		
		return this->VictoryBackgroundFile;
	}
	
	/**
	**	@brief	Get the civilization's defeat background file
	**
	**	@return	The civilization's defeat background file
	*/
	const String &GetDefeatBackgroundFile() const
	{
		if (!this->DefeatBackgroundFile.empty()) {
			return this->DefeatBackgroundFile;
		}
		
		if (this->ParentCivilization != nullptr) {
			return this->ParentCivilization->GetDefeatBackgroundFile();
		}
		
		return this->DefeatBackgroundFile;
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
	
	const std::map<const CGender *, std::vector<std::string>> &GetPersonalNames() const
	{
		if (this->PersonalNames.size() > 0) {
			return this->PersonalNames;
		}
		
		if (this->ParentCivilization != nullptr) {
			return this->ParentCivilization->GetPersonalNames();
		}
		
		return this->PersonalNames;
	}

	std::vector<std::string> GetUnitClassNames(const UnitClass *unit_class) const
	{
		std::map<const UnitClass *, std::vector<std::string>>::const_iterator find_iterator = this->UnitClassNames.find(unit_class);
		
		if (find_iterator != this->UnitClassNames.end() && !find_iterator->second.empty()) {
			return find_iterator->second;
		}
		
		if (this->ParentCivilization != nullptr) {
			return this->ParentCivilization->GetUnitClassNames(unit_class);
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
	
public:
	CCivilization *ParentCivilization = nullptr;
	std::string Adjective;			/// adjective pertaining to the civilization
private:
	String Interface;				/// the string identifier for the civilization's interface
public:
	CUnitSound UnitSounds;			/// sounds for unit events
private:
	CSpecies *Species = nullptr;	/// the civilization's species (e.g. human)
	CLanguage *Language = nullptr;	/// the language used by the civilization
	CCalendar *Calendar = nullptr;	/// the calendar used by the civilization
	Currency *Currency = nullptr;	/// the currency used by the civilization
	std::string Upgrade;			/// the string identifier for the civilization's upgrade
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
	std::map<const CGender *, std::vector<std::string>> PersonalNames;	/// Personal names for the civilization, mapped to the gender they pertain to (use nullptr for names which should be available for both genders)
	std::map<const UnitClass *, std::vector<std::string>> UnitClassNames;	/// Unit class names for the civilization, mapped to the unit class they pertain to, used for mechanical units, and buildings
	std::vector<std::string> FamilyNames;		/// Family names for the civilization
	std::vector<std::string> ProvinceNames;		/// Province names for the civilization
	std::vector<std::string> ShipNames;			/// Ship names for the civilization
	std::vector<CDeity *> Deities;
	std::vector<CSite *> Sites;					/// Sites used for this civilization if a randomly-generated one is required
	std::map<int, std::map<const CGender *, std::map<int, std::map<int, std::string>>>> MinisterTitles; /// this civilization's minister title for each minister type and government type
	std::map<std::string, std::map<CDate, bool>> HistoricalUpgrades;	/// historical upgrades of the faction, with the date of change
	std::map<const UnitClass *, const CUnitType *> ClassUnitTypes;		/// the unit type of a particular unit class for the civilization
	std::map<const UpgradeClass *, const CUpgrade *> ClassUpgrades;		/// the upgrade slot of a particular class for the civilization
private:
	String VictoryBackgroundFile;
	String DefeatBackgroundFile;
public:
	std::vector<CFiller> UIFillers;
	std::map<int, IconConfig> ButtonIcons;		/// icons for button actions
	
	friend int CclDefineCivilization(lua_State *l);

protected:
	static void _bind_methods();
};

#endif
