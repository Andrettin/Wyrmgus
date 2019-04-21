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
/**@name not_dependency.h - The not-dependency header file. */
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

#ifndef __NOT_DEPENDENCY_H__
#define __NOT_DEPENDENCY_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "dependency/dependency.h"

#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CConfigData;
class CPlayer;
class CUnit;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CNotDependency : public CDependency
{
public:
	CNotDependency() {}
	CNotDependency(const std::vector<const CDependency *> &dependencies) : Dependencies(dependencies) {}
	CNotDependency(const CDependency *dependency)
	{
		this->Dependencies.push_back(dependency);
	}
	
	virtual void ProcessConfigDataSection(const CConfigData *section) override;
	virtual bool Check(const CPlayer *player, const bool ignore_units = false) const override;
	virtual bool Check(const CUnit *unit, const bool ignore_units = false) const override;
	
	virtual std::string GetString(const std::string &prefix = "") const override
	{
		int element_count = 0;
		
		for (const CDependency *dependency : this->Dependencies) {
			if (!dependency->GetString(prefix + '\t').empty()) {
				element_count++;
			}
		}
		
		if (element_count >= 1) {
			std::string str = prefix + "NOT:\n";
		
			for (const CDependency *dependency : this->Dependencies) {
				str += dependency->GetString(prefix + '\t');
			}

			return str;
		} else {
			return std::string();
		}
	}

private:
	std::vector<const CDependency *> Dependencies;	/// The dependencies of which none should be true
};

#endif
