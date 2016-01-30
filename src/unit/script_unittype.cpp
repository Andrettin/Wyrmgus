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
/**@name script_unittype.cpp - The unit-type ccl functions. */
//
//      (c) Copyright 1999-2015 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "actions.h"
#include "animation.h"
//Wyrmgus start
#include "character.h" //for updating levels
//Wyrmgus end
#include "construct.h"
#include "editor.h"
#include "font.h"
#include "luacallback.h"
#include "map.h"
//Wyrmgus start
#include "network.h" //for updating levels
//Wyrmgus end
#include "player.h"
#include "script.h"
#include "sound.h"
#include "spells.h"
#include "ui.h"
#include "unit.h"
#include "unitsound.h"
#include "unit_manager.h"
//Wyrmgus start
#include "upgrade.h"
//Wyrmgus end
#include "video.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

CUnitTypeVar UnitTypeVar;    /// Variables for UnitType and unit.

// names of boolflags
static const char COWARD_KEY[] = "Coward";
static const char BUILDING_KEY[] = "Building";
static const char FLIP_KEY[] = "Flip";
static const char REVEALER_KEY[] = "Revealer";
static const char LANDUNIT_KEY[] = "LandUnit";
static const char AIRUNIT_KEY[] = "AirUnit";
static const char SEAUNIT_KEY[] = "SeaUnit";
static const char EXPLODEWHENKILLED_KEY[] = "ExplodeWhenKilled";
static const char VISIBLEUNDERFOG_KEY[] = "VisibleUnderFog";
static const char PERMANENTCLOAK_KEY[] = "PermanentCloak";
static const char DETECTCLOAK_KEY[] = "DetectCloak";
static const char ATTACKFROMTRANSPORTER_KEY[] = "AttackFromTransporter";
static const char VANISHES_KEY[] = "Vanishes";
static const char GROUNDATTACK_KEY[] = "GroundAttack";
static const char SHOREBUILDING_KEY[] = "ShoreBuilding";
static const char CANATTACK_KEY[] = "CanAttack";
//Wyrmgus start
static const char CANDOCK_KEY[] = "CanDock";
//Wyrmgus end
static const char BUILDEROUTSIDE_KEY[] = "BuilderOutside";
static const char BUILDERLOST_KEY[] = "BuilderLost";
static const char CANHARVEST_KEY[] = "CanHarvest";
static const char HARVESTER_KEY[] = "Harvester";
static const char SELECTABLEBYRECTANGLE_KEY[] = "SelectableByRectangle";
static const char ISNOTSELECTABLE_KEY[] = "IsNotSelectable";
static const char DECORATION_KEY[] = "Decoration";
static const char INDESTRUCTIBLE_KEY[] = "Indestructible";
static const char TELEPORTER_KEY[] = "Teleporter";
static const char SHIELDPIERCE_KEY[] = "ShieldPiercing";
static const char SAVECARGO_KEY[] = "SaveCargo";
static const char NONSOLID_KEY[] = "NonSolid";
static const char WALL_KEY[] = "Wall";
static const char NORANDOMPLACING_KEY[] = "NoRandomPlacing";
static const char ORGANIC_KEY[] = "organic";
static const char SIDEATTACK_KEY[] = "SideAttack";
static const char NOFRIENDLYFIRE_KEY[] = "NoFriendlyFire";
//Wyrmgus start
static const char ITEM_KEY[] = "Item";
static const char POWERUP_KEY[] = "PowerUp";
static const char INVENTORY_KEY[] = "Inventory";
static const char TRAP_KEY[] = "Trap";
static const char BRIDGE_KEY[] = "Bridge";
static const char MERCENARY_KEY[] = "Mercenary";
static const char FAUNA_KEY[] = "Fauna";
static const char PREDATOR_KEY[] = "Predator";
static const char SLIME_KEY[] = "Slime";
static const char PEOPLEAVERSION_KEY[] = "PeopleAversion";
static const char MOUNTED_KEY[] = "Mounted";
static const char DIMINUTIVE_KEY[] = "Diminutive";
static const char GIANT_KEY[] = "Giant";
static const char DRAGON_KEY[] = "Dragon";
static const char DETRITUS_KEY[] = "Detritus";
static const char FLESH_KEY[] = "Flesh";
static const char VEGETABLE_KEY[] = "Vegetable";
static const char INSECT_KEY[] = "Insect";
static const char DAIRY_KEY[] = "Dairy";
static const char DETRITIVORE_KEY[] = "Detritivore";
static const char CARNIVORE_KEY[] = "Carnivore";
static const char HERBIVORE_KEY[] = "Herbivore";
static const char INSECTIVORE_KEY[] = "Insectivore";
static const char HARVESTFROMOUTSIDE_KEY[] = "HarvestFromOutside";
static const char OBSTACLE_KEY[] = "Obstacle";
static const char AIRUNPASSABLE_KEY[] = "AirUnpassable";
static const char SLOWS_KEY[] = "Slows";
static const char GRAVEL_KEY[] = "Gravel";
static const char HACKDAMAGE_KEY[] = "HackDamage";
static const char PIERCEDAMAGE_KEY[] = "PierceDamage";
static const char BLUNTDAMAGE_KEY[] = "BluntDamage";
//Wyrmgus end

// names of the variable.
static const char HITPOINTS_KEY[] = "HitPoints";
static const char BUILD_KEY[] = "Build";
static const char MANA_KEY[] = "Mana";
static const char TRANSPORT_KEY[] = "Transport";
static const char RESEARCH_KEY[] = "Research";
static const char TRAINING_KEY[] = "Training";
static const char UPGRADETO_KEY[] = "UpgradeTo";
static const char GIVERESOURCE_KEY[] = "GiveResource";
static const char CARRYRESOURCE_KEY[] = "CarryResource";
static const char XP_KEY[] = "Xp";
static const char KILL_KEY[] = "Kill";
static const char SUPPLY_KEY[] = "Supply";
static const char DEMAND_KEY[] = "Demand";
static const char ARMOR_KEY[] = "Armor";
static const char SIGHTRANGE_KEY[] = "SightRange";
static const char ATTACKRANGE_KEY[] = "AttackRange";
static const char PIERCINGDAMAGE_KEY[] = "PiercingDamage";
static const char BASICDAMAGE_KEY[] = "BasicDamage";
//Wyrmgus start
static const char THORNSDAMAGE_KEY[] = "ThornsDamage";
static const char FIREDAMAGE_KEY[] = "FireDamage";
static const char COLDDAMAGE_KEY[] = "ColdDamage";
static const char ARCANEDAMAGE_KEY[] = "ArcaneDamage";
static const char LIGHTNINGDAMAGE_KEY[] = "LightningDamage";
static const char AIRDAMAGE_KEY[] = "AirDamage";
static const char EARTHDAMAGE_KEY[] = "EarthDamage";
static const char WATERDAMAGE_KEY[] = "WaterDamage";
static const char SPEED_KEY[] = "Speed";
static const char FIRERESISTANCE_KEY[] = "FireResistance";
static const char COLDRESISTANCE_KEY[] = "ColdResistance";
static const char ARCANERESISTANCE_KEY[] = "ArcaneResistance";
static const char LIGHTNINGRESISTANCE_KEY[] = "LightningResistance";
static const char AIRRESISTANCE_KEY[] = "AirResistance";
static const char EARTHRESISTANCE_KEY[] = "EarthResistance";
static const char WATERRESISTANCE_KEY[] = "WaterResistance";
static const char HACKRESISTANCE_KEY[] = "HackResistance";
static const char PIERCERESISTANCE_KEY[] = "PierceResistance";
static const char BLUNTRESISTANCE_KEY[] = "BluntResistance";
//Wyrmgus end
static const char POSX_KEY[] = "PosX";
static const char POSY_KEY[] = "PosY";
static const char TARGETPOSX_KEY[] = "TargetPosX";
static const char TARGETPOSY_KEY[] = "TargetPosY";
static const char RADARRANGE_KEY[] = "RadarRange";
static const char RADARJAMMERRANGE_KEY[] = "RadarJammerRange";
static const char AUTOREPAIRRANGE_KEY[] = "AutoRepairRange";
static const char BLOODLUST_KEY[] = "Bloodlust";
static const char HASTE_KEY[] = "Haste";
static const char SLOW_KEY[] = "Slow";
static const char INVISIBLE_KEY[] = "Invisible";
static const char UNHOLYARMOR_KEY[] = "UnholyArmor";
static const char SLOT_KEY[] = "Slot";
static const char SHIELD_KEY[] = "ShieldPoints";
static const char POINTS_KEY[] = "Points";
static const char MAXHARVESTERS_KEY[] = "MaxHarvesters";
static const char POISON_KEY[] = "Poison";
static const char SHIELDPERMEABILITY_KEY[] = "ShieldPermeability";
static const char SHIELDPIERCING_KEY[] = "ShieldPiercing";
static const char ISALIVE_KEY[] = "IsAlive";
static const char PLAYER_KEY[] = "Player";
static const char PRIORITY_KEY[] = "Priority";
//Wyrmgus start
static const char STRENGTH_KEY[] = "Strength";
static const char DEXTERITY_KEY[] = "Dexterity";
static const char INTELLIGENCE_KEY[] = "Intelligence";
static const char ACCURACY_KEY[] = "Accuracy";
static const char EVASION_KEY[] = "Evasion";
static const char LEVEL_KEY[] = "Level";
static const char LEVELUP_KEY[] = "LevelUp";
static const char XPREQUIRED_KEY[] = "XPRequired";
static const char VARIATION_KEY[] = "Variation";
static const char HITPOINTHEALING_KEY[] = "HitPointHealing";
static const char HITPOINTBONUS_KEY[] = "HitPointBonus";
static const char CRITICALSTRIKECHANCE_KEY[] = "CriticalStrikeChance";
static const char BACKSTAB_KEY[] = "Backstab";
static const char BONUSAGAINSTMOUNTED_KEY[] = "BonusAgainstMounted";
static const char BONUSAGAINSTBUILDINGS_KEY[] = "BonusAgainstBuildings";
static const char BONUSAGAINSTAIR_KEY[] = "BonusAgainstAir";
static const char BONUSAGAINSTGIANTS_KEY[] = "BonusAgainstGiants";
static const char BONUSAGAINSTDRAGONS_KEY[] = "BonusAgainstDragons";
static const char DAYSIGHTRANGEBONUS_KEY[] = "DaySightRangeBonus";
static const char NIGHTSIGHTRANGEBONUS_KEY[] = "NightSightRangeBonus";
static const char TRANSPARENCY_KEY[] = "Transparency";
static const char GENDER_KEY[] = "Gender";
static const char BIRTHCYCLE_KEY[] = "BirthCycle";
static const char HUNGER_KEY[] = "Hunger";
static const char STUN_KEY[] = "Stun";
static const char BLEEDING_KEY[] = "Bleeding";
static const char TIMEEFFICIENCYBONUS_KEY[] = "TimeEfficiencyBonus";
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CUnitTypeVar::CBoolKeys::CBoolKeys()
{

	const char *const tmp[] = {COWARD_KEY, BUILDING_KEY, FLIP_KEY, REVEALER_KEY,
							   LANDUNIT_KEY, AIRUNIT_KEY, SEAUNIT_KEY, EXPLODEWHENKILLED_KEY,
							   VISIBLEUNDERFOG_KEY, PERMANENTCLOAK_KEY, DETECTCLOAK_KEY,
							   ATTACKFROMTRANSPORTER_KEY, VANISHES_KEY, GROUNDATTACK_KEY,
							   //Wyrmgus start
//							   SHOREBUILDING_KEY, CANATTACK_KEY, BUILDEROUTSIDE_KEY,
							   SHOREBUILDING_KEY, CANATTACK_KEY, CANDOCK_KEY, BUILDEROUTSIDE_KEY,
							   //Wyrmgus end
							   BUILDERLOST_KEY, CANHARVEST_KEY, HARVESTER_KEY, SELECTABLEBYRECTANGLE_KEY,
							   ISNOTSELECTABLE_KEY, DECORATION_KEY, INDESTRUCTIBLE_KEY, TELEPORTER_KEY, SHIELDPIERCE_KEY,
							   //Wyrmgus start
//							   SAVECARGO_KEY, NONSOLID_KEY, WALL_KEY, NORANDOMPLACING_KEY, ORGANIC_KEY
							   SAVECARGO_KEY, NONSOLID_KEY, WALL_KEY, NORANDOMPLACING_KEY, ORGANIC_KEY, SIDEATTACK_KEY, NOFRIENDLYFIRE_KEY,
							   ITEM_KEY, POWERUP_KEY, INVENTORY_KEY, TRAP_KEY, BRIDGE_KEY,
							   MERCENARY_KEY,
							   FAUNA_KEY, PREDATOR_KEY, SLIME_KEY, PEOPLEAVERSION_KEY, MOUNTED_KEY, DIMINUTIVE_KEY, GIANT_KEY, DRAGON_KEY,
							   DETRITUS_KEY, FLESH_KEY, VEGETABLE_KEY, INSECT_KEY, DAIRY_KEY,
							   DETRITIVORE_KEY, CARNIVORE_KEY, HERBIVORE_KEY, INSECTIVORE_KEY,
							   HARVESTFROMOUTSIDE_KEY, OBSTACLE_KEY, AIRUNPASSABLE_KEY, SLOWS_KEY, GRAVEL_KEY,
							   HACKDAMAGE_KEY, PIERCEDAMAGE_KEY, BLUNTDAMAGE_KEY
							   //Wyrmgus end
							  };

	for (int i = 0; i < NBARALREADYDEFINED; ++i) {
		buildin[i].offset = i;
		buildin[i].keylen = strlen(tmp[i]);
		buildin[i].key = tmp[i];
	}
	Init();
}

CUnitTypeVar::CVariableKeys::CVariableKeys()
{

	const char *const tmp[] = {HITPOINTS_KEY, BUILD_KEY, MANA_KEY, TRANSPORT_KEY,
							   RESEARCH_KEY, TRAINING_KEY, UPGRADETO_KEY, GIVERESOURCE_KEY,
							   CARRYRESOURCE_KEY, XP_KEY, KILL_KEY,	SUPPLY_KEY, DEMAND_KEY, ARMOR_KEY,
							   SIGHTRANGE_KEY, ATTACKRANGE_KEY, PIERCINGDAMAGE_KEY,
							   //Wyrmgus start
//							   BASICDAMAGE_KEY, POSX_KEY, POSY_KEY, TARGETPOSX_KEY, TARGETPOSY_KEY, RADARRANGE_KEY,
							   BASICDAMAGE_KEY, THORNSDAMAGE_KEY,
							   FIREDAMAGE_KEY, COLDDAMAGE_KEY, ARCANEDAMAGE_KEY, LIGHTNINGDAMAGE_KEY, AIRDAMAGE_KEY, EARTHDAMAGE_KEY, WATERDAMAGE_KEY,
							   SPEED_KEY,
							   FIRERESISTANCE_KEY, COLDRESISTANCE_KEY, ARCANERESISTANCE_KEY, LIGHTNINGRESISTANCE_KEY, AIRRESISTANCE_KEY, EARTHRESISTANCE_KEY, WATERRESISTANCE_KEY,
							   HACKRESISTANCE_KEY, PIERCERESISTANCE_KEY, BLUNTRESISTANCE_KEY,
							   POSX_KEY, POSY_KEY, TARGETPOSX_KEY, TARGETPOSY_KEY, RADARRANGE_KEY,
							   //Wyrmgus end
							   RADARJAMMERRANGE_KEY, AUTOREPAIRRANGE_KEY, BLOODLUST_KEY, HASTE_KEY,
							   SLOW_KEY, INVISIBLE_KEY, UNHOLYARMOR_KEY, SLOT_KEY, SHIELD_KEY, POINTS_KEY,
							   MAXHARVESTERS_KEY, POISON_KEY, SHIELDPERMEABILITY_KEY, SHIELDPIERCING_KEY, ISALIVE_KEY, PLAYER_KEY,
//Wyrmgus
//							   PRIORITY_KEY
							   PRIORITY_KEY,
							   STRENGTH_KEY, DEXTERITY_KEY, INTELLIGENCE_KEY,
							   ACCURACY_KEY, EVASION_KEY, LEVEL_KEY, LEVELUP_KEY, XPREQUIRED_KEY, VARIATION_KEY, HITPOINTHEALING_KEY, HITPOINTBONUS_KEY, CRITICALSTRIKECHANCE_KEY,
							   BACKSTAB_KEY, BONUSAGAINSTMOUNTED_KEY, BONUSAGAINSTBUILDINGS_KEY, BONUSAGAINSTAIR_KEY, BONUSAGAINSTGIANTS_KEY,
							   BONUSAGAINSTDRAGONS_KEY,
							   DAYSIGHTRANGEBONUS_KEY, NIGHTSIGHTRANGEBONUS_KEY, TRANSPARENCY_KEY, GENDER_KEY, BIRTHCYCLE_KEY, HUNGER_KEY,
							   STUN_KEY, BLEEDING_KEY, TIMEEFFICIENCYBONUS_KEY
//Wyrmgus end
							  };

	for (int i = 0; i < NVARALREADYDEFINED; ++i) {
		buildin[i].offset = i;
		buildin[i].keylen = strlen(tmp[i]);
		buildin[i].key = tmp[i];
	}
	Init();
}

