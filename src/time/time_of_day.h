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
/**@name time_of_day.h - The time of day header file. */
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

#ifndef __TIME_OF_DAY_H__
#define __TIME_OF_DAY_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"
#include "video/color.h"

#include <core/object.h>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CGraphic;

class CTimeOfDay : public CDataType
{
	GDCLASS(CTimeOfDay, CDataType)
	DATA_TYPE_CLASS(CTimeOfDay)
	
public:
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;

	String GetIdent() const
	{
		return CDataType::GetIdent();
	}
	
	String GetName() const
	{
		return this->Name.c_str();
	}
	
	bool IsDawn() const
	{
		return this->Dawn;
	}
	
	bool IsDay() const
	{
		return this->Day;
	}
	
	bool IsDusk() const
	{
		return this->Dusk;
	}
	
	bool IsNight() const
	{
		return this->Night;
	}
	
	CGraphic *GetGraphic() const
	{
		return this->G;
	}
	
	/**
	**	@brief	Gets whether the time of day modifies the color of graphics
	**
	**	@return	Whether the time of day modifies the color of graphics
	*/
	bool HasColorModification() const
	{
		return this->ColorModification.R != 0 || this->ColorModification.G != 0 || this->ColorModification.B != 0;
	}
	
	const CColor &GetColorModification() const
	{
		return this->ColorModification;
	}
	
private:
	std::string Name;							/// Name of the time of day
	bool Dawn = false;							/// Whether this is a dawn time of day
	bool Day = false;							/// Whether this is a day time of day
	bool Dusk = false;							/// Whether this is a dusk time of day
	bool Night = false;							/// Whether this is a night time of day
	CColor ColorModification;					/// The color modification applied to graphics when the time of day is active
	CGraphic *G = nullptr;

protected:
	static void _bind_methods();
};

#endif
