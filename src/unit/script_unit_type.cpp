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
//      (c) Copyright 1999-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "stratagus.h"

#include "unit/unit_type.h"

#include "actions.h"
#include "animation/animation_set.h"
//Wyrmgus start
#include "character.h" //for updating levels
//Wyrmgus end
#include "editor.h"
#include "game/game.h"
#include "gender.h"
#include "item/item_slot.h"
#include "item/unique_item.h"
#include "luacallback.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/terrain_type.h"
#include "map/world.h"
//Wyrmgus start
#include "network.h" //for updating levels
//Wyrmgus end
#include "player/civilization.h"
#include "player/faction.h"
#include "player/player.h"
#include "population/employment_type.h"
#include "population/population_class.h"
//Wyrmgus start
#include "province.h"
//Wyrmgus end
#include "script.h"
#include "sound/sound.h"
#include "sound/unitsound.h"
#include "species/geological_era.h"
#include "species/species.h"
#include "species/taxon.h"
#include "species/taxonomic_rank.h"
#include "spell/spell.h"
#include "spell/status_effect.h"
#include "time/season.h"
#include "ui/button.h"
#include "ui/button_cmd.h"
#include "ui/button_level.h"
#include "ui/ui.h"
#include "unit/can_target_flag.h"
#include "unit/construction.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_domain.h"
#include "unit/unit_manager.h"
#include "unit/unit_type_variation.h"
#include "unit/variation_tag.h"
//Wyrmgus start
#include "upgrade/upgrade.h"
//Wyrmgus end
#include "util/assert_util.h"
#include "video/font.h"
#include "video/video.h"

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
static const char INEXHAUSTIBLE_KEY[] = "Inexhaustible";
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
static const char ORGANIC_KEY[] = "Organic";
static const char UNDEAD_KEY[] = "Undead";
static const char SIDEATTACK_KEY[] = "SideAttack";
static const char NOFRIENDLYFIRE_KEY[] = "NoFriendlyFire";
//Wyrmgus start
static const char TOWNHALL_KEY[] = "TownHall";
static const char MARKET_KEY[] = "Market";
static const char RECRUITHEROES_KEY[] = "RecruitHeroes";
static const char GARRISONTRAINING_KEY[] = "GarrisonTraining";
static const char INCREASESLUXURYDEMAND_KEY[] = "IncreasesLuxuryDemand";
static const char ITEM_KEY[] = "Item";
static const char POWERUP_KEY[] = "PowerUp";
static const char INVENTORY_KEY[] = "Inventory";
static const char TRAP_KEY[] = "Trap";
static const char TRADER_KEY[] = "Trader";
static const char FAUNA_KEY[] = "Fauna";
static const char PREDATOR_KEY[] = "Predator";
static const char SLIME_KEY[] = "Slime";
static const char PEOPLEAVERSION_KEY[] = "PeopleAversion";
static const char NEUTRAL_HOSTILE_KEY[] = "NeutralHostile";
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
static const char ETHEREAL_KEY[] = "Ethereal";
static const char CELESTIAL_BODY_KEY[] = "CelestialBody";
static const char CAPTURABLE_KEY[] = "Capturable";
static const char HIDDENOWNERSHIP_KEY[] = "HiddenOwnership";
static const char HIDDENINEDITOR_KEY[] = "HiddenInEditor";
static const char INVERTEDSOUTHEASTARMS_KEY[] = "InvertedSoutheastArms";
static const char INVERTEDEASTARMS_KEY[] = "InvertedEastArms";
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
static const char ACIDDAMAGE_KEY[] = "AcidDamage";
static const char SHADOW_DAMAGE_KEY[] = "ShadowDamage";
static const char SPEED_KEY[] = "Speed";
static const char FIRERESISTANCE_KEY[] = "FireResistance";
static const char COLDRESISTANCE_KEY[] = "ColdResistance";
static const char ARCANERESISTANCE_KEY[] = "ArcaneResistance";
static const char LIGHTNINGRESISTANCE_KEY[] = "LightningResistance";
static const char AIRRESISTANCE_KEY[] = "AirResistance";
static const char EARTHRESISTANCE_KEY[] = "EarthResistance";
static const char WATERRESISTANCE_KEY[] = "WaterResistance";
static const char ACIDRESISTANCE_KEY[] = "AcidResistance";
static const char SHADOW_RESISTANCE_KEY[] = "ShadowResistance";
static const char HACKRESISTANCE_KEY[] = "HackResistance";
static const char PIERCERESISTANCE_KEY[] = "PierceResistance";
static const char BLUNTRESISTANCE_KEY[] = "BluntResistance";
static const char DEHYDRATIONIMMUNITY_KEY[] = "DehydrationImmunity";
//Wyrmgus end
static const char POSX_KEY[] = "PosX";
static const char POSY_KEY[] = "PosY";
static const char TARGETPOSX_KEY[] = "TargetPosX";
static const char TARGETPOSY_KEY[] = "TargetPosY";
static const char RADARRANGE_KEY[] = "RadarRange";
static const char RADARJAMMERRANGE_KEY[] = "RadarJammerRange";
static const char AUTOREPAIRRANGE_KEY[] = "AutoRepairRange";
static const char SLOT_KEY[] = "Slot";
static const char SHIELD_KEY[] = "ShieldPoints";
static const char POINTS_KEY[] = "Points";
static const char MAXHARVESTERS_KEY[] = "MaxHarvesters";
static const char SHIELDPERMEABILITY_KEY[] = "ShieldPermeability";
static const char SHIELDPIERCING_KEY[] = "ShieldPiercing";
static const char ISALIVE_KEY[] = "IsAlive";
static const char PLAYER_KEY[] = "Player";
static const char PRIORITY_KEY[] = "Priority";
//Wyrmgus start
static const char STRENGTH_KEY[] = "Strength";
static const char DEXTERITY_KEY[] = "Dexterity";
static const char INTELLIGENCE_KEY[] = "Intelligence";
static const char CHARISMA_KEY[] = "Charisma";
static const char ACCURACY_KEY[] = "Accuracy";
static const char EVASION_KEY[] = "Evasion";
static const char LEVEL_KEY[] = "Level";
static const char LEVELUP_KEY[] = "LevelUp";
static const char XPREQUIRED_KEY[] = "XPRequired";
static const char VARIATION_KEY[] = "Variation";
static const char HITPOINTHEALING_KEY[] = "HitPointHealing";
static const char HITPOINTBONUS_KEY[] = "HitPointBonus";
static const char MANA_RESTORATION_KEY[] = "ManaRestoration";
static const char CRITICALSTRIKECHANCE_KEY[] = "CriticalStrikeChance";
static const char CHARGEBONUS_KEY[] = "ChargeBonus";
static const char BACKSTAB_KEY[] = "Backstab";
static const char BONUSAGAINSTMOUNTED_KEY[] = "BonusAgainstMounted";
static const char BONUSAGAINSTBUILDINGS_KEY[] = "BonusAgainstBuildings";
static const char BONUSAGAINSTAIR_KEY[] = "BonusAgainstAir";
static const char BONUSAGAINSTGIANTS_KEY[] = "BonusAgainstGiants";
static const char BONUSAGAINSTDRAGONS_KEY[] = "BonusAgainstDragons";
static const char DAYSIGHTRANGEBONUS_KEY[] = "DaySightRangeBonus";
static const char NIGHTSIGHTRANGEBONUS_KEY[] = "NightSightRangeBonus";
static const char KNOWLEDGEMAGIC_KEY[] = "KnowledgeMagic";
static const char KNOWLEDGEWARFARE_KEY[] = "KnowledgeWarfare";
static const char KNOWLEDGEMINING_KEY[] = "KnowledgeMining";
static const char MAGICLEVEL_KEY[] = "MagicLevel";
static const char TRANSPARENCY_KEY[] = "Transparency";
static const char GENDER_KEY[] = "Gender";
static const char BIRTHCYCLE_KEY[] = "BirthCycle";
static const char TIMEEFFICIENCYBONUS_KEY[] = "TimeEfficiencyBonus";
static const char RESEARCHSPEEDBONUS_KEY[] = "ResearchSpeedBonus";
static const char GARRISONEDRANGEBONUS_KEY[] = "GarrisonedRangeBonus";
static const char SPEEDBONUS_KEY[] = "SpeedBonus";
static const char RAIL_SPEED_BONUS_KEY[] = "RailSpeedBonus";
static const char GATHERINGBONUS_KEY[] = "GatheringBonus";
static const char COPPERGATHERINGBONUS_KEY[] = "CopperGatheringBonus";
static const char SILVERGATHERINGBONUS_KEY[] = "SilverGatheringBonus";
static const char GOLDGATHERINGBONUS_KEY[] = "GoldGatheringBonus";
static const char IRONGATHERINGBONUS_KEY[] = "IronGatheringBonus";
static const char MITHRILGATHERINGBONUS_KEY[] = "MithrilGatheringBonus";
static const char LUMBERGATHERINGBONUS_KEY[] = "LumberGatheringBonus";
static const char STONEGATHERINGBONUS_KEY[] = "StoneGatheringBonus";
static const char COALGATHERINGBONUS_KEY[] = "CoalGatheringBonus";
static const char JEWELRYGATHERINGBONUS_KEY[] = "JewelryGatheringBonus";
static const char FURNITUREGATHERINGBONUS_KEY[] = "FurnitureGatheringBonus";
static const char LEATHERGATHERINGBONUS_KEY[] = "LeatherGatheringBonus";
static const char GEMSGATHERINGBONUS_KEY[] = "GemsGatheringBonus";
static const char DISEMBARKMENTBONUS_KEY[] = "DisembarkmentBonus";
static const char TRADECOST_KEY[] = "TradeCost";
static const char SALVAGEFACTOR_KEY[] = "SalvageFactor";
static const char MUGGING_KEY[] = "Mugging";
static const char RAIDING_KEY[] = "Raiding";
static const char DESERTSTALK_KEY[] = "Desertstalk";
static const char FORESTSTALK_KEY[] = "Foreststalk";
static const char SWAMPSTALK_KEY[] = "Swampstalk";
static const char AURA_RANGE_BONUS_KEY[] = "AuraRangeBonus";
static const char LEADERSHIPAURA_KEY[] = "LeadershipAura";
static const char REGENERATIONAURA_KEY[] = "RegenerationAura";
static const char HYDRATINGAURA_KEY[] = "HydratingAura";
static const char ETHEREALVISION_KEY[] = "EtherealVision";
static const char HERO_KEY[] = "Hero";
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CUnitTypeVar::CBoolKeys::CBoolKeys()
{

	static constexpr std::array<const char *, NBARALREADYDEFINED> tmp = {
		COWARD_KEY, BUILDING_KEY, FLIP_KEY, REVEALER_KEY,
		LANDUNIT_KEY, AIRUNIT_KEY, SEAUNIT_KEY, EXPLODEWHENKILLED_KEY,
		VISIBLEUNDERFOG_KEY, PERMANENTCLOAK_KEY, DETECTCLOAK_KEY,
		ATTACKFROMTRANSPORTER_KEY, VANISHES_KEY, GROUNDATTACK_KEY,
		//Wyrmgus start
//		SHOREBUILDING_KEY, CANATTACK_KEY, BUILDEROUTSIDE_KEY,
		SHOREBUILDING_KEY, CANATTACK_KEY, CANDOCK_KEY, BUILDEROUTSIDE_KEY,
		//Wyrmgus end
		BUILDERLOST_KEY, CANHARVEST_KEY, INEXHAUSTIBLE_KEY, HARVESTER_KEY, SELECTABLEBYRECTANGLE_KEY,
		ISNOTSELECTABLE_KEY, DECORATION_KEY, INDESTRUCTIBLE_KEY, TELEPORTER_KEY, SHIELDPIERCE_KEY,
		//Wyrmgus start
//		SAVECARGO_KEY, NONSOLID_KEY, WALL_KEY, NORANDOMPLACING_KEY, ORGANIC_KEY
		SAVECARGO_KEY, NONSOLID_KEY, WALL_KEY, NORANDOMPLACING_KEY, ORGANIC_KEY, UNDEAD_KEY, SIDEATTACK_KEY, NOFRIENDLYFIRE_KEY,
		TOWNHALL_KEY, MARKET_KEY, RECRUITHEROES_KEY, GARRISONTRAINING_KEY, INCREASESLUXURYDEMAND_KEY, ITEM_KEY, POWERUP_KEY, INVENTORY_KEY, TRAP_KEY,
		TRADER_KEY,
		FAUNA_KEY, PREDATOR_KEY, SLIME_KEY, PEOPLEAVERSION_KEY, NEUTRAL_HOSTILE_KEY, MOUNTED_KEY, DIMINUTIVE_KEY, GIANT_KEY, DRAGON_KEY,
		DETRITUS_KEY, FLESH_KEY, VEGETABLE_KEY, INSECT_KEY, DAIRY_KEY,
		DETRITIVORE_KEY, CARNIVORE_KEY, HERBIVORE_KEY, INSECTIVORE_KEY,
		HARVESTFROMOUTSIDE_KEY, OBSTACLE_KEY, AIRUNPASSABLE_KEY, SLOWS_KEY, GRAVEL_KEY,
		HACKDAMAGE_KEY, PIERCEDAMAGE_KEY, BLUNTDAMAGE_KEY,
		ETHEREAL_KEY, CELESTIAL_BODY_KEY, CAPTURABLE_KEY, HIDDENOWNERSHIP_KEY, HIDDENINEDITOR_KEY, INVERTEDSOUTHEASTARMS_KEY, INVERTEDEASTARMS_KEY
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

	static constexpr std::array<const char *, NVARALREADYDEFINED> tmp = {
		HITPOINTS_KEY, BUILD_KEY, MANA_KEY, TRANSPORT_KEY,
		RESEARCH_KEY, TRAINING_KEY, UPGRADETO_KEY, GIVERESOURCE_KEY,
		CARRYRESOURCE_KEY, XP_KEY, KILL_KEY,	SUPPLY_KEY, DEMAND_KEY, ARMOR_KEY,
		SIGHTRANGE_KEY, ATTACKRANGE_KEY, PIERCINGDAMAGE_KEY,
		//Wyrmgus start
//		BASICDAMAGE_KEY, POSX_KEY, POSY_KEY, TARGETPOSX_KEY, TARGETPOSY_KEY, RADARRANGE_KEY,
		BASICDAMAGE_KEY, THORNSDAMAGE_KEY,
		FIREDAMAGE_KEY, COLDDAMAGE_KEY, ARCANEDAMAGE_KEY, LIGHTNINGDAMAGE_KEY, AIRDAMAGE_KEY, EARTHDAMAGE_KEY, WATERDAMAGE_KEY, ACIDDAMAGE_KEY, SHADOW_DAMAGE_KEY,
		SPEED_KEY,
		FIRERESISTANCE_KEY, COLDRESISTANCE_KEY, ARCANERESISTANCE_KEY, LIGHTNINGRESISTANCE_KEY, AIRRESISTANCE_KEY, EARTHRESISTANCE_KEY, WATERRESISTANCE_KEY, ACIDRESISTANCE_KEY, SHADOW_RESISTANCE_KEY,
		HACKRESISTANCE_KEY, PIERCERESISTANCE_KEY, BLUNTRESISTANCE_KEY, DEHYDRATIONIMMUNITY_KEY,
		POSX_KEY, POSY_KEY, TARGETPOSX_KEY, TARGETPOSY_KEY, RADARRANGE_KEY,
		//Wyrmgus end
		RADARJAMMERRANGE_KEY, AUTOREPAIRRANGE_KEY, SLOT_KEY, SHIELD_KEY, POINTS_KEY,
		MAXHARVESTERS_KEY, SHIELDPERMEABILITY_KEY, SHIELDPIERCING_KEY, ISALIVE_KEY, PLAYER_KEY,
		//Wyrmgus
//		PRIORITY_KEY
		PRIORITY_KEY,
		STRENGTH_KEY, DEXTERITY_KEY, INTELLIGENCE_KEY, CHARISMA_KEY,
		ACCURACY_KEY, EVASION_KEY, LEVEL_KEY, LEVELUP_KEY, XPREQUIRED_KEY, VARIATION_KEY,
		HITPOINTHEALING_KEY, HITPOINTBONUS_KEY, MANA_RESTORATION_KEY,
		CRITICALSTRIKECHANCE_KEY, CHARGEBONUS_KEY, BACKSTAB_KEY,
		BONUSAGAINSTMOUNTED_KEY, BONUSAGAINSTBUILDINGS_KEY, BONUSAGAINSTAIR_KEY, BONUSAGAINSTGIANTS_KEY, BONUSAGAINSTDRAGONS_KEY,
		DAYSIGHTRANGEBONUS_KEY, NIGHTSIGHTRANGEBONUS_KEY,
		KNOWLEDGEMAGIC_KEY, KNOWLEDGEWARFARE_KEY, KNOWLEDGEMINING_KEY,
		MAGICLEVEL_KEY, TRANSPARENCY_KEY, GENDER_KEY, BIRTHCYCLE_KEY,
		TIMEEFFICIENCYBONUS_KEY, RESEARCHSPEEDBONUS_KEY, GARRISONEDRANGEBONUS_KEY, SPEEDBONUS_KEY, RAIL_SPEED_BONUS_KEY,
		GATHERINGBONUS_KEY, COPPERGATHERINGBONUS_KEY, SILVERGATHERINGBONUS_KEY, GOLDGATHERINGBONUS_KEY, IRONGATHERINGBONUS_KEY, MITHRILGATHERINGBONUS_KEY, LUMBERGATHERINGBONUS_KEY, STONEGATHERINGBONUS_KEY, COALGATHERINGBONUS_KEY, JEWELRYGATHERINGBONUS_KEY, FURNITUREGATHERINGBONUS_KEY, LEATHERGATHERINGBONUS_KEY, GEMSGATHERINGBONUS_KEY,
		DISEMBARKMENTBONUS_KEY, TRADECOST_KEY, SALVAGEFACTOR_KEY, MUGGING_KEY, RAIDING_KEY,
		DESERTSTALK_KEY, FORESTSTALK_KEY, SWAMPSTALK_KEY,
		AURA_RANGE_BONUS_KEY, LEADERSHIPAURA_KEY, REGENERATIONAURA_KEY, HYDRATINGAURA_KEY,
		ETHEREALVISION_KEY, HERO_KEY
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
static void ParseBuildingRules(lua_State *l, std::vector<std::unique_ptr<CBuildRestriction>> &blist)
{
	const int args = lua_rawlen(l, -1);
	assert_throw(!(args & 1)); // must be even

	for (int i = 0; i < args; ++i) {
		const char *value = LuaToString(l, -1, i + 1);
		++i;
		lua_rawgeti(l, -1, i + 1);
		if (!lua_istable(l, -1)) {
			LuaError(l, "incorrect argument");
		}
		if (!strcmp(value, "distance")) {
			auto b = std::make_unique<CBuildRestrictionDistance>();

			for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
				value = LuaToString(l, -2);
				if (!strcmp(value, "Distance")) {
					b->Distance = LuaToNumber(l, -1);
				} else if (!strcmp(value, "DistanceType")) {
					value = LuaToString(l, -1);
					if (value[0] == '=') {
						if ((value[1] == '=' && value[2] == '\0') || (value[1] == '\0')) {
							b->DistanceType = DistanceTypeType::Equal;
						}
					} else if (value[0] == '>') {
						if (value[1] == '=' && value[2] == '\0') {
							b->DistanceType = DistanceTypeType::GreaterThanEqual;
						} else if (value[1] == '\0') {
							b->DistanceType = DistanceTypeType::GreaterThan;
						}
					} else if (value[0] == '<') {
						if (value[1] == '=' && value[2] == '\0') {
							b->DistanceType = DistanceTypeType::LessThanEqual;
						} else if (value[1] == '\0') {
							b->DistanceType = DistanceTypeType::LessThan;
						}
					} else if (value[0] == '!' && value[1] == '=' && value[2] == '\0') {
						b->DistanceType = DistanceTypeType::NotEqual;
					}
				} else if (!strcmp(value, "Type")) {
					b->RestrictTypeName = LuaToString(l, -1);
				} else if (!strcmp(value, "Class")) {
					b->restrict_class_name = LuaToString(l, -1);
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
			blist.push_back(std::move(b));
		} else if (!strcmp(value, "addon")) {
			auto b = std::make_unique<CBuildRestrictionAddOn>();

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
			blist.push_back(std::move(b));
		} else if (!strcmp(value, "ontop")) {
			auto b = std::make_unique<CBuildRestrictionOnTop>();

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
			blist.push_back(std::move(b));
		} else if (!strcmp(value, "has-unit")) {
			auto b = std::make_unique<CBuildRestrictionHasUnit>();

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
							b->CountType = DistanceTypeType::Equal;
						}
					} else if (value[0] == '>') {
						if (value[1] == '=' && value[2] == '\0') {
							b->CountType = DistanceTypeType::GreaterThanEqual;
						} else if (value[1] == '\0') {
							b->CountType = DistanceTypeType::GreaterThan;
						}
					} else if (value[0] == '<') {
						if (value[1] == '=' && value[2] == '\0') {
							b->CountType = DistanceTypeType::LessThanEqual;
						} else if (value[1] == '\0') {
							b->CountType = DistanceTypeType::LessThan;
						}
					} else if (value[0] == '!' && value[1] == '=' && value[2] == '\0') {
						b->CountType = DistanceTypeType::NotEqual;
					}
				} else {
					LuaError(l, "Unsupported BuildingRules has-unit tag: %s" _C_ value);
				}
			}
			blist.push_back(std::move(b));
		} else if (!strcmp(value, "surrounded-by")) {
			auto b = std::make_unique<CBuildRestrictionSurroundedBy>();

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
							b->CountType = DistanceTypeType::Equal;
						}
					} else if (value[0] == '>') {
						if (value[1] == '=' && value[2] == '\0') {
							b->CountType = DistanceTypeType::GreaterThanEqual;
						} else if (value[1] == '\0') {
							b->CountType = DistanceTypeType::GreaterThan;
						}
					} else if (value[0] == '<') {
						if (value[1] == '=' && value[2] == '\0') {
							b->CountType = DistanceTypeType::LessThanEqual;
						} else if (value[1] == '\0') {
							b->CountType = DistanceTypeType::LessThan;
						}
					} else if (value[0] == '!' && value[1] == '=' && value[2] == '\0') {
						b->CountType = DistanceTypeType::NotEqual;
					}
				} else if (!strcmp(value, "Distance")) {
					b->Distance = LuaToNumber(l, -1);
				} else if (!strcmp(value, "DistanceType")) {
					value = LuaToString(l, -1);
					if (value[0] == '=') {
						if ((value[1] == '=' && value[2] == '\0') || (value[1] == '\0')) {
							b->DistanceType = DistanceTypeType::Equal;
						}
					} else if (value[0] == '>') {
						if (value[1] == '=' && value[2] == '\0') {
							b->DistanceType = DistanceTypeType::GreaterThanEqual;
						} else if (value[1] == '\0') {
							b->DistanceType = DistanceTypeType::GreaterThan;
						}
					} else if (value[0] == '<') {
						if (value[1] == '=' && value[2] == '\0') {
							b->DistanceType = DistanceTypeType::LessThanEqual;
						} else if (value[1] == '\0') {
							b->DistanceType = DistanceTypeType::LessThan;
						}
					} else if (value[0] == '!' && value[1] == '=' && value[2] == '\0') {
						b->DistanceType = DistanceTypeType::NotEqual;
					}
				} else if (!strcmp(value, "Owner")) {
					b->RestrictTypeOwner = LuaToString(l, -1);
				} else if (!strcmp(value, "CheckBuilder")) {
					b->CheckBuilder = LuaToBoolean(l, -1);
				} else {
					LuaError(l, "Unsupported BuildingRules surrounded-by tag: %s" _C_ value);
				}
			}
			blist.push_back(std::move(b));
		} else if (!strcmp(value, "terrain")) {
			auto b = std::make_unique<CBuildRestrictionTerrain>();

			for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
				value = LuaToString(l, -2);
				if (!strcmp(value, "Terrain")) {
					b->RestrictTerrainTypeName = LuaToString(l, -1);
				} else {
					LuaError(l, "Unsupported BuildingRules terrain tag: %s" _C_ value);
				}
			}

			blist.push_back(std::move(b));
		} else if (!strcmp(value, "and")) {
			auto b = std::make_unique<CBuildRestrictionAnd>();
			ParseBuildingRules(l, b->and_list);
			blist.push_back(std::move(b));
		} else if (!strcmp(value, "or")) {
			auto b = std::make_unique<CBuildRestrictionOr>();
			ParseBuildingRules(l, b->or_list);
			blist.push_back(std::move(b));
		} else {
			LuaError(l, "Unsupported BuildingRules tag: %s" _C_ value);
		}
		lua_pop(l, 1);
	}
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
	wyrmgus::unit_type *type = wyrmgus::unit_type::try_get(str);
	const bool redefine = type != nullptr;
	if (redefine) {
		//Wyrmgus start
		type->RemoveButtons(ButtonCmd::Move);
		type->RemoveButtons(ButtonCmd::Stop);
		type->RemoveButtons(ButtonCmd::Attack);
		type->RemoveButtons(ButtonCmd::Patrol);
		type->RemoveButtons(ButtonCmd::StandGround);
		type->RemoveButtons(ButtonCmd::Return);
		//Wyrmgus end
	} else {
		type = wyrmgus::unit_type::add(str, nullptr);
	}

	//  Parse the list: (still everything could be changed!)
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		if (!strcmp(value, "Name")) {
			type->set_name(LuaToString(l, -1));
		//Wyrmgus start
		} else if (!strcmp(value, "Parent")) {
			const std::string parent_ident = LuaToString(l, -1);
			const wyrmgus::unit_type *parent_type = wyrmgus::unit_type::get(parent_ident);
			type->set_parent(parent_type);
		} else if (!strcmp(value, "Template")) {
			type->template_type = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Variations")) {
			type->DefaultStat.Variables[VARIATION_INDEX].Enable = 1;
			type->DefaultStat.Variables[VARIATION_INDEX].Value = 0;
			//remove previously defined variations, if any
			type->variations.clear();
			//remove previously defined layer variations, if any
			for (int i = 0; i < MaxImageLayers; ++i) {
				type->LayerVariations[i].clear();
			}
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				qunique_ptr<unit_type_variation> variation;
				int image_layer = -1;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for variations)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "layer")) {
						std::string image_layer_name = LuaToString(l, -1, k + 1);
						image_layer = GetImageLayerIdByName(image_layer_name);
					} else if (!strcmp(value, "variation-id")) {
						const std::string variation_identifier = LuaToString(l, -1, k + 1);
						variation = make_qunique<unit_type_variation>(variation_identifier, type, image_layer);
						variation->moveToThread(QApplication::instance()->thread());
					} else if (!strcmp(value, "name")) {
						variation->name = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "type-name")) {
						variation->type_name = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "button-key")) {
						variation->button_key = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "file")) {
						variation->image_file = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "file-when-loaded")) {
						const int res = GetResourceIdByName(LuaToString(l, -1, k + 1));
						++k;
						variation->FileWhenLoaded[res] = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "file-when-empty")) {
						const int res = GetResourceIdByName(LuaToString(l, -1, k + 1));
						++k;
						variation->FileWhenEmpty[res] = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "shadow-file")) {
						variation->ShadowFile = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "light-file")) {
						variation->LightFile = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "layer-file")) {
						const int layer_file_image_layer = GetImageLayerIdByName(LuaToString(l, -1, k + 1));
						++k;
						variation->LayerFiles[layer_file_image_layer] = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "frame-size")) {
						lua_rawgeti(l, -1, k + 1);
						Vec2i frame_size(0, 0);
						CclGetPos(l, &frame_size.x, &frame_size.y);
						lua_pop(l, 1);
						variation->frame_size = frame_size;
					} else if (!strcmp(value, "icon")) {
						variation->Icon.Name = LuaToString(l, -1, k + 1);
						variation->Icon.Icon = nullptr;
						variation->Icon.Load();
					} else if (!strcmp(value, "button-icon")) {
						const ButtonCmd button_action = GetButtonActionIdByName(LuaToString(l, -1, k + 1));
						++k;
						variation->ButtonIcons[button_action].Name = LuaToString(l, -1, k + 1);
						variation->ButtonIcons[button_action].Icon = nullptr;
						variation->ButtonIcons[button_action].Load();
					} else if (!strcmp(value, "animations")) {
						variation->animation_set = animation_set::get(LuaToString(l, -1, k + 1));
					} else if (!strcmp(value, "construction")) {
						variation->construction = construction::get(LuaToString(l, -1, k + 1));
					} else if (!strcmp(value, "upgrade-required")) {
						const std::string upgrade_ident = LuaToString(l, -1, k + 1);
						const CUpgrade *upgrade = CUpgrade::try_get(upgrade_ident);
						if (upgrade != nullptr) {
							variation->UpgradesRequired.push_back(upgrade);
						} else {
							variation->UpgradesRequired.push_back(CUpgrade::add(upgrade_ident, nullptr)); //if this upgrade doesn't exist, define it now (this is useful if the unit type is defined before the upgrade)
						}
					} else if (!strcmp(value, "upgrade-forbidden")) {
						const std::string upgrade_ident = LuaToString(l, -1, k + 1);
						const CUpgrade *upgrade = CUpgrade::try_get(upgrade_ident);
						if (upgrade != nullptr) {
							variation->UpgradesForbidden.push_back(upgrade);
						} else {
							variation->UpgradesForbidden.push_back(CUpgrade::add(upgrade_ident, nullptr)); //if this upgrade doesn't exist, define it now (this is useful if the unit type is defined before the upgrade)
						}
					} else if (!strcmp(value, "item-class-equipped")) {
						const std::string item_class_ident = LuaToString(l, -1, k + 1);
						const wyrmgus::item_class item_class = wyrmgus::string_to_item_class(item_class_ident);
						variation->item_classes_equipped.insert(item_class);
					} else if (!strcmp(value, "item-class-not-equipped")) {
						const std::string item_class_ident = LuaToString(l, -1, k + 1);
						const wyrmgus::item_class item_class = wyrmgus::string_to_item_class(item_class_ident);
						variation->item_classes_not_equipped.insert(item_class);
					} else if (!strcmp(value, "item-equipped")) {
						const std::string type_ident = LuaToString(l, -1, k + 1);
						const wyrmgus::unit_type *item_type = wyrmgus::unit_type::get(type_ident);
						variation->ItemsEquipped.push_back(item_type);
					} else if (!strcmp(value, "item-not-equipped")) {
						const std::string type_ident = LuaToString(l, -1, k + 1);
						const wyrmgus::unit_type *item_type = wyrmgus::unit_type::get(type_ident);
						variation->ItemsNotEquipped.push_back(item_type);
					} else if (!strcmp(value, "terrain")) {
						std::string terrain_ident = LuaToString(l, -1, k + 1);
						const wyrmgus::terrain_type *terrain = wyrmgus::terrain_type::get(terrain_ident);
						variation->Terrains.push_back(terrain);
					} else if (!strcmp(value, "terrain-forbidden")) {
						std::string terrain_ident = LuaToString(l, -1, k + 1);
						const wyrmgus::terrain_type *terrain = wyrmgus::terrain_type::get(terrain_ident);
						variation->TerrainsForbidden.push_back(terrain);
					} else if (!strcmp(value, "season")) {
						const std::string season_ident = LuaToString(l, -1, k + 1);
						wyrmgus::season *season = wyrmgus::season::get(season_ident);
						variation->Seasons.push_back(season);
					} else if (!strcmp(value, "forbidden-season")) {
						const std::string season_ident = LuaToString(l, -1, k + 1);
						wyrmgus::season *season = wyrmgus::season::get(season_ident);
						variation->ForbiddenSeasons.push_back(season);
					} else if (!strcmp(value, "resource-min")) {
						variation->ResourceMin = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "resource-max")) {
						variation->ResourceMax = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "weight")) {
						variation->Weight = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "tag")) {
						const std::string tag_identifier = LuaToString(l, -1, k + 1);
						const variation_tag *tag = variation_tag::get(tag_identifier);
						variation->tags.insert(tag);
					} else {
						printf("\n%s\n", type->get_name().c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}

				if (variation->ImageLayer != -1) {
					type->LayerVariations[variation->ImageLayer].push_back(std::move(variation));
				} else {
					type->variations.push_back(std::move(variation));
				}

				// assert_throw(variation->VariationId);
				lua_pop(l, 1);
			}
			
			type->DefaultStat.Variables[VARIATION_INDEX].Max = type->variations.size();
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
					type->image_file = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "size")) {
					lua_rawgeti(l, -1, k + 1);
					Vec2i frame_size;
					CclGetPos(l, &frame_size.x, &frame_size.y);
					type->frame_size = frame_size;
					lua_pop(l, 1);
				} else {
					LuaError(l, "Unsupported image tag: %s" _C_ value);
				}
			}
			if (redefine && type->Sprite != nullptr) {
				type->Sprite.reset();
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
				type->ShadowSprite.reset();
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
				type->LightSprite.reset();
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
				if (redefine && type->LayerSprites[image_layer] != nullptr) {
					type->LayerSprites[image_layer].reset();
				}
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "ButtonIcons")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					const ButtonCmd button_action = GetButtonActionIdByName(LuaToString(l, -1, k + 1));
					++k;
					type->ButtonIcons[button_action].Name = LuaToString(l, -1, k + 1);
					type->ButtonIcons[button_action].Icon = nullptr;
					type->ButtonIcons[button_action].Load();
				}
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "DefaultEquipment")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					const wyrmgus::item_slot item_slot = wyrmgus::string_to_item_slot(LuaToString(l, -1, k + 1));
					++k;
					wyrmgus::unit_type *default_equipment = wyrmgus::unit_type::get(LuaToString(l, -1, k + 1));
					type->DefaultEquipment[item_slot] = default_equipment;
				}
				lua_pop(l, 1);
			}
		//Wyrmgus end
		} else if (!strcmp(value, "Offset")) {
			Vec2i offset;
			CclGetPos(l, &offset.x, &offset.y);
			type->offset = offset;
		} else if (!strcmp(value, "Flip")) {
			type->Flip = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Animations")) {
			type->animation_set = wyrmgus::animation_set::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Icon")) {
			type->icon = icon::get(LuaToString(l, -1));
			//Wyrmgus end
		} else if (!strcmp(value, "Costs")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				const resource *res = resource::get(LuaToString(l, -1));
				lua_pop(l, 1);
				++k;
				type->DefaultStat.set_cost(res, LuaToNumber(l, -1, k + 1));
			}
		} else if (!strcmp(value, "Storing")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				const resource *res = resource::get(LuaToString(l, -1));
				lua_pop(l, 1);
				++k;
				type->DefaultStat.set_storing(res, LuaToNumber(l, -1, k + 1));
			}
		} else if (!strcmp(value, "ImproveProduction")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				const resource *res = resource::get(LuaToString(l, -1));
				lua_pop(l, 1);
				++k;
				type->DefaultStat.set_improve_income(res, res->get_default_income() + LuaToNumber(l, -1, k + 1));
			}
		//Wyrmgus start
		} else if (!strcmp(value, "ResourceDemand")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				const resource *res = resource::get(LuaToString(l, -1));
				lua_pop(l, 1);
				++k;
				type->DefaultStat.set_resource_demand(res, LuaToNumber(l, -1, k + 1));
			}
		} else if (!strcmp(value, "UnitStock")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(LuaToString(l, -1, k + 1));
				++k;
				type->DefaultStat.set_unit_stock(unit_type, LuaToNumber(l, -1, k + 1));
			}
		//Wyrmgus end
		} else if (!strcmp(value, "Construction")) {
			type->construction = wyrmgus::construction::get(LuaToString(l, -1));
		} else if (!strcmp(value, "DrawLevel")) {
			type->draw_level = LuaToNumber(l, -1);
		} else if (!strcmp(value, "MaxOnBoard")) {
			type->MaxOnBoard = LuaToNumber(l, -1);
			//Wyrmgus start
			type->DefaultStat.Variables[TRANSPORT_INDEX].Max = type->MaxOnBoard;
			type->DefaultStat.Variables[TRANSPORT_INDEX].Enable = 1;
			//Wyrmgus end
		} else if (!strcmp(value, "BoardSize")) {
			type->BoardSize = LuaToNumber(l, -1);
		} else if (!strcmp(value, "ButtonLevelForTransporter")) {
			type->ButtonLevelForTransporter = wyrmgus::button_level::get(LuaToString(l, -1));
		//Wyrmgus start
		} else if (!strcmp(value, "ButtonPos")) {
			type->ButtonPos = LuaToNumber(l, -1);
		} else if (!strcmp(value, "ButtonLevel")) {
			type->ButtonLevel = wyrmgus::button_level::get(LuaToString(l, -1));
		} else if (!strcmp(value, "ButtonPopup")) {
			type->ButtonPopup = LuaToString(l, -1);
		} else if (!strcmp(value, "ButtonHint")) {
			type->ButtonHint = LuaToString(l, -1);
		} else if (!strcmp(value, "ButtonKey")) {
			type->button_key = LuaToString(l, -1);
		} else if (!strcmp(value, "Trains")) {
			type->RemoveButtons(ButtonCmd::Train);
			type->RemoveButtons(ButtonCmd::Build);
			for (size_t i = 0; i < type->Trains.size(); ++i) {
				if (std::find(type->Trains[i]->TrainedBy.begin(), type->Trains[i]->TrainedBy.end(), type) != type->Trains[i]->TrainedBy.end()) {
					type->Trains[i]->TrainedBy.erase(std::remove(type->Trains[i]->TrainedBy.begin(), type->Trains[i]->TrainedBy.end(), type), type->Trains[i]->TrainedBy.end());
				}
			}
			type->Trains.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				value = LuaToString(l, -1, j + 1);
				wyrmgus::unit_type *trained_unit = wyrmgus::unit_type::get(value);
				type->Trains.push_back(trained_unit);
				trained_unit->TrainedBy.push_back(type);
			}
		//Wyrmgus end
		//Wyrmgus start
