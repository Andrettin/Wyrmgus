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
	DATA_TYPE(CSpeciesCategoryRank, DataElement)
	
public:
	static constexpr const char *ClassIdentifier = "species_category_rank";
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	virtual void Initialize() override;
	
	CSpeciesCategoryRank *GetLowerRank() const
	{
		return this->LowerRank;
	}
	
	CSpeciesCategoryRank *GetUpperRank() const
	{
		return this->UpperRank;
	}
	
	std::vector<CSpeciesCategoryRank *> GetAllUpperRanks() const
	{
		if (this->UpperRank == nullptr) {
			return std::vector<CSpeciesCategoryRank *>();
		}
		
		std::vector<CSpeciesCategoryRank *> upper_ranks = this->UpperRank->GetAllUpperRanks();
		upper_ranks.push_back(this->UpperRank);
		
		return upper_ranks;
	}
	
	std::vector<CSpeciesCategoryRank *> GetAllLowerRanks() const
	{
		if (this->LowerRank == nullptr) {
			return std::vector<CSpeciesCategoryRank *>();
		}
		
		std::vector<CSpeciesCategoryRank *> lower_ranks = this->LowerRank->GetAllLowerRanks();
		lower_ranks.push_back(this->LowerRank);
		
		return lower_ranks;
	}
	
	bool IsLowerThan(const CSpeciesCategoryRank *other_rank) const
	{
		std::vector<CSpeciesCategoryRank *> upper_ranks = this->GetAllUpperRanks();
		
		if (std::find(upper_ranks.begin(), upper_ranks.end(), other_rank) != upper_ranks.end()) {
			return true;
		}
		
		return false;
	}
	
	bool IsUpperThan(const CSpeciesCategoryRank *other_rank) const
	{
		std::vector<CSpeciesCategoryRank *> lower_ranks = this->GetAllLowerRanks();
		
		if (std::find(lower_ranks.begin(), lower_ranks.end(), other_rank) != lower_ranks.end()) {
			return true;
		}
		
		return false;
	}
	
private:
	CSpeciesCategoryRank *LowerRank = nullptr;	/// the rank directly below this one
	CSpeciesCategoryRank *UpperRank = nullptr;	/// the rank directly above this one

protected:
	static inline void _bind_methods() {}
};

#endif
