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
/**@name terrain_type.cpp - The terrain type source file. */
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "map/terrain_type.h"

#include "config.h"
#include "config_operator.h"
#include "iolib.h"
#include "map/map.h"
#include "map/tileset.h"
#include "time/season.h"
#include "upgrade/upgrade_structs.h"
#include "video/video.h"

#include <algorithm>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::map<std::string, CTerrainType *> CTerrainType::TerrainTypesByCharacter;
std::map<std::tuple<int, int, int>, CTerrainType *> CTerrainType::TerrainTypesByColor;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Load the graphics of the terrain types
*/
void CTerrainType::LoadTerrainTypeGraphics()
{
	for (CTerrainType *terrain_type : CTerrainType::GetAll()) {
		if (terrain_type->Graphics) {
			terrain_type->Graphics->Load();
		}
		for (std::map<const CSeason *, CGraphic *>::iterator sub_it = terrain_type->SeasonGraphics.begin(); sub_it != terrain_type->SeasonGraphics.end(); ++sub_it) {
			sub_it->second->Load();
		}
		if (terrain_type->ElevationGraphics) {
			terrain_type->ElevationGraphics->Load();
		}
		if (terrain_type->PlayerColorGraphics) {
			terrain_type->PlayerColorGraphics->Load();
		}
	}
}

/**
**	@brief	Remove the existing terrain types
*/
void CTerrainType::Clear()
{
	CTerrainType::TerrainTypesByCharacter.clear();
	CTerrainType::TerrainTypesByColor.clear();
	
	DataType<CTerrainType>::Clear();
}

/**
**	@brief	Get a terrain flag by name
**
**	@param	flag_name	The name of the terrain flag
**
**	@return	The terrain flag if it exists, or 0 otherwise
*/
uint16_t CTerrainType::GetTerrainFlagByName(const std::string &flag_name)
{
	if (flag_name == "land") {
		return MapFieldLandAllowed;
	} else if (flag_name == "coast") {
		return MapFieldCoastAllowed;
	} else if (flag_name == "water") {
		return MapFieldWaterAllowed;
	} else if (flag_name == "no-building") {
		return MapFieldNoBuilding;
	} else if (flag_name == "unpassable") {
		return MapFieldUnpassable;
	} else if (flag_name == "wall") {
		return MapFieldWall;
	} else if (flag_name == "air-unpassable") {
		return MapFieldAirUnpassable;
	} else if (flag_name == "railroad") {
		return MapFieldRailroad;
	} else if (flag_name == "road") {
		return MapFieldRoad;
	} else if (flag_name == "no-rail") {
		return MapFieldNoRail;
	} else {
		fprintf(stderr, "Flag \"%s\" doesn't exist.\n", flag_name.c_str());
		return 0;
	}
}

