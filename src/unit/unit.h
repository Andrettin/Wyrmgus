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
/**@name unit.h - The unit header file. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#ifndef __UNIT_H__
#define __UNIT_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "player.h"
#include "unit/unit_type.h"
#include "vec2i.h"

#include <core/object.h>

#include <set>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CAnimation;
class CBuildRestrictionOnTop;
class CCharacter;
class CConstructionFrame;
class CFile;
class CLanguage;
class CMapLayer;
class COrder;
class COrder_Built;
class CPlayer;
class CSpell;
class CUnit;
class CUnitColors;
class CUnitPtr;
class CUnitStats;
class CUnitType;
class CUpgrade;
class CVariable;
class CViewport;
class CWord;
class ItemClass;
class ItemSlot;
class Missile;
class PathFinderData;
class PaletteImage;
class UniqueItem;
class UnitTypeVariation;
struct lua_State;

typedef COrder *COrderPtr;

/*
** Configuration of the small (unit) AI.
*/
#define PRIORITY_FACTOR   0x00080000
#define HEALTH_FACTOR     0x00000001
#define DISTANCE_FACTOR   0x00010000
#define INRANGE_FACTOR    0x00008000
#define INRANGE_BONUS     0x01000000
#define CANATTACK_BONUS   0x00080000
#define AIPRIORITY_BONUS  0x04000000

//the range in which experience is distributed
constexpr int ExperienceRange = 6;

/// Called whenever the selected unit was updated
extern void SelectedUnitChanged();

/// Returns the map distance between to unittype as locations
extern int MapDistanceBetweenTypes(const CUnitType &src, const Vec2i &pos1, int src_z, const CUnitType &dst, const Vec2i &pos2, int dst_z);
								   
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

constexpr int NextDirection = 32;	/// Next direction N->NE->E...

#define UnitNotSeen 0x7fffffff		/// Unit not seen, used by CUnit::SeenFrame

/// The big unit structure
class CUnit : public Object
{
	GDCLASS(CUnit, Object)
	
public:
	CUnit() { Init(); }

	void Init();

	COrder *CurrentOrder() const { return Orders[0]; }

	unsigned int CurrentAction() const;

	bool IsIdle() const;

	void ClearAction();

	/// Increase a unit's reference count
	void RefsIncrease();
	/// Decrease a unit's reference count
	void RefsDecrease();

	/// Initialize unit structure with default values
	void Init(const CUnitType &type);
	/// Assign unit to player
	void AssignToPlayer(CPlayer &player);

	/// Draw a single unit
	void Draw(const CViewport &vp) const;
	/// Place a unit on map
	//Wyrmgus start
//	void Place(const Vector2i &pos);
	void Place(const Vector2i &pos, const int z);
	//Wyrmgus end

	/// Move unit to tile(pos). (Do special stuff : vision, cachelist, pathfinding)
	//Wyrmgus start
//	void MoveToXY(const Vector2i &pos);
	void MoveToXY(const Vector2i &pos, const int z);
	//Wyrmgus end
	/// Add a unit inside a container. Only deal with list stuff.
	void AddInContainer(CUnit &host);
	//Wyrmgus start
	void UpdateContainerAttackRange();
	void UpdateXPRequired();
	std::vector<String> GetPotentialNames() const;
	void GenerateName();
	bool IsNameValid() const;
	void UpdatePersonalName(const bool update_settlement_name = true);
	void UpdateExtraName();
	void UpdateSettlement();
	void UpdateBuildingSettlementAssignment(CSite *old_settlement = nullptr); //update the settlement assignment of surrounding buildings for this town hall
	void XPChanged();
	//Wyrmgus end
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
	
	void SetType(const CUnitType *new_type);
	
	[[nodiscard]]
	const CUnitType *GetType() const
	{
		return this->Type;
	}
	
	void SetPlayer(CPlayer *player);

	[[nodiscard]]
	CPlayer *GetPlayer() const
	{
		return this->Player;
	}
	
	void SetCurrentResource(const unsigned char resource_index);
	
	unsigned char GetCurrentResource() const
	{
		return this->CurrentResource;
	}
	
	void SetResourcesHeld(const int quantity);
	void ChangeResourcesHeld(const int quantity);
	
	[[nodiscard]]
	int GetResourcesHeld() const
	{
		return this->ResourcesHeld;
	}
	
	void ReplaceOnTop(CUnit &replaced_unit);

	//Wyrmgus start
	void ChangeExperience(int amount, int around_range = 0); //around_range is the range around the unit that other units of the same player will receive experience
	void IncreaseLevel(int level_quantity, bool automatic_learning = true);
	void Retrain();
	void HealingItemAutoUse();
	void SetCharacter(const std::string &character_full_name, bool custom_hero = false);
	bool CheckTerrainForVariation(const UnitTypeVariation *variation) const;
	bool CheckSeasonForVariation(const UnitTypeVariation *variation) const;
	void ChooseVariation(const CUnitType *new_type = nullptr, int image_layer = -1);
	void SetVariation(const UnitTypeVariation *new_variation, const CUnitType *new_type = nullptr, int image_layer = -1);
	
