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
//      (c) Copyright 2000-2019 by Lutz Sammer, Antonis Chaniotis and Andrettin
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

#ifndef __AI_LOCAL_H__
#define __AI_LOCAL_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "unit/unit_cache.h"
#include "upgrade/upgrade_structs.h" // MaxCost

#include <core/math/vector2.h>

#include <vector>

#ifdef __MORPHOS__
#undef Wait
#endif

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CUnit;
class CUnitType;
class CUpgrade;
class CPlayer;
class CSite;

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
	unsigned int Count = 0;		/// elements in table
	CUnitType *Type = nullptr;	/// the type
	//Wyrmgus start
	int Landmass = 0;			/// in which landmass the unit should be created
	//Wyrmgus end
};

/**
**  Ai unit-type in a force.
*/
class AiUnitType
{
public:
	unsigned int Want = 0;		/// number of this unit-type wanted
	const CUnitType *Type = nullptr;	/// unit-type self
};

/**
**  Roles for forces
*/
enum AiForceRole {
	AiForceRoleDefault = 0, /// So default is attacking
	AiForceRoleAttack = 0, /// Force should attack
	AiForceRoleDefend      /// Force should defend
};

enum AiForceAttackingState {
	AiForceAttackingState_Free = -1,
	AiForceAttackingState_Waiting = 0,
	AiForceAttackingState_Boarding,
	AiForceAttackingState_GoingToRallyPoint,
	AiForceAttackingState_AttackingWithTransporter,
	AiForceAttackingState_Attacking,
	//Wyrmgus start
	AiForceAttackingState_WaitingForTransporters,
	//Wyrmgus end
};

#define AI_WAIT_ON_RALLY_POINT 60          /// Max seconds AI units will wait on rally point

/**
**  Define an AI force.
**
**  A force is a group of units belonging together.
*/
class AiForce
{
	friend class AiForceManager;
public:
	void Remove(CUnit &unit)
	{
		if (Units.Remove(&unit)) {
			InternalRemoveUnit(&unit);
		}
	}

	/**
	**  Reset the force. But don't change its role and its demand.
	*/
	void Reset(bool types = false)
	{
		FormerForce = -1;
		Completed = false;
		Defending = false;
		Attacking = false;
		WaitOnRallyPoint = AI_WAIT_ON_RALLY_POINT;
		if (types) {
			UnitTypes.clear();
			State = AiForceAttackingState_Free;
		} else {
			State = AiForceAttackingState_Waiting;
		}
		Units.for_each(InternalRemoveUnit);
		Units.clear();
		HomePos.x = HomePos.y = GoalPos.x = GoalPos.y = -1;
		//Wyrmgus start
		HomeMapLayer = 0;
		GoalMapLayer = 0;
		//Wyrmgus end
	}
	inline size_t Size() const { return Units.size(); }

	inline bool IsAttacking() const { return (!Defending && Attacking); }

	int GetForceType() const;
	bool IsNaval() const;
	bool IsAirForce() const;
	bool IsHeroOnlyForce() const;
	
	//Wyrmgus start
//	void Attack(const Vector2i &pos);
	void Attack(const Vector2i &pos, int z);
	//Wyrmgus end
	void RemoveDeadUnit();
	int PlanAttack();

	void ReturnToHome();
	//Wyrmgus start
//	bool NewRallyPoint(const Vector2i &startPos, Vector2i *resultPos);
	bool NewRallyPoint(const Vector2i &startPos, Vector2i *resultPos, const int z);
	bool CheckTransporters(const Vector2i &pos, int z);
	//Wyrmgus end
	void Insert(CUnit &unit);

private:
	void CountTypes(unsigned int *counter, const size_t len);
	bool IsBelongsTo(const CUnitType &type);

	void Update();

	static void InternalRemoveUnit(CUnit *unit);

public:
	bool Completed = false;	/// Flag saying force is complete build
	bool Defending = false;	/// Flag saying force is defending
	bool Attacking = false;	/// Flag saying force is attacking
	AiForceRole Role = AiForceRoleDefault;	/// Role of the force