//		} else if (!strcmp(value, "StartingResources")) {
//			type->StartingResources = LuaToNumber(l, -1);
		} else if (!strcmp(value, "StartingResources")) {
			type->starting_resources.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				type->starting_resources.push_back(LuaToNumber(l, -1, j + 1));
			}
		//Wyrmgus end
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
				DefineVariableField(l, type->DefaultStat.Variables[SHIELD_INDEX], -1);
			} else if (lua_isnumber(l, -1)) {
				type->DefaultStat.Variables[SHIELD_INDEX].Max = LuaToNumber(l, -1);
				type->DefaultStat.Variables[SHIELD_INDEX].Value = 0;
				type->DefaultStat.Variables[SHIELD_INDEX].Increase = 1;
				type->DefaultStat.Variables[SHIELD_INDEX].Enable = 1;
			}
		} else if (!strcmp(value, "TileSize")) {
			Vec2i tile_size;
			CclGetPos(l, &tile_size.x, &tile_size.y);
			type->tile_size = tile_size;
		} else if (!strcmp(value, "NeutralMinimapColor")) {
			CColor color;
			color.Parse(l);
			type->neutral_minimap_color = QColor(color.R, color.G, color.B);
		} else if (!strcmp(value, "BoxSize")) {
			Vec2i box_size;
			CclGetPos(l, &box_size.x, &box_size.y);
			type->box_size = box_size;
		} else if (!strcmp(value, "BoxOffset")) {
			Vec2i box_offset;
			CclGetPos(l, &box_offset.x, &box_offset.y);
			type->box_offset = box_offset;
		} else if (!strcmp(value, "NumDirections")) {
			type->num_directions = LuaToNumber(l, -1);
		//Wyrmgus start
//		} else if (!strcmp(value, "ComputerReactionRange")) {
//			type->ReactRangeComputer = LuaToNumber(l, -1);
//		} else if (!strcmp(value, "PersonReactionRange")) {
//			type->ReactRangePerson = LuaToNumber(l, -1);
		//Wyrmgus end
		} else if (!strcmp(value, "Missile")) {
			type->Missile.Name = LuaToString(l, -1);
			type->Missile.Missile = nullptr;
		//Wyrmgus start
		} else if (!strcmp(value, "FireMissile")) {
			type->FireMissile.Name = LuaToString(l, -1);
			type->FireMissile.Missile = nullptr;
		//Wyrmgus end
		} else if (!strcmp(value, "MinAttackRange")) {
			type->MinAttackRange = LuaToNumber(l, -1);
		} else if (!strcmp(value, "MaxAttackRange")) {
			type->DefaultStat.Variables[ATTACKRANGE_INDEX].Value = LuaToNumber(l, -1);
			type->DefaultStat.Variables[ATTACKRANGE_INDEX].Max = LuaToNumber(l, -1);
			type->DefaultStat.Variables[ATTACKRANGE_INDEX].Enable = 1;
		//Wyrmgus start
//		} else if (!strcmp(value, "MaxHarvesters")) {
//			type->DefaultStat.Variables[MAXHARVESTERS_INDEX].Value = LuaToNumber(l, -1);
//			type->DefaultStat.Variables[MAXHARVESTERS_INDEX].Max = LuaToNumber(l, -1);
		//Wyrmgus end
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
			type->corpse_type = wyrmgus::unit_type::get(LuaToString(l, -1));
		} else if (!strcmp(value, "DamageType")) {
			value = LuaToString(l, -1);
			//int check = ExtraDeathIndex(value);
			type->DamageType = value;
		} else if (!strcmp(value, "ExplodeWhenKilled")) {
			type->ExplodeWhenKilled = 1;
			type->Explosion.Name = LuaToString(l, -1);
			type->Explosion.Missile = nullptr;
		} else if (!strcmp(value, "TeleportCost")) {
			type->TeleportCost = LuaToNumber(l, -1);
		} else if (!strcmp(value, "TeleportEffectIn")) {
			type->TeleportEffectIn = std::make_unique<LuaCallback>(l, -1);
		} else if (!strcmp(value, "TeleportEffectOut")) {
			type->TeleportEffectOut = std::make_unique<LuaCallback>(l, -1);
		} else if (!strcmp(value, "DeathExplosion")) {
			type->DeathExplosion = std::make_unique<LuaCallback>(l, -1);
		} else if (!strcmp(value, "OnHit")) {
			type->OnHit = std::make_unique<LuaCallback>(l, -1);
		} else if (!strcmp(value, "OnEachCycle")) {
			type->OnEachCycle = std::make_unique<LuaCallback>(l, -1);
		} else if (!strcmp(value, "OnEachSecond")) {
			type->OnEachSecond = std::make_unique<LuaCallback>(l, -1);
		} else if (!strcmp(value, "OnInit")) {
			type->OnInit = std::make_unique<LuaCallback>(l, -1);
		} else if (!strcmp(value, "Domain")) {
			value = LuaToString(l, -1);
			type->domain = string_to_unit_domain(value);
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
					type->Impact[ANIMATIONS_DEATHTYPES].Missile = nullptr;
				} else if (!strcmp(dtype, "shield")) {
					type->Impact[ANIMATIONS_DEATHTYPES + 1].Name = LuaToString(l, -1, k + 1);
					type->Impact[ANIMATIONS_DEATHTYPES + 1].Missile = nullptr;
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
						type->Impact[num].Missile = nullptr;
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
			} else if (!strcmp(value, "rally_point")) {
				type->MouseAction = MouseActionRallyPoint;
			//Wyrmgus start
			} else if (!strcmp(value, "trade")) {
				type->MouseAction = MouseActionTrade;
			//Wyrmgus end
			} else {
				LuaError(l, "Unsupported RightMouseAction: %s" _C_ value);
			}
		} else if (!strcmp(value, "CanAttack")) {
			type->CanAttack = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "RepairRange")) {
			type->RepairRange = LuaToNumber(l, -1);
		} else if (!strcmp(value, "RepairHp")) {
			type->repair_hp = LuaToNumber(l, -1);
		} else if (!strcmp(value, "RepairCosts")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				const resource *res = resource::get(LuaToString(l, -1));
				lua_pop(l, 1);
				++k;
				type->repair_costs[res] = LuaToNumber(l, -1, k + 1);
			}
		} else if (!strcmp(value, "CanTargetLand")) {
			if (LuaToBoolean(l, -1)) {
				type->can_target_flags |= can_target_flag::land;
			} else {
				type->can_target_flags &= ~can_target_flag::land;
			}
		} else if (!strcmp(value, "CanTargetSea")) {
			if (LuaToBoolean(l, -1)) {
				type->can_target_flags |= can_target_flag::water;
			} else {
				type->can_target_flags &= ~can_target_flag::water;
			}
		} else if (!strcmp(value, "CanTargetAir")) {
			if (LuaToBoolean(l, -1)) {
				type->can_target_flags |= can_target_flag::air;
			} else {
				type->can_target_flags &= ~can_target_flag::air;
			}
		} else if (!strcmp(value, "BuildingRules")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			// Free any old restrictions if they are redefined
			type->BuildingRules.clear();
			ParseBuildingRules(l, type->BuildingRules);
		} else if (!strcmp(value, "AiBuildingRules")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			// Free any old restrictions if they are redefined
			type->AiBuildingRules.clear();
			ParseBuildingRules(l, type->AiBuildingRules);
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
			type->random_movement_probability = LuaToNumber(l, -1);
		} else if (!strcmp(value, "RandomMovementDistance")) {
			type->random_movement_distance = LuaToNumber(l, -1);
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
				type->DefaultStat.Variables[TRANSPORT_INDEX].Enable = 1;
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
				wyrmgus::resource_info *res = nullptr;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "resource-id")) {
						lua_rawgeti(l, -1, k + 1);
						const int resource_id = CclGetResourceByName(l);
						const resource *resource = resource::get_all()[resource_id];
						if (type->get_resource_info(resource) == nullptr) {
							type->resource_infos[resource] = std::make_unique<resource_info>(type, resource);
						}
						res = type->resource_infos[resource].get();
						lua_pop(l, 1);
					} else if (!strcmp(value, "resource-step")) {
						res->ResourceStep = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "wait-at-resource")) {
						res->WaitAtResource = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "wait-at-depot")) {
						res->WaitAtDepot = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "resource-capacity")) {
						res->ResourceCapacity = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "lose-resources")) {
						res->LoseResources = 1;
						--k;
					} else if (!strcmp(value, "file-when-empty")) {
						res->image_file = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "file-when-loaded")) {
						res->loaded_image_file = LuaToString(l, -1, k + 1);
					} else {
						printf("\n%s\n", type->get_name().c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);
			}
			type->BoolFlag[HARVESTER_INDEX].value = 1;
		} else if (!strcmp(value, "GivesResource")) {
			const std::string resource_identifier = LuaToString(l, -1);
			type->given_resource = wyrmgus::resource::get(resource_identifier);
		} else if (!strcmp(value, "PopulationCost")) {
			type->population_cost = LuaToNumber(l, -1);
		} else if (!strcmp(value, "PopulationClass")) {
			type->population_class = population_class::get(LuaToString(l, -1));
		} else if (!strcmp(value, "EmploymentType")) {
			type->employment_type = employment_type::get(LuaToString(l, -1));
		} else if (!strcmp(value, "EmploymentCapacity")) {
			type->employment_capacity = LuaToNumber(l, -1);
		} else if (!strcmp(value, "CanStore")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				type->stored_resources.insert(resource::get_all()[CclGetResourceByName(l)]);
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "CanCastSpell")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			if (subargs == 0) {
				type->Spells.clear();
			}
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, -1, k + 1);
				wyrmgus::spell *spell = wyrmgus::spell::get(value);
				type->Spells.push_back(spell);
			}
		} else if (!strcmp(value, "AutoCastActive")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}

			const int subargs = lua_rawlen(l, -1);
			if (subargs == 0) {
				type->autocast_spells.clear();
			}
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, -1, k + 1);
				const wyrmgus::spell *spell = wyrmgus::spell::get(value);
				type->add_autocast_spell(spell);
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
			if (type->sound_set == nullptr) {
				type->sound_set = std::make_unique<wyrmgus::unit_sound_set>();
			}

			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, -1, k + 1);
				++k;

				if (!strcmp(value, "selected")) {
					type->sound_set->Selected.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "acknowledge")) {
					type->sound_set->Acknowledgement.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "attack")) {
					type->sound_set->Attack.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "idle")) {
					type->sound_set->Idle.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "hit")) {
					type->sound_set->Hit.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "miss")) {
					type->sound_set->Miss.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "fire-missile")) {
					type->sound_set->FireMissile.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "step")) {
					type->sound_set->Step.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "step-dirt")) {
					type->sound_set->StepDirt.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "step-grass")) {
					type->sound_set->StepGrass.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "step-gravel")) {
					type->sound_set->StepGravel.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "step-mud")) {
					type->sound_set->StepMud.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "step-stone")) {
					type->sound_set->StepStone.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "used")) {
					type->sound_set->Used.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "build")) {
					type->sound_set->Build.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "ready")) {
					type->sound_set->Ready.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "repair")) {
					type->sound_set->Repair.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "harvest")) {
					const std::string name = LuaToString(l, -1, k + 1);
					++k;
					const int resId = GetResourceIdByName(l, name.c_str());
					type->sound_set->Harvest[resId].Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "help")) {
					type->sound_set->Help.Name = LuaToString(l, -1, k + 1);
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
						type->sound_set->Dead[ANIMATIONS_DEATHTYPES].Name = name;
					} else {
						type->sound_set->Dead[death].Name = LuaToString(l, -1, k + 1);
					}
				} else {
					LuaError(l, "Unsupported sound tag: %s" _C_ value);
				}
			}
		//Wyrmgus start
		} else if (!strcmp(value, "Class")) {
			type->set_unit_class(wyrmgus::unit_class::get(LuaToString(l, -1)));
		} else if (!strcmp(value, "Civilization")) {
			std::string civilization_name = LuaToString(l, -1);
			wyrmgus::civilization *civilization = wyrmgus::civilization::get(civilization_name);
			type->civilization = civilization;
		} else if (!strcmp(value, "Faction")) {
			const std::string faction_name = LuaToString(l, -1);
			wyrmgus::faction *faction = wyrmgus::faction::get(faction_name);
			type->faction = faction;
		} else if (!strcmp(value, "Notes")) {
			type->set_notes(LuaToString(l, -1));
		} else if (!strcmp(value, "Description")) {
			type->set_description(LuaToString(l, -1));
		} else if (!strcmp(value, "Quote")) {
			type->set_quote(LuaToString(l, -1));
		} else if (!strcmp(value, "Gender")) {
			type->DefaultStat.Variables[GENDER_INDEX].Enable = 1;
			type->DefaultStat.Variables[GENDER_INDEX].Value = static_cast<int>(wyrmgus::string_to_gender(LuaToString(l, -1)));
			type->DefaultStat.Variables[GENDER_INDEX].Max = type->DefaultStat.Variables[GENDER_INDEX].Value;
		} else if (!strcmp(value, "Background")) {
			type->set_background(LuaToString(l, -1));
		} else if (!strcmp(value, "RequirementsString")) {
			type->RequirementsString = LuaToString(l, -1);
		} else if (!strcmp(value, "BuildingRulesString")) {
			type->BuildingRulesString = LuaToString(l, -1);
		} else if (!strcmp(value, "TrainQuantity")) {
			type->TrainQuantity = LuaToNumber(l, -1);
		} else if (!strcmp(value, "CostModifier")) {
			type->CostModifier = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Elixir")) {
			const std::string elixir_ident = LuaToString(l, -1);
			type->elixir = CUpgrade::get_or_add(elixir_ident, nullptr); //if this elixir upgrade doesn't exist, define it now (this is useful if the unit type is defined before the upgrade)
		} else if (!strcmp(value, "SoldUnits")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				wyrmgus::unit_type *sold_unit_type = wyrmgus::unit_type::get(LuaToString(l, -1, j + 1));
				type->SoldUnits.push_back(sold_unit_type);
			}
		} else if (!strcmp(value, "SpawnUnits")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				wyrmgus::unit_type *spawned_unit_type = wyrmgus::unit_type::get(LuaToString(l, -1, j + 1));
				type->SpawnUnits.push_back(spawned_unit_type);
			}
		} else if (!strcmp(value, "Drops")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				wyrmgus::unit_type *drop_type = wyrmgus::unit_type::get(LuaToString(l, -1, j + 1));
				type->Drops.push_back(drop_type);
			}
		} else if (!strcmp(value, "AiDrops")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				wyrmgus::unit_type *drop_type = wyrmgus::unit_type::get(LuaToString(l, -1, j + 1));
				type->AiDrops.push_back(drop_type);
			}
		} else if (!strcmp(value, "DropSpells")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				value = LuaToString(l, -1, j + 1);
				wyrmgus::spell *spell = wyrmgus::spell::get(value);
				type->DropSpells.push_back(spell);
			}
		} else if (!strcmp(value, "Affixes")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				const std::string affix_ident = LuaToString(l, -1, j + 1);
				type->Affixes.push_back(CUpgrade::get(affix_ident));
			}
		} else if (!strcmp(value, "Traits")) {
			type->traits.clear(); // remove previously defined traits, to allow unit types to not inherit traits from their parent unit types
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				const std::string trait_ident = LuaToString(l, -1, j + 1);
				type->traits.push_back(CUpgrade::get(trait_ident));
			}
		} else if (!strcmp(value, "StartingAbilities")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string ability_ident = LuaToString(l, -1, j + 1);
				type->StartingAbilities.push_back(CUpgrade::get(ability_ident));
			}
		} else if (!strcmp(value, "ItemClass")) {
			type->item_class = wyrmgus::string_to_item_class(LuaToString(l, -1));
		} else if (!strcmp(value, "Species")) {
			type->species = wyrmgus::species::get(LuaToString(l, -1));
			type->species->set_unit_type(type);
		} else if (!strcmp(value, "TerrainType")) {
			type->TerrainType = wyrmgus::terrain_type::get(LuaToString(l, -1));
			type->TerrainType->UnitType = type;
		} else if (!strcmp(value, "WeaponClasses")) {
			type->WeaponClasses.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				type->WeaponClasses.push_back(wyrmgus::string_to_item_class(LuaToString(l, -1, j + 1)));
			}
		//Wyrmgus end
		} else {
			int index = UnitTypeVar.VariableNameLookup[value];
			if (index != -1) { // valid index
				if (lua_isboolean(l, -1)) {
					type->DefaultStat.Variables[index].Enable = LuaToBoolean(l, -1);
				} else if (lua_istable(l, -1)) {
					DefineVariableField(l, type->DefaultStat.Variables[index], -1);
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
					type->BoolFlag[index].value = (LuaToNumber(l, -1) != 0);
				} else {
					type->BoolFlag[index].value = LuaToBoolean(l, -1);
				}
			} else {
				printf("\n%s\n", type->get_name().c_str());
				LuaError(l, "Unsupported tag: %s" _C_ value);
			}
		}
	}

	type->set_defined(true);
	
	return 0;
}

