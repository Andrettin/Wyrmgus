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
//      (c) Copyright 2019-2022 by Andrettin
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

#include "database/database.h"

#include "age.h"
#include "animation/animation_set.h"
#include "character.h"
#include "character_title.h"
#include "database/data_module.h"
#include "database/data_module_container.h"
#include "database/data_type_metadata.h"
#include "database/defines.h"
#include "database/predefines.h"
#include "database/gsml_data.h"
#include "database/gsml_operator.h"
#include "database/gsml_parser.h"
#include "database/gsml_property.h"
#include "dialogue.h"
#include "economy/resource.h"
#include "engine_interface.h"
#include "epithet.h"
#include "game/difficulty.h"
#include "gender.h"
#include "item/item_class.h"
#include "item/item_slot.h"
#include "item/recipe.h"
#include "item/unique_item.h"
#include "language/grammatical_gender.h"
#include "language/language.h"
#include "language/language_family.h"
#include "language/word.h"
#include "language/word_type.h"
#include "magic_domain.h"
#include "map/map_presets.h"
#include "map/map_projection.h"
#include "map/map_template.h"
#include "map/region.h"
#include "map/site.h"
#include "map/terrain_feature.h"
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "map/world.h"
#include "missile/missile_class.h"
#include "missile.h"
#include "player/civilization.h"
#include "player/civilization_group.h"
#include "player/civilization_group_rank.h"
#include "player/dynasty.h"
#include "player/faction.h"
#include "player/faction_tier.h"
#include "player/faction_type.h"
#include "player/government_type.h"
#include "player/player_color.h"
#include "population/employment_type.h"
#include "population/population_class.h"
#include "population/population_type.h"
#include "quest/achievement.h"
#include "quest/campaign.h"
#include "quest/quest.h"
#include "religion/deity.h"
#include "religion/pantheon.h"
#include "religion/religion.h"
#include "script/trigger_random_group.h"
#include "script/trigger_target.h"
#include "script/trigger_type.h"
#include "sound/music_type.h"
#include "sound/sound.h"
#include "species/ecological_niche.h"
#include "species/geological_era.h"
#include "species/species.h"
#include "species/taxon.h"
#include "species/taxonomic_rank.h"
#include "spell/spell.h"
#include "spell/spell_target_type.h"
#include "time/calendar.h"
#include "time/season_schedule.h"
#include "time/time_of_day.h"
#include "time/time_of_day_schedule.h"
#include "time/timeline.h"
#include "ui/button_level.h"
#include "ui/cursor.h"
#include "ui/cursor_type.h"
#include "ui/hotkey_setup.h"
#include "ui/icon.h"
#include "ui/interface_style.h"
#include "ui/resource_icon.h"
#include "unit/construction.h"
#include "unit/historical_unit.h"
#include "unit/unit_class.h"
#include "unit/unit_domain.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade_category.h"
#include "upgrade/upgrade_category_rank.h"
#include "upgrade/upgrade_class.h"
#include "upgrade/upgrade_structs.h"
#include "util/assert_util.h"
#include "util/colorization_type.h"
#include "util/geocoordinate.h"
#include "util/path_util.h"
#include "util/qunique_ptr.h"
#include "util/string_util.h"
#include "util/string_conversion_util.h"
#include "util/thread_pool.h"
#include "video/font.h"
#include "video/font_color.h"

