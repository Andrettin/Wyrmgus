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
//      (c) Copyright 2018-2022 by Andrettin
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

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace wyrmgus {

class timeline : public named_data_entry, public data_type<timeline>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::timeline* parent_timeline MEMBER parent_timeline READ get_parent_timeline)
	Q_PROPERTY(QDateTime point_of_divergence MEMBER point_of_divergence READ get_point_of_divergence)

public:
	static constexpr const char *class_identifier = "timeline";
	static constexpr const char *database_folder = "timelines";

	explicit timeline(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	timeline *get_parent_timeline() const
	{
		return this->parent_timeline;
	}

	const QDateTime &get_point_of_divergence() const
	{
		return this->point_of_divergence;
	}

	bool derives_from_timeline(const timeline *timeline) const
	{
		if (this->get_parent_timeline() == nullptr) {
			return false;
		}

		if (timeline == this->get_parent_timeline()) {
			return true;
		}

		return this->get_parent_timeline()->derives_from_timeline(timeline);
	}

	bool contains_timeline_date(const timeline *timeline, const QDateTime &date) const
	{
		if (timeline == this) {
			return true;
		} else if (this->get_parent_timeline() == nullptr && timeline != nullptr) {
			return false;
		}

		if (timeline == this->get_parent_timeline()) {
			return date < this->point_of_divergence;
		}

		return this->get_parent_timeline()->contains_timeline_date(timeline, date);
	}

private:
	timeline *parent_timeline = nullptr; //the timeline from which this one derives (null means the default timeline)
	QDateTime point_of_divergence; //the point of divergence from the parent timeline
};

}