	std::vector<AiUnitType> UnitTypes; /// Count and types of unit-type
	CUnitCache Units;  /// Units in the force

	// If attacking
	int FormerForce = -1;	/// Original force number
	AiForceAttackingState State = AiForceAttackingState_Free;	/// Attack state
	Vector2i GoalPos = Vector2i(-1, -1);	/// Attack point tile map position
	Vector2i HomePos = Vector2i(-1, -1);	/// Return after attack tile map position
	//Wyrmgus start
	int GoalMapLayer = 0;
	int HomeMapLayer = 0;
	//Wyrmgus end
	int WaitOnRallyPoint = AI_WAIT_ON_RALLY_POINT;	/// Counter for waiting on rally point
};

// forces
#define AI_MAX_FORCES 50							/// How many forces are supported
#define AI_MAX_FORCE_INTERNAL (AI_MAX_FORCES / 2)	/// The forces after AI_MAX_FORCE_INTERNAL are for internal use
#define AI_MAX_COMPLETED_FORCES (AI_MAX_FORCE_INTERNAL - 1)	/// How many completed forces the AI should have at maximum
#define AI_MAX_COMPLETED_FORCE_POP 90				/// How much population the AI completed forces should have at maximum (the AI will produce a new force if it is below this limit, even if that will make it go above it)

/**
**  AI force manager.
**
**  A Forces container for the force manager to handle
*/
class AiForceManager
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
	void RemoveDeadUnit();
	//Wyrmgus start
//	bool Assign(CUnit &unit, int force = -1);
	bool Assign(CUnit &unit, int force = -1, bool hero = false);
	//Wyrmgus end
	void Update();
	void UpdatePerHalfMinute();
	void UpdatePerMinute();
	unsigned int FindFreeForce(AiForceRole role = AiForceRoleDefault, int begin = 0, bool allow_hero_only_force = false);
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
	unsigned int Want = 0;		/// requested number
	unsigned int Made = 0;		/// built number
	const CUnitType *Type = nullptr;	/// unit-type
	unsigned long Wait = 0;		/// wait until this cycle
	Vector2i Pos = Vector2i(-1, -1);	/// build near pos on map
	//Wyrmgus start
	int MapLayer = 0;
	int Landmass = 0;
	const CSite *Settlement = nullptr;
	//Wyrmgus end
};

/**
**  AI exploration request
*/
class AiExplorationRequest
{
public:
	AiExplorationRequest(const Vector2i &pos, int mask) : pos(pos), Mask(mask) {}

public:
	Vector2i pos;          /// pos on map
	int Mask;           /// mask ( ex: MapFieldLandUnit )
};

/**
**  AI variables.
*/
class PlayerAi
{
public:
	PlayerAi()
	{
		memset(Reserve, 0, sizeof(Reserve));
		memset(Used, 0, sizeof(Used));
		memset(Needed, 0, sizeof(Needed));
		memset(Collect, 0, sizeof(Collect));
	}

public:
	CPlayer *Player = nullptr;		/// Engine player structure
	CAiType *AiType = nullptr;		/// AI type of this player AI
	// controller
	std::string Script;				/// Script executed
	unsigned long SleepCycles = 0;	/// Cycles to sleep

	AiForceManager Force;			/// Forces controlled by AI

	// resource manager
	int Reserve[MaxCosts];			/// Resources to keep in reserve
	int Used[MaxCosts];				/// Used resources
	int Needed[MaxCosts];			/// Needed resources
	int Collect[MaxCosts];			/// Collect % of resources
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
	std::vector<CUnitType *> UpgradeToRequests;		/// Upgrade to unit-type requested and priority list
	std::vector<const CUpgrade *> ResearchRequests;	/// Upgrades requested and priority list
	std::vector<AiBuildQueue> UnitTypeBuilt;		/// What the resource manager should build
	int LastRepairBuilding = 0;						/// Last building checked for repair in this turn
	//Wyrmgus start
	int LastPathwayConstructionBuilding = 0;		/// Last building checked for pathway construction in this turn
	std::vector<CUnit *> Scouts;				/// AI scouting units
	std::map<int, std::vector<CUnit *>> Transporters;	/// AI transporters, mapped to the sea (water "landmass") they belong to
	//Wyrmgus end
};

