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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "item/item_slot.h"
#include "player/player_container.h"
#include "spell/spell_container.h"
#include "unit/unit_class_container.h"
#include "unit/unit_type.h"
#include "unit/unit_type_container.h"
#include "unit/unit_variable.h"
#include "vec2i.h"

class CAnimation;
class CFile;
class CMapLayer;
class COrder;
class CPlayer;
class CUnit;
class CUpgrade;
class CViewport;
class Missile;
class PathFinderData;
enum class UnitAction : char;
enum class VariableAttribute;
struct lua_State;

extern int CclUnit(lua_State *l);

namespace wyrmgus {
	class animation_set;
	class character;
	class civilization_base;
	class construction;
	class construction_frame;
	class epithet;
	class landmass;
	class map_template;
	class on_top_build_restriction;
	class player_color;
	class renderer;
	class site;
	class spell;
	class tile;
	class time_of_day;
	class unique_item;
	class unit_manager;
	class unit_ref;
	class unit_stats;
	class unit_type;
	class unit_type_variation;
	class world;
	enum class gender;
	enum class item_class;
	enum class item_slot;
	enum class status_effect;
}

/*
** Configuration of the small (unit) AI.
*/
constexpr int PRIORITY_FACTOR = 0x00080000;
constexpr int HEALTH_FACTOR = 0x00000001;
constexpr int DISTANCE_FACTOR = 0x00010000;
constexpr int INRANGE_FACTOR = 0x00008000;
constexpr int INRANGE_BONUS = 0x01000000;
constexpr int CANATTACK_BONUS = 0x00080000;
constexpr int AIPRIORITY_BONUS = 0x04000000;

//the range in which experience is distributed
constexpr int ExperienceRange = 6;

/// Called whenever the selected unit was updated
extern void SelectedUnitChanged();

/// Returns the map distance between to unittype as locations
extern int MapDistanceBetweenTypes(const wyrmgus::unit_type &src, const Vec2i &pos1, int src_z, const wyrmgus::unit_type &dst, const Vec2i &pos2, int dst_z);
								   
extern int MapDistance(const Vec2i &src_size, const Vec2i &pos1, int src_z, const Vec2i &dst_size, const Vec2i &pos2, int dst_z);

/**
**  Unit/Missile headings.
**          N
**  NW              NE
**  W                E
**  SW              SE
**          S
*/
enum _directions_ {
	LookingN  = 0 * 32,      /// Unit looking north
	LookingNE = 1 * 32,      /// Unit looking north east
	LookingE  = 2 * 32,      /// Unit looking east
	LookingSE = 3 * 32,      /// Unit looking south east
	LookingS  = 4 * 32,      /// Unit looking south
	LookingSW = 5 * 32,      /// Unit looking south west
	LookingW  = 6 * 32,      /// Unit looking west
	LookingNW = 7 * 32       /// Unit looking north west
};

constexpr int NextDirection = 32;        /// Next direction N->NE->E...
constexpr int UnitNotSeen = 0x7fffffff;  /// Unit not seen, used by CUnit::SeenFrame

/// The big unit structure
class CUnit final
{
public:
	static constexpr unsigned char max_step_count = 10;
	static constexpr int max_sight_range = 16;
	static constexpr int aura_range = 6;

	CUnit();
	~CUnit();

	void Init();

	std::shared_ptr<wyrmgus::unit_ref> acquire_ref() const;

	int get_ref_count() const
	{
		return this->ref.use_count();
	}

	int get_tile_x() const
	{
		return this->tilePos.x;
	}

	int get_tile_y() const
	{
		return this->tilePos.y;
	}

	COrder *CurrentOrder() const
	{
		return this->Orders.front().get();
	}

	UnitAction CurrentAction() const;

	bool IsIdle() const;

	void ClearAction();

	/// Initialize unit structure with default values
	void Init(const wyrmgus::unit_type &type, const bool loading_saved_unit);
	void initialize_base_reference();
	void clear_base_reference();

	/// Assign unit to player
	void AssignToPlayer(CPlayer &player);

	const wyrmgus::player_color *get_player_color() const;

	const wyrmgus::species *get_species() const;
	const wyrmgus::civilization *get_civilization() const;
	const civilization_base *get_civilization_base() const;

	/// Draw a single unit
	void Draw(const CViewport &vp, std::vector<std::function<void(renderer *)>> &render_commands) const;
	/// Place a unit on map
	//Wyrmgus start
//	void Place(const Vec2i &pos);
	void Place(const Vec2i &pos, const int z);
	//Wyrmgus end

	/// Move unit to tile(pos). (Do special stuff : vision, cachelist, pathfinding)
	//Wyrmgus start
//	void MoveToXY(const Vec2i &pos);
	void MoveToXY(const Vec2i &pos, const int z);
	//Wyrmgus end
	/// Add a unit inside a container. Only deal with list stuff.
	void AddInContainer(CUnit &host);

	void drop_out_nearest(const Vec2i &goal_pos, const CUnit *container);

private:
	template <typename function_type>
	void drop_out_on_side_base(const int heading, const CUnit *container, const function_type &pos_check_function);

public:
	void drop_out_on_side(const int heading, const CUnit *container);

	std::vector<QPoint> get_tile_positions_in_distance_to(const QSize &target_size, const int min_distance, const int max_distance) const;

	//Wyrmgus start
	void UpdateContainerAttackRange();
	void UpdateXPRequired();
	void UpdatePersonalName(bool update_settlement_name = true);
	void update_epithet();
	void UpdateSettlement();
	void UpdateBuildingSettlementAssignment(const wyrmgus::site *old_settlement = nullptr); //update the settlement assignment of surrounding buildings for this town hall
	//Wyrmgus end
	void on_variable_changed(const int var_index, const int change);
	void XPChanged();
	/// Change owner of unit
	//Wyrmgus start
//	void ChangeOwner(CPlayer &newplayer);
	void ChangeOwner(CPlayer &newplayer, bool show_change = false);
	//Wyrmgus end

	/// Remove unit from map/groups/...
	#ifdef __MORPHOS__
	#undef Remove
	#endif
	void Remove(CUnit *host);

	void AssignWorkerToMine(CUnit &mine);
	void DeAssignWorkerFromMine(CUnit &mine);

	/// Release a unit
	void Release(bool final = false);
	