int GetSpriteIndex(const char *SpriteName);

/**
**  Get the resource ID from a SCM object.
**
**  @param l  Lua state.
**
**  @return   the resource id
*/
unsigned CclGetResourceByName(lua_State *l)
{
	const char *const tmp = LuaToString(l, -1);
	const std::string value = tmp ? tmp : "";
	const int resId = GetResourceIdByName(l, value.c_str());

	return resId;
}


/**
**  Find the index of a extra death type
*/
int ExtraDeathIndex(const char *death)
{
	for (unsigned int det = 0; det < ANIMATIONS_DEATHTYPES; ++det) {
		if (!strcmp(death, ExtraDeathTypes[det].c_str())) {
			return det;
		}
	}
	return ANIMATIONS_DEATHTYPES;
}

/**
**  Parse BuildingRules
**
**  @param l      Lua state.
**  @param blist  BuildingRestriction to fill in
*/
static void ParseBuildingRules(lua_State *l, std::vector<CBuildRestriction *> &blist)
{
	CBuildRestrictionAnd *andlist = new CBuildRestrictionAnd();

	const int args = lua_rawlen(l, -1);
	Assert(!(args & 1)); // must be even

	for (int i = 0; i < args; ++i) {
		const char *value = LuaToString(l, -1, i + 1);
		++i;
		lua_rawgeti(l, -1, i + 1);
		if (!lua_istable(l, -1)) {
			LuaError(l, "incorrect argument");
		}
		if (!strcmp(value, "distance")) {
			CBuildRestrictionDistance *b = new CBuildRestrictionDistance;

			for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
				value = LuaToString(l, -2);
				if (!strcmp(value, "Distance")) {
					b->Distance = LuaToNumber(l, -1);
				} else if (!strcmp(value, "DistanceType")) {
					value = LuaToString(l, -1);
					if (value[0] == '=') {
						if ((value[1] == '=' && value[2] == '\0') || (value[1] == '\0')) {
							b->DistanceType = Equal;
						}
					} else if (value[0] == '>') {
						if (value[1] == '=' && value[2] == '\0') {
							b->DistanceType = GreaterThanEqual;
						} else if (value[1] == '\0') {
							b->DistanceType = GreaterThan;
						}
					} else if (value[0] == '<') {
						if (value[1] == '=' && value[2] == '\0') {
							b->DistanceType = LessThanEqual;
						} else if (value[1] == '\0') {
							b->DistanceType = LessThan;
						}
					} else if (value[0] == '!' && value[1] == '=' && value[2] == '\0') {
						b->DistanceType = NotEqual;
					}
				} else if (!strcmp(value, "Type")) {
					b->RestrictTypeName = LuaToString(l, -1);
				} else if (!strcmp(value, "Owner")) {
					b->RestrictTypeOwner = LuaToString(l, -1);
				} else if (!strcmp(value, "CheckBuilder")) {
					b->CheckBuilder = LuaToBoolean(l, -1);
				} else if (!strcmp(value, "Diagonal")) {
					b->Diagonal = LuaToBoolean(l, -1);
				} else {
					LuaError(l, "Unsupported BuildingRules distance tag: %s" _C_ value);
				}
			}
			andlist->push_back(b);
		} else if (!strcmp(value, "addon")) {
			CBuildRestrictionAddOn *b = new CBuildRestrictionAddOn;

			for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
				value = LuaToString(l, -2);
				if (!strcmp(value, "OffsetX")) {
					b->Offset.x = LuaToNumber(l, -1);
				} else if (!strcmp(value, "OffsetY")) {
					b->Offset.y = LuaToNumber(l, -1);
				} else if (!strcmp(value, "Type")) {
					b->ParentName = LuaToString(l, -1);
				} else {
					LuaError(l, "Unsupported BuildingRules addon tag: %s" _C_ value);
				}
			}
			andlist->push_back(b);
		} else if (!strcmp(value, "ontop")) {
			CBuildRestrictionOnTop *b = new CBuildRestrictionOnTop;

			for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
				value = LuaToString(l, -2);
				if (!strcmp(value, "Type")) {
					b->ParentName = LuaToString(l, -1);
				} else if (!strcmp(value, "ReplaceOnDie")) {
					b->ReplaceOnDie = LuaToBoolean(l, -1);
				} else if (!strcmp(value, "ReplaceOnBuild")) {
					b->ReplaceOnBuild = LuaToBoolean(l, -1);
				} else {
					LuaError(l, "Unsupported BuildingRules ontop tag: %s" _C_ value);
				}
			}
			andlist->push_back(b);
		} else if (!strcmp(value, "has-unit")) {
			CBuildRestrictionHasUnit *b = new CBuildRestrictionHasUnit;

			for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
				value = LuaToString(l, -2);
				if (!strcmp(value, "Type")) {
					b->RestrictTypeName = LuaToString(l, -1);
				} else if (!strcmp(value, "Owner")) {
					b->RestrictTypeOwner = LuaToString(l, -1);
				} else if (!strcmp(value, "Count")) {
					b->Count = LuaToNumber(l, -1);
				} else if (!strcmp(value, "CountType")) {
					value = LuaToString(l, -1);
					if (value[0] == '=') {
						if ((value[1] == '=' && value[2] == '\0') || (value[1] == '\0')) {
							b->CountType = Equal;
						}
					} else if (value[0] == '>') {
						if (value[1] == '=' && value[2] == '\0') {
							b->CountType = GreaterThanEqual;
						} else if (value[1] == '\0') {
							b->CountType = GreaterThan;
						}
					} else if (value[0] == '<') {
						if (value[1] == '=' && value[2] == '\0') {
							b->CountType = LessThanEqual;
						} else if (value[1] == '\0') {
							b->CountType = LessThan;
						}
					} else if (value[0] == '!' && value[1] == '=' && value[2] == '\0') {
						b->CountType = NotEqual;
					}
				} else {
					LuaError(l, "Unsupported BuildingRules has-unit tag: %s" _C_ value);
				}
			}
			andlist->push_back(b);
		} else if (!strcmp(value, "surrounded-by")) {
			CBuildRestrictionSurroundedBy *b = new CBuildRestrictionSurroundedBy;

			for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
				value = LuaToString(l, -2);
				if (!strcmp(value, "Type")) {
					b->RestrictTypeName = LuaToString(l, -1);
				} else if (!strcmp(value, "Count")) {
					b->Count = LuaToNumber(l, -1);
				} else if (!strcmp(value, "CountType")) {
					value = LuaToString(l, -1);
					if (value[0] == '=') {
						if ((value[1] == '=' && value[2] == '\0') || (value[1] == '\0')) {
							b->CountType = Equal;
						}
					} else if (value[0] == '>') {
						if (value[1] == '=' && value[2] == '\0') {
							b->CountType = GreaterThanEqual;
						} else if (value[1] == '\0') {
							b->CountType = GreaterThan;
						}
					} else if (value[0] == '<') {
						if (value[1] == '=' && value[2] == '\0') {
							b->CountType = LessThanEqual;
						} else if (value[1] == '\0') {
							b->CountType = LessThan;
						}
					} else if (value[0] == '!' && value[1] == '=' && value[2] == '\0') {
						b->CountType = NotEqual;
					}
				} else if (!strcmp(value, "Distance")) {
					b->Distance = LuaToNumber(l, -1);
				} else if (!strcmp(value, "DistanceType")) {
					value = LuaToString(l, -1);
					if (value[0] == '=') {
						if ((value[1] == '=' && value[2] == '\0') || (value[1] == '\0')) {
							b->DistanceType = Equal;
						}
					} else if (value[0] == '>') {
						if (value[1] == '=' && value[2] == '\0') {
							b->DistanceType = GreaterThanEqual;
						} else if (value[1] == '\0') {
							b->DistanceType = GreaterThan;
						}
					} else if (value[0] == '<') {
						if (value[1] == '=' && value[2] == '\0') {
							b->DistanceType = LessThanEqual;
						} else if (value[1] == '\0') {
							b->DistanceType = LessThan;
						}
					} else if (value[0] == '!' && value[1] == '=' && value[2] == '\0') {
						b->DistanceType = NotEqual;
					}
				} else if (!strcmp(value, "Owner")) {
					b->RestrictTypeOwner = LuaToString(l, -1);
				} else if (!strcmp(value, "CheckBuilder")) {
					b->CheckBuilder = LuaToBoolean(l, -1);
				} else {
					LuaError(l, "Unsupported BuildingRules surrounded-by tag: %s" _C_ value);
				}
			}
			andlist->push_back(b);
		} else {
			LuaError(l, "Unsupported BuildingRules tag: %s" _C_ value);
		}
		lua_pop(l, 1);
	}
	blist.push_back(andlist);
}

static void UpdateDefaultBoolFlags(CUnitType &type)
{
	// BoolFlag
	type.BoolFlag[BUILDING_INDEX].value              = type.Building;
	type.BoolFlag[FLIP_INDEX].value                  = type.Flip;
	type.BoolFlag[LANDUNIT_INDEX].value              = type.LandUnit;
	type.BoolFlag[AIRUNIT_INDEX].value               = type.AirUnit;
	type.BoolFlag[SEAUNIT_INDEX].value               = type.SeaUnit;
	type.BoolFlag[EXPLODEWHENKILLED_INDEX].value     = type.ExplodeWhenKilled;
	type.BoolFlag[CANATTACK_INDEX].value             = type.CanAttack;
}

/**
**  Parse unit-type.
**
**  @param l  Lua state.
*/
static int CclDefineUnitType(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument");
	}

	// Slot identifier
	const char *str = LuaToString(l, 1);
	CUnitType *type = UnitTypeByIdent(str);
	int redefine;
	if (type) {
		redefine = 1;
	} else {
		type = NewUnitTypeSlot(str);
		redefine = 0;
	}

	//Wyrmgus start
