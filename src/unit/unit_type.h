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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "animation.h" //for the ANIMATIONS_DEATHTYPES constant
#include "color.h"
#include "database/detailed_data_entry.h"
#include "database/data_type.h"
#include "data_type.h"
#include "economy/resource_container.h"
#include "missileconfig.h"
#include "ui/icon.h"
#include "upgrade/upgrade_structs.h"
#include "util/color_container.h"
#include "vec2i.h"

#ifdef __MORPHOS__
#undef Enable
#endif

class CFile;
class CPlayer;
class CPlayerColorGraphic;
class LuaCallback;
enum class ButtonCmd;
enum class UnitTypeType;
struct lua_State;

static int CclDefineDependency(lua_State *l);
static int CclDefinePredependency(lua_State *l);
static int CclDefineUnitType(lua_State *l);

namespace wyrmgus {
	class animation_set;
	class button_level;
	class civilization;
	class civilization_group;
	class condition;
	class construction;
	class faction;
	class font;
	class icon;
	class missile_type;
	class name_generator;
	class player_color;
	class renderer;
	class resource;
	class species;
	class spell;
	class terrain_type;
	class time_of_day;
	class unit_class;
	class unit_sound_set;
	class unit_type;
	class unit_type_variation;
	class world;
	enum class gender;
	enum class item_class;
	enum class item_slot;
	enum class tile_flag : uint32_t;
}

constexpr int UnitSides = 8;
constexpr int MaxAttackPos = 5;

enum class GroupSelectionMode {
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

namespace wyrmgus {

class resource_info final
{
public:
	explicit resource_info(const wyrmgus::unit_type *unit_type, const wyrmgus::resource *resource)
		: unit_type(unit_type), resource(resource)
	{
	}

	std::unique_ptr<resource_info> duplicate(const wyrmgus::unit_type *unit_type) const
	{
		auto duplicate = std::make_unique<resource_info>(unit_type, this->get_resource());

		duplicate->ResourceStep = this->ResourceStep;
		duplicate->WaitAtResource = this->WaitAtResource;
		duplicate->WaitAtDepot = this->WaitAtDepot;
		duplicate->ResourceCapacity = this->ResourceCapacity;
		duplicate->LoseResources = this->LoseResources;
		duplicate->image_file = this->image_file;
		duplicate->loaded_image_file = this->loaded_image_file;

		return duplicate;
	}

	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);

	const wyrmgus::unit_type *get_unit_type() const
	{
		return this->unit_type;
	}

	const wyrmgus::resource *get_resource() const
	{
		return this->resource;
	}

	const std::filesystem::path &get_image_file() const
	{
		return this->image_file;
	}

	const std::filesystem::path &get_loaded_image_file() const
	{
		return this->loaded_image_file;
	}

private:
	const wyrmgus::unit_type *unit_type = nullptr; //the unit type to which the resource info belongs
	const wyrmgus::resource *resource = nullptr;        /// the resource harvested. Redundant.
	std::filesystem::path image_file;      /// Change the graphic when the unit is empty.
	std::filesystem::path loaded_image_file;     /// Change the graphic when the unit is loaded.
public:
	unsigned WaitAtResource = 0;    /// Cycles the unit waits while mining.
	unsigned ResourceStep = 0;      /// Resources the unit gains per mining cycle.
	int      ResourceCapacity = 0;  /// Max amount of resources to carry.
	unsigned WaitAtDepot = 0;       /// Cycles the unit waits while returning.
	unsigned char LoseResources = 0; /// The unit will lose it's resource when distracted.
	//  Runtime info:
	std::shared_ptr<CPlayerColorGraphic> SpriteWhenLoaded; /// The graphic corresponding to FileWhenLoaded.
	std::shared_ptr<CPlayerColorGraphic> SpriteWhenEmpty;  /// The graphic corresponding to FileWhenEmpty

	friend int ::CclDefineUnitType(lua_State *l);
};

}

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
	UNDEAD_INDEX,
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
	NEUTRAL_HOSTILE_INDEX,
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
	ETHEREAL_INDEX,					/// is only visible by units with ethereal vision.
	CELESTIAL_BODY_INDEX,
	HIDDENOWNERSHIP_INDEX,
	HIDDENINEDITOR_INDEX,
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
	SHADOW_DAMAGE_INDEX,
	SPEED_INDEX,
	FIRERESISTANCE_INDEX,
	COLDRESISTANCE_INDEX,
	ARCANERESISTANCE_INDEX,
	LIGHTNINGRESISTANCE_INDEX,
	AIRRESISTANCE_INDEX,
	EARTHRESISTANCE_INDEX,
	WATERRESISTANCE_INDEX,
	ACIDRESISTANCE_INDEX,
	SHADOW_RESISTANCE_INDEX,
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
	MANA_RESTORATION_INDEX,
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
	SPEEDBONUS_INDEX, //dummy variable for terrain types that increase movement speed, so that their bonus shows up correctly in the interface
	RAIL_SPEED_BONUS_INDEX, //speed bonus when in a rail tile
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
	//Wyrmgus end
	NVARALREADYDEFINED
};

class CUnit;

/**
**  Decoration for user defined variable.
**
**  It is used to show variables graphicly.
**  @todo add more stuff in this struct.
*/
class CDecoVar
{
public:
	CDecoVar() {}

	virtual ~CDecoVar()
	{
	}

	/// function to draw the decorations.
	virtual void Draw(int x, int y, const unit_type &type, const unit_variable &var, std::vector<std::function<void(renderer *)>> &render_commands) const = 0;

