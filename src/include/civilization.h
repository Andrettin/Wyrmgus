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
//      (c) Copyright 2018-2021 by Andrettin
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

#pragma once

#include "civilization_base.h"
#include "database/data_type.h"
#include "time/date.h"
#include "ui/ui.h"

class CAiBuildingTemplate;
class CCurrency;
class CUpgrade;
struct lua_State;

static int CclDefineCivilization(lua_State *l);

namespace wyrmgus {

class ai_force_template;
class calendar;
class character;
class deity;
class language;
class quest;
class resource;
class site;
enum class ai_force_type;
enum class character_title;
enum class faction_tier;
enum class faction_type;
enum class government_type;

class civilization final : public civilization_base, public data_type<civilization>
{
	Q_OBJECT

	Q_PROPERTY(QString link_string READ get_link_qstring CONSTANT)
	Q_PROPERTY(wyrmgus::civilization* parent_civilization MEMBER parent_civilization READ get_parent_civilization)
	Q_PROPERTY(bool visible MEMBER visible READ is_visible)
	Q_PROPERTY(bool playable MEMBER playable READ is_playable)
	Q_PROPERTY(QString interface READ get_interface_qstring)
	Q_PROPERTY(CUpgrade* upgrade MEMBER upgrade READ get_upgrade)
	Q_PROPERTY(wyrmgus::language* language MEMBER language)
	Q_PROPERTY(QString encyclopedia_background_file READ get_encyclopedia_background_file_qstring NOTIFY changed)

public:
	static constexpr const char *class_identifier = "civilization";
	static constexpr const char *database_folder = "civilizations";

	static civilization *add(const std::string &identifier, const wyrmgus::data_module *data_module)
	{
		civilization *civilization = data_type::add(identifier, data_module);
		civilization->ID = civilization::get_all().size() - 1;
		return civilization;
	}

	explicit civilization(const std::string &identifier);
	virtual ~civilization() override;

	virtual void process_sml_property(const sml_property &property) override;
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	virtual bool has_encyclopedia_entry() const override
	{
		if (!this->is_visible()) {
			return false;
		}

		return detailed_data_entry::has_encyclopedia_entry();
	}

	virtual std::string get_encyclopedia_text() const override;

	QString get_link_qstring() const
	{
		return QString::fromStdString(this->get_link_string());
	}

	civilization *get_parent_civilization() const
	{
		return this->parent_civilization;
	}
	
	const wyrmgus::species *get_species() const
	{
		if (civilization_base::get_species() != nullptr) {
			return civilization_base::get_species();
		}

		if (this->get_parent_civilization() != nullptr) {
			return this->get_parent_civilization()->get_species();
		}

		return nullptr;
	}
	
	int GetUpgradePriority(const CUpgrade *upgrade) const;
	int get_force_type_weight(const ai_force_type force_type) const;

	const std::string &get_interface() const
	{
		return this->interface;
	}

	QString get_interface_qstring() const
	{
		return QString::fromStdString(this->interface);
	}

	Q_INVOKABLE void set_interface(const std::string &interface)
	{
		this->interface = interface;
	}

	CUpgrade *get_upgrade() const
	{
		return this->upgrade;
	}

	const wyrmgus::language *get_language() const
	{
		return this->language;
	}

	wyrmgus::calendar *get_calendar() const;
	CCurrency *GetCurrency() const;

	bool is_visible() const
	{
		return this->visible;
	}

	bool is_playable() const
	{
		return this->playable;
	}

	const std::filesystem::path &get_encyclopedia_background_file() const;

	QString get_encyclopedia_background_file_qstring() const
	{
		return QString::fromStdString(this->get_encyclopedia_background_file().string());
	}

	void set_encyclopedia_background_file(const std::filesystem::path &filepath);

	Q_INVOKABLE void set_encyclopedia_background_file(const std::string &filepath)
	{
		this->set_encyclopedia_background_file(std::filesystem::path(filepath));
	}

