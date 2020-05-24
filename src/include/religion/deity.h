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
//      (c) Copyright 2018-2020 by Andrettin
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

#pragma once

#include "database/data_type.h"
#include "database/detailed_data_entry.h"
#include "data_type.h"
#include "ui/icon.h"

class CDeityDomain;
class CPantheon;
class CReligion;
class CUpgrade;
struct lua_State;

int CclDefineDeity(lua_State *l);

namespace stratagus {
	class civilization;
	class faction;
	class plane;
	enum class gender;
}

static constexpr int MAJOR_DEITY_DOMAIN_MAX = 3; //major deities can only have up to three domains
static constexpr int MINOR_DEITY_DOMAIN_MAX = 1; //minor deities can only have one domain

namespace stratagus {

class deity : public detailed_data_entry, public data_type<deity>, public CDataType
{
public:
	static constexpr const char *class_identifier = "deity";
	static constexpr const char *database_folder = "deities";

	static deity *get_by_upgrade(const CUpgrade *upgrade)
	{
		auto find_iterator = deity::deities_by_upgrade.find(upgrade);
		if (find_iterator != deity::deities_by_upgrade.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}
	
private:
	static inline std::map<const CUpgrade *, deity *> deities_by_upgrade;

public:
	explicit deity(const std::string &identifier);
	
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	
	std::string GetCulturalName(const civilization *civilization) const;

	plane *get_home_plane() const
	{
		return this->home_plane;
	}
	
	stratagus::gender gender;					//deity's gender
	bool Major = false;							//whether the deity is a major one or not
	CPantheon *Pantheon = nullptr;				//pantheon to which the deity belongs
private:
	plane *home_plane = nullptr;				//the home plane of the deity
public:
	CUpgrade *DeityUpgrade = nullptr;			//the deity's upgrade applied to a player that worships it
	CUpgrade *CharacterUpgrade = nullptr;		//the deity's upgrade applied to its character as an individual upgrade
	IconConfig Icon;							//deity's icon
	std::vector<civilization *> civilizations;	//civilizations which may worship the deity
	std::vector<CReligion *> Religions;			//religions for which this deity is available
	std::vector<std::string> Feasts;
	std::vector<CDeityDomain *> Domains;
	std::vector<faction *> HolyOrders;			//holy orders of this deity
	std::vector<CUpgrade *> Abilities;			//abilities linked to this deity
	std::map<const civilization *, std::string> CulturalNames;	//names of the deity in different cultures (for example, Odin is known as Hroptatyr by the dwarves)

	friend int ::CclDefineDeity(lua_State *l);
};

}