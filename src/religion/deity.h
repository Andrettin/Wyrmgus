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
/**@name deity.h - The deity header file. */
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

#ifndef __DEITY_H__
#define __DEITY_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"
#include "ui/icon_config.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCivilization;
class CDeityDomain;
class CFaction;
class CGender;
class CPantheon;
class CPlane;
class CReligion;
class CUpgrade;
struct lua_State;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

constexpr int MAJOR_DEITY_DOMAIN_MAX = 3; /// major deities can only have up to three domains
constexpr int MINOR_DEITY_DOMAIN_MAX = 1; /// minor deities can only have one domain

class CDeity : public DataElement, public DataType<CDeity>
{
	DATA_TYPE(CDeity, DataElement)

public:
	static constexpr const char *ClassIdentifier = "deity";
	
	static CDeity *GetByUpgrade(const CUpgrade *upgrade, const bool should_find = true);
	static void Remove(CDeity *deity);
	static void Clear();
	
private:
	static std::map<const CUpgrade *, CDeity *> DeitiesByUpgrade;
	
public:
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value);
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	virtual void Initialize() override;
	
	const CGender *GetGender() const
	{
		return this->Gender;
	}
	
	bool IsMajor() const
	{
		return this->Major;
	}
	
	String GetCulturalName(const CCivilization *civilization) const;
	
	const std::vector<CDeityDomain *> &GetDomains() const
	{
		return this->Domains;
	}
	
private:
	const CGender *Gender = nullptr;			/// the deity's gender
	bool Major = false;							/// Whether the deity is a major one or not
public:
	std::string Description;
	std::string Background;
	std::string Quote;
	CPantheon *Pantheon = nullptr;				/// Pantheon to which the deity belongs
	CPlane *HomePlane = nullptr;				/// The home plane of the deity
	CUpgrade *DeityUpgrade = nullptr;			/// The deity's upgrade applied to a player that worships it
	CUpgrade *CharacterUpgrade = nullptr;		/// The deity's upgrade applied to its character as an individual upgrade
	IconConfig Icon;							/// Deity's icon
	std::vector<CCivilization *> Civilizations;	/// Civilizations which may worship the deity
	std::vector<CReligion *> Religions;			/// Religions for which this deity is available
	std::vector<std::string> Feasts;
	std::vector<CDeityDomain *> Domains;
	std::vector<CFaction *> HolyOrders;			/// Holy orders of this deity
	std::vector<CUpgrade *> Abilities;			/// Abilities linked to this deity
	std::map<const CCivilization *, String> CulturalNames;	/// Names of the deity in different cultures (for example, Odin is known as Hroptatyr by the dwarves)

	friend int CclDefineDeity(lua_State *l);
	
protected:
	static void _bind_methods();
};

#endif
