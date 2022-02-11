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
//      (c) Copyright 2014-2022 by Andrettin
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

#include "economy/resource.h"
#include "ui/icon.h"
#include "unit/unit_type.h" //for the image layers enum
#include "upgrade/upgrade_structs.h" //for the costs enum

class CGraphic;
class CPlayerColorGraphic;

int CclDefineUnitType(lua_State *l);

namespace wyrmgus {

class and_condition;
class animation_set;
class construction;
class season;
class terrain_type;
class unit_type;
class variation_tag;
enum class item_class;

class unit_type_variation final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString identifier READ get_identifier_qstring CONSTANT)
	Q_PROPERTY(QString name READ get_name_qstring NOTIFY changed)

public:
	explicit unit_type_variation(const std::string &identifier, const unit_type *unit_type, const int image_layer = -1);
	~unit_type_variation();

	qunique_ptr<unit_type_variation> duplicate(const unit_type *unit_type) const
	{
		auto variation = make_qunique<unit_type_variation>(this->identifier, unit_type, this->ImageLayer);
		variation->moveToThread(QApplication::instance()->thread());

		variation->name = this->name;
		variation->type_name = this->type_name;
		variation->button_key = this->button_key;
		variation->image_file = this->image_file;
		for (unsigned int i = 0; i < MaxCosts; ++i) {
			variation->FileWhenLoaded[i] = this->FileWhenLoaded[i];
			variation->FileWhenEmpty[i] = this->FileWhenEmpty[i];
		}
		variation->ShadowFile = this->ShadowFile;
		variation->LightFile = this->LightFile;
		variation->frame_size = this->frame_size;
		variation->ResourceMin = this->ResourceMin;
		variation->ResourceMax = this->ResourceMax;
		variation->Weight = this->Weight;
		variation->Icon = this->Icon;
		variation->animation_set = this->animation_set;
		variation->construction = this->construction;
		variation->UpgradesRequired = this->UpgradesRequired;
		variation->UpgradesForbidden = this->UpgradesForbidden;
		variation->item_classes_equipped = this->item_classes_equipped;
		variation->item_classes_not_equipped = this->item_classes_not_equipped;
		variation->ItemsEquipped = this->ItemsEquipped;
		variation->ItemsNotEquipped = this->ItemsNotEquipped;
		variation->Terrains = this->Terrains;
		for (int i = 0; i < MaxImageLayers; ++i) {
			variation->LayerFiles[i] = this->LayerFiles[i];
		}
		variation->ButtonIcons = this->ButtonIcons;
		variation->player_conditions_ptr = this->player_conditions_ptr;
		variation->unit_conditions_ptr = this->unit_conditions_ptr;

		return variation;
	}

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	void ProcessConfigData(const CConfigData *config_data);
	void check() const;

	const std::string &get_identifier() const
	{
		return this->identifier;
	}

	QString get_identifier_qstring() const
	{
		return QString::fromStdString(this->get_identifier());
	}

	int get_index() const
	{
		return this->index;
	}

	const wyrmgus::unit_type *get_unit_type() const
	{
		return this->unit_type;
	}
	
	const std::string &get_name() const
	{
		return this->name;
	}

	Q_INVOKABLE void set_name(const std::string &name)
	{
		this->name = name;
	}

	QString get_name_qstring() const
	{
		return QString::fromStdString(this->get_name());
	}

	const std::filesystem::path &get_image_file() const
	{
		return this->image_file;
	}

	const QSize &get_frame_size() const
	{
		return this->frame_size;
	}
	
	const std::string &get_type_name() const
	{
		return this->type_name;
	}
	
	const std::string &get_button_key() const
	{
		return this->button_key;
	}
	
	const wyrmgus::animation_set *get_animation_set() const
	{
		return this->animation_set;
	}
	
	const wyrmgus::construction *get_construction() const
	{
		return this->construction;
	}
	
	const and_condition *get_player_conditions() const
	{
		return this->player_conditions_ptr;
	}

	const and_condition *get_unit_conditions() const
	{
		return this->unit_conditions_ptr;
	}

	const std::set<const variation_tag *> &get_tags() const
	{
		return this->tags;
	}

	bool has_tag(const variation_tag *tag) const
	{
		return this->tags.contains(tag);
	}

	size_t get_shared_tag_count(const std::set<const variation_tag *> &other_tags) const
	{
		size_t shared_tag_count = 0;

		for (const variation_tag *tag : other_tags) {
			if (this->has_tag(tag)) {
				++shared_tag_count;
			}
		}

		return shared_tag_count;
	}

