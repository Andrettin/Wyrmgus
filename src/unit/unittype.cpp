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
/**@name unittype.cpp - The unit types. */
//
//      (c) Copyright 1998-2006 by Lutz Sammer and Jimmy Salmon
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "unittype.h"

#include "animation.h"
#include "animation/animation_exactframe.h"
#include "animation/animation_frame.h"
#include "construct.h"
#include "iolib.h"
#include "luacallback.h"
#include "map.h"
#include "missile.h"
#include "player.h"
#include "script.h"
#include "sound.h"
#include "spells.h"
#include "tileset.h"
#include "translate.h"
#include "ui.h"
#include "unitsound.h"
#include "util.h"
#include "video.h"
//Wyrmgus start
#include "upgrade.h"
//Wyrmgus end

#include <ctype.h>

#include <string>
#include <map>
#include <cstring>

/*----------------------------------------------------------------------------
-- Documentation
----------------------------------------------------------------------------*/

/**
**  @class CUnitType unittype.h
**
**  \#include "unittype.h"
**
**  This class contains the information that is shared between all
**  units of the same type and determins if a unit is a building,
**  a person, ...
**
**  The unit-type class members:
**
**  CUnitType::Ident
**
**    Unique identifier of the unit-type, used to reference it in
**    config files and during startup. As convention they start with
**    "unit-" fe. "unit-farm".
**  @note Don't use this member in game, use instead the pointer
**  to this structure. See UnitTypeByIdent().
**
**  CUnitType::Name
**
**    Pretty name shown by the engine. The name should be shorter
**    than 17 characters and no word can be longer than 8 characters.
**
**  CUnitType::File
**
**    Path file name of the sprite file.
**
**  CUnitType::ShadowFile
**
**    Path file name of shadow sprite file.
**
**  CUnitType::DrawLevel
**
**    The Level/Order to draw this type of unit in. 0-255 usually.
**
**  CUnitType::Width CUnitType::Height
**
**    Size of a sprite frame in pixels. All frames of a sprite have
**    the same size. Also all sprites (tilesets) must have the same
**    size.
**
**  CUnitType::ShadowWidth CUnitType::ShadowHeight
**
**    Size of a shadow sprite frame in pixels. All frames of a sprite
**    have the same size. Also all sprites (tilesets) must have the
**    same size.
**
**  CUnitType::ShadowOffsetX CUnitType::ShadowOffsetY
**
**    Vertical offset to draw the shadow in pixels.
**
**  CUnitType::Animations
**
**    Animation scripts for the different actions. Currently the
**    animations still, move, attack and die are supported.
**  @see CAnimations
**  @see CAnimation
**
**  CUnitType::Icon
**
**    Icon to display for this unit-type. Contains configuration and
**    run time variable.
**  @note This icon can be used for training, but isn't used.
**
**  CUnitType::Missile
**
**    Configuration and run time variable of the missile weapon.
**  @note It is planned to support more than one weapons.
**  And the sound of the missile should be used as fire sound.
**
**  CUnitType::Explosion
**
**    Configuration and run time variable of the missile explosion.
**    This is the explosion that happens if unit is set to
**    ExplodeWhenKilled
**
**  CUnitType::CorpseName
**
**    Corpse unit-type name, should only be used during setup.
**
**  CUnitType::CorpseType
**
**    Corpse unit-type pointer, only this should be used during run
**    time. Many unit-types can share the same corpse.
**
**
**  @todo continue this documentation
**
**  CUnitType::Construction
**
**    What is shown in construction phase.
**
**  CUnitType::SightRange
**
**    Sight range
**
**  CUnitType::_HitPoints
**
**    Maximum hit points
**
**
**  CUnitType::_Costs[::MaxCosts]
**
**    How many resources needed
**
**  CUnitType::RepairHP
**
**    The HP given to a unit each cycle it's repaired.
**    If zero, unit cannot be repaired
**
**    CUnitType::RepairCosts[::MaxCosts]
**
**    Costs per repair cycle to fix a unit.
**
**  CUnitType::TileWidth
**
**    Tile size on map width
**
**  CUnitType::TileHeight
**
**    Tile size on map height
**
**  CUnitType::BoxWidth
**
**    Selected box size width
**
**  CUnitType::BoxHeight
**
**    Selected box size height
**
**  CUnitType::NumDirections
**
**    Number of directions the unit can face
**
**  CUnitType::MinAttackRange
**
**    Minimal attack range
**
**  CUnitType::ReactRangeComputer
**
**    Reacts on enemy for computer
**
**  CUnitType::ReactRangePerson
**
**    Reacts on enemy for person player
**
**  CUnitType::Priority
**
**    Priority value / AI Treatment
**
**  CUnitType::BurnPercent
**
**    The burning limit in percents. If the unit has lees than
**    this it will start to burn.
**
**  CUnitType::BurnDamageRate
**
**    Burn rate in HP per second
**
**  CUnitType::UnitType
**
**    Land / fly / naval
**
**  @note original only visual effect, we do more with this!
**
**  CUnitType::DecayRate
**
**    Decay rate in 1/6 seconds
**
**  CUnitType::AnnoyComputerFactor
**
**    How much this annoys the computer
**
**  @todo not used
**
**  CUnitType::MouseAction
**
**    Right click action
**
**  CUnitType::Points
**
**    How many points you get for unit. Used in the final score table.
**
**  CUnitType::CanTarget
**
**    Which units can it attack
**
**  Unit::Revealer
**
**    A special unit used to reveal the map for a time. This unit
**    has active sight even when Removed. It's used for Reveal map
**    type of spells.
**
**  CUnitType::LandUnit
**
**    Land animated
**
**  CUnitType::AirUnit
**
**    Air animated
**
**  CUnitType::SeaUnit
**
**    Sea animated
**
**  CUnitType::ExplodeWhenKilled
**
**    Death explosion animated
**
**  CUnitType::RandomMovementProbability
**
**    When the unit is idle this is the probability that it will
**    take a step in a random direction, in percents.
**
**  CUnitType::ClicksToExplode
**
**    If this is non-zero, then after that many clicks the unit will
**    commit suicide. Doesn't work with resource workers/resources.
**
**  CUnitType::Building
**
**    Unit is a Building
**
**  CUnitType::VisibleUnderFog
**
**    Unit is visible under fog of war.
**
**  CUnitType::PermanentCloak
**
**    Unit is permanently cloaked.
**
**  CUnitType::DetectCloak
**
**    These units can detect Cloaked units.
**
**  CUnitType::Coward
**
**    Unit is a coward, and acts defensively. it will not attack
**    at will and auto-casters will not buff it(bloodlust).
**
**  CUnitType::Transporter
**
**    Can transport units
**
**  CUnitType::AttackFromTransporter
**
**    Units inside this transporter can attack with missiles.
**
**  CUnitType::MaxOnBoard
**
**    Maximum units on board (for transporters), and resources
**
**  CUnitType::StartingResources
**    Amount of Resources a unit has when It's Built
**
**  CUnitType::DamageType
**    Unit's missile damage type (used for extra death animations)
**
**  CUnitType::GivesResource
**
**    This equals to the resource Id of the resource given
**    or 0 (TimeCost) for other buildings.
**
**  CUnitType::CanHarvest
**
**    Resource can be harvested. It's false for things like
**    oil patches.
**  @todo crappy name.
**
**  CUnitType::Harvester
**
**    Unit is a resource worker. Faster than examining ResInfo
**
**  CUnitType::ResInfo[::MaxCosts]
**
**    Information about resource harvesting. If NULL, it can't
**    harvest it.
**
**  CUnitType::NeutralMinimapColorRGB
**
**    Says what color a unit will have when it's neutral and
**    is displayed on the minimap.
**
**  CUnitType::CanStore[::MaxCosts]
**
**    What resource types we can store here.
**
**  CUnitType::Vanishes
**
**    Corpes & destroyed places
**
**  CUnitType::GroundAttack
**
**    Can do command ground attack
**
**  CUnitType::ShoreBuilding
**
**    Building must be build on coast
**
**  CUnitType::CanCastSpell
**
**    Unit is able to use spells
**
**  CUnitType::CanAttack
**
**    Unit is able to attack.
**
**  CUnitType::RepairRange
**
**    Unit can repair buildings. It will use the actack animation.
**    It will heal 4 points for every repair cycle, and cost 1 of
**    each resource, alternatively(1 cycle wood, 1 cycle gold)
**  @todo The above should be more configurable.
**    If units have a repair range, they can repair, and this is the
**    distance.
**
**  CUnitType::BuilderOutside
**
**    Only valid for buildings. When building the worker will
**    remain outside inside the building.
**
**  @warning Workers that can build buildings with the
**  @warning BuilderOutside flag must have the CanRepair flag.
**
**  CUnitType::BuilderLost
**
**    Only valid for buildings without the BuilderOutside flag.
**    The worker is lost when the building is completed.
**
**  CUnitType::SelectableByRectangle
**
**    Selectable with mouse rectangle
**
**    CUnitType::Teleporter
**
**    Can teleport other units.
**
**    CUnitType::ShieldPiercing
**
**    Can directly damage shield-protected units, without shield damaging.
**
**    CUnitType::SaveCargo
**
**    Unit unloads his passengers after death.
**
**  CUnitType::Sound
**
**    Sounds for events
**
**  CUnitType::Weapon
**
**    Current sound for weapon
**
**  @todo temporary solution
**
**  CUnitType::Supply
**
**    How much food does this unit supply.
**
**  CUnitType::Demand
**
**    Food demand
**
**  CUnitType::ImproveIncomes[::MaxCosts]
**
**    Gives the player an improved income.
**
**  CUnitType::FieldFlags
**
**    Flags that are set, if a unit enters a map field or cleared, if
**    a unit leaves a map field.
**
**  CUnitType::MovementMask
**
**    Movement mask, this value is and'ed to the map field flags, to
**    see if a unit can enter or placed on the map field.
**
**  CUnitType::Stats[::PlayerMax]
**
**    Unit status for each player
**  @todo This stats should? be moved into the player struct
**
**  CUnitType::Type
**
**    Type as number
**  @todo Should us a general name f.e. Slot here?
**
**  CUnitType::Sprite
**
**    Sprite images
**
**  CUnitType::ShadowSprite
**
**    Shadow sprite images
**
**  CUnitType::PlayerColorSprite
**
**    Sprite images of the player colors.  This image is drawn
**    over CUnitType::Sprite.  Used with OpenGL only.
**
**
*/
/**
**
**  @class ResourceInfo unittype.h
**
** \#include "unittype.h"
**
**    This class contains information about how a unit will harvest a resource.
**
**  ResourceInfo::FileWhenLoaded
**
**    The harvester's animation file will change when it's loaded.
**
**  ResourceInfo::FileWhenEmpty;
**
**    The harvester's animation file will change when it's empty.
**    The standard animation is used only when building/repairing.
**
**
**  ResourceInfo::HarvestFromOutside
**
**    Unit will harvest from the outside. The unit will use it's
**    Attack animation (seems it turned into a generic Action anim.)
**
**  ResourceInfo::ResourceId
**
**    The resource this is for. Mostly redundant.
**
**  ResourceInfo::FinalResource
**
**    The resource is converted to this at the depot. Useful for
**    a fisherman who harvests fish, but it all turns to food at the
**    depot.
**
**  ResourceInfo::FinalResourceConversionRate
**
**    The rate at which the resource is converted to the final resource at the depot. Useful for
**    silver mines returning a lower amount of gold.
**
**  ResourceInfo::WaitAtResource
**
**    Cycles the unit waits while inside a resource.
**
**  ResourceInfo::ResourceStep
**
**    The unit makes so-caled mining cycles. Each mining cycle
**    it does some sort of animation and gains ResourceStep
**    resources. You can stop after any number of steps.
**    when the quantity in the harvester reaches the maximum
**    (ResourceCapacity) it will return home. I this is 0 then
**    it's considered infinity, and ResourceCapacity will now
**    be the limit.
**
**  ResourceInfo::ResourceCapacity
**
**    Maximum amount of resources a harvester can carry. The
**    actual amount can be modified while unloading.
**
**  ResourceInfo::LoseResources
**
**    Special lossy behaviour for loaded harvesters. Harvesters
**    with loads other than 0 and ResourceCapacity will lose their
**    cargo on any new order.
**
**  ResourceInfo::WaitAtDepot
**
**    Cycles the unit waits while inside the depot to unload.
**
**  ResourceInfo::TerrainHarvester
**
**    The unit will harvest terrain. For now this only works
**    for wood. maybe it could be made to work for rocks, but
**    more than that requires a tileset rewrite.
**  @todo more configurable.
**
*/

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CUnitType *> UnitTypes;   /// unit-types definition
std::map<std::string, CUnitType *> UnitTypeMap;

