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
/**@name unit_type.h - The unit type header file. */
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

#ifndef __UNITTYPE_H__
#define __UNITTYPE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"
#include "detailed_data_element.h"
#include "missile/missile_config.h"
#include "sound/unit_sound.h"
#include "ui/icon_config.h"
#include "unit/variable.h"
#include "upgrade/upgrade_structs.h"
#include "vec2i.h"
#include "video/color.h"

#include <core/object.h>
#include <core/ustring.h>

#include <algorithm>
#include <climits>
#include <cstring>
#include <set>

#ifdef __MORPHOS__
#undef Enable
#endif

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CAnimations;
class CBuildRestriction;
class CCivilization;
class CConstruction;
class CDependency;
class CFile;
class CGender;
class CGraphic;
class CPlayer;
class CPlayerColorGraphic;
class CSpell;
class CUnitTypeVariation;
class CUpgrade;
class CWord;
class MissileType;
//Wyrmgus start
class CButtonLevel;
class CFaction;
class CPlane;
class CSpecies;
class CTerrainType;
class CVariable;
class CWorld;
//Wyrmgus end
class ItemClass;
class ItemSlot;
class LuaCallback;
class PaletteImage;
class UnitClass;
struct lua_State;

constexpr int UnitSides = 8;
constexpr int MaxAttackPos = 5;

enum GroupSelectionMode {
	SELECTABLE_BY_RECTANGLE_ONLY = 0,
	NON_SELECTABLE_BY_RECTANGLE_ONLY,
	SELECT_ALL
};

//Wyrmgus start
enum ImageLayers {
	LeftArmImageLayer,
	RightArmImageLayer,
	RightHandImageLayer,
	HairImageLayer,
	ClothingImageLayer,
	ClothingLeftArmImageLayer,
	ClothingRightArmImageLayer,
	PantsImageLayer,
	BootsImageLayer,
	WeaponImageLayer,
	ShieldImageLayer,
	HelmetImageLayer,
	BackpackImageLayer,
	MountImageLayer,
	
	MaxImageLayers
};
//Wyrmgus end

class ResourceInfo
{
public:
	std::string FileWhenLoaded;		/// Change the graphic when the unit is loaded.
	std::string FileWhenEmpty;		/// Change the graphic when the unit is empty.
	unsigned WaitAtResource = 0;	/// Cycles the unit waits while mining.
	unsigned ResourceStep = 0;		/// Resources the unit gains per mining cycle.
	int ResourceCapacity = 0;		/// Max amount of resources to carry.
	unsigned WaitAtDepot = 0;		/// Cycles the unit waits while returning.
	unsigned ResourceId = 0;		/// Id of the resource harvested. Redundant.
	bool LoseResources = false;		/// the unit will lose its resource when distracted.
	unsigned char RefineryHarvester = 0;	/// Unit have to build Refinery buildings for harvesting.
	//  Runtime info:
	CPlayerColorGraphic *SpriteWhenLoaded = nullptr;	/// The graphic corresponding to FileWhenLoaded.
	CPlayerColorGraphic *SpriteWhenEmpty = nullptr;	/// The graphic corresponding to FileWhenEmpty
};

// Index for boolflag already defined
enum {
	COWARD_INDEX = 0,				/// Unit will only attack if instructed.
	BUILDING_INDEX,
	FLIP_INDEX,
	REVEALER_INDEX,					/// reveal the fog of war
	LANDUNIT_INDEX,
	AIRUNIT_INDEX,
	SEAUNIT_INDEX,
	EXPLODEWHENKILLED_INDEX,
	VISIBLEUNDERFOG_INDEX,			/// Unit is visible under fog of war.
	PERMANENTCLOAK_INDEX,			/// Is only visible by CloakDetectors.
	DETECTCLOAK_INDEX,				/// Can see Cloaked units.
	ATTACKFROMTRANSPORTER_INDEX,	/// Can attack from transporter
	VANISHES_INDEX,					/// Corpses & destroyed places.
	GROUNDATTACK_INDEX,				/// Can do ground attack command.
	SHOREBUILDING_INDEX,			/// Building must be built on coast.
	CANATTACK_INDEX,
	//Wyrmgus start
	CANDOCK_INDEX,
	//Wyrmgus end
	BUILDEROUTSIDE_INDEX,			/// The builder stays outside during the construction.
	BUILDERLOST_INDEX,				/// The builder is lost after the construction.
	CANHARVEST_INDEX,				/// Resource can be harvested.
	//Wyrmgus start
	INEXHAUSTIBLE_INDEX,			/// Resource is not exhaustible
	//Wyrmgus end
	HARVESTER_INDEX,				/// Unit is a resource harvester.
	SELECTABLEBYRECTANGLE_INDEX,	/// Selectable with mouse rectangle.
	ISNOTSELECTABLE_INDEX,
	DECORATION_INDEX,				/// Unit is a decoration (act as tile).
	INDESTRUCTIBLE_INDEX,			/// Unit is indestructible (take no damage).
	TELEPORTER_INDEX,				/// Can teleport other units.
	SHIELDPIERCE_INDEX,
	SAVECARGO_INDEX,				/// Unit unloads his passengers after death.
	NONSOLID_INDEX,					/// Unit can be entered by other units.
	WALL_INDEX,						/// Use special logic for Direction field.
	NORANDOMPLACING_INDEX,			/// Don't use random frame rotation
	ORGANIC_INDEX,					/// Organic unit (used for death coil spell)
	SIDEATTACK_INDEX,
	NOFRIENDLYFIRE_INDEX,           /// Unit accepts friendly fire for splash attacks
	//Wyrmgus start
	TOWNHALL_INDEX,
	MARKET_INDEX,
	RECRUITHEROES_INDEX,
	GARRISONTRAINING_INDEX,
	INCREASESLUXURYDEMAND_INDEX,
	ITEM_INDEX,
	POWERUP_INDEX,
	INVENTORY_INDEX,
	TRAP_INDEX,
	BRIDGE_INDEX,
	TRADER_INDEX,
	FAUNA_INDEX,
	PREDATOR_INDEX,
	SLIME_INDEX,
	PEOPLEAVERSION_INDEX,
	MOUNTED_INDEX,
	RAIL_INDEX,
	DIMINUTIVE_INDEX,
	GIANT_INDEX,
	DRAGON_INDEX,
	DETRITUS_INDEX,
	FLESH_INDEX,
	VEGETABLE_INDEX,
	INSECT_INDEX,
	DAIRY_INDEX,
	DETRITIVORE_INDEX,
	CARNIVORE_INDEX,
	HERBIVORE_INDEX,
	INSECTIVORE_INDEX,
	HARVESTFROMOUTSIDE_INDEX,
	OBSTACLE_INDEX,
	AIRUNPASSABLE_INDEX,
	SLOWS_INDEX,
	HACKDAMAGE_INDEX,
	PIERCEDAMAGE_INDEX,
	BLUNTDAMAGE_INDEX,
	ETHEREAL_INDEX,					/// is only visible by units with ethereal vision.
	HIDDENOWNERSHIP_INDEX,
	INVERTEDSOUTHEASTARMS_INDEX,
	INVERTEDEASTARMS_INDEX,
	//Wyrmgus end
	NBARALREADYDEFINED
};