namespace wyrmgus {

/**
**	@brief	Process a GSML property for an instance of a QObject-derived class
*/
void database::process_gsml_property_for_object(QObject *object, const gsml_property &property)
{
	const QMetaObject *meta_object = object->metaObject();
	const std::string class_name = meta_object->className();
	const int property_count = meta_object->propertyCount();
	for (int i = 0; i < property_count; ++i) {
		QMetaProperty meta_property = meta_object->property(i);
		const char *property_name = meta_property.name();

		if (property_name != property.get_key()) {
			continue;
		}

		const QVariant::Type property_type = meta_property.type();
		const std::string property_class_name = meta_property.typeName();

		if (property_type == QVariant::Type::List || property_type == QVariant::Type::StringList || (property_class_name.starts_with("std::vector<") && property_class_name.ends_with(">"))) {
			database::modify_list_property_for_object(object, property_name, property.get_operator(), property.get_value());
			return;
		} else if (property_type == QVariant::String) {
			if (property.get_operator() != gsml_operator::assignment) {
				throw std::runtime_error("Only the assignment operator is available for string properties.");
			}

			const std::string method_name = "set_" + property.get_key();

			const bool success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(const std::string &, property.get_value()));

			if (!success) {
				throw std::runtime_error("Failed to set value for string property \"" + property.get_key() + "\".");
			}
			return;
		} else {
			QVariant new_property_value = this->process_gsml_property_value(property, meta_property, object);
			bool success = object->setProperty(property_name, new_property_value);
			if (!success) {
				throw std::runtime_error("Failed to set value for property \"" + std::string(property_name) + "\".");
			}
			return;
		}
	}

	throw std::runtime_error("Invalid " + std::string(meta_object->className()) + " property: \"" + property.get_key() + "\".");
}

QVariant database::process_gsml_property_value(const gsml_property &property, const QMetaProperty &meta_property, const QObject *object)
{
	const std::string class_name = meta_property.enclosingMetaObject()->className();
	const char *property_name = meta_property.name();
	const std::string property_class_name = meta_property.typeName();
	const QVariant::Type property_type = meta_property.type();

	QVariant new_property_value;
	if (property_type == QVariant::Bool) {
		if (property.get_operator() != gsml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for boolean properties.");
		}

		new_property_value = string::to_bool(property.get_value());
	} else if (static_cast<QMetaType::Type>(property_type) == QMetaType::UChar) {
		unsigned value = std::stoul(property.get_value());

		if (property.get_operator() == gsml_operator::addition) {
			value = object->property(property_name).toUInt() + value;
		} else if (property.get_operator() == gsml_operator::subtraction) {
			value = object->property(property_name).toUInt() - value;
		}

		new_property_value = static_cast<unsigned char>(value);
	} else if (property_type == QVariant::Int) {
		int value = std::stoi(property.get_value());

		if (property.get_operator() == gsml_operator::addition) {
			value = object->property(property_name).toInt() + value;
		} else if (property.get_operator() == gsml_operator::subtraction) {
			value = object->property(property_name).toInt() - value;
		}

		new_property_value = value;
	} else if (property_type == QVariant::LongLong) {
		long long value = std::stoll(property.get_value());

		if (property.get_operator() == gsml_operator::addition) {
			value = object->property(property_name).toLongLong() + value;
		} else if (property.get_operator() == gsml_operator::subtraction) {
			value = object->property(property_name).toLongLong() - value;
		}

		new_property_value = value;
	} else if (property_type == QVariant::UInt) {
		unsigned value = std::stoul(property.get_value());

		if (property.get_operator() == gsml_operator::addition) {
			value = object->property(property_name).toUInt() + value;
		} else if (property.get_operator() == gsml_operator::subtraction) {
			value = object->property(property_name).toUInt() - value;
		}

		new_property_value = value;
	} else if (property_type == QVariant::ULongLong) {
		unsigned long long value = std::stoull(property.get_value());

		if (property.get_operator() == gsml_operator::addition) {
			value = object->property(property_name).toULongLong() + value;
		} else if (property.get_operator() == gsml_operator::subtraction) {
			value = object->property(property_name).toULongLong() - value;
		}

		new_property_value = value;
	} else if (property_type == QVariant::Double) {
		double value = std::stod(property.get_value());

		if (property.get_operator() == gsml_operator::addition) {
			value = object->property(property_name).toDouble() + value;
		} else if (property.get_operator() == gsml_operator::subtraction) {
			value = object->property(property_name).toDouble() - value;
		}

		new_property_value = value;
	} else if (property_type == QVariant::DateTime) {
		if (property.get_operator() != gsml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for date-time properties.");
		}

		new_property_value = string::to_date(property.get_value());
	} else if (property_type == QVariant::Time) {
		if (property.get_operator() != gsml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for date-time properties.");
		}

		new_property_value = string::to_time(property.get_value());
	} else if (property_type == QVariant::Type::UserType) {
		if (property.get_operator() != gsml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for object reference properties.");
		}

		if (property_class_name == "std::string") {
			new_property_value = QVariant::fromValue(property.get_value());
		} else if (property_class_name == "std::filesystem::path") {
			new_property_value = QVariant::fromValue(std::filesystem::path(property.get_value()));
		} else if (property_class_name == "archimedes::centesimal_int") {
			new_property_value = QVariant::fromValue(centesimal_int(property.get_value()));
		} else if (property_class_name == "archimedes::decimal_int") {
			new_property_value = QVariant::fromValue(decimal_int(property.get_value()));
		} else if (property_class_name == "archimedes::decimillesimal_int") {
			new_property_value = QVariant::fromValue(decimillesimal_int(property.get_value()));
		} else if (property_class_name == "archimedes::map_projection*") {
			new_property_value = QVariant::fromValue(map_projection::from_string(property.get_value()));
		} else if (property_class_name == "wyrmgus::module*") {
			new_property_value = QVariant::fromValue(database::get()->get_module(property.get_value()));
		} else {
			const auto find_iterator = this->string_to_qvariant_conversion_map.find(property_class_name);
			if (find_iterator != this->string_to_qvariant_conversion_map.end()) {
				new_property_value = find_iterator->second(property.get_value());
			} else {
				throw std::runtime_error("Unknown type (\"" + property_class_name + "\") for object reference property \"" + std::string(property_name) + "\" (\"" + property_class_name + "\").");
			}
		}
	} else {
	throw std::runtime_error("Invalid type for property \"" + std::string(property_name) + "\": \"" + std::string(meta_property.typeName()) + "\".");
	}

	return new_property_value;
}

