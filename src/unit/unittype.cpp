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
//      (c) Copyright 1998-2015 by Lutz Sammer, Jimmy Salmon and Andrettin
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

//Wyrmgus start
#include "../ai/ai_local.h" //for using AiHelpers
//Wyrmgus end
#include "animation.h"
#include "animation/animation_exactframe.h"
#include "animation/animation_frame.h"
#include "construct.h"
//Wyrmgus start
#include "editor.h" //for personal name generation
//Wyrmgus end
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
**  CUnitType::Transporter
**
**    Can transport units
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
**    CUnitType::ShieldPiercing
**
**    Can directly damage shield-protected units, without shield damaging.
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
//Wyrmgus start
//CUnitType *UnitTypeHumanWall;       /// Human wall
//CUnitType *UnitTypeOrcWall;         /// Orc wall
//Wyrmgus end

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
bool LuxuryResources[MaxCosts];
int DefaultResourceFinalResources[MaxCosts];
int DefaultResourceFinalResourceConversionRates[MaxCosts];
int DefaultResourceInputResources[MaxCosts];
int DefaultResourcePrices[MaxCosts];
//Wyrmgus end

/**
**  Default names for the resources.
*/
std::string ExtraDeathTypes[ANIMATIONS_DEATHTYPES];

//Wyrmgus start
std::vector<std::string> UnitTypeClasses;
std::map<std::string, int> UnitTypeClassStringToIndex;
std::vector<std::string> UpgradeClasses;
std::map<std::string, int> UpgradeClassStringToIndex;
CUnitType *SettlementSiteUnitType;

std::vector<CSpecies *> Species;
std::vector<CSpeciesGenus *> SpeciesGenuses;
std::vector<CSpeciesFamily *> SpeciesFamilies;
std::vector<CSpeciesOrder *> SpeciesOrders;
std::vector<CSpeciesClass *> SpeciesClasses;
std::vector<CSpeciesPhylum *> SpeciesPhylums;
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
//Wyrmgus end

CUnitType::CUnitType() :
	Slot(0), Width(0), Height(0), OffsetX(0), OffsetY(0), DrawLevel(0),
	ShadowWidth(0), ShadowHeight(0), ShadowOffsetX(0), ShadowOffsetY(0),
	//Wyrmgus start
	TrainQuantity(0), ItemClass(-1), HairColor(0),
	Class(-1), Civilization(-1), Faction(-1), Species(NULL), TerrainType(NULL),
	//Wyrmgus end
	Animations(NULL), StillFrame(0),
	DeathExplosion(NULL), OnHit(NULL), OnEachCycle(NULL), OnEachSecond(NULL), OnInit(NULL),
	TeleportCost(0), TeleportEffectIn(NULL), TeleportEffectOut(NULL),
	CorpseType(NULL), Construction(NULL), RepairHP(0), TileSize(0),
	BoxWidth(0), BoxHeight(0), BoxOffsetX(0), BoxOffsetY(0), NumDirections(0),
	//Wyrmgus start
//	MinAttackRange(0), ReactRangeComputer(0), ReactRangePerson(0),
	MinAttackRange(0),
	//Wyrmgus end
	BurnPercent(0), BurnDamageRate(0), RepairRange(0),
	CanCastSpell(NULL), AutoCastActive(NULL),
	AutoBuildRate(0), RandomMovementProbability(0), RandomMovementDistance(1), ClicksToExplode(0),
	//Wyrmgus start
//	MaxOnBoard(0), BoardSize(1), ButtonLevelForTransporter(0), StartingResources(0),
	MaxOnBoard(0), BoardSize(1), ButtonLevelForTransporter(0), ButtonLevelForInventory(3), ButtonPos(0), ButtonLevel(0),
	//Wyrmgus end
	UnitType(UnitTypeLand), DecayRate(0), AnnoyComputerFactor(0), AiAdjacentRange(-1),
	MouseAction(0), CanTarget(0),
	Flip(0), LandUnit(0), AirUnit(0), SeaUnit(0),
	ExplodeWhenKilled(0), Building(0),
	CanAttack(0),
	Neutral(0),
	GivesResource(0), PoisonDrain(0), FieldFlags(0), MovementMask(0),
	//Wyrmgus start
	Elixir(NULL),