	unsigned int Index;     /// Index of the variable. @see DefineVariables

	//Wyrmgus start
	int MinValue;			/// Minimum value of the variable
	//Wyrmgus end

	int OffsetX;            /// Offset in X coord.
	int OffsetY;            /// Offset in Y coord.

	int OffsetXPercent;     /// Percent offset (tile_size.width()) in X coord.
	int OffsetYPercent;     /// Percent offset (tile_size.height()) in Y coord.

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

class CDecoVarBar final : public CDecoVar
{
public:
	/// function to draw the decorations.
	virtual void Draw(int x, int y, const unit_type &type, const unit_variable &var, std::vector<std::function<void(renderer *)>> &render_commands) const override;

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

class CDecoVarText final : public CDecoVar
{
public:
	/// function to draw the decorations.
	virtual void Draw(int x, int y, const unit_type &type, const unit_variable &var, std::vector<std::function<void(renderer *)>> &render_commands) const override;

	wyrmgus::font *Font = nullptr;  /// Font to use to display value.
	// FIXME : Add Color, format
};

/// Sprite contains frame from full (left)to empty state (right).
class CDecoVarSpriteBar final : public CDecoVar
{
public:
	CDecoVarSpriteBar() : NSprite(-1) {};
	/// function to draw the decorations.
	virtual void Draw(int x, int y, const unit_type &type, const unit_variable &var, std::vector<std::function<void(renderer *)>> &render_commands) const override;

	char NSprite; /// Index of number. (@see DefineSprites and @see GetSpriteIndex)
	// FIXME Sprite info. better way ?
};

/// use to show specific frame in a sprite.
class CDecoVarStaticSprite final : public CDecoVar
{
public:
	virtual void Draw(int x, int y, const unit_type &type, const unit_variable &var, std::vector<std::function<void(renderer *)>> &render_commands) const override;

	// FIXME Sprite info. and Replace n with more appropriate var.
	char NSprite = -1;  /// Index of sprite. (@see DefineSprites and @see GetSpriteIndex)
	int n = 0;         /// identifiant in SpellSprite
	int FadeValue = 0; /// if variable's value is below than FadeValue, it drawn transparent.
};

enum class DistanceTypeType {
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

	virtual std::unique_ptr<CBuildRestriction> duplicate() const = 0;

	virtual void Init() {};
	virtual bool Check(const CUnit *builder, const wyrmgus::unit_type &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const = 0;
};

class CBuildRestrictionAnd final : public CBuildRestriction
{
public:
	virtual std::unique_ptr<CBuildRestriction> duplicate() const override
	{
		auto b = std::make_unique<CBuildRestrictionAnd>();
		for (const auto &build_restriction : this->and_list) {
			b->and_list.push_back(build_restriction->duplicate());
		}
		return b;
	}

	virtual void Init() override
	{
		for (const auto &build_restriction : this->and_list) {
			build_restriction->Init();
		}
	}

	virtual bool Check(const CUnit *builder, const wyrmgus::unit_type &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const override;

public:
	std::vector<std::unique_ptr<CBuildRestriction>> and_list;
};

class CBuildRestrictionOr final : public CBuildRestriction
{
public:
	virtual std::unique_ptr<CBuildRestriction> duplicate() const override
	{
		auto b = std::make_unique<CBuildRestrictionOr>();
		for (const auto &build_restriction : this->or_list) {
			b->or_list.push_back(build_restriction->duplicate());
		}
		return b;
	}

	virtual void Init() override
	{
		for (const auto &build_restriction : this->or_list) {
			build_restriction->Init();
		}
	}

	virtual bool Check(const CUnit *builder, const wyrmgus::unit_type &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const override;

public:
	std::vector<std::unique_ptr<CBuildRestriction>> or_list;
};

class CBuildRestrictionAddOn final : public CBuildRestriction
{
	class functor
	{
	public:
		functor(const wyrmgus::unit_type *type, const Vec2i &_pos): Parent(type), pos(_pos) {}
		inline bool operator()(const CUnit *const unit) const;
	private:
		const wyrmgus::unit_type *const Parent;   /// building that is unit is an addon too.
		const Vec2i pos; //functor work position
	};
public:
	CBuildRestrictionAddOn() : Offset(0, 0) {}

	virtual std::unique_ptr<CBuildRestriction> duplicate() const override
	{
		auto b = std::make_unique<CBuildRestrictionAddOn>();
		b->Offset = this->Offset;
		b->ParentName = this->ParentName;
		b->Parent = this->Parent;
		return b;
	}

	virtual void Init() override;
	virtual bool Check(const CUnit *builder, const wyrmgus::unit_type &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const override;

	Vec2i Offset;           /// offset from the main building to place this
	std::string ParentName; /// building that is unit is an addon too.
	wyrmgus::unit_type *Parent = nullptr;      /// building that is unit is an addon too.
};

class CBuildRestrictionOnTop final : public CBuildRestriction
{
	class functor
	{
	public:
		functor(const wyrmgus::unit_type *type, const Vec2i &_pos): ontop(0), Parent(type), pos(_pos) {}
		inline bool operator()(CUnit *const unit);
		CUnit *ontop;   /// building that is unit is an addon too.
	private:
		const wyrmgus::unit_type *const Parent;  /// building that is unit is an addon too.
		const Vec2i pos;  //functor work position
	};
public:
	virtual std::unique_ptr<CBuildRestriction> duplicate() const override
	{
		auto b = std::make_unique<CBuildRestrictionOnTop>();
		b->ParentName = this->ParentName;
		b->Parent = this->Parent;
		b->ReplaceOnDie = this->ReplaceOnDie;
		b->ReplaceOnBuild = this->ReplaceOnBuild;
		return b;
	}

