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
/**@name unittype.h - The unit-types headerfile. */
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

#ifndef __UNITTYPE_H__
#define __UNITTYPE_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#ifndef __UPGRADE_STRUCTS_H__
#include "upgrade_structs.h"
#endif

#ifndef __UNITSOUND_H__
#include "unitsound.h"
#endif

#ifndef __ICONS_H__
#include "icons.h"
#endif

#include "color.h"
//Wyrmgus start
#include "map.h"	//required for PixelTileSize
//Wyrmgus end
#include "missileconfig.h"
#include "vec2i.h"

#include <climits>
#include <vector>
#include <algorithm>
#include <map>
#include <cstring>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CAnimations;
class CPlayerColorGraphic;
class CConstruction;
class MissileType;
class CFile;
//Wyrmgus start
class CUniqueItem;
//Wyrmgus end
struct lua_State;
#ifdef USE_MNG
class Mng;
#endif
class LuaCallback;

#define UnitSides 8
#define MaxAttackPos 5

CUnitType *UnitTypeByIdent(const std::string &ident);

enum GroupSelectionMode {
	SELECTABLE_BY_RECTANGLE_ONLY = 0,
	NON_SELECTABLE_BY_RECTANGLE_ONLY,
	SELECT_ALL
};

//Wyrmgus start
enum ImageLayers {
	LeftArmImageLayer,
	RightArmImageLayer,
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
	
	MaxImageLayers
};
//Wyrmgus end

class ResourceInfo
{
public:
	ResourceInfo() : WaitAtResource(0), ResourceStep(0),
		ResourceCapacity(0), WaitAtDepot(0), ResourceId(0), FinalResource(0),
		//Wyrmgus start
//		TerrainHarvester(0), LoseResources(0), HarvestFromOutside(0),
		FinalResourceConversionRate(0), LoseResources(0),
		//Wyrmgus end
		SpriteWhenLoaded(NULL), SpriteWhenEmpty(NULL)
	{}

	std::string FileWhenLoaded;     /// Change the graphic when the unit is loaded.
	std::string FileWhenEmpty;      /// Change the graphic when the unit is empty.
	unsigned WaitAtResource;        /// Cycles the unit waits while mining.
	unsigned ResourceStep;          /// Resources the unit gains per mining cycle.
	int      ResourceCapacity;      /// Max amount of resources to carry.
	unsigned WaitAtDepot;           /// Cycles the unit waits while returning.
	unsigned ResourceId;            /// Id of the resource harvested. Redundant.
	unsigned FinalResource;         /// Convert resource when delivered.
	//Wyrmgus start
//	unsigned char TerrainHarvester;    /// Unit will harvest terrain(wood only for now).
	unsigned FinalResourceConversionRate;         /// Rate at which the resource is converted when delivered.
	//Wyrmgus end
	unsigned char LoseResources;       /// The unit will lose it's resource when distracted.
	//Wyrmgus start
//	unsigned char HarvestFromOutside;  /// Unit harvests without entering the building.
	//Wyrmgus end
	unsigned char RefineryHarvester;   /// Unit have to build Refinery buildings for harvesting.
	//  Runtime info:
	CPlayerColorGraphic *SpriteWhenLoaded; /// The graphic corresponding to FileWhenLoaded.
	CPlayerColorGraphic *SpriteWhenEmpty;  /// The graphic corresponding to FileWhenEmpty
};

//Wyrmgus start
//unit variations
class VariationInfo
{
public:
	VariationInfo() : VariationId(""),
		FrameWidth(0), FrameHeight(0),
		Animations(NULL), Construction(NULL), Sprite(NULL), ShadowSprite(NULL)
	{
		memset(LayerSprites, 0, sizeof(LayerSprites));
		memset(SpriteWhenLoaded, 0, sizeof(SpriteWhenLoaded));
		memset(SpriteWhenEmpty, 0, sizeof(SpriteWhenEmpty));
	}
	
	~VariationInfo();