/**
**	@brief	Destructor
*/
CTerrainType::~CTerrainType()
{
	if (this->Graphics) {
		CGraphic::Free(this->Graphics);
	}
	for (std::map<const CSeason *, CGraphic *>::iterator iterator = this->SeasonGraphics.begin(); iterator != this->SeasonGraphics.end(); ++iterator) {
		CGraphic::Free(iterator->second);
	}
	if (this->ElevationGraphics) {
		CGraphic::Free(this->ElevationGraphics);
	}
	if (this->PlayerColorGraphics) {
		CPlayerColorGraphic::Free(this->PlayerColorGraphics);
	}
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CTerrainType::ProcessConfigData(const CConfigData *config_data)
{
	std::string graphics_file;
	std::string elevation_graphics_file;
	std::string player_color_graphics_file;
	
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
			continue;
		}
		
		if (property.Key == "name") {
			this->Name = property.Value;
		} else if (property.Key == "character") {
			this->Character = property.Value;
			
			if (CTerrainType::TerrainTypesByCharacter.find(this->Character) != CTerrainType::TerrainTypesByCharacter.end()) {
				fprintf(stderr, "Character \"%s\" is already used by another terrain type.\n", this->Character.c_str());
				continue;
			} else {
				CTerrainType::TerrainTypesByCharacter[this->Character] = this;
			}
		} else if (property.Key == "color") {
			this->Color = CColor::FromString(property.Value);
			
			if (CTerrainType::TerrainTypesByColor.find(std::tuple<int, int, int>(this->Color.R, this->Color.G, this->Color.B)) != CTerrainType::TerrainTypesByColor.end()) {
				fprintf(stderr, "Color is already used by another terrain type.\n");
				continue;
			}
			if (TerrainFeatureColorToIndex.find(std::tuple<int, int, int>(this->Color.R, this->Color.G, this->Color.B)) != TerrainFeatureColorToIndex.end()) {
				fprintf(stderr, "Color is already used by a terrain feature.\n");
				continue;
			}
			CTerrainType::TerrainTypesByColor[std::tuple<int, int, int>(this->Color.R, this->Color.G, this->Color.B)] = this;
		} else if (property.Key == "overlay") {
			this->Overlay = StringToBool(property.Value);
		} else if (property.Key == "buildable") {
			this->Buildable = StringToBool(property.Value);
		} else if (property.Key == "allow_single") {
			this->AllowSingle = StringToBool(property.Value);
		} else if (property.Key == "hidden") {
			this->Hidden = StringToBool(property.Value);
		} else if (property.Key == "resource") {
			std::string value = FindAndReplaceString(property.Value, "_", "-");
			this->Resource = GetResourceIdByName(value.c_str());
		} else if (property.Key == "flag") {
			std::string value = FindAndReplaceString(property.Value, "_", "-");
			uint16_t flag = CTerrainType::GetTerrainFlagByName(value);
			if (flag) {
				this->Flags |= flag;
			}
		} else if (property.Key == "graphics") {
			graphics_file = property.Value;
			if (!CanAccessFile(graphics_file.c_str())) {
				fprintf(stderr, "File \"%s\" doesn't exist.\n", property.Value.c_str());
			}
		} else if (property.Key == "elevation_graphics") {
			elevation_graphics_file = property.Value;
			if (!CanAccessFile(elevation_graphics_file.c_str())) {
				fprintf(stderr, "File \"%s\" doesn't exist.\n", property.Value.c_str());
			}
		} else if (property.Key == "player_color_graphics") {
			player_color_graphics_file = property.Value;
			if (!CanAccessFile(player_color_graphics_file.c_str())) {
				fprintf(stderr, "File \"%s\" doesn't exist.\n", property.Value.c_str());
			}
		} else if (property.Key == "pixel_width") {
			this->PixelTileSize.x = std::stoi(property.Value);
		} else if (property.Key == "pixel_height") {
			this->PixelTileSize.y = std::stoi(property.Value);
		} else if (property.Key == "base_terrain_type") {
			CTerrainType *base_terrain_type = CTerrainType::Get(property.Value);
			this->BaseTerrainTypes.push_back(base_terrain_type);
		} else if (property.Key == "inner_border_terrain_type") {
			CTerrainType *border_terrain_type = CTerrainType::Get(property.Value);
			this->InnerBorderTerrains.push_back(border_terrain_type);
			this->BorderTerrains.push_back(border_terrain_type);
			border_terrain_type->OuterBorderTerrains.push_back(this);
			border_terrain_type->BorderTerrains.push_back(this);
		} else if (property.Key == "outer_border_terrain_type") {
			CTerrainType *border_terrain_type = CTerrainType::Get(property.Value);
			this->OuterBorderTerrains.push_back(border_terrain_type);
			this->BorderTerrains.push_back(border_terrain_type);
			border_terrain_type->InnerBorderTerrains.push_back(this);
			border_terrain_type->BorderTerrains.push_back(this);
		} else if (property.Key == "solid_tile") {
			this->SolidTiles.push_back(std::stoi(property.Value));
		} else if (property.Key == "damaged_tile") {
			this->DamagedTiles.push_back(std::stoi(property.Value));
		} else if (property.Key == "destroyed_tile") {
			this->DestroyedTiles.push_back(std::stoi(property.Value));
		} else {
			fprintf(stderr, "Invalid terrain type property: \"%s\".\n", property.Key.c_str());
		}
	}
	
	for (const CConfigData *section : config_data->Sections) {
		if (section->Tag == "season_graphics") {
			std::string season_graphics_file;
			CSeason *season = nullptr;
			
			for (const CConfigProperty &property : section->Properties) {
				if (property.Operator != CConfigOperator::Assignment) {
					fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
					continue;
				}
				
				if (property.Key == "season") {
					season = CSeason::Get(property.Value);
				} else if (property.Key == "graphics") {
					season_graphics_file = property.Value;
					if (!CanAccessFile(season_graphics_file.c_str())) {
						fprintf(stderr, "File \"%s\" doesn't exist.\n", season_graphics_file.c_str());
					}
				} else {
					fprintf(stderr, "Invalid season graphics property: \"%s\".\n", property.Key.c_str());
				}
			}
			
			if (season_graphics_file.empty()) {
				fprintf(stderr, "Season graphics have no file.\n");
				continue;
			}
			
			if (!season) {
				fprintf(stderr, "Season graphics have no season.\n");
				continue;
			}
			
			if (CGraphic::Get(season_graphics_file) == nullptr) {
				CGraphic *graphics = CGraphic::New(season_graphics_file, this->PixelTileSize.x, this->PixelTileSize.y);
			}
			this->SeasonGraphics[season] = CGraphic::Get(season_graphics_file);
		} else if (section->Tag == "transition_tile" || section->Tag == "adjacent_transition_tile") {
			int transition_terrain_id = -1; //any terrain, by default
			int transition_type = -1;
			std::vector<int> tiles;
			
			for (const CConfigProperty &property : section->Properties) {
				if (property.Operator != CConfigOperator::Assignment) {
					fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
					continue;
				}
				
				if (property.Key == "terrain_type") {
					CTerrainType *transition_terrain = CTerrainType::Get(property.Value);
					if (transition_terrain != nullptr) {
						transition_terrain_id = transition_terrain->GetIndex();
					}
				} else if (property.Key == "transition_type") {
					std::string value = FindAndReplaceString(property.Value, "_", "-");
					transition_type = GetTransitionTypeIdByName(value);
				} else if (property.Key == "tile") {
					tiles.push_back(std::stoi(property.Value));
				} else {
					fprintf(stderr, "Invalid transition tile property: \"%s\".\n", property.Key.c_str());
				}
			}
			
			if (transition_type == -1) {
				fprintf(stderr, "Transition tile has no transition type.\n");
				continue;
			}
			
			for (size_t j = 0; j < tiles.size(); ++j) {
				if (section->Tag == "transition_tile") {
					this->TransitionTiles[std::tuple<int, int>(transition_terrain_id, transition_type)].push_back(tiles[j]);
				} else if (section->Tag == "adjacent_transition_tile") {
					this->AdjacentTransitionTiles[std::tuple<int, int>(transition_terrain_id, transition_type)].push_back(tiles[j]);
				}
			}
		} else {
			fprintf(stderr, "Invalid terrain type property: \"%s\".\n", section->Tag.c_str());
		}
	}
	
	//get the graphics here, so that we can take the pixel tile size into account
	if (!graphics_file.empty()) {
		if (CGraphic::Get(graphics_file) == nullptr) {
			CGraphic *graphics = CGraphic::New(graphics_file, this->PixelTileSize.x, this->PixelTileSize.y);
		}
		this->Graphics = CGraphic::Get(graphics_file);
	}
	if (!elevation_graphics_file.empty()) {
		if (CGraphic::Get(elevation_graphics_file) == nullptr) {
			CGraphic *graphics = CGraphic::New(elevation_graphics_file, this->PixelTileSize.x, this->PixelTileSize.y);
		}
		this->ElevationGraphics = CGraphic::Get(elevation_graphics_file);
	}
	if (!player_color_graphics_file.empty()) {
		if (CPlayerColorGraphic::Get(player_color_graphics_file) == nullptr) {
			CPlayerColorGraphic *graphics = CPlayerColorGraphic::New(player_color_graphics_file, this->PixelTileSize.x, this->PixelTileSize.y);
		}
		this->PlayerColorGraphics = CPlayerColorGraphic::Get(player_color_graphics_file);
	}
}