/**
**  Next unit type are used hardcoded in the source.
**
**  @todo find a way to make it configurable!
*/
CUnitType *UnitTypeHumanWall;       /// Human wall
CUnitType *UnitTypeOrcWall;         /// Orc wall

/**
**  Default incomes for a new player.
*/
int DefaultIncomes[MaxCosts];

/**
**  Default action for the resources.
*/
std::string DefaultActions[MaxCosts];

/**
**  Default names for the resources.
*/
std::string DefaultResourceNames[MaxCosts];

/**
**  Default amounts for the resources.
*/
int DefaultResourceAmounts[MaxCosts];

/**
**  Default max amounts for the resources.
*/
int DefaultResourceMaxAmounts[MaxCosts];

//Wyrmgus start
int DefaultResourcePrices[MaxCosts];
int DefaultResourceLaborInputs[MaxCosts];
int DefaultResourceOutputs[MaxCosts];
int ResourceGrandStrategyBuildingVariations[MaxCosts];
int ResourceGrandStrategyBuildingTerrainSpecificGraphic[MaxCosts][WorldMapTerrainTypeMax];
//Wyrmgus end

/**
**  Default names for the resources.
*/
std::string ExtraDeathTypes[ANIMATIONS_DEATHTYPES];

//Wyrmgus start
std::string UnitTypeClasses[UnitTypeClassMax];
std::map<std::string, int> UnitTypeClassStringToIndex;
std::string UpgradeClasses[UnitTypeClassMax];
std::map<std::string, int> UpgradeClassStringToIndex;
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

int GetResourceIdByName(const char *resourceName)
{
	for (unsigned int res = 0; res < MaxCosts; ++res) {
		if (!strcmp(resourceName, DefaultResourceNames[res].c_str())) {
			return res;
		}
	}
	return -1;
}

int GetResourceIdByName(lua_State *l, const char *resourceName)
{
	const int res = GetResourceIdByName(resourceName);
	if (res == -1) {
		LuaError(l, "Resource not found: %s" _C_ resourceName);
	}
	return res;
}

//Wyrmgus start
std::string GetResourceNameById(int resource_id)
{
	if (resource_id > 0 && resource_id < MaxCosts) {
		return DefaultResourceNames[resource_id];
	} else {
		return "";
	}
}

std::string GetGovernmentTypeNameById(int government_type)
{
	if (government_type == GovernmentTypeMonarchy) {
		return "monarchy";
	} else if (government_type == GovernmentTypeRepublic) {
		return "republic";
	} else if (government_type == GovernmentTypeTheocracy) {
		return "theocracy";
	}

	return "";
}