	std::string VariationId;		/// Variation's name.
	std::string TypeName;			/// Type name.
	std::string File;				/// Variation's graphics.
	std::string ShadowFile;			/// Variation's shadow graphics.
	std::string Tileset;			/// Variation's tileset.
	int FrameWidth;
	int FrameHeight;
	IconConfig Icon;				/// Icon to display for this unit
	CPlayerColorGraphic *Sprite;	/// The graphic corresponding to File.
	CGraphic *ShadowSprite;			/// The graphic corresponding to ShadowFile.
	CAnimations *Animations;        /// Animation scripts
	CConstruction *Construction;    /// What is shown in construction phase

	std::string UpgradesRequired[VariationMax];	/// Upgrades required by variation
	std::string UpgradesForbidden[VariationMax];	/// If player has one of these upgrades, unit can't have this variation
	std::vector<CUnitType *> ItemsEquipped;
	std::vector<CUnitType *> ItemsNotEquipped;

	std::string LayerFiles[MaxImageLayers];	/// Variation's layer graphics.
	std::string FileWhenLoaded[MaxCosts];     /// Change the graphic when the unit is loaded.
	std::string FileWhenEmpty[MaxCosts];      /// Change the graphic when the unit is empty.
	CPlayerColorGraphic *LayerSprites[MaxImageLayers];	/// The graphics corresponding to LayerFiles.
	CPlayerColorGraphic *SpriteWhenLoaded[MaxCosts]; /// The graphic corresponding to FileWhenLoaded.
	CPlayerColorGraphic *SpriteWhenEmpty[MaxCosts];  /// The graphic corresponding to FileWhenEmpty
};
//Wyrmgus end

/**
**  User defined variable type.
**
**  It is used to define variables and use it after
**  to manage magic, energy, shield or other stuff.
*/
class CVariable
{
public:
	CVariable() : Max(0), Value(0), Increase(0), Enable(0) {}

	bool operator ==(const CVariable &rhs) const
	{
		return this->Max == rhs.Max
			   && this->Value == rhs.Value
			   && this->Increase == rhs.Increase
			   && this->Enable == rhs.Enable;
	}
	bool operator !=(const CVariable &rhs) const { return !(*this == rhs); }

public:
	int Max;        /// Maximum for the variable. (Assume min is 0.)
	int Value;      /// Current (or initial) value of the variable (or initial value).
	char Increase;  /// Number to increase(decrease) Value by second.
	char Enable;    /// True if the unit doesn't have this variable. (f.e shield)
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
	ITEM_INDEX,
	POWERUP_INDEX,
	INVENTORY_INDEX,
	TRAP_INDEX,
	BRIDGE_INDEX,
	MERCENARY_INDEX,
	FAUNA_INDEX,
	PREDATOR_INDEX,
	SLIME_INDEX,
	PEOPLEAVERSION_INDEX,
	MOUNTED_INDEX,
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
	GRAVEL_INDEX,
	HACKDAMAGE_INDEX,
	PIERCEDAMAGE_INDEX,
	BLUNTDAMAGE_INDEX,
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
	SPEED_INDEX,
	FIRERESISTANCE_INDEX,
	COLDRESISTANCE_INDEX,
	ARCANERESISTANCE_INDEX,
	LIGHTNINGRESISTANCE_INDEX,
	AIRRESISTANCE_INDEX,
	EARTHRESISTANCE_INDEX,
	WATERRESISTANCE_INDEX,
	HACKRESISTANCE_INDEX,
	PIERCERESISTANCE_INDEX,
	BLUNTRESISTANCE_INDEX,
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
	ACCURACY_INDEX,
	EVASION_INDEX,
	LEVEL_INDEX,
	LEVELUP_INDEX,
	XPREQUIRED_INDEX,
	VARIATION_INDEX,
	HITPOINTHEALING_INDEX,
	HITPOINTBONUS_INDEX,
	CRITICALSTRIKECHANCE_INDEX,
	BACKSTAB_INDEX,
	BONUSAGAINSTMOUNTED_INDEX,
	BONUSAGAINSTBUILDINGS_INDEX,
	BONUSAGAINSTAIR_INDEX,
	BONUSAGAINSTGIANTS_INDEX,
	BONUSAGAINSTDRAGONS_INDEX,
	DAYSIGHTRANGEBONUS_INDEX,
	NIGHTSIGHTRANGEBONUS_INDEX,
	TRANSPARENCY_INDEX,
	GENDER_INDEX,
	BIRTHCYCLE_INDEX,
	HUNGER_INDEX,
	STUN_INDEX,
	BLEEDING_INDEX,
	TIMEEFFICIENCYBONUS_INDEX,
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

