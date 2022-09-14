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
//      (c) Copyright 2021-2022 by Andrettin
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

#include "engine_interface.h"

#include "character.h"
#include "database/defines.h"
#include "database/gsml_parser.h"
#include "database/preferences.h"
#include "dialogue_node_instance.h"
#include "editor.h"
#include "game/difficulty.h"
#include "game/game.h"
#include "game_concept.h"
#include "item/unique_item.h"
#include "language/word.h"
#include "literary_text.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/world.h"
#include "network/network_manager.h"
#include "player/civilization.h"
#include "player/dynasty.h"
#include "player/faction.h"
#include "player/player.h"
#include "quest/achievement.h"
#include "quest/campaign.h"
#include "quest/quest.h"
#include "religion/deity.h"
#include "parameters.h"
#include "results.h"
#include "script.h"
#include "sound/music_player.h"
#include "sound/music_type.h"
#include "sound/sound.h"
#include "ui/cursor.h"
#include "ui/cursor_type.h"
#include "ui/hotkey_setup.h"
#include "ui/interface.h"
#include "unit/unit_type.h"
#include "util/container_util.h"
#include "util/event_loop.h"
#include "util/exception_util.h"
#include "util/image_util.h"
#include "util/path_util.h"
#include "util/queue_util.h"
#include "util/qvariant_util.h"

#pragma warning(push, 0)
#include <QWindow>
#pragma warning(pop)