/**
**  AI Helper.
**
**  Contains information needed for the AI. If the AI needs an unit or
**  building or upgrade or spell, it could lookup in this tables to find
**  where it could be trained, built or researched.
*/
class AiHelper
{
public:
	/**
	** The index is the unit that should be trained, giving a table of all
	** units/buildings which could train this unit.
	*/
	std::vector<std::vector<CUnitType *> > Train;
	/**
	** The index is the unit that should be build, giving a table of all
	** units/buildings which could build this unit.
	*/
	std::vector<std::vector<CUnitType *> > Build;
	/**
	** The index is the upgrade that should be made, giving a table of all
	** units/buildings which could do the upgrade.
	*/
	std::vector<std::vector<CUnitType *> > Upgrade;
	/**
	** The index is the research that should be made, giving a table of all
	** units/buildings which could research this upgrade.
	*/
	std::vector<std::vector<CUnitType *> > Research;
	/**
	** The index is the unit that should be repaired, giving a table of all
	** units/buildings which could repair this unit.
	*/
	std::vector<std::vector<CUnitType *> > Repair;
	/**
	** The index is the unit-limit that should be solved, giving a table of all
	** units/buildings which could reduce this unit-limit.
	*/
	std::vector<std::vector<CUnitType *> > UnitLimit;
	/**
	** The index is the unit that should be made, giving a table of all
	** units/buildings which are equivalent.
	*/
	std::vector<std::vector<CUnitType *> > Equiv;

	/**
	** The index is the resource id - 1 (we can't mine TIME), giving a table of all
	** units/buildings/mines which can harvest this resource.
	*/
	std::vector<std::vector<CUnitType *> > Mines;

	/**
	** The index is the resource id - 1 (we can't store TIME), giving a table of all
	** units/buildings/mines which can store this resource.
	*/
	std::vector<std::vector<CUnitType *> > Depots;
	
	//Wyrmgus start
	/**
	** The index is the resource id - 1 (we can't trade TIME), giving a table of all
	** units/buildings/mines which can sell this resource. In index 0 (respective to CopperCost) we store all units with the "Market" tag instead.
	*/
	std::vector<std::vector<CUnitType *> > SellMarkets;
	
	/**
	** The index is the resource id - 1 (we can't trade TIME), giving a table of all
	** units/buildings/mines which can buy this resource.
	*/
	std::vector<std::vector<CUnitType *> > BuyMarkets;

	/**
	** The index is the unit type id, giving a table of all
	** resources it can choose to produce.
	*/
	std::vector<std::vector<int> > ProducedResources;

	/**
	** The index is the unit type, giving a table of all
	** upgrades that it can research.
	*/
	std::vector<std::vector<const CUpgrade *> > ResearchedUpgrades;

	/**
	** The index is the unit that should perform an upgrade, giving a table of all
	** possible (non-experience) upgrades for it.
	*/
	std::vector<std::vector<CUnitType *> > UpgradesTo;
	
	/**
	** The index is the unit that should acquire an experience upgrade, giving a table of all
	** possible upgrades for it.
	*/
	std::vector<std::vector<CUnitType *> > ExperienceUpgrades;
	
	/**
	** The index is the unit type that should acquire a learnable ability, giving a table of all
	** abilities that it can learn.
	*/
	std::vector<std::vector<const CUpgrade *> > LearnableAbilities;
	
