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
/**@name ai_local.h - The local AI header file. */
//
//      (c) Copyright 2000-2022 by Lutz Sammer, Antonis Chaniotis and Andrettin
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

#include "map/landmass_container.h"
#include "map/site_container.h"
#include "unit/unit_cache.h"
#include "unit/unit_class_container.h"
#include "unit/unit_type_container.h"
#include "upgrade/upgrade_structs.h" // MaxCost
#include "vec2i.h"

#ifdef __MORPHOS__
#undef Wait
#endif

class AiHelper;
class CPlayer;
class CUnit;
class CUpgrade;

namespace wyrmgus {
	class landmass;
	class resource;
	class site;
	class unit_class;
	class unit_ref;
	class unit_type;
	class upgrade_class;
	enum class ai_force_type;
	enum class tile_flag : uint32_t;
}

/**
**  Ai Type structure.
*/
class CAiType
{
public:
	CAiType() {}

	std::string Name;     /// Name of this ai
	std::string Race;     /// for this race
	std::string Class;    /// class of this ai
	std::string Script;   /// Main script
};

/**
**  AI unit-type table with counter in front.
*/
class AiRequestType
{
public:
	unsigned int Count = 0;  /// elements in table
	unit_type *Type = nullptr;     /// the type
	const wyrmgus::landmass *landmass = nullptr; //in which landmass the unit should be created
};

/**
**  Ai unit-type in a force.
*/
class AiUnitType final
{
public:
	unsigned int Want = 0; /// number of this unit-type wanted
	const wyrmgus::unit_type *Type = nullptr;   /// unit-type self
};

/**
**  Roles for forces
*/
enum class AiForceRole {
	Default = 0, /// So default is attacking
	Attack = 0, /// Force should attack
	Defend      /// Force should defend
};

enum class AiForceAttackingState {
	Free = -1,
	Waiting = 0,
	Boarding,
	GoingToRallyPoint,
	AttackingWithTransporter,
	Attacking,
	WaitingForTransporters
};

constexpr int AI_WAIT_ON_RALLY_POINT = 60; /// Max seconds AI units will wait on rally point

/**
**  Define an AI force.
**
**  A force is a group of units belonging together.
*/
class AiForce final
{
public:
	AiForce();
	~AiForce();

	const std::vector<std::shared_ptr<unit_ref>> &get_units() const
	{
		return this->units;
	}

	std::shared_ptr<unit_ref> take_last_unit();

	void Remove(CUnit *unit);
	void Reset(const bool types = false);

	size_t Size() const
	{
		return this->get_units().size();
	}

	bool IsAttacking() const
	{
		return (!Defending && Attacking);
	}

	wyrmgus::ai_force_type get_force_type() const;
	bool IsNaval() const;
	bool IsAirForce() const;
	bool IsHeroOnlyForce() const;
	
	//Wyrmgus start
//	void Attack(const Vec2i &pos);
	void Attack(const Vec2i &pos, int z);
	//Wyrmgus end
	void remove_dead_units();
	int PlanAttack();

	void ReturnToHome();
	//Wyrmgus start
//	bool NewRallyPoint(const Vec2i &startPos, Vec2i *resultPos);
	bool NewRallyPoint(const Vec2i &startPos, Vec2i *resultPos, int z);
	bool check_transporters(const QPoint &pos, const int z);
	//Wyrmgus end
	void Insert(std::shared_ptr<wyrmgus::unit_ref> &&unit);
	void Insert(CUnit *unit);

private:
	void count_types(unit_type_map<unsigned> &counter);
	bool can_be_assigned_to(const unit_type &type);

	void Update();

	static void InternalRemoveUnit(CUnit *unit);

public:
	bool Completed = false;    /// Flag saying force is complete build
	bool Defending = false;    /// Flag saying force is defending
	bool Attacking = false;    /// Flag saying force is attacking
	AiForceRole Role;  /// Role of the force

	std::vector<AiUnitType> UnitTypes; /// Count and types of unit-type
private:
	std::vector<std::shared_ptr<unit_ref>> units;  //units in the force

public:
	// If attacking
	int FormerForce = -1;             /// Original force number
	AiForceAttackingState State; /// Attack state
	Vec2i GoalPos = Vec2i(-1, -1); /// Attack point tile map position
	Vec2i HomePos = Vec2i(-1, -1); /// Return after attack tile map position
	//Wyrmgus start
	int GoalMapLayer = 0;
	int HomeMapLayer = 0;
	//Wyrmgus end
	int WaitOnRallyPoint; /// Counter for waiting on rally point

