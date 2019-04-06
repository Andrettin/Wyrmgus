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
/**@name deity_domain.h - The deity domain header file. */
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

#ifndef __DEITY_DOMAIN_H__
#define __DEITY_DOMAIN_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"
#include "detailed_data_element.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CUpgrade;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CDeityDomain : public DetailedDataElement, public DataType<CDeityDomain>
{
	DATA_TYPE(CDeityDomain, DetailedDataElement)

public:
	static constexpr const char *ClassIdentifier = "deity_domain";
	
	static CDeityDomain *GetByUpgrade(const CUpgrade *upgrade, const bool should_find = true);
	static void Remove(CDeityDomain *deity_domain);
	static void Clear();
	
private:
	static std::map<const CUpgrade *, CDeityDomain *> DeityDomainsByUpgrade;

public:
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	
	CUpgrade *Upgrade = nullptr;						/// Upgrade corresponding to the domain
	std::vector<CUpgrade *> Abilities;					/// Abilities linked to this domain

protected:
	static inline void _bind_methods() {}
};

#endif