signals:
	void changed();

private:
	std::string identifier;
	int index = -1;					//the variation's index within the appropriate variation vector of its unit type
	const wyrmgus::unit_type *unit_type = nullptr; //the unit type to which the variation belongs
public:
	int ImageLayer = -1;			/// The image layer to which the variation belongs (if any)
private:
	std::string name;
	std::filesystem::path image_file;
	QSize frame_size = QSize(0, 0);
	std::string type_name;			/// Type name.
	std::string button_key;
public:
	std::string ShadowFile;			/// Variation's shadow graphics.
	std::string LightFile;			/// Variation's light graphics.
	int ResourceMin = 0;
	int ResourceMax = 0;
	int Weight = 1;							/// The weight for when randomly choosing a variation
	IconConfig Icon;						/// Icon to display for this unit
	std::shared_ptr<CPlayerColorGraphic> Sprite; /// The graphic corresponding to File.
	std::shared_ptr<CGraphic> ShadowSprite;		/// The graphic corresponding to ShadowFile.
	std::shared_ptr<CGraphic> LightSprite;		/// The graphic corresponding to LightFile.
private:
	wyrmgus::animation_set *animation_set = nullptr;
	wyrmgus::construction *construction = nullptr;	/// What is shown in construction phase

public:
	std::vector<const CUpgrade *> UpgradesRequired;		/// Upgrades required by variation
	std::vector<const CUpgrade *> UpgradesForbidden;	/// If the player has one of these upgrades, the unit can't have this variation
	std::set<item_class> item_classes_equipped;
	std::set<item_class> item_classes_not_equipped;
	std::vector<const wyrmgus::unit_type *> ItemsEquipped;
	std::vector<const wyrmgus::unit_type *> ItemsNotEquipped;
	std::vector<const terrain_type *> Terrains;
	std::vector<const terrain_type *> TerrainsForbidden;
	std::vector<const season *> Seasons;
	std::vector<const season *> ForbiddenSeasons;

	std::string LayerFiles[MaxImageLayers];	/// Variation's layer graphics.
	std::filesystem::path FileWhenLoaded[MaxCosts];     /// Change the graphic when the unit is loaded.
	std::filesystem::path FileWhenEmpty[MaxCosts];      /// Change the graphic when the unit is empty.
	std::shared_ptr<CPlayerColorGraphic> LayerSprites[MaxImageLayers];	/// The graphics corresponding to LayerFiles.
	std::shared_ptr<CPlayerColorGraphic> SpriteWhenLoaded[MaxCosts]; /// The graphic corresponding to FileWhenLoaded.
	std::shared_ptr<CPlayerColorGraphic> SpriteWhenEmpty[MaxCosts];  /// The graphic corresponding to FileWhenEmpty
	
	std::map<ButtonCmd, IconConfig> ButtonIcons;				/// icons for button actions

private:
	std::unique_ptr<const and_condition> player_conditions;
	const and_condition *player_conditions_ptr = nullptr;
	std::unique_ptr<const and_condition> unit_conditions;
	const and_condition *unit_conditions_ptr = nullptr;
	std::set<const variation_tag *> tags;

	friend int ::CclDefineUnitType(lua_State *l);
	friend class unit_type;
};

}