	friend class AiForceManager;
};

// forces
constexpr int AI_MAX_FORCES = 50;			/// How many forces are supported
constexpr int AI_MAX_FORCE_INTERNAL = AI_MAX_FORCES / 2; /// The forces after AI_MAX_FORCE_INTERNAL are for internal use
constexpr int AI_MAX_COMPLETED_FORCES = AI_MAX_FORCE_INTERNAL - 1; /// How many completed forces the AI should have at maximum
constexpr int AI_MAX_COMPLETED_FORCE_POP = 90; /// How much population the AI completed forces should have at maximum (the AI will produce a new force if it is below this limit, even if that will make it go above it)

/**
**  AI force manager.
**
**  A Forces container for the force manager to handle
*/
class AiForceManager final
{
public:
	AiForceManager();

	inline size_t Size() const { return forces.size(); }

	const AiForce &operator[](unsigned int index) const { return forces[index]; }
	AiForce &operator[](unsigned int index) { return forces[index]; }

	int getIndex(AiForce *force) const
	{
		for (unsigned int i = 0; i < forces.size(); ++i) {
			if (force == &forces[i]) {
				return i;
			}
		}
		return -1;
	}

	unsigned int getScriptForce(unsigned int index)
	{
		if (script[index] == -1) {
			script[index] = FindFreeForce();
		}
		return script[index];
	}

	int GetForce(const CUnit &unit);
	void remove_dead_units();
	//Wyrmgus start
//	bool Assign(CUnit &unit, int force = -1);
	bool Assign(CUnit &unit, int force = -1, bool hero = false);
	//Wyrmgus end
	void Update();
	void UpdatePerHalfMinute();
	void UpdatePerMinute();
	unsigned int FindFreeForce(const AiForceRole role = AiForceRole::Default, int begin = 0, bool allow_hero_only_force = false);
	void CheckUnits(int *counter);
	void CheckForceRecruitment();
private:
	std::vector<AiForce> forces;
	char script[AI_MAX_FORCES];
};

/**
**  AI build queue.
**
**  List of orders for the resource manager to handle
*/
class AiBuildQueue
{
public:
	AiBuildQueue()
	{
		Pos.x = Pos.y = -1;
	}

public:
	unsigned int Want = 0;  /// requested number
	unsigned int Made = 0;  /// built number
	const wyrmgus::unit_type *Type = nullptr;    /// unit-type
	unsigned long Wait = 0; /// wait until this cycle
	Vec2i Pos;          /// build near pos on map
	//Wyrmgus start
	int MapLayer = 0;
	const wyrmgus::landmass *landmass = nullptr;
	const wyrmgus::site *settlement = nullptr;
	//Wyrmgus end
};

/**
**  AI exploration request
*/
class AiExplorationRequest final
{
public:
	explicit AiExplorationRequest(const Vec2i &pos, const tile_flag mask) : pos(pos), Mask(mask)
	{
	}

public:
	Vec2i pos;          /// pos on map
	tile_flag Mask;           /// mask ( ex: tile_flag::land_unit )
};

/**
**  AI variables.
*/
class PlayerAi final
{
public:
	static constexpr int enforced_peace_cycle_count = CYCLES_PER_MINUTE * 5;
	static constexpr int military_score_advantage_threshold = 100; //the relative military score must be 100% greater

	const resource_map<int> &get_reserve() const
	{
		return this->reserve;
	}