	[[nodiscard]]
	const UnitTypeVariation *GetVariation() const
	{
		return this->Variation;
	}
	
	[[nodiscard]]
	const UnitTypeVariation *GetLayerVariation(const unsigned int image_layer) const;
	
	void UpdateButtonIcons();
	void ChooseButtonIcon(const int button_action);
	void EquipItem(CUnit *item, const bool affect_character = true);
	void DeequipItem(CUnit *item, const bool affect_character = true);
	void ReadWork(const CUpgrade *work, const bool affect_character = true);
	void ConsumeElixir(const CUpgrade *elixir, const bool affect_character = true);
	void ApplyAura(int aura_index);
	void ApplyAuraEffect(int aura_index);
	void SetPrefix(const CUpgrade *prefix);
	void SetSuffix(const CUpgrade *suffix);
	void SetSpell(const CSpell *spell);
	void SetWork(const CUpgrade *work);
	void SetElixir(const CUpgrade *elixir);
	void SetUnique(UniqueItem *unique);
	void Identify();
	void CheckIdentification();
	void CheckKnowledgeChange(int variable, int change);
	void UpdateItemName();
	void GenerateDrop();
	void GenerateSpecialProperties(const CUnit *dropper = nullptr, const CPlayer *dropper_player = nullptr, const bool allow_unique = true, const bool sold_item = false, const bool always_magic = false);
	void GeneratePrefix(const CUnit *dropper, const CPlayer *dropper_player);
	void GenerateSuffix(const CUnit *dropper, const CPlayer *dropper_player);
	void GenerateSpell(const CUnit *dropper, const CPlayer *dropper_player);
	void GenerateWork(const CUnit *dropper, const CPlayer *dropper_player);
	void GenerateUnique(const CUnit *dropper, const CPlayer *dropper_player);
	void UpdateSoldUnits();
	void SellUnit(CUnit *sold_unit, int player);
	bool CanRecruitHero(const CCharacter *character) const;
	void ProduceResource(const int resource);
	void SellResource(const int resource, const int player);
	void BuyResource(const int resource, const int player);
	void Scout();
	//Wyrmgus end
	
	bool RestoreOrder();
	bool CanStoreOrder(COrder *order);

	// Cowards and invisible units don't attack unless ordered.
	[[nodiscard]]
	bool IsAgressive() const
	{
		//Wyrmgus start
//		return (Type->BoolFlag[CANATTACK_INDEX].value && !Type->BoolFlag[COWARD_INDEX].value
		return (CanAttack() && !Type->BoolFlag[COWARD_INDEX].value && Variable[TERROR_INDEX].Value == 0
		//Wyrmgus end
				&& Variable[INVISIBLE_INDEX].Value == 0);
	}

	/// Returns true, if unit is directly seen by an allied unit.
	[[nodiscard]]
	bool IsVisible(const CPlayer &player) const;

	[[nodiscard]]
	inline bool IsInvisibile(const CPlayer &player) const
	{
		return (&player != Player && !!Variable[INVISIBLE_INDEX].Value
				&& !player.IsBothSharedVision(*Player));
	}

	/**
	**  Returns true if unit is alive.
	**  Another unit can interact only with alive map units.
	**
	**  @return        True if alive, false otherwise.
	*/
	[[nodiscard]]
	bool IsAlive() const;

	/**
	**  Returns true if unit is alive and on the map.
	**  Another unit can interact only with alive map units.
	**
	**  @return        True if alive, false otherwise.
	*/
	[[nodiscard]]
	inline bool IsAliveOnMap() const
	{
		return !this->Removed && this->IsAlive();
	}