void database::process_gsml_scope_for_object(QObject *object, const gsml_data &scope)
{
	const QMetaObject *meta_object = object->metaObject();
	const std::string class_name = meta_object->className();
	const int property_count = meta_object->propertyCount();
	for (int i = 0; i < property_count; ++i) {
		QMetaProperty meta_property = meta_object->property(i);
		const char *property_name = meta_property.name();

		if (property_name != scope.get_tag()) {
			continue;
		}

		const QVariant::Type property_type = meta_property.type();
		const std::string property_class_name = meta_property.typeName();

		if (scope.get_operator() == gsml_operator::assignment) {
			if ((property_type == QVariant::Type::List || property_type == QVariant::Type::StringList || (property_class_name.starts_with("std::vector<") && property_class_name.ends_with(">"))) && !scope.get_values().empty()) {
				for (const std::string &value : scope.get_values()) {
					this->modify_list_property_for_object(object, property_name, gsml_operator::addition, value);
				}
				return;
			} else if (property_type == QVariant::Type::List && scope.has_children()) {
				scope.for_each_child([&](const gsml_data &child_scope) {
					database::modify_list_property_for_object(object, property_name, gsml_operator::addition, child_scope);
				});
				return;
			}
		} else {
			if (property_type == QVariant::Type::List) {
				database::modify_list_property_for_object(object, property_name, scope.get_operator(), scope);
				return;
			}
		}

		QVariant new_property_value = database::process_gsml_scope_value(scope, meta_property);
		const bool success = object->setProperty(property_name, new_property_value);
		if (!success) {
			throw std::runtime_error("Failed to set value for scope property \"" + std::string(property_name) + "\".");
		}
		return;
	}

	throw std::runtime_error("Invalid " + std::string(meta_object->className()) + " scope property: \"" + scope.get_tag() + "\".");
}

