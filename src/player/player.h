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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "economy/resource_container.h"
#include "map/landmass_container.h"
#include "map/site_container.h"
#include "player/player_container.h"
#include "ui/icon.h"
#include "unit/unit_class_container.h"
#include "unit/unit_type_container.h"
#include "upgrade/upgrade_structs.h"
#include "util/qunique_ptr.h"
#include "vec2i.h"

constexpr int DefaultTradeCost = 30;

class CCurrency;
class CDate;
class CFile;
class CGraphic;
class CProvince;
class CUnit;
class PlayerAi;
enum class ButtonCmd;
enum class DiplomacyState;
struct lua_State;

static int CclUnit(lua_State *l);

namespace wyrmgus {
	class age;
	class calendar;
	class character;
	class civilization;
	class civilization_base;
	class deity;
	class dynasty;
	class faction;
	class interface_style;
	class language;
	class player_color;
	class player_quest_objective;
	class quest;
	class religion;
	class resource;
	class site;
	class unit_class;
	class unit_type;
	class upgrade_class;
	enum class ai_force_type;
	enum class character_title;
	enum class faction_tier;
	enum class government_type;
	enum class gender;
	enum class player_type;
	enum class resource_storage_type;
	enum class unit_domain;
	enum class vassalage_type;
}

constexpr int PlayerNumNeutral = PlayerMax - 1;  /// this is the neutral player slot

class CPlayer final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring NOTIFY name_changed)
	Q_PROPERTY(bool active READ is_active_sync NOTIFY type_changed)
	Q_PROPERTY(bool alive READ is_alive_sync NOTIFY alive_changed)
	Q_PROPERTY(QVariantList current_special_resources READ get_current_special_resources_sync NOTIFY current_special_resources_changed)

public:
	static constexpr int max_heroes = 4; //maximum heroes per player
	static constexpr int max_quest_pool = 4;
	static constexpr size_t max_current_quests = 4;
	static constexpr int base_speed_factor = 100;

	static void SetThisPlayer(CPlayer *player);
	static CPlayer *GetThisPlayer();

	static std::vector<qunique_ptr<CPlayer>> Players;	//all players

	static CPlayer *get_neutral_player()
	{
		return CPlayer::Players[PlayerNumNeutral].get();
	}

	static const std::vector<CPlayer *> &get_non_neutral_players()
	{
		//get the players other than the neutral player
		return CPlayer::non_neutral_players;
	}

	static const std::vector<int> &get_revealed_player_indexes()
	{
		return CPlayer::revealed_player_indexes;
	}

private:
	static CPlayer *ThisPlayer; //player on local computer
	static inline std::vector<CPlayer *> non_neutral_players;
	static inline std::vector<int> revealed_player_indexes;