	/**
	**  Returns true, if unit is visible as an action goal for a player on the map.
	**
	**  @param player  Player to check for.
	**
	**  @return        True if visible, false otherwise.
	*/
	[[nodiscard]]
	inline bool IsVisibleAsGoal(const CPlayer &player) const
	{
		// Invisibility
		if (IsInvisibile(player)) {
			return false;
		}
		// Don't attack revealers
		if (this->Type->BoolFlag[REVEALER_INDEX].value) {
			return false;
		}
		//Wyrmgus start
//		if ((player.Type == PlayerComputer && !this->Type->BoolFlag[PERMANENTCLOAK_INDEX].value)
		if (
		//Wyrmgus end
			//Wyrmgus start
//			|| IsVisible(player) || IsVisibleOnRadar(player)) {
			IsVisible(player) || IsVisibleOnRadar(player)) {
			//Wyrmgus end
			return IsAliveOnMap();
		} else {
			return Type->BoolFlag[VISIBLEUNDERFOG_INDEX].value
				   && (Seen.ByPlayer & (1 << player.GetIndex()))
				   && !(Seen.Destroyed & (1 << player.GetIndex()));
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
	[[nodiscard]]
	inline bool IsVisibleOnMap(const CPlayer &player) const
	{
		return IsAliveOnMap() && !IsInvisibile(player) && IsVisible(player);
	}

	/// Returns true if unit is visible on minimap. Only for ThisPlayer.
	[[nodiscard]]
	bool IsVisibleOnMinimap() const;

	/// Returns true if unit is visible under radar (By player, or by shared vision)
	[[nodiscard]]
	bool IsVisibleOnRadar(const CPlayer &pradar) const;

	/// Returns true if unit is visible in a viewport. Only for ThisPlayer.
	[[nodiscard]]
	bool IsVisibleInViewport(const CViewport &vp) const;

	[[nodiscard]]
	bool IsEnemy(const CPlayer &player) const;
	
	[[nodiscard]]
	bool IsEnemy(const CUnit &unit) const;
	
	[[nodiscard]]
	bool IsAllied(const CPlayer &player) const;
	
	[[nodiscard]]
	bool IsAllied(const CUnit &unit) const;
	
	[[nodiscard]]
	bool IsSharedVision(const CPlayer &player) const;
	
	[[nodiscard]]
	bool IsSharedVision(const CUnit &unit) const;
	
	[[nodiscard]]
	bool IsBothSharedVision(const CPlayer &player) const;
	
	[[nodiscard]]
	bool IsBothSharedVision(const CUnit &unit) const;
	
	[[nodiscard]]
	bool IsTeamed(const CPlayer &player) const;
	
	[[nodiscard]]
	bool IsTeamed(const CUnit &unit) const;

	[[nodiscard]]
	bool IsUnusable(bool ignore_built_state = false) const;

	[[nodiscard]]
	int MapDistanceTo(const CUnit &dst) const;

	[[nodiscard]]
	int MapDistanceTo(const Vector2i &pos, int z) const;

	/**
	**  Test if unit can move.
	**  For the moment only check for move animation.
	**
	**  @return true if unit can move.
	*/
	[[nodiscard]]
	bool CanMove() const { return Type->CanMove(); }

	[[nodiscard]]
	int GetDrawLevel() const;

	[[nodiscard]]
	bool IsAttackRanged(const CUnit *goal, const Vector2i &goalPos, const int z) const;

	[[nodiscard]]
	Vector2i GetMapPixelPosTopLeft() const;
	
	[[nodiscard]]
	Vector2i GetMapPixelPosCenter() const
	{
		return this->GetMapPixelPosTopLeft() + this->GetHalfTilePixelSize();
	}
	
	[[nodiscard]]
	const Vector2i &GetTilePos() const
	{
		return this->TilePos;
	}
	
	[[nodiscard]]
	const Vector2i &GetTileSize() const
	{
		return this->Type->GetTileSize();
	}
	
	[[nodiscard]]
	Vector2i GetHalfTileSize() const
	{
		return this->GetTileSize() / 2;
	}
	
	[[nodiscard]]
	Vector2i GetTilePixelSize() const;
	
	[[nodiscard]]
	Vector2i GetHalfTilePixelSize() const
	{
		return this->GetTilePixelSize() / 2;
	}
	
	[[nodiscard]]
	Vector2i GetTileCenterPos() const
	{
		return this->GetTilePos() + this->Type->GetTileCenterPosOffset();
	}
	
	[[nodiscard]]
	CMapLayer *GetMapLayer() const
	{
		return this->MapLayer;
	}
	
	[[nodiscard]]
	const PaletteImage *GetImage() const;
	
	void SetFrame(const int frame);
	
	void ChangeFrame(const int change)
	{
		this->SetFrame(this->GetFrame() + change);
	}
	
	[[nodiscard]]
	int GetFrame() const
	{
		return this->Frame;
	}
	
	/**
	**	@brief	Get the unit's frame as appropriate for being displayed by a Godot sprite
	**
	**	@return	The frame for the Godot sprite
	*/
	[[nodiscard]]
	int GetSpriteFrame() const
	{
		if (this->GetFrame() < 0) {
			//if the frame is negative, we need to make it absolute and remove one to get the correct frame for Godot
			return std::abs(this->GetFrame()) - 1;
		} else {
			return this->GetFrame();
		}
	}
	
	void SetPixelOffset(const Vector2i &offset);
	
	void SetPixelOffset(const int offset_x, const int offset_y)
	{
		this->SetPixelOffset(Vector2i(offset_x, offset_y));
	}
	
	void ChangePixelOffset(const Vector2i &change)
	{
		this->SetPixelOffset(this->GetPixelOffset() + change);
	}
	
	void ChangePixelOffset(const int change_x, const int change_y)
	{
		this->ChangePixelOffset(Vector2(change_x, change_y));
	}
	
	[[nodiscard]]
	const Vector2i &GetPixelOffset() const
	{
		return this->PixelOffset;
	}
	
	[[nodiscard]]
	CUnit *GetFirstContainer() const;
	
	void SetSelected(const bool selected);
	
	[[nodiscard]]
	bool IsSelected() const
	{
		return this->Selected;
	}

	void SetIndividualUpgrade(const CUpgrade *upgrade, int quantity);
	int GetIndividualUpgrade(const CUpgrade *upgrade) const;
	int GetAvailableLevelUpUpgrades(bool only_units = false) const;
	int GetVariableValue(const int index) const;
	int GetVariableMax(const int index) const;
	char GetVariableIncrease(const int index) const;
	bool IsVariableEnabled(const int index) const;
	int GetModifiedVariable(const int index, const int variable_type = 0) const;
	int GetReactionRange() const;
	int GetItemSlotQuantity(const ItemSlot *item_slot) const;
	const ItemClass *GetCurrentWeaponClass() const;
	int GetItemVariableChange(const CUnit *item, int variable_index, bool increase = false) const;
	int GetDisplayPlayer() const;
	int GetPrice() const;
	int GetUnitStock(const CUnitType *unit_type) const;
	void SetUnitStock(const CUnitType *unit_type, const int quantity);
	void ChangeUnitStock(const CUnitType *unit_type, const int quantity);
	int GetUnitStockReplenishmentTimer(const CUnitType *unit_type) const;
	void SetUnitStockReplenishmentTimer(const CUnitType *unit_type, const int quantity);
	void ChangeUnitStockReplenishmentTimer(const CUnitType *unit_type, const int quantity);
	int GetResourceStep(const int resource) const;
	
	int GetTotalInsideCount(const CPlayer *player = nullptr, const bool ignore_items = true, const bool ignore_saved_cargo = false, const CUnitType *type = nullptr) const;
	bool CanAttack(bool count_inside = true) const;
	bool IsInCombat() const;
	bool CanHarvest(const CUnit *dest, bool only_harvestable = true) const;
	bool CanReturnGoodsTo(const CUnit *dest, int resource = 0) const;
	bool CanCastSpell(const CSpell *spell, const bool ignore_mana_and_cooldown) const;
	bool CanCastAnySpell() const;
	bool CanAutoCastSpell(const CSpell *spell) const;
	bool IsItemEquipped(const CUnit *item) const;
	bool IsItemClassEquipped(const ItemClass *item_class) const;
	bool IsItemTypeEquipped(const CUnitType *item_type) const;
	bool IsUniqueItemEquipped(const UniqueItem *unique) const;
	bool CanEquipItem(CUnit *item) const;
	bool CanEquipItemClass(const ItemClass *item_class) const;
	bool CanUseItem(CUnit *item) const;
	bool IsItemSetComplete(const CUnit *item) const;
	bool EquippingItemCompletesSet(const CUnit *item) const;
	bool DeequippingItemBreaksSet(const CUnit *item) const;
	bool HasInventory() const;
	bool CanLearnAbility(const CUpgrade *ability, const bool pre = false) const;
	bool CanHireMercenary(CUnitType *type, int civilization_id = -1) const;
	bool CanEat(const CUnit &unit) const;
	bool LevelCheck(const int level) const;
	bool IsAbilityEmpowered(const CUpgrade *ability) const;
	bool IsSpellEmpowered(const CSpell *spell) const;
	bool UpgradeRemovesExistingUpgrade(const CUpgrade *upgrade) const;
	bool HasAdjacentRailForUnitType(const CUnitType *type) const;
	CAnimations *GetAnimations() const;
	const CConstruction *GetConstruction() const;
	const CIcon *GetIcon() const;
	const CIcon *GetButtonIcon(int button_action) const;
	MissileConfig GetMissile() const;
	CPlayerColorGraphic *GetLayerSprite(int image_layer) const;
	std::string GetName() const;
	std::string GetTypeName() const;
	std::string GetMessageName() const;
	const CLanguage *GetLanguage() const;
	
	[[nodiscard]]
	bool IsUnderConstruction() const
	{
		return this->UnderConstruction;
	}
	
	[[nodiscard]]
	bool IsDiurnal() const;
	
	[[nodiscard]]
	bool IsNocturnal() const;
	
	void IncreaseVariable(const int index);
	void HandleBuffsEachCycle();
	void HandleBuffsEachSecond();
	bool HandleBurnAndPoison();
	void HandleUnitAction();
	
	[[nodiscard]]
	IntColor GetSelectionColor() const;

public:
	class CUnitManagerData
	{
		friend class CUnitManager;
	public:
		int GetUnitId() const { return slot; }
	private:
		int slot = -1;		/// index in UnitManager::unitSlots
		int unitSlot = -1;	/// index in UnitManager::units
	};
public:
	// @note int is faster than shorts
	unsigned int     Refs;         /// Reference counter
	unsigned int     ReleaseCycle; /// When this unit could be recycled
	CUnitManagerData UnitManagerData;
	size_t PlayerSlot;  /// index in Player->Units

	int    InsideCount;   /// Number of units inside.
	int    BoardCount;    /// Number of units transported inside.
	CUnit *UnitInside;    /// Pointer to one of the units inside.
	CUnit *Container;     /// Pointer to the unit containing it (or 0)
	CUnit *NextContained; /// Next unit in the container.
	CUnit *PrevContained; /// Previous unit in the container.

	CUnit *NextWorker; //pointer to next assigned worker to "Goal" resource.
	struct {
		CUnit *Workers; /// pointer to first assigned worker to this resource.
		int Assigned; /// how many units are assigned to harvesting from the resource.
		int Active; /// how many units are harvesting from the resource.
	} Resource; /// Resource still

	//Wyrmgus start
	std::map<const ItemSlot *, std::vector<CUnit *>> EquippedItems;	/// Pointer to unit's equipped items, per slot
	std::vector<CUnit *> SoldUnits;						/// units available for sale at this unit
	//Wyrmgus end
	
private:
	Vector2i TilePos = Vec2i(-1, -1);	/// Map position X
public:
	Vector2i RallyPointPos = Vec2i(-1, -1);	/// used for storing the rally point position (where units trained by this unit will be sent to)
private:
	CMapLayer *MapLayer = nullptr;			/// in which map layer the unit is
public:
	CMapLayer *RallyPointMapLayer = nullptr;	/// in which map layer the unit's rally point is

	unsigned int Offset;	/// Map position as flat index offset (x + y * w)

private:
	const CUnitType *Type = nullptr;	/// Pointer to unit-type (peon,...)
	CPlayer *Player = nullptr;			/// Owner of this unit
public:
	const CUnitStats *Stats = nullptr;	/// Current unit stats
	int CurrentSightRange; /// Unit's Current Sight Range

	// Pathfinding stuff:
	PathFinderData *pathFinderData = nullptr;

	// DISPLAY:
private:
	int Frame = 0;			/// Image frame: <0 is mirrored
public:
	CUnitColors *Colors = nullptr;	/// Player colors
	//Wyrmgus start
	std::string Name;		/// Unit's personal/proper name (if any)
	std::string ExtraName;	/// Unit's "extra" name (i.e. a nickname)
	std::string FamilyName;	/// Unit's family name
	CCharacter *Character;	/// Pointer to the character represented by this unit
	CSite *Settlement = nullptr;	/// Settlement (for if the unit is a town hall or a building associated to a settlement)
	CUpgrade *Trait = nullptr;	/// Unit's trait
private:
	const UnitTypeVariation *Variation = nullptr;	/// Which of the variations of its unit type this unit has
public:
	int LayerVariation[MaxImageLayers];	/// Which layer variations this unit has
	const CUpgrade *Prefix = nullptr;	/// Item unit's prefix
	const CUpgrade *Suffix = nullptr;	/// Item unit's suffix
	const CSpell *Spell = nullptr;	/// Item unit's spell
	const CUpgrade *Work = nullptr;	/// Item unit's literary work
	const CUpgrade *Elixir = nullptr;	/// Item unit's elixir
	UniqueItem *Unique = nullptr;	/// Whether the item is unique
	bool Bound;			/// Whether the item is bound to its owner
	bool Identified;	/// Whether the item has been identified
	CUnit *ConnectingDestination = nullptr;	/// Which connector this unit connects to (if any)
	std::map<int, const CIcon *> ButtonIcons;	/// icons for button actions
	//Wyrmgus end
	std::map<int, int> IndividualUpgrades;      /// individual upgrades which the unit has (and how many of it the unit has)

private:
	Vector2i PixelOffset = Vector2i(0, 0);	/// image displacement to map position
public:
	unsigned char Direction; //: 8; /// angle (0-255) unit looking
	//Wyrmgus start
	unsigned char GivesResource;	/// The resource currently given by the unit
	//Wyrmgus end
private:
	unsigned char CurrentResource = 0;
public:
	int ResourcesHeld = 0;		/// resources held by the unit
	std::map<const CUnitType *, int> UnitStock; 						/// How many of each unit type this unit has stocked
	std::map<const CUnitType *, int> UnitStockReplenishmentTimers; 	/// Replenishment timer for each unit type stock

	unsigned char DamagedType;   /// index of damage type of unit which damaged this unit
	unsigned long Attacked;      /// gamecycle unit was last attacked
	unsigned Blink : 3;          /// let selection rectangle blink
	unsigned Moving : 1;         /// the unit is moving
	unsigned ReCast : 1;         /// recast again next cycle
	unsigned AutoRepair : 1;     /// true if unit tries to repair on still action.

	unsigned Burning : 1;        /// unit is burning
	unsigned Destroyed : 1;      /// unit is destroyed pending reference
	unsigned Removed : 1;        /// unit is removed (not on map)
private:
	bool Selected = false;       /// unit is selected

	bool UnderConstruction = false;	/// unit is in construction
public:
	unsigned Active : 1;         /// unit is active for AI
	unsigned Boarded : 1;        /// unit is on board a transporter.
	unsigned CacheLock : 1;      /// unit is on lock by unitcache operations.

	unsigned Summoned : 1;       /// unit is summoned using spells.
	unsigned Waiting : 1;        /// unit is waiting and playing its still animation
	unsigned MineLow : 1;        /// rhis mine got a notification about its resources being low
	
	unsigned TeamSelected;		/// unit is selected by a team member.
	CPlayer *RescuedFrom = nullptr;	/// The original owner of a rescued unit.
	/// null if the unit was not rescued.
	/* Seen stuff. */
	int VisCount[PlayerMax];     /// Unit visibility counts
	struct _seen_stuff_ {
		const CConstructionFrame *CFrame = nullptr;	/// Seen construction frame
		int Frame;							/// last seen frame/stage of buildings
		const CUnitType *Type = nullptr;	/// Pointer to last seen unit-type
		Vector2i TilePos = Vector2i(-1, -1);	/// Last unit->TilePos Seen
		signed char IX;						/// Seen X image displacement to map position
		signed char IY;						/// seen Y image displacement to map position
		bool UnderConstruction = false;		/// Unit seen construction
		unsigned State : 3;					/// Unit seen build/upgrade state
		unsigned Destroyed : PlayerMax;		/// Unit seen destroyed or not
		unsigned ByPlayer : PlayerMax;		/// Track unit seen by player
	} Seen;

	CVariable *Variable; /// array of User Defined variables.
	
	unsigned long TTL;  /// time to live

	unsigned int GroupId;       /// unit belongs to this group id
	unsigned int LastGroup;     /// unit belongs to this last group

	unsigned int Wait;          /// action counter
	int Threshold;              /// The counter while ai unit couldn't change target.
	
	unsigned char StepCount;	/// How many steps the unit has taken without stopping (maximum 10)

	struct _unit_anim_ {
		const CAnimation *Anim;      /// Anim
		const CAnimation *CurrAnim;  /// CurrAnim
		int Wait;                    /// Wait
		int Unbreakable;             /// Unbreakable
	} Anim, WaitBackup;


	std::vector<COrder *> Orders; /// orders to process
	COrder *SavedOrder = nullptr;	/// order to continue after current
	COrder *NewOrder = nullptr;		/// order for new trained units
	COrder *CriticalOrder = nullptr;	/// order to do as possible in breakable animation.

	std::set<const CSpell *> AutoCastSpells;			/// spells to auto cast
	std::map<const CSpell *, int> SpellCoolDownTimers;	/// how much time the unit needs to wait before a spell will be ready

	CUnit *Goal; /// Generic/Teleporter goal pointer
	
	friend class COrder_Build;
	friend class COrder_Follow;
	friend class COrder_PickUp;
	friend class COrder_Trade;
	friend class COrder_Use;
	friend class CPlayer;
	friend class Spell_Summon;
	friend class Spell_Teleport;
	friend int CclCreateBuildingAtRandomLocationNear(lua_State *l);
	friend int CclCreateUnitInTransporter(lua_State *l);
	friend int CclMoveUnit(lua_State *l);
	friend int CclShowMapLocation(lua_State *l);
	friend int CclUnit(lua_State *l);
	friend void Finish(COrder_Built &order, CUnit &unit);
	friend void UnitInXY(CUnit &unit, const Vec2i &pos, const int z);
	
protected:
	static void _bind_methods();
};

constexpr CUnit *NoUnitP = (CUnit *) 0;		/// return value: for no unit found

/**
**  Returns unit number (unique to this unit)
*/
#define UnitNumber(unit) ((unit).UnitManagerData.GetUnitId())

/**
**  User preference.
*/
class CPreference
{
public:
	bool ShowSightRange = false;	/// Show sight range.
	bool ShowReactionRange = false;	/// Show reaction range.
	bool ShowAttackRange = false;	/// Show attack range.
	bool ShowMessages = true;		/// Show messages.
	bool BigScreen = false;			/// If true, shows the big screen(without panels)
	bool PauseOnLeave = true;		/// If true, game pauses when cursor is gone
	bool AiExplores = true;			/// If true, AI sends explorers to search for resources (almost useless thing)
	bool GrayscaleIcons = false;	/// Use grayscaled icons for unavailable units, upgrades, etc
	bool IconsShift = false;		/// Shift icons slightly when you press on them
	bool StereoSound = true;		/// Enables/disables stereo sound effects	
	bool MineNotifications = false;	/// Show mine is running low/depleted messages
	bool DeselectInMine = false;	/// Deselect peasants in mines
	bool NoStatusLineTooltips = false;	/// Don't show messages on status line
	//Wyrmgus start
	bool SepiaForGrayscale = false;	/// Use a sepia filter for grayscale icons
	bool PlayerColorCircle = false;	/// Show a player color circle below each unit
	bool ShowPathlines = false;		/// Show order pathlines
	//Wyrmgus end

	int ShowOrders = 0;			/// How many second show orders of unit on map.
	int ShowNameDelay = 0;		/// How many cycles need to wait until unit's name popup will appear.
	int ShowNameTime = 0;		/// How many cycles need to show unit's name popup.
	int AutosaveMinutes = 5;	/// Autosave the game every X minutes; autosave is disabled if the value is 0
	//Wyrmgus start
	int HotkeySetup = 0;		/// Hotkey layout (0 = default, 1 = position-based, 2 = position-based (except commands))
	//Wyrmgus end
	std::string SF2Soundfont;	/// Path to SF2 soundfont
	//Wyrmgus start
	CGraphic *IconFrameG = nullptr;
	CGraphic *PressedIconFrameG = nullptr;
	CGraphic *CommandButtonFrameG = nullptr;
	CGraphic *BarFrameG = nullptr;
	CGraphic *InfoPanelFrameG = nullptr;
	CGraphic *ProgressBarG = nullptr;
	//Wyrmgus end
};

extern CPreference Preference;

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

// in unit_draw.c
/// @todo could be moved into the user interface ?
extern unsigned long ShowOrdersCount;   /// Show orders for some time
extern unsigned long ShowNameDelay;     /// Delay to show unit's name
extern unsigned long ShowNameTime;      /// Show unit's name for some time
extern bool EnableTrainingQueue;               /// Config: training queues enabled
extern bool EnableBuildingCapture;             /// Config: building capture enabled
extern bool RevealAttacker;                    /// Config: reveal attacker enabled
extern int ResourcesMultiBuildersMultiplier;   /// Config: spend resources for building with multiple workers
extern const CViewport *CurrentViewport; /// CurrentViewport
extern void DrawUnitSelection(const CViewport &vp, const CUnit &unit);
extern void (*DrawSelection)(IntColor, int, int, int, int);

extern unsigned int MaxSelectable;    /// How many units could be selected
extern std::vector<CUnit *> Selected; /// currently selected units

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

/// Mark the field with the FieldFlags.
void MarkUnitFieldFlags(const CUnit &unit);
/// Unmark the field with the FieldFlags.
void UnmarkUnitFieldFlags(const CUnit &unit);
/// Update unit->CurrentSightRange.
void UpdateUnitSightRange(CUnit &unit);
/// Create a new unit
extern CUnit *MakeUnit(const CUnitType &type, CPlayer *player);
/// Create a new unit and place on map
//Wyrmgus start
//extern CUnit *MakeUnitAndPlace(const Vector2i &pos, const CUnitType &type, CPlayer *player);
extern CUnit *MakeUnitAndPlace(const Vector2i &pos, const CUnitType &type, CPlayer *player, int z);
//Wyrmgus end
//Wyrmgus start
/// Create a new unit and place it on the map, and update its player accordingly
extern CUnit *CreateUnit(const Vec2i &pos, const CUnitType &type, CPlayer *player, int z, bool no_bordering_building = false);
extern CUnit *CreateResourceUnit(const Vec2i &pos, const CUnitType &type, int z, bool allow_unique = true);
//Wyrmgus end
/// Find the nearest position at which unit can be placed.
//Wyrmgus start
//void FindNearestDrop(const CUnitType &type, const Vec2i &goalPos, Vec2i &resPos, int heading);
void FindNearestDrop(const CUnitType &type, const Vec2i &goalPos, Vec2i &resPos, int heading, int z, bool no_bordering_building = false, bool ignore_construction_requirements = false);
//Wyrmgus end
/// Handle the loss of a unit (food,...)
extern void UnitLost(CUnit &unit);
/// Remove the Orders of a Unit
extern void UnitClearOrders(CUnit &unit);
/// @todo more docu
extern void UpdateForNewUnit(const CUnit &unit, int upgrade);
/// @todo more docu
extern void NearestOfUnit(const CUnit &unit, const Vector2i &pos, Vector2i *dpos);

/// Call when an Unit goes under fog.
extern void UnitGoesUnderFog(CUnit &unit, const CPlayer &player);
/// Call when an Unit goes out of fog.
extern void UnitGoesOutOfFog(CUnit &unit, const CPlayer &player);

/// Does a recount for VisCount
extern void UnitCountSeen(CUnit &unit);

/// Check for rescue each second
extern void RescueUnits();

/// Convert direction (dx,dy) to heading (0-255)
extern int DirectionToHeading(const Vector2i &dir);

///Correct directions for placed wall.
extern void CorrectWallDirections(CUnit &unit);
/// Correct the surrounding walls.
extern void CorrectWallNeighBours(CUnit &unit);

/// Update frame from heading
extern void UnitUpdateHeading(CUnit &unit);
/// Heading and frame from delta direction
extern void UnitHeadingFromDeltaXY(CUnit &unit, const Vector2i &delta);

/// @todo more docu
extern void DropOutOnSide(CUnit &unit, int heading, const CUnit *container);
/// @todo more docu
extern void DropOutNearest(CUnit &unit, const Vector2i &goalPos, const CUnit *container);

/// Drop out all units in the unit
extern void DropOutAll(const CUnit &unit);

/// Return the rule used to build this building.
//Wyrmgus start
//extern CBuildRestrictionOnTop *OnTopDetails(const CUnit &unit, const CUnitType *parent);
extern CBuildRestrictionOnTop *OnTopDetails(const CUnitType &type, const CUnitType *parent);
//Wyrmgus end
/// @todo more docu
//Wyrmgus start
//extern CUnit *CanBuildHere(const CUnit *unit, const CUnitType &type, const Vector2i &pos);
extern CUnit *CanBuildHere(const CUnit *unit, const CUnitType &type, const Vector2i &pos, int z, bool no_bordering_building = false);
//Wyrmgus end
/// @todo more docu
//Wyrmgus start
//extern bool CanBuildOn(const Vector2i &pos, int mask);
extern bool CanBuildOn(const Vector2i &pos, int mask, int z);
//Wyrmgus end
/// FIXME: more docu
//Wyrmgus start
//extern CUnit *CanBuildUnitType(const CUnit *unit, const CUnitType &type, const Vector2i &pos, int real);
extern CUnit *CanBuildUnitType(const CUnit *unit, const CUnitType &type, const Vector2i &pos, int real, bool ignore_exploration, int z);
//Wyrmgus end
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
extern void HitUnit_NormalHitSpecialDamageEffects(CUnit &attacker, CUnit &target);
extern void HitUnit_SpecialDamageEffect(CUnit &target, int dmg_var);
extern void HitUnit_RunAway(CUnit &target, const CUnit &attacker);
//Wyrmgus end

/// Calculate the distance from current view point to coordinate
extern int ViewPointDistance(const Vec2i &pos);
/// Calculate the distance from current view point to unit
extern int ViewPointDistanceToUnit(const CUnit &dest);

/// Can this unit-type attack the other (destination)
extern int CanTarget(const CUnitType &type, const CUnitType &dest);
/// Can transporter transport the other unit
extern int CanTransport(const CUnit &transporter, const CUnit &unit);
//Wyrmgus start
/// Can the unit pick up the other unit
extern bool CanPickUp(const CUnit &picker, const CUnit &unit);
//Wyrmgus end

/// Generate a unit reference, a printable unique string for unit
extern std::string UnitReference(const CUnit &unit);
/// Generate a unit reference, a printable unique string for unit
extern std::string UnitReference(const CUnitPtr &unit);

/// save unit-structure
extern void SaveUnit(const CUnit &unit, CFile &file);

/// Initialize unit module
extern void InitUnits();
/// Clean unit module
extern void CleanUnits();

// in unit_draw.c
//--------------------
/// Draw nothing around unit
extern void DrawSelectionNone(IntColor, int, int, int, int);
/// Draw circle around unit
extern void DrawSelectionCircle(IntColor, int, int, int, int);
/// Draw circle filled with alpha around unit
extern void DrawSelectionCircleWithTrans(IntColor, int, int, int, int);
/// Draw rectangle around unit
extern void DrawSelectionRectangle(IntColor, int, int, int, int);
/// Draw rectangle filled with alpha around unit
extern void DrawSelectionRectangleWithTrans(IntColor, int, int, int, int);
/// Draw corners around unit
extern void DrawSelectionCorners(IntColor, int, int, int, int);

/// Register CCL decorations features
extern void DecorationCclRegister();
/// Get the amount of decorations
extern int GetDecorationsCount();
/// Load the decorations (health,mana) of units
extern void LoadDecorations();
/// Clean the decorations (health,mana) of units
extern void CleanDecorations();

/// Draw unit's shadow
//Wyrmgus start
//extern void DrawShadow(const CUnitType &type, int frame, const PixelPos &screenPos);
extern void DrawShadow(const CUnitType &type, CGraphic *sprite, int frame, const PixelPos &screenPos);
//Wyrmgus end
//Wyrmgus start
/// Draw unit's overlay
extern void DrawPlayerColorOverlay(const CUnitType &type, CPlayerColorGraphic *sprite, int player, int frame, const PixelPos &screenPos);
extern void DrawOverlay(const CUnitType &type, CGraphic *sprite, int player, int frame, const PixelPos &screenPos);
//Wyrmgus end
/// Draw all units visible on map in viewport
extern int FindAndSortUnits(const CViewport &vp, std::vector<CUnit *> &table);

/// Show a unit's orders.
extern void ShowOrder(const CUnit &unit);

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
extern bool UnitCanBeSelectedWith(CUnit &first_unit, CUnit &second_unit);
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
extern int SelectGroup(int group_number, GroupSelectionMode mode = SELECTABLE_BY_RECTANGLE_ONLY);
/// Add the units from the same group as the one in parameter
extern int AddGroupFromUnitToSelection(CUnit &unit);
/// Select the units from the same group as the one in parameter
extern int SelectGroupFromUnit(CUnit &unit);
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

#endif