	int OffsetXPercent;     /// Percent offset (TileWidth) in X coord.
	int OffsetYPercent;     /// Percent offset (TileHeight) in Y coord.

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
	CDecoVarText() : Font(NULL) {};
	/// function to draw the decorations.
	virtual void Draw(int x, int y, const CUnitType &type, const CVariable &var) const;

	CFont *Font;  /// Font to use to display value.
	// FIXME : Add Color, format
};

/// Sprite contains frame from full (left)to empty state (right).
class CDecoVarSpriteBar : public CDecoVar
{
public:
	CDecoVarSpriteBar() : NSprite(-1) {};
	/// function to draw the decorations.
	virtual void Draw(int x, int y,
					  const CUnitType &type, const CVariable &var) const;

	char NSprite; /// Index of number. (@see DefineSprites and @see GetSpriteIndex)
	// FIXME Sprite info. better way ?
};

/// use to show specific frame in a sprite.
class CDecoVarStaticSprite : public CDecoVar
{
public:
	CDecoVarStaticSprite() : NSprite(-1), n(0), FadeValue(0) {}
	/// function to draw the decorations.
	virtual void Draw(int x, int y, const CUnitType &type, const CVariable &var) const;

	// FIXME Sprite info. and Replace n with more appropriate var.
	char NSprite;  /// Index of sprite. (@see DefineSprites and @see GetSpriteIndex)
	int n;         /// identifiant in SpellSprite
	int FadeValue; /// if variable's value is below than FadeValue, it drawn transparent.
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

class CBuildRestriction
{
public:
	virtual ~CBuildRestriction() {}
	virtual void Init() {};
	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget) const = 0;
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
	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget) const;

	void push_back(CBuildRestriction *restriction) { _or_list.push_back(restriction); }
public:
	std::vector<CBuildRestriction *> _or_list;
};

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
	CBuildRestrictionAddOn() : Offset(0, 0), Parent(NULL) {}
	virtual ~CBuildRestrictionAddOn() {}
	virtual void Init() {this->Parent = UnitTypeByIdent(this->ParentName);}
	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget) const;

	Vec2i Offset;           /// offset from the main building to place this
	std::string ParentName; /// building that is unit is an addon too.
	CUnitType *Parent;      /// building that is unit is an addon too.
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
	CBuildRestrictionOnTop() : Parent(NULL), ReplaceOnDie(0), ReplaceOnBuild(0) {};
	virtual ~CBuildRestrictionOnTop() {};
	virtual void Init() {this->Parent = UnitTypeByIdent(this->ParentName);};
	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget) const;

	std::string ParentName;  /// building that is unit is an addon too.
	CUnitType *Parent;       /// building that is unit is an addon too.
	int ReplaceOnDie: 1;     /// recreate the parent on destruction
	int ReplaceOnBuild: 1;   /// remove the parent, or just build over it.
};

class CBuildRestrictionDistance : public CBuildRestriction
{
public:
	CBuildRestrictionDistance() : Distance(0), CheckBuilder(false), RestrictType(NULL), Diagonal(true) {};
	virtual ~CBuildRestrictionDistance() {};
	virtual void Init() {this->RestrictType = UnitTypeByIdent(this->RestrictTypeName);};
	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget) const;

	int Distance;        /// distance to build (circle)
	DistanceTypeType DistanceType;
	std::string RestrictTypeName;
	std::string RestrictTypeOwner;
	CUnitType *RestrictType;
	bool CheckBuilder;
	bool Diagonal;
};