//	Sprite(NULL), ShadowSprite(NULL)
	Sprite(NULL), ShadowSprite(NULL), LightSprite(NULL)
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
	//Wyrmgus end
	memset(MissileOffsets, 0, sizeof(MissileOffsets));
	//Wyrmgus start
	memset(LayerSprites, 0, sizeof(LayerSprites));
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

	//Wyrmgus start
	SoldUnits.clear();
	SpawnUnits.clear();
	Drops.clear();
	AiDrops.clear();
	DropSpells.clear();
	Affixes.clear();
	Traits.clear();
	StartingAbilities.clear();
	//Wyrmgus end

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
	
	for (int i = 0; i < MaxImageLayers; ++i) {
		for (size_t var = 0; var < LayerVarInfo[i].size(); ++var) {
			delete this->LayerVarInfo[i][var];
		}
		LayerVarInfo[i].clear();
	}
	//Wyrmgus end

	CGraphic::Free(Sprite);
	CGraphic::Free(ShadowSprite);
	//Wyrmgus start
	CGraphic::Free(LightSprite);
	for (int i = 0; i < MaxImageLayers; ++i) {
		CGraphic::Free(LayerSprites[i]);
	}
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
	return PixelSize(TileSize * PixelTileSize);
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
	if (!BoolFlag[ISNOTSELECTABLE_INDEX].value) {
		switch (mode) {
			case SELECTABLE_BY_RECTANGLE_ONLY:
				return BoolFlag[SELECTABLEBYRECTANGLE_INDEX].value;
			case NON_SELECTABLE_BY_RECTANGLE_ONLY:
				return !BoolFlag[SELECTABLEBYRECTANGLE_INDEX].value;
			default:
				return true;
		}
	}
	return false;
}

//Wyrmgus start
void CUnitType::RemoveButtons(int button_action, std::string mod_file)
{
	int buttons_size = UnitButtonTable.size();
	for (int i = (buttons_size - 1); i >= 0; --i) {
		if (button_action != -1 && UnitButtonTable[i]->Action != button_action) {
			continue;
		}
		if (!mod_file.empty() && UnitButtonTable[i]->Mod != mod_file) {
			continue;
		}
		
		if (UnitButtonTable[i]->UnitMask == ("," + this->Ident + ",")) { //delete the appropriate buttons
			delete UnitButtonTable[i];
			UnitButtonTable.erase(std::remove(UnitButtonTable.begin(), UnitButtonTable.end(), UnitButtonTable[i]), UnitButtonTable.end());
		} else if (UnitButtonTable[i]->UnitMask.find(this->Ident) != std::string::npos) { //remove this unit from the "ForUnit" array of the appropriate buttons
			UnitButtonTable[i]->UnitMask = FindAndReplaceString(UnitButtonTable[i]->UnitMask, this->Ident + ",", "");
		}
	}
}

int CUnitType::GetAvailableLevelUpUpgrades() const
{
	int value = 0;
	int upgrade_value = 0;
	
	if (((int) AiHelpers.ExperienceUpgrades.size()) > this->Slot) {
		for (size_t i = 0; i != AiHelpers.ExperienceUpgrades[this->Slot].size(); ++i) {
			int local_upgrade_value = 1;
			
			local_upgrade_value += AiHelpers.ExperienceUpgrades[this->Slot][i]->GetAvailableLevelUpUpgrades();
			
			if (local_upgrade_value > upgrade_value) {
				upgrade_value = local_upgrade_value;
			}
		}
	}
	
	value += upgrade_value;
	
	if (((int) AiHelpers.LearnableAbilities.size()) > this->Slot) {
		for (size_t i = 0; i != AiHelpers.LearnableAbilities[this->Slot].size(); ++i) {
			value += 1;
		}
	}
	
	return value;
}