// Index for variable already defined.
enum {
	HP_INDEX = 0,
	BUILD_INDEX,
	MANA_INDEX,
	TRANSPORT_INDEX,
	RESEARCH_INDEX,
	TRAINING_INDEX,
	UPGRADINGTO_INDEX,
	GIVERESOURCE_INDEX,
	CARRYRESOURCE_INDEX,
	XP_INDEX,
	KILL_INDEX,
	SUPPLY_INDEX,					/// Food supply
	DEMAND_INDEX,					/// Food demand
	ARMOR_INDEX,
	SIGHTRANGE_INDEX,
	ATTACKRANGE_INDEX,
	PIERCINGDAMAGE_INDEX,
	BASICDAMAGE_INDEX,
	//Wyrmgus start
	THORNSDAMAGE_INDEX,
	FIREDAMAGE_INDEX,
	COLDDAMAGE_INDEX,
	ARCANEDAMAGE_INDEX,
	LIGHTNINGDAMAGE_INDEX,
	AIRDAMAGE_INDEX,
	EARTHDAMAGE_INDEX,
	WATERDAMAGE_INDEX,
	ACIDDAMAGE_INDEX,
	SPEED_INDEX,
	FIRERESISTANCE_INDEX,
	COLDRESISTANCE_INDEX,
	ARCANERESISTANCE_INDEX,
	LIGHTNINGRESISTANCE_INDEX,
	AIRRESISTANCE_INDEX,
	EARTHRESISTANCE_INDEX,
	WATERRESISTANCE_INDEX,
	ACIDRESISTANCE_INDEX,
	HACKRESISTANCE_INDEX,
	PIERCERESISTANCE_INDEX,
	BLUNTRESISTANCE_INDEX,
	DEHYDRATIONIMMUNITY_INDEX,
	//Wyrmgus end
	POSX_INDEX,
	POSY_INDEX,
	TARGETPOSX_INDEX,
	TARGETPOSY_INDEX,
	RADAR_INDEX,
	RADARJAMMER_INDEX,
	AUTOREPAIRRANGE_INDEX,
	BLOODLUST_INDEX,
	HASTE_INDEX,
	SLOW_INDEX,
	INVISIBLE_INDEX,
	UNHOLYARMOR_INDEX,
	SLOT_INDEX,
	SHIELD_INDEX,
	POINTS_INDEX,
	MAXHARVESTERS_INDEX,
	POISON_INDEX,
	SHIELDPERMEABILITY_INDEX,
	SHIELDPIERCING_INDEX,
	ISALIVE_INDEX,
	PLAYER_INDEX,
	PRIORITY_INDEX,
	//Wyrmgus start
	STRENGTH_INDEX,
	DEXTERITY_INDEX,
	INTELLIGENCE_INDEX,
	CHARISMA_INDEX,
	ACCURACY_INDEX,
	EVASION_INDEX,
	LEVEL_INDEX,
	LEVELUP_INDEX,
	XPREQUIRED_INDEX,
	VARIATION_INDEX,
	HITPOINTHEALING_INDEX,
	HITPOINTBONUS_INDEX,
	CRITICALSTRIKECHANCE_INDEX,
	CHARGEBONUS_INDEX,
	BACKSTAB_INDEX,
	BONUSAGAINSTMOUNTED_INDEX,
	BONUSAGAINSTBUILDINGS_INDEX,
	BONUSAGAINSTAIR_INDEX,
	BONUSAGAINSTGIANTS_INDEX,
	BONUSAGAINSTDRAGONS_INDEX,
	DAYSIGHTRANGEBONUS_INDEX,
	NIGHTSIGHTRANGEBONUS_INDEX,
	KNOWLEDGEMAGIC_INDEX,
	KNOWLEDGEWARFARE_INDEX,
	KNOWLEDGEMINING_INDEX,
	MAGICLEVEL_INDEX,
	TRANSPARENCY_INDEX,
	GENDER_INDEX,
	BIRTHCYCLE_INDEX,
	STUN_INDEX,
	BLEEDING_INDEX,
	LEADERSHIP_INDEX,
	BLESSING_INDEX,
	INSPIRE_INDEX,
	PRECISION_INDEX,
	REGENERATION_INDEX,
	BARKSKIN_INDEX,
	INFUSION_INDEX,
	TERROR_INDEX,
	WITHER_INDEX,
	DEHYDRATION_INDEX,
	HYDRATING_INDEX,
	TIMEEFFICIENCYBONUS_INDEX,
	RESEARCHSPEEDBONUS_INDEX,
	GARRISONEDRANGEBONUS_INDEX,
	SPEEDBONUS_INDEX, // dummy variable for terrain types that increase movement speed, so that their bonus shows up correctly in the interface
	GATHERINGBONUS_INDEX,
	COPPERGATHERINGBONUS_INDEX,
	SILVERGATHERINGBONUS_INDEX,
	GOLDGATHERINGBONUS_INDEX,
	IRONGATHERINGBONUS_INDEX,
	MITHRILGATHERINGBONUS_INDEX,
	LUMBERGATHERINGBONUS_INDEX,
	STONEGATHERINGBONUS_INDEX,
	COALGATHERINGBONUS_INDEX,
	JEWELRYGATHERINGBONUS_INDEX,
	FURNITUREGATHERINGBONUS_INDEX,
	LEATHERGATHERINGBONUS_INDEX,
	GEMSGATHERINGBONUS_INDEX,
	DISEMBARKMENTBONUS_INDEX,
	TRADECOST_INDEX,
	SALVAGEFACTOR_INDEX,
	MUGGING_INDEX,
	RAIDING_INDEX,
	DESERTSTALK_INDEX,
	FORESTSTALK_INDEX,
	SWAMPSTALK_INDEX,
	LEADERSHIPAURA_INDEX,
	REGENERATIONAURA_INDEX,
	HYDRATINGAURA_INDEX,
	ETHEREALVISION_INDEX,
	HERO_INDEX,
	OWNERSHIPINFLUENCERANGE_INDEX,
	//Wyrmgus end
	NVARALREADYDEFINED
};