class CBuildRestrictionHasUnit : public CBuildRestriction
{
public:
	CBuildRestrictionHasUnit() : Count(0), RestrictType(NULL) {};
	virtual ~CBuildRestrictionHasUnit() {};
	virtual void Init() { this->RestrictType = UnitTypeByIdent(this->RestrictTypeName); };
	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget) const;
	
	int Count;
	DistanceTypeType CountType;
	std::string RestrictTypeName;
	CUnitType *RestrictType;
	std::string RestrictTypeOwner;
};

class CBuildRestrictionSurroundedBy : public CBuildRestriction
{
public:
	CBuildRestrictionSurroundedBy() : Count(0), Distance(0), DistanceType(Equal), CountType(Equal), RestrictType(NULL), CheckBuilder(false) {};
	virtual ~CBuildRestrictionSurroundedBy() {};
	virtual void Init() { this->RestrictType = UnitTypeByIdent(this->RestrictTypeName); };
	virtual bool Check(const CUnit *builder, const CUnitType &type, const Vec2i &pos, CUnit *&ontoptarget) const;
	
	int Distance;
	DistanceTypeType DistanceType;
	int Count;
	DistanceTypeType CountType;
	std::string RestrictTypeName;
	std::string RestrictTypeOwner;
	CUnitType *RestrictType;
	bool CheckBuilder;
};

/// Base structure of unit-type
/// @todo n0body: AutoBuildRate not implemented.
class CUnitType
{
public:
	CUnitType();
	~CUnitType();

	Vec2i GetHalfTileSize() const { return Vec2i(TileWidth / 2, TileHeight / 2); }
	//Wyrmgus start
	Vec2i GetHalfTilePixelSize() const { return Vec2i(TileWidth * PixelTileSize.x / 2, TileHeight * PixelTileSize.y / 2); }
	//Wyrmgus end
	PixelSize GetPixelSize() const;

	bool CheckUserBoolFlags(const char *BoolFlags) const;
	//Wyrmgus start
//	bool CanTransport() const { return MaxOnBoard > 0 && !GivesResource; }
	bool CanTransport() const { return MaxOnBoard > 0; }
	//Wyrmgus end
	bool CanMove() const;

	bool CanSelect(GroupSelectionMode mode = SELECTABLE_BY_RECTANGLE_ONLY) const;
	
	//Wyrmgus start
	int GetAvailableLevelUpUpgrades() const;
	VariationInfo *GetDefaultVariation(CPlayer &player, int image_layer = -1) const;
	std::string GetDefaultName(CPlayer &player) const;
	CPlayerColorGraphic *GetDefaultLayerSprite(CPlayer &player, int image_layer) const;
	//Wyrmgus end

public:
	std::string Ident;              /// Identifier
	std::string Name;               /// Pretty name shown from the engine
	//Wyrmgus start
	std::string Parent;				/// Parent unit type
	std::string Class;				/// Class identifier (i.e. infantry, archer, etc.)
	std::string Civilization;		/// Which civilization this unit belongs to, if any
	std::string Faction;			/// Which faction this unit belongs to, if any
	std::string Description;		/// Description of the unit type
	std::string Quote;				/// Quote of the unit type
	std::string Background;			/// Encyclopedia entry for the unit type
	std::string ChildUpgrade;		/// Which individual upgrade the children of this species get
	std::string Excrement;			/// Excrement unit for this unit type
	std::string PersonalNames[PersonalNameMax];	/// personal names
	std::string PersonalNamePrefixes[PersonalNameMax];	/// personal name prefixes
	std::string PersonalNameSuffixes[PersonalNameMax];	/// personal name suffixes
	std::vector<int> Drops;			/// Units which can spawn upon death (i.e. items).
	std::vector<int> AiDrops;		/// Units which can spawn upon death (i.e. items), only for AI-controlled units.
	std::vector<CUpgrade *> DropAffixes;	/// Affixes which can be applied to dropped items
	std::vector<SpellType *> DropSpells;	/// Spells which can be applied to dropped items
	std::vector<CUpgrade *> Affixes;	/// Affixes which can be generated for this unit type
	std::vector<CUpgrade *> Traits;	/// Which traits this unit type can have
	//Wyrmgus end
	int Slot;                       /// Type as number
	std::string File;               /// Sprite files
	std::string ShadowFile;         /// Shadow file
	//Wyrmgus start
	std::string LightFile;			/// Light file
	std::string LayerFiles[MaxImageLayers];	/// Layer files
	//Wyrmgus end