int GetGovernmentTypeIdByName(std::string government_type)
{
	if (government_type == "monarchy") {
		return GovernmentTypeMonarchy;
	} else if (government_type == "republic") {
		return GovernmentTypeRepublic;
	} else if (government_type == "theocracy") {
		return GovernmentTypeTheocracy;
	}

	return -1;
}

int GetItemTypeIdByName(std::string item_type)
{
	if (item_type == "Sword") {
		return SwordItemType;
	} else if (item_type == "Axe") {
		return AxeItemType;
	} else if (item_type == "Mace") {
		return MaceItemType;
	} else if (item_type == "Spear") {
		return SpearItemType;
	} else if (item_type == "Bow") {
		return BowItemType;
	} else if (item_type == "Throwing Axe") {
		return ThrowingAxeItemType;
	} else if (item_type == "Javelin") {
		return JavelinItemType;
	} else if (item_type == "Shield") {
		return ShieldItemType;
	} else if (item_type == "Helmet") {
		return HelmetItemType;
	} else if (item_type == "Armor") {
		return ArmorItemType;
	} else if (item_type == "Shoes") {
		return ShoesItemType;
	} else if (item_type == "Amulet") {
		return AmuletItemType;
	} else if (item_type == "Ring") {
		return RingItemType;
	} else if (item_type == "Potion") {
		return PotionItemType;
	}

	return -1;
}
//Wyrmgus end

CUnitType::CUnitType() :
	Slot(0), Width(0), Height(0), OffsetX(0), OffsetY(0), DrawLevel(0),
	ShadowWidth(0), ShadowHeight(0), ShadowOffsetX(0), ShadowOffsetY(0),
	//Wyrmgus start
	TechnologyPointCost(0), TrainQuantity(0),
	//Wyrmgus end
	Animations(NULL), StillFrame(0),
	DeathExplosion(NULL), OnHit(NULL), OnEachCycle(NULL), OnEachSecond(NULL), OnInit(NULL),
	TeleportCost(0), TeleportEffectIn(NULL), TeleportEffectOut(NULL),
	CorpseType(NULL), Construction(NULL), RepairHP(0), TileWidth(0), TileHeight(0),
	BoxWidth(0), BoxHeight(0), BoxOffsetX(0), BoxOffsetY(0), NumDirections(0),
	//Wyrmgus start
//	MinAttackRange(0), ReactRangeComputer(0), ReactRangePerson(0),
	MinAttackRange(0),
	//Wyrmgus end
	BurnPercent(0), BurnDamageRate(0), RepairRange(0),
	CanCastSpell(NULL), AutoCastActive(NULL),
	AutoBuildRate(0), RandomMovementProbability(0), RandomMovementDistance(1), ClicksToExplode(0),
	MaxOnBoard(0), BoardSize(1), ButtonLevelForTransporter(0), StartingResources(0),
	UnitType(UnitTypeLand), DecayRate(0), AnnoyComputerFactor(0), AiAdjacentRange(-1),
	MouseAction(0), CanTarget(0),
	//Wyrmgus start
//	Flip(0), Revealer(0), LandUnit(0), AirUnit(0), SeaUnit(0),
	Flip(0), LandUnit(0), AirUnit(0), SeaUnit(0),
//	ExplodeWhenKilled(0), Building(0), VisibleUnderFog(0),
	ExplodeWhenKilled(0), Building(0),
//	PermanentCloak(0), DetectCloak(0),
//	Coward(0), AttackFromTransporter(0),
//	Vanishes(0), GroundAttack(0), ShoreBuilding(0), CanAttack(0),
	CanAttack(0),
	//Wyrmgus end
	//Wyrmgus start
//	BuilderOutside(0), BuilderLost(0), CanHarvest(0), Harvester(0),
	//Wyrmgus end
	//Wyrmgus start
//	Neutral(0), SelectableByRectangle(0), IsNotSelectable(0), Decoration(0),
	Neutral(0),
//	Indestructible(0), Teleporter(0), SaveCargo(0),
//	NonSolid(0), Wall(0), NoRandomPlacing(0), Organic(0),
	//Wyrmgus end
	//Wyrmgus start
//	GivesResource(0), Supply(0), Demand(0), PoisonDrain(0), FieldFlags(0), MovementMask(0),
	GivesResource(0), PoisonDrain(0), FieldFlags(0), MovementMask(0),
//	Sprite(NULL), ShadowSprite(NULL)
	Sprite(NULL), ShadowSprite(NULL), LightSprite(NULL), LeftArmSprite(NULL), RightArmSprite(NULL), HairSprite(NULL),
	ClothingSprite(NULL), ClothingLeftArmSprite(NULL), ClothingRightArmSprite(NULL), PantsSprite(NULL), ShoesSprite(NULL),
	WeaponSprite(NULL), ShieldSprite(NULL), HelmetSprite(NULL)
	//Wyrmgus end
{
#ifdef USE_MNG
	memset(&Portrait, 0, sizeof(Portrait));
#endif
	memset(RepairCosts, 0, sizeof(RepairCosts));
	memset(CanStore, 0, sizeof(CanStore));
	//Wyrmgus start
	memset(GrandStrategyProductionEfficiencyModifier, 0, sizeof(GrandStrategyProductionEfficiencyModifier));
	//Wyrmgus end
	memset(ResInfo, 0, sizeof(ResInfo));
	//Wyrmgus start
	memset(VarInfo, 0, sizeof(VarInfo));
	memset(Drops, 0, sizeof(Drops));
//	memset(ImproveIncomes, 0, sizeof(ImproveIncomes));
	//Wyrmgus end
	memset(MissileOffsets, 0, sizeof(MissileOffsets));
	//Wyrmgus start
	memset(ShieldAnimation, 0, sizeof(ShieldAnimation));
	//Wyrmgus end
}

CUnitType::~CUnitType()
{
	delete DeathExplosion;
	delete OnHit;
	delete OnEachCycle;
	delete OnEachSecond;
	delete OnInit;
	delete TeleportEffectIn;
	delete TeleportEffectOut;

	BoolFlag.clear();

	// Free Building Restrictions if there are any
	for (std::vector<CBuildRestriction *>::iterator b = BuildingRules.begin();
		 b != BuildingRules.end(); ++b) {
		delete *b;
	}
	BuildingRules.clear();
	for (std::vector<CBuildRestriction *>::iterator b = AiBuildingRules.begin();
		 b != AiBuildingRules.end(); ++b) {
		delete *b;
	}
	AiBuildingRules.clear();

	delete[] CanCastSpell;
	delete[] AutoCastActive;

	for (int res = 0; res < MaxCosts; ++res) {
		if (this->ResInfo[res]) {
			if (this->ResInfo[res]->SpriteWhenLoaded) {
				CGraphic::Free(this->ResInfo[res]->SpriteWhenLoaded);
			}
			if (this->ResInfo[res]->SpriteWhenEmpty) {
				CGraphic::Free(this->ResInfo[res]->SpriteWhenEmpty);
			}
			delete this->ResInfo[res];
		}
	}

	//Wyrmgus start
	for (int var = 0; var < VariationMax; ++var) {
		if (this->VarInfo[var]) {
			delete this->VarInfo[var];
		}
	}
	
	for (int i = 0; i < AnimationFrameMax; ++i) {
		if (this->ShieldAnimation[i]) {
			delete this->ShieldAnimation[i];
		}
	}
	//Wyrmgus end

	CGraphic::Free(Sprite);
	CGraphic::Free(ShadowSprite);
	//Wyrmgus start
	CGraphic::Free(LightSprite);
	CGraphic::Free(LeftArmSprite);
	CGraphic::Free(RightArmSprite);
	CGraphic::Free(HairSprite);
	CGraphic::Free(ClothingSprite);
	CGraphic::Free(ClothingLeftArmSprite);
	CGraphic::Free(ClothingRightArmSprite);
	CGraphic::Free(PantsSprite);
	CGraphic::Free(ShoesSprite);
	CGraphic::Free(WeaponSprite);
	CGraphic::Free(ShieldSprite);
	CGraphic::Free(HelmetSprite);
	//Wyrmgus end
#ifdef USE_MNG
	if (this->Portrait.Num) {
		for (int j = 0; j < this->Portrait.Num; ++j) {
			delete this->Portrait.Mngs[j];
			// delete[] this->Portrait.Files[j];
		}
		delete[] this->Portrait.Mngs;
		delete[] this->Portrait.Files;
	}
#endif
}

