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
/**@name age.h - The age header file. */
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

#ifndef __AGE_H__
#define __AGE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"
#include "detailed_data_element.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CDependency;
class CUpgrade;
class PaletteImage;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CAge : public DetailedDataElement, public DataType<CAge>
{
	GDCLASS(CAge, DetailedDataElement)
	
public:
	CAge(const std::string &ident = "", const int index = -1) : DetailedDataElement(ident, index)
	{
	}
	
	~CAge();
	
public:
	static constexpr const char *ClassIdentifier = "age";
	
	static void SetCurrentAge(const CAge *new_age);
	static void CheckCurrentAge();
	
	static const CAge *CurrentAge;
	
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	virtual void Initialize() override;
	
	int GetPriority() const
	{
		return this->Priority;
	}
	
	int GetYearBoost() const
	{
		return this->YearBoost;
	}
	
	const PaletteImage *GetImage() const
	{
		return this->Image;
	}
	
private:
	int Priority = 0;
	int YearBoost = 0;
	const PaletteImage *Image = nullptr;
public:
	CDependency *Predependency = nullptr;
	CDependency *Dependency = nullptr;

protected:
	static void _bind_methods();
};

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern void SetCurrentAge(const std::string &age_ident);

#endif