	int get_reserve(const resource *resource) const
	{
		const auto find_iterator = this->reserve.find(resource);

		if (find_iterator != this->reserve.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_reserve(const resource *resource, const int quantity)
	{
		if (quantity == 0) {
			if (this->reserve.contains(resource)) {
				this->reserve.erase(resource);
			}
		} else {
			this->reserve[resource] = quantity;
		}
	}

	const resource_map<int> &get_collect() const
	{
		return this->collect;
	}

	int get_collect(const resource *resource) const
	{
		const auto find_iterator = this->collect.find(resource);

		if (find_iterator != this->collect.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_collect(const resource *resource, const int quantity)
	{
		if (quantity == 0) {
			if (this->collect.contains(resource)) {
				this->collect.erase(resource);
			}
		} else {
			this->collect[resource] = quantity;
		}
	}

	void check_settlement_construction();
	void check_settlement_construction(const site_set &settlements);
	void request_settlement_construction(const site *settlement, const unit_type *town_hall_type);

	void check_transporters();

	const landmass_map<std::vector<CUnit *>> &get_transporters() const
	{
		return this->transporters;
	}

	const std::vector<CUnit *> &get_transporters(const landmass *water_landmass) const
	{
		static const std::vector<CUnit *> empty_list;

		const auto find_iterator = this->transporters.find(water_landmass);
		if (find_iterator != this->transporters.end()) {
			return find_iterator->second;
		}

		return empty_list;
	}

	void add_transporter(CUnit *transporter, const landmass *water_landmass)
	{
		this->transporters[water_landmass].push_back(transporter);
	}

	void remove_transporter(CUnit *transporter);

	int get_transport_capacity(const landmass *water_landmass) const;
	int get_requested_transport_capacity(const landmass *water_landmass) const;
	void request_transport_capacity(const int capacity_needed, const landmass *landmass);
	bool check_unit_transport(const std::vector<std::shared_ptr<unit_ref>> &units, const landmass *home_landmass, const QPoint &goal_pos, const int z);

	const site_map<std::vector<std::shared_ptr<unit_ref>>> &get_site_transport_units() const
	{
		return this->site_transport_units;
	}

	bool is_site_transport_unit(const CUnit *unit) const;
	void add_site_transport_unit(CUnit *unit, const site *site);
	void remove_site_transport_unit(CUnit *unit);
	void transport_worker_to_site(const site *site);
	void check_site_transport_units();

	const std::vector<const CUpgrade *> &get_research_requests() const
	{
		return this->research_requests;
	}

	void add_research_request(const CUpgrade *upgrade);
	
	void check_factions();

	void evaluate_diplomacy();
	void check_quest_objectives();

	CPlayer *Player = nullptr;		/// Engine player structure
	CAiType *AiType = nullptr;		/// AI type of this player AI
	// controller
	std::string Script;				/// Script executed
	unsigned long SleepCycles = 0;	/// Cycles to sleep

	class AiForceManager Force;			/// Forces controlled by AI

	// resource manager
private:
	resource_map<int> reserve;			/// Resources to keep in reserve
public:
	resource_map<int> Used;				/// Used resources
	resource_map<int> Needed;			/// Needed resources
private:
	resource_map<int> collect;			/// Collect % of resources
public:
	long long int NeededMask = 0;	/// Mask for needed resources
	bool NeedSupply = false;		/// Flag need food
	bool ScriptDebug = false;		/// Flag script debugging on/off
	bool BuildDepots = true;		/// Build new depots if necessary
	//Wyrmgus start
	bool Scouting = false;			/// Whether the AI player is currently scouting (has no enemies it knows the location of)
	//Wyrmgus end

	std::vector<AiExplorationRequest> FirstExplorationRequest;	/// Requests for exploration
	unsigned long LastExplorationGameCycle = 0;	/// When did the last explore occur?
	unsigned long LastCanNotMoveGameCycle = 0;	/// Last can not move cycle
	std::vector<AiRequestType> UnitTypeRequests;	/// unit-types to build/train request,priority list
	std::vector<wyrmgus::unit_type *> UpgradeToRequests;		/// Upgrade to unit-type requested and priority list
private:
	std::vector<const CUpgrade *> research_requests; //upgrades requested and priority list
public:
	std::vector<AiBuildQueue> UnitTypeBuilt;		/// What the resource manager should build
	int LastRepairBuilding = 0;						/// Last building checked for repair in this turn
	//Wyrmgus start
	int LastPathwayConstructionBuilding = 0;		/// Last building checked for pathway construction in this turn
	std::vector<CUnit *> Scouts;				/// AI scouting units
	//Wyrmgus end
private:
	landmass_map<std::vector<CUnit *>> transporters; //AI transporters, mapped to the sea (water "landmass") they belong to
	site_map<std::vector<std::shared_ptr<unit_ref>>> site_transport_units; //units to be transported to certain sites
};

/**
**  AI Helper.
**
**  Contains information needed for the AI. If the AI needs an unit or
**  building or upgrade or spell, it could lookup in this tables to find
**  where it could be trained, built or researched.
*/
class AiHelper final
{
public:
	void init();

	void clear()
	{
		this->trainers.clear();
		this->trainer_classes.clear();
		this->builders.clear();
		this->builder_classes.clear();
		this->unit_type_upgrades.clear();
		this->unit_type_upgradees.clear();
		this->unit_class_upgrades.clear();
		this->unit_class_upgradees.clear();
		this->researchers.clear();
		this->researcher_classes.clear();
		this->Repair.clear();
		this->UnitLimit.clear();
		this->Mines.clear();
		this->Depots.clear();
		this->SellMarkets.clear();
		this->BuyMarkets.clear();
		this->produced_resources.clear();
		this->researched_upgrades.clear();
		this->researched_upgrade_classes.clear();
		this->ExperienceUpgrades.clear();
		this->LearnableAbilities.clear();
		this->NavalTransporters.clear();
	}

	const std::vector<const wyrmgus::unit_type *> &get_trainers(const wyrmgus::unit_type *unit_type) const
	{
		static std::vector<const wyrmgus::unit_type *> empty_vector;

		const auto find_iterator = this->trainers.find(unit_type);
		if (find_iterator != this->trainers.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	const std::vector<const wyrmgus::unit_class *> &get_trainer_classes(const wyrmgus::unit_class *unit_class) const
	{
		static std::vector<const wyrmgus::unit_class *> empty_vector;

		const auto find_iterator = this->trainer_classes.find(unit_class);
		if (find_iterator != this->trainer_classes.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	const std::vector<const wyrmgus::unit_type *> &get_builders(const wyrmgus::unit_type *unit_type) const
	{
		static std::vector<const wyrmgus::unit_type *> empty_vector;

		const auto find_iterator = this->builders.find(unit_type);
		if (find_iterator != this->builders.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	const std::vector<const wyrmgus::unit_class *> &get_builder_classes(const wyrmgus::unit_class *unit_class) const
	{
		static std::vector<const wyrmgus::unit_class *> empty_vector;

		const auto find_iterator = this->builder_classes.find(unit_class);
		if (find_iterator != this->builder_classes.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	const std::vector<const wyrmgus::unit_type *> &get_unit_type_upgrades(const wyrmgus::unit_type *unit_type) const
	{
		static std::vector<const wyrmgus::unit_type *> empty_vector;

		const auto find_iterator = this->unit_type_upgrades.find(unit_type);
		if (find_iterator != this->unit_type_upgrades.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	const std::vector<const wyrmgus::unit_type *> &get_unit_type_upgradees(const wyrmgus::unit_type *unit_type) const
	{
		static std::vector<const wyrmgus::unit_type *> empty_vector;

		const auto find_iterator = this->unit_type_upgradees.find(unit_type);
		if (find_iterator != this->unit_type_upgradees.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	const std::vector<const wyrmgus::unit_class *> &get_unit_class_upgrades(const wyrmgus::unit_class *unit_class) const
	{
		static std::vector<const wyrmgus::unit_class *> empty_vector;

		const auto find_iterator = this->unit_class_upgrades.find(unit_class);
		if (find_iterator != this->unit_class_upgrades.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	const std::vector<const wyrmgus::unit_class *> &get_unit_class_upgradees(const wyrmgus::unit_class *unit_class) const
	{
		static std::vector<const wyrmgus::unit_class *> empty_vector;

		const auto find_iterator = this->unit_class_upgradees.find(unit_class);
		if (find_iterator != this->unit_class_upgradees.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	const std::vector<const wyrmgus::unit_type *> &get_researchers(const CUpgrade *upgrade) const
	{
		static std::vector<const wyrmgus::unit_type *> empty_vector;

		const auto find_iterator = this->researchers.find(upgrade);
		if (find_iterator != this->researchers.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	const std::vector<const unit_class *> &get_researcher_classes(const upgrade_class *upgrade_class) const
	{
		static std::vector<const unit_class *> empty_vector;

		const auto find_iterator = this->researcher_classes.find(upgrade_class);
		if (find_iterator != this->researcher_classes.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	const std::vector<const resource *> &get_produced_resources(const unit_type *unit_type) const
	{
		static std::vector<const resource *> empty_vector;

		const auto find_iterator = this->produced_resources.find(unit_type);
		if (find_iterator != this->produced_resources.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	const std::vector<const CUpgrade *> &get_researched_upgrades(const unit_type *unit_type) const
	{
		static std::vector<const CUpgrade *> empty_vector;

		const auto find_iterator = this->researched_upgrades.find(unit_type);
		if (find_iterator != this->researched_upgrades.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	const std::vector<const upgrade_class *> &get_researched_upgrade_classes(const unit_class *unit_class) const
	{
		static std::vector<const upgrade_class *> empty_vector;

		const auto find_iterator = this->researched_upgrade_classes.find(unit_class);
		if (find_iterator != this->researched_upgrade_classes.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

private:
	//unit types associated with lists of other unit types which can train them
	unit_type_map<std::vector<const unit_type *>> trainers;

	//unit classes associated with lists of other unit classes which can train them
	unit_class_map<std::vector<const unit_class *>> trainer_classes;

	//(building) unit types associated with lists of other unit types which can build them
	unit_type_map<std::vector<const unit_type *>> builders;

	//(building) unit classes associated with lists of other unit classes which can build them
	unit_class_map<std::vector<const unit_class *>> builder_classes;

	//lists of unit type upgrades, mapped to the unit type for which they are available
	unit_type_map<std::vector<const unit_type *>> unit_type_upgrades;

	//lists of unit types which can perform unit type upgrades, mapped to the unit type to which they can upgrade
	unit_type_map<std::vector<const unit_type *>> unit_type_upgradees;

	//lists of unit class upgrades, mapped to the unit class for which they are available
	unit_class_map<std::vector<const unit_class *>> unit_class_upgrades;

	//lists of unit classes which can perform unit class upgrades, mapped to the unit class to which they can upgrade
	unit_class_map<std::vector<const unit_class *>> unit_class_upgradees;

	//upgrades associated with lists of unit types which can research them
	std::map<const CUpgrade *, std::vector<const unit_type *>> researchers;

	//upgrade classes associated with lists of unit classes which can research them
	std::map<const upgrade_class *, std::vector<const unit_class *>> researcher_classes;

public:
	/**
	** The index is the unit that should be repaired, giving a table of all
	** units/buildings which could repair this unit.
	*/
	std::vector<std::vector<const unit_type *>> Repair;

	/**
	** A table of all units/buildings which could reduce the unit-limit.
	*/
	std::vector<const unit_type *> UnitLimit;

	/**
	** The index is the unit that should be made, giving a table of all
	** units/buildings which are equivalent.
	*/
	std::vector<std::vector<const unit_type *>> Equiv;

	/**
	** The index is the resource id - 1 (we can't mine TIME), giving a table of all
	** units/buildings/mines which can harvest this resource.
	*/
	std::vector<std::vector<const unit_type *>> Mines;

	/**
	** The index is the resource id - 1 (we can't store TIME), giving a table of all
	** units/buildings/mines which can store this resource.
	*/
	std::vector<std::vector<const unit_type *>> Depots;
	
	//Wyrmgus start
	/**
	** The index is the resource id - 1 (we can't trade TIME), giving a table of all
	** units/buildings/mines which can sell this resource. In index 0 (respective to CopperCost) we store all units with the "Market" tag instead.
	*/
	std::vector<std::vector<const unit_type *>> SellMarkets;
	
	/**
	** The index is the resource id - 1 (we can't trade TIME), giving a table of all
	** units/buildings/mines which can buy this resource.
	*/
	std::vector<std::vector<const unit_type *>> BuyMarkets;

private:
	//lists of resources, mapped to the unit types that can produce them
	unit_type_map<std::vector<const resource *>> produced_resources;

	//unit types associated with lists of upgrades which they can research
	unit_type_map<std::vector<const CUpgrade *>> researched_upgrades;

	//unit classes associated with lists of upgrade classes which they can research
	unit_class_map<std::vector<const upgrade_class *>> researched_upgrade_classes;

public:
	/**
	** The index is the unit that should acquire an experience upgrade, giving a table of all
	** possible upgrades for it.
	*/
	std::vector<std::vector<const unit_type *>> ExperienceUpgrades;
	
	/**
	** The index is the unit type that should acquire a learnable ability, giving a table of all
	** abilities that it can learn.
	*/
	std::vector<std::vector<const CUpgrade *>> LearnableAbilities;
	
	/**
	** A table of all naval transporter units.
	*/
	std::vector<const unit_type *> NavalTransporters;
};

extern std::vector<std::unique_ptr<CAiType>> AiTypes;   /// List of all AI types
extern AiHelper AiHelpers; /// AI helper variables

extern int UnitTypeEquivs[UnitTypeMax + 1]; /// equivalence between unittypes
extern PlayerAi *AiPlayer; /// Current AI player

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

//
// Resource manager
//
extern void AiCheckWorkers();
/// Add unit-type request to resource manager
extern void AiAddUnitTypeRequest(const wyrmgus::unit_type &type, const int count, const landmass *landmass = nullptr, const site *settlement = nullptr, const Vec2i &pos = Vec2i(-1, -1), const int z = 0);
/// Add upgrade-to request to resource manager
extern void AiAddUpgradeToRequest(const unit_type &type);
/// Add research request to resource manager
extern void AiAddResearchRequest(const CUpgrade *upgrade);
/// Periodic called resource manager handler
extern void AiResourceManager();
/// Ask the ai to explore around pos
extern void AiExplore(const Vec2i &pos, const tile_flag exploreMask);
/// Make two unittypes be considered equals
extern void AiNewUnitTypeEquiv(const unit_type &a, const unit_type &b);
/// Remove any equivalence between unittypes
extern void AiResetUnitTypeEquiv();
/// Finds all equivalents units to a given one
extern int AiFindUnitTypeEquiv(const unit_type &type, int *result);
/// Finds all available equivalents units to a given one, in the preferred order
extern int AiFindAvailableUnitTypeEquiv(const unit_type &type, int *result);
extern bool AiRequestedTypeAllowed(const CPlayer &player, const unit_type &type, const bool allow_can_build_builder = false, const bool include_upgrade = false);
extern bool AiRequestedUpgradeAllowed(const CPlayer &player, const CUpgrade *upgrade, const bool allow_can_build_researcher = false);
extern int AiGetBuildRequestsCount(const PlayerAi &pai, int (&counter)[UnitTypeMax]);

extern void AiNewDepotRequest(CUnit &worker);
extern CUnit *AiGetSuitableDepot(const CUnit &worker, const CUnit &oldDepot, CUnit **resUnit);

//Wyrmgus start
extern void AiCheckDockConstruction();
extern void AiCheckUpgrades();
extern void AiCheckBuildings();
//Wyrmgus end

//
// Buildings
//
/// Find nice building place
extern bool AiFindBuildingPlace(const CUnit &worker, const wyrmgus::unit_type &type, const Vec2i &nearPos, Vec2i *resultPos, bool ignore_exploration, int z, const landmass *landmass = nullptr, const wyrmgus::site *settlement = nullptr);

//
// Forces
//
/// Cleanup units in force
extern void AiRemoveDeadUnitsInForces();
/// Assign a new unit to a force
extern bool AiAssignToForce(CUnit &unit);
/// Assign a free units to a force
extern void AiAssignFreeUnitsToForce(int force = -1);
/// Attack with force at position
//Wyrmgus start
//extern void AiAttackWithForceAt(unsigned int force, int x, int y);
extern void AiAttackWithForceAt(unsigned int force, int x, int y, int z);
//Wyrmgus end
/// Attack with force
extern void AiAttackWithForce(unsigned int force);
/// Attack with forces in array
extern void AiAttackWithForces(const std::array<int, AI_MAX_FORCE_INTERNAL + 1> &forces);

/// Periodically called force manager handlers
extern void AiForceManager();
extern void AiForceManagerEachHalfMinute();
extern void AiForceManagerEachMinute();

//
// Plans
//
/// Find a wall to attack
extern int AiFindWall(AiForce *force);
/// Plan the an attack
/// Send explorers around the map
extern void AiSendExplorers();
//Wyrmgus start
/// Get the quantity of units belonging to a particular type, possibly including requests
extern int AiGetUnitTypeCount(const PlayerAi &pai, const wyrmgus::unit_type *type, const landmass *landmass, const bool include_requests, const bool include_upgrades);
/// Get whether the AI has a particular upgrade, possibly including requests and currently under research upgrades
extern int AiGetUnitTypeRequestedCount(const PlayerAi &pai, const wyrmgus::unit_type *type, const landmass *landmass = nullptr, const wyrmgus::site *settlement = nullptr);
/// Get whether the AI has a particular upgrade, possibly including requests and currently under research upgrades
extern bool AiHasUpgrade(const PlayerAi &pai, const CUpgrade *upgrade, bool include_requests);
//Wyrmgus end
/// Check if the costs for an unit-type are available for the AI
extern int AiCheckUnitTypeCosts(const wyrmgus::unit_type &type);
/// Enemy units in distance
extern int AiEnemyUnitsInDistance(const CPlayer &player, const CUnit *unit,
								  //Wyrmgus start
//								  const QPoint &pos, const unsigned range);
								  const QPoint &pos, const unsigned range, const int z);
								  //Wyrmgus end

//
// Magic
//
/// Check for magic
extern void AiCheckMagic();