class CUnit;
class CUnitType;
class CFont;

/**
**  Decoration for user defined variable.
**
**  It is used to show variables graphicly.
**  @todo add more stuff in this struct.
*/
class CDecoVar
{
public:

	CDecoVar() {};
	virtual ~CDecoVar()
	{
	};

	/// function to draw the decorations.
	virtual void Draw(int x, int y, const CUnitType &type, const CVariable &var) const = 0;

	unsigned int Index;     /// Index of the variable. @see DefineVariables

	//Wyrmgus start
	int MinValue;			/// Minimum value of the variable
	//Wyrmgus end

	int OffsetX;            /// Offset in X coord.
	int OffsetY;            /// Offset in Y coord.

	int OffsetXPercent;     /// Percent offset (TileSize.x) in X coord.
	int OffsetYPercent;     /// Percent offset (TileSize.y) in Y coord.

	bool IsCenteredInX;     /// if true, use center of deco instead of left border
	bool IsCenteredInY;     /// if true, use center of deco instead of upper border

	bool ShowIfNotEnable;   /// if false, Show only if var is enable
	bool ShowWhenNull;      /// if false, don't show if var is null (F.E poison)
	bool HideHalf;          /// if true, don't show when 0 < var < max.
	bool ShowWhenMax;       /// if false, don't show if var is to max. (Like mana)
	bool ShowOnlySelected;  /// if true, show only for selected units.

	bool HideNeutral;       /// if true, don't show for neutral unit.
	bool HideAllied;        /// if true, don't show for allied unit. (but show own units)
	//Wyrmgus start
	bool HideSelf;			/// if true, don't show for own units.
	//Wyrmgus end
	bool ShowOpponent;      /// if true, show for opponent unit.
	
	bool ShowIfCanCastAnySpell;   /// if true, only show if the unit can cast a spell.
};

class CDecoVarBar : public CDecoVar
{
public:
	/// function to draw the decorations.
	virtual void Draw(int x, int y, const CUnitType &type, const CVariable &var) const;

	bool IsVertical;            /// if true, vertical bar, else horizontal.
	bool SEToNW;                /// (SouthEastToNorthWest), if false value 0 is on the left or up of the bar.
	int Height;                 /// Height of the bar.
	int Width;                  /// Width of the bar.
	bool ShowFullBackground;    /// if true, show background like value equal to max.
	char BorderSize;            /// Size of the border, 0 for no border.
	// FIXME color depend of percent (red, Orange, Yellow, Green...)
	IntColor Color;             /// Color of bar.
	IntColor BColor;            /// Color of background.
};

class CDecoVarText : public CDecoVar
{
public:
	/// function to draw the decorations.
	virtual void Draw(int x, int y, const CUnitType &type, const CVariable &var) const;

	CFont *Font = nullptr;	/// Font to use to display value.
	// FIXME : Add Color, format
};

/// Sprite contains frame from full (left)to empty state (right).
class CDecoVarSpriteBar : public CDecoVar
{
public:
	/// function to draw the decorations.
	virtual void Draw(int x, int y,
					  const CUnitType &type, const CVariable &var) const;

	char NSprite = -1;	/// Index of number. (@see DefineSprites and @see GetSpriteIndex)
	// FIXME Sprite info. better way ?
};

/// use to show specific frame in a sprite.
class CDecoVarStaticSprite : public CDecoVar
{
public:
	/// function to draw the decorations.
	virtual void Draw(int x, int y, const CUnitType &type, const CVariable &var) const;

	// FIXME Sprite info. and Replace n with more appropriate var.
	char NSprite = -1;	/// Index of sprite. (@see DefineSprites and @see GetSpriteIndex)
	int n = 0;			/// identifiant in SpellSprite
	int FadeValue = 0;	/// if variable's value is below than FadeValue, it drawn transparent.
};

enum UnitTypeType {
	UnitTypeLand,  /// Unit lives on land
	UnitTypeFly,   /// Unit lives in air
	//Wyrmgus start
	UnitTypeFlyLow,   /// Unit lives in air, but flies low, so that it can be attacked by melee land units and cannot fly over rocks
	//Wyrmgus end
	UnitTypeNaval  /// Unit lives on water
};

enum DistanceTypeType {
	Equal,
	NotEqual,
	LessThan,
	LessThanEqual,
	GreaterThan,
	GreaterThanEqual
};

//Wyrmgus start
/// Base structure of unit-type
/// @todo n0body: AutoBuildRate not implemented.
class CUnitType : public DetailedDataElement, public DataType<CUnitType>
{
	GDCLASS(CUnitType, DetailedDataElement)
	
public:
	CUnitType();
	~CUnitType();
	
	static constexpr const char *ClassIdentifier = "unit_type";
	
