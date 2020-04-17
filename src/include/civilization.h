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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "database/data_type.h"
#include "database/detailed_data_entry.h"
#include "player.h" //for certain enums
#include "time/date.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CAiBuildingTemplate;
class CCalendar;
class CCurrency;
class CDeity;
class CForceTemplate;
class CLanguage;
class CQuest;
class CUpgrade;
struct lua_State;

int CclDefineCivilization(lua_State *l);

namespace stratagus {

class unit_class;

class civilization final : public detailed_data_entry, public data_type<civilization>
{
	Q_OBJECT

	Q_PROPERTY(bool visible MEMBER visible READ is_visible)
	Q_PROPERTY(bool playable MEMBER playable READ is_playable)
	Q_PROPERTY(QString interface READ get_interface_qstring WRITE set_interface_qstring)
	Q_PROPERTY(QString default_color READ get_default_color_qstring WRITE set_default_color_qstring)
	Q_PROPERTY(QStringList ship_names READ get_ship_names_qstring_list)

public:
	static constexpr const char *class_identifier = "civilization";
	static constexpr const char *database_folder = "civilizations";

	static civilization *add(const std::string &identifier, const stratagus::module *module)
	{
		civilization *civilization = data_type::add(identifier, module);
		civilization->ID = civilization::get_all().size() - 1;
		return civilization;
	}

	civilization(const std::string &identifier) : detailed_data_entry(identifier)
	{
	}

	~civilization();

	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;
	
	int GetUpgradePriority(const CUpgrade *upgrade) const;
	int GetForceTypeWeight(int force_type) const;

	const std::string &get_interface() const
	{
		return this->interface;
	}

	QString get_interface_qstring() const
	{
		return QString::fromStdString(this->interface);
	}

	void set_interface_qstring(const QString &interface)
	{
		this->interface = interface.toStdString();
	}

	const std::string &get_default_color() const
	{
		return this->default_color;
	}

	QString get_default_color_qstring() const
	{
		return QString::fromStdString(this->default_color);
	}

	void set_default_color_qstring(const QString &default_color)
	{
		this->default_color = default_color.toStdString();
	}

	CCalendar *GetCalendar() const;
	CCurrency *GetCurrency() const;

	bool is_visible() const
	{
		return this->visible;
	}

	bool is_playable() const
	{
		return this->playable;
	}

	std::vector<CForceTemplate *> GetForceTemplates(int force_type) const;
	std::vector<CAiBuildingTemplate *> GetAiBuildingTemplates() const;
	const std::map<int, std::vector<std::string>> &GetPersonalNames() const;
	const std::vector<std::string> &get_unit_class_names(const unit_class *unit_class) const;

	const std::vector<std::string> &get_ship_names() const;

	QStringList get_ship_names_qstring_list() const;

	Q_INVOKABLE void add_ship_name(const std::string &ship_name)
	{
		this->ship_names.push_back(ship_name);
	}

	Q_INVOKABLE void remove_ship_name(const std::string &ship_name);

	CUnitType *get_class_unit_type(const unit_class *unit_class) const;

	void set_class_unit_type(const unit_class *unit_class, CUnitType *unit_type)
	{
		if (unit_type == nullptr) {
			this->class_unit_types.erase(unit_class);
			return;
		}

		this->class_unit_types[unit_class] = unit_type;
	}

	void remove_class_unit_type(CUnitType *unit_type)
	{
		for (std::map<const unit_class *, CUnitType *>::reverse_iterator iterator = this->class_unit_types.rbegin(); iterator != this->class_unit_types.rend(); ++iterator) {
			if (iterator->second == unit_type) {
				this->class_unit_types.erase(iterator->first);
			}
		}
	}

	int ID = -1;
	civilization *parent_civilization = nullptr;
	std::string Adjective;			/// adjective pertaining to the civilization
private:
	std::string interface; //the string identifier for the civilization's interface
	std::string default_color; //name of the civilization's default color (used for the encyclopedia, tech tree, etc.)
public:
	CUnitSound UnitSounds;			/// sounds for unit events
	CLanguage *Language = nullptr;	/// the language used by the civilization
	CCalendar *Calendar = nullptr;	/// the calendar used by the civilization
	CCurrency *Currency = nullptr;	/// the currency used by the civilization
private:
	bool visible = true; //whether the civilization is visible e.g. in the map editor
	bool playable = true; //civilizations are playable by default
public:
	std::vector<CQuest *> Quests;	/// quests belonging to this civilization
	std::map<const CUpgrade *, int> UpgradePriorities;		/// Priority for each upgrade
	std::map<int, std::vector<CForceTemplate *>> ForceTemplates;	/// Force templates, mapped to each force type
	std::map<int, int> ForceTypeWeights;	/// Weights for each force type
	std::vector<CAiBuildingTemplate *> AiBuildingTemplates;	/// AI building templates
	std::map<int, std::vector<std::string>> PersonalNames;	/// Personal names for the civilization, mapped to the gender they pertain to (use NoGender for names which should be available for both genders)
private:
	std::map<const unit_class *, std::vector<std::string>> unit_class_names;	/// Unit class names for the civilization, mapped to the unit class they pertain to, used for mechanical units, and buildings
public:
	std::vector<std::string> FamilyNames;		/// Family names for the civilization
	std::vector<std::string> ProvinceNames;		/// Province names for the civilization
private:
	std::vector<std::string> ship_names;			/// Ship names for the civilization
	std::map<const unit_class *, CUnitType *> class_unit_types; //the unit type slot of a particular class for the civilization
public:
	std::vector<CDeity *> Deities;
	std::vector<site *> sites; //sites used for this civilization if a randomly-generated one is required
	std::string MinisterTitles[MaxCharacterTitles][MaxGenders][MaxGovernmentTypes][MaxFactionTiers]; /// this civilization's minister title for each minister type and government type
	std::map<std::string, std::map<CDate, bool>> HistoricalUpgrades;	/// historical upgrades of the faction, with the date of change

	friend int ::CclDefineCivilization(lua_State *l);
};

}