PixelSize CUnitType::GetPixelSize() const
{
	return PixelSize(TileWidth * PixelTileSize.x, TileHeight * PixelTileSize.y);
}

bool CUnitType::CheckUserBoolFlags(const char *BoolFlags) const
{
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberBoolFlag(); ++i) { // User defined flags
		if (BoolFlags[i] != CONDITION_TRUE &&
			((BoolFlags[i] == CONDITION_ONLY) ^ (BoolFlag[i].value))) {
			return false;
		}
	}
	return true;
}

bool CUnitType::CanMove() const
{
	return Animations && Animations->Move;
}

bool CUnitType::CanSelect(GroupSelectionMode mode) const
{
	//Wyrmgus start
//	if (!IsNotSelectable) {
	if (!BoolFlag[ISNOTSELECTABLE_INDEX].value) {
	//Wyrmgus end
		switch (mode) {
			case SELECTABLE_BY_RECTANGLE_ONLY:
				//Wyrmgus start
//				return SelectableByRectangle;
				return BoolFlag[SELECTABLEBYRECTANGLE_INDEX].value;
				//Wyrmgus end
			case NON_SELECTABLE_BY_RECTANGLE_ONLY:
				//Wyrmgus start
//				return !SelectableByRectangle;
				return !BoolFlag[SELECTABLEBYRECTANGLE_INDEX].value;
				//Wyrmgus end
			default:
				return true;
		}
	}
	return false;
}

//Wyrmgus start
VariationInfo *CUnitType::GetDefaultVariation(CPlayer &player) const
{
	for (int i = 0; i < VariationMax; ++i) {
		VariationInfo *varinfo = this->VarInfo[i];
		if (!varinfo) {
			break;
		}
		if (!varinfo->Tileset.empty() && varinfo->Tileset != Map.Tileset->Name) {
			continue;
		}
		bool UpgradesCheck = true;
		for (int u = 0; u < VariationMax; ++u) {
			if (!varinfo->UpgradesRequired[u].empty() && UpgradeIdentAllowed(player, varinfo->UpgradesRequired[u].c_str()) != 'R') {
				UpgradesCheck = false;
				break;
			}
			if (!varinfo->UpgradesForbidden[u].empty() && UpgradeIdentAllowed(player, varinfo->UpgradesForbidden[u].c_str()) == 'R') {
				UpgradesCheck = false;
				break;
			}
		}
		if (UpgradesCheck == false) {
			continue;
		}
		return varinfo;
	}
	return this->VarInfo[VariationMax - 1];
}
//Wyrmgus end

void UpdateUnitStats(CUnitType &type, int reset)
{
	if (reset) {
		//Wyrmgus start
		/*
		for (int player = 0; player < PlayerMax; ++player) {
			type.Stats[player] = type.DefaultStat;
		}
		*/
		type.MapDefaultStat = type.DefaultStat;
		for (int player = 0; player < PlayerMax; ++player) {
			type.Stats[player] = type.MapDefaultStat;
		}
		type.MapSound = type.Sound;
		//Wyrmgus end
	}

	// Non-solid units can always be entered and they don't block anything
	//Wyrmgus start
//	if (type.NonSolid) {
	if (type.BoolFlag[NONSOLID_INDEX].value) {
	//Wyrmgus end
		if (type.Building) {
			type.MovementMask = MapFieldLandUnit |
								MapFieldSeaUnit |
								MapFieldBuilding |
								//Wyrmgus start
								MapFieldItem |
								MapFieldBridge |
								//Wyrmgus end
								MapFieldCoastAllowed |
								MapFieldWaterAllowed |
								MapFieldNoBuilding |
								MapFieldUnpassable;
			type.FieldFlags = MapFieldNoBuilding;
		} else {
			type.MovementMask = 0;
			type.FieldFlags = 0;
		}
		return;
	}

	//  As side effect we calculate the movement flags/mask here.
	switch (type.UnitType) {
		case UnitTypeLand:                              // on land
			//Wyrmgus start
			/*
			type.MovementMask =
				MapFieldLandUnit |
				MapFieldSeaUnit |
				MapFieldBuilding | // already occuppied
				MapFieldCoastAllowed |
				MapFieldWaterAllowed | // can't move on this
				MapFieldUnpassable;
			*/
			if (type.BoolFlag[DIMINUTIVE_INDEX].value) { // diminutive units can enter tiles occupied by other units and vice-versa
				type.MovementMask =
					MapFieldBuilding | // already occuppied
					MapFieldCoastAllowed |
					MapFieldWaterAllowed | // can't move on this
					MapFieldUnpassable;
			} else {
				type.MovementMask =
					MapFieldLandUnit |
					MapFieldSeaUnit |
					MapFieldBuilding | // already occuppied
					MapFieldCoastAllowed |
					MapFieldWaterAllowed | // can't move on this
					MapFieldUnpassable;
			}
			//Wyrmgus end
			break;
		case UnitTypeFly:                               // in air
			//Wyrmgus start
			/*
			type.MovementMask = MapFieldAirUnit; // already occuppied
				MapFieldAirUnit | // already occuppied
				MapFieldAirUnpassable;
			*/
			if (type.BoolFlag[DIMINUTIVE_INDEX].value) {
				type.MovementMask =
					MapFieldAirUnpassable;
			} else {
				type.MovementMask =
					MapFieldAirUnit | // already occuppied
					MapFieldAirUnpassable;
			}
			//Wyrmgus end
			break;
		//Wyrmgus start
		case UnitTypeFlyLow:                               // in low air
			if (type.BoolFlag[DIMINUTIVE_INDEX].value) {
				type.MovementMask =
					MapFieldBuilding |
					MapFieldUnpassable |
					MapFieldAirUnpassable;
			} else {
				type.MovementMask =
					MapFieldLandUnit |
					MapFieldSeaUnit |
					MapFieldBuilding |
					MapFieldUnpassable |
					MapFieldAirUnpassable;
			}
			break;
		case UnitTypeNaval:                             // on water
			//Wyrmgus start
			/*
			if (type.CanTransport()) {
				type.MovementMask =
					MapFieldLandUnit |
					MapFieldSeaUnit |
					MapFieldBuilding | // already occuppied
					//Wyrmgus start
					MapFieldBridge |
					//Wyrmgus end
					MapFieldLandAllowed; // can't move on this
				// Johns: MapFieldUnpassable only for land units?
			*/
			if (type.BoolFlag[CANDOCK_INDEX].value) {
					MapFieldLandUnit |
					MapFieldSeaUnit |
					MapFieldBuilding | // already occuppied
					//Wyrmgus start
					MapFieldBridge |
					//Wyrmgus end
					MapFieldLandAllowed | // can't move on this
					MapFieldUnpassable;
			} else if (type.BoolFlag[CANDOCK_INDEX].value && type.BoolFlag[DIMINUTIVE_INDEX].value) { //should add case for when is a transporter and is diminutive?
				type.MovementMask =
					MapFieldBuilding | // already occuppied
					MapFieldBridge |
					MapFieldLandAllowed | // can't move on this
					MapFieldUnpassable;
			} else if (type.BoolFlag[DIMINUTIVE_INDEX].value) { //should add case for when is a transporter and is diminutive?
				type.MovementMask =
					MapFieldBuilding | // already occuppied
					MapFieldBridge |
					MapFieldCoastAllowed |
					MapFieldLandAllowed | // can't move on this
					MapFieldUnpassable;
			//Wyrmgus end
			} else {
				type.MovementMask =
					MapFieldLandUnit |
					MapFieldSeaUnit |
					MapFieldBuilding | // already occuppied
					//Wyrmgus start
					MapFieldBridge |
					//Wyrmgus end
					MapFieldCoastAllowed |
					MapFieldLandAllowed | // can't move on this
					MapFieldUnpassable;
			}
			break;
		default:
			DebugPrint("Where moves this unit?\n");
			type.MovementMask = 0;
			break;
	}
	//Wyrmgus start
//	if (type.Building || type.ShoreBuilding) {
	if (type.Building || type.BoolFlag[SHOREBUILDING_INDEX].value) {
	//Wyrmgus end
		// Shore building is something special.
		//Wyrmgus start
//		if (type.ShoreBuilding) {
		if (type.BoolFlag[SHOREBUILDING_INDEX].value) {
		//Wyrmgus end
			type.MovementMask =
				MapFieldLandUnit |
				MapFieldSeaUnit |
				MapFieldBuilding | // already occuppied
				//Wyrmgus start
				MapFieldBridge |
				//Wyrmgus end
				MapFieldLandAllowed; // can't build on this
		}
		type.MovementMask |= MapFieldNoBuilding;
		//
		// A little chaos, buildings without HP can be entered.
		// The oil-patch is a very special case.
		//
		//Wyrmgus start
//		if (type.DefaultStat.Variables[HP_INDEX].Max) {
		if (type.MapDefaultStat.Variables[HP_INDEX].Max) {
		//Wyrmgus end
			type.FieldFlags = MapFieldBuilding;
		} else {
			type.FieldFlags = MapFieldNoBuilding;
		}
	//Wyrmgus start
	} else if (type.BoolFlag[ITEM_INDEX].value || type.BoolFlag[TRAP_INDEX].value) {
		type.MovementMask = MapFieldLandUnit |
							MapFieldSeaUnit |
							MapFieldBuilding |
							MapFieldCoastAllowed |
							MapFieldWaterAllowed |
							MapFieldUnpassable |
							MapFieldItem;
		type.FieldFlags = MapFieldItem;
	} else if (type.BoolFlag[BRIDGE_INDEX].value) {
		type.MovementMask = MapFieldSeaUnit |
							MapFieldBuilding |
							MapFieldLandAllowed |
							MapFieldUnpassable |
							MapFieldBridge;
		type.FieldFlags = MapFieldBridge;
	//Wyrmgus end
	//Wyrmgus start
//	} else {
	} else if (!type.BoolFlag[DIMINUTIVE_INDEX].value) {
	//Wyrmgus end
		switch (type.UnitType) {
			case UnitTypeLand: // on land
				type.FieldFlags = MapFieldLandUnit;			
				//Wyrmgus start
				if (type.BoolFlag[AIRUNPASSABLE_INDEX].value) { // for air unpassable units (i.e. doors)
					type.FieldFlags |= MapFieldAirUnpassable;
				}
				if (type.BoolFlag[GRAVEL_INDEX].value) {
					type.FieldFlags |= MapFieldGravel;
				}
				//Wyrmgus end
				break;
			case UnitTypeFly: // in air
				type.FieldFlags = MapFieldAirUnit;
				break;
			//Wyrmgus start
			case UnitTypeFlyLow: // in low air
				type.FieldFlags = MapFieldLandUnit;			
				if (type.BoolFlag[AIRUNPASSABLE_INDEX].value) { // for air unpassable units (i.e. doors)
					type.FieldFlags |= MapFieldAirUnpassable;
				}
				break;
			//Wyrmgus end
			case UnitTypeNaval: // on water
				type.FieldFlags = MapFieldSeaUnit;
				//Wyrmgus start
				if (type.BoolFlag[AIRUNPASSABLE_INDEX].value) { // for air unpassable units (i.e. doors)
					type.FieldFlags |= MapFieldAirUnpassable;
				}
				//Wyrmgus end
				break;
			default:
				DebugPrint("Where moves this unit?\n");
				type.FieldFlags = 0;
				break;
		}
	}
}