	static std::vector<CUnitType *> GetUnitUnitTypes();
	static std::vector<CUnitType *> GetBuildingUnitTypes();
	static std::vector<CUnitType *> GetItemUnitTypes();
	static CUnitType *Add(const std::string &ident);
	static void Clear();

	virtual bool ProcessConfigDataProperty(const String &key, String value) override;
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	virtual void Initialize() override;
	
	const Vec2i &GetTileSize() const
	{
		return this->TileSize;
	}
	
	Vec2i GetHalfTileSize() const
	{
		return this->GetTileSize() / 2;
	}
	
	PixelSize GetHalfTilePixelSize() const
	{
		return GetTilePixelSize() / 2;
	}
	
	PixelSize GetTilePixelSize() const;
	
	Vec2i GetTileCenterPosOffset() const
	{
		return (this->GetTileSize() - 1) / 2;
	}

	bool CheckUserBoolFlags(const char *BoolFlags) const;
	//Wyrmgus start
//	bool CanTransport() const { return MaxOnBoard > 0 && !GivesResource; }
	bool CanTransport() const { return MaxOnBoard > 0; }
	//Wyrmgus end
	bool CanMove() const;

	bool CanSelect(GroupSelectionMode mode = SELECTABLE_BY_RECTANGLE_ONLY) const;
	
	void SetParent(CUnitType *parent_type);
	
	const UnitClass *GetClass() const
	{
		return this->Class;
	}
	
	CCivilization *GetCivilization() const
	{
		return this->Civilization;
	}
	
	CFaction *GetFaction() const
	{
		return this->Faction;
	}

	bool IsHiddenInEditor() const
	{
		return this->IsHidden() || this->TerrainType != nullptr || this->GetIcon() == nullptr || this->BoolFlag[VANISHES_INDEX].value;
	}
	
	bool IsUnitUnitType() const
	{
		return !this->BoolFlag[BUILDING_INDEX].value && !this->BoolFlag[ITEM_INDEX].value && !this->BoolFlag[DECORATION_INDEX].value && !this->BoolFlag[VANISHES_INDEX].value;
	}
	
	CSpecies *GetSpecies() const
	{
		return this->Species;
	}
	
	const PaletteImage *GetImage() const
	{
		return this->Image;
	}
	
	const Vector2i &GetFrameSize() const;
	
	const Vector2i &GetOffset() const
	{
		return this->Offset;
	}
	
	int GetOffsetX() const
	{
		return this->GetOffset().x;
	}
	
	int GetOffsetY() const
	{
		return this->GetOffset().y;
	}
	
	int GetDrawLevel() const
	{
		return this->DrawLevel;
	}
	
	//Wyrmgus start
	void RemoveButtons(int button_action = -1, std::string mod_file = "");
	void UpdateDefaultBoolFlags();
	int GetAvailableLevelUpUpgrades() const;
	int GetResourceStep(const int resource, const int player) const;
	CUnitTypeVariation *GetDefaultVariation(const CPlayer *player, const int image_layer = -1) const;
	CUnitTypeVariation *GetVariation(const std::string &variation_name, int image_layer = -1) const;
	std::string GetRandomVariationIdent(int image_layer = -1) const;
	std::string GetDefaultName(const CPlayer *player) const;
	CPlayerColorGraphic *GetDefaultLayerSprite(const CPlayer *player, const int image_layer) const;
	bool CanExperienceUpgradeTo(CUnitType *type) const;
	std::string GetNamePlural() const;
	std::string GeneratePersonalName(const CFaction *faction, const CGender *gender) const;
	std::vector<std::string> GetPotentialPersonalNames(const CFaction *faction, const CGender *gender) const;
	
	std::vector<String> GetStatStrings() const;
	//Wyrmgus end
	
	int GetRepairHP() const
	{
		return this->RepairHP;
	}

	int GetBoardSize() const
	{
		return this->BoardSize;
	}

public:
	CUnitType *Parent = nullptr;	/// Parent unit type
private:
	UnitClass *Class = nullptr;		/// Class identifier (i.e. infantry, archer, etc.)
	const CWord *NameWord = nullptr;		/// the word for the unit type's name
	CCivilization *Civilization = nullptr;	/// Which civilization this unit belongs to, if any
	CFaction *Faction = nullptr;	/// Which faction this unit belongs to, if any
public:
	//Wyrmgus start
	std::string RequirementsString;	/// Requirements string of the unit type
	std::string ExperienceRequirementsString;	/// Experience requirements string of the unit type
	std::string BuildingRulesString;	/// Building rules string of the unit type
	const CUpgrade *Elixir = nullptr;		/// Which elixir does this (item) unit type always have
	std::vector<CUnitType *> SoldUnits;		/// Units which this unit can sell.
	std::vector<CUnitType *> SpawnUnits;	/// Units which this unit can spawn.
	std::vector<CUnitType *> Drops;			/// Units which can spawn upon death (i.e. items).
	std::vector<CUnitType *> AiDrops;		/// Units which can spawn upon death (i.e. items), only for AI-controlled units.
	std::vector<CSpell *> DropSpells;		/// Spells which can be applied to dropped items
	std::vector<const CUpgrade *> Affixes;	/// Affixes which can be generated for this unit type
	std::vector<const CUpgrade *> Traits;	/// Which traits this unit type can have
	std::vector<const CUpgrade *> StartingAbilities;	/// Abilities which the unit starts out with
	std::vector<CUnitType *> Trains;		/// Units trained by this unit
	std::vector<CUnitType *> TrainedBy;		/// Units which can train this unit
	std::map<std::string, std::vector<CUnitType *>> ModTrains;	/// Units trained by this unit (as set in a mod)
	std::map<std::string, std::vector<CUnitType *>> ModTrainedBy;	/// Units which can train this unit (as set in a mod)
	std::map<std::string, std::vector<CUnitType *>> ModAiDrops;	/// Units dropped by this unit, if it is AI-controlled (as set in a mod)
	//Wyrmgus end
private:
	PaletteImage *Image = nullptr;	/// the unit type's sprite
public:
	std::string ShadowFile;			/// Shadow file
	//Wyrmgus start
	std::string LightFile;			/// Light file
	std::string LayerFiles[MaxImageLayers];	/// Layer files
	std::map<int, IconConfig> ButtonIcons;				/// icons for button actions
	std::map<const ItemSlot *, CUnitType *> DefaultEquipment;	/// default equipment for the unit type, mapped to item slots
	//Wyrmgus end

private:
	Vector2i Offset = Vector2i(0, 0);					/// Sprite offset
	int DrawLevel = 0;									/// Level to Draw UnitType at
public:
	int ShadowWidth = 0;								/// Shadow sprite width
	int ShadowHeight = 0;								/// Shadow sprite height
	int ShadowOffsetX = 0;								/// Shadow horizontal offset
	int ShadowOffsetY = 0;								/// Shadow vertical offset
	//Wyrmgus start
	int TrainQuantity = 0;								/// Quantity to be trained
	int CostModifier = 0;								/// Cost modifier (cost increase for every unit of this type the player has)
	const ItemClass *ItemClass = nullptr;				/// Item class (if the unit type is an item)
private:
	CSpecies *Species = nullptr;
public:
	CTerrainType *TerrainType = nullptr;				/// The terrain type which the unit type becomes after being built
	std::vector<const ::ItemClass *> WeaponClasses;		/// Weapon classes that the unit type can use (if the unit type uses a weapon)
	std::map<const CGender *, std::vector<std::string>> PersonalNames;	/// Personal names for the unit type, mapped to the gender they pertain to (use nullptr for names which should be available for both genders)
	//Wyrmgus end
	PixelPos MissileOffsets[UnitSides][MaxAttackPos];     /// Attack offsets for missiles

