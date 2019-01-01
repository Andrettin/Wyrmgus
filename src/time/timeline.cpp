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
/**@name timeline.cpp - The timeline source file. */
//
//      (c) Copyright 2018-2019 by Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "time/timeline.h"

#include "config.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CTimeline *> CTimeline::Timelines;
std::map<std::string, CTimeline *> CTimeline::TimelinesByIdent;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Get a timeline
*/
CTimeline *CTimeline::GetTimeline(const std::string &ident)
{
	if (TimelinesByIdent.find(ident) != TimelinesByIdent.end()) {
		return TimelinesByIdent.find(ident)->second;
	}
	
	return nullptr;
}

CTimeline *CTimeline::GetOrAddTimeline(const std::string &ident)
{
	CTimeline *timeline = GetTimeline(ident);
	
	if (!timeline) {
		timeline = new CTimeline;
		timeline->Ident = ident;
		timeline->ID = Timelines.size();
		Timelines.push_back(timeline);
		TimelinesByIdent[ident] = timeline;
	}
	
	return timeline;
}

void CTimeline::ClearTimelines()
{
	for (size_t i = 0; i < Timelines.size(); ++i) {
		delete Timelines[i];
	}
	Timelines.clear();
}

void CTimeline::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "point_of_divergence") {
			this->PointOfDivergence = CDate::FromString(value);
		} else {
			fprintf(stderr, "Invalid timeline property: \"%s\".\n", key.c_str());
		}
	}
}

//@}