	int Width;                                            /// Sprite width
	int Height;                                           /// Sprite height
	int OffsetX;                                          /// Sprite horizontal offset
	int OffsetY;                                          /// Sprite vertical offset
	int DrawLevel;                                        /// Level to Draw UnitType at
	int ShadowWidth;                                      /// Shadow sprite width
	int ShadowHeight;                                     /// Shadow sprite height
	int ShadowOffsetX;                                    /// Shadow horizontal offset
	int ShadowOffsetY;                                    /// Shadow vertical offset
	//Wyrmgus start
	int TechnologyPointCost;								/// Technology point cost
	int TrainQuantity;										/// Quantity to be trained
	int Upkeep;												/// Gold upkeep (for grand strategy mode)
	int ItemClass;											/// Item class (if the unit type is an item)
	std::vector<int> WeaponClasses;							/// Weapon classes that the unit type can use (if the unit type uses a weapon)
	bool InvertedEastArms;									/// Whether the arms are inverted for the east/west graphics
	bool InvertedSoutheastArms;								/// Whether the arms are inverted for the southeast/southwest graphics
	//Wyrmgus end
	PixelPos MissileOffsets[UnitSides][MaxAttackPos];     /// Attack offsets for missiles

	CAnimations *Animations;        /// Animation scripts
	int StillFrame;                 /// Still frame

	IconConfig Icon;                /// Icon to display for this unit
#ifdef USE_MNG
	struct _portrait_ {
		std::string *Files;
		int Num;
		Mng **Mngs;
		mutable int CurrMng;
		mutable int NumIterations;
	} Portrait;
#endif
	MissileConfig Missile;                           /// Missile weapon
	//Wyrmgus start
	MissileConfig FireMissile;						 /// Missile weapon if the unit has fire damage
	//Wyrmgus end
	MissileConfig Explosion;                         /// Missile for unit explosion
	MissileConfig Impact[ANIMATIONS_DEATHTYPES + 2]; /// Missiles spawned if unit is hit(+shield)

	LuaCallback *DeathExplosion;
	LuaCallback *OnHit;             /// lua function called when unit is hit
	LuaCallback *OnEachCycle;       /// lua function called every cycle
	LuaCallback *OnEachSecond;      /// lua function called every second
	LuaCallback *OnInit;            /// lua function called on unit init

	int TeleportCost;               /// mana used for teleportation
	LuaCallback *TeleportEffectIn;   /// lua function to create effects before teleportation
	LuaCallback *TeleportEffectOut;  /// lua function to create effects after teleportation

	mutable std::string DamageType; /// DamageType (used for extra death animations and impacts)

	std::string CorpseName;         /// Corpse type name
	CUnitType *CorpseType;          /// Corpse unit-type

	CConstruction *Construction;    /// What is shown in construction phase

	int RepairHP;                   /// Amount of HP per repair
	int RepairCosts[MaxCosts];      /// How much it costs to repair

