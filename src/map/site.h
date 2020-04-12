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
/**@name site.h - The site header file. */
//
//      (c) Copyright 2018-2020 by Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"
#include "time/date.h"
#include "vec2i.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CFaction;
class CMapTemplate;
class CRegion;
class CUnit;
class CUnitType;
class CUniqueItem;

namespace stratagus {
	class civilization;
}

class CSite : public CDataType
{
public:
	static CSite *GetSite(const std::string &ident);
	static CSite *GetOrAddSite(const std::string &ident);
	static void ClearSites();
	
	static std::vector<CSite *> Sites;							/// Sites
	static std::map<std::string, CSite *> SitesByIdent;

	virtual void ProcessConfigData(const CConfigData *config_data) override;
	std::string GetCulturalName(const stratagus::civilization *civilization) const;

	std::string Name;
	bool Major = false;											/// Whether the site is a major one; major sites have settlement sites, and as such can have town halls
	Vec2i Position = Vec2i(-1, -1);								/// Position of the site in its map template
	CMapTemplate *MapTemplate = nullptr;						/// Map template where this site is located
	CUnit *SiteUnit = nullptr;									/// Unit which represents this site
	std::vector<CRegion *> Regions;								/// Regions where this site is located
	std::vector<CFaction *> Cores;						/// Factions which have this site as a core
	std::map<const stratagus::civilization *, std::string> CulturalNames;	/// Names for the site for each different culture/civilization
	std::map<CDate, const CFaction *> HistoricalOwners;				/// Historical owners of the site
	std::map<CDate, int> HistoricalPopulation;					/// Historical population
	std::vector<std::tuple<CDate, CDate, const CUnitType *, int, const CFaction *>> HistoricalUnits;	/// Historical quantity of a particular unit type (number of people for units representing a person)
	std::vector<std::tuple<CDate, CDate, int, CUniqueItem *, const CFaction *>> HistoricalBuildings; /// Historical buildings, with start and end date
	std::vector<std::tuple<CDate, CDate, const CUnitType *, CUniqueItem *, int>> HistoricalResources; /// Historical resources, with start and end date; the integer at the end is the resource quantity
};
