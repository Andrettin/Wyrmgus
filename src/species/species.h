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

#include <set>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CGender;
class CPlane;
class CSpeciesCategory;
class CTerrainType;
class CUnitType;
class CWord;
class CWorld;
struct lua_State;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CSpecies : public DetailedDataElement, public DataType<CSpecies>
{
	GDCLASS(CSpecies, DetailedDataElement)
	
public:
	static constexpr const char *ClassIdentifier = "species";
	
	virtual bool ProcessConfigDataProperty(const String &key, String value) override;
	virtual void Initialize() override;
	
	const CSpeciesCategory *GetCategory() const { return this->Category; }
	
	CPlane *GetHomePlane() const { return this->HomePlane; }
	
	CWorld *GetHomeworld() const { return this->Homeworld; }
	
	CUnitType *GetUnitType() const { return this->UnitType; }
	
	bool IsSapient() const { return this->Sapient; }
	
	bool IsPrehistoric() const { return this->Prehistoric; }
	
	const std::vector<const CGender *> &GetGenders() const;
	
	const std::set<const CTerrainType *> &GetNativeTerrainTypes() const { return this->NativeTerrainTypes; }
	
	bool IsNativeToTerrainType(const CTerrainType *terrain_type) const
	{
		return this->NativeTerrainTypes.find(terrain_type) != this->NativeTerrainTypes.end();
	}
	
	const std::vector<CSpecies *> &GetEvolvesFrom() const { return this->EvolvesFrom; }
	
	const std::vector<CSpecies *> &GetEvolvesTo() const { return this->EvolvesTo; }
	
	bool CanEvolveToAUnitType(const CTerrainType *terrain = nullptr, const bool sapient_only = false) const;
	CSpecies *GetRandomEvolution(const CTerrainType *terrain_type) const;
	std::vector<const CSpeciesCategory *> GetAllCategories() const;
	
	void AddSpecimenNameWord(CWord *word, const CGender *gender);
	const std::vector<CWord *> &GetSpecimenNameWords(const CGender *gender);
	
private:
	String NamePlural;
	CSpeciesCategory *Category = nullptr;
	String ScientificName;			/// the scientific name of the species
	int Era = -1;					/// era ID
	bool Sapient = false;			/// whether the species is sapient
	bool Prehistoric = false;		/// whether the species is prehistoric or not
public:
	std::string ChildUpgrade;		/// Which individual upgrade the children of this species get
private:
	CPlane *HomePlane = nullptr;
	CWorld *Homeworld = nullptr;
	CUnitType *UnitType = nullptr;
	std::vector<const CGender *> Genders;
	std::set<const CTerrainType *> NativeTerrainTypes;	/// in which terrains does this species live
	std::vector<CSpecies *> EvolvesFrom;			/// from which species this one can evolve
	std::vector<CSpecies *> EvolvesTo;				/// to which species this one can evolve
	
private:
	std::map<const CGender *, std::vector<CWord *>> SpecimenNameWords;	/// the words used for specimen name generation, mapped to the gender for which they can be used
	
	friend int CclDefineSpecies(lua_State *l);
	friend int CclDefineUnitType(lua_State *l);
	friend class CUnitType;	// necessary so that the unit type may set the species' type to itself

protected:
	static void _bind_methods();
};

#endif