VariationInfo *CUnitType::GetDefaultVariation(CPlayer &player, int image_layer) const
{
	int variation_max = image_layer == -1 ? VariationMax : this->LayerVarInfo[image_layer].size();
	for (int i = 0; i < variation_max; ++i) {
		VariationInfo *varinfo = image_layer == -1 ? this->VarInfo[i] : this->LayerVarInfo[image_layer][i];
		if (!varinfo) {
			break;
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
	return NULL;
}

VariationInfo *CUnitType::GetVariation(std::string variation_name, int image_layer) const
{
	int variation_max = image_layer == -1 ? VariationMax : this->LayerVarInfo[image_layer].size();
	for (int i = 0; i < variation_max; ++i) {
		VariationInfo *varinfo = image_layer == -1 ? this->VarInfo[i] : this->LayerVarInfo[image_layer][i];
		if (!varinfo) {
			break;
		}
		if (varinfo->VariationId == variation_name) {
			return varinfo;
		}
	}
	return NULL;
}

std::string CUnitType::GetRandomVariationIdent(int image_layer) const
{
	std::vector<std::string> variation_idents;
	int variation_max = image_layer == -1 ? VariationMax : this->LayerVarInfo[image_layer].size();
	for (int i = 0; i < variation_max; ++i) {
		VariationInfo *varinfo = image_layer == -1 ? this->VarInfo[i] : this->LayerVarInfo[image_layer][i];
		if (!varinfo) {
			break;
		}
		variation_idents.push_back(varinfo->VariationId);
	}
	
	if (variation_idents.size() > 0) {
		return variation_idents[SyncRand(variation_idents.size())];
	}
	
	return "";
}

std::string CUnitType::GetDefaultName(CPlayer &player) const
{
	VariationInfo *varinfo = this->GetDefaultVariation(player);
	if (varinfo && !varinfo->TypeName.empty()) {
		return varinfo->TypeName;
	} else {
		return this->Name;
	}
}

CPlayerColorGraphic *CUnitType::GetDefaultLayerSprite(CPlayer &player, int image_layer) const
{
	VariationInfo *varinfo = this->GetDefaultVariation(player);
	if (this->LayerVarInfo[image_layer].size() > 0 && this->GetDefaultVariation(player, image_layer)->Sprite) {
		return this->GetDefaultVariation(player, image_layer)->Sprite;
	} else if (varinfo && varinfo->LayerSprites[image_layer]) {
		return varinfo->LayerSprites[image_layer];
	} else if (this->LayerSprites[image_layer])  {
		return this->LayerSprites[image_layer];
	} else {
		return NULL;
	}
}

int CUnitType::GetDefaultHairColor(CPlayer &player) const
{
	VariationInfo *varinfo = this->GetDefaultVariation(player);
	VariationInfo *hair_varinfo = this->GetDefaultVariation(player, HairImageLayer);
	if (hair_varinfo && hair_varinfo->HairColor != 0) {
		return hair_varinfo->HairColor;
	} else if (varinfo && varinfo->HairColor != 0) {
		return varinfo->HairColor;
	} else {
		return this->HairColor;
	}
}

bool CUnitType::CanExperienceUpgradeTo(CUnitType *type) const
{
	if (((int) AiHelpers.ExperienceUpgrades.size()) > this->Slot) {
		for (size_t i = 0; i != AiHelpers.ExperienceUpgrades[this->Slot].size(); ++i) {
			if (type == AiHelpers.ExperienceUpgrades[this->Slot][i] || AiHelpers.ExperienceUpgrades[this->Slot][i]->CanExperienceUpgradeTo(type)) {
				return true;
			}
		}
	}
	
	return false;
}

std::string CUnitType::GetNamePlural() const
{
	return GetPluralForm(this->Name);
}

std::string CUnitType::GeneratePersonalName(CFaction *faction, int gender) const
{
	if (Editor.Running == EditorEditing) { // don't set the personal name if in the editor
		return "";
	}
	
	std::vector<std::string> potential_names;
	
	if (this->PersonalNames.find(NoGender) != this->PersonalNames.end()) {
		for (size_t i = 0; i < this->PersonalNames.find(NoGender)->second.size(); ++i) {
			potential_names.push_back(this->PersonalNames.find(NoGender)->second[i]);
		}
	}
	if (gender != -1 && gender != NoGender && this->PersonalNames.find(gender) != this->PersonalNames.end()) {
		for (size_t i = 0; i < this->PersonalNames.find(gender)->second.size(); ++i) {
			potential_names.push_back(this->PersonalNames.find(gender)->second[i]);
		}
	}
	
	if (potential_names.size() == 0 && this->Civilization != -1) {
		int civilization_id = this->Civilization;
		if (civilization_id != -1) {
			if (faction && civilization_id != faction->Civilization && PlayerRaces.Species[civilization_id] == PlayerRaces.Species[faction->Civilization]) {
				civilization_id = faction->Civilization;
			}
			CCivilization *civilization = PlayerRaces.Civilizations[civilization_id];
			if (faction && faction->Civilization != civilization_id) {
				faction = NULL;
			}
			if (this->Faction != -1 && !faction) {
				faction = PlayerRaces.Factions[civilization_id][this->Faction];
			}
			
			if (this->BoolFlag[ORGANIC_INDEX].value) {
				if (civilization->GetPersonalNames().find(NoGender) != civilization->GetPersonalNames().end()) {
					for (size_t i = 0; i < civilization->GetPersonalNames().find(NoGender)->second.size(); ++i) {
						potential_names.push_back(civilization->GetPersonalNames().find(NoGender)->second[i]);
					}
				}
				if (gender != -1 && gender != NoGender && civilization->GetPersonalNames().find(gender) != civilization->GetPersonalNames().end()) {
					for (size_t i = 0; i < civilization->GetPersonalNames().find(gender)->second.size(); ++i) {
						potential_names.push_back(civilization->GetPersonalNames().find(gender)->second[i]);
					}
				}
			} else {
				if (this->Class != -1 && civilization->GetUnitClassNames(this->Class).size() > 0) {
					return civilization->GetUnitClassNames(this->Class)[SyncRand(civilization->GetUnitClassNames(this->Class).size())];
				}
				
				if (this->UnitType == UnitTypeNaval) { // if is a ship
					if (faction && faction->GetShipNames().size() > 0) {
						return faction->GetShipNames()[SyncRand(faction->GetShipNames().size())];
					}
					
					if (civilization->GetShipNames().size() > 0) {
						return civilization->GetShipNames()[SyncRand(civilization->GetShipNames().size())];
					}
				}
			}
		}
	}
	
	if (potential_names.size() > 0) {
		return potential_names[SyncRand(potential_names.size())];
	}

	return "";
}
//Wyrmgus end

void UpdateUnitStats(CUnitType &type, int reset)
{
	if (reset) {
		type.MapDefaultStat = type.DefaultStat;
		for (std::map<std::string, CUnitStats>::iterator iterator = type.ModDefaultStats.begin(); iterator != type.ModDefaultStats.end(); ++iterator) {
			for (size_t i = 0; i < UnitTypeVar.GetNumberVariable(); ++i) {
				type.MapDefaultStat.Variables[i].Value += iterator->second.Variables[i].Value;
				type.MapDefaultStat.Variables[i].Max += iterator->second.Variables[i].Max;
				type.MapDefaultStat.Variables[i].Increase += iterator->second.Variables[i].Increase;
				if (iterator->second.Variables[i].Enable != 0) {
					type.MapDefaultStat.Variables[i].Enable = iterator->second.Variables[i].Enable;
				}
			}
			for (size_t i = 0; i < UnitTypes.size(); ++i) {
				type.MapDefaultStat.UnitStock[i] += iterator->second.UnitStock[i];
			}
			for (int i = 0; i < MaxCosts; ++i) {
				type.MapDefaultStat.Costs[i] += iterator->second.Costs[i];
				type.MapDefaultStat.ImproveIncomes[i] += iterator->second.ImproveIncomes[i];
				type.MapDefaultStat.ResourceDemand[i] += iterator->second.ResourceDemand[i];
			}
		}
		for (int player = 0; player < PlayerMax; ++player) {
			type.Stats[player] = type.MapDefaultStat;
		}
		
		type.MapSound = type.Sound;
		for (std::map<std::string, CUnitSound>::iterator iterator = type.ModSounds.begin(); iterator != type.ModSounds.end(); ++iterator) {
			if (!iterator->second.Selected.Name.empty()) {
				type.MapSound.Selected = iterator->second.Selected;
			}
			if (!iterator->second.Acknowledgement.Name.empty()) {
				type.MapSound.Acknowledgement = iterator->second.Acknowledgement;
			}
			if (!iterator->second.Attack.Name.empty()) {
				type.MapSound.Attack = iterator->second.Attack;
			}
			if (!iterator->second.Idle.Name.empty()) {
				type.MapSound.Idle = iterator->second.Idle;
			}
			if (!iterator->second.Hit.Name.empty()) {
				type.MapSound.Hit = iterator->second.Hit;
			}
			if (!iterator->second.Miss.Name.empty()) {
				type.MapSound.Miss = iterator->second.Miss;
			}
			if (!iterator->second.Step.Name.empty()) {
				type.MapSound.Step = iterator->second.Step;
			}
			if (!iterator->second.StepDirt.Name.empty()) {
				type.MapSound.StepDirt = iterator->second.StepDirt;
			}
			if (!iterator->second.StepGrass.Name.empty()) {
				type.MapSound.StepGrass = iterator->second.StepGrass;
			}
			if (!iterator->second.StepGravel.Name.empty()) {
				type.MapSound.StepGravel = iterator->second.StepGravel;
			}
			if (!iterator->second.StepMud.Name.empty()) {
				type.MapSound.StepMud = iterator->second.StepMud;
			}
			if (!iterator->second.StepStone.Name.empty()) {
				type.MapSound.StepStone = iterator->second.StepStone;
			}
			if (!iterator->second.Used.Name.empty()) {
				type.MapSound.Used = iterator->second.Used;
			}
			if (!iterator->second.Build.Name.empty()) {
				type.MapSound.Build = iterator->second.Build;
			}
			if (!iterator->second.Ready.Name.empty()) {
				type.MapSound.Ready = iterator->second.Ready;
			}
			if (!iterator->second.Repair.Name.empty()) {
				type.MapSound.Repair = iterator->second.Repair;
			}
			for (unsigned int j = 0; j < MaxCosts; ++j) {
				if (!iterator->second.Harvest[j].Name.empty()) {
					type.MapSound.Harvest[j] = iterator->second.Harvest[j];
				}
			}
			if (!iterator->second.Help.Name.empty()) {
				type.MapSound.Help = iterator->second.Help;
			}
			if (!iterator->second.Dead[ANIMATIONS_DEATHTYPES].Name.empty()) {
				type.MapSound.Dead[ANIMATIONS_DEATHTYPES] = iterator->second.Dead[ANIMATIONS_DEATHTYPES];
			}
			int death;
			for (death = 0; death < ANIMATIONS_DEATHTYPES; ++death) {
				if (!iterator->second.Dead[death].Name.empty()) {
					type.MapSound.Dead[death] = iterator->second.Dead[death];
				}
			}
		}
	}

	// Non-solid units can always be entered and they don't block anything
	if (type.BoolFlag[NONSOLID_INDEX].value) {
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
			
			if (type.BoolFlag[RAIL_INDEX].value) { //rail units can only move over railroads
				type.MovementMask |= MapFieldNoRail;
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
				type.MovementMask =
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
	if (type.Building || type.BoolFlag[SHOREBUILDING_INDEX].value) {
		// Shore building is something special.
		if (type.BoolFlag[SHOREBUILDING_INDEX].value) {
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
		//Wyrmgus start
		type.MovementMask |= MapFieldItem;
		if (type.TerrainType) {
			if ((type.TerrainType->Flags & MapFieldRailroad) || (type.TerrainType->Flags & MapFieldRoad)) {
				type.MovementMask |= MapFieldRailroad;
			}
			if (type.TerrainType->Flags & MapFieldRoad) {
				type.MovementMask |= MapFieldRoad;
			}
		}
		if (type.BoolFlag[AIRUNPASSABLE_INDEX].value) { // for air unpassable units (i.e. doors)
			type.FieldFlags |= MapFieldUnpassable;
			type.FieldFlags |= MapFieldAirUnpassable;
		}		
		//Wyrmgus end
		//
		// A little chaos, buildings without HP can be entered.
		// The oil-patch is a very special case.
		//
		if (type.MapDefaultStat.Variables[HP_INDEX].Max) {
			type.FieldFlags = MapFieldBuilding;
		} else {
			type.FieldFlags = MapFieldNoBuilding;
		}
	//Wyrmgus start
	} else if (type.BoolFlag[ITEM_INDEX].value || type.BoolFlag[POWERUP_INDEX].value || type.BoolFlag[TRAP_INDEX].value) {
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
					type.FieldFlags |= MapFieldUnpassable;
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
					type.FieldFlags |= MapFieldUnpassable;
					type.FieldFlags |= MapFieldAirUnpassable;
				}
				break;
			//Wyrmgus end
			case UnitTypeNaval: // on water
				type.FieldFlags = MapFieldSeaUnit;
				//Wyrmgus start
				if (type.BoolFlag[AIRUNPASSABLE_INDEX].value) { // for air unpassable units (i.e. doors)
					type.FieldFlags |= MapFieldUnpassable;
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
	file.printf("},\n\"improve-production\", {");
	for (unsigned int i = 0; i < MaxCosts; ++i) {
		if (i) {
			file.printf(" ");
		}
		file.printf("\"%s\", %d,", DefaultResourceNames[i].c_str(), stats.ImproveIncomes[i]);
	}
	//Wyrmgus start
	file.printf("},\n\"resource-demand\", {");
	for (unsigned int i = 0; i < MaxCosts; ++i) {
		if (i) {
			file.printf(" ");
		}
		file.printf("\"%s\", %d,", DefaultResourceNames[i].c_str(), stats.ResourceDemand[i]);
	}
	file.printf("},\n\"unit-stock\", {");
	for (size_t i = 0; i < UnitTypes.size(); ++i) {
		if (stats.UnitStock[i] == type.DefaultStat.UnitStock[i]) {
			continue;
		}
		if (i) {
			file.printf(" ");
		}
		file.printf("\"%s\", %d,", UnitTypes[i]->Ident.c_str(), stats.UnitStock[i]);
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
//Wyrmgus start
//void DrawUnitType(const CUnitType &type, CPlayerColorGraphic *sprite, int player, int frame, const PixelPos &screenPos)
void DrawUnitType(const CUnitType &type, CPlayerColorGraphic *sprite, int player, int frame, const PixelPos &screenPos, int hair_color)
//Wyrmgus end
{
	//Wyrmgus start
	if (sprite == NULL) {
		return;
	}
	//Wyrmgus end
	
	PixelPos pos = screenPos;
	// FIXME: move this calculation to high level.
	//Wyrmgus start
	pos -= (Vec2i(sprite->Width, sprite->Height) - type.TileSize * PixelTileSize) / 2;
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
				sprite->DrawPlayerColorFrameClipTransX(player, -frame - 1, pos.x, pos.y, int(256 - 2.56 * type.Stats[player].Variables[TRANSPARENCY_INDEX].Value), false, hair_color);
			} else {
				sprite->DrawPlayerColorFrameClipX(player, -frame - 1, pos.x, pos.y, false, hair_color);
			}
		} else {
			if (type.Stats[player].Variables[TRANSPARENCY_INDEX].Value > 0) {
				sprite->DrawPlayerColorFrameClipTrans(player, frame, pos.x, pos.y, int(256 - 2.56 * type.Stats[player].Variables[TRANSPARENCY_INDEX].Value), false, hair_color);
			} else {
				sprite->DrawPlayerColorFrameClip(player, frame, pos.x, pos.y, false, hair_color);
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
			sprite->DrawPlayerColorFrameClipTrans(player, frame, pos.x, pos.y, int(256 - 2.56 * type.Stats[player].Variables[TRANSPARENCY_INDEX].Value), false, hair_color);
		} else {
			sprite->DrawPlayerColorFrameClip(player, frame, pos.x, pos.y, false, hair_color);
		}
	}
	//Wyrmgus end
}

/**
**  Get the still animation frame
*/
static int GetStillFrame(const CUnitType &type)
{
	//Wyrmgus start
	if (type.Animations == NULL) {
		return 0;
	}
	//Wyrmgus end
	
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
		
		//Wyrmgus start
		/*
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
		*/
		InitUnitType(type);
		//Wyrmgus end
	}

	// LUDO : called after game is loaded -> don't reset stats !
	UpdateStats(reset_player_stats); // Calculate the stats
}

//Wyrmgus start
void InitUnitType(CUnitType &type)
{
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
//Wyrmgus end

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
		if (type.ShadowSprite->Surface->format->BytesPerPixel == 1) {
			//Wyrmgus start
//			type.ShadowSprite->MakeShadow();
			//Wyrmgus end
		}
	}

	if (type.BoolFlag[HARVESTER_INDEX].value) {
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
	for (int i = 0; i < MaxImageLayers; ++i) {
		if (!type.LayerFiles[i].empty()) {
			type.LayerSprites[i] = CPlayerColorGraphic::New(type.LayerFiles[i], type.Width, type.Height);
			type.LayerSprites[i]->Load();
			if (type.Flip) {
				type.LayerSprites[i]->Flip();
			}
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
		for (int j = 0; j < MaxImageLayers; ++j) {
			if (!varinfo->LayerFiles[j].empty()) {
				varinfo->LayerSprites[j] = CPlayerColorGraphic::New(varinfo->LayerFiles[j], frame_width, frame_height);
				varinfo->LayerSprites[j]->Load();
				if (type.Flip) {
					varinfo->LayerSprites[j]->Flip();
				}
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
	
	for (int i = 0; i < MaxImageLayers; ++i) {
		for (size_t j = 0; j < type.LayerVarInfo[i].size(); ++j) {
			VariationInfo *varinfo = type.LayerVarInfo[i][j];
			if (!varinfo->File.empty()) {
				varinfo->Sprite = CPlayerColorGraphic::New(varinfo->File, type.Width, type.Height);
				varinfo->Sprite->Load();
				if (type.Flip) {
					varinfo->Sprite->Flip();
				}
			}
		}
	}
	//Wyrmgus end
}


/**
** Return the amount of unit-types.
*/
int GetUnitTypesCount()
{
	int count = 0;
	for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
		CUnitType &type = *UnitTypes[i];

		if (type.Missile.IsEmpty() == false) count++;
		if (type.FireMissile.IsEmpty() == false) count++;
		if (type.Explosion.IsEmpty() == false) count++;


		if (!type.Sprite) {
			count++;
		}
	}
	return count;
}

/**
** Load the graphics for the unit-types.
*/
void LoadUnitTypes()
{
	for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
		CUnitType &type = *UnitTypes[i];

		//Wyrmgus start
		/*
		// Lookup icons.
		type.Icon.Load();

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
			ShowLoadProgress(_("Loading Unit \"%s\""), type.Name.c_str());
			LoadUnitTypeSprite(type);

			IncItemsLoaded();
		}
#endif
		// FIXME: should i copy the animations of same graphics?
		*/
		LoadUnitType(type);
		//Wyrmgus end
	}
}

//Wyrmgus start
void LoadUnitType(CUnitType &type)
{
	// Lookup icons.
	if (!type.Icon.Name.empty()) {
		type.Icon.Load();
	}

	for (int j = 0; j < VariationMax; ++j) {
		VariationInfo *varinfo = type.VarInfo[j];
		if (!varinfo) {
			continue;
		}
		if (!varinfo->Icon.Name.empty()) {
			varinfo->Icon.Load();
		}
	}

	// Lookup missiles.
	type.Missile.MapMissile();
	//Wyrmgus start
	type.FireMissile.MapMissile();
	//Wyrmgus end
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
		ShowLoadProgress(_("Loading Unit \"%s\""), type.Name.c_str());
		LoadUnitTypeSprite(type);

		IncItemsLoaded();
	}
#endif
	// FIXME: should i copy the animations of same graphics?
}
//Wyrmgus end

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
	//Wyrmgus start
//	UnitTypeHumanWall = NULL;
//	UnitTypeOrcWall = NULL;
	//Wyrmgus end
	
	//Wyrmgus start
	for (size_t i = 0; i < Species.size(); ++i) {
		delete Species[i];
	}
	Species.clear();
	for (size_t i = 0; i < SpeciesGenuses.size(); ++i) {
		delete SpeciesGenuses[i];
	}
	SpeciesGenuses.clear();
	for (size_t i = 0; i < SpeciesFamilies.size(); ++i) {
		delete SpeciesFamilies[i];
	}
	SpeciesFamilies.clear();
	for (size_t i = 0; i < SpeciesOrders.size(); ++i) {
		delete SpeciesOrders[i];
	}
	SpeciesOrders.clear();
	for (size_t i = 0; i < SpeciesClasses.size(); ++i) {
		delete SpeciesClasses[i];
	}
	SpeciesClasses.clear();
	for (size_t i = 0; i < SpeciesPhylums.size(); ++i) {
		delete SpeciesPhylums[i];
	}
	SpeciesPhylums.clear();
	//Wyrmgus end
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
	for (int i = 0; i < MaxImageLayers; ++i) {
		if (this->LayerSprites[i]) {
			CGraphic::Free(this->LayerSprites[i]);
		}
	}
	for (int res = 0; res < MaxCosts; ++res) {
		if (this->SpriteWhenLoaded[res]) {
			CGraphic::Free(this->SpriteWhenLoaded[res]);
		}
		if (this->SpriteWhenEmpty[res]) {
			CGraphic::Free(this->SpriteWhenEmpty[res]);
		}
	}
}

CSpecies *GetSpecies(std::string species_ident)
{
	for (size_t i = 0; i < Species.size(); ++i) {
		if (species_ident == Species[i]->Ident) {
			return Species[i];
		}
	}
	
	return NULL;
}

CSpeciesGenus *GetSpeciesGenus(std::string genus_ident)
{
	for (size_t i = 0; i < SpeciesGenuses.size(); ++i) {
		if (genus_ident == SpeciesGenuses[i]->Ident) {
			return SpeciesGenuses[i];
		}
	}
	
	return NULL;
}

CSpeciesFamily *GetSpeciesFamily(std::string family_ident)
{
	for (size_t i = 0; i < SpeciesFamilies.size(); ++i) {
		if (family_ident == SpeciesFamilies[i]->Ident) {
			return SpeciesFamilies[i];
		}
	}
	
	return NULL;
}

CSpeciesOrder *GetSpeciesOrder(std::string order_ident)
{
	for (size_t i = 0; i < SpeciesOrders.size(); ++i) {
		if (order_ident == SpeciesOrders[i]->Ident) {
			return SpeciesOrders[i];
		}
	}
	
	return NULL;
}

CSpeciesClass *GetSpeciesClass(std::string class_ident)
{
	for (size_t i = 0; i < SpeciesClasses.size(); ++i) {
		if (class_ident == SpeciesClasses[i]->Ident) {
			return SpeciesClasses[i];
		}
	}
	
	return NULL;
}

CSpeciesPhylum *GetSpeciesPhylum(std::string phylum_ident)
{
	for (size_t i = 0; i < SpeciesPhylums.size(); ++i) {
		if (phylum_ident == SpeciesPhylums[i]->Ident) {
			return SpeciesPhylums[i];
		}
	}
	
	return NULL;
}

bool CSpecies::CanEvolveToAUnitType(CTerrainType *terrain, bool sapient_only)
{
	for (size_t i = 0; i < this->EvolvesTo.size(); ++i) {
		if (
			(this->EvolvesTo[i]->Type != NULL && (!terrain || std::find(this->EvolvesTo[i]->Terrains.begin(), this->EvolvesTo[i]->Terrains.end(), terrain) != this->EvolvesTo[i]->Terrains.end()) && (!sapient_only || this->EvolvesTo[i]->Sapient))
			|| this->EvolvesTo[i]->CanEvolveToAUnitType(terrain, sapient_only)
		) {
			return true;
		}
	}
	return false;
}

CSpecies *CSpecies::GetRandomEvolution(CTerrainType *terrain)
{
	std::vector<CSpecies *> potential_evolutions;
	
	for (size_t i = 0; i < this->EvolvesTo.size(); ++i) {
		if (
			(this->EvolvesTo[i]->Type != NULL && std::find(this->EvolvesTo[i]->Terrains.begin(), this->EvolvesTo[i]->Terrains.end(), terrain) != this->EvolvesTo[i]->Terrains.end())
			|| this->EvolvesTo[i]->CanEvolveToAUnitType(terrain)
		) { //give preference to evolutions that are native to the current terrain
			potential_evolutions.push_back(this->EvolvesTo[i]);
		}
	}
	
	if (potential_evolutions.size() == 0) {
		for (size_t i = 0; i < this->EvolvesTo.size(); ++i) {
			if (this->EvolvesTo[i]->Type != NULL || this->EvolvesTo[i]->CanEvolveToAUnitType()) {
				potential_evolutions.push_back(this->EvolvesTo[i]);
			}
		}
	}
	
	if (potential_evolutions.size() > 0) {
		return potential_evolutions[SyncRand(potential_evolutions.size())];
	}
	
	return NULL;
}

std::string GetImageLayerNameById(int image_layer)
{
	if (image_layer == LeftArmImageLayer) {
		return "left-arm";
	} else if (image_layer == RightArmImageLayer) {
		return "right-arm";
	} else if (image_layer == RightHandImageLayer) {
		return "right-hand";
	} else if (image_layer == HairImageLayer) {
		return "hair";
	} else if (image_layer == ClothingImageLayer) {
		return "clothing";
	} else if (image_layer == ClothingLeftArmImageLayer) {
		return "clothing-left-arm";
	} else if (image_layer == ClothingRightArmImageLayer) {
		return "clothing-right-arm";
	} else if (image_layer == PantsImageLayer) {
		return "pants";
	} else if (image_layer == BootsImageLayer) {
		return "boots";
	} else if (image_layer == WeaponImageLayer) {
		return "weapon";
	} else if (image_layer == ShieldImageLayer) {
		return "shield";
	} else if (image_layer == HelmetImageLayer) {
		return "helmet";
	} else if (image_layer == BackpackImageLayer) {
		return "backpack";
	} else if (image_layer == MountImageLayer) {
		return "mount";
	}

	return "";
}

int GetImageLayerIdByName(std::string image_layer)
{
	if (image_layer == "left-arm") {
		return LeftArmImageLayer;
	} else if (image_layer == "right-arm") {
		return RightArmImageLayer;
	} else if (image_layer == "right-hand") {
		return RightHandImageLayer;
	} else if (image_layer == "hair") {
		return HairImageLayer;
	} else if (image_layer == "clothing") {
		return ClothingImageLayer;
	} else if (image_layer == "clothing-left-arm") {
		return ClothingLeftArmImageLayer;
	} else if (image_layer == "clothing-right-arm") {
		return ClothingRightArmImageLayer;
	} else if (image_layer == "pants") {
		return PantsImageLayer;
	} else if (image_layer == "boots") {
		return BootsImageLayer;
	} else if (image_layer == "weapon") {
		return WeaponImageLayer;
	} else if (image_layer == "shield") {
		return ShieldImageLayer;
	} else if (image_layer == "helmet") {
		return HelmetImageLayer;
	} else if (image_layer == "backpack") {
		return BackpackImageLayer;
	} else if (image_layer == "mount") {
		return MountImageLayer;
	}

	return -1;
}
//Wyrmgus end

//@}