	virtual void Init() override;
	virtual bool Check(const CUnit *builder, const wyrmgus::unit_type &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const override;

	std::string ParentName;  /// building that is unit is an addon too.
	wyrmgus::unit_type *Parent = nullptr;       /// building that is unit is an addon too.
	int ReplaceOnDie = 0;     /// recreate the parent on destruction
	int ReplaceOnBuild = 0;   /// remove the parent, or just build over it.
};

class CBuildRestrictionDistance final : public CBuildRestriction
{
public:
	virtual std::unique_ptr<CBuildRestriction> duplicate() const override
	{
		auto b = std::make_unique<CBuildRestrictionDistance>();
		b->Distance = this->Distance;
		b->DistanceType = this->DistanceType;
		b->RestrictTypeName = this->RestrictTypeName;
		b->RestrictTypeOwner = this->RestrictTypeOwner;
		b->RestrictType = this->RestrictType;
		b->restrict_class_name = this->restrict_class_name;
		b->restrict_class = this->restrict_class;
		b->CheckBuilder = this->CheckBuilder;
		b->Diagonal = this->Diagonal;
		return b;
	}

	virtual void Init() override;
	virtual bool Check(const CUnit *builder, const wyrmgus::unit_type &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const override;

	int Distance = 0;        /// distance to build (circle)
	DistanceTypeType DistanceType;
	std::string RestrictTypeName;
	std::string RestrictTypeOwner;
	wyrmgus::unit_type *RestrictType = nullptr;
	std::string restrict_class_name;
	const wyrmgus::unit_class *restrict_class = nullptr;
	bool CheckBuilder = false;
	bool Diagonal = true;
};

class CBuildRestrictionHasUnit final : public CBuildRestriction
{
public:
	virtual std::unique_ptr<CBuildRestriction> duplicate() const override
	{
		auto b = std::make_unique<CBuildRestrictionHasUnit>();
		b->Count = this->Count;
		b->CountType = this->CountType;
		b->RestrictTypeName = this->RestrictTypeName;
		b->RestrictType = this->RestrictType;
		b->RestrictTypeOwner = this->RestrictTypeOwner;
		return b;
	}

	virtual void Init() override;
	virtual bool Check(const CUnit *builder, const wyrmgus::unit_type &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const override;
	
	int Count = 0;
	DistanceTypeType CountType;
	std::string RestrictTypeName;
	wyrmgus::unit_type *RestrictType = nullptr;
	std::string RestrictTypeOwner;
};

class CBuildRestrictionSurroundedBy final : public CBuildRestriction
{
public:
	virtual std::unique_ptr<CBuildRestriction> duplicate() const override
	{
		auto b = std::make_unique<CBuildRestrictionSurroundedBy>();
		b->Distance = this->Distance;
		b->DistanceType = this->DistanceType;
		b->Count = this->Count;
		b->CountType = this->CountType;
		b->RestrictTypeName = this->RestrictTypeName;
		b->RestrictTypeOwner = this->RestrictTypeOwner;
		b->RestrictType = this->RestrictType;
		b->CheckBuilder = this->CheckBuilder;
		return b;
	}

	virtual void Init() override;
	virtual bool Check(const CUnit *builder, const wyrmgus::unit_type &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const override;
	
	int Distance = 0;
	DistanceTypeType DistanceType = DistanceTypeType::Equal;
	int Count = 0;
	DistanceTypeType CountType = DistanceTypeType::Equal;
	std::string RestrictTypeName;
	std::string RestrictTypeOwner;
	wyrmgus::unit_type *RestrictType = nullptr;
	bool CheckBuilder = false;
};

class CBuildRestrictionTerrain final : public CBuildRestriction
{
public:
	virtual std::unique_ptr<CBuildRestriction> duplicate() const override
	{
		auto b = std::make_unique<CBuildRestrictionTerrain>();
		b->RestrictTerrainTypeName = this->RestrictTerrainTypeName;
		b->RestrictTerrainType = this->RestrictTerrainType;
		return b;
	}

	virtual void Init() override;
	virtual bool Check(const CUnit *builder, const wyrmgus::unit_type &type, const Vec2i &pos, CUnit *&ontoptarget, int z) const override;