/**
**  Update the player stats for changed unit types.
**  @param reset indicates whether the default value should be set to each stat (level, upgrades)
*/
void UpdateStats(int reset)
{
	// Update players stats
	for (std::vector<CUnitType *>::size_type j = 0; j < UnitTypes.size(); ++j) {
		CUnitType &type = *UnitTypes[j];
		UpdateUnitStats(type, reset);
	}
}

/**
**  Save state of an unit-stats to file.
**
**  @param stats  Unit-stats to save.
**  @param ident  Unit-type ident.
**  @param plynr  Player number.
**  @param file   Output file.
*/
static bool SaveUnitStats(const CUnitStats &stats, const CUnitType &type, int plynr,
						  CFile &file)
{
	Assert(plynr < PlayerMax);

	if (stats == type.DefaultStat) {
		return false;
	}
	file.printf("DefineUnitStats(\"%s\", %d, {\n  ", type.Ident.c_str(), plynr);
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); ++i) {
		file.printf("\"%s\", {Value = %d, Max = %d, Increase = %d%s},\n  ",
					UnitTypeVar.VariableNameLookup[i], stats.Variables[i].Value,
					stats.Variables[i].Max, stats.Variables[i].Increase,
					stats.Variables[i].Enable ? ", Enable = true" : "");
	}
	file.printf("\"costs\", {");
	for (unsigned int i = 0; i < MaxCosts; ++i) {
		if (i) {
			file.printf(" ");
		}
		file.printf("\"%s\", %d,", DefaultResourceNames[i].c_str(), stats.Costs[i]);
	}
	file.printf("},\n\"storing\", {");
	for (unsigned int i = 0; i < MaxCosts; ++i) {
		if (i) {
			file.printf(" ");
		}
		file.printf("\"%s\", %d,", DefaultResourceNames[i].c_str(), stats.Storing[i]);
	}
	//Wyrmgus start
	file.printf("},\n\"improve-production\", {");
	for (unsigned int i = 0; i < MaxCosts; ++i) {
		if (i) {
			file.printf(" ");
		}
		file.printf("\"%s\", %d,", DefaultResourceNames[i].c_str(), stats.ImproveIncomes[i]);
	}
	//Wyrmgus end
	file.printf("}})\n");
	return true;
}

/**
**  Save state of the unit-type table to file.
**
**  @param file  Output file.
*/
void SaveUnitTypes(CFile &file)
{
	file.printf("\n--- -----------------------------------------\n");
	file.printf("--- MODULE: unittypes\n\n");

	// Save all stats
	for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
		const CUnitType &type = *UnitTypes[i];
		bool somethingSaved = false;

		for (int j = 0; j < PlayerMax; ++j) {
			if (Players[j].Type != PlayerNobody) {
				somethingSaved |= SaveUnitStats(type.Stats[j], type, j, file);
			}
		}
		if (somethingSaved) {
			file.printf("\n");
		}
	}
}

/**
**  Find unit-type by identifier.
**
**  @param ident  The unit-type identifier.
**
**  @return       Unit-type pointer.
*/
CUnitType *UnitTypeByIdent(const std::string &ident)
{
	std::map<std::string, CUnitType *>::iterator ret = UnitTypeMap.find(ident);
	if (ret != UnitTypeMap.end()) {
		return (*ret).second;
	}
	//Wyrmgus start
//	fprintf(stderr, "Unit type \"%s\" does not exist.\n", ident.c_str());
	//Wyrmgus end
	return NULL;
}

