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
//      (c) Copyright 2020-2021 by Andrettin
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

#include "database/data_entry.h"

#include "database/data_entry_history.h"
#include "database/database.h"
#include "game.h"
#include "quest/campaign.h"
#include "time/calendar.h"
#include "time/timeline.h"
#include "util/string_conversion_util.h"

namespace wyrmgus {

data_entry::data_entry(const std::string &identifier) : identifier(identifier)
{
}

data_entry::~data_entry()
{
}

void data_entry::process_sml_property(const sml_property &property)
{
	if (property.get_key() == "aliases") {
		return; //alias addition is already handled in the data type class
	}

	database::process_sml_property_for_object(this, property);
}

void data_entry::process_sml_scope(const sml_data &scope)
{
	if (scope.get_tag() == "history") {
		this->history_data.push_back(scope);
	} else {
		database::process_sml_scope_for_object(this, scope);
	}
}

void data_entry::process_sml_dated_property(const sml_property &property, const QDateTime &date)
{
	Q_UNUSED(date)

	try {
		this->get_history_base()->process_sml_property(property);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error processing history property \"" + property.get_key() + "\"."));
	}
}

void data_entry::process_sml_dated_scope(const sml_data &scope, const QDateTime &date)
{
	Q_UNUSED(date)

	try {
		this->get_history_base()->process_sml_scope(scope);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error processing history scope \"" + scope.get_tag() + "\"."));
	}
}

void data_entry::load_history()
{
	this->reset_history();

	std::map<QDateTime, std::vector<const sml_data *>> history_entries;

	for (const sml_data &data : this->history_data) {
		data.for_each_property([&](const sml_property &property) {
			this->process_sml_dated_property(property, QDateTime()); //properties outside of a date scope, to be applied regardless of start date
		});

		const campaign *current_campaign = game::get()->get_current_campaign();
		const timeline *current_timeline = current_campaign->get_timeline();

		data.for_each_child([&](const sml_data &history_entry) {
			const timeline *timeline = nullptr;
			const calendar *calendar = nullptr;

			if (!std::isdigit(history_entry.get_tag().front()) && history_entry.get_tag().front() != '-') {
				timeline = timeline::try_get(history_entry.get_tag());

				if (timeline == nullptr) {
					calendar = calendar::try_get(history_entry.get_tag());
				}

				if (calendar == nullptr) {
					//treat the scope as a property to be applied immediately
					this->process_sml_dated_scope(history_entry, QDateTime());
					return;
				}
			}

			if (timeline != nullptr) {
				if (current_timeline == nullptr || (current_timeline != timeline && !current_timeline->derives_from_timeline(timeline))) {
					return;
				}

				history_entry.for_each_child([&](const sml_data &timeline_entry) {
					QDateTime date = string::to_date(timeline_entry.get_tag());
					if (current_campaign->contains_timeline_date(timeline, date)) {
						history_entries[date].push_back(&timeline_entry);
					}
				});
			} else if (calendar != nullptr) {
				history_entry.for_each_child([&](const sml_data &calendar_entry) {
					QDateTime date = string::to_date(calendar_entry.get_tag());
					date = date.addYears(calendar->get_year_offset());

					if (current_campaign->contains_timeline_date(nullptr, date)) {
						history_entries[date].push_back(&calendar_entry);
					}
				});
			} else {
				QDateTime date = string::to_date(history_entry.get_tag());

				if (current_campaign->contains_timeline_date(nullptr, date)) {
					history_entries[date].push_back(&history_entry);
				}
			}
		});
	}

	for (const auto &kv_pair : history_entries) {
		const QDateTime &date = kv_pair.first;
		const std::vector<const sml_data *> &date_history_entries = kv_pair.second;

		for (const sml_data *history_entry : date_history_entries) {
			this->load_date_scope(*history_entry, date);
		}
	}
}

void data_entry::load_date_scope(const sml_data &date_scope, const QDateTime &date)
{
	date_scope.for_each_element([&](const sml_property &property) {
		this->process_sml_dated_property(property, date);
	}, [&](const sml_data &scope) {
		this->process_sml_dated_scope(scope, date);
	});
}

}
