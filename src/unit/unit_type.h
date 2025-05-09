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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "animation/animation_set.h" //for the ANIMATIONS_DEATHTYPES constant
#include "color.h"
#include "database/detailed_data_entry.h"
#include "database/data_type.h"
#include "economy/resource_container.h"
#include "missileconfig.h"
#include "ui/icon.h"
#include "unit/unit_stats.h"
#include "util/color_container.h"
#include "vec2i.h"

#ifdef __MORPHOS__
#undef Enable
#endif

class CFile;
class CPlayer;
class CPlayerColorGraphic;
class CUnit;
class CUpgrade;
class LuaCallback;
enum class ButtonCmd;
struct lua_State;

extern int CclDefineDependency(lua_State *l);
extern int CclDefinePredependency(lua_State *l);
extern int CclDefineUnitType(lua_State *l);

namespace archimedes {
	class name_generator;
	enum class colorization_type;
	enum class gender;
}

namespace wyrmgus {
	class and_build_restriction;
	class animation_set;
	class build_restriction;
	class button_level;
	class civilization;
	class civilization_group;
	class construction;
	class employment_type;
	class faction;
	class font;
	class icon;
	class missile_type;
	class player_color;
	class renderer;
	class resource;
	class site;
	class species;
	class spell;
	class terrain_type;
	class time_of_day;
	class unit_class;
	class unit_sound_set;
	class unit_type;
	class unit_type_variation;
	class variation_tag;
	class world;
	enum class can_target_flag;
	enum class item_class;
	enum class item_slot;
	enum class status_effect;
	enum class tile_flag : uint32_t;
	enum class unit_domain;

	template <typename scope_type>
	class condition;

	template <typename scope_type>
	class conditional_string;
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

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

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
	STAR_INDEX,
	ASTEROID_INDEX,
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
	SLOT_INDEX,
	SHIELD_INDEX,
	POINTS_INDEX,
	MAXHARVESTERS_INDEX,
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
	BONUS_AGAINST_INFANTRY_INDEX,
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
	COST_MODIFIER_INDEX,
	MUGGING_INDEX,
	RAIDING_INDEX,
	DESERTSTALK_INDEX,
	FORESTSTALK_INDEX,
	SWAMPSTALK_INDEX,
	AURA_RANGE_BONUS_INDEX,
	LEADERSHIPAURA_INDEX,
	REGENERATIONAURA_INDEX,
	HYDRATINGAURA_INDEX,
	ETHEREALVISION_INDEX,
	HERO_INDEX,
	CAPTURE_HP_THRESHOLD_INDEX,
	GARRISONED_GATHERING_INDEX,
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
	virtual ~CDecoVar()
	{
	}

	/// function to draw the decorations.
	virtual void Draw(int x, int y, const unit_type &type, const unit_variable &var, std::vector<std::function<void(renderer *)>> &render_commands) const = 0;

	unsigned int Index = 0;	/// Index of the variable. @see DefineVariables

	//Wyrmgus start
	int MinValue = 0;		/// Minimum value of the variable
	//Wyrmgus end

	int OffsetX = 0;		/// Offset in X coord.
	int OffsetY = 0;		/// Offset in Y coord.

	int OffsetXPercent = 0;	/// Percent offset (tile_size.width()) in X coord.
	int OffsetYPercent = 0;	/// Percent offset (tile_size.height()) in Y coord.

	bool IsCenteredInX = false;	/// if true, use center of deco instead of left border
	bool IsCenteredInY = false;	/// if true, use center of deco instead of upper border

	bool ShowIfNotEnable = false;   /// if false, Show only if var is enable
	bool ShowWhenNull = false;      /// if false, don't show if var is null (F.E poison)
	bool HideHalf = false;          /// if true, don't show when 0 < var < max.
	bool ShowWhenMax = false;       /// if false, don't show if var is to max. (Like mana)
	bool ShowOnlySelected = false;  /// if true, show only for selected units.

	bool HideNeutral = false;       /// if true, don't show for neutral unit.
	bool HideAllied = false;        /// if true, don't show for allied unit. (but show own units)
	//Wyrmgus start
	bool HideSelf = false;			/// if true, don't show for own units.
	//Wyrmgus end
	bool ShowOpponent = false;      /// if true, show for opponent unit.
	
	bool ShowIfCanCastAnySpell = false;   /// if true, only show if the unit can cast a spell.