//	type->NumDirections = 0;
	if (!redefine) {
		type->NumDirections = 0;
	}
	//Wyrmgus end
	type->Flip = 1;

	//  Parse the list: (still everything could be changed!)
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		if (!strcmp(value, "Name")) {
			type->Name = LuaToString(l, -1);
		//Wyrmgus start
		} else if (!strcmp(value, "Parent")) {
			type->Parent = LuaToString(l, -1);
			CUnitType *parent_type = UnitTypeByIdent(type->Parent);
			if (!parent_type) {
				LuaError(l, "Unit type %s not defined" _C_ type->Parent.c_str());
			}
			type->Class = parent_type->Class;
			type->DrawLevel = parent_type->DrawLevel;
			type->File = parent_type->File;
			type->Width = parent_type->Width;
			type->Height = parent_type->Height;
			type->OffsetX = parent_type->OffsetX;
			type->OffsetY = parent_type->OffsetY;
			type->ShadowFile = parent_type->ShadowFile;
			type->ShadowWidth = parent_type->ShadowWidth;
			type->ShadowHeight = parent_type->ShadowHeight;
			type->ShadowOffsetX = parent_type->ShadowOffsetX;
			type->ShadowOffsetY = parent_type->ShadowOffsetY;
			type->LightFile = parent_type->LightFile;
			type->TileWidth = parent_type->TileWidth;
			type->TileHeight = parent_type->TileHeight;
			type->BoxWidth = parent_type->BoxWidth;
			type->BoxHeight = parent_type->BoxHeight;
			type->BoxOffsetX = parent_type->BoxOffsetX;
			type->BoxOffsetY = parent_type->BoxOffsetY;
			type->Construction = parent_type->Construction;
			type->UnitType = parent_type->UnitType;
			type->Missile.Name = parent_type->Missile.Name;
			type->Missile.Missile = NULL;
			type->FireMissile.Name = parent_type->FireMissile.Name;
			type->FireMissile.Missile = NULL;
			type->ExplodeWhenKilled = parent_type->ExplodeWhenKilled;
			type->Explosion.Name = parent_type->Explosion.Name;
			type->Explosion.Missile = NULL;
			type->CorpseName = parent_type->CorpseName;
			type->CorpseType = NULL;
//			type->ReactRangeComputer = parent_type->ReactRangeComputer;
//			type->ReactRangePerson = parent_type->ReactRangePerson;
			type->MinAttackRange = parent_type->MinAttackRange;
			type->DefaultStat.Variables[ATTACKRANGE_INDEX].Value = parent_type->DefaultStat.Variables[ATTACKRANGE_INDEX].Value;
			type->DefaultStat.Variables[ATTACKRANGE_INDEX].Max = parent_type->DefaultStat.Variables[ATTACKRANGE_INDEX].Max;
			type->DefaultStat.Variables[PRIORITY_INDEX].Value = parent_type->DefaultStat.Variables[PRIORITY_INDEX].Value;
			type->DefaultStat.Variables[PRIORITY_INDEX].Max  = parent_type->DefaultStat.Variables[PRIORITY_INDEX].Max;
			type->AnnoyComputerFactor = parent_type->AnnoyComputerFactor;
			type->TechnologyPointCost = parent_type->TechnologyPointCost;
			type->TrainQuantity = parent_type->TrainQuantity;
			type->Upkeep = parent_type->Upkeep;
			type->ItemClass = parent_type->ItemClass;
			type->MaxOnBoard = parent_type->MaxOnBoard;
			type->RepairRange = parent_type->RepairRange;
			type->RepairHP = parent_type->RepairHP;
			type->MouseAction = parent_type->MouseAction;
			type->CanAttack = parent_type->CanAttack;
			type->CanTarget = parent_type->CanTarget;
			type->LandUnit = parent_type->LandUnit;
			type->SeaUnit = parent_type->SeaUnit;
			type->AirUnit = parent_type->AirUnit;
			type->Building = parent_type->Building;
			type->BoardSize = parent_type->BoardSize;
			type->ButtonLevelForTransporter = parent_type->ButtonLevelForTransporter;
			type->ButtonLevelForInventory = parent_type->ButtonLevelForInventory;
			type->StartingResources = parent_type->StartingResources;
			type->BurnPercent = parent_type->BurnPercent;
			type->BurnDamageRate = parent_type->BurnDamageRate;
			type->PoisonDrain = parent_type->PoisonDrain;
			type->AutoBuildRate = parent_type->AutoBuildRate;
			type->Animations = parent_type->Animations;
			type->Sound = parent_type->Sound;
			type->NumDirections = parent_type->NumDirections;
			type->NeutralMinimapColorRGB = parent_type->NeutralMinimapColorRGB;
			type->InvertedEastArms = parent_type->InvertedEastArms;
			type->InvertedSoutheastArms = parent_type->InvertedSoutheastArms;
			type->Icon.Name = parent_type->Icon.Name;
			type->Icon.Icon = NULL;
			if (parent_type->CanCastSpell) {
				type->CanCastSpell = new char[SpellTypeTable.size()];
				memset(type->CanCastSpell, 0, SpellTypeTable.size() * sizeof(char));
				for (unsigned int i = 0; i < SpellTypeTable.size(); ++i) {
					type->CanCastSpell[i] = parent_type->CanCastSpell[i];
				}
			}
			if (parent_type->AutoCastActive) {
				type->AutoCastActive = new char[SpellTypeTable.size()];
				memset(type->AutoCastActive, 0, SpellTypeTable.size() * sizeof(char));
				for (unsigned int i = 0; i < SpellTypeTable.size(); ++i) {
					type->AutoCastActive[i] = parent_type->AutoCastActive[i];
				}
			}
			for (unsigned int i = 0; i < MaxCosts; ++i) {
				type->DefaultStat.Costs[i] = parent_type->DefaultStat.Costs[i];
				type->RepairCosts[i] = parent_type->RepairCosts[i];
				type->DefaultStat.ImproveIncomes[i] = parent_type->DefaultStat.ImproveIncomes[i];
				type->CanStore[i] = parent_type->CanStore[i];
				type->GrandStrategyProductionEfficiencyModifier[i] = parent_type->GrandStrategyProductionEfficiencyModifier[i];
				
				if (parent_type->ResInfo[i]) {
					ResourceInfo *res = new ResourceInfo;
					res->ResourceId = i;
					type->ResInfo[i] = res;
					res->ResourceStep = parent_type->ResInfo[i]->ResourceStep;
					res->FinalResource = parent_type->ResInfo[i]->FinalResource;
					res->FinalResourceConversionRate = parent_type->ResInfo[i]->FinalResourceConversionRate;
					res->WaitAtResource = parent_type->ResInfo[i]->WaitAtResource;
					res->WaitAtDepot = parent_type->ResInfo[i]->WaitAtDepot;
					res->ResourceCapacity = parent_type->ResInfo[i]->ResourceCapacity;
					res->LoseResources = parent_type->ResInfo[i]->LoseResources;
					res->RefineryHarvester = parent_type->ResInfo[i]->RefineryHarvester;
					res->FileWhenEmpty = parent_type->ResInfo[i]->FileWhenEmpty;
					res->FileWhenLoaded = parent_type->ResInfo[i]->FileWhenLoaded;
				}
			}
			for (size_t i = 0; i < UnitTypes.size(); ++i) {
				type->DefaultStat.UnitStock[i] = parent_type->DefaultStat.UnitStock[i];
			}
			for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); ++i) {
				type->DefaultStat.Variables[i].Enable = parent_type->DefaultStat.Variables[i].Enable;
				type->DefaultStat.Variables[i].Value = parent_type->DefaultStat.Variables[i].Value;
				type->DefaultStat.Variables[i].Max = parent_type->DefaultStat.Variables[i].Max;
				type->DefaultStat.Variables[i].Increase = parent_type->DefaultStat.Variables[i].Increase;
			}
			for (unsigned int i = 0; i < UnitTypeVar.GetNumberBoolFlag(); ++i) {
				type->BoolFlag[i].value = parent_type->BoolFlag[i].value;
				type->BoolFlag[i].CanTransport = parent_type->BoolFlag[i].CanTransport;
			}
			for (size_t i = 0; i < parent_type->WeaponClasses.size(); ++i) {
				type->WeaponClasses.push_back(parent_type->WeaponClasses[i]);
			}
			for (size_t i = 0; i < parent_type->Drops.size(); ++i) {
				type->Drops.push_back(parent_type->Drops[i]);
			}
			for (size_t i = 0; i < parent_type->AiDrops.size(); ++i) {
				type->AiDrops.push_back(parent_type->AiDrops[i]);
			}
			for (size_t i = 0; i < parent_type->DropAffixes.size(); ++i) {
				type->DropAffixes.push_back(parent_type->DropAffixes[i]);
			}
			for (size_t i = 0; i < parent_type->DropSpells.size(); ++i) {
				type->DropSpells.push_back(parent_type->DropSpells[i]);
			}
			for (size_t i = 0; i < parent_type->Affixes.size(); ++i) {
				type->Affixes.push_back(parent_type->Affixes[i]);
			}
			for (size_t i = 0; i < parent_type->Traits.size(); ++i) {
				type->Traits.push_back(parent_type->Traits[i]);
			}
			for (unsigned int var_n = 0; var_n < VariationMax; ++var_n) {
				if (parent_type->VarInfo[var_n]) {
					VariationInfo *var = new VariationInfo;
					
					type->VarInfo[var_n] = var;
					
					var->VariationId = parent_type->VarInfo[var_n]->VariationId;
					var->TypeName = parent_type->VarInfo[var_n]->TypeName;
					var->File = parent_type->VarInfo[var_n]->File;
					for (unsigned int i = 0; i < MaxCosts; ++i) {
						var->FileWhenLoaded[i] = parent_type->VarInfo[var_n]->FileWhenLoaded[i];
						var->FileWhenEmpty[i] = parent_type->VarInfo[var_n]->FileWhenEmpty[i];
					}
					var->ShadowFile = parent_type->VarInfo[var_n]->ShadowFile;
					var->FrameWidth = parent_type->VarInfo[var_n]->FrameWidth;
					var->FrameHeight = parent_type->VarInfo[var_n]->FrameHeight;
					var->Icon.Name = parent_type->VarInfo[var_n]->Icon.Name;
					var->Icon.Icon = NULL;
					if (parent_type->VarInfo[var_n]->Animations) {
						var->Animations = parent_type->VarInfo[var_n]->Animations;
					}
					var->Construction = parent_type->VarInfo[var_n]->Construction;
					for (int u = 0; u < VariationMax; ++u) {
						var->UpgradesRequired[u] = parent_type->VarInfo[var_n]->UpgradesRequired[u];
						var->UpgradesForbidden[u] = parent_type->VarInfo[var_n]->UpgradesForbidden[u];
					}
					for (size_t i = 0; i < parent_type->VarInfo[var_n]->ItemsEquipped.size(); ++i) {
						var->ItemsEquipped.push_back(parent_type->VarInfo[var_n]->ItemsEquipped[i]);
					}
					for (size_t i = 0; i < parent_type->VarInfo[var_n]->ItemsNotEquipped.size(); ++i) {
						var->ItemsNotEquipped.push_back(parent_type->VarInfo[var_n]->ItemsNotEquipped[i]);
					}
					var->Tileset = parent_type->VarInfo[var_n]->Tileset;
					
					for (int i = 0; i < MaxImageLayers; ++i) {
						var->LayerFiles[i] = parent_type->VarInfo[var_n]->LayerFiles[i];
					}
				} else {
					break;
				}
			}
			for (int i = 0; i < MaxImageLayers; ++i) {
				type->LayerFiles[i] = parent_type->LayerFiles[i];
				
				//inherit layer variations
				for (size_t j = 0; j < parent_type->LayerVarInfo[i].size(); ++j) {
					VariationInfo *var = new VariationInfo;
						
					type->LayerVarInfo[i].push_back(var);
						
					var->VariationId = parent_type->LayerVarInfo[i][j]->VariationId;
					var->File = parent_type->LayerVarInfo[i][j]->File;
					for (int u = 0; u < VariationMax; ++u) {
						var->UpgradesRequired[u] = parent_type->LayerVarInfo[i][j]->UpgradesRequired[u];
						var->UpgradesForbidden[u] = parent_type->LayerVarInfo[i][j]->UpgradesForbidden[u];
					}
					for (size_t u = 0; u < parent_type->LayerVarInfo[i][j]->ItemsEquipped.size(); ++u) {
						var->ItemsEquipped.push_back(parent_type->LayerVarInfo[i][j]->ItemsEquipped[u]);
					}
					for (size_t u = 0; u < parent_type->LayerVarInfo[i][j]->ItemsNotEquipped.size(); ++u) {
						var->ItemsNotEquipped.push_back(parent_type->LayerVarInfo[i][j]->ItemsNotEquipped[u]);
					}
					var->Tileset = parent_type->LayerVarInfo[i][j]->Tileset;
				}
			}
			type->DefaultStat.Variables[PRIORITY_INDEX].Value = parent_type->DefaultStat.Variables[PRIORITY_INDEX].Value + 1; //increase priority by 1 to make it be chosen by the AI when building over the previous unit
			type->DefaultStat.Variables[PRIORITY_INDEX].Max = parent_type->DefaultStat.Variables[PRIORITY_INDEX].Max + 1;
		} else if (!strcmp(value, "Variations")) {
			type->DefaultStat.Variables[VARIATION_INDEX].Enable = 1;
			type->DefaultStat.Variables[VARIATION_INDEX].Value = 0;
			type->DefaultStat.Variables[VARIATION_INDEX].Max = VariationMax;
			//remove previously defined variations, if any
			for (int var_n = 0; var_n < VariationMax; ++var_n) {
				if (type->VarInfo[var_n]) {
					delete type->VarInfo[var_n];
					type->VarInfo[var_n] = NULL;
				}
			}
			//remove previously defined layer variations, if any
			for (int i = 0; i < MaxImageLayers; ++i) {
				for (size_t j = 0; j < type->LayerVarInfo[i].size(); ++j) {
					delete type->LayerVarInfo[i][j];
				}
				type->LayerVarInfo[i].clear();
			}
			int variation_count = 0;
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				VariationInfo *var = new VariationInfo;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for variations)");
				}
				int image_layer = -1;
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "layer")) {
						std::string image_layer_name = LuaToString(l, -1, k + 1);
						image_layer = GetImageLayerIdByName(image_layer_name);
						if (image_layer != -1) {
							type->LayerVarInfo[image_layer].push_back(var);
						} else {
							LuaError(l, "Image layer \"%s\" doesn't exist." _C_ image_layer_name.c_str());
						}
					} else if (!strcmp(value, "variation-id")) {
						var->VariationId = LuaToString(l, -1, k + 1);
						if (image_layer == -1) {
							type->VarInfo[variation_count] = var;
							variation_count += 1;
						}
					} else if (!strcmp(value, "type-name")) {
						var->TypeName = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "file")) {
						var->File = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "file-when-loaded")) {
						const int res = GetResourceIdByName(LuaToString(l, -1, k + 1));
						++k;
						var->FileWhenLoaded[res] = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "file-when-empty")) {
						const int res = GetResourceIdByName(LuaToString(l, -1, k + 1));
						++k;
						var->FileWhenEmpty[res] = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "shadow-file")) {
						var->ShadowFile = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "layer-file")) {
						int image_layer = GetImageLayerIdByName(LuaToString(l, -1, k + 1));
						++k;
						var->LayerFiles[image_layer] = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "frame-size")) {
						lua_rawgeti(l, -1, k + 1);
						CclGetPos(l, &var->FrameWidth, &var->FrameHeight);
						lua_pop(l, 1);
					} else if (!strcmp(value, "icon")) {
						var->Icon.Name = LuaToString(l, -1, k + 1);
						var->Icon.Icon = NULL;
					} else if (!strcmp(value, "animations")) {
						var->Animations = AnimationsByIdent(LuaToString(l, -1, k + 1));
						if (!var->Animations) {
							DebugPrint("Warning animation '%s' not found\n" _C_ LuaToString(l, -1, k + 1));
						}
					} else if (!strcmp(value, "construction")) {
						var->Construction = ConstructionByIdent(LuaToString(l, -1, k + 1));
					} else if (!strcmp(value, "upgrade-required")) {
						for (int u = 0; u < VariationMax; ++u) {
							if (var->UpgradesRequired[u].empty()) {
								var->UpgradesRequired[u] = LuaToString(l, -1, k + 1);
								break;
							}
						}
					} else if (!strcmp(value, "upgrade-forbidden")) {
						for (int u = 0; u < VariationMax; ++u) {
							if (var->UpgradesForbidden[u].empty()) {
								var->UpgradesForbidden[u] = LuaToString(l, -1, k + 1);
								break;
							}
						}
					} else if (!strcmp(value, "item-equipped")) {
						std::string type_ident = LuaToString(l, -1, k + 1);
						CUnitType *type = UnitTypeByIdent(type_ident);
						if (type) {
							var->ItemsEquipped.push_back(type);
						} else {
							LuaError(l, "Unit type %s not defined" _C_ type_ident.c_str());
						}
					} else if (!strcmp(value, "item-not-equipped")) {
						std::string type_ident = LuaToString(l, -1, k + 1);
						CUnitType *type = UnitTypeByIdent(type_ident);
						if (type) {
							var->ItemsNotEquipped.push_back(type);
						} else {
							LuaError(l, "Unit type %s not defined" _C_ type_ident.c_str());
						}
					} else if (!strcmp(value, "tileset")) {
						var->Tileset = LuaToString(l, -1, k + 1);
					} else {
						printf("\n%s\n", type->Name.c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				// Assert(var->VariationId);
				lua_pop(l, 1);
			}
		//Wyrmgus end
		} else if (!strcmp(value, "Image")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, -1, k + 1);
				++k;

				if (!strcmp(value, "file")) {
					type->File = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "size")) {
					lua_rawgeti(l, -1, k + 1);
					CclGetPos(l, &type->Width, &type->Height);
					lua_pop(l, 1);
				} else {
					LuaError(l, "Unsupported image tag: %s" _C_ value);
				}
			}
			if (redefine && type->Sprite) {
				CGraphic::Free(type->Sprite);
				type->Sprite = NULL;
			}
		} else if (!strcmp(value, "Shadow")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, -1, k + 1);
				++k;

				if (!strcmp(value, "file")) {
					type->ShadowFile = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "size")) {
					lua_rawgeti(l, -1, k + 1);
					CclGetPos(l, &type->ShadowWidth, &type->ShadowHeight);
					lua_pop(l, 1);
				} else if (!strcmp(value, "offset")) {
					lua_rawgeti(l, -1, k + 1);
					CclGetPos(l, &type->ShadowOffsetX, &type->ShadowOffsetY);
					lua_pop(l, 1);
				} else {
					LuaError(l, "Unsupported shadow tag: %s" _C_ value);
				}
			}
			if (redefine && type->ShadowSprite) {
				CGraphic::Free(type->ShadowSprite);
				type->ShadowSprite = NULL;
			}
		//Wyrmgus start
		} else if (!strcmp(value, "LightImage")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, -1, k + 1);
				++k;

				if (!strcmp(value, "file")) {
					type->LightFile = LuaToString(l, -1, k + 1);
				} else {
					LuaError(l, "Unsupported light tag: %s" _C_ value);
				}
			}
			if (redefine && type->LightSprite) {
				CGraphic::Free(type->LightSprite);
				type->LightSprite = NULL;
			}
		} else if (!strcmp(value, "LayerImages")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				const int subargs = lua_rawlen(l, -1);
				int image_layer = 0;
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;

					if (!strcmp(value, "layer")) {
						image_layer = GetImageLayerIdByName(LuaToString(l, -1, k + 1));
					} else if (!strcmp(value, "file")) {
						type->LayerFiles[image_layer] = LuaToString(l, -1, k + 1);
					} else {
						LuaError(l, "Unsupported layer image tag: %s" _C_ value);
					}
				}
				if (redefine && type->LayerSprites[image_layer]) {
					CGraphic::Free(type->LayerSprites[image_layer]);
					type->LayerSprites[image_layer] = NULL;
				}
				lua_pop(l, 1);
			}
		//Wyrmgus end
		} else if (!strcmp(value, "Offset")) {
			CclGetPos(l, &type->OffsetX, &type->OffsetY);
		} else if (!strcmp(value, "Flip")) {
			type->Flip = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Animations")) {
			type->Animations = AnimationsByIdent(LuaToString(l, -1));
			if (!type->Animations) {
				DebugPrint("Warning animation '%s' not found\n" _C_ LuaToString(l, -1));
			}
		} else if (!strcmp(value, "Icon")) {
			type->Icon.Name = LuaToString(l, -1);
			type->Icon.Icon = NULL;
#ifdef USE_MNG
		} else if (!strcmp(value, "Portrait")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			type->Portrait.Num = subargs;
			type->Portrait.Files = new std::string[type->Portrait.Num];
			type->Portrait.Mngs = new Mng *[type->Portrait.Num];
			memset(type->Portrait.Mngs, 0, type->Portrait.Num * sizeof(Mng *));
			for (int k = 0; k < subargs; ++k) {
				type->Portrait.Files[k] = LuaToString(l, -1, k + 1);
			}
#endif
		} else if (!strcmp(value, "Costs")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				const int res = CclGetResourceByName(l);
				lua_pop(l, 1);
				++k;
				type->DefaultStat.Costs[res] = LuaToNumber(l, -1, k + 1);
			}
		} else if (!strcmp(value, "Storing")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				const int res = CclGetResourceByName(l);
				lua_pop(l, 1);
				++k;
				type->DefaultStat.Storing[res] = LuaToNumber(l, -1, k + 1);
			}
		} else if (!strcmp(value, "ImproveProduction")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				const int res = CclGetResourceByName(l);
				lua_pop(l, 1);
				++k;
				type->DefaultStat.ImproveIncomes[res] = DefaultIncomes[res] + LuaToNumber(l, -1, k + 1);
			}
		//Wyrmgus start
		} else if (!strcmp(value, "UnitStock")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				const int unit_type_id = UnitTypeIdByIdent(LuaToString(l, -1, k + 1));
				++k;
				type->DefaultStat.UnitStock[unit_type_id] = LuaToNumber(l, -1, k + 1);
			}
		//Wyrmgus end
		} else if (!strcmp(value, "Construction")) {
			// FIXME: What if constructions aren't yet loaded?
			type->Construction = ConstructionByIdent(LuaToString(l, -1));
		} else if (!strcmp(value, "DrawLevel")) {
			type->DrawLevel = LuaToNumber(l, -1);
		} else if (!strcmp(value, "MaxOnBoard")) {
			type->MaxOnBoard = LuaToNumber(l, -1);
			//Wyrmgus start
			type->DefaultStat.Variables[TRANSPORT_INDEX].Max = type->MaxOnBoard;
			//Wyrmgus end
		} else if (!strcmp(value, "BoardSize")) {
			type->BoardSize = LuaToNumber(l, -1);
		} else if (!strcmp(value, "ButtonLevelForTransporter")) {
			type->ButtonLevelForTransporter = LuaToNumber(l, -1);
		//Wyrmgus start
		} else if (!strcmp(value, "ButtonLevelForInventory")) {
			type->ButtonLevelForInventory = LuaToNumber(l, -1);
		//Wyrmgus end
		} else if (!strcmp(value, "StartingResources")) {
			type->StartingResources = LuaToNumber(l, -1);
		} else if (!strcmp(value, "RegenerationRate")) {
			type->DefaultStat.Variables[HP_INDEX].Increase = LuaToNumber(l, -1);
		} else if (!strcmp(value, "BurnPercent")) {
			type->BurnPercent = LuaToNumber(l, -1);
		} else if (!strcmp(value, "BurnDamageRate")) {
			type->BurnDamageRate = LuaToNumber(l, -1);
		} else if (!strcmp(value, "PoisonDrain")) {
			type->PoisonDrain = LuaToNumber(l, -1);
		} else if (!strcmp(value, "ShieldPoints")) {
			if (lua_istable(l, -1)) {
				DefineVariableField(l, type->DefaultStat.Variables + SHIELD_INDEX, -1);
			} else if (lua_isnumber(l, -1)) {
				type->DefaultStat.Variables[SHIELD_INDEX].Max = LuaToNumber(l, -1);
				type->DefaultStat.Variables[SHIELD_INDEX].Value = 0;
				type->DefaultStat.Variables[SHIELD_INDEX].Increase = 1;
				type->DefaultStat.Variables[SHIELD_INDEX].Enable = 1;
			}
		} else if (!strcmp(value, "TileSize")) {
			CclGetPos(l, &type->TileWidth, &type->TileHeight);
		} else if (!strcmp(value, "NeutralMinimapColor")) {
			type->NeutralMinimapColorRGB.Parse(l);
		} else if (!strcmp(value, "BoxSize")) {
			CclGetPos(l, &type->BoxWidth, &type->BoxHeight);
		} else if (!strcmp(value, "BoxOffset")) {
			CclGetPos(l, &type->BoxOffsetX, &type->BoxOffsetY);
		} else if (!strcmp(value, "NumDirections")) {
			type->NumDirections = LuaToNumber(l, -1);
		//Wyrmgus start
//		} else if (!strcmp(value, "ComputerReactionRange")) {
//			type->ReactRangeComputer = LuaToNumber(l, -1);
//		} else if (!strcmp(value, "PersonReactionRange")) {
//			type->ReactRangePerson = LuaToNumber(l, -1);
		//Wyrmgus end
		} else if (!strcmp(value, "Missile")) {
			type->Missile.Name = LuaToString(l, -1);
			type->Missile.Missile = NULL;
		//Wyrmgus start
		} else if (!strcmp(value, "FireMissile")) {
			type->FireMissile.Name = LuaToString(l, -1);
			type->FireMissile.Missile = NULL;
		//Wyrmgus end
		} else if (!strcmp(value, "MinAttackRange")) {
			type->MinAttackRange = LuaToNumber(l, -1);
		} else if (!strcmp(value, "MaxAttackRange")) {
			type->DefaultStat.Variables[ATTACKRANGE_INDEX].Value = LuaToNumber(l, -1);
			type->DefaultStat.Variables[ATTACKRANGE_INDEX].Max = LuaToNumber(l, -1);
			type->DefaultStat.Variables[ATTACKRANGE_INDEX].Enable = 1;
		} else if (!strcmp(value, "MaxHarvesters")) {
			type->DefaultStat.Variables[MAXHARVESTERS_INDEX].Value = LuaToNumber(l, -1);
			type->DefaultStat.Variables[MAXHARVESTERS_INDEX].Max = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Priority")) {
			type->DefaultStat.Variables[PRIORITY_INDEX].Value  = LuaToNumber(l, -1);
			type->DefaultStat.Variables[PRIORITY_INDEX].Max  = LuaToNumber(l, -1);
		} else if (!strcmp(value, "AnnoyComputerFactor")) {
			type->AnnoyComputerFactor = LuaToNumber(l, -1);
		} else if (!strcmp(value, "AiAdjacentRange")) {
			type->AiAdjacentRange = LuaToNumber(l, -1);
		} else if (!strcmp(value, "DecayRate")) {
			type->DecayRate = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Corpse")) {
			type->CorpseName = LuaToString(l, -1);
			type->CorpseType = NULL;
		} else if (!strcmp(value, "DamageType")) {
			value = LuaToString(l, -1);
			//int check = ExtraDeathIndex(value);
			type->DamageType = value;
		} else if (!strcmp(value, "ExplodeWhenKilled")) {
			type->ExplodeWhenKilled = 1;
			type->Explosion.Name = LuaToString(l, -1);
			type->Explosion.Missile = NULL;
		} else if (!strcmp(value, "TeleportCost")) {
			type->TeleportCost = LuaToNumber(l, -1);
		} else if (!strcmp(value, "TeleportEffectIn")) {
			type->TeleportEffectIn = new LuaCallback(l, -1);
		} else if (!strcmp(value, "TeleportEffectOut")) {
			type->TeleportEffectOut = new LuaCallback(l, -1);
		} else if (!strcmp(value, "DeathExplosion")) {
			type->DeathExplosion = new LuaCallback(l, -1);
		} else if (!strcmp(value, "OnHit")) {
			type->OnHit = new LuaCallback(l, -1);
		} else if (!strcmp(value, "OnEachCycle")) {
			type->OnEachCycle = new LuaCallback(l, -1);
		} else if (!strcmp(value, "OnEachSecond")) {
			type->OnEachSecond = new LuaCallback(l, -1);
		} else if (!strcmp(value, "OnInit")) {
			type->OnInit = new LuaCallback(l, -1);
		} else if (!strcmp(value, "Type")) {
			value = LuaToString(l, -1);
			if (!strcmp(value, "land")) {
				type->UnitType = UnitTypeLand;
				//Wyrmgus start
				type->LandUnit = true;
				//Wyrmgus end
			} else if (!strcmp(value, "fly")) {
				type->UnitType = UnitTypeFly;
				//Wyrmgus start
				type->AirUnit = true;
				//Wyrmgus end
			//Wyrmgus start
			} else if (!strcmp(value, "fly-low")) {
				type->UnitType = UnitTypeFlyLow;
				//Wyrmgus start
				type->AirUnit = true;
				//Wyrmgus end
			//Wyrmgus end
			} else if (!strcmp(value, "naval")) {
				type->UnitType = UnitTypeNaval;
				//Wyrmgus start
				type->SeaUnit = true;
				//Wyrmgus end
			} else {
				LuaError(l, "Unsupported Type: %s" _C_ value);
			}
		} else if (!strcmp(value, "MissileOffsets")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				if (!lua_istable(l, -1) || lua_rawlen(l, -1) != UnitSides) {
					LuaError(l, "incorrect argument");
				}
				for (int m = 0; m < UnitSides; ++m) {
					lua_rawgeti(l, -1, m + 1);
					CclGetPos(l, &type->MissileOffsets[m][k].x, &type->MissileOffsets[m][k].y);
					lua_pop(l, 1);
				}
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "Impact")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				const char *dtype = LuaToString(l, -1, k + 1);
				++k;

				if (!strcmp(dtype, "general")) {
					type->Impact[ANIMATIONS_DEATHTYPES].Name = LuaToString(l, -1, k + 1);
					type->Impact[ANIMATIONS_DEATHTYPES].Missile = NULL;
				} else if (!strcmp(dtype, "shield")) {
					type->Impact[ANIMATIONS_DEATHTYPES + 1].Name = LuaToString(l, -1, k + 1);
					type->Impact[ANIMATIONS_DEATHTYPES + 1].Missile = NULL;
				} else {
					int num = 0;
					for (; num < ANIMATIONS_DEATHTYPES; ++num) {
						if (dtype == ExtraDeathTypes[num]) {
							break;
						}
					}
					if (num == ANIMATIONS_DEATHTYPES) {
						LuaError(l, "Death type not found: %s" _C_ dtype);
					} else {
						type->Impact[num].Name = LuaToString(l, -1, k + 1);
						type->Impact[num].Missile = NULL;
					}
				}
			}
		} else if (!strcmp(value, "RightMouseAction")) {
			value = LuaToString(l, -1);
			if (!strcmp(value, "none")) {
				type->MouseAction = MouseActionNone;
			} else if (!strcmp(value, "attack")) {
				type->MouseAction = MouseActionAttack;
			} else if (!strcmp(value, "move")) {
				type->MouseAction = MouseActionMove;
			} else if (!strcmp(value, "harvest")) {
				type->MouseAction = MouseActionHarvest;
			} else if (!strcmp(value, "spell-cast")) {
				type->MouseAction = MouseActionSpellCast;
			} else if (!strcmp(value, "sail")) {
				type->MouseAction = MouseActionSail;
			//Wyrmgus start
			} else if (!strcmp(value, "rally-point")) {
				type->MouseAction = MouseActionRallyPoint;
			//Wyrmgus end
			} else {
				LuaError(l, "Unsupported RightMouseAction: %s" _C_ value);
			}
		} else if (!strcmp(value, "CanAttack")) {
			type->CanAttack = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "RepairRange")) {
			type->RepairRange = LuaToNumber(l, -1);
		} else if (!strcmp(value, "RepairHp")) {
			type->RepairHP = LuaToNumber(l, -1);
		} else if (!strcmp(value, "RepairCosts")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				const int res = CclGetResourceByName(l);
				lua_pop(l, 1);
				++k;
				type->RepairCosts[res] = LuaToNumber(l, -1, k + 1);
			}
		} else if (!strcmp(value, "CanTargetLand")) {
			if (LuaToBoolean(l, -1)) {
				type->CanTarget |= CanTargetLand;
			} else {
				type->CanTarget &= ~CanTargetLand;
			}
		} else if (!strcmp(value, "CanTargetSea")) {
			if (LuaToBoolean(l, -1)) {
				type->CanTarget |= CanTargetSea;
			} else {
				type->CanTarget &= ~CanTargetSea;
			}
		} else if (!strcmp(value, "CanTargetAir")) {
			if (LuaToBoolean(l, -1)) {
				type->CanTarget |= CanTargetAir;
			} else {
				type->CanTarget &= ~CanTargetAir;
			}
		} else if (!strcmp(value, "Building")) {
			type->Building = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "BuildingRules")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			// Free any old restrictions if they are redefined
			for (std::vector<CBuildRestriction *>::iterator b = type->BuildingRules.begin();
				 b != type->BuildingRules.end(); ++b) {
				delete *b;
			}
			type->BuildingRules.clear();
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument");
				}
				ParseBuildingRules(l, type->BuildingRules);
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "AiBuildingRules")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			// Free any old restrictions if they are redefined
			for (std::vector<CBuildRestriction *>::iterator b = type->AiBuildingRules.begin();
				 b != type->AiBuildingRules.end(); ++b) {
				delete *b;
			}
			type->AiBuildingRules.clear();
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument");
				}
				ParseBuildingRules(l, type->AiBuildingRules);
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "AutoBuildRate")) {
			type->AutoBuildRate = LuaToNumber(l, -1);
		//Wyrmgus start
		/*
		} else if (!strcmp(value, "LandUnit")) {
			type->LandUnit = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "AirUnit")) {
			type->AirUnit = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "SeaUnit")) {
			type->SeaUnit = LuaToBoolean(l, -1);
		*/
		//Wyrmgus end
		} else if (!strcmp(value, "RandomMovementProbability")) {
			type->RandomMovementProbability = LuaToNumber(l, -1);
		} else if (!strcmp(value, "RandomMovementDistance")) {
			type->RandomMovementDistance = LuaToNumber(l, -1);
		} else if (!strcmp(value, "ClicksToExplode")) {
			type->ClicksToExplode = LuaToNumber(l, -1);
		} else if (!strcmp(value, "CanTransport")) {
			//  Warning: CanTransport should only be used AFTER all bool flags
			//  have been defined.
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			if (type->MaxOnBoard == 0) { // set default value.
				type->MaxOnBoard = 1;
				//Wyrmgus start
				type->DefaultStat.Variables[TRANSPORT_INDEX].Max = type->MaxOnBoard;
				//Wyrmgus end
			}

			if (type->BoolFlag.size() < UnitTypeVar.GetNumberBoolFlag()) {
				type->BoolFlag.resize(UnitTypeVar.GetNumberBoolFlag());
			}

			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, -1, k + 1);
				++k;

				const int index = UnitTypeVar.BoolFlagNameLookup[value];
				if (index != -1) {
					value = LuaToString(l, -1, k + 1);
					type->BoolFlag[index].CanTransport = Ccl2Condition(l, value);
					continue;
				}
				LuaError(l, "Unsupported flag tag for CanTransport: %s" _C_ value);
			}
		} else if (!strcmp(value, "CanGatherResources")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				//Wyrmgus start
//				ResourceInfo *res = new ResourceInfo;
				ResourceInfo *res;
				//Wyrmgus end
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "resource-id")) {
						lua_rawgeti(l, -1, k + 1);
						//Wyrmgus start
//						res->ResourceId = CclGetResourceByName(l);
//						type->ResInfo[res->ResourceId] = res;
						int resource_id = CclGetResourceByName(l);
						//allow redefinition
						res = type->ResInfo[resource_id];
						if (!res) {
							res = new ResourceInfo;
							type->ResInfo[resource_id] = res;
						}
						res->ResourceId = resource_id;
						//Wyrmgus end
						lua_pop(l, 1);
					} else if (!strcmp(value, "resource-step")) {
						res->ResourceStep = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "final-resource")) {
						lua_rawgeti(l, -1, k + 1);
						res->FinalResource = CclGetResourceByName(l);
						lua_pop(l, 1);
					//Wyrmgus start
					} else if (!strcmp(value, "final-resource-conversion-rate")) {
						res->FinalResourceConversionRate = LuaToNumber(l, -1, k + 1);
					//Wyrmgus end
					} else if (!strcmp(value, "wait-at-resource")) {
						res->WaitAtResource = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "wait-at-depot")) {
						res->WaitAtDepot = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "resource-capacity")) {
						res->ResourceCapacity = LuaToNumber(l, -1, k + 1);
					//Wyrmgus start
					/*
					} else if (!strcmp(value, "terrain-harvester")) {
						res->TerrainHarvester = 1;
						--k;
					*/
					//Wyrmgus end
					} else if (!strcmp(value, "lose-resources")) {
						res->LoseResources = 1;
						--k;
					//Wyrmgus start
					/*
					} else if (!strcmp(value, "harvest-from-outside")) {
						res->HarvestFromOutside = 1;
						--k;
					*/
					//Wyrmgus end
					} else if (!strcmp(value, "refinery-harvester")) {
						res->RefineryHarvester = 1;
						--k;
					} else if (!strcmp(value, "file-when-empty")) {
						res->FileWhenEmpty = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "file-when-loaded")) {
						res->FileWhenLoaded = LuaToString(l, -1, k + 1);
					} else {
						printf("\n%s\n", type->Name.c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				if (!res->FinalResource) {
					res->FinalResource = res->ResourceId;
				}
				//Wyrmgus start
				if (!res->FinalResourceConversionRate) {
					res->FinalResourceConversionRate = 100;
				}
				//Wyrmgus end
				Assert(res->ResourceId);
				lua_pop(l, 1);
			}
			type->BoolFlag[HARVESTER_INDEX].value = 1;
		} else if (!strcmp(value, "GivesResource")) {
			lua_pushvalue(l, -1);
			type->GivesResource = CclGetResourceByName(l);
			lua_pop(l, 1);
		} else if (!strcmp(value, "CanStore")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				type->CanStore[CclGetResourceByName(l)] = 1;
				lua_pop(l, 1);
			}
		//Wyrmgus start
		} else if (!strcmp(value, "GrandStrategyProductionEfficiencyModifier")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				const int res = CclGetResourceByName(l);
				lua_pop(l, 1);
				++k;
				type->GrandStrategyProductionEfficiencyModifier[res] = LuaToNumber(l, -1, k + 1);
			}
		//Wyrmgus end
		} else if (!strcmp(value, "CanCastSpell")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			//
			// Warning: can-cast-spell should only be used AFTER all spells
			// have been defined. FIXME: MaxSpellType=500 or something?
			//
			if (!type->CanCastSpell) {
				type->CanCastSpell = new char[SpellTypeTable.size()];
				memset(type->CanCastSpell, 0, SpellTypeTable.size() * sizeof(char));
			}
			const int subargs = lua_rawlen(l, -1);
			if (subargs == 0) {
				delete[] type->CanCastSpell;
				type->CanCastSpell = NULL;
			}
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, -1, k + 1);
				const SpellType *spell = SpellTypeByIdent(value);
				if (spell == NULL) {
					LuaError(l, "Unknown spell type: %s" _C_ value);
				}
				type->CanCastSpell[spell->Slot] = 1;
			}
		} else if (!strcmp(value, "AutoCastActive")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			//
			// Warning: AutoCastActive should only be used AFTER all spells
			// have been defined.
			//
			if (!type->AutoCastActive) {
				type->AutoCastActive = new char[SpellTypeTable.size()];
				memset(type->AutoCastActive, 0, SpellTypeTable.size() * sizeof(char));
			}
			const int subargs = lua_rawlen(l, -1);
			if (subargs == 0) {
				delete[] type->AutoCastActive;
				type->AutoCastActive = NULL;

			}
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, -1, k + 1);
				const SpellType *spell = SpellTypeByIdent(value);
				if (spell == NULL) {
					LuaError(l, "AutoCastActive : Unknown spell type: %s" _C_ value);
				}
				if (!spell->AutoCast) {
					LuaError(l, "AutoCastActive : Define autocast method for %s." _C_ value);
				}
				type->AutoCastActive[spell->Slot] = 1;
			}
		} else if (!strcmp(value, "CanTargetFlag")) {
			//
			// Warning: can-target-flag should only be used AFTER all bool flags
			// have been defined.
			//
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			if (type->BoolFlag.size() < UnitTypeVar.GetNumberBoolFlag()) {
				type->BoolFlag.resize(UnitTypeVar.GetNumberBoolFlag());
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, -1, k + 1);
				++k;
				int index = UnitTypeVar.BoolFlagNameLookup[value];
				if (index != -1) {
					value = LuaToString(l, -1, k + 1);
					type->BoolFlag[index].CanTargetFlag = Ccl2Condition(l, value);
					continue;
				}
				LuaError(l, "Unsupported flag tag for can-target-flag: %s" _C_ value);
			}
		} else if (!strcmp(value, "PriorityTarget")) {
			//
			// Warning: ai-priority-target should only be used AFTER all bool flags
			// have been defined.
			//
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			if (type->BoolFlag.size() < UnitTypeVar.GetNumberBoolFlag()) {
				type->BoolFlag.resize(UnitTypeVar.GetNumberBoolFlag());
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, -1, k + 1);
				++k;
				int index = UnitTypeVar.BoolFlagNameLookup[value];
				if (index != -1) {
					value = LuaToString(l, -1, k + 1);
					type->BoolFlag[index].AiPriorityTarget = Ccl2Condition(l, value);
					continue;
				}
				LuaError(l, "Unsupported flag tag for ai-priority-target: %s" _C_ value);
			}
		} else if (!strcmp(value, "Sounds")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, -1, k + 1);
				++k;

				if (!strcmp(value, "selected")) {
					type->Sound.Selected.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "acknowledge")) {
					type->Sound.Acknowledgement.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "attack")) {
					type->Sound.Attack.Name = LuaToString(l, -1, k + 1);
				//Wyrmgus start
				} else if (!strcmp(value, "idle")) {
					type->Sound.Idle.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "hit")) {
					type->Sound.Hit.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "miss")) {
					type->Sound.Miss.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "step")) {
					type->Sound.Step.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "step-dirt")) {
					type->Sound.StepDirt.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "step-grass")) {
					type->Sound.StepGrass.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "step-gravel")) {
					type->Sound.StepGravel.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "step-mud")) {
					type->Sound.StepMud.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "step-stone")) {
					type->Sound.StepStone.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "used")) {
					type->Sound.Used.Name = LuaToString(l, -1, k + 1);
				//Wyrmgus end
				} else if (!strcmp(value, "build")) {
					type->Sound.Build.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "ready")) {
					type->Sound.Ready.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "repair")) {
					type->Sound.Repair.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "harvest")) {
					const std::string name = LuaToString(l, -1, k + 1);
					++k;
					const int resId = GetResourceIdByName(l, name.c_str());
					type->Sound.Harvest[resId].Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "help")) {
					type->Sound.Help.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "dead")) {
					int death;

					const std::string name = LuaToString(l, -1, k + 1);
					for (death = 0; death < ANIMATIONS_DEATHTYPES; ++death) {
						if (name == ExtraDeathTypes[death]) {
							++k;
							break;
						}
					}
					if (death == ANIMATIONS_DEATHTYPES) {
						type->Sound.Dead[ANIMATIONS_DEATHTYPES].Name = name;
					} else {
						type->Sound.Dead[death].Name = LuaToString(l, -1, k + 1);
					}
				} else {
					LuaError(l, "Unsupported sound tag: %s" _C_ value);
				}
			}
		//Wyrmgus start
		} else if (!strcmp(value, "Class")) {
			type->Class = LuaToString(l, -1);
		} else if (!strcmp(value, "Civilization")) {
			type->Civilization = LuaToString(l, -1);
		} else if (!strcmp(value, "Faction")) {
			type->Faction = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			type->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Quote")) {
			type->Quote = LuaToString(l, -1);
		} else if (!strcmp(value, "Gender")) {
			type->DefaultStat.Variables[GENDER_INDEX].Enable = 1;
			type->DefaultStat.Variables[GENDER_INDEX].Value = GetGenderIdByName(LuaToString(l, -1));
			type->DefaultStat.Variables[GENDER_INDEX].Max = type->DefaultStat.Variables[GENDER_INDEX].Value;
		} else if (!strcmp(value, "Background")) {
			type->Background = LuaToString(l, -1);
		} else if (!strcmp(value, "PersonalNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				type->PersonalNames[j] = LuaToString(l, -1, j + 1);
			}
		} else if (!strcmp(value, "PersonalNamePrefixes")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				type->PersonalNamePrefixes[j] = LuaToString(l, -1, j + 1);
			}
		} else if (!strcmp(value, "PersonalNameSuffixes")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				type->PersonalNameSuffixes[j] = LuaToString(l, -1, j + 1);
			}
		} else if (!strcmp(value, "TechnologyPointCost")) {
			type->TechnologyPointCost = LuaToNumber(l, -1);
		} else if (!strcmp(value, "TrainQuantity")) {
			type->TrainQuantity = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Upkeep")) {
			type->Upkeep = LuaToNumber(l, -1);
		} else if (!strcmp(value, "ChildUpgrade")) {
			type->ChildUpgrade = LuaToString(l, -1);
		} else if (!strcmp(value, "Excrement")) {
			type->Excrement = LuaToString(l, -1);
		} else if (!strcmp(value, "Drops")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				int drop_type_id = UnitTypeIdByIdent(LuaToString(l, -1, j + 1));
				if (drop_type_id != -1) {
					type->Drops.push_back(drop_type_id);
				} else { // Error
					LuaError(l, "incorrect drop unit-type");
				}
			}
		} else if (!strcmp(value, "AiDrops")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				int drop_type_id = UnitTypeIdByIdent(LuaToString(l, -1, j + 1));
				if (drop_type_id != -1) {
					type->AiDrops.push_back(drop_type_id);
				} else { // Error
					LuaError(l, "incorrect drop unit-type");
				}
			}
		} else if (!strcmp(value, "DropAffixes")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string affix_ident = LuaToString(l, -1, j + 1);
				int affix_id = UpgradeIdByIdent(affix_ident);
				if (affix_id != -1) {
					type->DropAffixes.push_back(AllUpgrades[affix_id]);
				} else {
					type->DropAffixes.push_back(CUpgrade::New(affix_ident)); //if this affix doesn't exist, define it now (this is useful if the unit type is defined before the upgrade)
				}
			}
		} else if (!strcmp(value, "DropSpells")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				value = LuaToString(l, -1, j + 1);
				SpellType *spell = SpellTypeByIdent(value);
				if (spell != NULL) {
					type->DropSpells.push_back(spell);
				} else {
					LuaError(l, "Spell \"%s\" doesn't exist." _C_ value);
				}
			}
		} else if (!strcmp(value, "Affixes")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string affix_ident = LuaToString(l, -1, j + 1);
				int affix_id = UpgradeIdByIdent(affix_ident);
				if (affix_id != -1) {
					type->Affixes.push_back(AllUpgrades[affix_id]);
				} else {
					type->Affixes.push_back(CUpgrade::New(affix_ident)); //if this affix doesn't exist, define it now (this is useful if the unit type is defined before the upgrade)
				}
			}
		} else if (!strcmp(value, "Traits")) {
			type->Traits.clear(); // remove previously defined traits, to allow unit types to not inherit traits from their parent unit types
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string trait_ident = LuaToString(l, -1, j + 1);
				int trait_id = UpgradeIdByIdent(trait_ident);
				if (trait_id != -1) {
					type->Traits.push_back(AllUpgrades[trait_id]);
				} else {
					type->Traits.push_back(CUpgrade::New(trait_ident)); //if this trait doesn't exist, define it now (this is useful if the unit type is defined before the upgrade)
				}
			}
		} else if (!strcmp(value, "ItemClass")) {
			type->ItemClass = GetItemClassIdByName(LuaToString(l, -1));
		} else if (!strcmp(value, "WeaponClasses")) {
			type->WeaponClasses.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				int weapon_class_id = GetItemClassIdByName(LuaToString(l, -1, j + 1));
				if (weapon_class_id != -1) {
					type->WeaponClasses.push_back(weapon_class_id);
				} else { // Error
					LuaError(l, "incorrect weapon class");
				}
			}
		} else if (!strcmp(value, "InvertedEastArms")) {
			type->InvertedEastArms = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "InvertedSoutheastArms")) {
			type->InvertedSoutheastArms = LuaToBoolean(l, -1);
		//Wyrmgus end
		} else {
			int index = UnitTypeVar.VariableNameLookup[value];
			if (index != -1) { // valid index
				if (lua_isboolean(l, -1)) {
					type->DefaultStat.Variables[index].Enable = LuaToBoolean(l, -1);
				} else if (lua_istable(l, -1)) {
					DefineVariableField(l, type->DefaultStat.Variables + index, -1);
				} else if (lua_isnumber(l, -1)) {
					type->DefaultStat.Variables[index].Enable = 1;
					type->DefaultStat.Variables[index].Value = LuaToNumber(l, -1);
					type->DefaultStat.Variables[index].Max = LuaToNumber(l, -1);
				} else { // Error
					LuaError(l, "incorrect argument for the variable in unittype");
				}
				continue;
			}

			if (type->BoolFlag.size() < UnitTypeVar.GetNumberBoolFlag()) {
				type->BoolFlag.resize(UnitTypeVar.GetNumberBoolFlag());
			}

			index = UnitTypeVar.BoolFlagNameLookup[value];
			if (index != -1) {
				if (lua_isnumber(l, -1)) {
					type->BoolFlag[index].value = LuaToNumber(l, -1);
				} else {
					type->BoolFlag[index].value = LuaToBoolean(l, -1);
				}
			} else {
				printf("\n%s\n", type->Name.c_str());
				LuaError(l, "Unsupported tag: %s" _C_ value);
			}
		}
	}
	
	//Wyrmgus start
	if (!type->Class.empty()) { //if class is defined, then use this unit type to help build the classes table, and add this unit to the civilization class table (if the civilization is defined)
		int class_id = -1;
		for (unsigned int i = 0; i != UnitTypeClassMax; ++i) {
			if (UnitTypeClasses[i] == type->Class) {
				class_id = i;
				break;
			}
			if (UnitTypeClasses[i].empty()) { //if reached a blank slot, then the class isn't recorded yet; do so now
				UnitTypeClasses[i] = type->Class;
				SetUnitTypeClassStringToIndex(type->Class, i);
				class_id = i;
				break;
			}
		}
		if (!type->Civilization.empty()) {
			int civilization_id = PlayerRaces.GetRaceIndexByName(type->Civilization.c_str());
			
			if (!type->Faction.empty()) {
				int faction_id = PlayerRaces.GetFactionIndexByName(civilization_id, type->Faction);
				if (civilization_id != -1 && faction_id != -1 && class_id != -1) {
					PlayerRaces.FactionClassUnitTypes[civilization_id][faction_id][class_id] = type->Slot;
				}
			} else {
				if (civilization_id != -1 && class_id != -1) {
					PlayerRaces.CivilizationClassUnitTypes[civilization_id][class_id] = type->Slot;
				}
			}
		}
	}
	//Wyrmgus end

	// If number of directions is not specified, make a guess
	// Building have 1 direction and units 8
	if (type->Building && type->NumDirections == 0) {
		type->NumDirections = 1;
	} else if (type->NumDirections == 0) {
		type->NumDirections = 8;
	}
	
	//Wyrmgus start
	//unit type's level must be at least 1
	if (type->DefaultStat.Variables[LEVEL_INDEX].Value == 0) {
		type->DefaultStat.Variables[LEVEL_INDEX].Enable = 1;
		type->DefaultStat.Variables[LEVEL_INDEX].Value = 1;
		type->DefaultStat.Variables[LEVEL_INDEX].Max = 1;
	}
	//Wyrmgus end

	// FIXME: try to simplify/combine the flags instead
	if (type->MouseAction == MouseActionAttack && !type->CanAttack) {
		LuaError(l, "Unit-type '%s': right-attack is set, but can-attack is not\n" _C_ type->Name.c_str());
	}
	UpdateDefaultBoolFlags(*type);
	if (!CclInConfigFile) {
		UpdateUnitStats(*type, 1);
	}
	return 0;
}

