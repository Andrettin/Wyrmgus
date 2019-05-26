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

#ifndef __SITE_H__
#define __SITE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"
#include "time/date.h"
#include "vec2i.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCivilization;
class CFaction;
class CMapLayer;
class CMapTemplate;
class CRegion;
class CUnit;
class CUnitType;
class CWord;
class UniqueItem;
class UnitClass;
struct lua_State;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CSite : public DataElement, public DataType<CSite>
{
	DATA_TYPE(CSite, DataElement)
	
public:	
	static constexpr const char *ClassIdentifier = "site";
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	virtual void Initialize() override;

	const String &GetCulturalName(const CCivilization *civilization) const;
	
	bool IsMajor() const
	{
		return this->Major;
	}
	
	Vec2i GetMapPos() const;
	CMapLayer *GetMapLayer() const;

private:
	const CWord *NameWord = nullptr;
	bool Major = false;						/// whether the site is a major one; major sites have settlement sites, and as such can have town halls
public:
	Vec2i Position = Vec2i(-1, -1);			/// position of the site in its map template
	CMapTemplate *MapTemplate = nullptr;	/// map template where this site is located
	CUnit *SiteUnit = nullptr;				/// unit which represents this site
	std::vector<CRegion *> Regions;			/// regions where this site is located
	std::vector<CFaction *> Cores;			/// factions which have this site as a core
	std::map<const CCivilization *, String> CulturalNames;	/// names for the site for each different culture/civilization
	std::map<const CCivilization *, const CWord *> CulturalNameWords;	/// name words for the site for each different culture/civilization
	std::map<CDate, const CFaction *> HistoricalOwners;			/// historical owners of the site
	std::map<CDate, int> HistoricalPopulation;					/// historical population
	std::vector<std::tuple<CDate, CDate, const CUnitType *, int, const CFaction *>> HistoricalUnits;	/// historical quantity of a particular unit type (number of people for units representing a person)
	std::vector<std::tuple<CDate, CDate, const UnitClass *, UniqueItem *, const CFaction *>> HistoricalBuildings;	/// historical buildings, with start and end date
	std::vector<std::tuple<CDate, CDate, const CUnitType *, UniqueItem *, int>> HistoricalResources;	/// historical resources, with start and end date; the integer at the end is the resource quantity
	
	friend int CclDefineSite(lua_State *l);

protected:
	static void _bind_methods();
};

#endif
