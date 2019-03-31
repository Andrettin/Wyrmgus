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

#include "data_element.h"
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

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CSpecies : public DataElement, public DataType<CSpecies>
{
	GDCLASS(CSpecies, DataElement)
	
public:
	static constexpr const char *ClassIdentifier = "species";
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	virtual void Initialize() override;
	
	String GetName() const
	{
		return this->Name.c_str();
	}
	
	CSpeciesCategory *GetCategory() const
	{
		return this->Category;
	}
	
	String GetScientificName() const
	{
		return this->ScientificName.c_str();
	}
	
	bool IsSapient() const
	{
		return this->Sapient;
	}
	
	bool IsPrehistoric() const
	{
		return this->Prehistoric;
	}
	
	CPlane *GetHomePlane() const
	{
		return this->HomePlane;
	}
	
	CWorld *GetHomeworld() const
	{
		return this->Homeworld;
	}
	
	CUnitType *GetUnitType() const
	{
		return this->UnitType;
	}
	
	const std::vector<CTerrainType *> &GetNativeTerrainTypes() const
	{
		return this->NativeTerrainTypes;
	}
	
	bool IsNativeToTerrainType(const CTerrainType *terrain_type) const
	{
		return std::find(this->NativeTerrainTypes.begin(), this->NativeTerrainTypes.end(), terrain_type) != this->NativeTerrainTypes.end();
	}
	
	bool CanEvolveToAUnitType(const CTerrainType *terrain = nullptr, const bool sapient_only = false) const;
	CSpecies *GetRandomEvolution(const CTerrainType *terrain_type) const;
	
private:
	std::string Name;				/// name of the species
	CSpeciesCategory *Category = nullptr;
	std::string ScientificName;		/// The scientific name of the species
	std::string Description;		/// Description of the species
	std::string Quote;				/// Quote pertaining to the species
	std::string Background;			/// Background of the species
	int Era = -1;					/// Era ID
	bool Sapient = false;			/// Whether the species is sapient
	bool Prehistoric = false;		/// Whether the species is prehistoric or not
public:
	std::string ChildUpgrade;		/// Which individual upgrade the children of this species get
private:
	CPlane *HomePlane = nullptr;
	CWorld *Homeworld = nullptr;
	CUnitType *UnitType = nullptr;
	std::vector<CTerrainType *> NativeTerrainTypes;	/// in which terrains does this species live
	std::vector<CSpecies *> EvolvesFrom;	/// from which species this one can evolve
	std::vector<CSpecies *> EvolvesTo;		/// to which species this one can evolve
	
	friend int CclDefineSpecies(lua_State *l);
	friend int CclDefineUnitType(lua_State *l);
	friend class CUnitType;	// necessary so that the unit type may set the species' type to itself

protected:
	static inline void _bind_methods() {}
};

#endif