QVariant database::process_gsml_scope_value(const gsml_data &scope, const QMetaProperty &meta_property)
{
	const std::string class_name = meta_property.enclosingMetaObject()->className();
	const char *property_name = meta_property.name();
	const std::string property_type_name = meta_property.typeName();
	const QVariant::Type property_type = meta_property.type();

	QVariant new_property_value;
	if (property_type == QVariant::Color) {
		if (scope.get_operator() != gsml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for color properties.");
		}

		new_property_value = scope.to_color();
	} else if (property_type == QVariant::Point) {
		if (scope.get_operator() != gsml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for point properties.");
		}

		new_property_value = scope.to_point();
	} else if (property_type == QVariant::PointF) {
		if (scope.get_operator() != gsml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for point properties.");
		}

		new_property_value = scope.to_pointf();
	} else if (property_type == QVariant::Size) {
		if (scope.get_operator() != gsml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for size properties.");
		}

		new_property_value = scope.to_size();
	} else if (property_type_name == "archimedes::geocoordinate") {
		if (scope.get_operator() != gsml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is available for geocoordinate properties.");
		}

		new_property_value = QVariant::fromValue(scope.to_geocoordinate());
	} else {
		throw std::runtime_error("Invalid type for scope property \"" + std::string(property_name) + "\": \"" + std::string(meta_property.typeName()) + "\".");
	}

	return new_property_value;
}

void database::modify_list_property_for_object(QObject *object, const std::string &property_name, const gsml_operator gsml_operator, const std::string &value)
{
	const QMetaObject *meta_object = object->metaObject();
	const std::string class_name = meta_object->className();
	const int property_index = meta_object->indexOfProperty(property_name.c_str());
	QMetaProperty meta_property = meta_object->property(property_index);
	const QVariant::Type property_type = meta_property.type();
	const std::string property_class_name = meta_property.typeName();

	if (gsml_operator == gsml_operator::assignment) {
		throw std::runtime_error("The assignment operator is not available for list properties.");
	}

	std::string method_name;
	if (gsml_operator == gsml_operator::addition) {
		method_name = "add_";
	} else if (gsml_operator == gsml_operator::subtraction) {
		method_name = "remove_";
	}

	method_name += string::get_singular_form(property_name);

	bool success = false;

	if (property_name == "civilizations") {
		civilization *civilization_value = civilization::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(civilization *, civilization_value));
	} else if (property_name == "domains") {
		magic_domain *domain_value = magic_domain::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(magic_domain *, domain_value));
	} else if (property_name == "factions") {
		faction *faction_value = faction::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(faction *, faction_value));
	} else if (property_name == "files") {
		const std::filesystem::path filepath(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(const std::filesystem::path &, filepath));
	} else if (property_name == "map_templates") {
		map_template *map_template_value = map_template::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(map_template *, map_template_value));
	} else if (property_name == "regions" || property_name == "superregions") {
		region *region_value = region::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(region *, region_value));
	} else if (property_name == "religions") {
		religion *religion_value = religion::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(religion *, religion_value));
	} else if (property_name == "terrain_types" || property_name == "base_terrain_types" || property_name == "outer_border_terrain_types" || property_name == "inner_border_terrain_types") {
		terrain_type *terrain_type_value = terrain_type::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(terrain_type *, terrain_type_value));
	} else if (property_name == "unit_classes" || property_name == "building_classes") {
		unit_class *unit_class_value = unit_class::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(unit_class *, unit_class_value));
	} else if (property_name == "upgrades") {
		CUpgrade *upgrade_value = CUpgrade::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(CUpgrade *, upgrade_value));
	} else if (property_class_name == "std::vector<const wyrmgus::site*>") {
		const site *site_value = site::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(const site *, site_value));
	} else if (property_class_name == "std::vector<const CUpgrade*>") {
		const CUpgrade *upgrade_value = CUpgrade::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(const CUpgrade *, upgrade_value));
	} else if (property_class_name == "std::vector<const wyrmgus::upgrade_class*>") {
		const upgrade_class *upgrade_class_value = upgrade_class::get(value);
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(const upgrade_class *, upgrade_class_value));
	} else if (property_type == QVariant::Type::StringList) {
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(const std::string &, value));
	} else {
		const auto find_iterator = this->list_property_function_map.find(property_class_name);
		if (find_iterator != this->list_property_function_map.end()) {
			success = find_iterator->second(object, method_name, value);
		} else {
			throw std::runtime_error("Unknown type for list property \"" + property_name + "\" (in class \"" + class_name + "\").");
		}
	}

	if (!success) {
		throw std::runtime_error("Failed to add or remove value for list property \"" + property_name + "\".");
	}
}

