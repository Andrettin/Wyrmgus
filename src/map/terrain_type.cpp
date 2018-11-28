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
/**@name terrain_type.cpp - The terrain types. */
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "map/terrain_type.h"

#include "config.h"
#include "iolib.h"
#include "map/map.h"
#include "map/tileset.h"
#include "upgrade_structs.h"
#include "video.h"

#include <algorithm>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CTerrainType *> CTerrainType::TerrainTypes;
std::map<std::string, CTerrainType *> CTerrainType::TerrainTypesByIdent;
std::map<std::string, CTerrainType *> CTerrainType::TerrainTypesByCharacter;
std::map<std::tuple<int, int, int>, CTerrainType *> CTerrainType::TerrainTypesByColor;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a terrain type
**
**	@param	ident	The terrain type's string identifier
**
**	@return	The terrain type if found, null otherwise
*/
CTerrainType *CTerrainType::GetTerrainType(const std::string &ident, const bool should_find)
{
	if (TerrainTypesByIdent.find(ident) != TerrainTypesByIdent.end()) {
		return TerrainTypesByIdent.find(ident)->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid terrain type: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a terrain type
**
**	@param	ident	The terrain type's string identifier
**
**	@return	The terrain type if found, otherwise a new terrain type is created and returned
*/
CTerrainType *CTerrainType::GetOrAddTerrainType(const std::string &ident)
{
	CTerrainType *terrain_type = GetTerrainType(ident, false);
	
	if (!terrain_type) {
		terrain_type = new CTerrainType;
		terrain_type->Ident = ident;
		terrain_type->ID = CTerrainType::TerrainTypes.size();
		TerrainTypes.push_back(terrain_type);
		TerrainTypesByIdent[ident] = terrain_type;
	}
	
	return terrain_type;
}

/**
**	@brief	Load the graphics of the terrain types
*/
void CTerrainType::LoadTerrainTypeGraphics()
{
	for (std::vector<CTerrainType *>::iterator it = TerrainTypes.begin(); it != TerrainTypes.end(); ++it) {
		if ((*it)->Graphics) {
			(*it)->Graphics->Load();
		}
		if ((*it)->WinterGraphics) {
			(*it)->WinterGraphics->Load();
		}
		if ((*it)->ElevationGraphics) {
			(*it)->ElevationGraphics->Load();
		}
		if ((*it)->PlayerColorGraphics) {
			(*it)->PlayerColorGraphics->Load();
		}
	}
}

/**
**	@brief	Remove the existing terrain types
*/
void CTerrainType::ClearTerrainTypes()
{
	for (size_t i = 0; i < TerrainTypes.size(); ++i) {
		delete TerrainTypes[i];
	}
	TerrainTypes.clear();
	TerrainTypesByIdent.clear();
	TerrainTypesByCharacter.clear();
	TerrainTypesByColor.clear();
}

/**
**	@brief	Destructor
*/
CTerrainType::~CTerrainType()
{
	if (this->Graphics) {
		CGraphic::Free(this->Graphics);
	}
	if (this->WinterGraphics) {
		CGraphic::Free(this->WinterGraphics);
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
	std::string winter_graphics_file;
	std::string elevation_graphics_file;
	std::string player_color_graphics_file;
	
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "character") {
			this->Character = value;
		} else if (key == "color") {
			this->Color = CColor::FromString(value);
			
			if (CTerrainType::TerrainTypesByColor.find(std::tuple<int, int, int>(this->Color.R, this->Color.G, this->Color.B)) != CTerrainType::TerrainTypesByColor.end()) {
				fprintf(stderr, "Color is already used by another terrain type.\n");
				continue;
			}
			if (TerrainFeatureColorToIndex.find(std::tuple<int, int, int>(this->Color.R, this->Color.G, this->Color.B)) != TerrainFeatureColorToIndex.end()) {
				fprintf(stderr, "Color is already used by a terrain feature.\n");
				continue;
			}
			CTerrainType::TerrainTypesByColor[std::tuple<int, int, int>(this->Color.R, this->Color.G, this->Color.B)] = this;
		} else if (key == "overlay") {
			this->Overlay = StringToBool(value);
		} else if (key == "buildable") {
			this->Buildable = StringToBool(value);
		} else if (key == "allow_single") {
			this->AllowSingle = StringToBool(value);
		} else if (key == "hidden") {
			this->Hidden = StringToBool(value);
		} else if (key == "resource") {
			value = FindAndReplaceString(value, "_", "-");
			this->Resource = GetResourceIdByName(value.c_str());
		} else if (key == "flag") {
			value = FindAndReplaceString(value, "_", "-");
			if (value == "land") {
				this->Flags |= MapFieldLandAllowed;
			} else if (value == "coast") {
				this->Flags |= MapFieldCoastAllowed;
			} else if (value == "water") {
				this->Flags |= MapFieldWaterAllowed;
			} else if (value == "no-building") {
				this->Flags |= MapFieldNoBuilding;
			} else if (value == "unpassable") {
				this->Flags |= MapFieldUnpassable;
			} else if (value == "wall") {
				this->Flags |= MapFieldWall;
			} else if (value == "rock") {
				this->Flags |= MapFieldRocks;
			} else if (value == "forest") {
				this->Flags |= MapFieldForest;
			} else if (value == "air-unpassable") {
				this->Flags |= MapFieldAirUnpassable;
			} else if (value == "desert") {
				this->Flags |= MapFieldDesert;
			} else if (value == "dirt") {
				this->Flags |= MapFieldDirt;
			} else if (value == "grass") {
				this->Flags |= MapFieldGrass;
			} else if (value == "gravel") {
				this->Flags |= MapFieldGravel;
			} else if (value == "mud") {
				this->Flags |= MapFieldMud;
			} else if (value == "railroad") {
				this->Flags |= MapFieldRailroad;
			} else if (value == "road") {
				this->Flags |= MapFieldRoad;
			} else if (value == "no-rail") {
				this->Flags |= MapFieldNoRail;
			} else if (value == "stone-floor") {
				this->Flags |= MapFieldStoneFloor;
			} else if (value == "stumps") {
				this->Flags |= MapFieldStumps;
			} else {
				fprintf(stderr, "Flag \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "graphics") {
			graphics_file = value;
			if (!CanAccessFile(graphics_file.c_str())) {
				fprintf(stderr, "File \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "winter_graphics") {
			winter_graphics_file = value;
			if (!CanAccessFile(winter_graphics_file.c_str())) {
				fprintf(stderr, "File \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "elevation_graphics") {
			elevation_graphics_file = value;
			if (!CanAccessFile(elevation_graphics_file.c_str())) {
				fprintf(stderr, "File \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "player_color_graphics") {
			player_color_graphics_file = value;
			if (!CanAccessFile(player_color_graphics_file.c_str())) {
				fprintf(stderr, "File \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "pixel_width") {
			this->PixelTileSize.x = std::stoi(value);
		} else if (key == "pixel_height") {
			this->PixelTileSize.y = std::stoi(value);
		} else if (key == "base_terrain_type") {
			value = FindAndReplaceString(value, "_", "-");
			CTerrainType *base_terrain_type = GetTerrainType(value);
			this->BaseTerrainTypes.push_back(base_terrain_type);
		} else if (key == "outer_border_terrain_type") {
			value = FindAndReplaceString(value, "_", "-");
			CTerrainType *border_terrain_type = GetTerrainType(value);
			this->OuterBorderTerrains.push_back(border_terrain_type);
			this->BorderTerrains.push_back(border_terrain_type);
			border_terrain_type->InnerBorderTerrains.push_back(this);
			border_terrain_type->BorderTerrains.push_back(this);
		} else if (key == "solid_tile") {
			this->SolidTiles.push_back(std::stoi(value));
		} else if (key == "damaged_tile") {
			this->DamagedTiles.push_back(std::stoi(value));
		} else if (key == "destroyed_tile") {
			this->DestroyedTiles.push_back(std::stoi(value));
		} else {
			fprintf(stderr, "Invalid terrain type property: \"%s\".\n", key.c_str());
		}
	}
	
	for (size_t i = 0; i < config_data->Children.size(); ++i) {
		CConfigData *child_config_data = config_data->Children[i];
		
		if (child_config_data->Tag == "transition_tile" || child_config_data->Tag == "adjacent_transition_tile") {
			int transition_terrain_id = -1; //any terrain, by default
			int transition_type = -1;
			std::vector<int> tiles;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "terrain_type") {
					value = FindAndReplaceString(value, "_", "-");
					CTerrainType *transition_terrain = CTerrainType::GetTerrainType(value);
					if (!transition_terrain) {
						fprintf(stderr, "Terrain type \"%s\" doesn't exist.\n", value.c_str());
					} else {
						transition_terrain_id = transition_terrain->ID;
					}
				} else if (key == "transition_type") {
					value = FindAndReplaceString(value, "_", "-");
					transition_type = GetTransitionTypeIdByName(value);
				} else if (key == "tile") {
					tiles.push_back(std::stoi(value));
				} else {
					fprintf(stderr, "Invalid transition tile property: \"%s\".\n", key.c_str());
				}
			}
			
			if (transition_type == -1) {
				fprintf(stderr, "Transition tile has no transition type.\n");
				continue;
			}
			
			for (size_t j = 0; j < tiles.size(); ++j) {
				if (child_config_data->Tag == "transition_tile") {
					this->TransitionTiles[std::tuple<int, int>(transition_terrain_id, transition_type)].push_back(tiles[j]);
				} else if (child_config_data->Tag == "adjacent_transition_tile") {
					this->AdjacentTransitionTiles[std::tuple<int, int>(transition_terrain_id, transition_type)].push_back(tiles[j]);
				}
			}
		} else {
			fprintf(stderr, "Invalid terrain type property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
	
	//get the graphics here, so that we can take the pixel tile size into account
	if (!graphics_file.empty()) {
		if (CGraphic::Get(graphics_file) == nullptr) {
			CGraphic *graphics = CGraphic::New(graphics_file, this->PixelTileSize.x, this->PixelTileSize.y);
		}
		this->Graphics = CGraphic::Get(graphics_file);
	}
	if (!winter_graphics_file.empty()) {
		if (CGraphic::Get(winter_graphics_file) == nullptr) {
			CGraphic *graphics = CGraphic::New(winter_graphics_file, this->PixelTileSize.x, this->PixelTileSize.y);
		}
		this->WinterGraphics = CGraphic::Get(winter_graphics_file);
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
**	@param	is_winter	Whether the graphics to be obtained are the winter ones (if available) or not
**
**	@return	The graphics
*/
CGraphic *CTerrainType::GetGraphics(const bool is_winter) const
{
	if (this->WinterGraphics && is_winter) {
		return this->WinterGraphics;
	} else {
		return this->Graphics;
	}
}

//@}