	int TileWidth;                  /// Tile size on map width
	int TileHeight;                 /// Tile size on map height
	int BoxWidth;                   /// Selected box size width
	int BoxHeight;                  /// Selected box size height
	int BoxOffsetX;                 /// Selected box size horizontal offset
	int BoxOffsetY;                 /// Selected box size vertical offset
	int NumDirections;              /// Number of directions unit can face
	int MinAttackRange;             /// Minimal attack range
	//Wyrmgus start
	/*
	int ReactRangeComputer;         /// Reacts on enemy for computer
	int ReactRangePerson;           /// Reacts on enemy for person player
	*/
	//Wyrmgus end
	int BurnPercent;                /// Burning percent.
	int BurnDamageRate;             /// HP burn rate per sec
	int RepairRange;                /// Units repair range.
#define InfiniteRepairRange INT_MAX
	char *CanCastSpell;             /// Unit is able to use spells.
	char *AutoCastActive;           /// Default value for autocast.
	int AutoBuildRate;              /// The rate at which the building builds itself
	int RandomMovementProbability;  /// Probability to move randomly.
	int RandomMovementDistance;  	/// Quantity of tiles to move randomly.
	int ClicksToExplode;            /// Number of consecutive clicks until unit suicides.
	int MaxOnBoard;                 /// Number of Transporter slots.
	int BoardSize;                  /// How much "cells" unit occupies inside transporter
	int ButtonLevelForTransporter;  /// On which button level game will show units inside transporter
	//Wyrmgus start
	int ButtonLevelForInventory;	/// On which button level the game will show items inside inventory
	//Wyrmgus end
	int StartingResources;          /// Amount of Resources on build
	/// originally only visual effect, we do more with this!
	UnitTypeType UnitType;          /// Land / fly / naval
	int DecayRate;                  /// Decay rate in 1/6 seconds
	// TODO: not used
	int AnnoyComputerFactor;        /// How much this annoys the computer
	int AiAdjacentRange;            /// Min radius for AI build surroundings checking
	int MouseAction;                /// Right click action
#define MouseActionNone      0      /// Nothing
#define MouseActionAttack    1      /// Attack
#define MouseActionMove      2      /// Move
#define MouseActionHarvest   3      /// Harvest resources
#define MouseActionSpellCast 5      /// Cast the first spell known
#define MouseActionSail      6      /// Sail
//Wyrmgus start
#define MouseActionRallyPoint 7		/// Rally point
//Wyrmgus end
	int CanTarget;                  /// Which units can it attack
#define CanTargetLand 1             /// Can attack land units
#define CanTargetSea  2             /// Can attack sea units
#define CanTargetAir  4             /// Can attack air units

	unsigned Flip : 1;              /// Flip image when facing left
	unsigned LandUnit : 1;          /// Land animated
	unsigned AirUnit : 1;           /// Air animated
	unsigned SeaUnit : 1;           /// Sea animated
	unsigned ExplodeWhenKilled : 1; /// Death explosion animated
	unsigned Building : 1;          /// Building
	unsigned CanAttack : 1;         /// Unit can attack.
	unsigned Neutral : 1;           /// Unit is neutral, used by the editor

	CUnitStats DefaultStat;
	CUnitStats MapDefaultStat;
	struct BoolFlags {
		bool value;             /// User defined flag. Used for (dis)allow target.
		char CanTransport;      /// Can transport units with this flag.
		char CanTargetFlag;     /// Flag needed to target with missile.
		char AiPriorityTarget;  /// Attack this units first.
	};
	std::vector<BoolFlags> BoolFlag;

	int CanStore[MaxCosts];             /// Resources that we can store here.
	int GivesResource;                  /// The resource this unit gives.
	//Wyrmgus start
	int GrandStrategyProductionEfficiencyModifier[MaxCosts];	/// production modifier for a particular resource for grand strategy mode (used for buildings)
	//Wyrmgus end
	ResourceInfo *ResInfo[MaxCosts];    /// Resource information.
	//Wyrmgus start
	VariationInfo *VarInfo[VariationMax];						/// Variation information.
	std::vector<VariationInfo *> LayerVarInfo[MaxImageLayers];	/// Layer variation information.
	//Wyrmgus end
	std::vector<CBuildRestriction *> BuildingRules;   /// Rules list for building a building.
	std::vector<CBuildRestriction *> AiBuildingRules; /// Rules list for for AI to build a building.
	CColor NeutralMinimapColorRGB;   /// Minimap Color for Neutral Units.

