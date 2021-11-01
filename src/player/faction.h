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
//      (c) Copyright 2019-2021 by Andrettin
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

#include "database/data_type.h"
#include "database/detailed_data_entry.h"
#include "time/date.h"
#include "ui/icon.h"
#include "ui/ui.h"
#include "unit/unit_class_container.h"

class CAiBuildingTemplate;
class CCurrency;
class CUpgrade;
class LuaCallback;
enum class ButtonCmd;

static int CclDefineFaction(lua_State *l);

namespace wyrmgus {

class ai_force_template;
class and_condition;
class character;
class civilization;
class deity;
class dynasty;
class faction_history;
class icon;
class name_generator;
class resource;
class site;
class unit_class;
class unit_type;
class upgrade_class;
enum class ai_force_type;
enum class character_title;
enum class diplomacy_state;
enum class faction_tier;
enum class faction_type;
enum class gender;
enum class government_type;

class faction final : public detailed_data_entry, public data_type<faction>
{
	Q_OBJECT

	Q_PROPERTY(QString link_string READ get_link_qstring CONSTANT)
	Q_PROPERTY(wyrmgus::civilization* civilization MEMBER civilization NOTIFY changed)
	Q_PROPERTY(wyrmgus::faction_type type MEMBER type READ get_type)
	Q_PROPERTY(wyrmgus::faction* parent_faction MEMBER parent_faction)
	Q_PROPERTY(wyrmgus::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(wyrmgus::player_color* color MEMBER color READ get_color NOTIFY changed)
	Q_PROPERTY(wyrmgus::faction_tier default_tier MEMBER default_tier READ get_default_tier)
	Q_PROPERTY(wyrmgus::faction_tier min_tier MEMBER min_tier READ get_min_tier)
	Q_PROPERTY(wyrmgus::faction_tier max_tier MEMBER max_tier READ get_max_tier)
	Q_PROPERTY(wyrmgus::government_type default_government_type MEMBER default_government_type READ get_default_government_type)
	Q_PROPERTY(bool playable MEMBER playable READ is_playable)
	Q_PROPERTY(CUpgrade* upgrade MEMBER upgrade)
	Q_PROPERTY(wyrmgus::site* default_capital MEMBER default_capital READ get_default_capital)
	Q_PROPERTY(bool simple_name MEMBER simple_name READ uses_simple_name)
	Q_PROPERTY(bool short_name MEMBER short_name READ uses_short_name)
	Q_PROPERTY(bool definite_article MEMBER definite_article READ uses_definite_article)
	Q_PROPERTY(wyrmgus::deity* holy_order_deity MEMBER holy_order_deity READ get_holy_order_deity)

public:
	using title_name_map = std::map<government_type, std::map<faction_tier, std::string>>;
	using character_title_name_map = std::map<character_title, std::map<government_type, std::map<faction_tier, std::map<gender, std::string>>>>;

	static constexpr const char *class_identifier = "faction";
	static constexpr const char *database_folder = "factions";

	static faction *add(const std::string &identifier, const wyrmgus::data_module *data_module)
	{
		faction *faction = data_type::add(identifier, data_module);
		faction->ID = faction::get_all().size() - 1;
		return faction;
	}

	static bool compare_encyclopedia_entries(const faction *lhs, const faction *rhs);

	static void process_title_names(title_name_map &title_names, const sml_data &scope);
	static void process_title_name_scope(title_name_map &title_names, const sml_data &scope);
	static void process_character_title_name_scope(character_title_name_map &character_title_names, const sml_data &scope);
	static void process_character_title_name_scope(std::map<government_type, std::map<faction_tier, std::map<gender, std::string>>> &character_title_names, const sml_data &scope);
	static void process_character_title_name_scope(std::map<faction_tier, std::map<gender, std::string>> &character_title_names, const sml_data &scope);

	explicit faction(const std::string &identifier);
	~faction();

	virtual void process_sml_property(const sml_property &property) override;
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;
	virtual data_entry_history *get_history_base() override;

	const faction_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	virtual bool has_encyclopedia_entry() const override
	{
		if (this->get_icon() == nullptr) {
			return false;
		}

		return detailed_data_entry::has_encyclopedia_entry();
	}

	virtual std::string get_encyclopedia_text() const override;

	QString get_link_qstring() const
	{
		return QString::fromStdString(this->get_link_string());
	}

	wyrmgus::civilization *get_civilization() const
	{
		return this->civilization;
	}

	faction_type get_type() const
	{
		return this->type;
	}

	const faction *get_parent_faction() const
	{
		return this->parent_faction;
	}

	const wyrmgus::icon *get_icon() const
	{
		return this->icon;
	}
	
	CCurrency *GetCurrency() const;

	faction_tier get_default_tier() const
	{
		return this->default_tier;
	}

	faction_tier get_min_tier() const
	{
		return this->min_tier;
	}

	faction_tier get_max_tier() const
	{
		return this->max_tier;
	}

	government_type get_default_government_type() const
	{
		return this->default_government_type;
	}

	bool is_playable() const
	{
		return this->playable;
	}

	player_color *get_color() const
	{
		return this->color;
	}

	const CUpgrade *get_upgrade() const
	{
		return this->upgrade;
	}

	site *get_default_capital() const
	{
		return this->default_capital;
	}

	bool uses_simple_name() const;

	bool uses_short_name() const
	{
		return this->short_name;
	}

	bool uses_definite_article() const
	{
		return this->definite_article;
	}

	deity *get_holy_order_deity() const
	{
		return this->holy_order_deity;
	}

	std::string_view get_title_name(const government_type government_type, const faction_tier tier) const;
	std::string_view get_character_title_name(const character_title title_type, const government_type government_type, const faction_tier tier, const gender gender) const;

	std::string get_titled_name(const government_type government_type, const faction_tier tier) const
	{
		if (this->uses_simple_name()) {
			return this->get_name();
		}

		const std::string title_name = std::string(this->get_title_name(government_type, tier));
		if (this->uses_short_name()) {
			return this->Adjective + " " + title_name;
		} else {
			return title_name + " of " + this->get_name();
		}
	}

	std::string get_default_titled_name() const
	{
		return this->get_titled_name(this->get_default_government_type(), this->get_default_tier());
	}

	bool develops_from_faction(const faction *faction, const bool include_indirect) const;
	bool develops_to_faction(const faction *faction, const bool include_indirect) const;

	int GetUpgradePriority(const CUpgrade *upgrade) const;
	int get_force_type_weight(const ai_force_type force_type) const;
	const std::vector<std::unique_ptr<ai_force_template>> &get_ai_force_templates(const ai_force_type force_type) const;
	const std::vector<std::unique_ptr<CAiBuildingTemplate>> &GetAiBuildingTemplates() const;

	const unit_class_map<std::unique_ptr<name_generator>> &get_unit_class_name_generators() const
	{
		return this->unit_class_name_generators;
	}

	const name_generator *get_unit_class_name_generator(const unit_class *unit_class) const;

	const name_generator *get_ship_name_generator() const
	{
		return this->ship_name_generator.get();
	}

	unit_type *get_class_unit_type(const unit_class *unit_class) const;

	void set_class_unit_type(const unit_class *unit_class, unit_type *unit_type)
	{
		if (unit_type == nullptr) {
			this->class_unit_types.erase(unit_class);
			return;
		}

		this->class_unit_types[unit_class] = unit_type;
	}

	void remove_class_unit_type(unit_type *unit_type)
	{
		for (unit_class_map<wyrmgus::unit_type *>::reverse_iterator iterator = this->class_unit_types.rbegin(); iterator != this->class_unit_types.rend(); ++iterator) {
			if (iterator->second == unit_type) {
				this->class_unit_types.erase(iterator->first);
			}
		}
	}

	bool is_class_unit_type(const unit_type *unit_type) const;

	CUpgrade *get_class_upgrade(const upgrade_class *upgrade_class) const;

	void set_class_upgrade(const upgrade_class *upgrade_class, CUpgrade *upgrade)
	{
		if (upgrade == nullptr) {
			this->class_upgrades.erase(upgrade_class);
			return;
		}

		this->class_upgrades[upgrade_class] = upgrade;
	}

	void remove_class_upgrade(CUpgrade *upgrade)
	{
		for (std::map<const upgrade_class *, CUpgrade *>::reverse_iterator iterator = this->class_upgrades.rbegin(); iterator != this->class_upgrades.rend(); ++iterator) {
			if (iterator->second == upgrade) {
				this->class_upgrades.erase(iterator->first);
			}
		}
	}

	const std::vector<CFiller> &get_ui_fillers() const;

	const std::unique_ptr<const and_condition> &get_preconditions() const
	{
		return this->preconditions;
	}

	const std::unique_ptr<const and_condition> &get_conditions() const
	{
		return this->conditions;
	}

	const std::vector<const dynasty *> &get_dynasties() const
	{
		return this->dynasties;
	}

	void add_dynasty(const dynasty *dynasty)
	{
		this->dynasties.push_back(dynasty);
	}

	void remove_dynasty(const dynasty *dynasty);

	const std::vector<character *> &get_characters() const
	{
		return this->characters;
	}

	void add_character(character *character)
	{
		this->characters.push_back(character);
	}

	const std::vector<const site *> &get_core_settlements() const
	{
		return this->core_settlements;
	}

	void add_core_settlement(const site *settlement)
	{
		this->core_settlements.push_back(settlement);
	}

signals:
	void changed();

public:
	std::string Adjective;												/// adjective pertaining to the faction
	std::string DefaultAI = "land-attack";
	int ID = -1;														/// faction ID
private:
	wyrmgus::civilization *civilization = nullptr;
	faction_type type; //faction type (i.e. tribe or polity)
	faction *parent_faction = nullptr;
	faction_tier default_tier;
	faction_tier min_tier;
	faction_tier max_tier;
	government_type default_government_type;
	bool playable = true;
	wyrmgus::icon *icon = nullptr;
public:
	CCurrency *Currency = nullptr;										/// The faction's currency
private:
	deity *holy_order_deity = nullptr; //deity this faction belongs to, if it is a holy order
	player_color *color = nullptr; /// faction color
	CUpgrade *upgrade = nullptr; //upgrade applied when the faction is set to a player
	site *default_capital = nullptr;
	bool simple_name = false;
	bool short_name = false;
	bool definite_article = false; //whether the faction's name should be preceded by a definite article (e.g. "the Netherlands")
public:
	std::vector<faction *> DevelopsFrom;								/// from which factions can this faction develop
	std::vector<faction *> DevelopsTo;									/// to which factions this faction can develop
private:
	title_name_map title_names;
	character_title_name_map character_title_names;
public:
	std::map<const CUpgrade *, int> UpgradePriorities;					/// Priority for each upgrade
	std::map<ButtonCmd, IconConfig> ButtonIcons;						/// icons for button actions
private:
	unit_class_map<unit_type *> class_unit_types; //the unit type slot of a particular class for the faction
	std::map<const upgrade_class *, CUpgrade *> class_upgrades; //the upgrade slot of a particular class for the faction
public:
	std::vector<std::string> ProvinceNames;								/// Province names for the faction
private:
	unit_class_map<std::unique_ptr<name_generator>> unit_class_name_generators;
	std::unique_ptr<name_generator> ship_name_generator;
	std::vector<CFiller> ui_fillers;
	std::unique_ptr<const and_condition> preconditions;
	std::unique_ptr<const and_condition> conditions;
	std::vector<const site *> core_settlements; //the core settlements of this faction (required to found it)
public:
	std::vector<site *> sites; /// Sites used for this faction if it needs a randomly-generated settlement
private:
	std::vector<const dynasty *> dynasties; //which dynasties are available to this faction
	std::vector<character *> characters;
	std::map<ai_force_type, std::vector<std::unique_ptr<ai_force_template>>> ai_force_templates; //force templates, mapped to each force type
	std::map<ai_force_type, int> ai_force_type_weights;							/// Weights for each force type
public:
	std::vector<std::unique_ptr<CAiBuildingTemplate>> AiBuildingTemplates;	/// AI building templates
	std::map<std::tuple<CDate, CDate, character_title>, character *> HistoricalMinisters;	/// historical ministers of the faction (as well as heads of state and government), mapped to the beginning and end of the rule, and the enum of the title in question
	std::map<int, faction_tier> HistoricalTiers; /// dates in which this faction's tier changed; faction tier mapped to year
	std::map<int, government_type> HistoricalGovernmentTypes;						/// dates in which this faction's government type changed; government type mapped to year
private:
	std::unique_ptr<faction_history> history;
public:
	std::map<std::pair<CDate, faction *>, diplomacy_state> HistoricalDiplomacyStates;	/// dates in which this faction's diplomacy state to another faction changed; diplomacy state mapped to year and faction
	std::map<std::pair<CDate, int>, int> HistoricalResources;	/// dates in which this faction's storage of a particular resource changed; resource quantities mapped to date and resource
	std::vector<std::pair<CDate, std::string>> HistoricalCapitals;		/// historical capitals of the faction; the values are: date and settlement ident
	std::filesystem::path Mod;							/// To which mod (or map), if any, this faction belongs

	friend int ::CclDefineFaction(lua_State *l);
};

}
