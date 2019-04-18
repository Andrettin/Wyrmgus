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
#include "detailed_data_element.h"

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

class CSpecies : public DetailedDataElement, public DataType<CSpecies>
{
	DATA_TYPE(CSpecies, DetailedDataElement)
	
public:
	static constexpr const char *ClassIdentifier = "species";
	
private:
	static inline bool InitializeClass()
	{
		REGISTER_PROPERTY(Category);
		REGISTER_PROPERTY(EvolvesFrom);
		REGISTER_PROPERTY(EvolvesTo);
		REGISTER_PROPERTY(NamePlural);
		REGISTER_PROPERTY(NativeTerrainTypes);
		REGISTER_PROPERTY(Prehistoric);
		REGISTER_PROPERTY(Sapient);
		REGISTER_PROPERTY(ScientificName);
		
		return true;
	}
	
	static inline bool ClassInitialized = InitializeClass();

public:
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	virtual void Initialize() override;
	
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
	
	bool IsNativeToTerrainType(const CTerrainType *terrain_type) const
	{
		return std::find(this->NativeTerrainTypes.begin(), this->NativeTerrainTypes.end(), terrain_type) != this->NativeTerrainTypes.end();
	}
	
	bool CanEvolveToAUnitType(const CTerrainType *terrain = nullptr, const bool sapient_only = false) const;
	CSpecies *GetRandomEvolution(const CTerrainType *terrain_type) const;
	std::vector<CSpeciesCategory *> GetAllCategories() const;
	
public:
	ExposedProperty<String> NamePlural;
	ExposedProperty<CSpeciesCategory *> Category = nullptr;
	ExposedProperty<String> ScientificName;		/// the scientific name of the species
private:
	int Era = -1;								/// era ID
public:
	ExposedProperty<bool> Sapient = false;		/// whether the species is sapient
	ExposedProperty<bool> Prehistoric = false;	/// whether the species is prehistoric or not
public:
	std::string ChildUpgrade;		/// Which individual upgrade the children of this species get
private:
	CPlane *HomePlane = nullptr;
	CWorld *Homeworld = nullptr;
	CUnitType *UnitType = nullptr;
public:
	ExposedProperty<std::vector<CTerrainType *>> NativeTerrainTypes;	/// in which terrains does this species live
	;	
	ExposedProperty<std::vector<CSpecies *>> EvolvesFrom {		/// from which species this one can evolve
		ExposedProperty<std::vector<CSpecies *>>::ValueType(),
		ExposedProperty<std::vector<CSpecies *>>::AdderType([this](CSpecies *species) {
			(*this->EvolvesFrom.Value).push_back(species);
			(*species->EvolvesTo.Value).push_back(this);
		}),
		ExposedProperty<std::vector<CSpecies *>>::RemoverType([this](CSpecies *species) {
			(*this->EvolvesFrom.Value).erase(std::remove((*this->EvolvesFrom.Value).begin(), (*this->EvolvesFrom.Value).end(), species), (*this->EvolvesFrom.Value).end());
			(*species->EvolvesTo.Value).erase(std::remove((*species->EvolvesTo.Value).begin(), (*species->EvolvesTo.Value).end(), this), (*species->EvolvesTo.Value).end());
		})
	};
	ExposedProperty<std::vector<CSpecies *>> EvolvesTo {		/// to which species this one can evolve
		ExposedProperty<std::vector<CSpecies *>>::ValueType(),
		ExposedProperty<std::vector<CSpecies *>>::AdderType([this](CSpecies *species) {
			(*this->EvolvesTo.Value).push_back(species);
			(*species->EvolvesFrom.Value).push_back(this);
		}),
		ExposedProperty<std::vector<CSpecies *>>::RemoverType([this](CSpecies *species) {
			(*this->EvolvesTo.Value).erase(std::remove((*this->EvolvesTo.Value).begin(), (*this->EvolvesTo.Value).end(), species), (*this->EvolvesTo.Value).end());
			(*species->EvolvesFrom.Value).erase(std::remove((*species->EvolvesFrom.Value).begin(), (*species->EvolvesFrom.Value).end(), this), (*species->EvolvesFrom.Value).end());
		})
	};
	
private:
	friend int CclDefineSpecies(lua_State *l);
	friend int CclDefineUnitType(lua_State *l);
	friend class CUnitType;	// necessary so that the unit type may set the species' type to itself

protected:
	static void _bind_methods();
};

#endif
