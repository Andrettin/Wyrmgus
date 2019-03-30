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
/**@name religion.h - The religion header file. */
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

#ifndef __RELIGION_H__
#define __RELIGION_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "database.h"
#include "data_element.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CConfigData;
class CDeityDomain;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CReligion : public DataElement, public Database<CReligion>
{
	GDCLASS(CReligion, DataElement)

public:
	static constexpr const char *GetClassIdentifier()
	{
		return "religion";
	}
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	
	std::string Name;							/// Name of the religion
	std::string Description;
	std::string Background;
	std::string Quote;
	bool CulturalDeities = false;				/// Whether the religion's deities (or equivalent) must belong to the civilization that has the religion; for instance: the deities under paganism must belong to the civilization of the player, but under hinduism they musn't (meaning that a Teuton player which has hinduism as a religion can select Hindu deities, but an Indian pagan cannot select Teuton pagan deities)
	std::vector<CDeityDomain *> Domains;

protected:
	static inline void _bind_methods() {}
};

#endif