/**
**  Parse unit-stats.
**
**  @param l  Lua state.
*/
static int CclDefineUnitStats(lua_State *l)
{
	CUnitType *type = UnitTypeByIdent(LuaToString(l, 1));
	const int playerId = LuaToNumber(l, 2);

	Assert(type);
	Assert(playerId < PlayerMax);

	CUnitStats *stats = &type->Stats[playerId];
	if (!stats->Variables) {
		stats->Variables = new CVariable[UnitTypeVar.GetNumberVariable()];
	}

	// Parse the list: (still everything could be changed!)
	const int args = lua_rawlen(l, 3);
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, 3, j + 1);
		++j;

		if (!strcmp(value, "costs")) {
			lua_rawgeti(l, 3, j + 1);
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);

			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, 3, j + 1);
				value = LuaToString(l, -1, k + 1);
				++k;
				const int resId = GetResourceIdByName(l, value);
				stats->Costs[resId] = LuaToNumber(l, -1, k + 1);
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "storing")) {
			lua_rawgeti(l, 3, j + 1);
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);

			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, 3, j + 1);
				value = LuaToString(l, -1, k + 1);
				++k;
				const int resId = GetResourceIdByName(l, value);
				stats->Storing[resId] = LuaToNumber(l, -1, k + 1);
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "improve-production")) {
			lua_rawgeti(l, 3, j + 1);
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);

			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, 3, j + 1);
				value = LuaToString(l, -1, k + 1);
				++k;
				const int resId = GetResourceIdByName(l, value);
				stats->ImproveIncomes[resId] = LuaToNumber(l, -1, k + 1);
				lua_pop(l, 1);
			}
		//Wyrmgus start
		} else if (!strcmp(value, "unit-stock")) {
			lua_rawgeti(l, 3, j + 1);
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);

			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, 3, j + 1);
				const int unit_type_id = UnitTypeIdByIdent(LuaToString(l, -1, k + 1));
				++k;
				stats->UnitStock[unit_type_id] = LuaToNumber(l, -1, k + 1);
				lua_pop(l, 1);
			}
		//Wyrmgus end
		} else {
			int i = UnitTypeVar.VariableNameLookup[value];// User variables
			if (i != -1) { // valid index
				lua_rawgeti(l, 3, j + 1);
				if (lua_istable(l, -1)) {
					DefineVariableField(l, stats->Variables + i, -1);
				} else if (lua_isnumber(l, -1)) {
					stats->Variables[i].Enable = 1;
					stats->Variables[i].Value = LuaToNumber(l, -1);
					stats->Variables[i].Max = LuaToNumber(l, -1);
				} else { // Error
					LuaError(l, "incorrect argument for the variable in unittype");
				}
				continue;
			}
			// This leaves a half initialized unit
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	return 0;
}

