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
#include "video/palette_image.h"
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
		if (terrain_type->Graphics == nullptr) {
			if (terrain_type->GetImage() != nullptr) {
				CGraphic *graphics = CGraphic::Get(terrain_type->GetImage()->GetFile().utf8().get_data());
				if (graphics == nullptr) {
					graphics = CGraphic::New(terrain_type->GetImage()->GetFile().utf8().get_data(), CMap::PixelTileSize.x, CMap::PixelTileSize.y);
				}
				terrain_type->Graphics = graphics;
				terrain_type->Graphics->Load();
			}
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
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CTerrainType::ProcessConfigDataProperty(const String &key, String value)
{
	if (key == "character") {
		this->Character = value.utf8().get_data();
		
		if (CTerrainType::TerrainTypesByCharacter.find(this->Character) != CTerrainType::TerrainTypesByCharacter.end()) {
			fprintf(stderr, "Character \"%s\" is already used by another terrain type.\n", this->Character.c_str());
		} else {
			CTerrainType::TerrainTypesByCharacter[this->Character] = this;
		}
	} else if (key == "color") {
		this->Color = CColor::FromString(value.utf8().get_data());
		
		if (CTerrainType::TerrainTypesByColor.find(std::tuple<int, int, int>(this->Color.R, this->Color.G, this->Color.B)) != CTerrainType::TerrainTypesByColor.end()) {
			fprintf(stderr, "Color is already used by another terrain type.\n");
		} else if (TerrainFeatureColorToIndex.find(std::tuple<int, int, int>(this->Color.R, this->Color.G, this->Color.B)) != TerrainFeatureColorToIndex.end()) {
			fprintf(stderr, "Color is already used by a terrain feature.\n");
		} else {
			CTerrainType::TerrainTypesByColor[std::tuple<int, int, int>(this->Color.R, this->Color.G, this->Color.B)] = this;
		}
	} else if (key == "overlay") {
		this->Overlay = StringToBool(value);
	} else if (key == "buildable") {
		this->Buildable = StringToBool(value);
	} else if (key == "allow_single") {
		this->AllowSingle = StringToBool(value);
	} else if (key == "hidden") {
		this->Hidden = StringToBool(value);
	} else if (key == "resource") {
		value = value.replace("_", "-");
		this->Resource = GetResourceIdByName(value.utf8().get_data());
	} else if (key == "flag") {
		value = value.replace("_", "-");
		uint16_t flag = CTerrainType::GetTerrainFlagByName(value.utf8().get_data());
		if (flag) {
			this->Flags |= flag;
		}
	} else if (key == "elevation_graphics") {
		std::string elevation_graphics_file = value.utf8().get_data();
		if (CanAccessFile(elevation_graphics_file.c_str())) {
			if (!elevation_graphics_file.empty()) {
				if (CGraphic::Get(elevation_graphics_file) == nullptr) {
					CGraphic *graphics = CGraphic::New(elevation_graphics_file, CMap::PixelTileSize.x, CMap::PixelTileSize.y);
				}
				this->ElevationGraphics = CGraphic::Get(elevation_graphics_file);
			}
		} else {
			fprintf(stderr, "File \"%s\" doesn't exist.\n", value.utf8().get_data());
		}
	} else if (key == "player_color_graphics") {
		std::string player_color_graphics_file = value.utf8().get_data();
		if (CanAccessFile(player_color_graphics_file.c_str())) {
			if (!player_color_graphics_file.empty()) {
				if (CPlayerColorGraphic::Get(player_color_graphics_file) == nullptr) {
					CPlayerColorGraphic *graphics = CPlayerColorGraphic::New(player_color_graphics_file, CMap::PixelTileSize.x, CMap::PixelTileSize.y);
				}
				this->PlayerColorGraphics = CPlayerColorGraphic::Get(player_color_graphics_file);
			}
		} else {
			fprintf(stderr, "File \"%s\" doesn't exist.\n", value.utf8().get_data());
		}
	} else if (key == "pixel_width") {
		this->PixelTileSize.x = value.to_int();
	} else if (key == "pixel_height") {
		this->PixelTileSize.y = value.to_int();
	} else if (key == "base_terrain_type") {
		CTerrainType *base_terrain_type = CTerrainType::Get(value);
		this->BaseTerrainTypes.push_back(base_terrain_type);
	} else if (key == "inner_border_terrain_type") {
		CTerrainType *border_terrain_type = CTerrainType::Get(value);
		this->InnerBorderTerrains.push_back(border_terrain_type);
		this->BorderTerrains.push_back(border_terrain_type);
		border_terrain_type->OuterBorderTerrains.push_back(this);
		border_terrain_type->BorderTerrains.push_back(this);
	} else if (key == "outer_border_terrain_type") {
		CTerrainType *border_terrain_type = CTerrainType::Get(value);
		this->OuterBorderTerrains.push_back(border_terrain_type);
		this->BorderTerrains.push_back(border_terrain_type);
		border_terrain_type->InnerBorderTerrains.push_back(this);
		border_terrain_type->BorderTerrains.push_back(this);
	} else if (key == "solid_tile") {
		this->SolidTiles.push_back(value.to_int());
	} else if (key == "damaged_tile") {
		this->DamagedTiles.push_back(value.to_int());
	} else if (key == "destroyed_tile") {
		this->DestroyedTiles.push_back(value.to_int());
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CTerrainType::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "image") {
		String image_ident = "terrain_" + this->GetIdent();
		image_ident = image_ident.replace("_", "-");
		PaletteImage *image = PaletteImage::GetOrAdd(image_ident.utf8().get_data());
		image->ProcessConfigData(section);
		this->Image = image;
	} else if (section->Tag == "season_graphics") {
		std::string season_graphics_file;
		CSeason *season = nullptr;
		
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.utf8().get_data(), property.Operator);
				continue;
			}
			
			if (property.Key == "season") {
				season = CSeason::Get(property.Value);
			} else if (property.Key == "graphics") {
				season_graphics_file = property.Value.utf8().get_data();
				if (!CanAccessFile(season_graphics_file.c_str())) {
					fprintf(stderr, "File \"%s\" doesn't exist.\n", season_graphics_file.c_str());
				}
			} else {
				fprintf(stderr, "Invalid season graphics property: \"%s\".\n", property.Key.utf8().get_data());
			}
		}
		
		if (season_graphics_file.empty()) {
			fprintf(stderr, "Season graphics have no file.\n");
			return true;
		}
		
		if (!season) {
			fprintf(stderr, "Season graphics have no season.\n");
			return true;
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
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.utf8().get_data(), property.Operator);
				continue;
			}
			
			if (property.Key == "terrain_type") {
				CTerrainType *transition_terrain = CTerrainType::Get(property.Value);
				if (transition_terrain != nullptr) {
					transition_terrain_id = transition_terrain->GetIndex();
				}
			} else if (property.Key == "transition_type") {
				String value = property.Value.replace("_", "-");
				transition_type = GetTransitionTypeIdByName(value.utf8().get_data());
			} else if (property.Key == "tile") {
				tiles.push_back(property.Value.to_int());
			} else {
				fprintf(stderr, "Invalid transition tile property: \"%s\".\n", property.Key.utf8().get_data());
			}
		}
		
		if (transition_type == -1) {
			fprintf(stderr, "Transition tile has no transition type.\n");
			return true;
		}
		
		for (size_t j = 0; j < tiles.size(); ++j) {
			if (section->Tag == "transition_tile") {
				this->TransitionTiles[std::tuple<int, int>(transition_terrain_id, transition_type)].push_back(tiles[j]);
			} else if (section->Tag == "adjacent_transition_tile") {
				this->AdjacentTransitionTiles[std::tuple<int, int>(transition_terrain_id, transition_type)].push_back(tiles[j]);
			}
		}
	} else {
		return false;
	}
	
	return true;
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
	ClassDB::bind_method(D_METHOD("set_image", "ident"), +[](CTerrainType *terrain_type, const String &ident){ terrain_type->Image = PaletteImage::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_image"), +[](const CTerrainType *terrain_type){ return const_cast<PaletteImage *>(terrain_type->GetImage()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "image"), "set_image", "get_image");
	
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
