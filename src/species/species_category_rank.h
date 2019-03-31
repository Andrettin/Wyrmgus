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
/**@name species_category_rank.h - The species category rank header file. */
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

#ifndef __SPECIES_CATEGORY_RANK_H__
#define __SPECIES_CATEGORY_RANK_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CSpeciesCategoryRank : public DataElement, public DataType<CSpeciesCategoryRank>
{
	GDCLASS(CSpeciesCategoryRank, DataElement)
	
public:
	static constexpr const char *ClassIdentifier = "species_category_rank";
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	virtual void Initialize() override;
	
	String GetName() const
	{
		return this->Name.c_str();
	}
	
	CSpeciesCategoryRank *GetLowerRank() const
	{
		return this->LowerRank;
	}
	
	CSpeciesCategoryRank *GetUpperRank() const
	{
		return this->UpperRank;
	}
	
private:
	std::string Name;				/// name of the species category rank
	CSpeciesCategoryRank *LowerRank = nullptr;	/// the rank directly below this one
	CSpeciesCategoryRank *UpperRank = nullptr;	/// the rank directly above this one

protected:
	static inline void _bind_methods() {}
};

#endif