// ----------------------------------------------------------------------------

/**
**  Access unit-type object
**
**  @param l  Lua state.
*/
CUnitType *CclGetUnitType(lua_State *l)
{
	// Be kind allow also strings or symbols
	if (lua_isstring(l, -1)) {
		const char *str = LuaToString(l, -1);
		return UnitTypeByIdent(str);
	} else if (lua_isuserdata(l, -1)) {
		LuaUserData *data = (LuaUserData *)lua_touserdata(l, -1);
		if (data->Type == LuaUnitType) {
			return (CUnitType *)data->Data;
		}
	}
	LuaError(l, "CclGetUnitType: not a unit-type");
	return NULL;
}

/**
**  Get unit-type structure.
**
**  @param l  Lua state.
**
**  @return   Unit-type structure.
*/
static int CclUnitType(lua_State *l)
{
	LuaCheckArgs(l, 1);

	const char *str = LuaToString(l, 1);
	CUnitType *type = UnitTypeByIdent(str);
	LuaUserData *data = (LuaUserData *)lua_newuserdata(l, sizeof(LuaUserData));
	data->Type = LuaUnitType;
	data->Data = type;
	return 1;
}

/**
**  Get all unit-type structures.
**
**  @param l  Lua state.
**
**  @return   An array of all unit-type structures.
*/
static int CclUnitTypeArray(lua_State *l)
{
	LuaCheckArgs(l, 0);

	lua_newtable(l);

	for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
		LuaUserData *data = (LuaUserData *)lua_newuserdata(l, sizeof(LuaUserData));
		data->Type = LuaUnitType;
		data->Data = UnitTypes[i];
		lua_rawseti(l, 1, i + 1);
	}
	return 1;
}