namespace wyrmgus {

engine_interface::engine_interface()
{
	connect(preferences::get(), &preferences::scale_factor_changed, this, &engine_interface::scale_factor_changed);
}

engine_interface::~engine_interface()
{
}

parameters *engine_interface::get_parameters() const
{
	return parameters::get();
}

defines *engine_interface::get_defines() const
{
	return defines::get();
}

preferences *engine_interface::get_preferences() const
{
	return preferences::get();
}

game *engine_interface::get_game() const
{
	return game::get();
}

CEditor *engine_interface::get_map_editor() const
{
	return CEditor::get();
}

network_manager *engine_interface::get_network_manager() const
{
	return network_manager::get();
}

double engine_interface::get_scale_factor() const
{
	return preferences::get()->get_scale_factor().to_double();
}

QString engine_interface::get_save_path() const
{
	std::filesystem::path save_path = database::get_save_path();
	save_path.make_preferred();
	return path::to_qstring(save_path);
}

QString engine_interface::get_shaders_path() const
{
	std::filesystem::path path = database::get()->get_shaders_path();
	path.make_preferred();
	return path::to_qstring(path);
}

QString engine_interface::get_user_maps_path() const
{
	std::filesystem::path path = database::get_user_maps_path();
	path.make_preferred();
	return path::to_qstring(path);
}

void engine_interface::call_lua_command(const QString &command)
{
	event_loop::get()->post([command]() {
		CclCommand(command.toStdString());
	});
}

void engine_interface::play_sound(const QString &sound_identifier)
{
	try {
		const sound *sound = sound::get(sound_identifier.toStdString());

		event_loop::get()->post([sound]() {
			PlayGameSound(sound, MaxSampleVolume);
		});
	} catch (const std::exception &exception) {
		exception::report(exception);
	}
}

void engine_interface::play_music(const QString &type_str)
{
	try {
		const music_type type = string_to_music_type(type_str.toStdString());

		event_loop::get()->post([type]() {
			music_player::get()->play_music_type(type);
		});
	} catch (const std::exception &exception) {
		exception::report(exception);
	}
}

void engine_interface::exit()
{
	event_loop::get()->post([]() {
		if (CEditor::get()->is_running()) {
			CEditor::get()->set_running(false);
			GameResult = GameExit;
		} else {
			StopGame(GameExit);
		}
	});
}

bool engine_interface::eventFilter(QObject *source, QEvent *event)
{
	Q_UNUSED(source)

	switch (event->type()) {
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease:
		case QEvent::MouseMove: {
			const QMouseEvent *mouse_event = static_cast<QMouseEvent *>(event);

			if (mouse_event->type() == QEvent::MouseMove && !this->cursor_restriction_rect.isNull() && !this->cursor_restriction_rect.contains(mouse_event->pos())) {
				const int clamped_x = std::clamp(mouse_event->pos().x(), this->cursor_restriction_rect.x(), this->cursor_restriction_rect.right());
				const int clamped_y = std::clamp(mouse_event->pos().y(), this->cursor_restriction_rect.y(), this->cursor_restriction_rect.bottom());
				const QPoint clamped_pos(clamped_x, clamped_y);

				this->set_cursor_pos(clamped_pos);
				return true;
			}

			this->store_input_event(std::make_unique<QMouseEvent>(*mouse_event));
			return true;
		}
		case QEvent::HoverEnter:
		case QEvent::HoverLeave:
		case QEvent::HoverMove: {
			const QHoverEvent *hover_event = static_cast<QHoverEvent *>(event);

			if (hover_event->type() == QEvent::HoverMove && !this->cursor_restriction_rect.isNull() && !this->cursor_restriction_rect.contains(hover_event->pos())) {
				const int clamped_x = std::clamp(hover_event->pos().x(), this->cursor_restriction_rect.x(), this->cursor_restriction_rect.right());
				const int clamped_y = std::clamp(hover_event->pos().y(), this->cursor_restriction_rect.y(), this->cursor_restriction_rect.bottom());
				const QPoint clamped_pos(clamped_x, clamped_y);

				this->set_cursor_pos(clamped_pos);
				return true;
			}

			this->store_input_event(std::make_unique<QHoverEvent>(*hover_event));
			return true;
		}
		case QEvent::KeyPress:
		case QEvent::KeyRelease: {
			const QKeyEvent *key_event = static_cast<QKeyEvent *>(event);
			this->store_input_event(std::make_unique<QKeyEvent>(*key_event));

			if (key_event->key() == Qt::Key_Tab || key_event->key() == Qt::Key_Backtab) {
				return true; //consume tab events to prevent tab focus switching
			}

			return false; //return false so that QML may also process the key event
		}
		default:
			return false;
	}
}

void engine_interface::set_cursor_pos(const QPoint &pos)
{
	const QWindowList windows = QApplication::topLevelWindows();

	if (windows.empty()) {
		return;
	}

	const QWindow *window = windows.at(0);

	const QPoint global_pos = window->mapToGlobal(pos);

	QCursor::setPos(global_pos);
}

QVariantList engine_interface::get_visible_campaigns() const
{
	return container::to_qvariant_list(campaign::get_all_visible());
}

QVariantList engine_interface::get_playable_civilizations() const
{
	std::vector<civilization *> playable_civilizations;

	for (civilization *civilization : civilization::get_all()) {
		if (!civilization->is_playable()) {
			continue;
		}

		playable_civilizations.push_back(civilization);
	}

	std::sort(playable_civilizations.begin(), playable_civilizations.end(), [](const civilization *lhs, const civilization *rhs) {
		return lhs->get_identifier() < rhs->get_identifier();
	});

	return container::to_qvariant_list(playable_civilizations);
}

void engine_interface::load_map_info(const std::filesystem::path &filepath)
{
	CMap::get()->get_info()->reset();

	if (filepath.extension() == ".wmp") {
		gsml_parser parser;
		database::process_gsml_data(CMap::get()->get_info(), parser.parse(filepath));
	} else {
		LuaLoadFile(path::to_string(filepath));
	}

	CMap::get()->get_info()->set_presentation_filepath(filepath);

	if (!CMap::get()->get_info()->is_hidden()) {
		this->map_infos.push_back(CMap::get()->get_info()->duplicate());
	}

	CMap::get()->get_info()->reset();
}

void engine_interface::load_map_info(const QUrl &file_url)
{
	this->clear_map_infos();
	this->load_map_info(path::from_qurl(file_url));
	CMap::get()->set_info(this->map_infos.front()->duplicate());
}

void engine_interface::load_map_infos()
{
	this->clear_map_infos();

	try {
		const std::vector<std::filesystem::path> map_paths = database::get()->get_maps_paths();

		for (const std::filesystem::path &map_path : map_paths) {
			if (!std::filesystem::exists(map_path)) {
				continue;
			}

			std::filesystem::recursive_directory_iterator dir_iterator(map_path);

			for (const std::filesystem::directory_entry &dir_entry : dir_iterator) {
				if (dir_entry.is_directory()) {
					if (dir_entry.path().filename() == "campaign" || dir_entry.path().filename() == "hidden") {
						dir_iterator.disable_recursion_pending();
						continue;
					}
				}

				if (!dir_entry.is_regular_file()) {
					continue;
				}

				if (dir_entry.path().extension() != ".smp" && dir_entry.path().extension() != ".wmp") {
					continue;
				}

				this->load_map_info(dir_entry.path());
			}
		}

		std::sort(this->map_infos.begin(), this->map_infos.end(), [](const qunique_ptr<map_info> &lhs, const qunique_ptr<map_info> &rhs) {
			if (lhs->get_name() != rhs->get_name()) {
				return lhs->get_name() < rhs->get_name();
			}

			return lhs->get_setup_filepath() < rhs->get_setup_filepath();
		});
	} catch (const std::exception &exception) {
		exception::report(exception);
	}
}

void engine_interface::clear_map_infos()
{
	this->map_infos.clear();
}

QStringList engine_interface::get_map_worlds() const
{
	std::set<std::string> map_worlds;

	for (const auto &map_info : this->map_infos) {
		map_worlds.insert(map_info->MapWorld);
	}

	QStringList map_world_qstring_list = container::to_qstring_list(map_worlds);

	std::sort(map_world_qstring_list.begin(), map_world_qstring_list.end(), [](const QString &lhs, const QString &rhs) {
		if (lhs == "Custom" || rhs == "Custom") {
			return lhs != "Custom";
		}

		if (lhs == "Random" || rhs == "Random") {
			return lhs != "Random";
		}

		return lhs < rhs;
	});

	return map_world_qstring_list;
}

QVariantList engine_interface::get_map_infos(const QString &world) const
{
	std::vector<map_info *> map_infos;

	const std::string world_str = world.toStdString();

	for (const auto &map_info : this->map_infos) {
		if (!world_str.empty() && map_info->MapWorld != world_str) {
			continue;
		}

		map_infos.push_back(map_info.get());
	}

	return container::to_qvariant_list(map_infos);
}

QVariantList engine_interface::get_difficulties() const
{
	QVariantList difficulties;

	for (int i = static_cast<int>(difficulty::none) + 1; i < static_cast<int>(difficulty::count); ++i) {
		difficulties.push_back(i);
	}

	return difficulties;
}

QString engine_interface::get_difficulty_name(const int difficulty_index) const
{
	const difficulty difficulty = static_cast<wyrmgus::difficulty>(difficulty_index);
	return QString::fromStdString(wyrmgus::get_difficulty_name(difficulty));
}

QVariantList engine_interface::get_hotkey_setups() const
{
	QVariantList hotkey_setups;

	for (int i = static_cast<int>(hotkey_setup::default_setup); i < static_cast<int>(hotkey_setup::count); ++i) {
		hotkey_setups.push_back(i);
	}

	return hotkey_setups;
}

QString engine_interface::get_hotkey_setup_name(const int hotkey_setup_index) const
{
	const hotkey_setup hotkey_setup = static_cast<wyrmgus::hotkey_setup>(hotkey_setup_index);
	return QString::fromStdString(wyrmgus::get_hotkey_setup_name(hotkey_setup));
}

QVariantList engine_interface::get_achievements() const
{
	std::vector<achievement *> achievements;

	for (achievement *achievement : achievement::get_all()) {
		if (achievement->is_hidden()) {
			continue;
		}

		achievements.push_back(achievement);
	}

	return container::to_qvariant_list(achievements);
}

QVariantList engine_interface::get_legacy_quests() const
{
	std::vector<quest *> quests;

	for (quest *quest : quest::get_all()) {
		if (quest->is_hidden()) {
			continue;
		}

		if (quest->World.empty()) {
			continue;
		}

		if (quest->Map.empty()) {
			continue;
		}

		quests.push_back(quest);
	}

	return container::to_qvariant_list(quests);
}

QVariantList engine_interface::get_custom_heroes() const
{
	return container::to_qvariant_list(character::get_custom_heroes());
}

void engine_interface::create_custom_hero(const QString &name, const QString &surname, const QVariant &civilization, const QVariant &unit_type, const QVariantList &trait_variants, const QString &variation_identifier)
{
	std::vector<const CUpgrade *> traits;

	for (const QVariant &trait_variant : trait_variants) {
		traits.push_back(qvariant::to_object<CUpgrade>(trait_variant));
	}

	character::create_custom_hero(name.toStdString(), surname.toStdString(), qvariant::to_object<wyrmgus::civilization>(civilization), qvariant::to_object<wyrmgus::unit_type>(unit_type), traits, variation_identifier.toStdString());
}

void engine_interface::delete_custom_hero(const QVariant &hero)
{
	character::remove_custom_hero(qvariant::to_object<character>(hero));
}

bool engine_interface::is_name_valid_for_custom_hero(const QString &name) const
{
	return character::is_name_valid_for_custom_hero(name.toStdString());
}

QVariantList engine_interface::get_building_encyclopedia_entries() const
{
	return container::to_qvariant_list(unit_type::get_building_encyclopedia_entries());
}

QVariantList engine_interface::get_character_encyclopedia_entries() const
{
	return container::to_qvariant_list(character::get_encyclopedia_entries());
}

QVariantList engine_interface::get_civilization_encyclopedia_entries() const
{
	return container::to_qvariant_list(civilization::get_encyclopedia_entries());
}

QVariantList engine_interface::get_deity_encyclopedia_entries() const
{
	return container::to_qvariant_list(deity::get_encyclopedia_entries());
}

QVariantList engine_interface::get_dynasty_encyclopedia_entries() const
{
	return container::to_qvariant_list(dynasty::get_encyclopedia_entries());
}

QVariantList engine_interface::get_faction_encyclopedia_entries() const
{
	return container::to_qvariant_list(faction::get_encyclopedia_entries());
}

QVariantList engine_interface::get_game_concept_encyclopedia_entries() const
{
	return container::to_qvariant_list(game_concept::get_encyclopedia_entries());
}

QVariantList engine_interface::get_item_encyclopedia_entries() const
{
	return container::to_qvariant_list(unit_type::get_item_encyclopedia_entries());
}

QVariantList engine_interface::get_literary_text_encyclopedia_entries() const
{
	return container::to_qvariant_list(literary_text::get_encyclopedia_entries());
}

QVariantList engine_interface::get_magic_prefix_encyclopedia_entries() const
{
	return container::to_qvariant_list(CUpgrade::get_magic_prefix_encyclopedia_entries());
}

QVariantList engine_interface::get_magic_suffix_encyclopedia_entries() const
{
	return container::to_qvariant_list(CUpgrade::get_magic_suffix_encyclopedia_entries());
}

QVariantList engine_interface::get_technology_encyclopedia_entries() const
{
	return container::to_qvariant_list(CUpgrade::get_technology_encyclopedia_entries());
}

QVariantList engine_interface::get_unique_item_encyclopedia_entries() const
{
	return container::to_qvariant_list(unique_item::get_encyclopedia_entries());
}

QVariantList engine_interface::get_unit_encyclopedia_entries() const
{
	return container::to_qvariant_list(unit_type::get_unit_encyclopedia_entries());
}

QVariantList engine_interface::get_world_encyclopedia_entries() const
{
	return container::to_qvariant_list(world::get_encyclopedia_entries());
}

QObject *engine_interface::get_link_target(const QString &link_str) const
{
	try {
		const QStringList link_str_list = link_str.split(':');

		if (link_str_list.size() != 2) {
			throw std::runtime_error("Invalid link string: \"" + link_str.toStdString() + "\".");
		}

		const std::string link_type = link_str_list.at(0).toStdString();
		const std::string link_target = link_str_list.at(1).toStdString();

		QObject *object = nullptr;

		if (link_type == "character") {
			object = character::get(link_target);
		} else if (link_type == "civilization") {
			object = civilization::get(link_target);
		} else if (link_type == "deity") {
			object = deity::get(link_target);
		} else if (link_type == "faction") {
			object = faction::get(link_target);
		} else if (link_type == "game_concept") {
			object = game_concept::get(link_target);
		} else if (link_type == "literary_text") {
			object = literary_text::get(link_target);
		} else if (link_type == "unique_item") {
			object = unique_item::get(link_target);
		} else if (link_type == "unit_type") {
			object = unit_type::get(link_target);
		} else if (link_type == "upgrade") {
			object = CUpgrade::get(link_target);
		} else if (link_type == "word") {
			object = word::get(link_target);
		} else if (link_type == "world") {
			object = world::get(link_target);
		} else {
			throw std::runtime_error("Invalid link type: \"" + link_type + "\".");
		}

		return object;
	} catch (const std::exception &exception) {
		exception::report(exception);
		return nullptr;
	}
}

int engine_interface::get_max_map_width() const
{
	return MaxMapWidth;
}

int engine_interface::get_max_map_height() const
{
	return MaxMapHeight;
}

CPlayer *engine_interface::get_this_player() const
{
	return CPlayer::GetThisPlayer();
}

QVariantList engine_interface::get_non_neutral_players() const
{
	return container::to_qvariant_list(CPlayer::get_non_neutral_players());
}

QVariantList engine_interface::get_main_resources() const
{
	return container::to_qvariant_list(resource::get_main_resources());
}

void engine_interface::set_current_interface_style(interface_style *interface_style)
{
	if (interface_style == nullptr) {
		this->set_current_interface_style(defines::get()->get_default_interface_style());
		return;
	}

	if (interface_style == this->get_current_interface_style()) {
		return;
	}

	this->current_interface_style = interface_style;

	emit current_interface_style_changed();
}

void engine_interface::set_current_time_of_day(const time_of_day *time_of_day)
{
	if (time_of_day == this->current_time_of_day) {
		return;
	}

	this->current_time_of_day = time_of_day;

	emit current_time_of_day_changed();
}

void engine_interface::update_current_time_of_day()
{
	if (UI.SelectedViewport == nullptr) {
		this->set_current_time_of_day(nullptr);
		return;
	}

	this->set_current_time_of_day(UI.SelectedViewport->get_center_tile_time_of_day());
}

void engine_interface::set_current_season(const season *season)
{
	if (season == this->current_season) {
		return;
	}

	this->current_season = season;

	emit current_season_changed();
}

void engine_interface::update_current_season()
{
	if (UI.SelectedViewport == nullptr) {
		this->set_current_season(nullptr);
		return;
	}

	this->set_current_season(UI.SelectedViewport->get_center_tile_season());
}

void engine_interface::set_map_view_top_left_pixel_pos(const QPoint &pixel_pos)
{
	if (pixel_pos == this->get_map_view_top_left_pixel_pos()) {
		return;
	}

	this->map_view_top_left_pixel_pos = pixel_pos;

	emit map_view_top_left_pixel_pos_changed();
}

void engine_interface::update_map_view_top_left_pixel_pos()
{
	if (UI.SelectedViewport == nullptr) {
		this->set_map_view_top_left_pixel_pos(QPoint(0, 0));
		return;
	}

	this->set_map_view_top_left_pixel_pos(UI.SelectedViewport->get_scaled_map_top_left_pixel_pos());
}

map_info *engine_interface::get_map_info() const
{
	return CMap::get()->get_info();
}

void engine_interface::set_modal_dialog_open_async(const bool value)
{
	event_loop::get()->post([this, value]() {
		this->set_cursor_restriction_rect(QRect());
		cursor::set_current_cursor(UI.get_cursor(cursor_type::point), true);
		this->modal_dialog_open = value;
	});
}

void engine_interface::load_game(const QUrl &file_url)
{
	this->load_game_deferred(path::from_qurl(file_url));
}

void engine_interface::load_game_deferred(const std::filesystem::path &filepath)
{
	event_loop::get()->co_spawn([filepath]() -> boost::asio::awaitable<void> {
		co_await ::load_game(filepath);
	});
}

void engine_interface::check_achievements()
{
	event_loop::get()->post([]() {
		achievement::check_achievements();
	});
}

void engine_interface::add_dialogue_node_instance(qunique_ptr<dialogue_node_instance> &&dialogue_node_instance)
{
	wyrmgus::dialogue_node_instance *dialogue_node_instance_ptr = dialogue_node_instance.get();
	this->dialogue_node_instances.push_back(std::move(dialogue_node_instance));
	emit dialogue_node_called(dialogue_node_instance_ptr);
}

void engine_interface::remove_dialogue_node_instance(dialogue_node_instance *dialogue_node_instance)
{
	emit dialogue_node_closed(dialogue_node_instance);
	std::erase_if(this->dialogue_node_instances, [dialogue_node_instance](const qunique_ptr<wyrmgus::dialogue_node_instance> &element) {
		return element.get() == dialogue_node_instance;
	});
}

void engine_interface::crop_image_frames(const QString &filepath, const QSize &src_frame_size, const QSize &min_size) const
{
	try {
		QImage image(filepath);
		image = image::crop_frames(image, src_frame_size, min_size);
		image.save(filepath);
	} catch (const std::exception &exception) {
		exception::report(exception);
	}
}

}