	//Wyrmgus start
	void SetResourcesHeld(int quantity);
	void ChangeResourcesHeld(int quantity);
	void ReplaceOnTop(CUnit &replaced_unit);
	//Wyrmgus end
	void restore_ontop();

	void set_experience(const int amount);
	void change_experience(const int base_amount, const int around_range = 0); //around_range is the range around the unit that other units of the same player will receive experience
	void IncreaseLevel(int level_quantity, bool automatic_learning = true);

	void restore_hp(const int amount);
	void restore_hp_percent(const int percent);
	void restore_mana(const int amount);
	void restore_mana_percent(const int percent);

	//Wyrmgus start
	void Retrain();
	void auto_use_item();
	void set_character(wyrmgus::character *character);
	void SetCharacter(const std::string &character_identifier, const bool custom_hero = false);
	void apply_character_properties();
	bool CheckTerrainForVariation(const wyrmgus::unit_type_variation *variation) const;
	bool CheckSeasonForVariation(const wyrmgus::unit_type_variation *variation) const;
	bool can_have_variation(const wyrmgus::unit_type_variation *variation) const;
	void ChooseVariation(const wyrmgus::unit_type *new_type = nullptr, const bool ignore_old_variation = false, const int image_layer = -1, const bool notify = true);
	void SetVariation(const wyrmgus::unit_type_variation *new_variation, const int image_layer = -1, const bool notify = true);
	const wyrmgus::unit_type_variation *GetVariation() const;
	const wyrmgus::unit_type_variation *GetLayerVariation(const unsigned int image_layer) const;
	void UpdateButtonIcons();
	void ChooseButtonIcon(const ButtonCmd button_action);
	void EquipItem(CUnit &item, bool affect_character = true);
	void DeequipItem(CUnit &item, bool affect_character = true);
	void ReadWork(const CUpgrade *work, bool affect_character = true);
	void ConsumeElixir(const CUpgrade *elixir, bool affect_character = true);
	int get_aura_range() const;
	void ApplyAura(const int aura_index);
	void ApplyAuraEffect(const int aura_index);
	void SetPrefix(const CUpgrade *prefix);
	void SetSuffix(const CUpgrade *suffix);
	void SetSpell(const wyrmgus::spell *spell);
	void SetWork(const CUpgrade *work);
	void SetElixir(const CUpgrade *elixir);

	const wyrmgus::unique_item *get_unique() const
	{
		return this->unique;
	}

	void set_unique(const wyrmgus::unique_item *unique);

	const wyrmgus::character *get_character() const
	{
		return this->character;
	}

	wyrmgus::character *get_character()
	{
		return this->character;
	}

	const wyrmgus::site *get_site() const
	{
		return this->site;
	}

	void set_site(const wyrmgus::site *site);
	void update_site_owner();

	const wyrmgus::site *get_settlement() const
	{
		return this->settlement;
	}

	void set_settlement(const wyrmgus::site *settlement);

	const wyrmgus::site *get_home_settlement() const
	{
		return this->home_settlement;
	}

	void set_home_settlement(const wyrmgus::site *settlement);
	void update_home_settlement();

	void Identify();
	void CheckIdentification();
	void CheckKnowledgeChange(int variable, int change);
	void UpdateItemName();
	void GenerateDrop();
	void generate_special_properties(const CUnit *dropper, const CPlayer *dropper_player, const bool allow_unique, const bool sold_item, const bool always_magic);
	void generate_prefix(const CUnit *dropper, const CPlayer *dropper_player);
	void generate_suffix(const CUnit *dropper, const CPlayer *dropper_player);
	void generate_spell(const CUnit *dropper, const CPlayer *dropper_player);
	void generate_work(const CUnit *dropper, const CPlayer *dropper_player);
	void generate_unique(const CUnit *dropper, const CPlayer *dropper_player);

	void UpdateSoldUnits();
	void SellUnit(CUnit *sold_unit, int player);

	void spawn_units();
	void spawn_units(const std::vector<const unit_type *> &spawned_types);
	int get_nearby_spawned_demand(const std::vector<const unit_type *> &spawned_types) const;

	void ProduceResource(const resource *resource);
	void sell_resource(const resource *resource, const int player);
	void buy_resource(const resource *resource, const int player);
	void Scout();
	//Wyrmgus end
	
	bool RestoreOrder();
	bool CanStoreOrder(COrder *order);

	void clear_orders();
	void clear_special_orders();

	void clear_all_orders()
	{
		this->clear_orders();
		this->clear_special_orders();
	}

	// Cowards and invisible units don't attack unless ordered.
	bool IsAgressive() const;

	/// Returns true, if unit is directly seen by an allied unit.
	bool IsVisible(const CPlayer &player) const;

	bool is_invisible(const CPlayer &player) const;

	/**
	**  Returns true if unit is alive.
	**  Another unit can interact only with alive map units.
	**
	**  @return        True if alive, false otherwise.
	*/
	bool IsAlive() const;

	/**
	**  Returns true if unit is alive and on the map.
	**  Another unit can interact only with alive map units.
	**
	**  @return        True if alive, false otherwise.
	*/
	bool IsAliveOnMap() const
	{
		return !Removed && IsAlive();
	}

	/**
	**  Returns true, if unit is visible as an action goal for a player on the map.
	**
	**  @param player  Player to check for.
	**
	**  @return        True if visible, false otherwise.
	*/
	bool IsVisibleAsGoal(const CPlayer &player) const
	{
		// Invisibility
		if (this->is_invisible(player)) {
			return false;
		}
		// Don't attack revealers
		if (this->Type->BoolFlag[REVEALER_INDEX].value) {
			return false;
		}
		//Wyrmgus start
//		if ((player.get_type() == player_type::computer && !this->Type->BoolFlag[PERMANENTCLOAK_INDEX].value)
		if (
		//Wyrmgus end
			//Wyrmgus start
//			|| IsVisible(player) || IsVisibleOnRadar(player)) {
			IsVisible(player) || IsVisibleOnRadar(player)) {
			//Wyrmgus end
			return IsAliveOnMap();
		} else {
			return Type->BoolFlag[VISIBLEUNDERFOG_INDEX].value
				   && this->is_seen_by_player(&player)
				   && !this->is_seen_destroyed_by_player(&player);
		}
	}