/**
**  Get the ident of the unit-type structure.
**
**  @param l  Lua state.
**
**  @return   The identifier of the unit-type.
*/
static int CclGetUnitTypeIdent(lua_State *l)
{
	LuaCheckArgs(l, 1);

	const CUnitType *type = CclGetUnitType(l);
	if (type) {
		lua_pushstring(l, type->Ident.c_str());
	} else {
		LuaError(l, "unit '%s' not defined" _C_ LuaToString(l, -1));
	}
	return 1;
}

/**
**  Get the name of the unit-type structure.
**
**  @param l  Lua state.
**
**  @return   The name of the unit-type.
*/
static int CclGetUnitTypeName(lua_State *l)
{
	LuaCheckArgs(l, 1);

	const CUnitType *type = CclGetUnitType(l);
	lua_pushstring(l, type->Name.c_str());
	return 1;
}

/**
**  Set the name of the unit-type structure.
**
**  @param l  Lua state.
**
**  @return   The name of the unit-type.
*/
static int CclSetUnitTypeName(lua_State *l)
{
	LuaCheckArgs(l, 2);

	lua_pushvalue(l, 1);
	CUnitType *type = CclGetUnitType(l);
	lua_pop(l, 1);
	type->Name = LuaToString(l, 2);

	lua_pushvalue(l, 2);
	return 1;
}