/**
**  Parse unit-stats.
**
**  @param l  Lua state.
*/
static int CclDefineUnitStats(lua_State *l)
{
	wyrmgus::unit_type *type = wyrmgus::unit_type::get(LuaToString(l, 1));
	const int playerId = LuaToNumber(l, 2);

	assert_throw(playerId < PlayerMax);

	CUnitStats *stats = &type->Stats[playerId];
	if (stats->Variables.empty()) {
		stats->Variables.resize(UnitTypeVar.GetNumberVariable());
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
				const resource *res = resource::get(value);
				stats->set_cost(res, LuaToNumber(l, -1, k + 1));
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
				const resource *res = resource::get(value);
				stats->set_storing(res, LuaToNumber(l, -1, k + 1));
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
				const resource *res = resource::get(value);
				stats->set_improve_income(res, LuaToNumber(l, -1, k + 1));
				lua_pop(l, 1);
			}
		//Wyrmgus start
		} else if (!strcmp(value, "resource-demand")) {
			lua_rawgeti(l, 3, j + 1);
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);

			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, 3, j + 1);
				value = LuaToString(l, -1, k + 1);
				++k;
				const resource *res = resource::get(value);
				stats->set_resource_demand(res, LuaToNumber(l, -1, k + 1));
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "unit-stock")) {
			lua_rawgeti(l, 3, j + 1);
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);

			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, 3, j + 1);
				wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(LuaToString(l, -1, k + 1));
				++k;
				stats->set_unit_stock(unit_type, LuaToNumber(l, -1, k + 1));
				lua_pop(l, 1);
			}
		//Wyrmgus end
		} else {
			int i = UnitTypeVar.VariableNameLookup[value];// User variables
			if (i != -1) { // valid index
				lua_rawgeti(l, 3, j + 1);
				if (lua_istable(l, -1)) {
					DefineVariableField(l, stats->Variables[i], -1);
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
wyrmgus::unit_type *CclGetUnitType(lua_State *l)
{
	//Wyrmgus start
	if (lua_isnil(l, -1)) {
		return nullptr;
	}
	//Wyrmgus end
	
	// Be kind allow also strings or symbols
	if (lua_isstring(l, -1)) {
		const char *str = LuaToString(l, -1);
		return wyrmgus::unit_type::try_get(str);
	} else if (lua_isuserdata(l, -1)) {
		LuaUserData *data = (LuaUserData *)lua_touserdata(l, -1);
		if (data->Type == LuaUnitType) {
			return (wyrmgus::unit_type *)data->Data;
		}
	}
	LuaError(l, "CclGetUnitType: not a unit-type");
	return nullptr;
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
	wyrmgus::unit_type *type = wyrmgus::unit_type::get(str);
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

	for (wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
		LuaUserData *data = (LuaUserData *)lua_newuserdata(l, sizeof(LuaUserData));
		data->Type = LuaUnitType;
		data->Data = unit_type;
		lua_rawseti(l, 1, unit_type->Slot + 1);
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

	const wyrmgus::unit_type *type = CclGetUnitType(l);
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

	const wyrmgus::unit_type *type = CclGetUnitType(l);
	lua_pushstring(l, type->get_name().c_str());
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
	wyrmgus::unit_type *type = CclGetUnitType(l);
	lua_pop(l, 1);
	type->set_name(LuaToString(l, 2));

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
	std::string ident = LuaToString(l, 1);
	const wyrmgus::unit_type *type = wyrmgus::unit_type::get(ident.c_str());
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, type->get_name().c_str());
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "NamePlural")) {
		lua_pushstring(l, type->GetNamePlural().c_str());
		return 1;
	} else if (!strcmp(data, "Parent")) {
		if (type->Parent != nullptr) {
			lua_pushstring(l, type->Parent->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Class")) {
		if (type->get_item_class() != wyrmgus::item_class::none) {
			lua_pushstring(l, wyrmgus::item_class_to_string(type->get_item_class()).c_str());
		} else if (type->get_unit_class() != nullptr) {
			lua_pushstring(l, type->get_unit_class()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Civilization")) {
		if (type->get_civilization() != nullptr) {
			lua_pushstring(l, type->get_civilization()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Faction")) {
		if (type->get_faction() != nullptr) {
			lua_pushstring(l, type->get_faction()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Notes")) {
		lua_pushstring(l, type->get_notes().c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, type->get_description().c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, type->get_quote().c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, type->get_background().c_str());
		return 1;
	} else if (!strcmp(data, "RequirementsString")) {
		lua_pushstring(l, type->RequirementsString.c_str());
		return 1;
	} else if (!strcmp(data, "ExperienceRequirementsString")) {
		lua_pushstring(l, type->ExperienceRequirementsString.c_str());
		return 1;
	} else if (!strcmp(data, "BuildingRulesString")) {
		lua_pushstring(l, type->BuildingRulesString.c_str());
		return 1;
	} else if (!strcmp(data, "Image")) {
		lua_pushstring(l, type->get_image_file().string().c_str());
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Shadow")) {
		lua_pushstring(l, type->ShadowFile.c_str());
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "Width")) {
		lua_pushnumber(l, type->get_frame_size().width());
		return 1;
	} else if (!strcmp(data, "Height")) {
		lua_pushnumber(l, type->get_frame_size().height());
		return 1;
	} else if (!strcmp(data, "Animations")) {
		if (type->get_animation_set() != nullptr) {
			lua_pushstring(l, type->get_animation_set()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "Icon")) {
		if (type->get_icon() != nullptr) {
			lua_pushstring(l, type->get_icon()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Costs")) {
		LuaCheckArgs(l, 3);
		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		lua_pushnumber(l, type->DefaultStat.get_cost(resource));
		return 1;
	} else if (!strcmp(data, "ImproveProduction")) {
		LuaCheckArgs(l, 3);
		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		lua_pushnumber(l, type->DefaultStat.get_improve_income(resource));
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "UnitStock")) {
		LuaCheckArgs(l, 3);
		wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(LuaToString(l, 3));
		lua_pushnumber(l, type->DefaultStat.get_unit_stock(unit_type));
		return 1;
	} else if (!strcmp(data, "TrainQuantity")) {
		lua_pushnumber(l, type->TrainQuantity);
		return 1;
	} else if (!strcmp(data, "CostModifier")) {
		lua_pushnumber(l, type->CostModifier);
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "DrawLevel")) {
		lua_pushnumber(l, type->get_draw_level());
		return 1;
	} else if (!strcmp(data, "TileWidth")) {
		lua_pushnumber(l, type->get_tile_width());
		return 1;
	} else if (!strcmp(data, "TileHeight")) {
		lua_pushnumber(l, type->get_tile_height());
		return 1;
	} else if (!strcmp(data, "Species")) {
		if (type->get_species() != nullptr) {
			lua_pushstring(l, type->get_species()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "ItemClass")) {
		if (type->get_item_class() != wyrmgus::item_class::none) {
			lua_pushstring(l, wyrmgus::item_class_to_string(type->get_item_class()).c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "ItemSlot")) {
		const wyrmgus::item_slot item_slot = wyrmgus::get_item_class_slot(type->get_item_class());
		if (item_slot != wyrmgus::item_slot::none) {
			lua_pushstring(l, wyrmgus::item_slot_to_string(item_slot).c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "ItemSlotId")) {
		const int item_slot = static_cast<int>(wyrmgus::get_item_class_slot(type->get_item_class()));
		lua_pushnumber(l, item_slot);
		return 1;
	} else if (!strcmp(data, "WeaponClasses")) {
		lua_createtable(l, type->WeaponClasses.size(), 0);
		for (size_t i = 1; i <= type->WeaponClasses.size(); ++i)
		{
			lua_pushstring(l, wyrmgus::item_class_to_string(type->WeaponClasses[i-1]).c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
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
		lua_pushnumber(l, type->DefaultStat.Variables[ATTACKRANGE_INDEX].Value);
		return 1;
	} else if (!strcmp(data, "Priority")) {
		lua_pushnumber(l, type->DefaultStat.Variables[PRIORITY_INDEX].Value);
		return 1;
	} else if (!strcmp(data, "Domain")) {
		lua_pushstring(l, unit_domain_to_string(type->get_domain()).c_str());
		return 1;
	} else if (!strcmp(data, "Corpse")) {
		if (type->get_corpse_type() != nullptr) {
			lua_pushstring(l, type->get_corpse_type()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "CanAttack")) {
		lua_pushboolean(l, type->CanAttack);
		return 1;
	} else if (!strcmp(data, "Building")) {
		lua_pushboolean(l, type->BoolFlag[BUILDING_INDEX].value);
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Item")) {
		lua_pushboolean(l, type->BoolFlag[ITEM_INDEX].value);
		return 1;
	} else if (!strcmp(data, "ButtonPos")) {
		lua_pushnumber(l, type->ButtonPos);
		return 1;
	} else if (!strcmp(data, "ButtonKey")) {
		lua_pushstring(l, type->get_button_key().c_str());
		return 1;
	} else if (!strcmp(data, "ButtonHint")) {
		lua_pushstring(l, type->ButtonHint.c_str());
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "GivesResource")) {
		if (type->get_given_resource() != nullptr) {
			lua_pushstring(l, type->get_given_resource()->get_identifier().c_str());
			return 1;
		} else {
			lua_pushstring(l, "");
			return 1;
		}
	} else if (!strcmp(data, "Sounds")) {
		LuaCheckArgs(l, 3);
		const std::string sound_type = LuaToString(l, 3);

		const wyrmgus::unit_sound_set *sound_set = type->get_sound_set();

		if (sound_set == nullptr) {
			lua_pushstring(l, "");
			return 1;
		}

		if (sound_type == "selected") {
			lua_pushstring(l, sound_set->Selected.Name.c_str());
		} else if (sound_type == "acknowledge") {
			lua_pushstring(l, sound_set->Acknowledgement.Name.c_str());
		} else if (sound_type == "attack") {
			lua_pushstring(l, sound_set->Attack.Name.c_str());
		} else if (sound_type == "idle") {
			lua_pushstring(l, sound_set->Idle.Name.c_str());
		} else if (sound_type == "hit") {
			lua_pushstring(l, sound_set->Hit.Name.c_str());
		} else if (sound_type == "miss") {
			lua_pushstring(l, sound_set->Miss.Name.c_str());
		} else if (sound_type == "fire-missile") {
			lua_pushstring(l, sound_set->FireMissile.Name.c_str());
		} else if (sound_type == "step") {
			lua_pushstring(l, sound_set->Step.Name.c_str());
		} else if (sound_type == "step-dirt") {
			lua_pushstring(l, sound_set->StepDirt.Name.c_str());
		} else if (sound_type == "step-grass") {
			lua_pushstring(l, sound_set->StepGrass.Name.c_str());
		} else if (sound_type == "step-gravel") {
			lua_pushstring(l, sound_set->StepGravel.Name.c_str());
		} else if (sound_type == "step-mud") {
			lua_pushstring(l, sound_set->StepMud.Name.c_str());
		} else if (sound_type == "step-stone") {
			lua_pushstring(l, sound_set->StepStone.Name.c_str());
		} else if (sound_type == "used") {
			lua_pushstring(l, sound_set->Used.Name.c_str());
		} else if (sound_type == "build") {
			lua_pushstring(l, sound_set->Build.Name.c_str());
		} else if (sound_type == "ready") {
			lua_pushstring(l, sound_set->Ready.Name.c_str());
		} else if (sound_type == "repair") {
			lua_pushstring(l, sound_set->Repair.Name.c_str());
		} else if (sound_type == "harvest") {
			LuaCheckArgs(l, 4);
			const std::string sound_subtype = LuaToString(l, 4);
			const int resId = GetResourceIdByName(sound_subtype.c_str());
			lua_pushstring(l, sound_set->Harvest[resId].Name.c_str());
		} else if (sound_type == "help") {
			lua_pushstring(l, sound_set->Help.Name.c_str());
		} else if (sound_type == "dead") {
			if (lua_gettop(l) < 4) {
				lua_pushstring(l, sound_set->Dead[ANIMATIONS_DEATHTYPES].Name.c_str());
			} else {
				int death;
				const std::string sound_subtype = LuaToString(l, 4);

				for (death = 0; death < ANIMATIONS_DEATHTYPES; ++death) {
					if (sound_subtype == ExtraDeathTypes[death]) {
						break;
					}
				}
				if (death == ANIMATIONS_DEATHTYPES) {
					lua_pushstring(l, sound_set->Dead[ANIMATIONS_DEATHTYPES].Name.c_str());
				} else {
					lua_pushstring(l, sound_set->Dead[death].Name.c_str());
				}
			}
		}
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Drops")) {
		lua_createtable(l, type->Drops.size(), 0);
		for (size_t i = 1; i <= type->Drops.size(); ++i)
		{
			lua_pushstring(l, type->Drops[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "AiDrops")) {
		lua_createtable(l, type->AiDrops.size(), 0);
		for (size_t i = 1; i <= type->AiDrops.size(); ++i)
		{
			lua_pushstring(l, type->AiDrops[i - 1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Affixes")) {
		lua_createtable(l, type->Affixes.size(), 0);
		for (size_t i = 1; i <= type->Affixes.size(); ++i)
		{
			lua_pushstring(l, type->Affixes[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Droppers")) { // unit types which can drop this one
		std::vector<const wyrmgus::unit_type *> droppers;
		for (const wyrmgus::unit_type *other_unit_type : wyrmgus::unit_type::get_all()) {
			if (other_unit_type->is_template()) {
				continue;
			}

			if (
				std::find(other_unit_type->Drops.begin(), other_unit_type->Drops.end(), type) != other_unit_type->Drops.end()
				|| std::find(other_unit_type->AiDrops.begin(), other_unit_type->AiDrops.end(), type) != other_unit_type->AiDrops.end()
			) {
				droppers.push_back(other_unit_type);
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
		lua_createtable(l, type->get_traits().size(), 0);
		for (size_t i = 1; i <= type->get_traits().size(); ++i)
		{
			lua_pushstring(l, type->get_traits()[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "StartingAbilities")) {
		lua_createtable(l, type->StartingAbilities.size(), 0);
		for (size_t i = 1; i <= type->StartingAbilities.size(); ++i)
		{
			lua_pushstring(l, type->StartingAbilities[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Prefixes")) {
		std::vector<const CUpgrade *> prefixes;
		for (size_t i = 0; i < type->Affixes.size(); ++i)
		{
			if (type->Affixes[i]->is_magic_prefix()) {
				prefixes.push_back(type->Affixes[i]);
			}
		}
		if (type->get_item_class() != wyrmgus::item_class::none) {
			for (const CUpgrade *upgrade : CUpgrade::get_all()) {
				if (upgrade->is_magic_prefix() && upgrade->has_affixed_item_class(type->get_item_class())) {
					prefixes.push_back(upgrade);
				}
			}
		}
		
		lua_createtable(l, prefixes.size(), 0);
		for (size_t i = 1; i <= prefixes.size(); ++i)
		{
			lua_pushstring(l, prefixes[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Suffixes")) {
		std::vector<const CUpgrade *> suffixes;
		for (size_t i = 0; i < type->Affixes.size(); ++i)
		{
			if (type->Affixes[i]->is_magic_suffix()) {
				suffixes.push_back(type->Affixes[i]);
			}
		}
		if (type->get_item_class() != wyrmgus::item_class::none) {
			for (CUpgrade *upgrade : CUpgrade::get_all()) {
				if (upgrade->is_magic_suffix() && upgrade->has_affixed_item_class(type->get_item_class())) {
					suffixes.push_back(upgrade);
				}
			}
		}
		
		lua_createtable(l, suffixes.size(), 0);
		for (size_t i = 1; i <= suffixes.size(); ++i)
		{
			lua_pushstring(l, suffixes[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Works")) {
		std::vector<CUpgrade *> works;
		if (type->get_item_class() != wyrmgus::item_class::none) {
			for (CUpgrade *upgrade : CUpgrade::get_all()) {
				if (upgrade->Work == type->get_item_class() && !upgrade->UniqueOnly) {
					works.push_back(upgrade);
				}
			}
		}
		
		lua_createtable(l, works.size(), 0);
		for (size_t i = 1; i <= works.size(); ++i)
		{
			lua_pushstring(l, works[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Uniques")) {
		std::vector<const wyrmgus::unique_item *> uniques;
		for (const wyrmgus::unique_item *unique : wyrmgus::unique_item::get_all())
		{
			if (unique->get_unit_type() == type) {
				uniques.push_back(unique);
			}
		}
		
		lua_createtable(l, uniques.size(), 0);
		for (size_t i = 1; i <= uniques.size(); ++i)
		{
			lua_pushstring(l, uniques[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Variations")) {
		std::vector<std::string> variation_idents;
		for (const auto &variation : type->get_variations()) {
			if (std::find(variation_idents.begin(), variation_idents.end(), variation->get_identifier()) == variation_idents.end()) {
				variation_idents.push_back(variation->get_identifier());
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
		for (const auto &layer_variation : type->LayerVariations[image_layer]) {
			if (std::find(variation_idents.begin(), variation_idents.end(), layer_variation->get_identifier()) == variation_idents.end()) {
				variation_idents.push_back(layer_variation->get_identifier());
			}
		}
		
		lua_createtable(l, variation_idents.size(), 0);
		for (size_t i = 1; i <= variation_idents.size(); ++i)
		{
			lua_pushstring(l, variation_idents[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Trains")) {
		lua_createtable(l, type->Trains.size(), 0);
		for (size_t i = 1; i <= type->Trains.size(); ++i)
		{
			lua_pushstring(l, type->Trains[i - 1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	//Wyrmgus end
	} else {
		int index = UnitTypeVar.VariableNameLookup[data];
		if (index != -1) { // valid index
			lua_pushnumber(l, type->DefaultStat.Variables[index].Value);
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
void DefineVariableField(lua_State *l, wyrmgus::unit_variable &var, int lua_index)
{
	if (lua_index < 0) { // relative index
		--lua_index;
	}
	lua_pushnil(l);
	while (lua_next(l, lua_index)) {
		const char *key = LuaToString(l, -2);

		if (!strcmp(key, "Value")) {
			var.Value = LuaToNumber(l, -1);
		} else if (!strcmp(key, "Max")) {
			var.Max = LuaToNumber(l, -1);
		} else if (!strcmp(key, "Increase")) {
			var.Increase = LuaToNumber(l, -1);
		} else if (!strcmp(key, "Enable")) {
			var.Enable = LuaToBoolean(l, -1);
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
		DefineVariableField(l, UnitTypeVar.Variable[index], j + 1);
	}

	//update previously-defined unit types
	for (wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
		for (size_t i = unit_type->DefaultStat.Variables.size(); i < UnitTypeVar.GetNumberVariable(); ++i) {
			unit_type->DefaultStat.Variables.push_back(UnitTypeVar.Variable[i]);
		}
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
		for (wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) { // adjust array for unit already defined
			unit_type->BoolFlag.resize(new_size);
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
		int Index = 0;
		//Wyrmgus start
		int MinValue = 0;
		//Wyrmgus end
		int OffsetX = 0;
		int OffsetY = 0;
		int OffsetXPercent = 0;
		int OffsetYPercent = 0;
		bool IsCenteredInX = false;
		bool IsCenteredInY = false;
		bool ShowIfNotEnable = false;
		bool ShowWhenNull = false;
		bool HideHalf = false;
		bool ShowWhenMax = false;
		bool ShowOnlySelected = false;
		bool HideNeutral = false;
		bool HideAllied = false;
		//Wyrmgus start
		bool HideSelf = false;
		//Wyrmgus end
		bool ShowOpponent = false;
		bool ShowIfCanCastAnySpell = false;
		std::optional<wyrmgus::status_effect> status_effect;
		bool show_as_status_effect = false;
		bool hero_symbol = false;
		bool hp_bar = false;
		bool resource_bar = false;
	} tmp;

	const int nargs = lua_gettop(l);
	for (int i = 0; i < nargs; i++) {
		assert_throw(lua_istable(l, i + 1));
		CDecoVar *decovar = nullptr;
		memset(&tmp, 0, sizeof(tmp));
		lua_pushnil(l);
		while (lua_next(l, i + 1)) {
			const char *key = LuaToString(l, -2);
			if (!strcmp(key, "Index")) {
				const char *const value = LuaToString(l, -1);
				tmp.Index = UnitTypeVar.VariableNameLookup[value];// User variables
				assert_throw(tmp.Index != -1);
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
			} else if (!strcmp(key, "ShowIfCanCastAnySpell")) {
				tmp.ShowIfCanCastAnySpell = LuaToBoolean(l, -1);
			} else if (!strcmp(key, "StatusEffect")) {
				tmp.status_effect = string_to_status_effect(LuaToString(l, -1));
			} else if (!strcmp(key, "ShowAsStatusEffect")) {
				tmp.show_as_status_effect = LuaToBoolean(l, -1);
			} else if (!strcmp(key, "HeroSymbol")) {
				tmp.hero_symbol = LuaToBoolean(l, -1);
			} else if (!strcmp(key, "HPBar")) {
				tmp.hp_bar = LuaToBoolean(l, -1);
			} else if (!strcmp(key, "ResourceBar")) {
				tmp.resource_bar = LuaToBoolean(l, -1);
			} else if (!strcmp(key, "Method")) {
				assert_throw(lua_istable(l, -1));
				lua_rawgeti(l, -1, 1); // MethodName
				lua_rawgeti(l, -2, 2); // Data
				assert_throw(lua_istable(l, -1));
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

					decovartext->Font = wyrmgus::font::get(LuaToString(l, -1, 1));
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
		decovar->ShowIfCanCastAnySpell = tmp.ShowIfCanCastAnySpell;
		decovar->status_effect = tmp.status_effect;
		decovar->show_as_status_effect = tmp.show_as_status_effect;
		decovar->hero_symbol = tmp.hero_symbol;
		decovar->hp_bar = tmp.hp_bar;
		decovar->resource_bar = tmp.resource_bar;

		//Wyrmgus start
//		UnitTypeVar.DecoVar.push_back(decovar);
		bool already_defined = false;
		for (std::vector<CDecoVar *>::iterator it = UnitTypeVar.DecoVar.begin();
			 it != UnitTypeVar.DecoVar.end(); ++it) {
			if (static_cast<int>((*it)->Index) == tmp.Index && (*it)->status_effect == tmp.status_effect) {
				//replace other decorations which use the same variable
				*it = decovar;
				already_defined = true;
			}
		}

		if (!already_defined) {
			UnitTypeVar.DecoVar.push_back(decovar);
		}
		//Wyrmgus end
	}

	assert_throw(lua_gettop(l) != 0);
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

//Wyrmgus start
static int CclGetUnitTypes(lua_State *l)
{
	std::vector<std::string> unit_types;
	for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
		unit_types.push_back(unit_type->Ident);
	}
		
	lua_createtable(l, unit_types.size(), 0);
	for (size_t i = 1; i <= unit_types.size(); ++i)
	{
		lua_pushstring(l, unit_types[i-1].c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetAnimations(lua_State *l)
{
	std::vector<std::string> animations;

	for (const wyrmgus::animation_set *animation_set : wyrmgus::animation_set::get_all()) {
		animations.push_back(animation_set->get_identifier());
	}

	lua_createtable(l, animations.size(), 0);
	for (size_t i = 1; i <= animations.size(); ++i)
	{
		lua_pushstring(l, animations[i-1].c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}
//Wyrmgus end

// ----------------------------------------------------------------------------

/**
**  Update unit variables which are not user defined.
*/
void UpdateUnitVariables(CUnit &unit)
{
	const wyrmgus::unit_type *type = unit.Type;
	
	//Wyrmgus start
	if (!type) {
		fprintf(stderr, "Error in UpdateUnitVariables: Unit has no type\n");
		return;
	}
	//Wyrmgus end

	for (int i = 0; i < NVARALREADYDEFINED; i++) { // default values
		switch (i) {
			case ARMOR_INDEX:
			case PIERCINGDAMAGE_INDEX:
			case BASICDAMAGE_INDEX:
			case SUPPLY_INDEX:
			case DEMAND_INDEX:
			case THORNSDAMAGE_INDEX:
			case FIREDAMAGE_INDEX:
			case COLDDAMAGE_INDEX:
			case ARCANEDAMAGE_INDEX:
			case LIGHTNINGDAMAGE_INDEX:
			case AIRDAMAGE_INDEX:
			case EARTHDAMAGE_INDEX:
			case WATERDAMAGE_INDEX:
			case ACIDDAMAGE_INDEX:
			case SHADOW_DAMAGE_INDEX:
			case SPEED_INDEX:
			case FIRERESISTANCE_INDEX:
			case COLDRESISTANCE_INDEX:
			case ARCANERESISTANCE_INDEX:
			case LIGHTNINGRESISTANCE_INDEX:
			case AIRRESISTANCE_INDEX:
			case EARTHRESISTANCE_INDEX:
			case WATERRESISTANCE_INDEX:
			case ACIDRESISTANCE_INDEX:
			case SHADOW_RESISTANCE_INDEX:
			case HACKRESISTANCE_INDEX:
			case PIERCERESISTANCE_INDEX:
			case BLUNTRESISTANCE_INDEX:
			case DEHYDRATIONIMMUNITY_INDEX:
			case MANA_INDEX:
			case KILL_INDEX:
			case XP_INDEX:
			case GIVERESOURCE_INDEX:
			case AUTOREPAIRRANGE_INDEX:
			case HP_INDEX:
			case SHIELD_INDEX:
			case POINTS_INDEX:
			case MAXHARVESTERS_INDEX:
			case SHIELDPERMEABILITY_INDEX:
			case SHIELDPIERCING_INDEX:
			case ISALIVE_INDEX:
			case PLAYER_INDEX:
			case PRIORITY_INDEX:
			case SIGHTRANGE_INDEX:
			case ATTACKRANGE_INDEX:
			case STRENGTH_INDEX:
			case DEXTERITY_INDEX:
			case INTELLIGENCE_INDEX:
			case CHARISMA_INDEX:
			case ACCURACY_INDEX:
			case EVASION_INDEX:
			case LEVEL_INDEX:
			case LEVELUP_INDEX:
			case XPREQUIRED_INDEX:
			case VARIATION_INDEX:
			case HITPOINTHEALING_INDEX:
			case HITPOINTBONUS_INDEX:
			case MANA_RESTORATION_INDEX:
			case CRITICALSTRIKECHANCE_INDEX:
			case CHARGEBONUS_INDEX:
			case BACKSTAB_INDEX:
			case BONUSAGAINSTMOUNTED_INDEX:
			case BONUSAGAINSTBUILDINGS_INDEX:
			case BONUSAGAINSTAIR_INDEX:
			case BONUSAGAINSTGIANTS_INDEX:
			case BONUSAGAINSTDRAGONS_INDEX:
			case DAYSIGHTRANGEBONUS_INDEX:
			case NIGHTSIGHTRANGEBONUS_INDEX:
			case KNOWLEDGEMAGIC_INDEX:
			case KNOWLEDGEWARFARE_INDEX:
			case KNOWLEDGEMINING_INDEX:
			case MAGICLEVEL_INDEX:
			case TRANSPARENCY_INDEX:
			case GENDER_INDEX:
			case BIRTHCYCLE_INDEX:
			case TIMEEFFICIENCYBONUS_INDEX:
			case RESEARCHSPEEDBONUS_INDEX:
			case GARRISONEDRANGEBONUS_INDEX:
			case SPEEDBONUS_INDEX:
			case RAIL_SPEED_BONUS_INDEX:
			case GATHERINGBONUS_INDEX:
			case COPPERGATHERINGBONUS_INDEX:
			case SILVERGATHERINGBONUS_INDEX:
			case GOLDGATHERINGBONUS_INDEX:
			case IRONGATHERINGBONUS_INDEX:
			case MITHRILGATHERINGBONUS_INDEX:
			case LUMBERGATHERINGBONUS_INDEX:
			case STONEGATHERINGBONUS_INDEX:
			case COALGATHERINGBONUS_INDEX:
			case JEWELRYGATHERINGBONUS_INDEX:
			case FURNITUREGATHERINGBONUS_INDEX:
			case LEATHERGATHERINGBONUS_INDEX:
			case GEMSGATHERINGBONUS_INDEX:
			case DISEMBARKMENTBONUS_INDEX:
			case TRADECOST_INDEX:
			case SALVAGEFACTOR_INDEX:
			case MUGGING_INDEX:
			case RAIDING_INDEX:
			case DESERTSTALK_INDEX:
			case FORESTSTALK_INDEX:
			case SWAMPSTALK_INDEX:
			case AURA_RANGE_BONUS_INDEX:
			case LEADERSHIPAURA_INDEX:
			case REGENERATIONAURA_INDEX:
			case HYDRATINGAURA_INDEX:
			case ETHEREALVISION_INDEX:
			case HERO_INDEX:
				continue;
			default:
				break;
		}

		unit.Variable[i].Value = 0;
		unit.Variable[i].Max = 0;
		unit.Variable[i].Enable = 1;
	}

	//Wyrmgus start
	unit.Variable[VARIATION_INDEX].Max = unit.Type->get_variations().size();
	unit.Variable[VARIATION_INDEX].Enable = 1;
	unit.Variable[VARIATION_INDEX].Value = unit.Variation;

	unit.Variable[TRANSPARENCY_INDEX].Max = 100;

	unit.Variable[LEVEL_INDEX].Max = 100000;
	//Wyrmgus end

	// Shield permeability
	unit.Variable[SHIELDPERMEABILITY_INDEX].Max = 100;

	// Transport
	unit.Variable[TRANSPORT_INDEX].Value = unit.BoardCount;
	unit.Variable[TRANSPORT_INDEX].Max = unit.Type->MaxOnBoard;

	unit.CurrentOrder()->UpdateUnitVariables(unit);

	// Resources.
	//Wyrmgus start
//	if (unit.Type->GivesResource) {
	if (unit.GivesResource) {
	//Wyrmgus end
		unit.Variable[GIVERESOURCE_INDEX].Value = unit.ResourcesHeld;
		unit.Variable[GIVERESOURCE_INDEX].Max = unit.ResourcesHeld > unit.Variable[GIVERESOURCE_INDEX].Max ? unit.ResourcesHeld : unit.Variable[GIVERESOURCE_INDEX].Max;
		//Wyrmgus start
		unit.Variable[GIVERESOURCE_INDEX].Enable = 1;
		//Wyrmgus end
	}
	if (unit.Type->BoolFlag[HARVESTER_INDEX].value && unit.get_current_resource() != nullptr) {
		unit.Variable[CARRYRESOURCE_INDEX].Value = unit.ResourcesHeld;
		unit.Variable[CARRYRESOURCE_INDEX].Max = unit.Type->get_resource_info(unit.get_current_resource())->ResourceCapacity;
	}

	//Wyrmgus start
	/*
	// SightRange
	unit.Variable[SIGHTRANGE_INDEX].Value = type->DefaultStat.Variables[SIGHTRANGE_INDEX].Value;
	unit.Variable[SIGHTRANGE_INDEX].Max = unit.Stats->Variables[SIGHTRANGE_INDEX].Max;
	*/
	//Wyrmgus end

	// AttackRange
	//Wyrmgus start
//	unit.Variable[ATTACKRANGE_INDEX].Value = type->DefaultStat.Variables[ATTACKRANGE_INDEX].Max;
//	unit.Variable[ATTACKRANGE_INDEX].Max = unit.Stats->Variables[ATTACKRANGE_INDEX].Max;
	//Wyrmgus end

	// Priority
	unit.Variable[PRIORITY_INDEX].Value = type->DefaultStat.Variables[PRIORITY_INDEX].Max;
	unit.Variable[PRIORITY_INDEX].Max = unit.Stats->Variables[PRIORITY_INDEX].Max;

	// Position
	if (unit.MapLayer != nullptr) {
		unit.Variable[POSX_INDEX].Value = unit.tilePos.x;
		unit.Variable[POSX_INDEX].Max = unit.MapLayer->get_width();
		unit.Variable[POSY_INDEX].Value = unit.tilePos.y;
		unit.Variable[POSY_INDEX].Max = unit.MapLayer->get_height();
	}

	// Target Position
	const Vec2i goalPos = unit.CurrentOrder()->GetGoalPos();
	unit.Variable[TARGETPOSX_INDEX].Value = goalPos.x;
	unit.Variable[TARGETPOSX_INDEX].Max = CMap::get()->Info->MapWidths[unit.CurrentOrder()->GetGoalMapLayer()];
	unit.Variable[TARGETPOSY_INDEX].Value = goalPos.y;
	unit.Variable[TARGETPOSY_INDEX].Max = CMap::get()->Info->MapHeights[unit.CurrentOrder()->GetGoalMapLayer()];

	// RadarRange
	unit.Variable[RADAR_INDEX].Value = unit.Stats->Variables[RADAR_INDEX].Value;
	unit.Variable[RADAR_INDEX].Max = unit.Stats->Variables[RADAR_INDEX].Value;

	// RadarJammerRange
	unit.Variable[RADARJAMMER_INDEX].Value = unit.Stats->Variables[RADARJAMMER_INDEX].Value;
	unit.Variable[RADARJAMMER_INDEX].Max = unit.Stats->Variables[RADARJAMMER_INDEX].Value;

	// SlotNumber
	unit.Variable[SLOT_INDEX].Value = UnitNumber(unit);
	unit.Variable[SLOT_INDEX].Max = wyrmgus::unit_manager::get()->GetUsedSlotCount();

	// Is Alive
	unit.Variable[ISALIVE_INDEX].Value = unit.IsAlive() ? 1 : 0;
	unit.Variable[ISALIVE_INDEX].Max = 1;

	// Player
	unit.Variable[PLAYER_INDEX].Value = unit.Player->get_index();
	unit.Variable[PLAYER_INDEX].Max = PlayerMax;
	
	for (int i = 0; i < NVARALREADYDEFINED; i++) { // default values
		unit.Variable[i].Enable &= unit.Variable[i].Max > 0;
		//Wyrmgus start
//		if (unit.Variable[i].Value > unit.Variable[i].Max) {
		if (unit.Variable[i].Value > unit.GetModifiedVariable(i, VariableAttribute::Max)) {
		//Wyrmgus end
			DebugPrint("Value out of range: '%s'(%d), for variable '%s',"
					   " value = %d, max = %d\n"
					   _C_ type->Ident.c_str() _C_ UnitNumber(unit) _C_ UnitTypeVar.VariableNameLookup[i]
					   //Wyrmgus start
//					   _C_ unit.Variable[i].Value _C_ unit.Variable[i].Max);
					   _C_ unit.Variable[i].Value _C_ unit.GetModifiedVariable(i, VariableAttribute::Max));
					   //Wyrmgus end
			unit.Variable[i].Value = std::clamp(unit.Variable[i].Value, 0, unit.Variable[i].Max);
		}
	}
}

//Wyrmgus start
/**
**  Define a species phylum.
**
**  @param l  Lua state.
*/
static int CclDefineSpeciesPhylum(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string phylum_ident = LuaToString(l, 1);
	wyrmgus::taxon *phylum = wyrmgus::taxon::add(phylum_ident, nullptr);
	phylum->rank = wyrmgus::taxonomic_rank::phylum;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			phylum->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Kingdom")) {
			phylum->supertaxon = wyrmgus::taxon::get(LuaToString(l, -1));
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a species class.
**
**  @param l  Lua state.
*/
static int CclDefineSpeciesClass(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string class_ident = LuaToString(l, 1);
	wyrmgus::taxon *species_class = wyrmgus::taxon::add(class_ident, nullptr);
	species_class->rank = wyrmgus::taxonomic_rank::class_rank;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			species_class->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Phylum")) {
			std::string phylum_ident = LuaToString(l, -1);
			wyrmgus::taxon *phylum = wyrmgus::taxon::get(phylum_ident);
			species_class->supertaxon = phylum;
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a species order.
**
**  @param l  Lua state.
*/
static int CclDefineSpeciesOrder(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string order_ident = LuaToString(l, 1);
	wyrmgus::taxon *order = wyrmgus::taxon::add(order_ident, nullptr);
	order->rank = wyrmgus::taxonomic_rank::order;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			order->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Class")) {
			std::string class_ident = LuaToString(l, -1);
			wyrmgus::taxon *species_class = wyrmgus::taxon::get(class_ident);
			order->supertaxon = species_class;
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a species family.
**
**  @param l  Lua state.
*/
static int CclDefineSpeciesFamily(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string family_ident = LuaToString(l, 1);
	wyrmgus::taxon *family = wyrmgus::taxon::add(family_ident, nullptr);
	family->rank = wyrmgus::taxonomic_rank::family;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			family->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Order")) {
			std::string order_ident = LuaToString(l, -1);
			wyrmgus::taxon *order = wyrmgus::taxon::get(order_ident);
			family->supertaxon = order;
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a species genus.
**
**  @param l  Lua state.
*/
static int CclDefineSpeciesGenus(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string genus_ident = LuaToString(l, 1);
	wyrmgus::taxon *genus = wyrmgus::taxon::add(genus_ident, nullptr);
	genus->rank = wyrmgus::taxonomic_rank::genus;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			genus->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "CommonName")) {
			genus->common_name = LuaToString(l, -1);
		} else if (!strcmp(value, "Family")) {
			std::string family_ident = LuaToString(l, -1);
			wyrmgus::taxon *family = wyrmgus::taxon::get(family_ident);
			genus->supertaxon = family;
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a species.
**
**  @param l  Lua state.
*/
static int CclDefineSpecies(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string species_ident = LuaToString(l, 1);
	wyrmgus::species *species = wyrmgus::species::get_or_add(species_ident, nullptr);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			species->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Description")) {
			species->set_description(LuaToString(l, -1));
		} else if (!strcmp(value, "Quote")) {
			species->set_quote(LuaToString(l, -1));
		} else if (!strcmp(value, "Background")) {
			species->set_background(LuaToString(l, -1));
		} else if (!strcmp(value, "Era")) {
			const std::string era_ident = LuaToString(l, -1);
			species->era = wyrmgus::string_to_geological_era(era_ident);
		} else if (!strcmp(value, "Sapient")) {
			species->sapient = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Supertaxon") || !strcmp(value, "Genus")) {
			std::string genus_ident = LuaToString(l, -1);
			wyrmgus::taxon *genus = wyrmgus::taxon::get(genus_ident);
			species->supertaxon = genus;
		} else if (!strcmp(value, "Species")) {
			species->specific_name = LuaToString(l, -1);
		} else if (!strcmp(value, "Homeworld")) {
			const std::string world_ident = LuaToString(l, -1);
			wyrmgus::world *world = wyrmgus::world::get(world_ident);
			species->homeworld = world;
		} else if (!strcmp(value, "Terrains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				wyrmgus::terrain_type *terrain = wyrmgus::terrain_type::get(LuaToString(l, -1, j + 1));
				species->native_terrain_types.push_back(terrain);
			}
		} else if (!strcmp(value, "EvolvesFrom")) {
			species->pre_evolutions.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string evolves_from_ident = LuaToString(l, -1, j + 1);
				wyrmgus::species *evolves_from = wyrmgus::species::get(evolves_from_ident);
				species->pre_evolutions.push_back(evolves_from);
				evolves_from->evolutions.push_back(species);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

static int CclGetSpecies(lua_State *l)
{
	lua_createtable(l, wyrmgus::species::get_all().size(), 0);
	for (size_t i = 1; i <= wyrmgus::species::get_all().size(); ++i)
	{
		lua_pushstring(l, wyrmgus::species::get_all()[i-1]->get_identifier().c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Get species data.
**
**  @param l  Lua state.
*/
static int CclGetSpeciesData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string species_ident = LuaToString(l, 1);
	const wyrmgus::species *species = wyrmgus::species::get(species_ident);
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, species->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "ScientificName")) {
		lua_pushstring(l, species->get_scientific_name().c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, species->get_description().c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, species->get_quote().c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, species->get_background().c_str());
		return 1;
	} else if (!strcmp(data, "Family")) {
		const wyrmgus::taxon *family = species->get_supertaxon_of_rank(wyrmgus::taxonomic_rank::family);
		if (family != nullptr) {
			lua_pushstring(l, family->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Supertaxon")) {
		if (species->get_supertaxon() != nullptr) {
			lua_pushstring(l, species->get_supertaxon()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Era")) {
		lua_pushstring(l, wyrmgus::geological_era_to_string(species->get_era()).c_str());
		return 1;
	} else if (!strcmp(data, "Sapient")) {
		lua_pushboolean(l, species->is_sapient());
		return 1;
	} else if (!strcmp(data, "Prehistoric")) {
		lua_pushboolean(l, species->is_prehistoric());
		return 1;
	} else if (!strcmp(data, "Homeworld")) {
		if (species->get_homeworld() != nullptr) {
			lua_pushstring(l, species->get_homeworld()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Type")) {
		if (species->get_unit_type() != nullptr) {
			lua_pushstring(l, species->get_unit_type()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Terrains")) {
		lua_createtable(l, species->get_native_terrain_types().size(), 0);
		for (size_t i = 1; i <= species->get_native_terrain_types().size(); ++i)
		{
			lua_pushstring(l, species->get_native_terrain_types()[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "EvolvesFrom")) {
		lua_createtable(l, species->get_pre_evolutions().size(), 0);
		for (size_t i = 1; i <= species->get_pre_evolutions().size(); ++i)
		{
			lua_pushstring(l, species->get_pre_evolutions()[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "EvolvesTo")) {
		lua_createtable(l, species->get_evolutions().size(), 0);
		for (size_t i = 1; i <= species->get_evolutions().size(); ++i)
		{
			lua_pushstring(l, species->get_evolutions()[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

static int CclGetTaxonData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}

	const std::string taxon_ident = LuaToString(l, 1);
	const wyrmgus::taxon *taxon = wyrmgus::taxon::get(taxon_ident);
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, taxon->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "CommonName")) {
		lua_pushstring(l, taxon->get_common_name().c_str());
		return 1;
	} else if (!strcmp(data, "Supertaxon")) {
		if (taxon->get_supertaxon() != nullptr) {
			lua_pushstring(l, taxon->get_supertaxon()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}
//Wyrmgus end

//Wyrmgus start
/**
**  Define settlement site unit
**
**  @param l  Lua state.
*/
static int CclSetSettlementSiteUnit(lua_State *l)
{
	LuaCheckArgs(l, 1);
	settlement_site_unit_type = wyrmgus::unit_type::get(LuaToString(l, 1));

	return 0;
}
//Wyrmgus end

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
	//Wyrmgus start
	lua_register(Lua, "GetUnitTypes", CclGetUnitTypes);
	lua_register(Lua, "GetAnimations", CclGetAnimations);
	//Wyrmgus end

	UnitTypeVar.Init();

	lua_register(Lua, "UnitType", CclUnitType);
	lua_register(Lua, "UnitTypeArray", CclUnitTypeArray);
	// unit type structure access
	lua_register(Lua, "GetUnitTypeIdent", CclGetUnitTypeIdent);
	lua_register(Lua, "GetUnitTypeName", CclGetUnitTypeName);
	lua_register(Lua, "SetUnitTypeName", CclSetUnitTypeName);
	lua_register(Lua, "GetUnitTypeData", CclGetUnitTypeData);
	//Wyrmgus start
	lua_register(Lua, "DefineSpeciesPhylum", CclDefineSpeciesPhylum);
	lua_register(Lua, "DefineSpeciesClass", CclDefineSpeciesClass);
	lua_register(Lua, "DefineSpeciesOrder", CclDefineSpeciesOrder);
	lua_register(Lua, "DefineSpeciesFamily", CclDefineSpeciesFamily);
	lua_register(Lua, "DefineSpeciesGenus", CclDefineSpeciesGenus);
	lua_register(Lua, "DefineSpecies", CclDefineSpecies);
	lua_register(Lua, "GetSpecies", CclGetSpecies);
	lua_register(Lua, "GetSpeciesData", CclGetSpeciesData);
	lua_register(Lua, "GetTaxonData", CclGetTaxonData);
	lua_register(Lua, "SetSettlementSiteUnit", CclSetSettlementSiteUnit);
	//Wyrmgus end
}