//Wyrmgus start
int GetUnitTypeClassIndexByName(const std::string &class_name)
{
	if (UnitTypeClassStringToIndex.find(class_name) != UnitTypeClassStringToIndex.end()) {
		return UnitTypeClassStringToIndex[class_name];
	}
	return -1;
}

void SetUnitTypeClassStringToIndex(std::string class_name, int class_id)
{
	UnitTypeClassStringToIndex[class_name] = class_id;
}

int GetUpgradeClassIndexByName(const std::string &class_name)
{
	if (UpgradeClassStringToIndex.find(class_name) != UpgradeClassStringToIndex.end()) {
		return UpgradeClassStringToIndex[class_name];
	}
	return -1;
}

void SetUpgradeClassStringToIndex(std::string class_name, int class_id)
{
	UpgradeClassStringToIndex[class_name] = class_id;
}
//Wyrmgus end

/**
**  Allocate an empty unit-type slot.
**
**  @param ident  Identifier to identify the slot (malloced by caller!).
**
**  @return       New allocated (zeroed) unit-type pointer.
*/
CUnitType *NewUnitTypeSlot(const std::string &ident)
{
	size_t new_bool_size = UnitTypeVar.GetNumberBoolFlag();
	CUnitType *type = new CUnitType;

	if (!type) {
		fprintf(stderr, "Out of memory\n");
		ExitFatal(-1);
	}
	type->Slot = UnitTypes.size();
	type->Ident = ident;
	type->BoolFlag.resize(new_bool_size);

	type->DefaultStat.Variables = new CVariable[UnitTypeVar.GetNumberVariable()];
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); ++i) {
		type->DefaultStat.Variables[i] = UnitTypeVar.Variable[i];
	}
	UnitTypes.push_back(type);
	UnitTypeMap[type->Ident] = type;
	return type;
}

/**
**  Draw unit-type on map.
**
**  @param type    Unit-type pointer.
**  @param sprite  Sprite to use for drawing
**  @param player  Player number for color substitution.
**  @param frame   Animation frame of unit-type.
**  @param screenPos  Screen pixel (top left) position to draw unit-type.
**
**  @todo  Do screen position caculation in high level.
**         Better way to handle in x mirrored sprites.
*/
void DrawUnitType(const CUnitType &type, CPlayerColorGraphic *sprite, int player, int frame, const PixelPos &screenPos)
{
	PixelPos pos = screenPos;
	// FIXME: move this calculation to high level.
	//Wyrmgus start
//	pos.x -= (type.Width - type.TileWidth * PixelTileSize.x) / 2;
//	pos.y -= (type.Height - type.TileHeight * PixelTileSize.y) / 2;
	pos.x -= (sprite->Width - type.TileWidth * PixelTileSize.x) / 2;
	pos.y -= (sprite->Height - type.TileHeight * PixelTileSize.y) / 2;
	//Wyrmgus end
	pos.x += type.OffsetX;
	pos.y += type.OffsetY;

	//Wyrmgus start
	/*
	if (type.Flip) {
		if (frame < 0) {
			sprite->DrawPlayerColorFrameClipX(player, -frame - 1, pos.x, pos.y);
		} else {
			sprite->DrawPlayerColorFrameClip(player, frame, pos.x, pos.y);
		}
	} else {
		const int row = type.NumDirections / 2 + 1;

		if (frame < 0) {
			frame = ((-frame - 1) / row) * type.NumDirections + type.NumDirections - (-frame - 1) % row;
		} else {
			frame = (frame / row) * type.NumDirections + frame % row;
		}
		sprite->DrawPlayerColorFrameClip(player, frame, pos.x, pos.y);
	}
	*/
	if (type.Flip) {
		if (frame < 0) {
			if (type.Stats[player].Variables[TRANSPARENCY_INDEX].Value > 0) {
				sprite->DrawPlayerColorFrameClipTransX(player, -frame - 1, pos.x, pos.y, int(256 - 2.56 * type.Stats[player].Variables[TRANSPARENCY_INDEX].Value), false);
			} else {
				sprite->DrawPlayerColorFrameClipX(player, -frame - 1, pos.x, pos.y, false);
			}
		} else {
			if (type.Stats[player].Variables[TRANSPARENCY_INDEX].Value > 0) {
				sprite->DrawPlayerColorFrameClipTrans(player, frame, pos.x, pos.y, int(256 - 2.56 * type.Stats[player].Variables[TRANSPARENCY_INDEX].Value), false);
			} else {
				sprite->DrawPlayerColorFrameClip(player, frame, pos.x, pos.y, false);
			}
		}
	} else {
		const int row = type.NumDirections / 2 + 1;

		if (frame < 0) {
			frame = ((-frame - 1) / row) * type.NumDirections + type.NumDirections - (-frame - 1) % row;
		} else {
			frame = (frame / row) * type.NumDirections + frame % row;
		}
		if (type.Stats[player].Variables[TRANSPARENCY_INDEX].Value > 0) {
			sprite->DrawPlayerColorFrameClipTrans(player, frame, pos.x, pos.y, int(256 - 2.56 * type.Stats[player].Variables[TRANSPARENCY_INDEX].Value), false);
		} else {
			sprite->DrawPlayerColorFrameClip(player, frame, pos.x, pos.y, false);
		}
	}
	//Wyrmgus end
}

/**
**  Get the still animation frame
*/
static int GetStillFrame(const CUnitType &type)
{
	CAnimation *anim = type.Animations->Still;

	while (anim) {
		if (anim->Type == AnimationFrame) {
			CAnimation_Frame &a_frame = *static_cast<CAnimation_Frame *>(anim);
			// Use the frame facing down
			return a_frame.ParseAnimInt(NULL) + type.NumDirections / 2;
		} else if (anim->Type == AnimationExactFrame) {
			CAnimation_ExactFrame &a_frame = *static_cast<CAnimation_ExactFrame *>(anim);

			return a_frame.ParseAnimInt(NULL);
		}
		anim = anim->Next;
	}
	return type.NumDirections / 2;
}

/**
**  Init unit types.
*/
void InitUnitTypes(int reset_player_stats)
{
	for (size_t i = 0; i < UnitTypes.size(); ++i) {
		CUnitType &type = *UnitTypes[i];
		Assert(type.Slot == (int)i);

		if (type.Animations == NULL) {
			DebugPrint(_("unit-type '%s' without animations, ignored.\n") _C_ type.Ident.c_str());
			continue;
		}
		//  Add idents to hash.
		UnitTypeMap[type.Ident] = UnitTypes[i];

		// Determine still frame
		type.StillFrame = GetStillFrame(type);

		// Lookup BuildingTypes
		for (std::vector<CBuildRestriction *>::iterator b = type.BuildingRules.begin();
			 b < type.BuildingRules.end(); ++b) {
			(*b)->Init();
		}

		// Lookup AiBuildingTypes
		for (std::vector<CBuildRestriction *>::iterator b = type.AiBuildingRules.begin();
			 b < type.AiBuildingRules.end(); ++b) {
			(*b)->Init();
		}
	}

	// LUDO : called after game is loaded -> don't reset stats !
	UpdateStats(reset_player_stats); // Calculate the stats
}