	std::optional<wyrmgus::status_effect> status_effect;
	bool show_as_status_effect = false;
	bool hero_symbol = false;
	bool hp_bar = false;
	bool resource_bar = false;
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

namespace wyrmgus {

/// Base structure of unit-type
/// @todo n0body: AutoBuildRate not implemented.
class unit_type final : public detailed_data_entry, public data_type<unit_type>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::unit_class* unit_class MEMBER unit_class WRITE set_unit_class NOTIFY changed)
	Q_PROPERTY(bool template MEMBER template_type READ is_template)
	Q_PROPERTY(wyrmgus::unit_domain domain MEMBER domain READ get_domain NOTIFY changed)
	Q_PROPERTY(wyrmgus::civilization* civilization MEMBER civilization NOTIFY changed)
	Q_PROPERTY(wyrmgus::civilization_group* civilization_group MEMBER civilization_group NOTIFY changed)
	Q_PROPERTY(wyrmgus::faction* faction MEMBER faction NOTIFY changed)
	Q_PROPERTY(wyrmgus::animation_set* animation_set MEMBER animation_set)
	Q_PROPERTY(wyrmgus::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(QSize tile_size MEMBER tile_size READ get_tile_size)
	Q_PROPERTY(QSize box_size MEMBER box_size READ get_box_size)
	Q_PROPERTY(QPoint box_offset MEMBER box_offset READ get_box_offset)
	Q_PROPERTY(std::filesystem::path image_file MEMBER image_file WRITE set_image_file)
	Q_PROPERTY(QSize frame_size MEMBER frame_size READ get_frame_size)
	Q_PROPERTY(QPoint offset MEMBER offset READ get_offset)
	Q_PROPERTY(int num_directions MEMBER num_directions READ get_num_directions)
	Q_PROPERTY(wyrmgus::player_color* conversible_player_color MEMBER conversible_player_color READ get_conversible_player_color)
	Q_PROPERTY(int hue_rotation MEMBER hue_rotation READ get_hue_rotation)
	Q_PROPERTY(wyrmgus::colorization_type colorization MEMBER colorization READ get_colorization)
	Q_PROPERTY(int draw_level MEMBER draw_level READ get_draw_level)
	Q_PROPERTY(wyrmgus::item_class item_class MEMBER item_class READ get_item_class)
	Q_PROPERTY(wyrmgus::species* species MEMBER species)
	Q_PROPERTY(wyrmgus::terrain_type* terrain_type MEMBER terrain_type)
	Q_PROPERTY(CUpgrade* elixir MEMBER elixir)
	Q_PROPERTY(int max_spawned_demand MEMBER max_spawned_demand READ get_max_spawned_demand)
	Q_PROPERTY(QVariantList traits READ get_traits_qvariant_list NOTIFY changed)
	Q_PROPERTY(wyrmgus::unit_type* corpse_type MEMBER corpse_type READ get_corpse_type)
	Q_PROPERTY(int repair_hp MEMBER repair_hp READ get_repair_hp)
	Q_PROPERTY(wyrmgus::construction* construction MEMBER construction)
	Q_PROPERTY(wyrmgus::construction* on_top_construction MEMBER on_top_construction)
	Q_PROPERTY(wyrmgus::resource* given_resource MEMBER given_resource)
	Q_PROPERTY(int random_movement_probability MEMBER random_movement_probability READ get_random_movement_probability)
	Q_PROPERTY(int neutral_random_movement_probability MEMBER neutral_random_movement_probability READ get_neutral_random_movement_probability)
	Q_PROPERTY(int random_movement_distance MEMBER random_movement_distance READ get_random_movement_distance)
	Q_PROPERTY(quint64 default_mass MEMBER default_mass READ get_default_mass)
	Q_PROPERTY(wyrmgus::employment_type* employment_type MEMBER employment_type)
	Q_PROPERTY(int employment_capacity MEMBER employment_capacity READ get_employment_capacity)
	Q_PROPERTY(QColor neutral_minimap_color MEMBER neutral_minimap_color READ get_neutral_minimap_color)
	Q_PROPERTY(QString encyclopedia_background_file READ get_encyclopedia_background_file_qstring NOTIFY changed)
	Q_PROPERTY(bool disabled MEMBER disabled READ is_disabled)

public:
	static constexpr const char *class_identifier = "unit_type";
	static constexpr const char property_class_identifier[] = "wyrmgus::unit_type*";
	static constexpr const char *database_folder = "unit_types";

	static unit_type *add(const std::string &identifier, const wyrmgus::data_module *data_module)
	{
		unit_type *unit_type = data_type::add(identifier, data_module);
		unit_type->Slot = static_cast<int>(unit_type::get_all().size()) - 1;
		return unit_type;
	}

	static const unit_type *try_get_by_0_ad_template_name(const std::string &str)
	{
		const auto find_iterator = unit_type::unit_types_by_0_ad_template_name.find(str);
		if (find_iterator != unit_type::unit_types_by_0_ad_template_name.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	static void clear()
	{
		data_type::clear();

		unit_type::unit_types_by_0_ad_template_name.clear();
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

private:
	static inline std::map<std::string, const unit_type *> unit_types_by_0_ad_template_name;

public:
	explicit unit_type(const std::string &identifier);
	~unit_type();

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
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

	unit_domain get_domain() const
	{
		return this->domain;
	}

	bool is_land_unit() const;
	bool is_water_unit() const;
	bool is_air_unit() const;

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

	const QPoint &get_box_offset() const
	{
		return this->box_offset;
	}

	QRect get_scaled_box_rect(const QPoint &center_pixel_pos) const;

	const std::filesystem::path &get_image_file() const
	{
		return this->image_file;
	}

	void set_image_file(const std::filesystem::path &filepath);

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

	int get_hue_rotation() const
	{
		return this->hue_rotation;
	}

	colorization_type get_colorization() const
	{
		return this->colorization;
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
	bool CanTransport() const
	{
		return this->MaxOnBoard > 0;
	}
	//Wyrmgus end

	bool CanMove() const;

	bool CanSelect(GroupSelectionMode mode = GroupSelectionMode::SELECTABLE_BY_RECTANGLE_ONLY) const;

	bool can_target(const CUnit *unit) const;

	bool can_repair() const
	{
		return this->RepairRange > 0;
	}
	
	void set_parent(const unit_type *parent_type);
	void RemoveButtons(const ButtonCmd button_action);

	void UpdateDefaultBoolFlags();
	void calculate_movement_mask();

	int GetAvailableLevelUpUpgrades() const;
	int get_resource_step(const resource *resource, const int player) const;
	const unit_type_variation *GetDefaultVariation(const CPlayer *player, const int image_layer = -1) const;
	const unit_type_variation *get_variation(const std::set<const variation_tag *> &variation_tags, const int image_layer = -1) const;
	std::string GetRandomVariationIdent(int image_layer = -1) const;
	const std::string &get_default_name(const CPlayer *player) const;
	std::string get_name_for_player(const CPlayer *player) const;
	const std::shared_ptr<CPlayerColorGraphic> &GetDefaultLayerSprite(const CPlayer *player, const int image_layer) const;
	const std::string &get_default_button_key(const CPlayer *player) const;
	bool CanExperienceUpgradeTo(const unit_type *type) const;
	std::string GetNamePlural() const;
	std::string generate_personal_name(const wyrmgus::faction *faction, const gender gender) const;
	bool is_personal_name_valid(const std::string &name, const faction *faction, const gender gender) const;
	const name_generator *get_name_generator(const wyrmgus::faction *faction, const gender gender) const;

	int get_incremental_cost_modifier() const
	{
		return this->incremental_cost_modifier;
	}

	wyrmgus::item_class get_item_class() const
	{
		return this->item_class;
	}

	wyrmgus::species *get_species() const
	{
		return this->species;
	}

	const wyrmgus::terrain_type *get_terrain_type() const
	{
		return this->terrain_type;
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

	const std::vector<const unit_type *> &get_spawned_units() const
	{
		return this->spawned_units;
	}

	const std::vector<const unit_type *> &get_neutral_spawned_units() const
	{
		return this->neutral_spawned_units;
	}

	int get_max_spawned_demand() const
	{
		return this->max_spawned_demand;
	}

	const std::vector<const CUpgrade *> &get_affixes() const
	{
		return this->affixes;
	}

	const std::vector<CUpgrade *> &get_traits() const
	{
		return this->traits;
	}

	QVariantList get_traits_qvariant_list() const;

	Q_INVOKABLE void add_trait(CUpgrade *trait)
	{
		this->traits.push_back(trait);
	}

	Q_INVOKABLE void remove_trait(CUpgrade *trait);

	unit_type *get_corpse_type() const
	{
		return this->corpse_type;
	}

	const wyrmgus::construction *get_construction() const
	{
		return this->construction;
	}

	const wyrmgus::construction *get_on_top_construction() const
	{
		return this->on_top_construction;
	}

	int get_repair_hp() const
	{
		return this->repair_hp;
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

	int get_neutral_random_movement_probability() const
	{
		return this->neutral_random_movement_probability;
	}

	int get_random_movement_distance() const
	{
		return this->random_movement_distance;
	}

	uint64_t get_default_mass() const
	{
		return this->default_mass;
	}

	const wyrmgus::employment_type *get_employment_type() const
	{
		return this->employment_type;
	}

	int get_employment_capacity() const
	{
		return this->employment_capacity;
	}

	const std::vector<qunique_ptr<unit_type_variation>> &get_variations() const
	{
		return this->variations;
	}

	std::vector<variation_tag *> get_custom_hero_hair_color_tags() const;
	Q_INVOKABLE QVariantList get_custom_hero_hair_color_tags_qvariant_list() const;

	const and_build_restriction *get_build_restrictions() const
	{
		return this->build_restrictions.get();
	}

	const and_build_restriction *get_ai_build_restrictions() const
	{
		return this->ai_build_restrictions.get();
	}

	bool has_ontop_buildings() const
	{
		return !this->ontop_buildings.empty();
	}

	const QColor &get_neutral_minimap_color() const
	{
		return this->neutral_minimap_color;
	}

	const std::filesystem::path &get_encyclopedia_background_file() const;
	QString get_encyclopedia_background_file_qstring() const;

	void set_encyclopedia_background_file(const std::filesystem::path &filepath);

	Q_INVOKABLE void set_encyclopedia_background_file(const std::string &filepath)
	{
		this->set_encyclopedia_background_file(std::filesystem::path(filepath));
	}

	const unit_sound_set *get_sound_set() const
	{
		return this->sound_set.get();
	}

	const std::unique_ptr<condition<CPlayer>> &get_preconditions() const
	{
		return this->preconditions;
	}

	const std::unique_ptr<condition<CPlayer>> &get_conditions() const
	{
		return this->conditions;
	}

	const std::string &get_requirements_string() const;
	const std::string &get_requirements_string(const CPlayer *player) const;

	bool can_gain_experience() const;
	bool is_infantry() const;

	gender get_gender() const;

	std::string get_build_verb_string() const;
	std::string get_destroy_verb_string() const;

	bool pos_borders_impassable(const QPoint &pos, const int z) const;
	bool can_be_dropped_on_pos(const QPoint &pos, const int z, const bool no_building_bordering_impassable, const bool ignore_ontop, const site *settlement) const;

	bool is_disabled() const
	{
		return this->disabled;
	}

	void map_to_0_ad_template_name(const std::string &str);

	Q_INVOKABLE void pack_image_layers(const QString &image_filepath) const;

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
	std::string requirements_string;
	std::vector<std::unique_ptr<const conditional_string<CPlayer>>> conditional_requirements_strings;
public:
	std::string ExperienceRequirementsString;	/// Experience requirements string of the unit type
	std::string BuildingRulesString;	/// Building rules string of the unit type
private:
	CUpgrade *elixir = nullptr; //which elixir does this (item) unit type always have
public:
	std::vector<unit_type *> SoldUnits;		/// Units which this unit can sell.
private:
	std::vector<const unit_type *> spawned_units;
	std::vector<const unit_type *> neutral_spawned_units;
	int max_spawned_demand = 0; //the maximum amount of total food demand the nearby units of the spawned unit types for this unit type can have before spawning stops
public:
	std::vector<unit_type *> Drops;			/// Units which can spawn upon death (i.e. items).
	std::vector<unit_type *> AiDrops;		/// Units which can spawn upon death (i.e. items), only for AI-controlled units.
	std::vector<spell *> DropSpells;		/// Spells which can be applied to dropped items
private:
	std::vector<const CUpgrade *> affixes;		/// Affixes which can be generated for this unit type
	std::vector<CUpgrade *> traits; //which traits this unit type can have
public:
	std::vector<const CUpgrade *> StartingAbilities;	/// Abilities which the unit starts out with
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
	int hue_rotation = 0;
	colorization_type colorization;
	color_set hue_ignored_colors;
	int draw_level = 0;                                   /// Level to Draw UnitType at
public:
	int ShadowWidth = 0;					/// Shadow sprite width
	int ShadowHeight = 0;					/// Shadow sprite height
	int ShadowOffsetX = 0;					/// Shadow horizontal offset
	int ShadowOffsetY = 0;					/// Shadow vertical offset
	int TrainQuantity = 0;					/// Quantity to be trained
private:
	int incremental_cost_modifier = 0; //cost increase for every unit of this type the player has
	wyrmgus::item_class item_class; //item class (if the unit type is an item)
	wyrmgus::species *species = nullptr;
	wyrmgus::terrain_type *terrain_type = nullptr;
public:
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

	wyrmgus::construction *construction = nullptr; //what is shown in the construction phase
	wyrmgus::construction *on_top_construction = nullptr;

	int repair_hp = 0;				/// Amount of HP per repair
	resource_map<int> repair_costs;      /// How much it costs to repair

private:
	QSize tile_size = QSize(0, 0);
	QSize box_size = QSize(0, 0);
	QPoint box_offset = QPoint(0, 0);
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
	int random_movement_probability = 0; //probability to move randomly
	int neutral_random_movement_probability = 0; //probability to move randomly if owned by the neutral player
	int random_movement_distance = 1; //quantity of tiles to move randomly
public:
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
	unit_domain domain; //whether the unit is a land, water or etc. unit
public:
	//Wyrmgus end
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

private:
	can_target_flag can_target_flags; //which units can it attack

public:
	unsigned Flip : 1 = 1;              /// Flip image when facing left
	unsigned ExplodeWhenKilled : 1 = 0; /// Death explosion animated
	unsigned CanAttack : 1 = 0;         /// Unit can attack.
	unsigned Neutral : 1 = 0;           /// Unit is neutral, used by the editor

private:
	uint64_t default_mass = 0; //the default mass of the unit type, if it is a celestial body

public:
	unit_stats DefaultStat;

	struct BoolFlags {
		bool value;             /// User defined flag. Used for (dis)allow target.
		char CanTransport;      /// Can transport units with this flag.
		char CanTargetFlag;     /// Flag needed to target with missile.
		char AiPriorityTarget;  /// Attack this units first.
	};
	std::vector<BoolFlags> BoolFlag;

private:
	wyrmgus::employment_type *employment_type = nullptr;
	int employment_capacity = 0;
	resource_set stored_resources;             /// Resources that we can store here.
	resource *given_resource = nullptr; //the resource this unit gives
	resource_map<std::unique_ptr<resource_info>> resource_infos;    /// Resource information.
	std::vector<qunique_ptr<unit_type_variation>> variations;
public:
	//Wyrmgus start
	std::vector<qunique_ptr<unit_type_variation>> LayerVariations[MaxImageLayers];	/// Layer variation information
	//Wyrmgus end
private:
	std::unique_ptr<and_build_restriction> build_restrictions;
	std::unique_ptr<and_build_restriction> ai_build_restrictions;
	std::vector<const unit_type *> ontop_buildings; //buildings which can be built on top of this one
	QColor neutral_minimap_color; //minimap color for neutral units
	std::filesystem::path encyclopedia_background_file;

	std::unique_ptr<unit_sound_set> sound_set;			/// Sounds for events

public:
	int PoisonDrain = 0;                /// How much health is drained every second when poisoned

	// --- FILLED UP ---

	tile_flag FieldFlags;            /// Unit map field flags
	tile_flag MovementMask;          /// Unit check this map flags for move

	/// @todo This stats should? be moved into the player struct
	unit_stats Stats[PlayerMax];     /// Unit status for each player

	std::shared_ptr<CPlayerColorGraphic> Sprite;     /// Sprite images
	std::shared_ptr<CGraphic> ShadowSprite;          /// Shadow sprite image
	//Wyrmgus start
	std::shared_ptr<CGraphic> LightSprite;						/// Light sprite image
	std::array<std::shared_ptr<CPlayerColorGraphic>, MaxImageLayers> LayerSprites{};	/// Layer sprite images
	//Wyrmgus end

private:
	std::unique_ptr<condition<CPlayer>> preconditions;
	std::unique_ptr<condition<CPlayer>> conditions;
	bool disabled = false;

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
			k.keylen = static_cast<unsigned int>(strlen(key));
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

//Wyrmgus start
extern std::string GetImageLayerNameById(int image_layer);
extern int GetImageLayerIdByName(const std::string &image_layer);
//Wyrmgus end

extern std::string GetItemEffectsString(const std::string &item_ident);
