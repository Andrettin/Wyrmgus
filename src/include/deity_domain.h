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
//      (c) Copyright 2018 by Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <map>
#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CConfigData;
class CUpgrade;

class CDeityDomain
{
public:
	CDeityDomain() :
		Upgrade(NULL)
	{
	}
	
	static CDeityDomain *GetDeityDomain(std::string deity_domain_ident);
	static CDeityDomain *GetOrAddDeityDomain(std::string deity_domain_ident);
	static CDeityDomain *GetDeityDomainByUpgrade(const CUpgrade *upgrade);
	static void ClearDeityDomains();
	
	static std::vector<CDeityDomain *> DeityDomains;	/// Deity domains
	static std::map<std::string, CDeityDomain *> DeityDomainsByIdent;
	static std::map<const CUpgrade *, CDeityDomain *> DeityDomainsByUpgrade;
	
	std::string Ident;									/// Ident of the domain
	std::string Name;									/// Name of the domain
	CUpgrade *Upgrade;									/// Upgrade corresponding to the domain
	std::vector<CUpgrade *> Abilities;					/// Abilities linked to this domain
};

//@}

#endif // !__DEITY_DOMAIN_H__