/**
**  Get unit type data.
**
**  @param l  Lua state.
*/
static int CclGetUnitTypeData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	lua_pushvalue(l, 1);
	const CUnitType *type = CclGetUnitType(l);
	lua_pop(l, 1);
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, type->Name.c_str());
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Parent")) {
		lua_pushstring(l, type->Parent.c_str());
		return 1;
	} else if (!strcmp(data, "Class")) {
		if (type->ItemClass == -1) {
			lua_pushstring(l, type->Class.c_str());
		} else {
			lua_pushstring(l, GetItemClassNameById(type->ItemClass).c_str());
		}
		return 1;
	} else if (!strcmp(data, "Civilization")) {
		lua_pushstring(l, type->Civilization.c_str());
		return 1;
	} else if (!strcmp(data, "Faction")) {
		lua_pushstring(l, type->Faction.c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, type->Description.c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, type->Quote.c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, type->Background.c_str());
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "Icon")) {
		lua_pushstring(l, type->Icon.Name.c_str());
		return 1;
	} else if (!strcmp(data, "Costs")) {
		LuaCheckArgs(l, 3);
		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		if (!GameRunning && Editor.Running != EditorEditing) {
			lua_pushnumber(l, type->DefaultStat.Costs[resId]);
		} else {
			lua_pushnumber(l, type->MapDefaultStat.Costs[resId]);
		}
		return 1;
	} else if (!strcmp(data, "ImproveProduction")) {
		LuaCheckArgs(l, 3);
		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		if (!GameRunning && Editor.Running != EditorEditing) {
			lua_pushnumber(l, type->DefaultStat.ImproveIncomes[resId]);
		} else {
			lua_pushnumber(l, type->MapDefaultStat.ImproveIncomes[resId]);
		}
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "UnitStock")) {
		LuaCheckArgs(l, 3);
		const int unit_type_id = UnitTypeIdByIdent(LuaToString(l, 3));
		if (!GameRunning && Editor.Running != EditorEditing) {
			lua_pushnumber(l, type->DefaultStat.UnitStock[unit_type_id]);
		} else {
			lua_pushnumber(l, type->MapDefaultStat.UnitStock[unit_type_id]);
		}
		return 1;
	} else if (!strcmp(data, "ChildUpgrade")) {
		lua_pushstring(l, type->ChildUpgrade.c_str());
		return 1;
	} else if (!strcmp(data, "Excrement")) {
		lua_pushstring(l, type->Excrement.c_str());
		return 1;
	} else if (!strcmp(data, "TechnologyPointCost")) {
		lua_pushnumber(l, type->TechnologyPointCost);
		return 1;
	} else if (!strcmp(data, "TrainQuantity")) {
		lua_pushnumber(l, type->TrainQuantity);
		return 1;
	} else if (!strcmp(data, "Upkeep")) {
		lua_pushnumber(l, type->Upkeep);
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "DrawLevel")) {
		lua_pushnumber(l, type->DrawLevel);
		return 1;
	} else if (!strcmp(data, "TileWidth")) {
		lua_pushnumber(l, type->TileWidth);
		return 1;
	} else if (!strcmp(data, "TileHeight")) {
		lua_pushnumber(l, type->TileHeight);
		return 1;
	//Wyrmgus start
	/*
	} else if (!strcmp(data, "ComputerReactionRange")) {
		lua_pushnumber(l, type->ReactRangeComputer);
		return 1;
	} else if (!strcmp(data, "PersonReactionRange")) {
		lua_pushnumber(l, type->ReactRangePerson);
		return 1;
	*/
	} else if (!strcmp(data, "ItemClass")) {
		lua_pushstring(l, GetItemClassNameById(type->ItemClass).c_str());
		return 1;
	} else if (!strcmp(data, "WeaponClasses")) {
		lua_createtable(l, type->WeaponClasses.size(), 0);
		for (size_t i = 1; i <= type->WeaponClasses.size(); ++i)
		{
			lua_pushstring(l, GetItemClassNameById(type->WeaponClasses[i-1]).c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "Missile")) {
		lua_pushstring(l, type->Missile.Name.c_str());
		return 1;
	} else if (!strcmp(data, "FireMissile")) {
		lua_pushstring(l, type->FireMissile.Name.c_str());
		return 1;
	} else if (!strcmp(data, "MinAttackRange")) {
		lua_pushnumber(l, type->MinAttackRange);
		return 1;
	} else if (!strcmp(data, "MaxAttackRange")) {
		if (!GameRunning && Editor.Running != EditorEditing) {
			lua_pushnumber(l, type->DefaultStat.Variables[ATTACKRANGE_INDEX].Value);
		} else {
			lua_pushnumber(l, type->MapDefaultStat.Variables[ATTACKRANGE_INDEX].Value);
		}
		return 1;
	} else if (!strcmp(data, "Priority")) {
		if (!GameRunning && Editor.Running != EditorEditing) {
			lua_pushnumber(l, type->DefaultStat.Variables[PRIORITY_INDEX].Value);
		} else {
			lua_pushnumber(l, type->MapDefaultStat.Variables[PRIORITY_INDEX].Value);
		}
		return 1;
	} else if (!strcmp(data, "Type")) {
		if (type->UnitType == UnitTypeLand) {
			lua_pushstring(l, "land");
			return 1;
		} else if (type->UnitType == UnitTypeFly) {
			lua_pushstring(l, "fly");
			return 1;
		//Wyrmgus start
		} else if (type->UnitType == UnitTypeFlyLow) {
			lua_pushstring(l, "fly-low");
			return 1;
		//Wyrmgus end
		} else if (type->UnitType == UnitTypeNaval) {
			lua_pushstring(l, "naval");
			return 1;
		}
	} else if (!strcmp(data, "Corpse")) {
		lua_pushstring(l, type->CorpseName.c_str());
		return 1;
	} else if (!strcmp(data, "CanAttack")) {
		lua_pushboolean(l, type->CanAttack);
		return 1;
	} else if (!strcmp(data, "Building")) {
		lua_pushboolean(l, type->Building);
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Item")) {
		lua_pushboolean(l, type->BoolFlag[ITEM_INDEX].value);
		return 1;
	} else if (!strcmp(data, "Mercenary")) {
		lua_pushboolean(l, type->BoolFlag[MERCENARY_INDEX].value);
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "LandUnit")) {
		lua_pushboolean(l, type->LandUnit);
		return 1;
	} else if (!strcmp(data, "GivesResource")) {
		if (type->GivesResource > 0) {
			lua_pushstring(l, DefaultResourceNames[type->GivesResource].c_str());
			return 1;
		} else {
			lua_pushstring(l, "");
			return 1;
		}
	} else if (!strcmp(data, "Sounds")) {
		LuaCheckArgs(l, 3);
		const std::string sound_type = LuaToString(l, 3);
		if (sound_type == "selected") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.Selected.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.Selected.Name.c_str());
			}
		} else if (sound_type == "acknowledge") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.Acknowledgement.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.Acknowledgement.Name.c_str());
			}
		} else if (sound_type == "attack") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.Attack.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.Attack.Name.c_str());
			}
		//Wyrmgus start
		} else if (sound_type == "idle") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.Idle.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.Idle.Name.c_str());
			}
		} else if (sound_type == "hit") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.Hit.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.Hit.Name.c_str());
			}
		} else if (sound_type == "miss") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.Miss.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.Miss.Name.c_str());
			}
		} else if (sound_type == "step") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.Step.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.Step.Name.c_str());
			}
		} else if (sound_type == "step-dirt") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.StepDirt.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.StepDirt.Name.c_str());
			}
		} else if (sound_type == "step-grass") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.StepGrass.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.StepGrass.Name.c_str());
			}
		} else if (sound_type == "step-gravel") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.StepGravel.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.StepGravel.Name.c_str());
			}
		} else if (sound_type == "step-mud") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.StepMud.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.StepMud.Name.c_str());
			}
		} else if (sound_type == "step-stone") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.StepStone.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.StepStone.Name.c_str());
			}
		} else if (sound_type == "used") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.Used.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.Used.Name.c_str());
			}
		//Wyrmgus end
		} else if (sound_type == "build") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.Build.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.Build.Name.c_str());
			}
		} else if (sound_type == "ready") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.Ready.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.Ready.Name.c_str());
			}
		} else if (sound_type == "repair") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.Repair.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.Repair.Name.c_str());
			}
		} else if (sound_type == "harvest") {
			LuaCheckArgs(l, 4);
			const std::string sound_subtype = LuaToString(l, 4);
			const int resId = GetResourceIdByName(sound_subtype.c_str());
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.Harvest[resId].Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.Harvest[resId].Name.c_str());
			}
		} else if (sound_type == "help") {
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushstring(l, type->Sound.Help.Name.c_str());
			} else {
				lua_pushstring(l, type->MapSound.Help.Name.c_str());
			}
		} else if (sound_type == "dead") {
			if (lua_gettop(l) < 4) {
				if (!GameRunning && Editor.Running != EditorEditing) {
					lua_pushstring(l, type->Sound.Dead[ANIMATIONS_DEATHTYPES].Name.c_str());
				} else {
					lua_pushstring(l, type->MapSound.Dead[ANIMATIONS_DEATHTYPES].Name.c_str());
				}
			} else {
				int death;
				const std::string sound_subtype = LuaToString(l, 4);

				for (death = 0; death < ANIMATIONS_DEATHTYPES; ++death) {
					if (sound_subtype == ExtraDeathTypes[death]) {
						break;
					}
				}
				if (death == ANIMATIONS_DEATHTYPES) {
					if (!GameRunning && Editor.Running != EditorEditing) {
						lua_pushstring(l, type->Sound.Dead[ANIMATIONS_DEATHTYPES].Name.c_str());
					} else {
						lua_pushstring(l, type->MapSound.Dead[ANIMATIONS_DEATHTYPES].Name.c_str());
					}
				} else {
					if (!GameRunning && Editor.Running != EditorEditing) {
						lua_pushstring(l, type->Sound.Dead[death].Name.c_str());
					} else {
						lua_pushstring(l, type->MapSound.Dead[death].Name.c_str());
					}
				}
			}
		}
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Drops")) {
		lua_createtable(l, type->Drops.size(), 0);
		for (size_t i = 1; i <= type->Drops.size(); ++i)
		{
			lua_pushstring(l, UnitTypes[type->Drops[i-1]]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "AiDrops")) {
		lua_createtable(l, type->AiDrops.size(), 0);
		for (size_t i = 1; i <= type->AiDrops.size(); ++i)
		{
			lua_pushstring(l, UnitTypes[type->AiDrops[i-1]]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "DropAffixes")) {
		lua_createtable(l, type->DropAffixes.size(), 0);
		for (size_t i = 1; i <= type->DropAffixes.size(); ++i)
		{
			lua_pushstring(l, type->DropAffixes[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Affixes")) {
		lua_createtable(l, type->Affixes.size(), 0);
		for (size_t i = 1; i <= type->Affixes.size(); ++i)
		{
			lua_pushstring(l, type->Affixes[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Droppers")) { // unit types which can drop this one
		std::vector<CUnitType *> droppers;
		for (size_t i = 0; i < UnitTypes.size(); ++i) {
			if (
				std::find(UnitTypes[i]->Drops.begin(), UnitTypes[i]->Drops.end(), type->Slot) != UnitTypes[i]->Drops.end()
				|| std::find(UnitTypes[i]->AiDrops.begin(), UnitTypes[i]->AiDrops.end(), type->Slot) != UnitTypes[i]->AiDrops.end()
			) {
				droppers.push_back(UnitTypes[i]);
			}
		}
		
		lua_createtable(l, droppers.size(), 0);
		for (size_t i = 1; i <= droppers.size(); ++i)
		{
			lua_pushstring(l, droppers[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Traits")) {
		lua_createtable(l, type->Traits.size(), 0);
		for (size_t i = 1; i <= type->Traits.size(); ++i)
		{
			lua_pushstring(l, type->Traits[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Prefixes")) {
		std::vector<CUpgrade *> prefixes;
		for (size_t i = 0; i < type->Affixes.size(); ++i)
		{
			if (type->Affixes[i]->MagicPrefix) {
				prefixes.push_back(type->Affixes[i]);
			}
		}
		if (type->ItemClass != -1) {
			for (size_t i = 0; i < AllUpgrades.size(); ++i) {
				if (AllUpgrades[i]->MagicPrefix && AllUpgrades[i]->ItemPrefix[type->ItemClass]) {
					prefixes.push_back(AllUpgrades[i]);
				}
			}
		}
		
		lua_createtable(l, prefixes.size(), 0);
		for (size_t i = 1; i <= prefixes.size(); ++i)
		{
			lua_pushstring(l, prefixes[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Suffixes")) {
		std::vector<CUpgrade *> suffixes;
		for (size_t i = 0; i < type->Affixes.size(); ++i)
		{
			if (type->Affixes[i]->MagicSuffix) {
				suffixes.push_back(type->Affixes[i]);
			}
		}
		if (type->ItemClass != -1) {
			for (size_t i = 0; i < AllUpgrades.size(); ++i) {
				if (AllUpgrades[i]->MagicSuffix && AllUpgrades[i]->ItemSuffix[type->ItemClass]) {
					suffixes.push_back(AllUpgrades[i]);
				}
			}
		}
		
		lua_createtable(l, suffixes.size(), 0);
		for (size_t i = 1; i <= suffixes.size(); ++i)
		{
			lua_pushstring(l, suffixes[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Variations")) {
		std::vector<std::string> variation_idents;
		for (int var_n = 0; var_n < VariationMax; ++var_n) {
			if (type->VarInfo[var_n] && std::find(variation_idents.begin(), variation_idents.end(), type->VarInfo[var_n]->VariationId) == variation_idents.end()) {
				variation_idents.push_back(type->VarInfo[var_n]->VariationId);
			}
		}
		
		lua_createtable(l, variation_idents.size(), 0);
		for (size_t i = 1; i <= variation_idents.size(); ++i)
		{
			lua_pushstring(l, variation_idents[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "LayerVariations")) {
		LuaCheckArgs(l, 3);
		const std::string image_layer_name = LuaToString(l, 3);
		const int image_layer = GetImageLayerIdByName(image_layer_name);
		
		std::vector<std::string> variation_idents;
		for (size_t var_n = 0; var_n < type->LayerVarInfo[image_layer].size(); ++var_n) {
			if (type->LayerVarInfo[image_layer][var_n] && std::find(variation_idents.begin(), variation_idents.end(), type->LayerVarInfo[image_layer][var_n]->VariationId) == variation_idents.end()) {
				variation_idents.push_back(type->LayerVarInfo[image_layer][var_n]->VariationId);
			}
		}
		
		lua_createtable(l, variation_idents.size(), 0);
		for (size_t i = 1; i <= variation_idents.size(); ++i)
		{
			lua_pushstring(l, variation_idents[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	//Wyrmgus end
	} else {
		int index = UnitTypeVar.VariableNameLookup[data];
		if (index != -1) { // valid index
			if (!GameRunning && Editor.Running != EditorEditing) {
				lua_pushnumber(l, type->DefaultStat.Variables[index].Value);
			} else {
				lua_pushnumber(l, type->MapDefaultStat.Variables[index].Value);
			}
			return 1;
		}

		index = UnitTypeVar.BoolFlagNameLookup[data];
		if (index != -1) {
			lua_pushboolean(l, type->BoolFlag[index].value);
			return 1;
		} else {
			LuaError(l, "Invalid field: %s" _C_ data);
		}
	}

	return 0;
}

// ----------------------------------------------------------------------------

/**
**  Define the field of the UserDefined variables.
**
**  @param l          Lua state.
**  @param var        Variable to set.
**  @param lua_index  Index of the table where are the infos
**
**  @internal Use to not duplicate code.
*/
void DefineVariableField(lua_State *l, CVariable *var, int lua_index)
{
	if (lua_index < 0) { // relative index
		--lua_index;
	}
	lua_pushnil(l);
	while (lua_next(l, lua_index)) {
		const char *key = LuaToString(l, -2);

		if (!strcmp(key, "Value")) {
			var->Value = LuaToNumber(l, -1);
		} else if (!strcmp(key, "Max")) {
			var->Max = LuaToNumber(l, -1);
		} else if (!strcmp(key, "Increase")) {
			var->Increase = LuaToNumber(l, -1);
		} else if (!strcmp(key, "Enable")) {
			var->Enable = LuaToBoolean(l, -1);
		} else { // Error.
			LuaError(l, "incorrect field '%s' for variable\n" _C_ key);
		}
		lua_pop(l, 1); // pop the value;
	}
}

/**
**  Define user variables.
**
**  @param l  Lua state.
*/
static int CclDefineVariables(lua_State *l)
{
	int old = UnitTypeVar.GetNumberVariable();

	const int args = lua_gettop(l);
	for (int j = 0; j < args; ++j) {
		const char *str = LuaToString(l, j + 1);

		const int index = UnitTypeVar.VariableNameLookup.AddKey(str);
		if (index == old) {
			old++;
			UnitTypeVar.Variable.resize(old);
		} else {
			DebugPrint("Warning, User Variable \"%s\" redefined\n" _C_ str);
		}
		if (!lua_istable(l, j + 2)) { // No change => default value.
			continue;
		}
		++j;
		DefineVariableField(l, &(UnitTypeVar.Variable[index]), j + 1);
	}
	return 0;
}

/**
**  Define boolean flag.
**
**  @param l  Lua state.
*/
static int CclDefineBoolFlags(lua_State *l)
{
	const unsigned int old = UnitTypeVar.GetNumberBoolFlag();
	const int args = lua_gettop(l);
	for (int j = 0; j < args; ++j) {
		const char *str = LuaToString(l, j + 1);

		UnitTypeVar.BoolFlagNameLookup.AddKey(str);

	}

	if (0 < old && old != UnitTypeVar.GetNumberBoolFlag()) {
		size_t new_size = UnitTypeVar.GetNumberBoolFlag();
		for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) { // adjust array for unit already defined
			UnitTypes[i]->BoolFlag.resize(new_size);
		}
	}
	return 0;
}

/**
**  Define Decorations for user variables
**
**  @param l  Lua state.
**
**  @todo modify Assert with luastate with User Error.
**  @todo continue to add configuration.
*/
static int CclDefineDecorations(lua_State *l)
{
	struct {
		int Index;
		//Wyrmgus start
		int MinValue;
		//Wyrmgus end
		int OffsetX;
		int OffsetY;
		int OffsetXPercent;
		int OffsetYPercent;
		bool IsCenteredInX;
		bool IsCenteredInY;
		bool ShowIfNotEnable;
		bool ShowWhenNull;
		bool HideHalf;
		bool ShowWhenMax;
		bool ShowOnlySelected;
		bool HideNeutral;
		bool HideAllied;
		//Wyrmgus start
		bool HideSelf;
		//Wyrmgus end
		bool ShowOpponent;
	} tmp;

	const int nargs = lua_gettop(l);
	for (int i = 0; i < nargs; i++) {
		Assert(lua_istable(l, i + 1));
		CDecoVar *decovar = NULL;
		memset(&tmp, 0, sizeof(tmp));
		lua_pushnil(l);
		while (lua_next(l, i + 1)) {
			const char *key = LuaToString(l, -2);
			if (!strcmp(key, "Index")) {
				const char *const value = LuaToString(l, -1);
				tmp.Index = UnitTypeVar.VariableNameLookup[value];// User variables
				Assert(tmp.Index != -1);
			//Wyrmgus start
			} else if (!strcmp(key, "MinValue")) {
				tmp.MinValue = LuaToNumber(l, -1);
			//Wyrmgus end
			} else if (!strcmp(key, "Offset")) {
				CclGetPos(l, &tmp.OffsetX, &tmp.OffsetY);
			} else if (!strcmp(key, "OffsetPercent")) {
				CclGetPos(l, &tmp.OffsetXPercent, &tmp.OffsetYPercent);
			} else if (!strcmp(key, "CenterX")) {
				tmp.IsCenteredInX = LuaToBoolean(l, -1);
			} else if (!strcmp(key, "CenterY")) {
				tmp.IsCenteredInY = LuaToBoolean(l, -1);
			} else if (!strcmp(key, "ShowIfNotEnable")) {
				tmp.ShowIfNotEnable = LuaToBoolean(l, -1);
			} else if (!strcmp(key, "ShowWhenNull")) {
				tmp.ShowWhenNull = LuaToBoolean(l, -1);
			} else if (!strcmp(key, "HideHalf")) {
				tmp.HideHalf = LuaToBoolean(l, -1);
			} else if (!strcmp(key, "ShowWhenMax")) {
				tmp.ShowWhenMax = LuaToBoolean(l, -1);
			} else if (!strcmp(key, "ShowOnlySelected")) {
				tmp.ShowOnlySelected = LuaToBoolean(l, -1);
			} else if (!strcmp(key, "HideNeutral")) {
				tmp.HideNeutral = LuaToBoolean(l, -1);
			} else if (!strcmp(key, "HideAllied")) {
				tmp.HideAllied = LuaToBoolean(l, -1);
			//Wyrmgus start
			} else if (!strcmp(key, "HideSelf")) {
				tmp.HideSelf = LuaToBoolean(l, -1);
			//Wyrmgus end
			} else if (!strcmp(key, "ShowOpponent")) {
				tmp.ShowOpponent = LuaToBoolean(l, -1);
			} else if (!strcmp(key, "Method")) {
				Assert(lua_istable(l, -1));
				lua_rawgeti(l, -1, 1); // MethodName
				lua_rawgeti(l, -2, 2); // Data
				Assert(lua_istable(l, -1));
				key = LuaToString(l, -2);
				if (!strcmp(key, "bar")) {
					CDecoVarBar *decovarbar = new CDecoVarBar;
					lua_pushnil(l);
					while (lua_next(l, -2)) {
						key = LuaToString(l, -2);
						if (!strcmp(key, "Height")) {
							decovarbar->Height = LuaToNumber(l, -1);
						} else if (!strcmp(key, "Width")) {
							decovarbar->Width = LuaToNumber(l, -1);
						} else if (!strcmp(key, "Orientation")) {
							key = LuaToString(l, -1);
							if (!strcmp(key, "horizontal")) {
								decovarbar->IsVertical = 0;
							} else if (!strcmp(key, "vertical")) {
								decovarbar->IsVertical = 1;
							} else { // Error
								LuaError(l, "invalid Orientation '%s' for bar in DefineDecorations" _C_ key);
							}
						} else if (!strcmp(key, "SEToNW")) {
							decovarbar->SEToNW = LuaToBoolean(l, -1);
						} else if (!strcmp(key, "BorderSize")) {
							decovarbar->BorderSize = LuaToNumber(l, -1);
						} else if (!strcmp(key, "ShowFullBackground")) {
							decovarbar->ShowFullBackground = LuaToBoolean(l, -1);
#if 0 // FIXME Color configuration
						} else if (!strcmp(key, "Color")) {
							decovar->Color = // FIXME
						} else if (!strcmp(key, "BColor")) {
							decovar->BColor = // FIXME
#endif
						} else {
							LuaError(l, "'%s' invalid for Method bar" _C_ key);
						}
						lua_pop(l, 1); // Pop value
					}
					decovar = decovarbar;
				} else if (!strcmp(key, "text")) {
					CDecoVarText *decovartext = new CDecoVarText;

					decovartext->Font = CFont::Get(LuaToString(l, -1, 1));
					// FIXME : More arguments ? color...
					decovar = decovartext;
				} else if (!strcmp(key, "sprite")) {
					CDecoVarSpriteBar *decovarspritebar = new CDecoVarSpriteBar;
					decovarspritebar->NSprite = GetSpriteIndex(LuaToString(l, -1, 1));
					if (decovarspritebar->NSprite == -1) {
						LuaError(l, "invalid sprite-name '%s' for Method in DefineDecorations" _C_ LuaToString(l, -1, 1));
					}
					// FIXME : More arguments ?
					decovar = decovarspritebar;
				} else if (!strcmp(key, "static-sprite")) {
					CDecoVarStaticSprite *decovarstaticsprite = new CDecoVarStaticSprite;
					if (lua_rawlen(l, -1) == 2) {
						decovarstaticsprite->NSprite = GetSpriteIndex(LuaToString(l, -1, 1));
						decovarstaticsprite->n = LuaToNumber(l, -1, 2);
					} else {
						decovarstaticsprite->NSprite = GetSpriteIndex(LuaToString(l, -1, 1));
						decovarstaticsprite->n = LuaToNumber(l, -1, 2);
						decovarstaticsprite->FadeValue = LuaToNumber(l, -1, 3);
					}
					decovar = decovarstaticsprite;
				} else { // Error
					LuaError(l, "invalid method '%s' for Method in DefineDecorations" _C_ key);
				}
				lua_pop(l, 2); // MethodName and data
			} else { // Error
				LuaError(l, "invalid key '%s' for DefineDecorations" _C_ key);
			}
			lua_pop(l, 1); // Pop the value
		}
		decovar->Index = tmp.Index;
		//Wyrmgus start
		decovar->MinValue = tmp.MinValue;
		//Wyrmgus end
		decovar->OffsetX = tmp.OffsetX;
		decovar->OffsetY = tmp.OffsetY;
		decovar->OffsetXPercent = tmp.OffsetXPercent;
		decovar->OffsetYPercent = tmp.OffsetYPercent;
		decovar->IsCenteredInX = tmp.IsCenteredInX;
		decovar->IsCenteredInY = tmp.IsCenteredInY;
		decovar->ShowIfNotEnable = tmp.ShowIfNotEnable;
		decovar->ShowWhenNull = tmp.ShowWhenNull;
		decovar->HideHalf = tmp.HideHalf;
		decovar->ShowWhenMax = tmp.ShowWhenMax;
		decovar->ShowOnlySelected = tmp.ShowOnlySelected;
		decovar->HideNeutral = tmp.HideNeutral;
		decovar->HideAllied = tmp.HideAllied;
		//Wyrmgus start
		decovar->HideSelf = tmp.HideSelf;
		//Wyrmgus end
		decovar->ShowOpponent = tmp.ShowOpponent;
		//Wyrmgus start
//		UnitTypeVar.DecoVar.push_back(decovar);
		bool already_defined = false;
		for (std::vector<CDecoVar *>::iterator it = UnitTypeVar.DecoVar.begin();
			 it != UnitTypeVar.DecoVar.end(); ++it) {
			if ((*it)->Index == tmp.Index) { // replace other decorations which use the same variable
				*it = decovar;
				already_defined = true;
			}
		}
		if (!already_defined) {
			UnitTypeVar.DecoVar.push_back(decovar);
		}
		//Wyrmgus end
	}
	Assert(lua_gettop(l));
	return 0;
}

/**
**  Define default extra death types.
**
**  @param l  Lua state.
*/
static int CclDefineExtraDeathTypes(lua_State *l)
{
	unsigned int args;

	for (unsigned int i = 0; i < ANIMATIONS_DEATHTYPES; ++i) {
		ExtraDeathTypes[i].clear();
	}
	args = lua_gettop(l);
	for (unsigned int i = 0; i < ANIMATIONS_DEATHTYPES && i < args; ++i) {
		ExtraDeathTypes[i] = LuaToString(l, i + 1);
	}
	return 0;
}
// ----------------------------------------------------------------------------

/**
**  Update unit variables which are not user defined.
*/
void UpdateUnitVariables(CUnit &unit)
{
	const CUnitType *type = unit.Type;

	for (int i = 0; i < NVARALREADYDEFINED; i++) { // default values
		if (i == ARMOR_INDEX || i == PIERCINGDAMAGE_INDEX || i == BASICDAMAGE_INDEX
			//Wyrmgus start
			|| i == SUPPLY_INDEX || i == DEMAND_INDEX
			|| i == THORNSDAMAGE_INDEX
			|| i == FIREDAMAGE_INDEX || i == COLDDAMAGE_INDEX || i == ARCANEDAMAGE_INDEX || i == LIGHTNINGDAMAGE_INDEX || i == AIRDAMAGE_INDEX || i == EARTHDAMAGE_INDEX || i == WATERDAMAGE_INDEX
			|| i == SPEED_INDEX
			|| i == FIRERESISTANCE_INDEX || i == COLDRESISTANCE_INDEX || i == ARCANERESISTANCE_INDEX || i == LIGHTNINGRESISTANCE_INDEX || i == AIRRESISTANCE_INDEX || i == EARTHRESISTANCE_INDEX || i == WATERRESISTANCE_INDEX
			|| i == HACKRESISTANCE_INDEX || i == PIERCERESISTANCE_INDEX || i == BLUNTRESISTANCE_INDEX
			//Wyrmgus end
			|| i == MANA_INDEX || i == KILL_INDEX || i == XP_INDEX || i == GIVERESOURCE_INDEX
			//Wyrmgus start
			|| i == AUTOREPAIRRANGE_INDEX
			//Wyrmgus end
			|| i == BLOODLUST_INDEX || i == HASTE_INDEX || i == SLOW_INDEX
			|| i == INVISIBLE_INDEX || i == UNHOLYARMOR_INDEX || i == HP_INDEX
			|| i == SHIELD_INDEX || i == POINTS_INDEX || i == MAXHARVESTERS_INDEX
			|| i == POISON_INDEX || i == SHIELDPERMEABILITY_INDEX || i == SHIELDPIERCING_INDEX
			//Wyrmgus
//			|| i == ISALIVE_INDEX || i == PLAYER_INDEX) {
			|| i == ISALIVE_INDEX || i == PLAYER_INDEX || i == PRIORITY_INDEX || i == SIGHTRANGE_INDEX || i == ATTACKRANGE_INDEX
			|| i == STRENGTH_INDEX || i == DEXTERITY_INDEX || i == INTELLIGENCE_INDEX
			|| i == ACCURACY_INDEX || i == EVASION_INDEX
			|| i == LEVEL_INDEX || i == LEVELUP_INDEX || i == XPREQUIRED_INDEX || i == VARIATION_INDEX || i == HITPOINTHEALING_INDEX || i == HITPOINTBONUS_INDEX || i == CRITICALSTRIKECHANCE_INDEX
			|| i == BACKSTAB_INDEX || i == BONUSAGAINSTMOUNTED_INDEX || i == BONUSAGAINSTBUILDINGS_INDEX || i == BONUSAGAINSTAIR_INDEX || i == BONUSAGAINSTGIANTS_INDEX || i == BONUSAGAINSTDRAGONS_INDEX
			|| i == DAYSIGHTRANGEBONUS_INDEX || i == NIGHTSIGHTRANGEBONUS_INDEX || i == TRANSPARENCY_INDEX || i == GENDER_INDEX || i == BIRTHCYCLE_INDEX || i == HUNGER_INDEX || i == STUN_INDEX || i == BLEEDING_INDEX || i == TIMEEFFICIENCYBONUS_INDEX) {
			//Wyrmgus end
			continue;
		}
		unit.Variable[i].Value = 0;
		unit.Variable[i].Max = 0;
		unit.Variable[i].Enable = 1;
	}

	//Wyrmgus
	unit.Variable[VARIATION_INDEX].Max = VariationMax;
	unit.Variable[VARIATION_INDEX].Enable = 1;
	unit.Variable[VARIATION_INDEX].Value = unit.Variation;

	unit.Variable[TRANSPARENCY_INDEX].Max = 100;

	unit.Variable[LEVEL_INDEX].Max = 100000;
	if (!IsNetworkGame() && unit.Character != NULL && unit.Character->Persistent && unit.Player->AiEnabled == false) {
		if (unit.Variable[LEVEL_INDEX].Value > unit.Character->Level) { //save level, if unit has a persistent character
			unit.Character->Level = unit.Variable[LEVEL_INDEX].Value;
			SaveHero(unit.Character);
		}
	}
	
	if (unit.Variable[GENDER_INDEX].Value == NoGender && unit.Type->BoolFlag[ORGANIC_INDEX].value) { // Gender: 0 = Not Set, 1 = Male, 2 = Female, 3 = Asexual
		unit.Variable[GENDER_INDEX].Value = SyncRand(2) + 1;
		unit.Variable[GENDER_INDEX].Max = MaxGenders;
		unit.Variable[GENDER_INDEX].Enable = 1;
	}
	
	if (unit.Variable[BIRTHCYCLE_INDEX].Value && (GameCycle - unit.Variable[BIRTHCYCLE_INDEX].Value) > 1000 && !unit.Type->ChildUpgrade.empty()) { // 1000 cycles until maturation, for all species (should change this to have different maturation times for different species)
		unit.Variable[BIRTHCYCLE_INDEX].Value = 0;
		IndividualUpgradeLost(unit, CUpgrade::Get(unit.Type->ChildUpgrade));
	}

	if (unit.Type->BoolFlag[ORGANIC_INDEX].value && unit.Type->BoolFlag[FAUNA_INDEX].value) { // only fauna can have a hunger value
		unit.Variable[HUNGER_INDEX].Enable = 1;
		unit.Variable[HUNGER_INDEX].Max = 1000;
		unit.Variable[HUNGER_INDEX].Increase = 1;
	}
	//Wyrmgus end

	// Shield permeability
	unit.Variable[SHIELDPERMEABILITY_INDEX].Max = 100;

	// Transport
	unit.Variable[TRANSPORT_INDEX].Value = unit.BoardCount;
	unit.Variable[TRANSPORT_INDEX].Max = unit.Type->MaxOnBoard;

	unit.CurrentOrder()->UpdateUnitVariables(unit);

	// Resources.
	if (unit.Type->GivesResource) {
		unit.Variable[GIVERESOURCE_INDEX].Value = unit.ResourcesHeld;
		unit.Variable[GIVERESOURCE_INDEX].Max = unit.ResourcesHeld > unit.Variable[GIVERESOURCE_INDEX].Max ? unit.ResourcesHeld : unit.Variable[GIVERESOURCE_INDEX].Max;
		//Wyrmgus start
		unit.Variable[GIVERESOURCE_INDEX].Enable = 1;
		//Wyrmgus end
	}
	if (unit.Type->BoolFlag[HARVESTER_INDEX].value && unit.CurrentResource) {
		unit.Variable[CARRYRESOURCE_INDEX].Value = unit.ResourcesHeld;
		unit.Variable[CARRYRESOURCE_INDEX].Max = unit.Type->ResInfo[unit.CurrentResource]->ResourceCapacity;
	}

	//Wyrmgus start
	/*
	// SightRange
	unit.Variable[SIGHTRANGE_INDEX].Value = type->MapDefaultStat.Variables[SIGHTRANGE_INDEX].Value;
	unit.Variable[SIGHTRANGE_INDEX].Max = unit.Stats->Variables[SIGHTRANGE_INDEX].Max;
	*/
	//Wyrmgus end

	// AttackRange
	//Wyrmgus start
//	unit.Variable[ATTACKRANGE_INDEX].Value = type->MapDefaultStat.Variables[ATTACKRANGE_INDEX].Max;
//	unit.Variable[ATTACKRANGE_INDEX].Max = unit.Stats->Variables[ATTACKRANGE_INDEX].Max;
	//Wyrmgus end

	// Priority
	unit.Variable[PRIORITY_INDEX].Value = type->MapDefaultStat.Variables[PRIORITY_INDEX].Max;
	unit.Variable[PRIORITY_INDEX].Max = unit.Stats->Variables[PRIORITY_INDEX].Max;

	// Position
	unit.Variable[POSX_INDEX].Value = unit.tilePos.x;
	unit.Variable[POSX_INDEX].Max = Map.Info.MapWidth;
	unit.Variable[POSY_INDEX].Value = unit.tilePos.y;
	unit.Variable[POSY_INDEX].Max = Map.Info.MapHeight;

	// Target Position
	const Vec2i goalPos = unit.CurrentOrder()->GetGoalPos();
	unit.Variable[TARGETPOSX_INDEX].Value = goalPos.x;
	unit.Variable[TARGETPOSX_INDEX].Max = Map.Info.MapWidth;
	unit.Variable[TARGETPOSY_INDEX].Value = goalPos.y;
	unit.Variable[TARGETPOSY_INDEX].Max = Map.Info.MapHeight;

	// RadarRange
	unit.Variable[RADAR_INDEX].Value = unit.Stats->Variables[RADAR_INDEX].Value;
	unit.Variable[RADAR_INDEX].Max = unit.Stats->Variables[RADAR_INDEX].Value;

	// RadarJammerRange
	unit.Variable[RADARJAMMER_INDEX].Value = unit.Stats->Variables[RADARJAMMER_INDEX].Value;
	unit.Variable[RADARJAMMER_INDEX].Max = unit.Stats->Variables[RADARJAMMER_INDEX].Value;

	// SlotNumber
	unit.Variable[SLOT_INDEX].Value = UnitNumber(unit);
	unit.Variable[SLOT_INDEX].Max = UnitManager.GetUsedSlotCount();

	// Is Alive
	unit.Variable[ISALIVE_INDEX].Value = unit.IsAlive() ? 1 : 0;
	unit.Variable[ISALIVE_INDEX].Max = 1;

	// Player
	unit.Variable[PLAYER_INDEX].Value = unit.Player->Index;
	unit.Variable[PLAYER_INDEX].Max = PlayerMax;

	for (int i = 0; i < NVARALREADYDEFINED; i++) { // default values
		unit.Variable[i].Enable &= unit.Variable[i].Max > 0;
		if (unit.Variable[i].Value > unit.Variable[i].Max) {
			DebugPrint("Value out of range: '%s'(%d), for variable '%s',"
					   " value = %d, max = %d\n"
					   _C_ type->Ident.c_str() _C_ UnitNumber(unit) _C_ UnitTypeVar.VariableNameLookup[i]
					   _C_ unit.Variable[i].Value _C_ unit.Variable[i].Max);
			clamp(&unit.Variable[i].Value, 0, unit.Variable[i].Max);
		}
	}
}

/**
**  Set the map default stat for a unit type
**
**  @param ident			Unit type ident
**  @param variable_key		Key of the desired variable
**  @param value			Value to set to
**  @param variable_type	Type to be modified (i.e. "Value", "Max", etc.); alternatively, resource type if variable_key equals "Costs"
*/
void SetMapStat(std::string ident, std::string variable_key, int value, std::string variable_type)
{
	CUnitType *type = UnitTypeByIdent(ident.c_str());
	
	if (variable_key == "Costs") {
		const int resId = GetResourceIdByName(variable_type.c_str());
		type->MapDefaultStat.Costs[resId] = value;
		for (int player = 0; player < PlayerMax; ++player) {
			type->Stats[player].Costs[resId] = type->MapDefaultStat.Costs[resId];
		}
	} else if (variable_key == "ImproveProduction") {
		const int resId = GetResourceIdByName(variable_type.c_str());
		type->MapDefaultStat.ImproveIncomes[resId] = value;
		for (int player = 0; player < PlayerMax; ++player) {
			type->Stats[player].ImproveIncomes[resId] = type->MapDefaultStat.ImproveIncomes[resId];
		}
	} else if (variable_key == "UnitStock") {
		const int unit_type_id = UnitTypeIdByIdent(variable_type);
		type->MapDefaultStat.UnitStock[unit_type_id] = value;
		for (int player = 0; player < PlayerMax; ++player) {
			type->Stats[player].UnitStock[unit_type_id] = type->MapDefaultStat.UnitStock[unit_type_id];
		}
	} else {
		int variable_index = UnitTypeVar.VariableNameLookup[variable_key.c_str()];
		if (variable_index != -1) { // valid index
			if (variable_type == "Value") {
				type->MapDefaultStat.Variables[variable_index].Value = value;
				for (int player = 0; player < PlayerMax; ++player) {
					type->Stats[player].Variables[variable_index].Value = type->MapDefaultStat.Variables[variable_index].Value;
				}
			} else if (variable_type == "Max") {
				type->MapDefaultStat.Variables[variable_index].Max = value;
				for (int player = 0; player < PlayerMax; ++player) {
					type->Stats[player].Variables[variable_index].Max = type->MapDefaultStat.Variables[variable_index].Max;
				}
			} else if (variable_type == "Increase") {
				type->MapDefaultStat.Variables[variable_index].Increase = value;
				for (int player = 0; player < PlayerMax; ++player) {
					type->Stats[player].Variables[variable_index].Increase = type->MapDefaultStat.Variables[variable_index].Increase;
				}
			} else if (variable_type == "Enable") {
				type->MapDefaultStat.Variables[variable_index].Enable = value;
				for (int player = 0; player < PlayerMax; ++player) {
					type->Stats[player].Variables[variable_index].Enable = type->MapDefaultStat.Variables[variable_index].Enable;
				}
			} else {
				fprintf(stderr, "Invalid type: %s\n", variable_type.c_str());
				return;
			}
		} else {
			fprintf(stderr, "Invalid variable: %s\n", variable_key.c_str());
			return;
		}
	}
}

/**
**  Set the map sound for a unit type
**
**  @param ident			Unit type ident
**  @param sound_type		Type of the sound
**  @param sound			The sound to be set for that type
*/
void SetMapSound(std::string ident, std::string sound, std::string sound_type, std::string sound_subtype)
{
	if (sound.empty()) {
		return;
	}
	CUnitType *type = UnitTypeByIdent(ident.c_str());
	
	if (sound_type == "selected") {
		type->MapSound.Selected.Name = sound;
	} else if (sound_type == "acknowledge") {
		type->MapSound.Acknowledgement.Name = sound;
	} else if (sound_type == "attack") {
		type->MapSound.Attack.Name = sound;
	//Wyrmgus start
	} else if (sound_type == "idle") {
		type->MapSound.Idle.Name = sound;
	} else if (sound_type == "hit") {
		type->MapSound.Hit.Name = sound;
	} else if (sound_type == "miss") {
		type->MapSound.Miss.Name = sound;
	} else if (sound_type == "step") {
		type->MapSound.Step.Name = sound;
	} else if (sound_type == "step-dirt") {
		type->MapSound.StepDirt.Name = sound;
	} else if (sound_type == "step-grass") {
		type->MapSound.StepGrass.Name = sound;
	} else if (sound_type == "step-gravel") {
		type->MapSound.StepGravel.Name = sound;
	} else if (sound_type == "step-mud") {
		type->MapSound.StepMud.Name = sound;
	} else if (sound_type == "step-stone") {
		type->MapSound.StepStone.Name = sound;
	} else if (sound_type == "used") {
		type->MapSound.Used.Name = sound;
	//Wyrmgus end
	} else if (sound_type == "build") {
		type->MapSound.Build.Name = sound;
	} else if (sound_type == "ready") {
		type->MapSound.Ready.Name = sound;
	} else if (sound_type == "repair") {
		type->MapSound.Repair.Name = sound;
	} else if (sound_type == "harvest") {
		const int resId = GetResourceIdByName(sound_subtype.c_str());
		type->MapSound.Harvest[resId].Name = sound;
	} else if (sound_type == "help") {
		type->MapSound.Help.Name = sound;
	} else if (sound_type == "dead") {
		int death;

		for (death = 0; death < ANIMATIONS_DEATHTYPES; ++death) {
			if (sound_subtype == ExtraDeathTypes[death]) {
				break;
			}
		}
		if (death == ANIMATIONS_DEATHTYPES) {
			type->MapSound.Dead[ANIMATIONS_DEATHTYPES].Name = sound;
		} else {
			type->MapSound.Dead[death].Name = sound;
		}
	}
}

/**
**  Register CCL features for unit-type.
*/
void UnitTypeCclRegister()
{
	lua_register(Lua, "DefineUnitType", CclDefineUnitType);
	lua_register(Lua, "DefineUnitStats", CclDefineUnitStats);
	lua_register(Lua, "DefineBoolFlags", CclDefineBoolFlags);
	lua_register(Lua, "DefineVariables", CclDefineVariables);
	lua_register(Lua, "DefineDecorations", CclDefineDecorations);

	lua_register(Lua, "DefineExtraDeathTypes", CclDefineExtraDeathTypes);

	UnitTypeVar.Init();

	lua_register(Lua, "UnitType", CclUnitType);
	lua_register(Lua, "UnitTypeArray", CclUnitTypeArray);
	// unit type structure access
	lua_register(Lua, "GetUnitTypeIdent", CclGetUnitTypeIdent);
	lua_register(Lua, "GetUnitTypeName", CclGetUnitTypeName);
	lua_register(Lua, "SetUnitTypeName", CclSetUnitTypeName);
	lua_register(Lua, "GetUnitTypeData", CclGetUnitTypeData);
}

//@}
