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

#pragma once

#include "util/qunique_ptr.h"
#include "util/singleton.h"

class CEditor;
class CPlayer;

namespace wyrmgus {

class defines;
class dialogue_node_instance;
class game;
class interface_style;
class map_info;
class network_manager;
class parameters;
class preferences;
class season;
class time_of_day;

//interface for the engine, to be used in the context of QML
class engine_interface final : public QObject, public singleton<engine_interface>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::parameters* parameters READ get_parameters CONSTANT)
	Q_PROPERTY(wyrmgus::defines* defines READ get_defines CONSTANT)
	Q_PROPERTY(wyrmgus::preferences* preferences READ get_preferences CONSTANT)
	Q_PROPERTY(wyrmgus::game* game READ get_game CONSTANT)
	Q_PROPERTY(CEditor* map_editor READ get_map_editor CONSTANT)
	Q_PROPERTY(wyrmgus::network_manager* network_manager READ get_network_manager CONSTANT)
	Q_PROPERTY(bool running READ is_running NOTIFY running_changed)
	Q_PROPERTY(double scale_factor READ get_scale_factor NOTIFY scale_factor_changed)
	Q_PROPERTY(QString save_path READ get_save_path CONSTANT)
	Q_PROPERTY(QString user_maps_path READ get_user_maps_path CONSTANT)
	Q_PROPERTY(QString loading_message READ get_loading_message NOTIFY loading_message_changed)
	Q_PROPERTY(QVariantList custom_heroes READ get_custom_heroes NOTIFY custom_heroes_changed)
	Q_PROPERTY(int max_map_width READ get_max_map_width CONSTANT)
	Q_PROPERTY(int max_map_height READ get_max_map_height CONSTANT)
	Q_PROPERTY(CPlayer* this_player READ get_this_player NOTIFY this_player_changed)
	Q_PROPERTY(QVariantList non_neutral_players READ get_non_neutral_players CONSTANT)
	Q_PROPERTY(QVariantList main_resources READ get_main_resources CONSTANT)
	Q_PROPERTY(wyrmgus::interface_style* current_interface_style READ get_current_interface_style NOTIFY current_interface_style_changed)
	Q_PROPERTY(wyrmgus::time_of_day* current_time_of_day READ get_current_time_of_day NOTIFY current_time_of_day_changed)
	Q_PROPERTY(wyrmgus::season* current_season READ get_current_season NOTIFY current_season_changed)
	Q_PROPERTY(QPoint map_view_top_left_pixel_pos READ get_map_view_top_left_pixel_pos NOTIFY map_view_top_left_pixel_pos_changed)
	Q_PROPERTY(wyrmgus::map_info* map_info READ get_map_info NOTIFY map_info_changed)
	Q_PROPERTY(bool modal_dialog_open READ is_modal_dialog_open WRITE set_modal_dialog_open_async)
	Q_PROPERTY(bool lua_dialog_open READ is_lua_dialog_open NOTIFY lua_dialog_open_changed)

