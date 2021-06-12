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
//      (c) Copyright 2021 by Andrettin
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

namespace boost::asio {
	class io_context;
}

namespace wyrmgus {

class defines;
class game;
class map_info;
class parameters;
class preferences;

//interface for the engine, to be used in the context of QML
class engine_interface final : public QObject, public singleton<engine_interface>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::parameters* parameters READ get_parameters CONSTANT)
	Q_PROPERTY(wyrmgus::defines* defines READ get_defines CONSTANT)
	Q_PROPERTY(wyrmgus::preferences* preferences READ get_preferences CONSTANT)
	Q_PROPERTY(wyrmgus::game* game READ get_game CONSTANT)
	Q_PROPERTY(bool running READ is_running NOTIFY running_changed)
	Q_PROPERTY(QString save_path READ get_save_path CONSTANT)
	Q_PROPERTY(QString loading_message READ get_loading_message NOTIFY loading_message_changed)

public:
	engine_interface();
	~engine_interface();

	void post(const std::function<void()> &function);

	std::future<void> async(const std::function<void()> &function)
	{
		std::shared_ptr<std::promise<void>> promise = std::make_unique<std::promise<void>>();
		std::future<void> future = promise->get_future();

		this->post([promise, function]() {
			function();
			promise->set_value();
		});

		return future;
	}

	void sync(const std::function<void()> &function)
	{
		if (this->is_waiting_for_interface()) {
			function();
			return;
		}

		//post an action, and then wait for it to be completed
		std::future<void> future = this->async(function);
		future.wait();
	}

	void run_event_loop();

	parameters *get_parameters() const;
	defines *get_defines() const;
	preferences *get_preferences() const;
	game *get_game() const;

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

	QString get_save_path() const;

	const QString &get_loading_message() const
	{
		std::shared_lock<std::shared_mutex> lock(this->loading_message_mutex);
		return this->loading_message;
	}

	void set_loading_message(const QString &loading_message)
	{
		{
			std::unique_lock<std::shared_mutex> lock(this->loading_message_mutex);

			if (loading_message == this->loading_message) {
				return;
			}

			this->loading_message = loading_message;
		}

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
		std::lock_guard<std::mutex> lock(this->input_event_mutex);
		return std::move(this->stored_input_events);
	}

	void store_input_event(std::unique_ptr<QInputEvent> &&event)
	{
		std::lock_guard<std::mutex> lock(this->input_event_mutex);
		this->stored_input_events.push(std::move(event));
	}

	Q_INVOKABLE QVariantList get_difficulties() const;
	Q_INVOKABLE QString get_difficulty_name(const int difficulty_index) const;

	Q_INVOKABLE QVariantList get_visible_campaigns() const;
	Q_INVOKABLE QVariantList get_playable_civilizations() const;

	Q_INVOKABLE void load_map_infos();
	Q_INVOKABLE void clear_map_infos();
	Q_INVOKABLE QStringList get_map_worlds() const;
	Q_INVOKABLE QVariantList get_map_infos(const QString &world) const;

	Q_INVOKABLE QVariantList get_achievements() const;
	Q_INVOKABLE QVariantList get_legacy_quests() const;

	Q_INVOKABLE QVariantList get_custom_heroes() const;
	Q_INVOKABLE void create_custom_hero(const QString &name, const QString &surname, const QVariant &civilization, const QVariant &unit_type, const QVariant &trait, const QString &variation_identifier);

	Q_INVOKABLE QVariantList get_building_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_character_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_civilization_encyclopedia_entries() const;
	Q_INVOKABLE QVariantList get_deity_encyclopedia_entries() const;
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

signals:
	void running_changed();
	void loading_message_changed();
	void encyclopediaEntryOpened(QString link);

private:
	std::queue<std::function<void()>> posted_commands;
	std::mutex command_mutex;
	bool running = false;
	std::promise<void> map_view_created_promise;
	std::atomic<bool> waiting_for_interface = false;
	std::queue<std::unique_ptr<QInputEvent>> stored_input_events;
	std::mutex input_event_mutex;
	QString loading_message; //the loading message to be displayed
	mutable std::shared_mutex loading_message_mutex;
	std::vector<qunique_ptr<map_info>> map_infos;
};

}