	CAnimations *Animations = nullptr;	/// Animation scripts
	int StillFrame = 0;					/// Still frame

	MissileConfig Missile;			/// Missile weapon
	//Wyrmgus start
	MissileConfig FireMissile;		/// Missile weapon if the unit has fire damage
	//Wyrmgus end
	MissileConfig Explosion;		/// Missile for unit explosion
	MissileConfig Impact[ANIMATIONS_DEATHTYPES + 2];	/// Missiles spawned if unit is hit(+shield)

	LuaCallback *DeathExplosion = nullptr;
	LuaCallback *OnEachSecond = nullptr;	/// lua function called every second
	LuaCallback *OnInit = nullptr;			/// lua function called on unit init

	int TeleportCost = 0;						/// mana used for teleportation

	mutable std::string DamageType;	/// DamageType (used for extra death animations and impacts)

	std::string CorpseName;				/// Corpse type name
	CUnitType *CorpseType = nullptr;	/// Corpse unit-type

	CConstruction *Construction = nullptr;	/// What is shown in construction phase

private:
	int RepairHP = 0;				/// Amount of HP per repair
public:
	int RepairCosts[MaxCosts];		/// How much it costs to repair

	Vec2i TileSize = Vec2i(0, 0);	/// Tile size
	int BoxWidth = 0;				/// Selected box size width
	int BoxHeight = 0;				/// Selected box size height
	int BoxOffsetX = 0;				/// Selected box size horizontal offset
	int BoxOffsetY = 0;				/// Selected box size vertical offset
	int NumDirections = 0;			/// Number of directions unit can face
	int MinAttackRange = 0;			/// Minimal attack range
	int BurnPercent = 0;			/// Burning percent.
	int BurnDamageRate = 0;			/// HP burn rate per sec
	int RepairRange = 0;			/// Units repair range.
#define InfiniteRepairRange INT_MAX
	std::vector<CSpell *> Spells;	/// Spells the unit is able to cast.
	std::set<const CSpell *> AutoCastActiveSpells;	/// Default spells for autocast.
	int AutoBuildRate = 0;			/// The rate at which the building builds itself
	int RandomMovementProbability = 0;	/// Probability to move randomly.
	int RandomMovementDistance = 1;	/// Quantity of tiles to move randomly.
	int ClicksToExplode = 0;		/// Number of consecutive clicks until unit suicides.
	int MaxOnBoard = 0;				/// Number of Transporter slots.
private:
	int BoardSize = 1;				/// How many "cells" the unit occupies inside a transporter
public:
	CButtonLevel *ButtonLevelForTransporter = nullptr;	/// On which button level game will show units inside transporter
	//Wyrmgus start
	int ButtonPos = 0;				/// Position of this unit as a train/build button
	CButtonLevel *ButtonLevel = nullptr;	/// Level of this unit's button
	std::string ButtonPopup;		/// Popup of this unit's button
	std::string ButtonHint;			/// Hint of this unit's button
	std::string ButtonKey;			/// Hotkey of this unit's button
//	int StartingResources = 0;		/// Amount of Resources on build
	std::vector<int> StartingResources;	/// Amount of Resources on build
	//Wyrmgus end
	/// originally only visual effect, we do more with this!
	UnitTypeType UnitType = UnitTypeLand;	/// Land / fly / naval
	int DecayRate = 0;				/// Decay rate in 1/6 seconds
	// TODO: not used
	int AnnoyComputerFactor = 0;	/// How much this annoys the computer
	int AiAdjacentRange = -1;		/// Min radius for AI build surroundings checking
	int MouseAction = 0;			/// Right click action
#define MouseActionNone      0		/// Nothing
#define MouseActionAttack    1		/// Attack
#define MouseActionMove      2		/// Move
#define MouseActionHarvest   3		/// Harvest resources
#define MouseActionSpellCast 5		/// Cast the first spell known
#define MouseActionSail      6		/// Sail
//Wyrmgus start
#define MouseActionRallyPoint 7		/// Rally point
#define MouseActionTrade      8		/// Trade
//Wyrmgus end
	int CanTarget = 0;				/// Which units can it attack
#define CanTargetLand 1				/// Can attack land units
#define CanTargetSea  2				/// Can attack sea units
#define CanTargetAir  4				/// Can attack air units

	bool Flip = true;				/// Flip image when facing left
	bool LandUnit = false;			/// Land animated
	bool AirUnit = false;			/// Air animated
	bool SeaUnit = false;			/// Sea animated
	bool ExplodeWhenKilled = false;	/// Death explosion animated
	bool CanAttack = false;			/// Unit can attack.
	bool Neutral = false;			/// Unit is neutral, used by the editor