	const std::vector<const civilization *> &get_develops_from() const
	{
		return this->develops_from;
	}

	const std::vector<const civilization *> &get_develops_to() const
	{
		return this->develops_to;
	}

	cursor *get_cursor(const cursor_type type) const;

	void set_cursor(const cursor_type type, cursor *cursor)
	{
		if (this->cursors.contains(type)) {
			throw std::runtime_error("Another cursor is already registered for type \"" + std::to_string(static_cast<int>(type)) + "\".");
		}

		this->cursors[type] = cursor;
	}

	std::string_view get_title_name(const government_type government_type, const faction_tier tier) const;
	std::string_view get_character_title_name(const character_title title_type, const faction_type faction_type, const government_type government_type, const faction_tier tier, const gender gender) const;
	void process_character_title_name_scope(const sml_data &scope);
	void process_character_title_name_scope(const character_title title_type, const sml_data &scope);

	const std::vector<std::unique_ptr<ai_force_template>> &get_ai_force_templates(const ai_force_type force_type) const;
	const std::vector<std::unique_ptr<CAiBuildingTemplate>> &GetAiBuildingTemplates() const;

	unit_type *get_class_unit_type(const unit_class *unit_class) const;
	CUpgrade *get_class_upgrade(const upgrade_class *upgrade_class) const;

	const std::vector<CFiller> &get_ui_fillers() const
	{
		if (!this->ui_fillers.empty()) {
			return this->ui_fillers;
		}

		if (this->get_parent_civilization() != nullptr) {
			return this->get_parent_civilization()->get_ui_fillers();
		}

		return this->ui_fillers;
	}

	const std::vector<character *> &get_characters() const
	{
		return this->characters;
	}

	void add_character(character *character)
	{
		this->characters.push_back(character);
	}

	Q_INVOKABLE QVariantList get_tech_tree_entries() const;

signals:
	void changed();

public:
	int ID = -1;
private:
	civilization *parent_civilization = nullptr;
public:
	std::string Adjective;			/// adjective pertaining to the civilization
private:
	std::string interface; //the string identifier for the civilization's interface
	CUpgrade *upgrade = nullptr;
	wyrmgus::language *language = nullptr;	/// the language used by the civilization
	wyrmgus::calendar *calendar = nullptr;	/// the calendar used by the civilization
public:
	CCurrency *Currency = nullptr;	/// the currency used by the civilization
private:
	bool visible = true; //whether the civilization is visible e.g. in the map editor
	bool playable = true; //civilizations are playable by default
	std::filesystem::path encyclopedia_background_file;
	std::vector<const civilization *> develops_from; //from which civilizations this civilization develops
	std::vector<const civilization *> develops_to; //to which civilizations this civilization develops
	std::map<cursor_type, cursor *> cursors;
public:
	std::vector<quest *> Quests;	/// quests belonging to this civilization
	std::map<const CUpgrade *, int> UpgradePriorities;		/// Priority for each upgrade
private:
	std::map<ai_force_type, std::vector<std::unique_ptr<ai_force_template>>> ai_force_templates;	/// Force templates, mapped to each force type
	std::map<ai_force_type, int> ai_force_type_weights;	/// Weights for each force type
public:
	std::vector<std::unique_ptr<CAiBuildingTemplate>> AiBuildingTemplates;	/// AI building templates
public:
	std::vector<std::string> ProvinceNames;		/// Province names for the civilization
private:
	std::vector<CFiller> ui_fillers;
	std::vector<character *> characters;
public:
	std::vector<deity *> Deities;
	std::vector<site *> sites; //sites used for this civilization if a randomly-generated one is required
private:
	std::map<government_type, std::map<faction_tier, std::string>> title_names;
	std::map<character_title, std::map<faction_type, std::map<government_type, std::map<faction_tier, std::map<gender, std::string>>>>> character_title_names;

	friend int ::CclDefineCivilization(lua_State *l);
};

}