	/**
	** The sole index is 0, giving a table of all
	** naval transporter units.
	*/
	std::vector<std::vector<CUnitType *> > NavalTransporters;
	//Wyrmgus end
};

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern std::vector<CAiType *> AiTypes;   /// List of all AI types
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
//Wyrmgus start
//extern void AiAddUnitTypeRequest(const CUnitType &type, int count);
extern void AiAddUnitTypeRequest(const CUnitType &type, const int count, const int landmass = 0, CSite *settlement = nullptr, const Vector2i pos = Vector2i(-1, -1), const int z = 0);
//Wyrmgus end
/// Add upgrade-to request to resource manager
extern void AiAddUpgradeToRequest(const CUnitType &type);
/// Add research request to resource manager
extern void AiAddResearchRequest(const CUpgrade *upgrade);
/// Periodic called resource manager handler
extern void AiResourceManager();
/// Ask the ai to explore around pos
extern void AiExplore(const Vector2i &pos, int exploreMask);
/// Make two unittypes be considered equals
extern void AiNewUnitTypeEquiv(const CUnitType &a, const CUnitType &b);
/// Remove any equivalence between unittypes
extern void AiResetUnitTypeEquiv();
/// Finds all equivalents units to a given one
extern int AiFindUnitTypeEquiv(const CUnitType &type, int *result);
/// Finds all available equivalents units to a given one, in the preferred order
extern int AiFindAvailableUnitTypeEquiv(const CUnitType &type, int *result);
extern bool AiRequestedTypeAllowed(const CPlayer &player, const CUnitType &type, bool allow_can_build_builder = false, bool include_upgrade = false);
extern int AiGetBuildRequestsCount(const PlayerAi &pai, int (&counter)[UnitTypeMax]);

extern void AiNewDepotRequest(CUnit &worker);
extern CUnit *AiGetSuitableDepot(const CUnit &worker, const CUnit &oldDepot, CUnit **resUnit);

//Wyrmgus start
extern void AiTransportCapacityRequest(int capacity_needed, int landmass);
extern void AiCheckSettlementConstruction();
extern void AiCheckDockConstruction();
extern void AiCheckUpgrades();
extern void AiCheckBuildings();
//Wyrmgus end

//
// Buildings
//
/// Find nice building place
//Wyrmgus start
//extern bool AiFindBuildingPlace(const CUnit &worker, const CUnitType &type, const Vector2i &nearPos, Vector2i *resultPos);
extern bool AiFindBuildingPlace(const CUnit &worker, const CUnitType &type, const Vector2i &nearPos, Vector2i *resultPos, bool ignore_exploration, int z, int landmass = 0, const CSite *settlement = nullptr);
//Wyrmgus end

//
// Forces
//
/// Cleanup units in force
extern void AiRemoveDeadUnitInForces();
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
extern void AiAttackWithForces(int *forces);

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
/// Assign free transporters according to their water zone (water "landmass")
extern void AiCheckTransporters();
/// Get the current transport capacity of the AI for a given water zone
extern int AiGetTransportCapacity(int water_landmass);
/// Get the current requested transport capacity of the AI for a given water zone
extern int AiGetRequestedTransportCapacity(int water_landmass);
/// Get the quantity of units belonging to a particular type, possibly including requests
extern int AiGetUnitTypeCount(const PlayerAi &pai, const CUnitType *type, const int landmass, const bool include_requests, const bool include_upgrades);
/// Get whether the AI has a particular upgrade, possibly including requests and currently under research upgrades
extern int AiGetUnitTypeRequestedCount(const PlayerAi &pai, const CUnitType *type, const int landmass = 0, const CSite *settlement = nullptr);
/// Get whether the AI has a particular upgrade, possibly including requests and currently under research upgrades
extern bool AiHasUpgrade(const PlayerAi &pai, const CUpgrade *upgrade, bool include_requests);
//Wyrmgus end
/// Check if the costs for an unit-type are available for the AI
extern int AiCheckUnitTypeCosts(const CUnitType &type);
/// Enemy units in distance
extern int AiEnemyUnitsInDistance(const CPlayer &player, const CUnitType *type,
								  //Wyrmgus start
//								  const Vector2i &pos, unsigned range);
								  const Vector2i &pos, unsigned range, int z);
								  //Wyrmgus end

//
// Magic
//
/// Check for magic
extern void AiCheckMagic();

#endif
