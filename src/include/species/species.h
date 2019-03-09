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
/**@name species.h - The species header file. */
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

#ifndef __SPECIES_H__
#define __SPECIES_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CPlane;
class CSpeciesCategory;
class CTerrainType;
class CUnitType;
class CWorld;
struct lua_State;

class CSpecies : public CDataType
{
	DATA_TYPE_CLASS(CSpecies)
	
public:
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	
	String GetName() const
	{
		return this->Name.c_str();
	}
	
	CSpeciesCategory *GetCategory() const
	{
		return this->Category;
	}
	
	bool IsNativeToTerrainType(const CTerrainType *terrain_type) const
	{
		return std::find(this->Terrains.begin(), this->Terrains.end(), terrain_type) != this->Terrains.end();
	}
	
	bool CanEvolveToAUnitType(const CTerrainType *terrain = nullptr, const bool sapient_only = false) const;
	CSpecies *GetRandomEvolution(const CTerrainType *terrain_type) const;
	
private:
	std::string Name;				/// name of the species
public:
	int Era = -1;					/// Era ID
	bool Sapient = false;			/// Whether the species is sapient
	bool Prehistoric = false;		/// Whether the species is prehistoric or not
	std::string Description;		/// Description of the species
	std::string Quote;				/// Quote pertaining to the species
	std::string Background;			/// Background of the species
private:
	CSpeciesCategory *Category = nullptr;
public:
	std::string Species;
	std::string ChildUpgrade;		/// Which individual upgrade the children of this species get
	CPlane *HomePlane = nullptr;
	CWorld *Homeworld = nullptr;
	CUnitType *Type = nullptr;
	std::vector<CTerrainType *> Terrains;	/// in which terrains does this species live
	std::vector<CSpecies *> EvolvesFrom;	/// from which species this one can evolve
	std::vector<CSpecies *> EvolvesTo;		/// to which species this one can evolve
	
	friend int CclDefineSpecies(lua_State *l);
};

#endif