public:
	explicit CPlayer(const int index);
	~CPlayer();

	int get_index() const
	{
		return this->index;
	}

	player_type get_type() const
	{
		return this->type;
	}

	void set_type(const player_type type)
	{
		if (type == this->get_type()) {
			return;
		}

		std::unique_lock<std::shared_mutex> lock(this->mutex);

		this->type = type;

		emit type_changed();
	}

	bool is_active() const;

	bool is_active_sync() const
	{
		std::shared_lock<std::shared_mutex> lock(this->mutex);

		return this->is_active();
	}

	bool is_neutral_player() const;

	const std::string &get_name() const
	{
		return this->name;
	}

	QString get_name_qstring() const
	{
		//we only need to lock here and not in the get_name() function because the only possible contention is between a write from the Wyrmgus thread and a read from the Qt one; besides, get_name() returns a reference, which would make a lock there pointless
		std::shared_lock<std::shared_mutex> lock(this->mutex);

		return QString::fromStdString(this->get_name());
	}

	void set_name(const std::string &name);
	void update_name_from_faction();
	bool uses_definite_article() const;

	//Wyrmgus start
	const wyrmgus::civilization *get_civilization() const;
	void set_civilization(const wyrmgus::civilization *civilization);

	const wyrmgus::faction *get_faction() const
	{
		return this->faction;
	}

	void set_faction(const wyrmgus::faction *faction);
	Q_INVOKABLE void set_faction_async(wyrmgus::faction *faction);
	void set_random_faction();

	wyrmgus::faction_tier get_faction_tier() const
	{
		return this->faction_tier;
	}

	void set_faction_tier(const wyrmgus::faction_tier tier);

	wyrmgus::government_type get_government_type() const
	{
		return this->government_type;
	}

	void set_government_type(const wyrmgus::government_type government_type);

	const wyrmgus::religion *get_religion() const
	{
		return this->religion;
	}

	const wyrmgus::dynasty *get_dynasty() const
	{
		return this->dynasty;
	}

	void set_dynasty(const wyrmgus::dynasty *dynasty);

	interface_style *get_interface_style() const;

	const wyrmgus::age *get_age() const
	{
		return this->age;
	}

	void check_age();
	void set_age(const wyrmgus::age *age);
	CCurrency *GetCurrency() const;

	bool is_alive() const
	{
		return this->alive;
	}

	bool is_alive_sync() const
	{
		std::shared_lock<std::shared_mutex> lock(this->mutex);

		return this->is_alive();
	}

	void set_alive(const bool alive)
	{
		if (alive == this->is_alive()) {
			return;
		}

		std::unique_lock<std::shared_mutex> lock(this->mutex);

		this->alive = alive;

		emit alive_changed();
	}

	/// Clear turn related player data
	void Clear();

	std::vector<CUnit *>::const_iterator UnitBegin() const;
	std::vector<CUnit *>::const_iterator UnitEnd() const;

	const wyrmgus::player_color *get_player_color() const
	{
		return this->player_color;
	}

	const QColor &get_minimap_color() const;

	const std::vector<std::unique_ptr<wyrmgus::player_quest_objective>> &get_quest_objectives() const
	{
		return this->quest_objectives;
	}

	void share_upgrade_progress(CPlayer &other_player, CUnit &unit);
	int get_player_color_usage_count(const wyrmgus::player_color *player_color) const;
	void update_territory_tiles();

	unit_type *get_class_unit_type(const wyrmgus::unit_class *unit_class) const;
	bool is_class_unit_type(const unit_type *unit_type) const;
	CUpgrade *get_class_upgrade(const wyrmgus::upgrade_class *upgrade_class) const;

	bool has_upgrade(const CUpgrade *upgrade) const;
	bool has_upgrade_class(const wyrmgus::upgrade_class *upgrade_class) const;

	void acquire_upgrade(const CUpgrade *upgrade);
	void lose_upgrade(const CUpgrade *upgrade);

	const unit_class *get_default_population_class(const unit_domain domain) const;

	std::vector<CUnit *> get_town_hall_units() const;
	std::vector<const wyrmgus::site *> get_settlements() const;
	bool has_settlement(const wyrmgus::site *settlement) const;
	bool has_coastal_settlement() const;
	bool HasSettlementNearWaterZone(const landmass *water_zone) const;
	bool has_settlement_with_resource_source(const wyrmgus::resource *resource) const;
	const wyrmgus::site *GetNearestSettlement(const Vec2i &pos, int z, const Vec2i &size) const;
	void update_building_settlement_assignment(const wyrmgus::site *old_settlement, const int z) const;
	site_set get_border_settlements() const;
	player_set get_border_players() const;

	bool HasUnitBuilder(const wyrmgus::unit_type *type, const wyrmgus::site *settlement = nullptr) const;
	bool HasUpgradeResearcher(const CUpgrade *upgrade) const;

	template <bool preconditions_only = false>
	bool can_found_faction(const wyrmgus::faction *faction) const;

	std::vector<wyrmgus::faction *> get_potential_factions() const;

	template <bool preconditions_only = false>
	bool can_choose_dynasty(const wyrmgus::dynasty *dynasty) const;

	bool is_character_available_for_recruitment(const wyrmgus::character *character, bool ignore_neutral = false) const;
	std::vector<wyrmgus::character *> get_recruitable_heroes_from_list(const std::vector<wyrmgus::character *> &heroes);
	bool is_upgrade_quest_objective(const CUpgrade *upgrade) const;
	bool UpgradeRemovesExistingUpgrade(const CUpgrade *upgrade, const bool ignore_lower_ai_priority) const;
	std::string get_full_name() const;
	std::string_view get_faction_title_name() const;
	std::string_view GetCharacterTitleName(const character_title title_type, const wyrmgus::gender gender) const;
	std::vector<CUnit *> get_builders(const unit_type *building) const;
	landmass_set get_builder_landmasses(const unit_type *building) const;
	std::vector<const CUpgrade *> GetResearchableUpgrades();

	const std::vector<CUnit *> &get_units() const
	{
		return this->Units;
	}

	CUnit &GetUnit(int index) const;
	int GetUnitCount() const;

	void AddUnit(CUnit &unit);
	void RemoveUnit(CUnit &unit);

	CUnit *get_last_created_unit() const
	{
		return this->last_created_unit;
	}

	void UpdateFreeWorkers();
	//Wyrmgus start
	void PerformResourceTrade();
	bool HasMarketUnit() const;
	CUnit *GetMarketUnit() const;
	std::vector<int> GetAutosellResources() const;
	void AutosellResource(const int resource);
	void UpdateLevelUpUnits();

	const std::vector<wyrmgus::quest *> &get_available_quests() const
	{
		return this->available_quests;
	}

	const std::vector<wyrmgus::quest *> &get_current_quests() const
	{
		return this->current_quests;
	}

	void update_quest_pool();
	void on_available_quests_changed();
	void update_current_quests();
	void accept_quest(wyrmgus::quest *quest);
	void complete_quest(wyrmgus::quest *quest);
	void fail_quest(wyrmgus::quest *quest, const std::string &failure_reason_string = "");
	void remove_current_quest(wyrmgus::quest *quest);
	bool can_quest_be_available(const wyrmgus::quest *quest) const;
	bool can_accept_quest(const wyrmgus::quest *quest) const;
	bool check_quest_completion(const wyrmgus::quest *quest) const;
	std::pair<bool, std::string> check_quest_failure(const wyrmgus::quest *quest) const;
	bool has_quest(const wyrmgus::quest *quest) const;
	bool is_quest_completed(const wyrmgus::quest *quest) const;

	void on_unit_built(const CUnit *unit);
	void on_unit_destroyed(const CUnit *unit);
	void on_resource_gathered(const wyrmgus::resource *resource, const int quantity);

	void AddModifier(CUpgrade *modifier, int cycles);
	void RemoveModifier(CUpgrade *modifier);

	bool at_war() const;

	bool can_declare_war_on(const CPlayer *other_player) const
	{
		if (this->is_independent()) {
			return true;
		}

		return this->is_any_vassal_of(other_player);
	}

	//Wyrmgus start
