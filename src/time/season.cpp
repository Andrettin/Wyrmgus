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
/**@name season.cpp - The season source file. */
//
//      (c) Copyright 2018 by Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "time/season.h"

#include "config.h"
#include "mod.h"
#include "video.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CSeason *> CSeason::Seasons;
std::map<std::string, CSeason *> CSeason::SeasonsByIdent;
	
/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a season
**
**	@param	ident			The season's string identifier
**	@param	should_find		Whether it is an error if the season could not be found; this is true by default
**
**	@return	The season if found, or null otherwise
*/
CSeason *CSeason::GetSeason(const std::string &ident, const bool should_find)
{
	std::map<std::string, CSeason *>::const_iterator find_iterator = SeasonsByIdent.find(ident);
	
	if (find_iterator != SeasonsByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid season: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a season
**
**	@param	ident	The season's string identifier
**
**	@return	The season if found, or a newly-created one otherwise
*/
CSeason *CSeason::GetOrAddSeason(const std::string &ident)
{
	CSeason *season = GetSeason(ident, false);
	
	if (!season) {
		season = new CSeason;
		season->Ident = ident;
		Seasons.push_back(season);
		SeasonsByIdent[ident] = season;
	}
	
	return season;
}

/**
**	@brief	Remove the existing seasons
*/
void CSeason::ClearSeasons()
{
	for (size_t i = 0; i < Seasons.size(); ++i) {
		delete Seasons[i];
	}
	Seasons.clear();
	SeasonsByIdent.clear();
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CSeason::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else {
			fprintf(stderr, "Invalid season property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "image") {
			std::string file;
			Vec2i size(0, 0);
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "file") {
					file = CMod::GetCurrentModPath() + value;
				} else if (key == "width") {
					size.x = std::stoi(value);
				} else if (key == "height") {
					size.y = std::stoi(value);
				} else {
					fprintf(stderr, "Invalid image property: \"%s\".\n", key.c_str());
				}
			}
			
			if (file.empty()) {
				fprintf(stderr, "Image has no file.\n");
				continue;
			}
			
			if (size.x == 0) {
				fprintf(stderr, "Image has no width.\n");
				continue;
			}
			
			if (size.y == 0) {
				fprintf(stderr, "Image has no height.\n");
				continue;
			}
			
			this->G = CGraphic::New(file, size.x, size.y);
			this->G->Load();
			this->G->UseDisplayFormat();
		} else {
			fprintf(stderr, "Invalid season property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}