	/**
	**  Returns true, if unit is visible for this player on the map.
	**  The unit has to be out of fog of war and alive
	**
	**  @param player  Player to check for.
	**
	**  @return        True if visible, false otherwise.
	*/
	bool IsVisibleOnMap(const CPlayer &player) const
	{
		return IsAliveOnMap() && !this->is_invisible(player) && IsVisible(player);
	}

	/// Returns true if unit is visible on minimap. Only for ThisPlayer.
	bool IsVisibleOnMinimap() const;

	// Returns true if unit is visible under radar (By player, or by shared vision)
	bool IsVisibleOnRadar(const CPlayer &pradar) const;

	/// Returns true if unit is visible in a viewport. Only for ThisPlayer.
	bool IsVisibleInViewport(const CViewport &vp) const;

	bool is_enemy_of(const CPlayer &player) const;
	bool is_enemy_of(const CUnit &unit) const;
	bool is_allied_with(const CPlayer &player) const;
	bool is_allied_with(const CUnit &unit) const;
	bool has_shared_vision_with(const CPlayer *player) const;
	bool has_shared_vision_with(const CUnit &unit) const;
	bool has_mutual_shared_vision_with(const CPlayer *player) const;
	bool has_mutual_shared_vision_with(const CUnit &unit) const;
	bool IsTeamed(const CPlayer &player) const;
	bool IsTeamed(const CUnit &unit) const;

	bool IsUnusable(const bool ignore_built_state = false) const;

	bool is_ai_active() const
	{
		return this->Active != 0;
	}

	bool is_under_construction() const
	{
		return this->UnderConstruction != 0;
	}

	int MapDistanceTo(const CUnit &dst) const;

	int MapDistanceTo(const Vec2i &pos, int z) const;

	/**
	**  Test if unit can move.
	**  For the moment only check for move animation.
	**
	**  @return true if unit can move.
	*/
	bool CanMove() const
	{
		return this->Type->CanMove();
	}

	int GetDrawLevel() const;

	bool IsAttackRanged(CUnit *goal, const Vec2i &goalPos, int z);

	PixelPos get_map_pixel_pos_top_left() const;
	PixelPos get_scaled_map_pixel_pos_top_left() const;
	PixelPos get_map_pixel_pos_center() const;
	PixelPos get_scaled_map_pixel_pos_center() const;
	
	const QSize &get_tile_size() const;

	int get_tile_width() const
	{
		return this->get_tile_size().width();
	}

	int get_tile_height() const
	{
		return this->get_tile_size().height();
	}

	//Wyrmgus start
	Vec2i GetHalfTileSize() const;
	PixelSize get_tile_pixel_size() const;
	PixelSize get_scaled_tile_pixel_size() const;
	PixelSize get_half_tile_pixel_size() const;
	PixelSize get_scaled_half_tile_pixel_size() const;
	QPoint get_bottom_right_tile_pos() const;
	QPoint get_center_tile_pos() const;
	const wyrmgus::tile *get_center_tile() const;

	QRect get_tile_rect() const
	{
		return QRect(this->tilePos, this->get_bottom_right_tile_pos());
	}

	const QPoint &get_pixel_offset() const
	{
		return this->pixel_offset;
	}

	QPoint get_scaled_pixel_offset() const;

	const resource *get_given_resource() const;
	const resource *get_current_resource() const;
	
	CUnit *GetFirstContainer() const;

	const std::vector<CUnit *> &get_units_inside() const
	{
		return this->units_inside;
	}

	bool has_units_inside() const
	{
		return !this->get_units_inside().empty();
	}

	void add_unit_inside(CUnit *unit)
	{
		this->units_inside.push_back(unit);
	}

	void remove_unit_inside(CUnit *unit)
	{
		std::erase(this->units_inside, unit);
	}

	bool has_rally_point() const
	{
		return this->get_rally_point_pos().x() != -1 && this->get_rally_point_pos().y() != -1;
	}

	const QPoint &get_rally_point_pos() const
	{
		return this->rally_point_pos;
	}

	const CMapLayer *get_rally_point_map_layer() const
	{
		return this->rally_point_map_layer;
	}

	void set_rally_point(const QPoint &pos, const CMapLayer *map_layer)
	{
		this->rally_point_pos = pos;
		this->rally_point_map_layer = map_layer;
	}

	void SetIndividualUpgrade(const CUpgrade *upgrade, int quantity);
	int GetIndividualUpgrade(const CUpgrade *upgrade) const;
	int GetAvailableLevelUpUpgrades(bool only_units = false) const;

	int get_variable_value(const int var_index) const
	{
		return this->Variable[var_index].Value;
	}

	void set_variable_value(const int var_index, const int value)
	{
		this->Variable[var_index].Value = value;
	}

	void change_variable_value(const int var_index, const int change)
	{
		this->set_variable_value(var_index, this->get_variable_value(var_index) + change);
	}

	int get_variable_max(const int var_index) const
	{
		return this->Variable[var_index].Max;
	}

	void set_variable_max(const int var_index, const int max)
	{
		this->Variable[var_index].Max = max;
	}

	char get_variable_increase(const int var_index) const
	{
		return this->Variable[var_index].Increase;
	}

	int GetModifiedVariable(const int index, const VariableAttribute variable_type) const;
	int GetModifiedVariable(const int index) const;

	int get_best_attack_range() const;

	int GetReactionRange() const;
	unsigned get_item_slot_quantity(const wyrmgus::item_slot item_slot) const;
	wyrmgus::item_class GetCurrentWeaponClass() const;
	int GetItemVariableChange(const CUnit *item, int variable_index, bool increase = false) const;
	const CPlayer *get_display_player() const;
	int GetPrice() const;

	const unit_type_map<int> &get_unit_stocks() const
	{
		return this->unit_stocks;
	}