	CUnitStats DefaultStat;
	CUnitStats MapDefaultStat;
	//Wyrmgus start
	std::map<std::string, CUnitStats> ModDefaultStats;
	//Wyrmgus end
	struct BoolFlags {
		bool value;				/// User defined flag. Used for (dis)allow target.
		char CanTransport;		/// Can transport units with this flag.
		char CanTargetFlag;		/// Flag needed to target with missile.
		char AiPriorityTarget;	/// Attack this units first.
	};
	std::vector<BoolFlags> BoolFlag;

	int CanStore[MaxCosts];				/// Resources that we can store here.
	int GivesResource = 0;				/// The resource this unit gives.
	//Wyrmgus start
	int GrandStrategyProductionEfficiencyModifier[MaxCosts];	/// production modifier for a particular resource for grand strategy mode (used for buildings)
	//Wyrmgus end
	ResourceInfo *ResInfo[MaxCosts];	/// Resource information.
	std::vector<CUnitTypeVariation *> Variations;				/// Variation information
	//Wyrmgus start
	std::vector<CUnitTypeVariation *> LayerVariations[MaxImageLayers];	/// Layer variation information
	//Wyrmgus end
	std::vector<CBuildRestriction *> BuildingRules;		/// Rules list for building a building.
	std::vector<CBuildRestriction *> AiBuildingRules;	/// Rules list for for AI to build a building.
	CColor NeutralMinimapColorRGB;	/// Minimap Color for Neutral Units.

	CUnitSound Sound;				/// Sounds for events
	CUnitSound MapSound;			/// Sounds for events, map-specific
	//Wyrmgus start
	std::map<std::string, CUnitSound> ModSounds;
	//Wyrmgus end

	int PoisonDrain = 0;			/// How much health is drained every second when poisoned

	// --- FILLED UP ---

	uint16_t FieldFlags = 0;		/// Unit map field flags
	uint16_t MovementMask = 0;		/// Unit check this map flags for move

	/// @todo This stats should? be moved into the player struct
	CUnitStats Stats[PlayerMax];	/// Unit status for each player

	CPlayerColorGraphic *Sprite = nullptr;	/// Sprite images
	CGraphic *ShadowSprite = nullptr;		/// Shadow sprite image
	//Wyrmgus start
	CGraphic *LightSprite = nullptr;		/// Light sprite image
	CPlayerColorGraphic *LayerSprites[MaxImageLayers];	/// Layer sprite images
	
	CDependency *Predependency = nullptr;
	CDependency *Dependency = nullptr;
	
	std::string Mod;				/// To which mod (or map), if any, this unit type belongs
	//Wyrmgus end
	
	friend int CclDefineUnitType(lua_State *l);

protected:
	static void _bind_methods();
};

class CBuildRestriction
{
public:
	virtual ~CBuildRestriction() {}
	virtual void Init() {};
	//Wyrmgus start
//	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget) const = 0;
	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const = 0;
	//Wyrmgus end
	
	virtual CBuildRestriction *Duplicate() = 0;
};

class CBuildRestrictionAnd : public CBuildRestriction
{
public:
	virtual ~CBuildRestrictionAnd()
	{
		for (std::vector<CBuildRestriction *>::const_iterator i = _or_list.begin();
			 i != _or_list.end(); ++i) {
			delete *i;
		}
		_or_list.clear();
	}
	virtual void Init()
	{
		for (std::vector<CBuildRestriction *>::const_iterator i = _or_list.begin();
			 i != _or_list.end(); ++i) {
			(*i)->Init();
		}
	}
	//Wyrmgus start
//	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget) const;
	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const;
	//Wyrmgus end
	
	virtual CBuildRestriction *Duplicate() override
	{
		CBuildRestrictionAnd *duplicate = new CBuildRestrictionAnd;
		
		for (CBuildRestriction *build_restriction : this->_or_list) {
			duplicate->push_back(build_restriction->Duplicate());
		}
		
		return duplicate;
	}

	void push_back(CBuildRestriction *restriction) { _or_list.push_back(restriction); }
public:
	std::vector<CBuildRestriction *> _or_list;
};

//Wyrmgus start
class CBuildRestrictionOr : public CBuildRestriction
{
public:
	virtual ~CBuildRestrictionOr()
	{
		for (std::vector<CBuildRestriction *>::const_iterator i = _or_list.begin();
			 i != _or_list.end(); ++i) {
			delete *i;
		}
		_or_list.clear();
	}
	virtual void Init()
	{
		for (std::vector<CBuildRestriction *>::const_iterator i = _or_list.begin();
			 i != _or_list.end(); ++i) {
			(*i)->Init();
		}
	}
	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const;
	
	virtual CBuildRestriction *Duplicate() override
	{
		CBuildRestrictionOr *duplicate = new CBuildRestrictionOr;
		
		for (CBuildRestriction *build_restriction : this->_or_list) {
			duplicate->push_back(build_restriction->Duplicate());
		}
		
		return duplicate;
	}

	void push_back(CBuildRestriction *restriction) { _or_list.push_back(restriction); }
public:
	std::vector<CBuildRestriction *> _or_list;
};
//Wyrmgus end

class CBuildRestrictionAddOn : public CBuildRestriction
{
	class functor
	{
	public:
		functor(const CUnitType *type, const Vec2i &_pos): Parent(type), pos(_pos) {}
		inline bool operator()(const CUnit *const unit) const;
	private:
		const CUnitType *const Parent;   /// building that is unit is an addon too.
		const Vec2i pos; //functor work position
	};
public:
	virtual ~CBuildRestrictionAddOn() {}
	virtual void Init() {this->Parent = CUnitType::Get(this->ParentName); }
	//Wyrmgus start
//	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget) const;
	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const;
	//Wyrmgus end
	