void database::modify_list_property_for_object(QObject *object, const std::string &property_name, const gsml_operator gsml_operator, const gsml_data &scope)
{
	const QMetaObject *meta_object = object->metaObject();
	const std::string class_name = meta_object->className();

	if (gsml_operator == gsml_operator::assignment) {
		throw std::runtime_error("The assignment operator is not available for list properties.");
	}

	std::string method_name;
	if (gsml_operator == gsml_operator::addition) {
		method_name = "add_";
	} else if (gsml_operator == gsml_operator::subtraction) {
		method_name = "remove_";
	}

	method_name += string::get_singular_form(property_name);

	bool success = false;

	if (property_name == "colors") {
		const QColor color = scope.to_color();
		success = QMetaObject::invokeMethod(object, method_name.c_str(), Qt::ConnectionType::DirectConnection, Q_ARG(QColor, color));
	} else {
		throw std::runtime_error("Unknown type for list property \"" + property_name + "\" (in class \"" + class_name + "\").");
	}

	if (!success) {
		throw std::runtime_error("Failed to add or remove value for list property \"" + property_name + "\".");
	}
}

std::filesystem::path database::get_documents_path()
{
	std::filesystem::path documents_path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation).toStdString();
	if (documents_path.empty()) {
		throw std::runtime_error("No documents path found.");
	}

	documents_path /= QApplication::applicationName().toStdString();

	return documents_path;
}