	std::string RestrictTerrainTypeName;
	wyrmgus::terrain_type *RestrictTerrainType = nullptr;
};

namespace wyrmgus {

/// Base structure of unit-type
/// @todo n0body: AutoBuildRate not implemented.
class unit_type final : public detailed_data_entry, public data_type<unit_type>, public CDataType
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::unit_class* unit_class MEMBER unit_class WRITE set_unit_class NOTIFY changed)
	Q_PROPERTY(bool template MEMBER template_type READ is_template)
	Q_PROPERTY(wyrmgus::civilization* civilization MEMBER civilization NOTIFY changed)
	Q_PROPERTY(wyrmgus::civilization_group* civilization_group MEMBER civilization_group NOTIFY changed)
	Q_PROPERTY(wyrmgus::faction* faction MEMBER faction NOTIFY changed)
	Q_PROPERTY(wyrmgus::animation_set* animation_set MEMBER animation_set)
	Q_PROPERTY(wyrmgus::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(QSize tile_size MEMBER tile_size READ get_tile_size)
	Q_PROPERTY(QSize box_size MEMBER box_size READ get_box_size)
	Q_PROPERTY(QString image_file READ get_image_file_qstring)
	Q_PROPERTY(QSize frame_size MEMBER frame_size READ get_frame_size)
	Q_PROPERTY(QPoint offset MEMBER offset READ get_offset)
	Q_PROPERTY(int num_directions MEMBER num_directions READ get_num_directions)
	Q_PROPERTY(wyrmgus::player_color* conversible_player_color MEMBER conversible_player_color READ get_conversible_player_color)
	Q_PROPERTY(double hue_rotation MEMBER hue_rotation READ get_hue_rotation)
	Q_PROPERTY(int draw_level MEMBER draw_level READ get_draw_level)
	Q_PROPERTY(wyrmgus::item_class item_class MEMBER item_class READ get_item_class)
	Q_PROPERTY(wyrmgus::species* species MEMBER species)
	Q_PROPERTY(CUpgrade* elixir MEMBER elixir)
	Q_PROPERTY(wyrmgus::unit_type* corpse_type MEMBER corpse_type READ get_corpse_type)
	Q_PROPERTY(wyrmgus::construction* construction MEMBER construction READ get_construction)
	Q_PROPERTY(wyrmgus::resource* given_resource MEMBER given_resource)
	Q_PROPERTY(int random_movement_probability MEMBER random_movement_probability READ get_random_movement_probability)
	Q_PROPERTY(quint64 default_mass MEMBER default_mass READ get_default_mass)
	Q_PROPERTY(QColor neutral_minimap_color MEMBER neutral_minimap_color READ get_neutral_minimap_color)
	Q_PROPERTY(QString encyclopedia_background_file READ get_encyclopedia_background_file_qstring NOTIFY changed)

public:
	static constexpr const char *class_identifier = "unit_type";
	static constexpr const char *database_folder = "unit_types";

	static unit_type *add(const std::string &identifier, const wyrmgus::data_module *data_module)
	{
		unit_type *unit_type = data_type::add(identifier, data_module);
		unit_type->Slot = unit_type::get_all().size() - 1;
		return unit_type;
	}

	static bool compare_building_encyclopedia_entries(const unit_type *lhs, const unit_type *rhs);
	static bool compare_item_encyclopedia_entries(const unit_type *lhs, const unit_type *rhs);
	static bool compare_unit_encyclopedia_entries(const unit_type *lhs, const unit_type *rhs);
	static bool compare_encyclopedia_entries(const unit_type *lhs, const unit_type *rhs);

	static std::vector<unit_type *> get_unit_encyclopedia_entries()
	{
		std::vector<unit_type *> entries;

		for (unit_type *unit_type : unit_type::get_encyclopedia_entries()) {
			if (unit_type->BoolFlag[BUILDING_INDEX].value || unit_type->BoolFlag[ITEM_INDEX].value) {
				continue;
			}

			entries.push_back(unit_type);
		}

		unit_type::sort_encyclopedia_entries(entries);

		return entries;
	}

	static std::vector<unit_type *> get_building_encyclopedia_entries()
	{
		std::vector<unit_type *> entries;

		for (unit_type *unit_type : unit_type::get_encyclopedia_entries()) {
			if (!unit_type->BoolFlag[BUILDING_INDEX].value) {
				continue;
			}

			entries.push_back(unit_type);
		}

		unit_type::sort_encyclopedia_entries(entries);

		return entries;
	}

	static std::vector<unit_type *> get_item_encyclopedia_entries()
	{
		std::vector<unit_type *> entries;

		for (unit_type *unit_type : unit_type::get_encyclopedia_entries()) {
			if (!unit_type->BoolFlag[ITEM_INDEX].value) {
				continue;
			}

			entries.push_back(unit_type);
		}

		unit_type::sort_encyclopedia_entries(entries);

		return entries;
	}

	explicit unit_type(const std::string &identifier);
	~unit_type();

	virtual void process_sml_property(const sml_property &property) override;
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void initialize() override;
	virtual void check() const override;

	virtual bool has_encyclopedia_entry() const override;
	virtual std::string get_encyclopedia_text() const override;

	int get_index() const
	{
		return this->Slot;
	}
	
	const wyrmgus::unit_class *get_unit_class() const
	{
		return this->unit_class;
	}

	void set_unit_class(wyrmgus::unit_class *unit_class);

	bool is_template() const
	{
		return this->template_type;
	}

	const wyrmgus::civilization_group *get_civilization_group() const
	{
		return this->civilization_group;
	}

	const wyrmgus::civilization *get_civilization() const
	{
		return this->civilization;
	}

	const wyrmgus::civilization *get_faction_civilization(const wyrmgus::faction *faction) const;
	const wyrmgus::civilization *get_player_civilization(const CPlayer *player) const;

	const wyrmgus::faction *get_faction() const
	{
		return this->faction;
	}

	const std::vector<const unit_type *> &get_subtypes() const
	{
		return this->subtypes;
	}

	const QSize &get_tile_size() const
	{
		return this->tile_size;
	}

	int get_tile_width() const
	{
		return this->get_tile_size().width();
	}

	int get_tile_height() const
	{
		return this->get_tile_size().height();
	}

	QSize get_half_tile_size() const;

	PixelSize get_half_tile_pixel_size() const
	{
		return this->get_tile_pixel_size() / 2;
	}

	PixelSize get_tile_pixel_size() const;
	PixelSize get_scaled_tile_pixel_size() const;

	PixelSize get_scaled_half_tile_pixel_size() const
	{
		return this->get_scaled_tile_pixel_size() / 2;
	}

	QPoint get_tile_center_pos_offset() const;

	const QSize &get_box_size() const
	{
		return this->box_size;
	}

	int get_box_width() const
	{
		return this->get_box_size().width();
	}

	int get_box_height() const
	{
		return this->get_box_size().height();
	}

	const std::filesystem::path &get_image_file() const
	{
		return this->image_file;
	}

	void set_image_file(const std::filesystem::path &filepath);

	QString get_image_file_qstring() const
	{
		return QString::fromStdString(this->get_image_file().string());
	}

	Q_INVOKABLE void set_image_file(const std::string &filepath)
	{
		this->set_image_file(std::filesystem::path(filepath));
	}

	const QSize &get_frame_size() const
	{
		return this->frame_size;
	}

	int get_frame_width() const
	{
		return this->get_frame_size().width();
	}

	int get_frame_height() const
	{
		return this->get_frame_size().height();
	}

	const QPoint &get_offset() const
	{
		return this->offset;
	}

	int get_num_directions() const
	{
		return this->num_directions;
	}

	player_color *get_conversible_player_color() const
	{
		return this->conversible_player_color;
	}

	double get_hue_rotation() const
	{
		return this->hue_rotation;
	}

	const color_set &get_hue_ignored_colors() const
	{
		return this->hue_ignored_colors;
	}

	const std::vector<const spell *> &get_autocast_spells() const
	{
		return this->autocast_spells;
	}

	bool is_autocast_spell(const spell *spell) const;
	void add_autocast_spell(const spell *spell);

	bool CheckUserBoolFlags(const char *BoolFlags) const;
	//Wyrmgus start
//	bool CanTransport() const { return MaxOnBoard > 0 && !GivesResource; }
	bool CanTransport() const { return MaxOnBoard > 0; }
	//Wyrmgus end
	bool CanMove() const;

	bool CanSelect(GroupSelectionMode mode = GroupSelectionMode::SELECTABLE_BY_RECTANGLE_ONLY) const;
	
	void set_parent(const unit_type *parent_type);
	void RemoveButtons(const ButtonCmd button_action, const std::string &mod_file = "");

	void UpdateDefaultBoolFlags();
	void calculate_movement_mask();

	int GetAvailableLevelUpUpgrades() const;
	int get_resource_step(const resource *resource, const int player) const;
	const unit_type_variation *GetDefaultVariation(const CPlayer *player, const int image_layer = -1) const;
	unit_type_variation *GetVariation(const std::string &variation_name, int image_layer = -1) const;
	std::string GetRandomVariationIdent(int image_layer = -1) const;
	const std::string &GetDefaultName(const CPlayer *player) const;
	const std::shared_ptr<CPlayerColorGraphic> &GetDefaultLayerSprite(const CPlayer *player, const int image_layer) const;
	const std::string &get_default_button_key(const CPlayer *player) const;
	bool CanExperienceUpgradeTo(const unit_type *type) const;
	std::string GetNamePlural() const;
	std::string generate_personal_name(const wyrmgus::faction *faction, const gender gender) const;
	bool is_personal_name_valid(const std::string &name, const faction *faction, const gender gender) const;
	const name_generator *get_name_generator(const wyrmgus::faction *faction, const gender gender) const;

	wyrmgus::item_class get_item_class() const
	{
		return this->item_class;
	}

	wyrmgus::species *get_species() const
	{
		return this->species;
	}

	const wyrmgus::animation_set *get_animation_set() const
	{
		return this->animation_set;
	}

	wyrmgus::icon *get_icon() const
	{
		return this->icon;
	}

	int get_draw_level() const
	{
		return this->draw_level;
	}

	const CUpgrade *get_elixir() const
	{
		return this->elixir;
	}

	unit_type *get_corpse_type() const
	{
		return this->corpse_type;
	}

	wyrmgus::construction *get_construction() const
	{
		return this->construction;
	}

	const resource_map<int> &get_repair_costs() const
	{
		return this->repair_costs;
	}

	const std::string &get_button_key() const
	{
		return this->button_key;
	}

	const resource_set &get_stored_resources() const
	{
		return this->stored_resources;
	}

	bool can_store(const resource *resource) const
	{
		if (resource == nullptr) {
			throw std::runtime_error("A null resource was provided to unit_type::can_store().");
		}

		return this->stored_resources.contains(resource);
	}

	const resource *get_given_resource() const
	{
		return this->given_resource;
	}

	bool can_produce_a_resource() const;

	const resource_map<std::unique_ptr<resource_info>> &get_resource_infos() const
	{
		return this->resource_infos;
	}

	const resource_info *get_resource_info(const resource *resource) const
	{
		const auto find_iterator = this->resource_infos.find(resource);

		if (find_iterator != this->resource_infos.end()) {
			return find_iterator->second.get();
		}

		return nullptr;
	}

	const std::vector<int> &get_starting_resources() const
	{
		return this->starting_resources;
	}

	int get_random_movement_probability() const
	{
		return this->random_movement_probability;
	}

	uint64_t get_default_mass() const
	{
		return this->default_mass;
	}

	const std::vector<std::unique_ptr<unit_type_variation>> &get_variations() const
	{
		return this->variations;
	}

	const QColor &get_neutral_minimap_color() const
	{
		return this->neutral_minimap_color;
	}

	const std::filesystem::path &get_encyclopedia_background_file() const;

	QString get_encyclopedia_background_file_qstring() const
	{
		return QString::fromStdString(this->get_encyclopedia_background_file().string());
	}

	void set_encyclopedia_background_file(const std::filesystem::path &filepath);

	Q_INVOKABLE void set_encyclopedia_background_file(const std::string &filepath)
	{
		this->set_encyclopedia_background_file(std::filesystem::path(filepath));
	}

	const unit_sound_set *get_sound_set() const
	{
		return this->sound_set.get();
	}

	const std::unique_ptr<condition> &get_preconditions() const
	{
		return this->preconditions;
	}

	const std::unique_ptr<condition> &get_conditions() const
	{
		return this->conditions;
	}

	bool can_gain_experience() const;

	gender get_gender() const;

	std::string get_build_verb_string() const;
	std::string get_destroy_verb_string() const;

signals:
	void changed();

public:
	const unit_type *Parent = nullptr;				/// Parent unit type
	//Wyrmgus start
private:
	wyrmgus::unit_class *unit_class = nullptr; //unit class (e.g. infantry, archer, etc.)
	bool template_type = false;
	wyrmgus::civilization_group *civilization_group = nullptr;
	wyrmgus::civilization *civilization = nullptr; //which civilization this unit belongs to, if any
	wyrmgus::faction *faction = nullptr; //which faction this unit belongs to, if any
	std::vector<const unit_type *> subtypes; //subtypes of this type; when a unit of this type is created, it has a subtype picked automatically instead
public:
	std::string RequirementsString;	/// Requirements string of the unit type
	std::string ExperienceRequirementsString;	/// Experience requirements string of the unit type
	std::string BuildingRulesString;	/// Building rules string of the unit type
private:
	CUpgrade *elixir = nullptr; //which elixir does this (item) unit type always have
public:
	std::vector<unit_type *> SoldUnits;		/// Units which this unit can sell.
	std::vector<unit_type *> SpawnUnits;	/// Units which this unit can spawn.
	std::vector<unit_type *> Drops;			/// Units which can spawn upon death (i.e. items).
	std::vector<unit_type *> AiDrops;		/// Units which can spawn upon death (i.e. items), only for AI-controlled units.
	std::vector<spell *> DropSpells;		/// Spells which can be applied to dropped items
	std::vector<CUpgrade *> Affixes;		/// Affixes which can be generated for this unit type
	std::vector<CUpgrade *> Traits;			/// Which traits this unit type can have
	std::vector<CUpgrade *> StartingAbilities;	/// Abilities which the unit starts out with
	std::vector<unit_type *> Trains;		/// Units trained by this unit
	std::vector<unit_type *> TrainedBy;		/// Units which can train this unit
	std::map<std::filesystem::path, std::vector<unit_type *>> ModTrains; //units trained by this unit (as set in a mod)
	std::map<std::filesystem::path, std::vector<unit_type *>> ModTrainedBy; //units which can train this unit (as set in a mod)
	std::map<std::filesystem::path, std::vector<unit_type *>> ModAiDrops; //units dropped by this unit, if it is AI-controlled (as set in a mod)
	//Wyrmgus end
	int Slot = 0;                    /// Type as number
private:
	std::filesystem::path image_file;
public:
	std::string ShadowFile;         /// Shadow file
	//Wyrmgus start
	std::string LightFile;			/// Light file
	std::string LayerFiles[MaxImageLayers];	/// Layer files
	std::map<ButtonCmd, IconConfig> ButtonIcons;		//icons for button actions
	std::map<item_slot, unit_type *> DefaultEquipment; //default equipment for the unit type, mapped to item slots
	//Wyrmgus end
private:
	QSize frame_size = QSize(0, 0); //sprite frame size
	QPoint offset = QPoint(0, 0); //sprite horizontal offset
	player_color *conversible_player_color = nullptr; //the conversible player color for the unit graphics
	double hue_rotation = 0;
	color_set hue_ignored_colors;
	int draw_level = 0;                                   /// Level to Draw UnitType at
public:
	int ShadowWidth = 0;					/// Shadow sprite width
	int ShadowHeight = 0;					/// Shadow sprite height
	int ShadowOffsetX = 0;					/// Shadow horizontal offset
	int ShadowOffsetY = 0;					/// Shadow vertical offset
	int TrainQuantity = 0;					/// Quantity to be trained
	int CostModifier = 0;					/// Cost modifier (cost increase for every unit of this type the player has)
private:
	wyrmgus::item_class item_class; //item class (if the unit type is an item)
	wyrmgus::species *species = nullptr;
public:
	wyrmgus::terrain_type *TerrainType = nullptr;
	std::vector<wyrmgus::item_class> WeaponClasses; //weapon classes that the unit type can use (if the unit type uses a weapon)
	PixelPos MissileOffsets[UnitSides][MaxAttackPos];     /// Attack offsets for missiles

private:
	wyrmgus::animation_set *animation_set = nullptr;        /// Animation scripts
public:
	int StillFrame = 0;				/// Still frame

private:
	wyrmgus::icon *icon = nullptr;                /// Icon to display for this unit
public:
	MissileConfig Missile;                           /// Missile weapon
	//Wyrmgus start
	MissileConfig FireMissile;						 /// Missile weapon if the unit has fire damage
	//Wyrmgus end
	MissileConfig Explosion;                         /// Missile for unit explosion
	MissileConfig Impact[ANIMATIONS_DEATHTYPES + 2]; /// Missiles spawned if unit is hit(+shield)

	std::unique_ptr<LuaCallback> DeathExplosion;
	std::unique_ptr<LuaCallback> OnHit; //lua function called when unit is hit
	std::unique_ptr<LuaCallback> OnEachCycle; //lua function called every cycle
	std::unique_ptr<LuaCallback> OnEachSecond; //lua function called every second
	std::unique_ptr<LuaCallback> OnInit; //lua function called on unit init

	int TeleportCost = 0;               /// mana used for teleportation
	std::unique_ptr<LuaCallback> TeleportEffectIn;   /// lua function to create effects before teleportation
	std::unique_ptr<LuaCallback> TeleportEffectOut;  /// lua function to create effects after teleportation

	mutable std::string DamageType; /// DamageType (used for extra death animations and impacts)

private:
	unit_type *corpse_type = nullptr; //corpse unit-type

	wyrmgus::construction *construction = nullptr;    /// What is shown in the construction phase

public:
	int RepairHP = 0;				/// Amount of HP per repair
private:
	resource_map<int> repair_costs;      /// How much it costs to repair

private:
	QSize tile_size = QSize(0, 0);
	QSize box_size = QSize(0, 0);
public:
	int BoxOffsetX = 0;				/// Selected box size horizontal offset
	int BoxOffsetY = 0;				/// Selected box size vertical offset
private:
	int num_directions = 0;			/// Number of directions unit can face
public:
	int MinAttackRange = 0;			/// Minimal attack range
	//Wyrmgus start
	/*
	int ReactRangeComputer;         /// Reacts on enemy for computer
	int ReactRangePerson;           /// Reacts on enemy for person player
	*/
	//Wyrmgus end
	int BurnPercent = 0;			/// Burning percent.
	int BurnDamageRate = 0;			/// HP burn rate per sec
	int RepairRange = 0;			/// Units repair range.
#define InfiniteRepairRange INT_MAX
	std::vector<spell *> Spells;	/// Spells the unit is able to cast.
private:
	std::vector<const spell *> autocast_spells; //the list of autocast spells
public:
	int AutoBuildRate = 0;			/// The rate at which the building builds itself
private:
	int random_movement_probability = 0;  /// Probability to move randomly.
public:
	int RandomMovementDistance = 1;  /// Quantity of tiles to move randomly.
	int ClicksToExplode = 0;		/// Number of consecutive clicks until unit suicides.
	int MaxOnBoard = 0;				/// Number of Transporter slots.
	int BoardSize = 1;				/// How much "cells" unit occupies inside transporter
	wyrmgus::button_level *ButtonLevelForTransporter = nullptr; //on which button level game will show units inside transporter
	//Wyrmgus start
	int ButtonPos = 0;				/// Position of this unit as a train/build button
	wyrmgus::button_level *ButtonLevel = nullptr;	/// Level of this unit's button
	std::string ButtonPopup;		/// Popup of this unit's button
	std::string ButtonHint;			/// Hint of this unit's button
private:
	std::string button_key;			/// Hotkey of this unit's button
//	int starting_resources = 0;          /// Amount of Resources on build
	std::vector<int> starting_resources;          /// Amount of Resources on build
public:
	//Wyrmgus end
	/// originally only visual effect, we do more with this!
	UnitTypeType UnitType;          /// Land / fly / naval
	int DecayRate = 0;				/// Decay rate in 1/6 seconds
	// TODO: not used
	int AnnoyComputerFactor = 0;	/// How much this annoys the computer
	int AiAdjacentRange = -1;		/// Min radius for AI build surroundings checking
	int MouseAction = 0;			/// Right click action
#define MouseActionNone      0      /// Nothing
#define MouseActionAttack    1      /// Attack
#define MouseActionMove      2      /// Move
#define MouseActionHarvest   3      /// Harvest resources
#define MouseActionSpellCast 5      /// Cast the first spell known
#define MouseActionSail      6      /// Sail
//Wyrmgus start
#define MouseActionRallyPoint 7		/// Rally point
#define MouseActionTrade      8		/// Trade
//Wyrmgus end
	int CanTarget = 0;                  /// Which units can it attack
#define CanTargetLand 1             /// Can attack land units
#define CanTargetSea  2             /// Can attack sea units
#define CanTargetAir  4             /// Can attack air units

	unsigned Flip : 1;              /// Flip image when facing left
	unsigned LandUnit : 1;          /// Land animated
	unsigned AirUnit : 1;           /// Air animated
	unsigned SeaUnit : 1;           /// Sea animated
	unsigned ExplodeWhenKilled : 1; /// Death explosion animated
	unsigned CanAttack : 1;         /// Unit can attack.
	unsigned Neutral : 1;           /// Unit is neutral, used by the editor

private:
	uint64_t default_mass = 0; //the default mass of the unit type, if it is a celestial body

public:
	CUnitStats DefaultStat;
	CUnitStats MapDefaultStat;
	//Wyrmgus start
	std::map<std::filesystem::path, CUnitStats> ModDefaultStats;
	//Wyrmgus end
	struct BoolFlags {
		bool value;             /// User defined flag. Used for (dis)allow target.
		char CanTransport;      /// Can transport units with this flag.
		char CanTargetFlag;     /// Flag needed to target with missile.
		char AiPriorityTarget;  /// Attack this units first.
	};
	std::vector<BoolFlags> BoolFlag;

private:
	resource_set stored_resources;             /// Resources that we can store here.
	resource *given_resource = nullptr; //the resource this unit gives
	resource_map<std::unique_ptr<resource_info>> resource_infos;    /// Resource information.
	std::vector<std::unique_ptr<unit_type_variation>> variations;
public:
	//Wyrmgus start
	std::vector<std::unique_ptr<unit_type_variation>> LayerVariations[MaxImageLayers];	/// Layer variation information
	//Wyrmgus end
	std::vector<std::unique_ptr<CBuildRestriction>> BuildingRules;   /// Rules list for building a building.
	std::vector<std::unique_ptr<CBuildRestriction>> AiBuildingRules; /// Rules list for for AI to build a building.
private:
	QColor neutral_minimap_color; //minimap color for neutral units
	std::filesystem::path encyclopedia_background_file;

	std::unique_ptr<unit_sound_set> sound_set;			/// Sounds for events
public:
	std::unique_ptr<unit_sound_set> MapSound;			/// Sounds for events, map-specific
	//Wyrmgus start
	std::map<std::filesystem::path, unit_sound_set> ModSounds;
	//Wyrmgus end

	int PoisonDrain = 0;                /// How much health is drained every second when poisoned

	// --- FILLED UP ---

	tile_flag FieldFlags;            /// Unit map field flags
	tile_flag MovementMask;          /// Unit check this map flags for move

	/// @todo This stats should? be moved into the player struct
	CUnitStats Stats[PlayerMax];     /// Unit status for each player

	std::shared_ptr<CPlayerColorGraphic> Sprite;     /// Sprite images
	std::shared_ptr<CGraphic> ShadowSprite;          /// Shadow sprite image
	//Wyrmgus start
	std::shared_ptr<CGraphic> LightSprite;						/// Light sprite image
	std::shared_ptr<CPlayerColorGraphic> LayerSprites[MaxImageLayers];	/// Layer sprite images
	
private:
	std::unique_ptr<condition> preconditions;
	std::unique_ptr<condition> conditions;
	
public:
	std::filesystem::path Mod;							/// To which mod (or map), if any, this unit type belongs
	//Wyrmgus end

	friend int ::CclDefineUnitType(lua_State *l);
	friend int ::CclDefineDependency(lua_State *l);
	friend int ::CclDefinePredependency(lua_State *l);
};

}

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
			int offset = 0;
			unsigned int keylen = 0;
			const char *key = nullptr;
		};