public:
	engine_interface();
	~engine_interface();

	parameters *get_parameters() const;
	defines *get_defines() const;
	preferences *get_preferences() const;
	game *get_game() const;
	CEditor *get_map_editor() const;
	network_manager *get_network_manager() const;

	bool is_running() const
	{
		return this->running;
	}

	void set_running(const bool running)
	{
		if (running == this->is_running()) {
			return;
		}

		this->running = running;

		emit running_changed();
	}

	double get_scale_factor() const;

	QString get_save_path() const;
	QString get_user_maps_path() const;

	const QString &get_loading_message() const
	{
		return this->loading_message;
	}

	void set_loading_message(const QString &loading_message)
	{
		if (loading_message == this->loading_message) {
			return;
		}

		this->loading_message = loading_message;

		emit loading_message_changed();
	}

	Q_INVOKABLE void call_lua_command(const QString &command);
	Q_INVOKABLE void play_sound(const QString &sound_identifier);
	Q_INVOKABLE void play_music(const QString &type_str);

	Q_INVOKABLE void exit();

	std::future<void> get_map_view_created_future()
	{
		return this->map_view_created_promise.get_future();
	}

	Q_INVOKABLE void on_map_view_created()
	{
		this->map_view_created_promise.set_value();
	}

	void reset_map_view_created_promise()
	{
		this->map_view_created_promise = std::promise<void>();
	}

	bool is_waiting_for_interface() const
	{
		return this->waiting_for_interface;
	}

	void set_waiting_for_interface(const bool value)
	{
		this->waiting_for_interface = value;
	}

	Q_INVOKABLE void install_event_filter_on(QObject *object)
	{
		object->installEventFilter(this);
	}

	virtual bool eventFilter(QObject *watched, QEvent *event) override;

	std::queue<std::unique_ptr<QInputEvent>> take_stored_input_events()
	{
		return std::move(this->stored_input_events);
	}

	void store_input_event(std::unique_ptr<QInputEvent> &&event)
	{
		this->stored_input_events.push(std::move(event));
	}

	Q_INVOKABLE QVariantList get_difficulties() const;
	Q_INVOKABLE QString get_difficulty_name(const int difficulty_index) const;

	Q_INVOKABLE QVariantList get_hotkey_setups() const;
	Q_INVOKABLE QString get_hotkey_setup_name(const int hotkey_setup_index) const;

	Q_INVOKABLE QVariantList get_visible_campaigns() const;
	Q_INVOKABLE QVariantList get_playable_civilizations() const;

	void load_map_info(const std::filesystem::path &filepath);
	Q_INVOKABLE void load_map_info(const QUrl &file_url);
	Q_INVOKABLE void load_map_infos();
	Q_INVOKABLE void clear_map_infos();
	Q_INVOKABLE QStringList get_map_worlds() const;
	Q_INVOKABLE QVariantList get_map_infos(const QString &world = "") const;

	Q_INVOKABLE QVariantList get_achievements() const;
	Q_INVOKABLE QVariantList get_legacy_quests() const;

	QVariantList get_custom_heroes() const;
	Q_INVOKABLE void create_custom_hero(const QString &name, const QString &surname, const QVariant &civilization, const QVariant &unit_type, const QVariant &trait, const QString &variation_identifier);
	Q_INVOKABLE void delete_custom_hero(const QVariant &hero);
	Q_INVOKABLE bool is_name_valid_for_custom_hero(const QString &name) const;

	Q_INVOKABLE QVariantList get_building_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_character_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_civilization_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_deity_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_dynasty_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_faction_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_game_concept_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_item_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_literary_text_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_magic_prefix_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_magic_suffix_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_technology_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_unique_item_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_unit_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_world_encyclopedia_entries() const;

	Q_INVOKABLE QObject *get_link_target(const QString &link_str) const;

	int get_max_map_width() const;
	int get_max_map_height() const;

	CPlayer *get_this_player() const;
	QVariantList get_non_neutral_players() const;

	QVariantList get_main_resources() const;

	interface_style *get_current_interface_style() const
	{
		return this->current_interface_style;
	}

	void set_current_interface_style(interface_style *interface_style);

	time_of_day *get_current_time_of_day() const
	{
		return const_cast<time_of_day *>(this->current_time_of_day);
	}

	void set_current_time_of_day(const time_of_day *time_of_day);
	void update_current_time_of_day();

	season *get_current_season() const
	{
		return const_cast<season *>(this->current_season);
	}

	void set_current_season(const season *season);
	void update_current_season();

	const QPoint &get_map_view_top_left_pixel_pos() const
	{
		return this->map_view_top_left_pixel_pos;
	}

	void set_map_view_top_left_pixel_pos(const QPoint &pixel_pos);
	void update_map_view_top_left_pixel_pos();

	map_info *get_map_info() const;

	bool is_modal_dialog_open() const
	{
		return this->modal_dialog_open;
	}

	void set_modal_dialog_open_async(const bool value);

	bool is_lua_dialog_open() const
	{
		return this->open_lua_dialog_count > 0;
	}

	void set_lua_dialog_open_count(const int count)
	{
		if (count == this->open_lua_dialog_count) {
			return;
		}

		const int old_count = this->open_lua_dialog_count;

		this->open_lua_dialog_count = count;

		if (count == 0 || old_count == 0) {
			emit lua_dialog_open_changed();
		}
	}

	void change_lua_dialog_open_count(const int change)
	{
		this->set_lua_dialog_open_count(this->open_lua_dialog_count + change);
	}

	Q_INVOKABLE void load_game(const QUrl &file_url);
	void load_game_deferred(const std::filesystem::path &filepath);

	Q_INVOKABLE void check_achievements();

	void add_dialogue_node_instance(qunique_ptr<dialogue_node_instance> &&dialogue_node_instance);
	void remove_dialogue_node_instance(dialogue_node_instance *dialogue_node_instance);

	Q_INVOKABLE void crop_image_frames(const QString &filepath, const QSize &src_frame_size, const QSize &min_size) const;

signals:
	void running_changed();
	void scale_factor_changed();
	void loading_message_changed();
	void custom_heroes_changed();
	void this_player_changed();
	void current_interface_style_changed();
	void current_time_of_day_changed();
	void current_season_changed();
	void map_view_top_left_pixel_pos_changed();
	void map_info_changed();
	void encyclopediaEntryOpened(QString link);
	void factionChoiceDialogOpened(const QVariantList &factions);
	void achievementUnlockedDialogOpened(QObject *achievement);
	void dialogue_node_called(QObject *dialogue_node_instance);
	void dialogue_node_closed(QObject *dialogue_node_instance);
	void questCompletedDialogOpened(QObject *quest, const QString &rewards_string);
	void questFailedDialogOpened(QObject *quest, const QString &failure_reason_string);
	void population_dialog_opened(QObject *settlement_game_data, const QVariantList &population_units);
	void lua_dialog_open_changed();

private:
	bool running = false;
	std::promise<void> map_view_created_promise;
	std::atomic<bool> waiting_for_interface = false;
	std::queue<std::unique_ptr<QInputEvent>> stored_input_events;
	QString loading_message; //the loading message to be displayed
	interface_style *current_interface_style = nullptr;
	const time_of_day *current_time_of_day = nullptr;
	const season *current_season = nullptr;
	QPoint map_view_top_left_pixel_pos;
	bool modal_dialog_open = false;
	int open_lua_dialog_count = 0;
	std::vector<qunique_ptr<map_info>> map_infos;
	std::vector<qunique_ptr<dialogue_node_instance>> dialogue_node_instances;
};

}