std::filesystem::path database::get_user_data_path()
{
	std::filesystem::path path = path::from_qstring(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
	if (path.empty()) {
		throw std::runtime_error("No user data path found.");
	}

	//ignore the organization name for the user data path, e.g. the path should be "[AppName]" and not "[OrganizationName]/[AppName]"
	if (path::to_string(path.parent_path().filename()) == QApplication::organizationName().toStdString()) {
		path = path.parent_path().parent_path() / path.filename();
	}

	//ensure that the user data path exists
	database::ensure_path_exists(path);

	return path;
}

void database::ensure_path_exists(const std::filesystem::path &path)
{
	//create the path if necessary
	if (!std::filesystem::exists(path)) {
		const bool success = std::filesystem::create_directories(path);
		if (!success) {
			throw std::runtime_error("Failed to create path for Wyrmsun: \"" + path.string() + "\".");
		}
	}
}


const std::filesystem::path &database::get_base_path(const data_module *data_module) const
{
	if (data_module != nullptr) {
		return data_module->get_path();
	}

	return this->get_root_path();
}

boost::asio::awaitable<void> database::parse_folder(const std::filesystem::path &path, std::vector<gsml_data> &gsml_data_list)
{
	std::filesystem::recursive_directory_iterator dir_iterator(path);

	std::map<int, std::set<std::filesystem::path>> filepaths_by_depth;

	for (const std::filesystem::directory_entry &dir_entry : dir_iterator) {
		if (!dir_entry.is_regular_file() || dir_entry.path().extension() != ".txt") {
			continue;
		}

		//ensure that files with a lower depth will be processed earlier than those with a higher one, and that files will be processed in alphabetical order
		filepaths_by_depth[dir_iterator.depth()].insert(dir_entry.path());
	}

	std::vector<boost::asio::awaitable<gsml_data>> awaitables;

	for (const auto &kv_pair : filepaths_by_depth) {
		for (const std::filesystem::path &filepath : kv_pair.second) {
			boost::asio::awaitable<gsml_data> awaitable = thread_pool::get()->co_spawn_awaitable([&filepath]() -> boost::asio::awaitable<gsml_data> {
				gsml_parser parser;
				co_return parser.parse(filepath);
			});

			awaitables.push_back(std::move(awaitable));
		}
	}

	for (boost::asio::awaitable<gsml_data> &awaitable : awaitables) {
		gsml_data_list.push_back(co_await std::move(awaitable));
	}
}

database::database()
{
}

database::~database()
{
}

boost::asio::awaitable<void> database::parse()
{
	const auto data_paths_with_module = this->get_data_paths_with_module();
	for (const auto &kv_pair : data_paths_with_module) {
		const std::filesystem::path &path = kv_pair.first;
		const data_module *data_module = kv_pair.second;

		std::vector<boost::asio::awaitable<void>> awaitables;

		//parse the files in each data type's folder
		for (const std::unique_ptr<data_type_metadata> &metadata : this->metadata) {
			boost::asio::awaitable<void> awaitable = metadata->get_parsing_function()(path, data_module);
			awaitables.push_back(std::move(awaitable));
		}

		//we need to wait for the awaitables per module, so that this remains lock-free, as each data type has its own parsed GSML data list
		for (boost::asio::awaitable<void> &awaitable : awaitables) {
			co_await std::move(awaitable);
		}
	}
}

[[nodiscard]]
boost::asio::awaitable<void> database::load(const bool initial_definition)
{
	if (initial_definition) {
		//sort the metadata instances so they are placed after their class' dependencies' metadata
		std::sort(this->metadata.begin(), this->metadata.end(), [](const std::unique_ptr<data_type_metadata> &a, const std::unique_ptr<data_type_metadata> &b) {
			if (a->has_database_dependency_on(b)) {
				return false;
			} else if (b->has_database_dependency_on(a)) {
				return true;
			}

			if (a->get_database_dependency_count() != b->get_database_dependency_count()) {
				return a->get_database_dependency_count() < b->get_database_dependency_count();
			}

			return a->get_class_identifier() < b->get_class_identifier();
		});

		this->load_predefines();
		this->process_modules();
		co_await this->parse();
	}

	try {
		//create or process data entries for each data type
		for (const std::unique_ptr<data_type_metadata> &metadata : this->metadata) {
			metadata->get_processing_function()(initial_definition);
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to process database."));
	}
}

void database::load_predefines()
{
	try {
		predefines::get()->load(this->get_data_path());
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to load predefines."));
	}
}

void database::load_defines()
{
	for (const auto &kv_pair : this->get_data_paths_with_module()) {
		const std::filesystem::path &path = kv_pair.first;
		const data_module *data_module = kv_pair.second;

		try {
			defines::get()->load(path);
		} catch (...) {
			if (data_module != nullptr) {
				std::throw_with_nested(std::runtime_error("Failed to load the defines for the \"" + data_module->get_identifier() + "\" module."));
			} else {
				std::throw_with_nested(std::runtime_error("Failed to load defines."));
			}
		}
	}
}

void database::load_history()
{
	try {
		civilization::load_history_database();
		civilization_group::load_history_database();
		faction::load_history_database();
		site::load_history_database();
		region::load_history_database(); //must be loaded after sites, since it relies on their population data having been loaded first
		character::load_history_database();
		historical_unit::load_history_database();
		map_template::load_history_database();
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error loading history."));
	}
}

void database::initialize()
{
	defines::get()->initialize();

	//initialize data entries for each data type
	for (const std::unique_ptr<data_type_metadata> &metadata : this->metadata) {
		try {
			metadata->get_initialization_function()();
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Error initializing the instances of the " + metadata->get_class_identifier() + " class."));
		}
	}

	//process text for data entries for each data type
	for (const std::unique_ptr<data_type_metadata> &metadata : this->metadata) {
		try {
			metadata->get_text_processing_function()();
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Error processing text for the instances of the " + metadata->get_class_identifier() + " class."));
		}
	}

	this->initialized = true;

	//check if data entries are valid for each data type
	for (const std::unique_ptr<data_type_metadata> &metadata : this->metadata) {
		try {
			metadata->get_checking_function()();
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Error when checking the instances of the " + metadata->get_class_identifier() + " class."));
		}
	}

	quest::load_quest_completion();
	achievement::load_achievements();

	engine_interface::get()->set_running(true);
}

void database::clear()
{
	//clear data entries for each data type
	for (const std::unique_ptr<data_type_metadata> &metadata : this->metadata) {
		metadata->get_clearing_function()();
	}

	this->initialized = false;
}

void database::register_metadata(std::unique_ptr<data_type_metadata> &&metadata)
{
	this->metadata.push_back(std::move(metadata));
}

void database::process_modules()
{
	if (std::filesystem::exists(this->get_modules_path())) {
		this->process_modules_at_dir(this->get_modules_path());
	}

	if (std::filesystem::exists(this->get_dlcs_path())) {
		this->process_modules_at_dir(this->get_dlcs_path());
	}

	if (predefines::get()->is_documents_modules_loading_enabled() && std::filesystem::exists(database::get_documents_modules_path())) {
		this->process_modules_at_dir(database::get_documents_modules_path());
	}

	if (std::filesystem::exists(this->get_workshop_path())) {
		this->process_modules_at_dir(this->get_workshop_path());
	}

	for (const qunique_ptr<data_module> &data_module : this->modules) {
		const std::filesystem::path module_filepath = data_module->get_path() / "module.txt";

		if (std::filesystem::exists(module_filepath)) {
			gsml_parser parser;
			database::process_gsml_data(data_module, parser.parse(module_filepath));
		}
	}

	std::sort(this->modules.begin(), this->modules.end(), [](const qunique_ptr<data_module> &a, const qunique_ptr<data_module> &b) {
		return data_module_compare()(a.get(), b.get());
	});
}

void database::process_modules_at_dir(const std::filesystem::path &path, data_module *parent_module)
{
	std::filesystem::directory_iterator dir_iterator(path);

	for (const std::filesystem::directory_entry &dir_entry : dir_iterator) {
		if (!dir_entry.is_directory()) {
			continue;
		}

		if (dir_entry.path().stem().string().front() == '.') {
			continue; //ignore hidden directories, e.g. ".git" dirs
		}

		const std::string module_identifier = dir_entry.path().stem().string();
		auto data_module = make_qunique<wyrmgus::data_module>(module_identifier, dir_entry.path(), parent_module);

		std::filesystem::path submodules_path = dir_entry.path() / "modules";
		if (std::filesystem::exists(submodules_path)) {
			this->process_modules_at_dir(submodules_path, data_module.get());
		}

		this->modules_by_identifier[module_identifier] = data_module.get();
		this->modules.push_back(std::move(data_module));
	}
}

std::vector<std::filesystem::path> database::get_module_paths() const
{
	std::vector<std::filesystem::path> module_paths;

	for (const qunique_ptr<data_module> &data_module : this->modules) {
		module_paths.push_back(data_module->get_path());
	}

	return module_paths;
}

std::vector<std::pair<std::filesystem::path, const data_module *>> database::get_module_paths_with_module() const
{
	std::vector<std::pair<std::filesystem::path, const data_module *>> module_paths;

	for (const qunique_ptr<data_module> &data_module : this->modules) {
		module_paths.emplace_back(data_module->get_path(), data_module.get());
	}

	return module_paths;
}

void database::register_string_to_qvariant_conversion(const std::string &class_name, std::function<QVariant(const std::string &)> &&function)
{
	assert_throw(!this->string_to_qvariant_conversion_map.contains(class_name));

	this->string_to_qvariant_conversion_map[class_name] = std::move(function);
}

void database::register_list_property_function(const std::string &class_name, std::function<bool(QObject *object, const std::string &, const std::string &)> &&function)
{
	assert_throw(!this->list_property_function_map.contains(class_name));

	this->list_property_function_map[class_name] = std::move(function);
}

}