		std::array<DataKey, SIZE> buildin;
		std::map<std::string, int> user;
		unsigned int TotalKeys = SIZE;

		void Init()
		{
			std::sort(buildin.begin(), buildin.end(), DataKey::key_pred);
		}

		const char *operator[](int index) const
		{
			for (unsigned int i = 0; i < SIZE; ++i) {
				if (buildin[i].offset == index) {
					return buildin[i].key;
				}
			}
			for (std::map<std::string, int>::const_iterator
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
		int operator[](const char *const key) const
		{
			DataKey k{};
			k.key = key;
			k.keylen = strlen(key);
			const auto it = std::lower_bound(buildin.begin(), buildin.end(), k, DataKey::key_pred);
			if (it != buildin.end() && it->keylen == k.keylen && 0 == strcmp(it->key, key)) {
				return it->offset;
			} else {
				std::map<std::string, int>::const_iterator
				ret(user.find(key));
				if (ret != user.end()) {
					return (*ret).second;
				}
			}

			return -1;
		}

		int operator[](const std::string &key) const
		{
			return (*this)[key.c_str()];
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
	std::vector<wyrmgus::unit_variable> Variable;   /// Array of user defined variables (default value for unittype).
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
extern wyrmgus::unit_type *settlement_site_unit_type;
//Wyrmgus end

extern wyrmgus::unit_type *CclGetUnitType(lua_State *l);  /// Access unit-type object
extern void UnitTypeCclRegister();               /// Register ccl features

extern void UpdateUnitStats(wyrmgus::unit_type &type, int reset_to_default);       /// Update unit stats
extern void UpdateStats(int reset_to_default);       /// Update unit stats
//Wyrmgus start

extern std::string GetUnitTypeStatsString(const std::string &unit_type_ident);
//Wyrmgus end

extern void SaveUnitTypes(CFile &file);              /// Save the unit-type table
/// Draw the sprite frame of unit-type
extern void DrawUnitType(const wyrmgus::unit_type &type, const std::shared_ptr<CPlayerColorGraphic> &sprite,
						 int player, int frame, const PixelPos &screenPos, const wyrmgus::time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands);

extern void InitUnitTypes(int reset_player_stats);   /// Init unit-type table
//Wyrmgus start
extern void InitUnitType(wyrmgus::unit_type &type);			/// Init unit-type
//Wyrmgus end
extern void LoadUnitTypeSprite(wyrmgus::unit_type &unittype); /// Load the sprite for a unittype
extern int GetUnitTypesCount();                     /// Get the amount of unit-types
extern void LoadUnitTypes();                     /// Load the unit-type data
extern void LoadUnitType(unit_type *unit_type);	/// Load a unittype
extern void CleanUnitTypeVariables();                    /// Cleanup unit-type module

// in script_unittype.c

/// Parse User Variables field.
extern void DefineVariableField(lua_State *l, wyrmgus::unit_variable &var, int lua_index);

/// Update custom Variables with other variable (like Hp, ...)
extern void UpdateUnitVariables(CUnit &unit);

extern void SetModStat(const std::string &mod_file, const std::string &ident, const std::string &variable_key, const int value, const std::string &variable_type);
extern void SetModSound(const std::string &mod_file, const std::string &ident, const std::string &sound, const std::string &sound_type, const std::string &sound_subtype = "");

//Wyrmgus start
extern std::string GetImageLayerNameById(int image_layer);
extern int GetImageLayerIdByName(const std::string &image_layer);
//Wyrmgus end

extern std::string GetItemEffectsString(const std::string &item_ident);