//	void SetStartView(const Vec2i &pos) { StartPos = pos; }
	void SetStartView(const Vec2i &pos, int z) { StartPos = pos; StartMapLayer = z; }
	//Wyrmgus end

	int get_resource(const resource *resource) const
	{
		const auto find_iterator = this->resources.find(resource);

		if (find_iterator != this->resources.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_resource(const resource *resource, const int quantity);

	void change_resource(const resource *resource, const int quantity)
	{
		this->set_resource(resource, this->get_resource(resource) + quantity);
	}

	int get_max_resource(const resource *resource) const
	{
		const auto find_iterator = this->max_resources.find(resource);

		if (find_iterator != this->max_resources.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_max_resource(const resource *resource, const int quantity)
	{
		if (quantity == 0) {
			if (this->max_resources.contains(resource)) {
				this->max_resources.erase(resource);
			}
		} else {
			this->max_resources[resource] = quantity;
		}
	}

	void change_max_resource(const resource *resource, const int quantity)
	{
		this->set_max_resource(resource, this->get_max_resource(resource) + quantity);
	}

	int get_last_resource(const resource *resource) const
	{
		const auto find_iterator = this->last_resources.find(resource);

		if (find_iterator != this->last_resources.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_last_resource(const resource *resource, const int quantity)
	{
		if (quantity == 0) {
			if (this->last_resources.contains(resource)) {
				this->last_resources.erase(resource);
			}
		} else {
			this->last_resources[resource] = quantity;
		}
	}

	int get_stored_resource(const resource *resource) const
	{
		const auto find_iterator = this->stored_resources.find(resource);

		if (find_iterator != this->stored_resources.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_stored_resource(const resource *resource, const int quantity);

	void change_stored_resource(const resource *resource, const int quantity)
	{
		this->set_stored_resource(resource, this->get_stored_resource(resource) + quantity);
	}

	int get_resource_demand(const resource *resource) const
	{
		const auto find_iterator = this->resource_demands.find(resource);

		if (find_iterator != this->resource_demands.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_resource_demand(const resource *resource, const int quantity)
	{
		if (quantity == 0) {
			if (this->resource_demands.contains(resource)) {
				this->resource_demands.erase(resource);
			}
		} else {
			this->resource_demands[resource] = quantity;
		}
	}

	void change_resource_demand(const resource *resource, const int quantity)
	{
		this->set_resource_demand(resource, this->get_resource_demand(resource) + quantity);
	}

	int get_stored_resource_demand(const resource *resource) const
	{
		const auto find_iterator = this->stored_resource_demands.find(resource);

		if (find_iterator != this->stored_resource_demands.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_stored_resource_demand(const resource *resource, const int quantity)
	{
		if (quantity == 0) {
			if (this->stored_resource_demands.contains(resource)) {
				this->stored_resource_demands.erase(resource);
			}
		} else {
			this->stored_resource_demands[resource] = quantity;
		}
	}

	void change_stored_resource_demand(const resource *resource, const int quantity)
	{
		this->set_stored_resource_demand(resource, this->get_stored_resource_demand(resource) + quantity);
	}

	int get_income(const resource *resource) const
	{
		const auto find_iterator = this->incomes.find(resource);

		if (find_iterator != this->incomes.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_income(const resource *resource, const int quantity);

	int get_processing_bonus(const resource *resource) const;
	Q_INVOKABLE int get_processing_bonus_sync(wyrmgus::resource *resource) const;
	std::string get_children_processing_bonus_string(const resource *resource) const;

	Q_INVOKABLE QString get_children_processing_bonus_string_sync(resource *resource) const
	{
		std::shared_lock<std::shared_mutex> lock(this->mutex);

		return QString::fromStdString(this->get_children_processing_bonus_string(resource));
	}

	int get_revenue(const resource *resource) const
	{
		const auto find_iterator = this->revenues.find(resource);

		if (find_iterator != this->revenues.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_revenue(const resource *resource, const int quantity)
	{
		if (quantity == 0) {
			if (this->revenues.contains(resource)) {
				this->revenues.erase(resource);
			}
		} else {
			this->revenues[resource] = quantity;
		}
	}

	int get_price(const resource *resource) const
	{
		const auto find_iterator = this->prices.find(resource);

		if (find_iterator != this->prices.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_price(const resource *resource, const int quantity)
	{
		if (quantity == 0) {
			if (this->prices.contains(resource)) {
				this->prices.erase(resource);
			}
		} else {
			this->prices[resource] = quantity;
		}
	}

	void change_price(const resource *resource, const int quantity)
	{
		this->set_price(resource, this->get_price(resource) + quantity);
	}

	int get_resource_harvest_speed(const resource *resource) const
	{
		const auto find_iterator = this->resource_harvest_speeds.find(resource);

		if (find_iterator != this->resource_harvest_speeds.end()) {
			return find_iterator->second;
		}

		return CPlayer::base_speed_factor;
	}

	void set_resource_harvest_speed(const resource *resource, const int quantity)
	{
		if (quantity == CPlayer::base_speed_factor) {
			if (this->resource_harvest_speeds.contains(resource)) {
				this->resource_harvest_speeds.erase(resource);
			}
		} else {
			this->resource_harvest_speeds[resource] = quantity;
		}
	}

	int get_resource_return_speed(const resource *resource) const
	{
		const auto find_iterator = this->resource_return_speeds.find(resource);

		if (find_iterator != this->resource_return_speeds.end()) {
			return find_iterator->second;
		}

		return CPlayer::base_speed_factor;
	}

	void set_resource_return_speed(const resource *resource, const int quantity)
	{
		if (quantity == CPlayer::base_speed_factor) {
			if (this->resource_return_speeds.contains(resource)) {
				this->resource_return_speeds.erase(resource);
			}
		} else {
			this->resource_return_speeds[resource] = quantity;
		}
	}

	int get_infantry_cost_modifier() const
	{
		return this->infantry_cost_modifier;
	}

	void change_infantry_cost_modifier(const int change)
	{
		this->infantry_cost_modifier += change;
	}

	int get_cavalry_cost_modifier() const
	{
		return this->cavalry_cost_modifier;
	}

	void change_cavalry_cost_modifier(const int change)
	{
		this->cavalry_cost_modifier += change;
	}

	const wyrmgus::unit_type_map<std::vector<CUnit *>> &get_units_by_type() const
	{
		return this->units_by_type;
	}

	const std::vector<CUnit *> &get_type_units(const wyrmgus::unit_type *unit_type) const
	{
		static std::vector<CUnit *> empty_vector;

		const auto find_iterator = this->get_units_by_type().find(unit_type);
		if (find_iterator != this->get_units_by_type().end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	const wyrmgus::unit_class_map<std::vector<CUnit *>> &get_units_by_class() const
	{
		return this->units_by_class;
	}

	const std::vector<CUnit *> &get_class_units(const wyrmgus::unit_class *unit_class) const
	{
		static std::vector<CUnit *> empty_vector;

		const auto find_iterator = this->get_units_by_class().find(unit_class);
		if (find_iterator != this->get_units_by_class().end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	bool is_revealed() const
	{
		return this->revealed;
	}

	void set_revealed(const bool revealed);

	PlayerAi *get_ai() const
	{
		return this->Ai.get();
	}

	int get_score() const
	{
		return this->score;
	}

	void change_score(const int change)
	{
		this->score += change;
	}

	int get_military_score() const
	{
		return this->military_score;
	}

	int get_military_score_with_overlords_against(const CPlayer *other_player) const
	{
		int score = this->get_military_score();

		if (this->get_overlord() != nullptr && this->get_overlord() != other_player) {
			score += this->get_overlord()->get_military_score_with_overlords_against(other_player);
		}

		return score;
	}

	void change_military_score(const int change)
	{
		this->military_score += change;
	}

	void calculate_military_score();
	int get_military_score_percent_advantage_over(const CPlayer *other_player) const;
	bool has_military_advantage_over(const CPlayer *other_player) const;

	int get_resource_total(const resource *resource) const
	{
		const auto find_iterator = this->resource_totals.find(resource);

		if (find_iterator != this->resource_totals.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_resource_total(const resource *resource, const int quantity)
	{
		if (quantity == 0) {
			if (this->resource_totals.contains(resource)) {
				this->resource_totals.erase(resource);
			}
		} else {
			this->resource_totals[resource] = quantity;
		}
	}

	void change_resource_total(const resource *resource, const int quantity)
	{
		this->set_resource_total(resource, this->get_resource_total(resource) + quantity);
	}

	/// Get a resource of the player
	int get_resource(const wyrmgus::resource *resource, const resource_storage_type type) const;
	Q_INVOKABLE int get_resource_sync(wyrmgus::resource *resource) const;

	/// Adds/subtracts some resources to/from the player store
	void change_resource(const wyrmgus::resource *resource, const int value, const bool store);
	/// Set a resource of the player
	void set_resource(const wyrmgus::resource *resource, const int value, const resource_storage_type type);
	/// Check, if there enough resources for action.
	bool check_resource(const resource *resource, const int value);
	//Wyrmgus start
	/// Increase resource price
	void increase_resource_price(const resource *resource);
	/// Decrease resource price
	void decrease_resource_price(const resource *resource);
	/// Converges prices with another player
	int ConvergePricesWith(CPlayer &player, int max_convergences);
	/// Get the resource price
	int get_resource_price(const resource *resource) const;
	/// Get the effective resource demand for the player, given the current prices
	int get_effective_resource_demand(const resource *resource) const;

	int get_effective_resource_sell_price(const resource *resource, const int traded_quantity = 100) const;
	int get_effective_resource_buy_price(const resource *resource, const int traded_quantity = 100) const;

	/// Get the total price difference between this player and another one
	int GetTotalPriceDifferenceWith(const CPlayer &player) const;
	/// Get the trade potential between this player and another one
	int GetTradePotentialWith(const CPlayer &player) const;
	//Wyrmgus end

	void pay_overlord_tax(const wyrmgus::resource *resource, const int taxable_quantity);

	/// Returns count of specified unittype
	int GetUnitTotalCount(const wyrmgus::unit_type &type) const;
	/// Check if the unit-type didn't break any unit limits and supply/demand
	int CheckLimits(const wyrmgus::unit_type &type) const;

	/// Check if enough resources are available for costs
	int CheckCosts(const resource_map<int> &costs, const bool notify = true) const;
	/// Check if enough resources are available for a new unit-type
	int CheckUnitType(const wyrmgus::unit_type &type, bool hire = false) const;

	/// Add costs to the resources
	void AddCosts(const int *costs);
	/// Add costs for an unit-type to the resources
	void AddUnitType(const wyrmgus::unit_type &type, bool hire = false);
	/// Add a factor of costs to the resources
	void AddCostsFactor(const resource_map<int> &costs, const int factor);
	/// Remove costs from the resources
	void subtract_costs(const resource_map<int> &costs);
	/// Remove costs for an unit-type from the resources
	void SubUnitType(const wyrmgus::unit_type &type, bool hire = false);
	/// Remove a factor of costs from the resources
	void SubCostsFactor(const resource_map<int> &costs, const int factor);

	//Wyrmgus start
	resource_map<int> GetUnitTypeCosts(const unit_type *type, const bool hire = false, const bool ignore_one = false) const;
	int GetUnitTypeCostsMask(const wyrmgus::unit_type *type, bool hire = false) const;
	resource_map<int> GetUpgradeCosts(const CUpgrade *upgrade) const;
	int GetUpgradeCostsMask(const CUpgrade *upgrade) const;

	void SetUnitTypeCount(const wyrmgus::unit_type *type, int quantity);
	void ChangeUnitTypeCount(const wyrmgus::unit_type *type, int quantity);
	int GetUnitTypeCount(const wyrmgus::unit_type *type) const;

	void SetUnitTypeUnderConstructionCount(const wyrmgus::unit_type *type, int quantity);
	void ChangeUnitTypeUnderConstructionCount(const wyrmgus::unit_type *type, int quantity);
	int GetUnitTypeUnderConstructionCount(const wyrmgus::unit_type *type) const;

	void SetUnitTypeAiActiveCount(const wyrmgus::unit_type *type, int quantity);
	void ChangeUnitTypeAiActiveCount(const wyrmgus::unit_type *type, int quantity);
	int GetUnitTypeAiActiveCount(const wyrmgus::unit_type *type) const;

	int get_unit_class_count(const wyrmgus::unit_class *unit_class) const
	{
		const auto find_iterator = this->units_by_class.find(unit_class);
		if (find_iterator != this->units_by_class.end()) {
			return static_cast<int>(find_iterator->second.size());
		}

		return 0;
	}

	void IncreaseCountsForUnit(CUnit *unit, const bool type_change = false);
	void DecreaseCountsForUnit(CUnit *unit, const bool type_change = false);
	//Wyrmgus end

	/// Does the player have units of a given type
	bool has_unit_type(const wyrmgus::unit_type *unit_type) const;

	int get_population() const;

	/// Notify player about a problem
	//Wyrmgus start
//	void Notify(int type, const Vec2i &pos, const char *fmt, ...) const PRINTF_VAARG_ATTRIBUTE(4, 5); // Don't forget to count this
	void Notify(int type, const Vec2i &pos, int z, const char *fmt, ...) const PRINTF_VAARG_ATTRIBUTE(5, 6); // Don't forget to count this
	//Wyrmgus end
	/// Notify player about a problem
	void Notify(const char *fmt, ...) const PRINTF_VAARG_ATTRIBUTE(2, 3); // Don't forget to count this

	const player_index_set &get_enemies() const
	{
		return this->enemies;
	}

	bool has_enemy_stance_with(const int index) const
	{
		return this->enemies.contains(index);
	}

	bool has_enemy_stance_with(const CPlayer *other_player) const
	{
		return this->has_enemy_stance_with(other_player->get_index());
	}

	Q_INVOKABLE bool has_enemy_stance_with_sync(CPlayer *other_player) const
	{
		std::shared_lock<std::shared_mutex> lock(this->mutex);

		return this->has_enemy_stance_with(other_player->get_index());
	}

	bool is_enemy_of(const CPlayer &player) const;
	bool is_enemy_of(const CUnit &unit) const;

	bool has_allied_stance_with(const int index) const
	{
		return this->allies.contains(index);
	}

	Q_INVOKABLE bool has_allied_stance_with_sync(CPlayer *other_player) const
	{
		std::shared_lock<std::shared_mutex> lock(this->mutex);

		return this->has_allied_stance_with(other_player->get_index());
	}

	bool is_allied_with(const CPlayer &player) const;
	bool is_allied_with(const CUnit &unit) const;

	const player_index_set &get_shared_vision() const
	{
		return this->shared_vision;
	}

	bool has_shared_vision_with(const int player_index) const
	{
		return this->shared_vision.contains(player_index);
	}

	bool has_shared_vision_with(const CPlayer *player) const;
	bool has_shared_vision_with(const CUnit &unit) const;

	Q_INVOKABLE bool has_shared_vision_with_sync(CPlayer *other_player) const
	{
		std::shared_lock<std::shared_mutex> lock(this->mutex);

		return this->has_shared_vision_with(other_player);
	}

	const player_index_set &get_mutual_shared_vision() const
	{
		return this->mutual_shared_vision;
	}

	bool has_mutual_shared_vision_with(const int player_index) const
	{
		return this->mutual_shared_vision.contains(player_index);
	}

	bool has_mutual_shared_vision_with(const CPlayer *player) const;
	bool has_mutual_shared_vision_with(const CUnit &unit) const;
	bool is_vision_sharing() const;

	//whether what is visible for this player is visible for the other player (but not necessarily the other way around)
	bool shares_visibility_with(const CPlayer *other_player) const
	{
		return this->has_mutual_shared_vision_with(other_player) || this->is_revealed();
	}

	bool IsTeamed(const CPlayer &player) const;
	bool IsTeamed(const CUnit &unit) const;

	bool is_independent() const
	{
		return this->get_overlord() == nullptr;
	}

	bool is_overlord_of(const CPlayer *player) const
	{
		return player->get_overlord() == this;
	}

	bool is_any_overlord_of(const CPlayer *player) const
	{
		if (this->is_overlord_of(player)) {
			return true;
		}

		for (const CPlayer *vassal : this->get_vassals()) {
			if (vassal->is_any_overlord_of(player)) {
				return true;
			}
		}

		return false;
	}

	bool is_vassal_of(const CPlayer *player) const
	{
		return player == this->get_overlord();
	}

	bool is_any_vassal_of(const CPlayer *player) const
	{
		if (this->is_vassal_of(player)) {
			return true;
		}

		if (this->get_overlord() != nullptr) {
			return this->get_overlord()->is_any_vassal_of(player);
		}

		return false;
	}

	bool is_tile_explored(const QPoint &tile_pos, const int z) const;
	bool is_player_capital_explored(const CPlayer *other_player) const;
	//Wyrmgus start
	bool HasContactWith(const CPlayer &player) const;
	bool has_neutral_faction_type() const;
	bool has_building_access(const CPlayer *player, const ButtonCmd button_action) const;
	bool has_building_access(const CPlayer *player) const;
	bool has_building_access(const CUnit *unit, const ButtonCmd) const;
	bool has_building_access(const CUnit *unit) const;
	bool HasHero(const wyrmgus::character *hero) const;
	//Wyrmgus end

	void set_neutral_diplomatic_stance_with(const CPlayer *player);
	Q_INVOKABLE void set_neutral_diplomatic_stance_with_async(CPlayer *player);
	void set_allied_diplomatic_stance_with(const CPlayer *player);
	Q_INVOKABLE void set_allied_diplomatic_stance_with_async(CPlayer *player);
	void set_enemy_diplomatic_stance_with(CPlayer *player);
	Q_INVOKABLE void set_enemy_diplomatic_stance_with_async(CPlayer *player);
	void SetDiplomacyCrazyWith(const CPlayer &player);

	void set_shared_vision_with(CPlayer *player, const bool shared_vision);
	Q_INVOKABLE void set_shared_vision_with_async(CPlayer *player, const bool shared_vision);

	CPlayer *get_realm_player()
	{
		if (this->get_overlord() != nullptr) {
			return this->get_overlord()->get_realm_player();
		}

		return this;
	}

	CPlayer *get_overlord() const
	{
		return this->overlord;
	}

	void set_overlord(CPlayer *overlord, const wyrmgus::vassalage_type);

	CPlayer *get_top_overlord() const
	{
		if (this->get_overlord() != nullptr && this->get_overlord()->get_overlord() != nullptr) {
			return this->get_overlord()->get_top_overlord();
		}

		return this->get_overlord();
	}

	CPlayer *get_tier_overlord(const int tier) const
	{
		if (tier == 0) {
			return nullptr;
		} else if (tier == 1) {
			return this->get_overlord();
		} else {
			return this->get_overlord()->get_tier_overlord(tier - 1);
		}
	}

	int get_overlord_depth() const
	{
		if (this->get_overlord() != nullptr) {
			return this->get_overlord()->get_overlord_depth() + 1;
		}

		return 0;
	}

	void establish_overlordship_alliance(CPlayer *overlord);
	void break_overlordship_alliance(CPlayer *overlord);

	const std::vector<CPlayer *> &get_vassals() const
	{
		return this->vassals;
	}

	const player_set &get_recent_trade_partners() const
	{
		return this->recent_trade_partners;
	}

	void add_recent_trade_partner(CPlayer *other_player)
	{
		this->recent_trade_partners.insert(other_player);
	}

	void clear_recent_trade_partners()
	{
		this->recent_trade_partners.clear();
	}

	QVariantList get_current_special_resources_sync() const;

	void add_current_special_resource(const resource *resource)
	{
		if (this->current_special_resources.contains(resource)) {
			return;
		}

		std::optional<std::unique_lock<std::shared_mutex>> lock;

		if (this == CPlayer::GetThisPlayer()) {
			lock = std::unique_lock<std::shared_mutex>(this->mutex);
		}

		this->current_special_resources.insert(resource);

		emit current_special_resources_changed();
	}

	void remove_current_special_resource(const resource *resource)
	{
		if (!this->current_special_resources.contains(resource)) {
			return;
		}

		std::optional<std::unique_lock<std::shared_mutex>> lock;

		if (this == CPlayer::GetThisPlayer()) {
			lock = std::unique_lock<std::shared_mutex>(this->mutex);
		}

		this->current_special_resources.erase(resource);

		emit current_special_resources_changed();
	}

	void check_special_resource(const resource *resource);

	void Init(player_type type);
	void Save(CFile &file) const;
	void Load(lua_State *l);

	void apply_history(const CDate &start_date);
	void apply_civilization_history(const wyrmgus::civilization_base *civilization);

	void add_settlement_to_explored_territory(const site *settlement);

signals:
	void name_changed();
	void type_changed();
	void alive_changed();
	void resource_stored_changed(const int resource_index, const int amount);
	void resource_processing_bonus_changed(const int resource_index, const int bonus);
	void resource_children_processing_bonus_string_changed(const int resource_index, const QString &str);
	void current_special_resources_changed();
	void diplomatic_stances_changed();
	void shared_vision_changed();

private:
	const int index = 0;          /// player as number
	std::string name;
	player_type type; //type of the player (human, computer, ...)
public:
	int Race = 0; //race of the player (orc, human, ...)
private:
	const wyrmgus::faction *faction = nullptr;
	wyrmgus::faction_tier faction_tier;
	wyrmgus::government_type government_type;
	wyrmgus::religion *religion = nullptr; //religion of the player
	const wyrmgus::dynasty *dynasty = nullptr; //ruling dynasty of the player
	const wyrmgus::age *age = nullptr; //the current age the player/faction is in
public:
	std::string AiName; //AI for computer

private:
	bool alive = false; //whether the player has any units

public:
	// friend enemy detection
	int Team = 0;          /// team of player

	Vec2i StartPos = Vec2i(0, 0);  /// map tile start position
	//Wyrmgus start
	int StartMapLayer = 0;  /// map tile start map layer
	//Wyrmgus end

private:
	CPlayer *overlord = nullptr;	/// overlord of this player
	wyrmgus::vassalage_type vassalage_type;

	std::vector<CPlayer *> vassals;	/// vassals of this player

private:
	resource_map<int> resources;      /// resources in overall store
	resource_map<int> max_resources;   /// max resources can be stored
	resource_map<int> stored_resources;/// resources in store buildings (can't exceed MaxResources)
	resource_map<int> last_resources;  /// last values for revenue
	resource_map<int> incomes;        /// income of the resources
	resource_map<int> revenues;       /// income rate of the resources
	//Wyrmgus start
	resource_map<int> prices;		  /// price of each resource
	resource_map<int> resource_demands; /// demand for the resources
	resource_map<int> stored_resource_demands; /// stored demand for the resources (converted into a trade action when reaches 100)
	
public:
	int TradeCost;					/// cost of trading
	//Wyrmgus end

private:
	resource_map<int> resource_harvest_speeds; /// speed factor for harvesting resources
	resource_map<int> resource_return_speeds;  /// speed factor for returning resources
public:
	int SpeedBuild;                  /// speed factor for building
	int SpeedTrain;                  /// speed factor for training
	int SpeedUpgrade;                /// speed factor for upgrading
	int SpeedResearch;               /// speed factor for researching
private:
	int infantry_cost_modifier = 0;
	int cavalry_cost_modifier = 0;

public:
	wyrmgus::unit_type_map<int> UnitTypesCount;  						/// total units of unit-type
	wyrmgus::unit_type_map<int> UnitTypesUnderConstructionCount;  		/// total under construction units of unit-type
	wyrmgus::unit_type_map<int> UnitTypesAiActiveCount;  				/// total units of unit-type that have their AI set to active
private:
	wyrmgus::unit_type_map<std::vector<CUnit *>> units_by_type; //units owned by this player for each type
	wyrmgus::unit_class_map<std::vector<CUnit *>> units_by_class;
public:
	wyrmgus::unit_type_map<std::vector<CUnit *>> AiActiveUnitsByType;	/// AI active units owned by this player for each type
	std::vector<CUnit *> Heroes;							/// hero units owned by this player
	std::vector<const wyrmgus::deity *> Deities;			/// deities chosen by this player
private:
	std::vector<wyrmgus::quest *> available_quests;			/// quests available to this player
	std::vector<wyrmgus::quest *> current_quests;			/// quests being pursued by this player
	std::vector<const wyrmgus::quest *> completed_quests;	/// quests completed by this player
	std::vector<std::unique_ptr<wyrmgus::player_quest_objective>> quest_objectives; //objectives of the player's current quests
public:
	std::vector<std::pair<CUpgrade *, int>> Modifiers;						/// Modifiers affecting the player, and until which cycle it should last
	std::vector<int> AutosellResources;
	//Wyrmgus end

	bool AiEnabled = false; //handle AI on local computer
private:
	bool revealed = false; //whether the player has been revealed (i.e. after losing the last town hall)
public:
	std::unique_ptr<PlayerAi> Ai;          /// Ai structure pointer

	int NumBuildings = 0;   /// # buildings
	//Wyrmgus start
	int NumBuildingsUnderConstruction = 0; /// # buildings under construction
	int NumTownHalls = 0;
	//Wyrmgus end
	int Supply = 0;         /// supply available/produced
	int Demand = 0;         /// demand of player

	int UnitLimit;       /// # food units allowed
	int BuildingLimit;   /// # buildings allowed
	int TotalUnitLimit;  /// # total unit number allowed

private:
	int score = 0; //player score points
	int military_score = 0; //how militarily powerful the player is
public:
	int TotalUnits = 0;
	int TotalBuildings = 0;
private:
	resource_map<int> resource_totals;
public:
	int TotalRazings = 0;
	int TotalKills = 0;      /// How many units killed
	//Wyrmgus start
	int UnitTypeKills[UnitTypeMax];  /// total killed units of each unit type
	//Wyrmgus end

	//Wyrmgus start
	int LostTownHallTimer = 0;	/// The timer for when the player lost the last town hall (to make the player's units be revealed)
	int HeroCooldownTimer = 0;	/// The cooldown timer for recruiting heroes
	//Wyrmgus end
	
private:
	const wyrmgus::player_color *player_color = nullptr; /// player color for units and portraits

public:
	std::vector<CUnit *> FreeWorkers;	/// Container for free workers
	//Wyrmgus start
	std::vector<CUnit *> LevelUpUnits;	/// Container for units with available level up upgrades
	//Wyrmgus end

	// Upgrades/Allows:
	CAllow Allow;                 /// Allowed for player
	CUpgradeTimers UpgradeTimers; /// Timer for the upgrades

private:
	std::vector<CUnit *> Units; /// units of this player
	CUnit *last_created_unit = nullptr;
	player_index_set enemies; //enemies for this player
	player_index_set allies; //allies for this player
	player_index_set shared_vision; //set of player indexes that this player has shared vision with
	player_index_set mutual_shared_vision; //set of player indexes that this player has mutual shared vision with
	player_set recent_trade_partners;
	resource_set current_special_resources; //the special resources currently produced or stored by the player
	mutable std::shared_mutex mutex; //mutex for protecting player data which is written from the Wyrmgus thread, but which can be read from the Qt thread

	friend void CleanPlayers();
	friend void SetPlayersPalette();
	friend int ::CclUnit(lua_State *l);
};

//Wyrmgus start
class CAiBuildingTemplate final
{
public:
	const wyrmgus::unit_class *get_unit_class() const
	{
		return this->unit_class;
	}

	void set_unit_class(const wyrmgus::unit_class *unit_class)
	{
		this->unit_class = unit_class;
	}

	int get_priority() const
	{
		return this->priority;
	}

	void set_priority(const int priority)
	{
		this->priority = priority;
	}

	bool is_per_settlement() const
	{
		return this->per_settlement;
	}

	void set_per_settlement(const bool per_settlement)
	{
		this->per_settlement = per_settlement;
	}

private:
	const wyrmgus::unit_class *unit_class = nullptr; /// Building's unit class
	int priority = 100;
	bool per_settlement = false;	/// Whether the building should be constructed for each settlement
};
//Wyrmgus end

/**
**  Races for the player
**  Mapped with #PlayerRaces to a symbolic name.
*/
class PlayerRace
{
public:
	//Wyrmgus start
	std::string TranslateName(const std::string &name, const wyrmgus::language *language);
	//Wyrmgus end

public:
	//Wyrmgus start
	std::map<ButtonCmd, IconConfig> ButtonIcons[MAX_RACES];					/// icons for button actions
	//Wyrmgus end
};

/**
**  Notify types. Noties are send to the player.
*/
enum NotifyType {
	NotifyRed,     /// Red alram
	NotifyYellow,  /// Yellow alarm
	NotifyGreen    /// Green alarm
};

extern int NumPlayers; //how many player slots used
extern bool NoRescueCheck; //disable rescue check

extern PlayerRace PlayerRaces;  /// Player races

/// Init players
extern void InitPlayers();
/// Clean up players
extern void CleanPlayers();
/// Save players
extern void SavePlayers(CFile &file);

/// Create a new player
extern void CreatePlayer(const player_type type);

//Wyrmgus start
extern CPlayer *GetFactionPlayer(const wyrmgus::faction *faction);
extern CPlayer *GetOrAddFactionPlayer(const wyrmgus::faction *faction);
//Wyrmgus end

/// Initialize the computer opponent AI
extern void PlayersInitAi();
/// Called each game cycle for player handlers (AI)
extern void PlayersEachCycle();
/// Called each second for a given player handler (AI)
extern void PlayersEachSecond(int player);
//Wyrmgus start
/// Called each half minute for a given player handler (AI)
extern void PlayersEachHalfMinute(int player);
/// Called each minute for a given player handler (AI)
extern void PlayersEachMinute(int player);
//Wyrmgus end

/// register ccl features
extern void PlayerCclRegister();

/// Allowed to select multiple units, maybe not mine
inline bool CanSelectMultipleUnits(const CPlayer &player)
{
	return &player == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->IsTeamed(player);
}

//Wyrmgus start
//for tolua++
enum PlayerTypes {
	PlayerNeutral = 2,
	PlayerNobody = 3,
	PlayerComputer = 4,
	PlayerPerson = 5,
	PlayerRescuePassive = 6,
	PlayerRescueActive = 7,
};