	virtual CBuildRestriction *Duplicate() override
	{
		CBuildRestrictionAddOn *duplicate = new CBuildRestrictionAddOn;
		duplicate->Offset = this->Offset;
		duplicate->ParentName = this->ParentName;
		duplicate->Parent = this->Parent;
		return duplicate;
	}

	Vec2i Offset = Vec2i(0, 0);		/// offset from the main building to place this
	std::string ParentName;			/// building that is unit is an addon too.
	CUnitType *Parent = nullptr;	/// building that is unit is an addon too.
};

class CBuildRestrictionOnTop : public CBuildRestriction
{
	class functor
	{
	public:
		functor(const CUnitType *type, const Vec2i &_pos): ontop(0), Parent(type), pos(_pos) {}
		inline bool operator()(CUnit *const unit);
		CUnit *ontop;   /// building that is unit is an addon too.
	private:
		const CUnitType *const Parent;  /// building that is unit is an addon too.
		const Vec2i pos;  //functor work position
	};
public:
	virtual ~CBuildRestrictionOnTop() {};
	virtual void Init() {this->Parent = CUnitType::Get(this->ParentName);};
	//Wyrmgus start
//	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget) const;
	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const;
	//Wyrmgus end
	
	virtual CBuildRestriction *Duplicate() override
	{
		CBuildRestrictionOnTop *duplicate = new CBuildRestrictionOnTop;
		duplicate->ParentName = this->ParentName;
		duplicate->Parent = this->Parent;
		duplicate->ReplaceOnDie = this->ReplaceOnDie;
		duplicate->ReplaceOnBuild = this->ReplaceOnBuild;
		return duplicate;
	}

	std::string ParentName;	/// building that is unit is an addon too.
	CUnitType *Parent = nullptr;	/// building that is unit is an addon too.
	int ReplaceOnDie = 0;	/// recreate the parent on destruction
	int ReplaceOnBuild = 0;	/// remove the parent, or just build over it.
};

class CBuildRestrictionDistance : public CBuildRestriction
{
public:
	virtual ~CBuildRestrictionDistance() {};
	virtual void Init();
	//Wyrmgus start
//	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget) const;
	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const;
	//Wyrmgus end
	
	virtual CBuildRestriction *Duplicate() override
	{
		CBuildRestrictionDistance *duplicate = new CBuildRestrictionDistance;
		duplicate->Distance = this->Distance;
		duplicate->DistanceType = this->DistanceType;
		duplicate->RestrictTypeName = this->RestrictTypeName;
		duplicate->RestrictTypeOwner = this->RestrictTypeOwner;
		duplicate->RestrictType = this->RestrictType;
		duplicate->RestrictClassName = this->RestrictClassName;
		duplicate->RestrictClass = this->RestrictClass;
		duplicate->CheckBuilder = this->CheckBuilder;
		duplicate->Diagonal = this->Diagonal;
		return duplicate;
	}

	int Distance = 0;	/// distance to build (circle)
	DistanceTypeType DistanceType;
	std::string RestrictTypeName;
	std::string RestrictTypeOwner;
	const CUnitType *RestrictType = nullptr;
	std::string RestrictClassName;
	const UnitClass *RestrictClass = nullptr;
	bool CheckBuilder = false;
	bool Diagonal = true;
};

class CBuildRestrictionHasUnit : public CBuildRestriction
{
public:
	virtual ~CBuildRestrictionHasUnit() {};
	virtual void Init() { this->RestrictType = CUnitType::Get(this->RestrictTypeName); };
	//Wyrmgus start
//	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget) const;
	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const;
	//Wyrmgus end
	
	virtual CBuildRestriction *Duplicate() override
	{
		CBuildRestrictionHasUnit *duplicate = new CBuildRestrictionHasUnit;
		duplicate->Count = this->Count;
		duplicate->CountType = this->CountType;
		duplicate->RestrictTypeName = this->RestrictTypeName;
		duplicate->RestrictTypeOwner = this->RestrictTypeOwner;
		duplicate->RestrictType = this->RestrictType;
		return duplicate;
	}
	
	int Count = 0;
	DistanceTypeType CountType;
	std::string RestrictTypeName;
	std::string RestrictTypeOwner;
	CUnitType *RestrictType = nullptr;
};

class CBuildRestrictionSurroundedBy : public CBuildRestriction
{
public:
	virtual ~CBuildRestrictionSurroundedBy() {}
	virtual void Init() { this->RestrictType = CUnitType::Get(this->RestrictTypeName); }
	//Wyrmgus start
//	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget) const;
	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const;
	//Wyrmgus end
	
	virtual CBuildRestriction *Duplicate() override
	{
		CBuildRestrictionSurroundedBy *duplicate = new CBuildRestrictionSurroundedBy;
		duplicate->Distance = this->Distance;
		duplicate->DistanceType = this->DistanceType;
		duplicate->Count = this->Count;
		duplicate->CountType = this->CountType;
		duplicate->RestrictTypeName = this->RestrictTypeName;
		duplicate->RestrictTypeOwner = this->RestrictTypeOwner;
		duplicate->RestrictType = this->RestrictType;
		duplicate->CheckBuilder = this->CheckBuilder;
		return duplicate;
	}
	
	int Distance = 0;
	DistanceTypeType DistanceType = Equal;
	int Count = 0;
	DistanceTypeType CountType = Equal;
	std::string RestrictTypeName;
	std::string RestrictTypeOwner;
	CUnitType *RestrictType = nullptr;
	bool CheckBuilder = false;
};

//Wyrmgus start
class CBuildRestrictionTerrain : public CBuildRestriction
{
public:
	virtual ~CBuildRestrictionTerrain() {}
	virtual void Init();
	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const;
	
	virtual CBuildRestriction *Duplicate() override
	{
		CBuildRestrictionTerrain *duplicate = new CBuildRestrictionTerrain;
		duplicate->RestrictTerrainTypeName = this->RestrictTerrainTypeName;
		duplicate->RestrictTerrainType = this->RestrictTerrainType;
		return duplicate;
	}