/**
**	@brief	Get the graphics for the terrain type
**
**	@param	season	The season for the graphics, if any
**
**	@return	The graphics
*/
CGraphic *CTerrainType::GetGraphics(const CSeason *season) const
{
	std::map<const CSeason *, CGraphic *>::const_iterator find_iterator = this->SeasonGraphics.find(season);
	
	if (find_iterator != this->SeasonGraphics.end()) {
		return find_iterator->second;
	} else {
		return this->Graphics;
	}
}

void CTerrainType::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_tree", "tree"), +[](CTerrainType *terrain_type, const bool tree){ terrain_type->Tree = tree; });
	ClassDB::bind_method(D_METHOD("is_tree"), &CTerrainType::IsTree);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "tree"), "set_tree", "is_tree");
	
	ClassDB::bind_method(D_METHOD("set_rock", "rock"), +[](CTerrainType *terrain_type, const bool rock){ terrain_type->Rock = rock; });
	ClassDB::bind_method(D_METHOD("is_rock"), &CTerrainType::IsRock);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "rock"), "set_rock", "is_rock");
	
	ClassDB::bind_method(D_METHOD("set_desert", "desert"), +[](CTerrainType *terrain_type, const bool desert){ terrain_type->Desert = desert; });
	ClassDB::bind_method(D_METHOD("is_desert"), &CTerrainType::IsDesert);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "desert"), "set_desert", "is_desert");
	
	ClassDB::bind_method(D_METHOD("set_swamp", "swamp"), +[](CTerrainType *terrain_type, const bool swamp){ terrain_type->Swamp = swamp; });
	ClassDB::bind_method(D_METHOD("is_swamp"), &CTerrainType::IsSwamp);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "swamp"), "set_swamp", "is_swamp");
}
