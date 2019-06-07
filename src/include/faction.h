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
/**@name faction.h - The faction header file. */
//
//      (c) Copyright 2019 by Andrettin
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

#ifndef __FACTION_H__
#define __FACTION_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "character.h" // because of "MaxCharacterTitles"
#include "data_type.h"
#include "detailed_data_element.h"
#include "time/date.h"
#include "ui/ui.h" // for the UI fillers

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CAiBuildingTemplate;
class CCivilization;
class Currency;
class CDeity;
class CDynasty;
class CForceTemplate;
class CGender;
class CPlayerColor;
class CUpgrade;
class CSite;
class FactionType;
class LuaCallback;
class UnitClass;
class UpgradeClass;

/*----------------------------------------------------------------------------
--  Enumerations
----------------------------------------------------------------------------*/

enum GovernmentTypes {
	GovernmentTypeNoGovernmentType,
	GovernmentTypeMonarchy,
	GovernmentTypeRepublic,
	GovernmentTypeTheocracy,
	
	MaxGovernmentTypes
};

enum FactionTiers {
	FactionTierNoFactionTier,
	FactionTierBarony,
	FactionTierCounty,
	FactionTierDuchy,
	FactionTierGrandDuchy,
	FactionTierKingdom,
	FactionTierEmpire,
	
	MaxFactionTiers
};

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CFaction : public DetailedDataElement, public DataType<CFaction>
{
	GDCLASS(CFaction, DetailedDataElement)
	
public:
	~CFaction();
	
	static constexpr const char *ClassIdentifier = "faction";
	
	static int GetFactionIndex(const std::string &faction_ident);
	static const CUnitType *GetFactionClassUnitType(const CFaction *faction, const UnitClass *unit_class);
	static const CUpgrade *GetFactionClassUpgrade(const CFaction *faction, const UpgradeClass *upgrade_class);
	static std::vector<CFiller> GetFactionUIFillers(const CFaction *faction);
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	virtual void Initialize() override;
	
	const CFaction *GetParentFaction() const { return this->ParentFaction; }
	
	CCivilization *GetCivilization() const { return this->Civilization; }
	
	const FactionType *GetType() const { return this->Type; }
	
	/**
	**	@brief	Get the faction's primary color
	**
	**	@return	The faction's primary color
	*/
	const CPlayerColor *GetPrimaryColor() const;
	
	const CPlayerColor *GetSecondaryColor() const;
	
	virtual CIcon *GetIcon() const override;

	const CUpgrade *GetUpgrade() const;
	
	const String &GetAdjective() const { return this->Adjective; }
	
	Currency *GetCurrency() const;
	int GetUpgradePriority(const CUpgrade *upgrade) const;
	int GetForceTypeWeight(int force_type) const;
	std::vector<CForceTemplate *> GetForceTemplates(int force_type) const;
	std::vector<CAiBuildingTemplate *> GetAiBuildingTemplates() const;
	const std::vector<std::string> &GetShipNames() const;
	
	const std::string &GetDefaultAI() const { return this->DefaultAI; }
	
	bool IsPlayable() const { return this->Playable; }

private:
	std::string FactionUpgrade;											/// faction upgrade applied when the faction is set
	String Adjective;													/// adjective pertaining to the faction
	std::string DefaultAI = "land-attack";
	CCivilization *Civilization = nullptr;								/// the faction's civilization
	const FactionType *Type = nullptr;									/// faction type (e.g. tribe or polity)
public:
	int DefaultTier = FactionTierBarony;								/// default faction tier
	int DefaultGovernmentType = GovernmentTypeMonarchy;					/// default government type
private:
	const CFaction *ParentFaction = nullptr;							/// the parent faction of this faction
	bool Playable = true;												/// faction playability
public:
	bool DefiniteArticle = false;										/// whether the faction's name should be preceded by a definite article (e.g. "the Netherlands")
private:
	Currency *Currency = nullptr;										/// the faction's currency
public:
	CDeity *HolyOrderDeity = nullptr;									/// deity this faction belongs to, if it is a holy order
	LuaCallback *Conditions = nullptr;
private:
	const CPlayerColor *PrimaryColor = nullptr;							/// the faction's primary player colors
	const CPlayerColor *SecondaryColor = nullptr;
public:
	std::vector<CFaction *> DevelopsFrom;								/// from which factions can this faction develop
	std::vector<CFaction *> DevelopsTo;									/// to which factions this faction can develop
	std::vector<CDynasty *> Dynasties;									/// which dynasties are available to this faction
	std::string Titles[MaxGovernmentTypes][MaxFactionTiers];			/// this faction's title for each government type and faction tier
	std::map<int, std::map<const CGender *, std::map<int, std::map<int, std::string>>>> MinisterTitles; /// this faction's minister title for each minister type and government type
	std::map<const CUpgrade *, int> UpgradePriorities;					/// Priority for each upgrade
	std::map<int, IconConfig> ButtonIcons;								/// icons for button actions
	std::map<const UnitClass *, const CUnitType *> ClassUnitTypes;		/// the unit type of a particular unit class for the faction
	std::map<const UpgradeClass *, const CUpgrade *> ClassUpgrades;		/// the upgrade slot of a particular class for the faction
	std::vector<std::string> ProvinceNames;								/// Province names for the faction
	std::vector<std::string> ShipNames;									/// Ship names for the faction
	std::vector<CSite *> Cores;											/// Core sites of this faction (required to found it)
	std::vector<CSite *> Sites;											/// Sites used for this faction if it needs a randomly-generated settlement
	std::map<int, std::vector<CForceTemplate *>> ForceTemplates;		/// Force templates, mapped to each force type
	std::map<int, int> ForceTypeWeights;								/// Weights for each force type
	std::vector<CAiBuildingTemplate *> AiBuildingTemplates;				/// AI building templates
	std::map<std::tuple<CDate, CDate, int>, CCharacter *> HistoricalMinisters;	/// historical ministers of the faction (as well as heads of state and government), mapped to the beginning and end of the rule, and the enum of the title in question
	std::map<std::string, std::map<CDate, bool>> HistoricalUpgrades;	/// historical upgrades of the faction, with the date of change
	std::map<int, int> HistoricalTiers;									/// dates in which this faction's tier changed; faction tier mapped to year
	std::map<int, int> HistoricalGovernmentTypes;						/// dates in which this faction's government type changed; government type mapped to year
	std::map<std::pair<CDate, const CFaction *>, int> HistoricalDiplomacyStates;	/// dates in which this faction's diplomacy state to another faction changed; diplomacy state mapped to year and faction
	std::map<std::pair<CDate, int>, int> HistoricalResources;	/// dates in which this faction's storage of a particular resource changed; resource quantities mapped to date and resource
	std::vector<std::pair<CDate, std::string>> HistoricalCapitals;		/// historical capitals of the faction; the values are: date and settlement ident
	std::vector<CFiller> UIFillers;
	
	std::string Mod;													/// To which mod (or map), if any, this faction belongs

	friend int CclDefineFaction(lua_State *l);
	
protected:
	static void _bind_methods();
};

#endif