	int get_unit_stock(const unit_type *unit_type) const
	{
		if (unit_type == nullptr) {
			return 0;
		}

		const auto find_iterator = this->unit_stocks.find(unit_type);
		if (find_iterator != this->unit_stocks.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_unit_stock(const unit_type *unit_type, const int quantity)
	{
		if (!unit_type) {
			return;
		}

		if (quantity <= 0) {
			if (this->unit_stocks.contains(unit_type)) {
				this->unit_stocks.erase(unit_type);
			}
		} else {
			this->unit_stocks[unit_type] = quantity;
		}
	}

	void change_unit_stock(const unit_type *unit_type, const int quantity)
	{
		this->set_unit_stock(unit_type, this->get_unit_stock(unit_type) + quantity);
	}

	const unit_class_map<int> &get_unit_class_stocks() const
	{
		return this->unit_class_stocks;
	}

	int get_unit_class_stock(const unit_class *unit_class) const
	{
		if (unit_class == nullptr) {
			return 0;
		}

		const auto find_iterator = this->unit_class_stocks.find(unit_class);
		if (find_iterator != this->unit_class_stocks.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_unit_class_stock(const unit_class *unit_class, const int quantity)
	{
		if (unit_class == nullptr) {
			return;
		}

		if (quantity <= 0) {
			if (this->unit_class_stocks.contains(unit_class)) {
				this->unit_class_stocks.erase(unit_class);
			}
		} else {
			this->unit_class_stocks[unit_class] = quantity;
		}
	}

	void change_unit_class_stock(const unit_class *unit_class, const int quantity)
	{
		this->set_unit_class_stock(unit_class, this->get_unit_class_stock(unit_class) + quantity);
	}

	int get_unit_stock_replenishment_timer(const unit_type *unit_type) const
	{
		const auto find_iterator = this->unit_stock_replenishment_timers.find(unit_type);

		if (find_iterator != this->unit_stock_replenishment_timers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_unit_stock_replenishment_timer(const unit_type *unit_type, const int quantity)
	{
		if (unit_type == nullptr) {
			return;
		}

		if (quantity <= 0) {
			if (this->unit_stock_replenishment_timers.contains(unit_type)) {
				this->unit_stock_replenishment_timers.erase(unit_type);
			}
		} else {
			this->unit_stock_replenishment_timers[unit_type] = quantity;
		}
	}

	void change_unit_stock_replenishment_timer(const unit_type *unit_type, const int quantity)
	{
		this->set_unit_stock_replenishment_timer(unit_type, this->get_unit_stock_replenishment_timer(unit_type) + quantity);
	}

	int get_unit_class_stock_replenishment_timer(const unit_class *unit_class) const
	{
		const auto find_iterator = this->unit_class_stock_replenishment_timers.find(unit_class);

		if (find_iterator != this->unit_class_stock_replenishment_timers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_unit_class_stock_replenishment_timer(const unit_class *unit_class, const int quantity)
	{
		if (unit_class == nullptr) {
			return;
		}

		if (quantity <= 0) {
			if (this->unit_class_stock_replenishment_timers.contains(unit_class)) {
				this->unit_class_stock_replenishment_timers.erase(unit_class);
			}
		} else {
			this->unit_class_stock_replenishment_timers[unit_class] = quantity;
		}
	}

	void change_unit_class_stock_replenishment_timer(const unit_class *unit_class, const int quantity)
	{
		this->set_unit_class_stock_replenishment_timer(unit_class, this->get_unit_class_stock_replenishment_timer(unit_class) + quantity);
	}

	int get_resource_step(const resource *resource) const;
	int GetTotalInsideCount(const CPlayer *player = nullptr, const bool ignore_items = true, const bool ignore_saved_cargo = false, const wyrmgus::unit_type *type = nullptr) const;
	bool CanAttack(bool count_inside = true) const;
	bool IsInCombat() const;
	bool can_harvest(const resource *resource) const;
	bool can_harvest(const CUnit *dest, const bool only_harvestable = true) const;
	bool can_return_goods_to(const CUnit *dest, const resource *resource = nullptr) const;
	bool can_repair() const;
	bool CanCastSpell(const wyrmgus::spell *spell, const bool ignore_mana_and_cooldown) const;
	bool CanCastAnySpell() const;

	const std::vector<const wyrmgus::spell *> &get_autocast_spells() const
	{
		return this->autocast_spells;
	}

	bool is_autocast_spell(const wyrmgus::spell *spell) const;
	void add_autocast_spell(const wyrmgus::spell *spell);
	void remove_autocast_spell(const wyrmgus::spell *spell);
	bool CanAutoCastSpell(const wyrmgus::spell *spell) const;

	const wyrmgus::spell_map<int> &get_spell_cooldown_timers() const
	{
		return this->spell_cooldown_timers;
	}

	int get_spell_cooldown_timer(const wyrmgus::spell *spell) const
	{
		const auto find_iterator = this->spell_cooldown_timers.find(spell);
		if (find_iterator != this->spell_cooldown_timers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_spell_cooldown_timer(const wyrmgus::spell *spell, const int cooldown)
	{
		this->spell_cooldown_timers[spell] = cooldown;
	}

	void decrement_spell_cooldown_timers()
	{
		if (this->spell_cooldown_timers.empty()) {
			return;
		}

		std::vector<const wyrmgus::spell *> spells_to_remove;

		for (auto &kv_pair : this->spell_cooldown_timers) {
			int &cooldown = kv_pair.second;
			--cooldown;

			if (cooldown <= 0) {
				spells_to_remove.push_back(kv_pair.first);
			}
		}

		for (const wyrmgus::spell *spell : spells_to_remove) {
			this->spell_cooldown_timers.erase(spell);
		}
	}

	bool has_status_effect(const status_effect status_effect) const
	{
		return this->status_effect_timers.contains(status_effect);
	}

	void apply_status_effect(const status_effect status_effect, const int cycles)
	{
		this->set_status_effect_timer(status_effect, std::max(cycles, this->get_status_effect_timer(status_effect)));
	}

	void remove_status_effect(const status_effect status_effect)
	{
		this->set_status_effect_timer(status_effect, 0);
	}

	const std::map<status_effect, int> &get_status_effect_timers() const
	{
		return this->status_effect_timers;
	}

	int get_status_effect_timer(const status_effect status_effect) const
	{
		const auto find_iterator = this->status_effect_timers.find(status_effect);
		if (find_iterator != this->status_effect_timers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_status_effect_timer(const status_effect status_effect, const int cycles)
	{
		if (cycles <= 0) {
			if (this->status_effect_timers.contains(status_effect)) {
				this->status_effect_timers.erase(status_effect);
			}
		} else {
			this->status_effect_timers[status_effect] = cycles;
		}
	}

	void decrement_status_effect_timers()
	{
		if (this->status_effect_timers.empty()) {
			return;
		}

		std::vector<status_effect> effects_to_remove;

		for (auto &[status_effect, cycles] : this->status_effect_timers) {
			--cycles;

			if (cycles <= 0) {
				effects_to_remove.push_back(status_effect);
			}
		}

		for (const status_effect status_effect : effects_to_remove) {
			this->status_effect_timers.erase(status_effect);
		}
	}

	bool IsItemEquipped(const CUnit *item) const;
	bool is_item_class_equipped(const wyrmgus::item_class item_class) const;
	bool is_item_type_equipped(const wyrmgus::unit_type *item_type) const;
	bool is_unique_item_equipped(const unique_item *unique) const;
	bool is_equipped() const;
	bool can_equip_item(const CUnit *item) const;
	bool can_equip_item_class(const item_class item_class) const;
	bool CanUseItem(CUnit *item) const;
	bool is_item_set_complete(const CUnit *item) const;
	bool equipping_item_completes_set(const CUnit *item) const;
	bool DeequippingItemBreaksSet(const CUnit *item) const;
	bool HasInventory() const;

	template <bool precondition = false>
	bool can_learn_ability(const CUpgrade *ability) const;

	bool CanEat(const CUnit &unit) const;
	int level_check_score(const int level) const;
	bool LevelCheck(const int level) const;
	bool is_spell_empowered(const wyrmgus::spell *spell) const;
	bool UpgradeRemovesExistingUpgrade(const CUpgrade *upgrade) const;
	bool HasAdjacentRailForUnitType(const wyrmgus::unit_type *type) const;
	const wyrmgus::animation_set *get_animation_set() const;
	const wyrmgus::construction *get_construction() const;
	const wyrmgus::icon *get_icon() const;
	const wyrmgus::icon *GetButtonIcon(const ButtonCmd button_action) const;
	MissileConfig GetMissile() const;
	const std::shared_ptr<CPlayerColorGraphic> &GetLayerSprite(int image_layer) const;
	const std::string &get_simple_name() const;
	std::string get_full_name() const;

	const std::string &get_base_type_name() const;
	std::string get_type_name() const;
	std::string GetMessageName() const;
	//Wyrmgus end

	const wyrmgus::epithet *get_epithet() const
	{
		return this->epithet;
	}

	const std::string &get_surname() const
	{
		return this->surname;
	}

	const wyrmgus::time_of_day *get_center_tile_time_of_day() const;
	const wyrmgus::site *get_center_tile_settlement() const;
	const CPlayer *get_center_tile_owner() const;
	const landmass *get_center_tile_landmass() const;
	const world *get_center_tile_world() const;

	bool is_seen_by_player(const CPlayer *player) const;

	bool is_seen_by_player(const int index) const
	{
		return this->Seen.by_player.contains(index);
	}

	bool is_seen_destroyed_by_player(const CPlayer *player) const;

	bool is_seen_destroyed_by_player(const int index) const
	{
		return this->Seen.destroyed.contains(index);
	}

	bool is_in_tile_rect(const QRect &tile_rect, int z) const;
	bool is_in_subtemplate_area(const wyrmgus::map_template *subtemplate) const;

	wyrmgus::gender get_gender() const;

	unsigned char get_step_count() const
	{
		return this->step_count;
	}

	void increment_step_count()
	{
		++this->step_count;
		this->step_count = std::min(this->get_step_count(), CUnit::max_step_count);
	}

	void reset_step_count()
	{
		this->step_count = 0;
	}

	bool is_capturable() const;
	bool is_near_site(const wyrmgus::site *site) const;
	bool counts_for_military_score() const;

public:
	class CUnitManagerData final
	{
	public:
		int GetUnitId() const
		{
			return this->slot;
		}

	private:
		int slot = -1;           /// index in UnitManager::unitSlots
		int unitSlot = -1;       /// index in UnitManager::units

		friend class wyrmgus::unit_manager;
	};

private:
	std::shared_ptr<wyrmgus::unit_ref> base_ref; //base reference for the unit
	std::weak_ptr<wyrmgus::unit_ref> ref; //the handle to the unit's reference object
public:
	// @note int is faster than shorts
	unsigned int     ReleaseCycle; /// When this unit could be recycled
	CUnitManagerData UnitManagerData;
	size_t PlayerSlot;  /// index in Player->Units

private:
	std::vector<CUnit *> units_inside;
public:
	int    BoardCount;    /// Number of units transported inside.
	CUnit *Container;     /// Pointer to the unit containing it (or 0)

	struct {
		std::vector<std::shared_ptr<wyrmgus::unit_ref>> Workers; ///references to the workers assigned to this resource.
		int Active = 0; /// how many units are harvesting from the resource.
	} Resource; /// Resource still

	//Wyrmgus start
	std::vector<CUnit *> EquippedItems[static_cast<int>(wyrmgus::item_slot::count)];	/// Pointer to unit's equipped items, per slot
	std::vector<CUnit *> SoldUnits;						/// units available for sale at this unit
	//Wyrmgus end
	
	Vec2i tilePos = Vec2i(-1, -1); /// Map position X
private:
	QPoint rally_point_pos = QPoint(-1, -1); //used for storing the rally point position (where units trained by this unit will be sent to)
public:
	//Wyrmgus start
	CMapLayer *MapLayer = nullptr;			/// in which map layer the unit is
private:
	const CMapLayer *rally_point_map_layer = nullptr; //in which map layer the unit's rally point is
	//Wyrmgus end

public:
	unsigned int Offset;/// Map position as flat index offset (x + y * w)

	const wyrmgus::unit_type *Type;        /// Pointer to unit-type (peon,...)
	CPlayer    *Player;            /// Owner of this unit
	const unit_stats *Stats = nullptr;       /// Current unit stats
	int         CurrentSightRange; /// Unit's Current Sight Range

	// Pathfinding stuff:
	std::unique_ptr<PathFinderData> pathFinderData;

	// DISPLAY:
	int         Frame;      /// Image frame: <0 is mirrored
	//Wyrmgus start
	std::string Name;		/// Unit's personal/proper name (if any)
private:
	const wyrmgus::epithet *epithet = nullptr;
	std::string surname;
	wyrmgus::character *character = nullptr; //character represented by this unit
	const wyrmgus::site *settlement = nullptr;	//settlement (for if the unit is a town hall or a building associated to a settlement)
	const wyrmgus::site *site = nullptr; //the site to which the unit belongs, if it is a site unit (not necessarily the same as the settlement, e.g. if the site is a non-major one)
	const wyrmgus::site *home_settlement = nullptr; //home settlement for the unit, i.e. from where it gets its food
public:
	CUpgrade *Trait;	/// Unit's trait
	int Variation;      /// Which of the variations of its unit type this unit has
	int LayerVariation[MaxImageLayers];	/// Which layer variations this unit has
	const CUpgrade *Prefix = nullptr;	/// Item unit's prefix
	const CUpgrade *Suffix = nullptr;	/// Item unit's suffix
	const wyrmgus::spell *Spell = nullptr; /// Item unit's spell
	const CUpgrade *Work = nullptr;		/// Item unit's literary work
	const CUpgrade *Elixir = nullptr;	/// Item unit's elixir
private:
	const wyrmgus::unique_item *unique = nullptr;		/// Whether the item is unique
public:
	bool Bound;			/// Whether the item is bound to its owner
	bool Identified;	/// Whether the item has been identified
	CUnit *ConnectingDestination;	/// Which connector this unit connects to (if any)
	std::map<ButtonCmd, const wyrmgus::icon *> ButtonIcons;				/// icons for button actions
	//Wyrmgus end
	std::map<int, int> IndividualUpgrades;      /// individual upgrades which the unit has (and how many of it the unit has)

	QPoint pixel_offset;         /// pixel image displacement to map position
	unsigned char Direction; //: 8; /// angle (0-255) unit looking
	//Wyrmgus start
	unsigned char GivesResource;	/// The resource currently given by the unit
	//Wyrmgus end
	unsigned char CurrentResource;
	int ResourcesHeld;      /// Resources Held by a unit
private:
	unit_type_map<int> unit_stocks; //how many of each unit type this unit has stocked
	unit_class_map<int> unit_class_stocks;
	unit_type_map<int> unit_stock_replenishment_timers;
	unit_class_map<int> unit_class_stock_replenishment_timers;

public:
	unsigned char DamagedType;   /// Index of damage type of unit which damaged this unit
	unsigned long Attacked;      /// gamecycle unit was last attacked
	unsigned Blink : 3;          /// Let selection rectangle blink
	unsigned Moving : 1;         /// The unit is moving
	unsigned ReCast : 1;         /// Recast again next cycle
	unsigned AutoRepair : 1;     /// True if unit tries to repair on still action.

	unsigned Burning : 1;        /// unit is burning
	unsigned Destroyed : 1;      /// unit is destroyed pending reference
	unsigned Removed : 1;        /// unit is removed (not on map)
	unsigned Selected : 1;       /// unit is selected

	unsigned UnderConstruction : 1;    /// Unit is in construction
	unsigned Active : 1;         /// Unit is active for AI
	unsigned Boarded : 1;        /// Unit is on board a transporter.
	unsigned CacheLock : 1;      /// Unit is on lock by unitcache operations.

	unsigned Summoned : 1;       /// Unit is summoned using spells.
	unsigned Waiting : 1;        /// Unit is waiting and playing its still animation
	unsigned MineLow : 1;        /// This mine got a notification about its resources being low
	
	unsigned TeamSelected;  /// unit is selected by a team member.
	CPlayer *RescuedFrom;        /// The original owner of a rescued unit.
	/// null if the unit was not rescued.
	/* Seen stuff. */
	std::array<int, PlayerMax> VisCount;     /// Unit visibility counts
	struct _seen_stuff_ {
		const wyrmgus::construction_frame *cframe = nullptr; /// Seen construction frame
		int Frame = 0; /// last seen frame/stage of buildings
		const wyrmgus::unit_type *Type = nullptr; /// Pointer to last seen unit-type
		Vec2i tilePos = Vec2i(-1, -1); /// Last unit->tilePos Seen
		QPoint pixel_offset = QPoint(0, 0); /// seen pixel image displacement to map position
		unsigned UnderConstruction : 1 = 0; /// Unit seen construction
		unsigned State : 3 = 0; /// Unit seen build/upgrade state
		wyrmgus::player_index_set destroyed;  /// Unit seen destroyed or not
		wyrmgus::player_index_set by_player;   /// Track unit seen by player
	} Seen;

	std::vector<wyrmgus::unit_variable> Variable; /// array of User Defined variables.

	unsigned long TTL;  /// time to live

	unsigned int GroupId;       /// unit belongs to this group id
	unsigned int LastGroup;     /// unit belongs to this last group

	unsigned int Wait;          /// action counter
	int Threshold;              /// The counter while ai unit couldn't change target.
	
private:
	unsigned char step_count = 0;	/// How many steps the unit has taken without stopping (maximum 10)
	int best_contained_unit_attack_range = 0;

public:
	struct _unit_anim_ {
		const CAnimation *Anim = nullptr;      /// Anim
		const CAnimation *CurrAnim = nullptr;  /// CurrAnim
		int Wait = 0;                    /// Wait
		bool Unbreakable = false;             /// Unbreakable
	} Anim, WaitBackup;

	std::vector<std::unique_ptr<COrder>> Orders; /// orders to process
	std::unique_ptr<COrder> SavedOrder;         /// order to continue after current
	std::unique_ptr<COrder> NewOrder;           /// order for new trained units
	std::unique_ptr<COrder> CriticalOrder;      /// order to do as possible in breakable animation.

private:
	std::vector<const wyrmgus::spell *> autocast_spells; //the list of autocast spells
	spell_map<int> spell_cooldown_timers; //how many cycles the unit needs to wait before spell will be ready
	std::map<status_effect, int> status_effect_timers; //how many cycles need to pass until a status effect wears off

public:
	CUnit *Goal; /// Generic/Teleporter goal pointer

	friend int CclUnit(lua_State *l);
};

constexpr CUnit *NoUnitP = nullptr;        /// return value: for no unit found

/**
**  Returns unit number (unique to this unit)
*/
inline int UnitNumber(const CUnit &unit)
{
	return unit.UnitManagerData.GetUnitId();
}

/**
**  User preference.
*/
class CPreference final
{
public:
	bool ShowSightRange = false;     /// Show sight range.
	bool ShowReactionRange = false;  /// Show reaction range.
	bool ShowAttackRange = false;    /// Show attack range.
	bool PauseOnLeave = true;       /// If true, game pauses when cursor is gone
	bool AiExplores = true;         /// If true, AI sends explorers to search for resources (almost useless thing)
	bool GrayscaleIcons = false;     /// Use grayscaled icons for unavailable units, upgrades, etc
	bool IconsShift = false;         /// Shift icons slightly when you press on them
	bool StereoSound = true;        /// Enables/disables stereo sound effects	
	bool MineNotifications = false;  /// Show mine is running low/depleted messages
	bool DeselectInMine = false;     /// Deselect peasants in mines
	//Wyrmgus start
	bool SepiaForGrayscale = false;		/// Use a sepia filter for grayscale icons
	//Wyrmgus end

	int ShowOrders = 0;			/// How many second show orders of unit on map.
	int ShowNameDelay = 0;		/// How many cycles need to wait until unit's name popup will appear.
	int ShowNameTime = 0;		/// How many cycles need to show unit's name popup.
	std::string SF2Soundfont;/// Path to SF2 soundfont
};

extern CPreference Preference;

// in unit_draw.c
/// @todo could be moved into the user interface ?
extern unsigned long ShowOrdersCount;   /// Show orders for some time
extern unsigned long ShowNameDelay;     /// Delay to show unit's name
extern unsigned long ShowNameTime;      /// Show unit's name for some time
extern bool EnableTrainingQueue;               /// Config: training queues enabled
extern bool RevealAttacker;                    /// Config: reveal attacker enabled
extern int ResourcesMultiBuildersMultiplier;   /// Config: spend resources for building with multiple workers
extern const CViewport *CurrentViewport; /// CurrentViewport
extern void DrawUnitSelection(const CViewport &vp, const CUnit &unit, std::vector<std::function<void(renderer *)>> &render_commands);
extern void (*DrawSelection)(IntColor, IntColor, int, int, int, int, std::vector<std::function<void(renderer *)>> &render_commands);

extern unsigned int MaxSelectable;    /// How many units could be selected
extern std::vector<CUnit *> Selected; /// currently selected units

/// Mark the field with the FieldFlags.
void MarkUnitFieldFlags(const CUnit &unit);
/// Unmark the field with the FieldFlags.
void UnmarkUnitFieldFlags(const CUnit &unit);
/// Update unit->CurrentSightRange.
void UpdateUnitSightRange(CUnit &unit);
/// Create a new unit
extern CUnit *MakeUnit(const wyrmgus::unit_type &type, CPlayer *player);
/// Create a new unit and place on map
extern CUnit *MakeUnitAndPlace(const Vec2i &pos, const wyrmgus::unit_type &type, CPlayer *player, int z);
/// Create a new unit and place it on the map, and update its player accordingly
extern CUnit *CreateUnit(const Vec2i &pos, const unit_type &type, CPlayer *player, const int z, const bool no_building_bordering_impassable = false, const site *settlement = nullptr, const bool ignore_ontop = false);
extern CUnit *CreateResourceUnit(const Vec2i &pos, const wyrmgus::unit_type &type, int z, bool allow_unique = true);
/// Find the nearest position at which unit can be placed.
QPoint FindNearestDrop(const unit_type &type, const QPoint &goal_pos, const int heading, const int z, const bool no_building_bordering_impassable = false, const bool ignore_ontop = false, const site *settlement = nullptr);
/// Handle the loss of a unit (food,...)
extern void UnitLost(CUnit &unit);
/// @todo more docu
extern void UpdateForNewUnit(const CUnit &unit, int upgrade);
/// @todo more docu
extern void NearestOfUnit(const CUnit &unit, const Vec2i &pos, Vec2i *dpos);

/// Call when an Unit goes under fog.
extern void UnitGoesUnderFog(CUnit &unit, const CPlayer &player);
/// Call when an Unit goes out of fog.
extern void UnitGoesOutOfFog(CUnit &unit, const CPlayer &player);

/// Does a recount for VisCount
extern void UnitCountSeen(CUnit &unit);

/// Check for rescue each second
extern void RescueUnits();

/// Convert direction (dx,dy) to heading (0-255)
extern int DirectionToHeading(const Vec2i &dir);

/// Update frame from heading
extern void UnitUpdateHeading(CUnit &unit, const bool notify = true);
/// Heading and frame from delta direction
extern void UnitHeadingFromDeltaXY(CUnit &unit, const Vec2i &delta);

/// Drop out all units in the unit
extern void DropOutAll(const CUnit &unit);

/// Return the rule used to build this building.
extern const on_top_build_restriction *OnTopDetails(const unit_type &type, const unit_type *parent);
/// @todo more docu
extern CUnit *CanBuildHere(const CUnit *unit, const wyrmgus::unit_type &type, const QPoint &pos, const int z, const bool no_bordering_impassable = false);
/// @todo more docu
extern bool CanBuildOn(const QPoint &pos, const tile_flag mask, const int z, const CPlayer *player, const wyrmgus::unit_type *unit_type);
/// FIXME: more docu
extern CUnit *CanBuildUnitType(const CUnit *unit, const wyrmgus::unit_type &type, const QPoint &pos, const int real, const bool ignore_exploration, const int z, const bool no_bordering_impassable = false);
/// Get the suitable animation frame depends of unit's damaged type.
extern int ExtraDeathIndex(const char *death);

/// Get unit under cursor
extern CUnit *UnitOnScreen(int x, int y);

/// Let a unit die
extern void LetUnitDie(CUnit &unit, bool suicide = false);
/// Destroy all units inside another unit
extern void DestroyAllInside(CUnit &source);
/// Calculate some value to measure the unit's priority for AI
extern int ThreatCalculate(const CUnit &unit, const CUnit &dest);
/// Hit unit with damage, if destroyed give attacker the points
//Wyrmgus start
//extern void HitUnit(CUnit *attacker, CUnit &target, int damage, const Missile *missile = nullptr);
extern void HitUnit(CUnit *attacker, CUnit &target, int damage, const Missile *missile = nullptr, bool show_damage = true);
extern void HitUnit_IncreaseScoreForKill(CUnit &attacker, CUnit &target, const bool include_contained_units);
extern void HitUnit_NormalHitSpecialDamageEffects(CUnit &attacker, CUnit &target);
extern void HitUnit_SpecialDamageEffect(CUnit &target, int dmg_var);
extern void HitUnit_RunAway(CUnit &target, const CUnit &attacker);
//Wyrmgus end

/// Calculate the distance from current view point to coordinate
extern int ViewPointDistance(const Vec2i &pos);
/// Calculate the distance from current view point to unit
extern int ViewPointDistanceToUnit(const CUnit &dest);

/// Can transporter transport the other unit
extern int CanTransport(const CUnit &transporter, const CUnit &unit);
//Wyrmgus start
/// Can the unit pick up the other unit
extern bool CanPickUp(const CUnit &picker, const CUnit &unit);
//Wyrmgus end

/// Generate a unit reference, a printable unique string for unit
extern std::string UnitReference(const CUnit *unit);

/// save unit-structure
extern void SaveUnit(const CUnit &unit, CFile &file);

/// Initialize unit module
extern void InitUnits();
/// Clean unit module
extern void CleanUnits();

// in unit_draw.c
//--------------------
/// Draw nothing around unit
extern void DrawSelectionNone(IntColor, IntColor, int, int, int, int, std::vector<std::function<void(renderer *)>> &render_commands);
/// Draw circle around unit
extern void DrawSelectionCircle(IntColor, IntColor, int, int, int, int, std::vector<std::function<void(renderer *)>> &render_commands);
/// Draw circle filled with alpha around unit
extern void DrawSelectionCircleWithTrans(IntColor, IntColor, int, int, int, int, std::vector<std::function<void(renderer *)>> &render_commands);
/// Draw rectangle around unit
extern void DrawSelectionRectangle(IntColor, IntColor, int, int, int, int, std::vector<std::function<void(renderer *)>> &render_commands);
/// Draw rectangle filled with alpha around unit
extern void DrawSelectionRectangleWithTrans(IntColor, IntColor, int, int, int, int, std::vector<std::function<void(renderer *)>> &render_commands);
/// Draw corners around unit
extern void DrawSelectionCorners(IntColor, IntColor, int, int, int, int, std::vector<std::function<void(renderer *)>> &render_commands);

/// Register CCL decorations features
extern void DecorationCclRegister();
/// Get the amount of decorations
extern int GetDecorationsCount();
/// Load the decorations (health,mana) of units
extern void LoadDecorations();
/// Clean the decorations (health,mana) of units
extern void CleanDecorations();

/// Draw unit's shadow
extern void DrawShadow(const unit_type &type, const std::shared_ptr<CGraphic> &sprite, int frame, const PixelPos &screenPos, std::vector<std::function<void(renderer *)>> &render_commands);
//Wyrmgus start
/// Draw unit's overlay
extern void DrawPlayerColorOverlay(const unit_type &type, const std::shared_ptr<CPlayerColorGraphic> &sprite, const int player, int frame, const PixelPos &screenPos, const wyrmgus::time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands);
extern void DrawOverlay(const unit_type &type, const std::shared_ptr<CGraphic> &sprite, int player, int frame, const PixelPos &screenPos, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands);
//Wyrmgus end
/// Draw all units visible on map in viewport
extern int FindAndSortUnits(const CViewport &vp, std::vector<CUnit *> &table);

/// Show a unit's orders.
extern void ShowOrder(const CUnit &unit, std::vector<std::function<void(renderer *)>> &render_commands);

// in groups.c

/// Save groups
extern void SaveGroups(CFile &file);
/// Cleanup groups
extern void CleanGroups();
/// Get the array of units of a particular group
extern const std::vector<CUnit *> &GetUnitsOfGroup(int num);

/// Remove all units from a group
extern void ClearGroup(int num);
/// Add the array of units to the group
extern void AddToGroup(CUnit **units, unsigned int nunits, int num);
/// Set the contents of a particular group with an array of units
extern void SetGroup(CUnit **units, unsigned int nunits, int num);
/// Remove a unit from a group
extern void RemoveUnitFromGroups(CUnit &unit);
//Wyrmgus start
/// Remove a unit from a group which has more than one unit in it
extern void RemoveUnitFromNonSingleGroups(CUnit &unit);
//Wyrmgus end
/// Register CCL group features
extern void GroupCclRegister();
extern bool IsGroupTainted(int num);

// in selection.c

/// Check if unit is the currently only selected
extern bool IsOnlySelected(const CUnit &unit);

///  Save selection to restore after.
extern void SaveSelection();
///  Restore selection.
extern void RestoreSelection();
/// Clear current selection
extern void UnSelectAll();
/// Changed TeamUnit Selection
extern void ChangeTeamSelectedUnits(CPlayer &player, const std::vector<CUnit *> &units);
/// Add a unit to selection
extern int SelectUnit(CUnit &unit);
/// Select one unit as selection
extern void SelectSingleUnit(CUnit &unit);
/// Remove a unit from selection
extern void UnSelectUnit(CUnit &unit);
//Wyrmgus start
/// Check whether two units can be selected together
extern bool UnitCanBeSelectedWith(const CUnit &first_unit, const CUnit &second_unit);
//Wyrmgus end
/// Add a unit to selected if not already selected, remove it otherwise
extern int ToggleSelectUnit(CUnit &unit);
/// Select units from the same type (if selectable by rectangle)
//Wyrmgus start
//extern int SelectUnitsByType(CUnit &base);
extern int SelectUnitsByType(CUnit &base, bool only_visible = true);
//Wyrmgus end
/// Toggle units from the same type (if selectable by rectangle)
extern int ToggleUnitsByType(CUnit &base);
/// Select the units belonging to a particular group
extern int SelectGroup(int group_number, GroupSelectionMode mode = GroupSelectionMode::SELECTABLE_BY_RECTANGLE_ONLY);
/// Add the units from the same group as the one in parameter
extern int AddGroupFromUnitToSelection(const CUnit &unit);
/// Select the units from the same group as the one in parameter
extern int SelectGroupFromUnit(const CUnit &group_unit);
//Wyrmgus start
/// Select entire army
extern int SelectArmy();
//Wyrmgus end
/// Select the units in the selection rectangle
extern int SelectUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright);
/// Select ground units in the selection rectangle
extern int SelectGroundUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright);
/// Select flying units in the selection rectangle
extern int SelectAirUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright);
/// Add the units in the selection rectangle to the current selection
extern int AddSelectedUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright);
/// Add ground units in the selection rectangle to the current selection
extern int AddSelectedGroundUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright);
/// Add flying units in the selection rectangle to the current selection
extern int AddSelectedAirUnitsInRectangle(const PixelPos &corner_topleft, const PixelPos &corner_bottomright);

/// Save current selection state
extern void SaveSelections(CFile &file);
/// Clean up selections
extern void CleanSelections();
/// Register CCL selection features
extern void SelectionCclRegister();

// in ccl_unit.c

/// register CCL units features
extern void UnitCclRegister();