	std::string RestrictTerrainTypeName;
	const CTerrainType *RestrictTerrainType = nullptr;
};
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/// @todo this hardcoded unit-types must be removed!!
//Wyrmgus start
//extern CUnitType *UnitTypeHumanWall;  /// Human wall
//extern CUnitType *UnitTypeOrcWall;    /// Orc wall
//Wyrmgus end

/**
**  Variable info for unit and unit type.
*/
class CUnitTypeVar
{
public:
	static constexpr int SpellEffects[] = {BLOODLUST_INDEX, HASTE_INDEX, SLOW_INDEX, INVISIBLE_INDEX, UNHOLYARMOR_INDEX, POISON_INDEX, STUN_INDEX, BLEEDING_INDEX, LEADERSHIP_INDEX, BLESSING_INDEX, INSPIRE_INDEX, PRECISION_INDEX, REGENERATION_INDEX, BARKSKIN_INDEX, INFUSION_INDEX, TERROR_INDEX, WITHER_INDEX, DEHYDRATION_INDEX, HYDRATING_INDEX};
	
	template <const unsigned int SIZE>
	struct CKeys {

		struct DataKey {
			static bool key_pred(const DataKey &lhs,
								 const DataKey &rhs)
			{
				return ((lhs.keylen == rhs.keylen) ?
						(strcmp(lhs.key, rhs.key) < 0) : (lhs.keylen < rhs.keylen));
			}
			int offset;
			unsigned int keylen;
			const char *key;
		};

		CKeys(): TotalKeys(SIZE) {}

		DataKey buildin[SIZE];
		std::map<std::string, int> user;
		unsigned int TotalKeys;

		void Init()
		{
			std::sort(buildin, buildin + SIZE, DataKey::key_pred);
		}

		const char *operator[](int index)
		{
			for (unsigned int i = 0; i < SIZE; ++i) {
				if (buildin[i].offset == index) {
					return buildin[i].key;
				}
			}
			for (std::map<std::string, int>::iterator
				 it(user.begin()), end(user.end());
				 it != end; ++it) {
				if ((*it).second == index) {
					return ((*it).first).c_str();
				}
			}
			return nullptr;
		}

		/**
		**  Return the index of the external storage array/vector.
		**
		**  @param varname  Name of the variable.
		**
		**  @return Index of the variable, -1 if not found.
		*/
		int operator[](const char *const key)
		{
			DataKey k;
			k.key = key;
			k.keylen = strlen(key);
			const DataKey *p = std::lower_bound(buildin, buildin + SIZE,
												k, DataKey::key_pred);
			if ((p != buildin + SIZE) && p->keylen == k.keylen &&
				0 == strcmp(p->key, key)) {
				return p->offset;
			} else {
				std::map<std::string, int>::iterator
				ret(user.find(key));
				if (ret != user.end()) {
					return (*ret).second;
				}
			}
			return -1;
		}

		int AddKey(const char *const key)
		{
			int index = this->operator[](key);
			if (index != -1) {
				DebugPrint("Warning, Key '%s' already defined\n" _C_ key);
				return index;
			}
			user[key] = TotalKeys++;
			return TotalKeys - 1;
		}

	};

	struct CBoolKeys : public CKeys<NBARALREADYDEFINED> {
		CBoolKeys();
	};

	struct CVariableKeys : public CKeys<NVARALREADYDEFINED> {
		CVariableKeys();
	};

	CUnitTypeVar() {}

	void Init();
	void Clear();

	CBoolKeys BoolFlagNameLookup;      /// Container of name of user defined bool flag.
	CVariableKeys VariableNameLookup;  /// Container of names of user defined variables.

	//EventType *Event;                  /// Array of functions sets to call when en event occurs.
	std::vector<CVariable> Variable;   /// Array of user defined variables (default value for unittype).
	std::vector<CDecoVar *> DecoVar;   /// Array to describe how showing variable.

	unsigned int GetNumberBoolFlag() const
	{
		return BoolFlagNameLookup.TotalKeys;
	}

	unsigned int GetNumberVariable() const
	{
		return VariableNameLookup.TotalKeys;
	}
};

extern CUnitTypeVar UnitTypeVar;

//Wyrmgus start
extern CUnitType *SettlementSiteUnitType;
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern CUnitType *CclGetUnitType(lua_State *l);  /// Access unit-type object
extern void UnitTypeCclRegister();               /// Register ccl features

extern void UpdateUnitStats(CUnitType &type, int reset_to_default);       /// Update unit stats
extern void UpdateStats(int reset_to_default);       /// Update unit stats

//Wyrmgus start
extern std::string GetUnitTypeStatsString(const std::string &unit_type_ident);
//Wyrmgus end

extern void SaveUnitTypes(CFile &file);              /// Save the unit-type table
/// Draw the sprite frame of unit-type
extern void DrawUnitType(const CUnitType &type, CPlayerColorGraphic *sprite,
						 int player, int frame, const PixelPos &screenPos);

extern void InitUnitTypes(int reset_player_stats);   /// Init unit-type table
//Wyrmgus start
extern void InitUnitType(CUnitType &type);			/// Init unit-type
//Wyrmgus end
extern void LoadUnitTypeSprite(CUnitType &unittype); /// Load the sprite for a unittype
extern int GetUnitTypesCount();                     /// Get the amount of unit-types
extern void LoadUnitTypes();                     /// Load the unit-type data
//Wyrmgus start
extern void LoadUnitType(CUnitType &unittype);	/// Load a unittype
//Wyrmgus end

// in script_unit_type.cpp

/// Parse User Variables field.
extern void DefineVariableField(lua_State *l, CVariable *var, int lua_index);

/// Update custom Variables with other variable (like Hp, ...)
extern void UpdateUnitVariables(CUnit &unit);

extern void SetModStat(const std::string &mod_file, const std::string &ident, const std::string &variable_key, const int value, const std::string &variable_type);
extern void SetModSound(const std::string &mod_file, const std::string &ident, const std::string &sound, const std::string &sound_type, const std::string &sound_subtype = "");

//Wyrmgus start
extern std::string GetImageLayerNameById(int image_layer);
extern int GetImageLayerIdByName(const std::string &image_layer);
//Wyrmgus end

#endif