	CUnitSound Sound;				/// Sounds for events
	CUnitSound MapSound;			/// Sounds for events, map-specific

	int PoisonDrain;                /// How much health is drained every second when poisoned

	// --- FILLED UP ---

	//Wyrmgus start
//	unsigned FieldFlags;            /// Unit map field flags
//	unsigned MovementMask;          /// Unit check this map flags for move
	unsigned long FieldFlags;            /// Unit map field flags
	unsigned long MovementMask;          /// Unit check this map flags for move
	//Wyrmgus end

	/// @todo This stats should? be moved into the player struct
	CUnitStats Stats[PlayerMax];     /// Unit status for each player

	CPlayerColorGraphic *Sprite;     /// Sprite images
	CGraphic *ShadowSprite;          /// Shadow sprite image
	//Wyrmgus start
	CGraphic *LightSprite;						/// Light sprite image
	CPlayerColorGraphic *LayerSprites[MaxImageLayers];	/// Layer sprite images
	//Wyrmgus end
};

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern std::vector<CUnitType *> UnitTypes;   /// All unit-types

/// @todo this hardcoded unit-types must be removed!!
extern CUnitType *UnitTypeHumanWall;  /// Human wall
extern CUnitType *UnitTypeOrcWall;    /// Orc wall

/**
**  Variable info for unit and unittype.
*/
class CUnitTypeVar
{
public:

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
			return NULL;
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
extern std::string UnitTypeClasses[UnitTypeClassMax]; //list of unit type classes; built with CclDefineUnitType
extern std::string UpgradeClasses[UnitTypeClassMax]; //list of upgrade classes; built with CclDefineModifier
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/
extern CUnitType *CclGetUnitType(lua_State *l);  /// Access unit-type object
extern void UnitTypeCclRegister();               /// Register ccl features

extern void UpdateUnitStats(CUnitType &type, int reset_to_default);       /// Update unit stats
extern void UpdateStats(int reset_to_default);       /// Update unit stats
extern CUnitType *UnitTypeByIdent(const std::string &ident);/// Get unit-type by ident
//Wyrmgus start
extern int GetUnitTypeClassIndexByName(const std::string &class_name);
extern void SetUnitTypeClassStringToIndex(std::string class_name, int class_id);
extern int GetUpgradeClassIndexByName(const std::string &class_name);
extern void SetUpgradeClassStringToIndex(std::string class_name, int class_id);
//Wyrmgus end

extern void SaveUnitTypes(CFile &file);              /// Save the unit-type table
extern CUnitType *NewUnitTypeSlot(const std::string &ident);/// Allocate an empty unit-type slot
/// Draw the sprite frame of unit-type
extern void DrawUnitType(const CUnitType &type, CPlayerColorGraphic *sprite,
						 int player, int frame, const PixelPos &screenPos);

extern void InitUnitTypes(int reset_player_stats);   /// Init unit-type table
extern void LoadUnitTypeSprite(CUnitType &unittype); /// Load the sprite for a unittype
extern int GetUnitTypesCount();                     /// Get the amount of unit-types
extern void LoadUnitTypes();                     /// Load the unit-type data
extern void CleanUnitTypes();                    /// Cleanup unit-type module

// in script_unittype.c

/// Parse User Variables field.
extern void DefineVariableField(lua_State *l, CVariable *var, int lua_index);

/// Update custom Variables with other variable (like Hp, ...)
extern void UpdateUnitVariables(CUnit &unit);

extern void SetMapStat(std::string ident, std::string variable_key, int value, std::string variable_type);
extern void SetMapSound(std::string ident, std::string sound, std::string sound_type, std::string sound_subtype = "");

//Wyrmgus start
extern std::string GetImageLayerNameById(int image_layer);
extern int GetImageLayerIdByName(std::string image_layer);
//Wyrmgus end

//@}

#endif // !__UNITTYPE_H__