/**
**  Loads the Sprite for a unit type
**
**  @param type  type of unit to load
*/
void LoadUnitTypeSprite(CUnitType &type)
{
	if (!type.ShadowFile.empty()) {
		type.ShadowSprite = CGraphic::ForceNew(type.ShadowFile, type.ShadowWidth, type.ShadowHeight);
		type.ShadowSprite->Load();
		if (type.Flip) {
			type.ShadowSprite->Flip();
		}
		//Wyrmgus start
//		type.ShadowSprite->MakeShadow();
		if (type.ShadowSprite->Surface->format->BytesPerPixel == 1) {
//			type.ShadowSprite->MakeShadow();
		}
		//Wyrmgus end
	}

	//Wyrmgus start
//	if (type.Harvester) {
	if (type.BoolFlag[HARVESTER_INDEX].value) {
	//Wyrmgus end
		for (int i = 0; i < MaxCosts; ++i) {
			ResourceInfo *resinfo = type.ResInfo[i];
			if (!resinfo) {
				continue;
			}
			if (!resinfo->FileWhenLoaded.empty()) {
				resinfo->SpriteWhenLoaded = CPlayerColorGraphic::New(resinfo->FileWhenLoaded,
																	 type.Width, type.Height);
				resinfo->SpriteWhenLoaded->Load();
				if (type.Flip) {
					resinfo->SpriteWhenLoaded->Flip();
				}
			}
			if (!resinfo->FileWhenEmpty.empty()) {
				resinfo->SpriteWhenEmpty = CPlayerColorGraphic::New(resinfo->FileWhenEmpty,
																	type.Width, type.Height);
				resinfo->SpriteWhenEmpty->Load();
				if (type.Flip) {
					resinfo->SpriteWhenEmpty->Flip();
				}
			}
		}
	}

	if (!type.File.empty()) {
		type.Sprite = CPlayerColorGraphic::New(type.File, type.Width, type.Height);
		type.Sprite->Load();
		if (type.Flip) {
			type.Sprite->Flip();
		}
	}

#ifdef USE_MNG
	if (type.Portrait.Num) {
		for (int i = 0; i < type.Portrait.Num; ++i) {
			type.Portrait.Mngs[i] = new Mng;
			type.Portrait.Mngs[i]->Load(type.Portrait.Files[i]);
		}
		// FIXME: should be configurable
		type.Portrait.CurrMng = 0;
		type.Portrait.NumIterations = SyncRand() % 16 + 1;
	}
#endif

	//Wyrmgus start
	if (!type.LightFile.empty()) {
		type.LightSprite = CGraphic::New(type.LightFile, type.Width, type.Height);
		type.LightSprite->Load();
		if (type.Flip) {
			type.LightSprite->Flip();
		}
	}
	if (!type.LeftArmFile.empty()) {
		type.LeftArmSprite = CPlayerColorGraphic::New(type.LeftArmFile, type.Width, type.Height);
		type.LeftArmSprite->Load();
		if (type.Flip) {
			type.LeftArmSprite->Flip();
		}
	}
	if (!type.RightArmFile.empty()) {
		type.RightArmSprite = CPlayerColorGraphic::New(type.RightArmFile, type.Width, type.Height);
		type.RightArmSprite->Load();
		if (type.Flip) {
			type.RightArmSprite->Flip();
		}
	}
	if (!type.HairFile.empty()) {
		type.HairSprite = CPlayerColorGraphic::New(type.HairFile, type.Width, type.Height);
		type.HairSprite->Load();
		if (type.Flip) {
			type.HairSprite->Flip();
		}
	}
	if (!type.ClothingFile.empty()) {
		type.ClothingSprite = CPlayerColorGraphic::New(type.ClothingFile, type.Width, type.Height);
		type.ClothingSprite->Load();
		if (type.Flip) {
			type.ClothingSprite->Flip();
		}
	}
	if (!type.ClothingLeftArmFile.empty()) {
		type.ClothingLeftArmSprite = CPlayerColorGraphic::New(type.ClothingLeftArmFile, type.Width, type.Height);
		type.ClothingLeftArmSprite->Load();
		if (type.Flip) {
			type.ClothingLeftArmSprite->Flip();
		}
	}
	if (!type.ClothingRightArmFile.empty()) {
		type.ClothingRightArmSprite = CPlayerColorGraphic::New(type.ClothingRightArmFile, type.Width, type.Height);
		type.ClothingRightArmSprite->Load();
		if (type.Flip) {
			type.ClothingRightArmSprite->Flip();
		}
	}
	if (!type.PantsFile.empty()) {
		type.PantsSprite = CPlayerColorGraphic::New(type.PantsFile, type.Width, type.Height);
		type.PantsSprite->Load();
		if (type.Flip) {
			type.PantsSprite->Flip();
		}
	}
	if (!type.ShoesFile.empty()) {
		type.ShoesSprite = CPlayerColorGraphic::New(type.ShoesFile, type.Width, type.Height);
		type.ShoesSprite->Load();
		if (type.Flip) {
			type.ShoesSprite->Flip();
		}
	}
	if (!type.WeaponFile.empty()) {
		type.WeaponSprite = CPlayerColorGraphic::New(type.WeaponFile, type.Width, type.Height);
		type.WeaponSprite->Load();
		if (type.Flip) {
			type.WeaponSprite->Flip();
		}
	}
	if (!type.ShieldFile.empty()) {
		type.ShieldSprite = CPlayerColorGraphic::New(type.ShieldFile, type.Width, type.Height);
		type.ShieldSprite->Load();
		if (type.Flip) {
			type.ShieldSprite->Flip();
		}
	}
	if (!type.HelmetFile.empty()) {
		type.HelmetSprite = CPlayerColorGraphic::New(type.HelmetFile, type.Width, type.Height);
		type.HelmetSprite->Load();
		if (type.Flip) {
			type.HelmetSprite->Flip();
		}
	}
	//Wyrmgus end

	//Wyrmgus start
	for (int i = 0; i < VariationMax; ++i) {
		VariationInfo *varinfo = type.VarInfo[i];
		if (!varinfo) {
			continue;
		}
		int frame_width = type.Width;
		int frame_height = type.Height;
		if (varinfo->FrameWidth && varinfo->FrameHeight) {
			frame_width = varinfo->FrameWidth;
			frame_height = varinfo->FrameHeight;
		}
		if (!varinfo->File.empty()) {
			varinfo->Sprite = CPlayerColorGraphic::New(varinfo->File,
																	 frame_width, frame_height);
			varinfo->Sprite->Load();
			if (type.Flip) {
				varinfo->Sprite->Flip();
			}
		}
		if (!varinfo->ShadowFile.empty()) {
			varinfo->ShadowSprite = CGraphic::New(varinfo->ShadowFile, type.ShadowWidth, type.ShadowHeight);
			varinfo->ShadowSprite->Load();
			if (type.Flip) {
				varinfo->ShadowSprite->Flip();
			}
			if (varinfo->ShadowSprite->Surface->format->BytesPerPixel == 1) {
//				varinfo->ShadowSprite->MakeShadow();
			}
		}
		if (!varinfo->LeftArmFile.empty()) {
			varinfo->LeftArmSprite = CPlayerColorGraphic::New(varinfo->LeftArmFile, frame_width, frame_height);
			varinfo->LeftArmSprite->Load();
			if (type.Flip) {
				varinfo->LeftArmSprite->Flip();
			}
		}
		if (!varinfo->RightArmFile.empty()) {
			varinfo->RightArmSprite = CPlayerColorGraphic::New(varinfo->RightArmFile, frame_width, frame_height);
			varinfo->RightArmSprite->Load();
			if (type.Flip) {
				varinfo->RightArmSprite->Flip();
			}
		}
		if (!varinfo->HairFile.empty()) {
			varinfo->HairSprite = CPlayerColorGraphic::New(varinfo->HairFile, frame_width, frame_height);
			varinfo->HairSprite->Load();
			if (type.Flip) {
				varinfo->HairSprite->Flip();
			}
		}
		if (!varinfo->ClothingFile.empty()) {
			varinfo->ClothingSprite = CPlayerColorGraphic::New(varinfo->ClothingFile, frame_width, frame_height);
			varinfo->ClothingSprite->Load();
			if (type.Flip) {
				varinfo->ClothingSprite->Flip();
			}
		}
		if (!varinfo->ClothingLeftArmFile.empty()) {
			varinfo->ClothingLeftArmSprite = CPlayerColorGraphic::New(varinfo->ClothingLeftArmFile, frame_width, frame_height);
			varinfo->ClothingLeftArmSprite->Load();
			if (type.Flip) {
				varinfo->ClothingLeftArmSprite->Flip();
			}
		}
		if (!varinfo->ClothingRightArmFile.empty()) {
			varinfo->ClothingRightArmSprite = CPlayerColorGraphic::New(varinfo->ClothingRightArmFile, frame_width, frame_height);
			varinfo->ClothingRightArmSprite->Load();
			if (type.Flip) {
				varinfo->ClothingRightArmSprite->Flip();
			}
		}
		if (!varinfo->PantsFile.empty()) {
			varinfo->PantsSprite = CPlayerColorGraphic::New(varinfo->PantsFile, frame_width, frame_height);
			varinfo->PantsSprite->Load();
			if (type.Flip) {
				varinfo->PantsSprite->Flip();
			}
		}
		if (!varinfo->ShoesFile.empty()) {
			varinfo->ShoesSprite = CPlayerColorGraphic::New(varinfo->ShoesFile, frame_width, frame_height);
			varinfo->ShoesSprite->Load();
			if (type.Flip) {
				varinfo->ShoesSprite->Flip();
			}
		}
		if (!varinfo->WeaponFile.empty()) {
			varinfo->WeaponSprite = CPlayerColorGraphic::New(varinfo->WeaponFile, frame_width, frame_height);
			varinfo->WeaponSprite->Load();
			if (type.Flip) {
				varinfo->WeaponSprite->Flip();
			}
		}
		if (!varinfo->ShieldFile.empty()) {
			varinfo->ShieldSprite = CPlayerColorGraphic::New(varinfo->ShieldFile, frame_width, frame_height);
			varinfo->ShieldSprite->Load();
			if (type.Flip) {
				varinfo->ShieldSprite->Flip();
			}
		}
		if (!varinfo->HelmetFile.empty()) {
			varinfo->HelmetSprite = CPlayerColorGraphic::New(varinfo->HelmetFile, frame_width, frame_height);
			varinfo->HelmetSprite->Load();
			if (type.Flip) {
				varinfo->HelmetSprite->Flip();
			}
		}
		for (int j = 0; j < MaxCosts; ++j) {
			if (!varinfo->FileWhenLoaded[j].empty()) {
				varinfo->SpriteWhenLoaded[j] = CPlayerColorGraphic::New(varinfo->FileWhenLoaded[j],
																		 frame_width, frame_height);
				varinfo->SpriteWhenLoaded[j]->Load();
				if (type.Flip) {
					varinfo->SpriteWhenLoaded[j]->Flip();
				}
			}
			if (!varinfo->FileWhenEmpty[j].empty()) {
				varinfo->SpriteWhenEmpty[j] = CPlayerColorGraphic::New(varinfo->FileWhenEmpty[j],
																		 frame_width, frame_height);
				varinfo->SpriteWhenEmpty[j]->Load();
				if (type.Flip) {
					varinfo->SpriteWhenEmpty[j]->Flip();
				}
			}
		}
	}
	//Wyrmgus end
}

