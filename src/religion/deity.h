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

#include "data_type.h"
#include "ui/icon_config.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCivilization;
class CDeityDomain;
class CFaction;
class CPantheon;
class CPlane;
class CReligion;
class CUpgrade;

#define MAJOR_DEITY_DOMAIN_MAX 3 // major deities can only have up to three domains
#define MINOR_DEITY_DOMAIN_MAX 1 // minor deities can only have one domain

class CDeity : public CDataType
{
public:
	static CDeity *GetDeity(const std::string &ident, const bool should_find = true);
	static CDeity *GetOrAddDeity(const std::string &ident);
	static CDeity *GetDeityByUpgrade(const CUpgrade *upgrade, const bool should_find = true);
	static void ClearDeities();
	
	static std::vector<CDeity *> Deities;		/// Deities
	static std::map<std::string, CDeity *> DeitiesByIdent;
	static std::map<const CUpgrade *, CDeity *> DeitiesByUpgrade;
	
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	
	std::string GetCulturalName(const CCivilization *civilization) const;
	
	int Gender = 0;								/// Deity's gender
	bool Major = false;							/// Whether the deity is a major one or not
	std::string Name;							/// Name of the deity
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
	std::map<const CCivilization *, std::string> CulturalNames;	/// Names of the deity in different cultures (for example, Odin is known as Hroptatyr by the dwarves)
};

#endif