/**
** Load the graphics for the unit-types.
*/
void LoadUnitTypes()
{
	for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
		CUnitType &type = *UnitTypes[i];

		// Lookup icons.
		//Wyrmgus start
//		type.Icon.Load();
		if (!type.Icon.Name.empty()) {
			type.Icon.Load();
		}
		//Wyrmgus end

		//Wyrmgus start
		for (int j = 0; j < VariationMax; ++j) {
			VariationInfo *varinfo = type.VarInfo[j];
			if (!varinfo) {
				continue;
			}
			if (!varinfo->Icon.Name.empty()) {
				varinfo->Icon.Load();
			}
		}
		//Wyrmgus end

		// Lookup missiles.
		type.Missile.MapMissile();
		type.Explosion.MapMissile();

		// Lookup impacts
		for (int i = 0; i < ANIMATIONS_DEATHTYPES + 2; ++i) {
			type.Impact[i].MapMissile();
		}
		// Lookup corpse.
		if (!type.CorpseName.empty()) {
			type.CorpseType = UnitTypeByIdent(type.CorpseName);
		}
#ifndef DYNAMIC_LOAD
		// Load Sprite
		if (!type.Sprite) {
			ShowLoadProgress(_("Unit \"%s\""), type.Name.c_str());
			LoadUnitTypeSprite(type);
		}
#endif
		// FIXME: should i copy the animations of same graphics?
	}
}

void CUnitTypeVar::Init()
{
	// Variables.
	Variable.resize(GetNumberVariable());
	size_t new_size = UnitTypeVar.GetNumberBoolFlag();
	for (unsigned int i = 0; i < UnitTypes.size(); ++i) { // adjust array for unit already defined
		UnitTypes[i]->BoolFlag.resize(new_size);
	}
}

void CUnitTypeVar::Clear()
{
	Variable.clear();

	for (std::vector<CDecoVar *>::iterator it = DecoVar.begin();
		 it != DecoVar.end(); ++it) {
		delete(*it);
	}
	DecoVar.clear();
}

/**
**  Cleanup the unit-type module.
*/
void CleanUnitTypes()
{
	DebugPrint("FIXME: icon, sounds not freed.\n");
	FreeAnimations();

	// Clean all unit-types
	for (size_t i = 0; i < UnitTypes.size(); ++i) {
		delete UnitTypes[i];
	}
	UnitTypes.clear();
	UnitTypeMap.clear();
	UnitTypeVar.Clear();

	// Clean hardcoded unit types.
	UnitTypeHumanWall = NULL;
	UnitTypeOrcWall = NULL;
}

//Wyrmgus start
VariationInfo::~VariationInfo()
{
	if (this->Sprite) {
		CGraphic::Free(this->Sprite);
	}
	if (this->ShadowSprite) {
		CGraphic::Free(this->ShadowSprite);
	}
	if (this->LeftArmSprite) {
		CGraphic::Free(this->LeftArmSprite);
	}
	if (this->RightArmSprite) {
		CGraphic::Free(this->RightArmSprite);
	}
	if (this->HairSprite) {
		CGraphic::Free(this->HairSprite);
	}
	if (this->ClothingSprite) {
		CGraphic::Free(this->ClothingSprite);
	}
	if (this->ClothingLeftArmSprite) {
		CGraphic::Free(this->ClothingLeftArmSprite);
	}
	if (this->ClothingRightArmSprite) {
		CGraphic::Free(this->ClothingRightArmSprite);
	}
	if (this->PantsSprite) {
		CGraphic::Free(this->PantsSprite);
	}
	if (this->ShoesSprite) {
		CGraphic::Free(this->ShoesSprite);
	}
	if (this->WeaponSprite) {
		CGraphic::Free(this->WeaponSprite);
	}
	if (this->ShieldSprite) {
		CGraphic::Free(this->ShieldSprite);
	}
	if (this->HelmetSprite) {
		CGraphic::Free(this->HelmetSprite);
	}
	for (int res = 0; res < MaxCosts; ++res) {
		if (this->SpriteWhenLoaded[res]) {
			CGraphic::Free(this->SpriteWhenLoaded[res]);
		}
		if (this->SpriteWhenEmpty[res]) {
			CGraphic::Free(this->SpriteWhenEmpty[res]);
		}
	}
	for (int i = 0; i < AnimationFrameMax; ++i) {
		if (this->ShieldAnimation[i]) {
			delete this->ShieldAnimation[i];
		}
	}
}
//Wyrmgus end

//@}
