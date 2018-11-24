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
/**@name map_template.cpp - The map template source file. */
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

#include "map_template.h"

#include <fstream>

#include "calendar.h"
#include "config.h"
#include "editor.h"
#include "game.h"
#include "iolib.h"
#include "map.h"
#include "plane.h"
#include "player.h"
#include "quest.h"
#include "settings.h"
#include "terrain_type.h"
#include "tileset.h"
#include "translate.h"
#include "unit.h"
#include "unit_find.h"
#include "unittype.h"
#include "video.h"
#include "world.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CMapTemplate *> CMapTemplate::MapTemplates;
std::map<std::string, CMapTemplate *> CMapTemplate::MapTemplatesByIdent;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Get a map template
*/
CMapTemplate *CMapTemplate::GetMapTemplate(std::string ident)
{
	if (ident.empty()) {
		return NULL;
	}
	
	if (MapTemplatesByIdent.find(ident) != MapTemplatesByIdent.end()) {
		return MapTemplatesByIdent[ident];
	}
	
	return NULL;
}

CMapTemplate *CMapTemplate::GetOrAddMapTemplate(std::string ident)
{
	CMapTemplate *map_template = GetMapTemplate(ident);
	
	if (!map_template) {
		map_template = new CMapTemplate;
		map_template->Ident = ident;
		MapTemplates.push_back(map_template);
		MapTemplatesByIdent[ident] = map_template;
	}
	
	return map_template;
}

void CMapTemplate::ClearMapTemplates()
{
	for (size_t i = 0; i < MapTemplates.size(); ++i) {
		delete MapTemplates[i];
	}
	MapTemplates.clear();
}

void CMapTemplate::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "plane") {
			value = FindAndReplaceString(value, "_", "-");
			CPlane *plane = CPlane::GetPlane(value);
			if (plane) {
				this->Plane = plane;
			} else {
				fprintf(stderr, "Plane \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "world") {
			value = FindAndReplaceString(value, "_", "-");
			CWorld *world = CWorld::GetWorld(value);
			if (world) {
				this->World = world;
				this->Plane = this->World->Plane;
			} else {
				fprintf(stderr, "World \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "surface_layer") {
			this->SurfaceLayer = std::stoi(value);
			if (this->SurfaceLayer >= (int) UI.SurfaceLayerButtons.size()) {
				UI.SurfaceLayerButtons.resize(this->SurfaceLayer + 1);
			}
		} else if (key == "terrain_file") {
			this->TerrainFile = value;
		} else if (key == "overlay_terrain_file") {
			this->OverlayTerrainFile = value;
		} else if (key == "terrain_image") {
			this->TerrainImage = value;
		} else if (key == "overlay_terrain_image") {
			this->OverlayTerrainImage = value;
		} else if (key == "width") {
			this->Width = std::stoi(value);
		} else if (key == "height") {
			this->Height = std::stoi(value);
		} else if (key == "scale") {
			this->Scale = std::stoi(value);
		} else if (key == "pixel_tile_width") {
			this->PixelTileSize.x = std::stoi(value);
		} else if (key == "pixel_tile_height") {
			this->PixelTileSize.y = std::stoi(value);
		} else if (key == "overland") {
			this->Overland = StringToBool(value);
		} else if (key == "base_terrain_type") {
			value = FindAndReplaceString(value, "_", "-");
			CTerrainType *terrain_type = CTerrainType::GetTerrainType(value);
			if (terrain_type) {
				this->BaseTerrainType = terrain_type;
			} else {
				fprintf(stderr, "Terrain type \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "base_overlay_terrain_type") {
			value = FindAndReplaceString(value, "_", "-");
			CTerrainType *terrain_type = CTerrainType::GetTerrainType(value);
			if (terrain_type) {
				this->BaseOverlayTerrainType = terrain_type;
			} else {
				fprintf(stderr, "Terrain type \"%s\" does not exist.\n", value.c_str());
			}
		} else {
			fprintf(stderr, "Invalid map template property: \"%s\".\n", key.c_str());
		}
	}
	
	for (size_t i = 0; i < config_data->Children.size(); ++i) {
		CConfigData *child_config_data = config_data->Children[i];
		
		if (child_config_data->Tag == "generated_neutral_unit" || child_config_data->Tag == "player_location_generated_neutral_unit") {
			CUnitType *unit_type = NULL;
			int quantity = 1;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "unit_type") {
					value = FindAndReplaceString(value, "_", "-");
					unit_type = UnitTypeByIdent(value);
					if (!unit_type) {
						fprintf(stderr, "Unit type \"%s\" doesn't exist.\n", value.c_str());
					}
				} else if (key == "quantity") {
					quantity = std::stoi(value);
				} else {
					fprintf(stderr, "Invalid generated neutral unit property: \"%s\".\n", key.c_str());
				}
			}
			
			if (!unit_type) {
				fprintf(stderr, "Generated neutral unit has no unit type.\n");
				continue;
			}
			
			if (child_config_data->Tag == "generated_neutral_unit") {
				this->GeneratedNeutralUnits.push_back(std::pair<CUnitType *, int>(unit_type, quantity));
			} else if (child_config_data->Tag == "player_location_generated_neutral_unit") {
				this->PlayerLocationGeneratedNeutralUnits.push_back(std::pair<CUnitType *, int>(unit_type, quantity));
			}
		} else {
			fprintf(stderr, "Invalid map template property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

void CMapTemplate::ApplyTerrainFile(bool overlay, Vec2i template_start_pos, Vec2i map_start_pos, int z)
{
	std::string terrain_file;
	if (overlay) {
		terrain_file = this->OverlayTerrainFile;
	} else {
		terrain_file = this->TerrainFile;
	}
	
	if (terrain_file.empty()) {
		return;
	}
	
	const std::string terrain_filename = LibraryFileName(terrain_file.c_str());
		
	if (!CanAccessFile(terrain_filename.c_str())) {
		fprintf(stderr, "File \"%s\" not found.\n", terrain_filename.c_str());
	}
	
	std::ifstream is_map(terrain_filename);
	
	std::string line_str;
	int y = 0;
	while (std::getline(is_map, line_str))
	{
		if (y < template_start_pos.y || y >= (template_start_pos.y + Map.Info.MapHeights[z])) {
			y += 1;
			continue;
		}
		int x = 0;
		
		for (unsigned int i = 0; i < line_str.length(); ++i) {
			if (x < template_start_pos.x || x >= (template_start_pos.x + Map.Info.MapWidths[z])) {
				x += 1;
				continue;
			}
			std::string terrain_character = line_str.substr(i, 1);
			CTerrainType *terrain = NULL;
			if (CTerrainType::TerrainTypesByCharacter.find(terrain_character) != CTerrainType::TerrainTypesByCharacter.end()) {
				terrain = CTerrainType::TerrainTypesByCharacter.find(terrain_character)->second;
			}
			if (terrain) {
				Vec2i real_pos(map_start_pos.x + x - template_start_pos.x, map_start_pos.y + y - template_start_pos.y);
				Map.Field(real_pos, z)->SetTerrain(terrain);
			}
			x += 1;
		}
		
		y += 1;
	}

//	std::string filename = this->Ident;
//	if (overlay) {
//		filename += "-overlay";
//	}
//	filename += ".png";
//	SaveMapTemplatePNG(filename.c_str(), this, overlay);
}

void CMapTemplate::ApplyTerrainImage(bool overlay, Vec2i template_start_pos, Vec2i map_start_pos, int z)
{
	std::string terrain_file;
	if (overlay) {
		terrain_file = this->OverlayTerrainImage;
	} else {
		terrain_file = this->TerrainImage;
	}
	
	if (terrain_file.empty()) {
		ApplyTerrainFile(overlay, template_start_pos, map_start_pos, z);
		return;
	}
	
	const std::string terrain_filename = LibraryFileName(terrain_file.c_str());
		
	if (!CanAccessFile(terrain_filename.c_str())) {
		fprintf(stderr, "File \"%s\" not found.\n", terrain_filename.c_str());
	}
	
	CGraphic *terrain_image = CGraphic::New(terrain_filename);
	terrain_image->Load();
	
	SDL_LockSurface(terrain_image->Surface);
	const SDL_PixelFormat *f = terrain_image->Surface->format;
	const int bpp = terrain_image->Surface->format->BytesPerPixel;
	Uint8 r, g, b;

	for (int y = 0; y < terrain_image->Height; ++y) {
		if (y < template_start_pos.y || y >= (template_start_pos.y + (Map.Info.MapHeights[z] / this->Scale))) {
			continue;
		}
		
		for (int x = 0; x < terrain_image->Width; ++x) {
			if (x < template_start_pos.x || x >= (template_start_pos.x + (Map.Info.MapWidths[z] / this->Scale))) {
				continue;
			}

			Uint32 c = *reinterpret_cast<Uint32 *>(&reinterpret_cast<Uint8 *>(terrain_image->Surface->pixels)[x * 4 + y * terrain_image->Surface->pitch]);
			Uint8 a;

			Video.GetRGBA(c, terrain_image->Surface->format, &r, &g, &b, &a);
			
			if (a == 0) { //transparent pixels means leaving the area as it is (e.g. if it is a subtemplate use the main template's terrain for this tile instead)
				continue;
			}

			CTerrainType *terrain = NULL;
			short terrain_feature_id = -1;
			if (TerrainFeatureColorToIndex.find(std::tuple<int, int, int>(r, g, b)) != TerrainFeatureColorToIndex.end()) {
				terrain_feature_id = TerrainFeatureColorToIndex.find(std::tuple<int, int, int>(r, g, b))->second;
				terrain = TerrainFeatures[terrain_feature_id]->TerrainType;
			} else if (CTerrainType::TerrainTypesByColor.find(std::tuple<int, int, int>(r, g, b)) != CTerrainType::TerrainTypesByColor.end()) {
				terrain = CTerrainType::TerrainTypesByColor.find(std::tuple<int, int, int>(r, g, b))->second;
			}
			for (int sub_y = 0; sub_y < this->Scale; ++sub_y) {
				for (int sub_x = 0; sub_x < this->Scale; ++sub_x) {
					Vec2i real_pos(map_start_pos.x + ((x - template_start_pos.x) * this->Scale) + sub_x, map_start_pos.y + ((y - template_start_pos.y) * this->Scale) + sub_y);

					if (!Map.Info.IsPointOnMap(real_pos, z)) {
						continue;
					}
					
					if (terrain) {
						Map.Field(real_pos, z)->SetTerrain(terrain);
						
						if (terrain_feature_id != -1) {
							Map.Field(real_pos, z)->TerrainFeature = TerrainFeatures[terrain_feature_id];
						}
					} else {
						if (r != 0 || g != 0 || b != 0 || !overlay) { //fully black pixels represent areas in overlay terrain files that don't have any overlays
							fprintf(stderr, "Invalid map terrain: (%d, %d) (RGB: %d/%d/%d)\n", x, y, r, g, b);
						} else if (overlay && Map.Field(real_pos, z)->OverlayTerrain) { //fully black pixel in overlay terrain map = no overlay
							Map.Field(real_pos, z)->RemoveOverlayTerrain();
						}
					}
				}
			}
		}
	}
	SDL_UnlockSurface(terrain_image->Surface);
	
	CGraphic::Free(terrain_image);
}

void CMapTemplate::Apply(Vec2i template_start_pos, Vec2i map_start_pos, int z)
{
	if (SaveGameLoading) {
		return;
	}
	
	if (template_start_pos.x < 0 || template_start_pos.x >= this->Width || template_start_pos.y < 0 || template_start_pos.y >= this->Height) {
		fprintf(stderr, "Invalid map coordinate for map template \"%s\": (%d, %d)\n", this->Ident.c_str(), template_start_pos.x, template_start_pos.y);
		return;
	}
	
	if (z >= (int) Map.MapLayers.size()) {
		CMapLayer *map_layer = new CMapLayer;
		map_layer->ID = Map.MapLayers.size();
		Map.Info.MapWidths.push_back(std::min(this->Width * this->Scale, Map.Info.MapWidth));
		Map.Info.MapHeights.push_back(std::min(this->Height * this->Scale, Map.Info.MapHeight));
		map_layer->Fields = new CMapField[Map.Info.MapWidths[z] * Map.Info.MapHeights[z]];
		map_layer->Plane = this->Plane;
		map_layer->World = this->World;
		map_layer->SurfaceLayer = this->SurfaceLayer;
		map_layer->PixelTileSize = this->PixelTileSize;
		map_layer->Overland = this->Overland;
		Map.MapLayers.push_back(map_layer);
	} else {
		if (!this->IsSubtemplateArea()) {
			Map.MapLayers[z]->Plane = this->Plane;
			Map.MapLayers[z]->World = this->World;
			Map.MapLayers[z]->SurfaceLayer = this->SurfaceLayer;
			Map.MapLayers[z]->PixelTileSize = this->PixelTileSize;
			Map.MapLayers[z]->Overland = this->Overland;
		}
	}

	if (CurrentCampaign) {
		Map.Info.MapWidths[z] = CurrentCampaign->MapSizes[z].x;
		Map.Info.MapHeights[z] = CurrentCampaign->MapSizes[z].y;
	}
	
	if (!this->IsSubtemplateArea()) {
		Map.MapLayers[z]->TimeOfDay = NoTimeOfDay;
		if (
			this->SurfaceLayer == 0
			&& !GameSettings.Inside
			&& !GameSettings.NoTimeOfDay
			&& Editor.Running == EditorNotRunning
		) {
			if (this->World && this->World->HoursPerDay) {
				Map.MapLayers[z]->HoursPerDay = this->World->HoursPerDay;
			} else if (!this->World && this->Plane && this->Plane->HoursPerDay) {
				Map.MapLayers[z]->HoursPerDay = this->Plane->HoursPerDay;
			}
			if (Map.MapLayers[z]->HoursPerDay) {
				Map.MapLayers[z]->TimeOfDay = CCalendar::GetTimeOfDay(GameHour, Map.MapLayers[z]->HoursPerDay);
			}
		}
	}
	
	Vec2i map_end(std::min(Map.Info.MapWidths[z], map_start_pos.x + (this->Width * this->Scale)), std::min(Map.Info.MapHeights[z], map_start_pos.y + (this->Height * this->Scale)));
	if (!Map.Info.IsPointOnMap(map_start_pos, z)) {
		fprintf(stderr, "Invalid map coordinate for map template \"%s\": (%d, %d)\n", this->Ident.c_str(), map_start_pos.x, map_start_pos.y);
		return;
	}
	
	ShowLoadProgress(_("Applying \"%s\" Map Template Terrain"), this->Name.c_str());
	
	if (this->BaseTerrainType) {
		for (int x = map_start_pos.x; x < map_end.x; ++x) {
			for (int y = map_start_pos.y; y < map_end.y; ++y) {
				Map.Field(Vec2i(x, y), z)->SetTerrain(this->BaseTerrainType);
			}
		}
	}
	
	if (this->BaseOverlayTerrainType) {
		for (int x = map_start_pos.x; x < map_end.x; ++x) {
			for (int y = map_start_pos.y; y < map_end.y; ++y) {
				Map.Field(Vec2i(x, y), z)->SetTerrain(this->BaseOverlayTerrainType);
			}
		}
	}
	
	this->ApplyTerrainImage(false, template_start_pos, map_start_pos, z);
	this->ApplyTerrainImage(true, template_start_pos, map_start_pos, z);
	
	if (this->OutputTerrainImage) {
		std::string filename = this->Ident;
		std::string overlay_filename = this->Ident;
		overlay_filename += "-overlay";
		filename += ".png";
		overlay_filename += ".png";
		SaveMapTemplatePNG(filename.c_str(), this, false);
		SaveMapTemplatePNG(overlay_filename.c_str(), this, true);
	}

	if (CurrentCampaign) {
		for (size_t i = 0; i < HistoricalTerrains.size(); ++i) {
			Vec2i history_pos = std::get<0>(HistoricalTerrains[i]);
			if (history_pos.x < template_start_pos.x || history_pos.x >= (template_start_pos.x + (Map.Info.MapWidths[z] / this->Scale)) || history_pos.y < template_start_pos.y || history_pos.y >= (template_start_pos.y + (Map.Info.MapHeights[z] / this->Scale))) {
				continue;
			}
			if (CurrentCampaign->StartDate.ContainsDate(std::get<2>(HistoricalTerrains[i])) || std::get<2>(HistoricalTerrains[i]).Year == 0) {
				CTerrainType *historical_terrain = std::get<1>(HistoricalTerrains[i]);
				
				for (int sub_x = 0; sub_x < this->Scale; ++sub_x) {
					for (int sub_y = 0; sub_y < this->Scale; ++sub_y) {
						Vec2i real_pos(map_start_pos.x + ((history_pos.x - template_start_pos.x) * this->Scale) + sub_x, map_start_pos.y + ((history_pos.y - template_start_pos.y) * this->Scale) + sub_y);

						if (!Map.Info.IsPointOnMap(real_pos, z)) {
							continue;
						}
						
						if (historical_terrain) {
							if (historical_terrain->Overlay && ((historical_terrain->Flags & MapFieldRoad) || (historical_terrain->Flags & MapFieldRailroad)) && !(Map.Field(real_pos, z)->Flags & MapFieldLandAllowed)) {
								continue;
							}
							Map.Field(real_pos, z)->SetTerrain(historical_terrain);
						} else { //if the terrain type is NULL, then that means a previously set overlay terrain should be removed
							Map.Field(real_pos, z)->RemoveOverlayTerrain();
						}
					}
				}
			}
		}
	}
	
	if (this->IsSubtemplateArea() && this->SurroundingTerrainType) {
		Vec2i surrounding_start_pos(map_start_pos - Vec2i(1, 1));
		Vec2i surrounding_end(map_end + Vec2i(1, 1));
		for (int x = surrounding_start_pos.x; x < surrounding_end.x; ++x) {
			for (int y = surrounding_start_pos.y; y < surrounding_end.y; y += (surrounding_end.y - surrounding_start_pos.y - 1)) {
				Vec2i surrounding_pos(x, y);
				if (!Map.Info.IsPointOnMap(surrounding_pos, z) || Map.IsPointInASubtemplateArea(surrounding_pos, z)) {
					continue;
				}
				Map.Field(surrounding_pos, z)->SetTerrain(this->SurroundingTerrainType);
			}
		}
		for (int x = surrounding_start_pos.x; x < surrounding_end.x; x += (surrounding_end.x - surrounding_start_pos.x - 1)) {
			for (int y = surrounding_start_pos.y; y < surrounding_end.y; ++y) {
				Vec2i surrounding_pos(x, y);
				if (!Map.Info.IsPointOnMap(surrounding_pos, z) || Map.IsPointInASubtemplateArea(surrounding_pos, z)) {
					continue;
				}
				Map.Field(surrounding_pos, z)->SetTerrain(this->SurroundingTerrainType);
			}
		}
	}
	
	if (CurrentCampaign && CurrentCampaign->Faction && !this->IsSubtemplateArea() && ThisPlayer->Faction != CurrentCampaign->Faction->ID) {
		ThisPlayer->SetCivilization(CurrentCampaign->Faction->Civilization);
		ThisPlayer->SetFaction(CurrentCampaign->Faction);
		ThisPlayer->Resources[CopperCost] = 2500; // give the player enough resources to start up
		ThisPlayer->Resources[WoodCost] = 2500;
		ThisPlayer->Resources[StoneCost] = 2500;
	}
	
	for (size_t i = 0; i < this->Subtemplates.size(); ++i) {
		Vec2i subtemplate_pos(this->Subtemplates[i]->SubtemplatePosition - Vec2i((this->Subtemplates[i]->Width - 1) / 2, (this->Subtemplates[i]->Height - 1) / 2));
		bool found_location = false;
		
		if (subtemplate_pos.x < 0 || subtemplate_pos.y < 0) {
			Vec2i min_pos(map_start_pos);
			Vec2i max_pos(map_end.x - this->Subtemplates[i]->Width, map_end.y - this->Subtemplates[i]->Height);
			int while_count = 0;
			while (while_count < 1000) {
				subtemplate_pos.x = SyncRand(max_pos.x - min_pos.x + 1) + min_pos.x;
				subtemplate_pos.y = SyncRand(max_pos.y - min_pos.y + 1) + min_pos.y;
				
				bool on_map = Map.Info.IsPointOnMap(subtemplate_pos, z) && Map.Info.IsPointOnMap(Vec2i(subtemplate_pos.x + this->Subtemplates[i]->Width - 1, subtemplate_pos.y + this->Subtemplates[i]->Height - 1), z);
				
				bool on_subtemplate_area = false;
				for (int x = 0; x < this->Subtemplates[i]->Width; ++x) {
					for (int y = 0; y < this->Subtemplates[i]->Height; ++y) {
						if (Map.IsPointInASubtemplateArea(subtemplate_pos + Vec2i(x, y), z)) {
							on_subtemplate_area = true;
							break;
						}
					}
					if (on_subtemplate_area) {
						break;
					}
				}
				
				if (on_map && !on_subtemplate_area) {
					found_location = true;
					break;
				}
				
				while_count += 1;
			}
		} else {
			subtemplate_pos.x = map_start_pos.x + subtemplate_pos.x - template_start_pos.x;
			subtemplate_pos.y = map_start_pos.y + subtemplate_pos.y - template_start_pos.y;
			found_location = true;
		}
		
		if (found_location) {
			if (subtemplate_pos.x >= 0 && subtemplate_pos.y >= 0 && subtemplate_pos.x < Map.Info.MapWidths[z] && subtemplate_pos.y < Map.Info.MapHeights[z]) {
				this->Subtemplates[i]->Apply(Vec2i(0, 0), subtemplate_pos, z);
			}
				
			Map.MapLayers[z]->SubtemplateAreas.push_back(std::tuple<Vec2i, Vec2i, CMapTemplate *>(subtemplate_pos, Vec2i(subtemplate_pos.x + this->Subtemplates[i]->Width - 1, subtemplate_pos.y + this->Subtemplates[i]->Height - 1), this->Subtemplates[i]));
				
			if (subtemplate_pos.x >= 0 && subtemplate_pos.y >= 0 && subtemplate_pos.x < Map.Info.MapWidths[z] && subtemplate_pos.y < Map.Info.MapHeights[z]) {
				for (size_t j = 0; j < this->Subtemplates[i]->ExternalGeneratedTerrains.size(); ++j) {
					Vec2i external_start_pos(subtemplate_pos.x - (this->Subtemplates[i]->Width / 2), subtemplate_pos.y - (this->Subtemplates[i]->Height / 2));
					Vec2i external_end(subtemplate_pos.x + this->Subtemplates[i]->Width + (this->Subtemplates[i]->Width / 2), subtemplate_pos.y + this->Subtemplates[i]->Height + (this->Subtemplates[i]->Height / 2));
					int map_width = (external_end.x - external_start_pos.x);
					int map_height = (external_end.y - external_start_pos.y);
					int expansion_number = 0;
						
					int degree_level = this->Subtemplates[i]->ExternalGeneratedTerrains[j].second;
						
					if (degree_level == ExtremelyHighDegreeLevel) {
						expansion_number = map_width * map_height / 2;
					} else if (degree_level == VeryHighDegreeLevel) {
						expansion_number = map_width * map_height / 4;
					} else if (degree_level == HighDegreeLevel) {
						expansion_number = map_width * map_height / 8;
					} else if (degree_level == MediumDegreeLevel) {
						expansion_number = map_width * map_height / 16;
					} else if (degree_level == LowDegreeLevel) {
						expansion_number = map_width * map_height / 32;
					} else if (degree_level == VeryLowDegreeLevel) {
						expansion_number = map_width * map_height / 64;
					}
						
					Map.GenerateTerrain(this->Subtemplates[i]->ExternalGeneratedTerrains[j].first, 0, expansion_number, external_start_pos, external_end - Vec2i(1, 1), !this->Subtemplates[i]->TerrainFile.empty() || !this->Subtemplates[i]->TerrainImage.empty(), z);
				}
			}
		}
	}
	
	if (!this->IsSubtemplateArea()) {
		Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
		Map.AdjustTileMapTransitions(map_start_pos, map_end, z);
		Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
	}
	
	ShowLoadProgress(_("Applying \"%s\" Map Template Units"), this->Name.c_str());

	for (std::map<std::pair<int, int>, std::tuple<CUnitType *, int, CUniqueItem *>>::iterator iterator = this->Resources.begin(); iterator != this->Resources.end(); ++iterator) {
		Vec2i unit_raw_pos(iterator->first.first, iterator->first.second);
		Vec2i unit_pos(map_start_pos.x + unit_raw_pos.x - template_start_pos.x, map_start_pos.y + unit_raw_pos.y - template_start_pos.y);
		if (!Map.Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		
		const CUnitType *type = std::get<0>(iterator->second);
		
		Vec2i unit_offset((type->TileSize - 1) / 2);
		
		if (!OnTopDetails(*type, NULL) && !UnitTypeCanBeAt(*type, unit_pos - unit_offset, z) && Map.Info.IsPointOnMap(unit_pos - unit_offset, z) && Map.Info.IsPointOnMap(unit_pos - unit_offset + Vec2i(type->TileSize - 1), z)) {
			fprintf(stderr, "Unit \"%s\" should be placed on (%d, %d), but it cannot be there.\n", type->Ident.c_str(), unit_raw_pos.x, unit_raw_pos.y);
		}

		CUnit *unit = CreateResourceUnit(unit_pos - unit_offset, *type, z);
		
		if (std::get<1>(iterator->second)) {
			unit->SetResourcesHeld(std::get<1>(iterator->second));
			unit->Variable[GIVERESOURCE_INDEX].Value = std::get<1>(iterator->second);
			unit->Variable[GIVERESOURCE_INDEX].Max = std::get<1>(iterator->second);
			unit->Variable[GIVERESOURCE_INDEX].Enable = 1;
		}
		
		if (std::get<2>(iterator->second)) {
			unit->SetUnique(std::get<2>(iterator->second));
		}
	}

	if (CurrentCampaign != NULL) {
		this->ApplyConnectors(template_start_pos, map_start_pos, z);
	}
	this->ApplySites(template_start_pos, map_start_pos, z);
	this->ApplyUnits(template_start_pos, map_start_pos, z);
	
	ShowLoadProgress(_("Generating \"%s\" Map Template Random Terrain"), this->Name.c_str());
	
	for (size_t i = 0; i < this->GeneratedTerrains.size(); ++i) {
		int map_width = (map_end.x - map_start_pos.x);
		int map_height = (map_end.y - map_start_pos.y);
		int seed_number = 0;
		int expansion_number = 0;
		
		int degree_level = this->GeneratedTerrains[i].second;
		
		if (degree_level == ExtremelyHighDegreeLevel) {
			expansion_number = map_width * map_height / 2;
			seed_number = map_width * map_height / 128;
		} else if (degree_level == VeryHighDegreeLevel) {
			expansion_number = map_width * map_height / 4;
			seed_number = map_width * map_height / 256;
		} else if (degree_level == HighDegreeLevel) {
			expansion_number = map_width * map_height / 8;
			seed_number = map_width * map_height / 512;
		} else if (degree_level == MediumDegreeLevel) {
			expansion_number = map_width * map_height / 16;
			seed_number = map_width * map_height / 1024;
		} else if (degree_level == LowDegreeLevel) {
			expansion_number = map_width * map_height / 32;
			seed_number = map_width * map_height / 2048;
		} else if (degree_level == VeryLowDegreeLevel) {
			expansion_number = map_width * map_height / 64;
			seed_number = map_width * map_height / 4096;
		}
		
		seed_number = std::max(1, seed_number);
		
		Map.GenerateTerrain(this->GeneratedTerrains[i].first, seed_number, expansion_number, map_start_pos, map_end - Vec2i(1, 1), !this->TerrainFile.empty() || !this->TerrainImage.empty(), z);
	}
	
	if (!this->IsSubtemplateArea()) {
		Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
		Map.AdjustTileMapTransitions(map_start_pos, map_end, z);
		Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
	}
	
	ShowLoadProgress(_("Generating \"%s\" Map Template Random Units"), this->Name.c_str());

	// now, generate the units and heroes that were set to be generated at a random position (by having their position set to {-1, -1})
	if (CurrentCampaign != NULL) {
		this->ApplyConnectors(template_start_pos, map_start_pos, z, true);
	}
	this->ApplyUnits(template_start_pos, map_start_pos, z, true);

	for (int i = 0; i < PlayerMax; ++i) {
		if (Players[i].Type != PlayerPerson && Players[i].Type != PlayerComputer && Players[i].Type != PlayerRescueActive) {
			continue;
		}
		if (Map.IsPointInASubtemplateArea(Players[i].StartPos, z)) {
			continue;
		}
		if (Players[i].StartPos.x < map_start_pos.x || Players[i].StartPos.y < map_start_pos.y || Players[i].StartPos.x >= map_end.x || Players[i].StartPos.y >= map_end.y || Players[i].StartMapLayer != z) {
			continue;
		}
		if (Players[i].StartPos.x == 0 && Players[i].StartPos.y == 0) {
			continue;
		}
		// add five workers at the player's starting location
		if (Players[i].NumTownHalls > 0) {
			int worker_type_id = PlayerRaces.GetFactionClassUnitType(Players[i].Faction, GetUnitTypeClassIndexByName("worker"));
			if (worker_type_id != -1 && Players[i].GetUnitTypeCount(UnitTypes[worker_type_id]) == 0) { //only create if the player doesn't have any workers created in another manner
				Vec2i worker_unit_offset((UnitTypes[worker_type_id]->TileSize - 1) / 2);
				
				Vec2i worker_pos(Players[i].StartPos);

				bool start_pos_has_town_hall = false;
				std::vector<CUnit *> table;
				Select(worker_pos - Vec2i(4, 4), worker_pos + Vec2i(4, 4), table, z, HasSamePlayerAs(Players[i]));
				for (size_t j = 0; j < table.size(); ++j) {
					if (table[j]->Type->BoolFlag[TOWNHALL_INDEX].value) {
						start_pos_has_town_hall = true;
						break;
					}
				}
				
				if (!start_pos_has_town_hall) { //if the start pos doesn't have a town hall, create the workers in the position of a town hall the player has
					for (int j = 0; j < Players[i].GetUnitCount(); ++j) {
						CUnit *town_hall_unit = &Players[i].GetUnit(j);
						if (!town_hall_unit->Type->BoolFlag[TOWNHALL_INDEX].value) {
							continue;
						}
						if (town_hall_unit->MapLayer != z) {
							continue;
						}
						worker_pos = town_hall_unit->tilePos;
					}
				}
				
				for (int j = 0; j < 5; ++j) {
					CUnit *worker_unit = CreateUnit(worker_pos, *UnitTypes[worker_type_id], &Players[i], Players[i].StartMapLayer);
				}
			}
		}
		
		if (Players[i].NumTownHalls > 0 || Players[i].Index == ThisPlayer->Index) {
			for (size_t j = 0; j < this->PlayerLocationGeneratedNeutralUnits.size(); ++j) {
				Map.GenerateNeutralUnits(this->PlayerLocationGeneratedNeutralUnits[j].first, this->PlayerLocationGeneratedNeutralUnits[j].second, Players[i].StartPos - Vec2i(8, 8), Players[i].StartPos + Vec2i(8, 8), true, z);
			}
		}
	}
	
	for (size_t i = 0; i < this->GeneratedNeutralUnits.size(); ++i) {
		bool grouped = this->GeneratedNeutralUnits[i].first->GivesResource && this->GeneratedNeutralUnits[i].first->TileSize.x == 1 && this->GeneratedNeutralUnits[i].first->TileSize.y == 1; // group small resources
		Map.GenerateNeutralUnits(this->GeneratedNeutralUnits[i].first, this->GeneratedNeutralUnits[i].second, map_start_pos, map_end - Vec2i(1, 1), grouped, z);
	}
}

void CMapTemplate::ApplySites(Vec2i template_start_pos, Vec2i map_start_pos, int z)
{
	Vec2i map_end(std::min(Map.Info.MapWidths[z], map_start_pos.x + this->Width), std::min(Map.Info.MapHeights[z], map_start_pos.y + this->Height));

	for (std::map<std::pair<int, int>, CSite *>::iterator site_iterator = this->Sites.begin(); site_iterator != this->Sites.end(); ++site_iterator) {
		Vec2i site_raw_pos(site_iterator->second->Position);
		Vec2i site_pos(map_start_pos + ((site_raw_pos - template_start_pos) * this->Scale));

		if (!Map.Info.IsPointOnMap(site_pos, z) || site_pos.x < map_start_pos.x || site_pos.y < map_start_pos.y) {
			continue;
		}

		if (site_iterator->second->Major && SettlementSiteUnitType) { //add a settlement site for major sites
			Vec2i unit_offset((SettlementSiteUnitType->TileSize - 1) / 2);
			if (!UnitTypeCanBeAt(*SettlementSiteUnitType, site_pos - unit_offset, z) && Map.Info.IsPointOnMap(site_pos - unit_offset, z) && Map.Info.IsPointOnMap(site_pos - unit_offset + Vec2i(SettlementSiteUnitType->TileSize - 1), z)) {
				fprintf(stderr, "The settlement site for \"%s\" should be placed on (%d, %d), but it cannot be there.\n", site_iterator->second->Ident.c_str(), site_raw_pos.x, site_raw_pos.y);
			}
			CUnit *unit = CreateUnit(site_pos - unit_offset, *SettlementSiteUnitType, &Players[PlayerNumNeutral], z, true);
			unit->Settlement = site_iterator->second;
			unit->Settlement->SiteUnit = unit;
			Map.SiteUnits.push_back(unit);
		}
		
		for (size_t j = 0; j < site_iterator->second->HistoricalResources.size(); ++j) {
			if (
				(!CurrentCampaign && std::get<1>(site_iterator->second->HistoricalResources[j]).Year == 0 && std::get<1>(site_iterator->second->HistoricalResources[j]).Year == 0)
				|| (
					CurrentCampaign && CurrentCampaign->StartDate.ContainsDate(std::get<0>(site_iterator->second->HistoricalResources[j]))
					&& (!CurrentCampaign->StartDate.ContainsDate(std::get<1>(site_iterator->second->HistoricalResources[j])) || std::get<1>(site_iterator->second->HistoricalResources[j]).Year == 0)
				)
			) {
				const CUnitType *type = std::get<2>(site_iterator->second->HistoricalResources[j]);
				if (!type) {
					fprintf(stderr, "Error in CMap::ApplySites (site ident \"%s\"): historical resource type is NULL.\n", site_iterator->second->Ident.c_str());
					continue;
				}
				Vec2i unit_offset((type->TileSize - 1) / 2);
				CUnit *unit = CreateResourceUnit(site_pos - unit_offset, *type, z, false); // don't generate unique resources when setting special properties, since for map templates unique resources are supposed to be explicitly indicated
				if (std::get<3>(site_iterator->second->HistoricalResources[j])) {
					unit->SetUnique(std::get<3>(site_iterator->second->HistoricalResources[j]));
				}
				int resource_quantity = std::get<4>(site_iterator->second->HistoricalResources[j]);
				if (resource_quantity) { //set the resource_quantity after setting the unique unit, so that unique resources can be decreased in quantity over time
					unit->SetResourcesHeld(resource_quantity);
					unit->Variable[GIVERESOURCE_INDEX].Value = resource_quantity;
					unit->Variable[GIVERESOURCE_INDEX].Max = resource_quantity;
					unit->Variable[GIVERESOURCE_INDEX].Enable = 1;
				}
			}
		}
		
		if (!CurrentCampaign) {
			continue;
		}
		
		CFaction *site_owner = NULL;
		for (std::map<CDate, CFaction *>::reverse_iterator owner_iterator = site_iterator->second->HistoricalOwners.rbegin(); owner_iterator != site_iterator->second->HistoricalOwners.rend(); ++owner_iterator) {
			if (CurrentCampaign->StartDate.ContainsDate(owner_iterator->first)) { // set the owner to the latest historical owner given the scenario's start date
				site_owner = owner_iterator->second;
				break;
			}
		}
		
		if (!site_owner) {
			continue;
		}
		
		CPlayer *player = GetOrAddFactionPlayer(site_owner);
		
		if (!player) {
			continue;
		}
		
		bool is_capital = false;
		for (int i = ((int) site_owner->HistoricalCapitals.size() - 1); i >= 0; --i) {
			if (CurrentCampaign->StartDate.ContainsDate(site_owner->HistoricalCapitals[i].first) || site_owner->HistoricalCapitals[i].first.Year == 0) {
				if (site_owner->HistoricalCapitals[i].second == site_iterator->second->Ident) {
					is_capital = true;
				}
				break;
			}
		}
		
		if ((player->StartPos.x == 0 && player->StartPos.y == 0) || is_capital) {
			player->SetStartView(site_pos, z);
		}
		
		CUnitType *pathway_type = NULL;
		for (size_t j = 0; j < site_iterator->second->HistoricalBuildings.size(); ++j) {
			if (
				CurrentCampaign->StartDate.ContainsDate(std::get<0>(site_iterator->second->HistoricalBuildings[j]))
				&& (!CurrentCampaign->StartDate.ContainsDate(std::get<1>(site_iterator->second->HistoricalBuildings[j])) || std::get<1>(site_iterator->second->HistoricalBuildings[j]).Year == 0)
			) {
				int unit_type_id = -1;
				unit_type_id = PlayerRaces.GetFactionClassUnitType(site_owner->ID, std::get<2>(site_iterator->second->HistoricalBuildings[j]));
				if (unit_type_id == -1) {
					continue;
				}
				const CUnitType *type = UnitTypes[unit_type_id];
				if (type->TerrainType) {
					if ((type->TerrainType->Flags & MapFieldRoad) || (type->TerrainType->Flags & MapFieldRailroad)) {
						pathway_type = UnitTypes[unit_type_id];
					}
				}
			}
		}
		
		bool first_building = true;
		for (size_t j = 0; j < site_iterator->second->HistoricalBuildings.size(); ++j) {
			if (
				CurrentCampaign->StartDate.ContainsDate(std::get<0>(site_iterator->second->HistoricalBuildings[j]))
				&& (!CurrentCampaign->StartDate.ContainsDate(std::get<1>(site_iterator->second->HistoricalBuildings[j])) || std::get<1>(site_iterator->second->HistoricalBuildings[j]).Year == 0)
			) {
				CFaction *building_owner = std::get<4>(site_iterator->second->HistoricalBuildings[j]);
				int unit_type_id = -1;
				if (building_owner) {
					unit_type_id = PlayerRaces.GetFactionClassUnitType(building_owner->ID, std::get<2>(site_iterator->second->HistoricalBuildings[j]));
				} else {
					unit_type_id = PlayerRaces.GetFactionClassUnitType(site_owner->ID, std::get<2>(site_iterator->second->HistoricalBuildings[j]));
				}
				if (unit_type_id == -1) {
					continue;
				}
				const CUnitType *type = UnitTypes[unit_type_id];
				if (type->TerrainType) {
					continue;
				}
				if (type->BoolFlag[TOWNHALL_INDEX].value && !site_iterator->second->Major) {
					fprintf(stderr, "Error in CMap::ApplySites (site ident \"%s\"): site has a town hall, but isn't set as a major one.\n", site_iterator->second->Ident.c_str());
					continue;
				}
				Vec2i unit_offset((type->TileSize - 1) / 2);
				if (first_building) {
					if (!OnTopDetails(*type, NULL) && !UnitTypeCanBeAt(*type, site_pos - unit_offset, z) && Map.Info.IsPointOnMap(site_pos - unit_offset, z) && Map.Info.IsPointOnMap(site_pos - unit_offset + Vec2i(type->TileSize - 1), z)) {
						fprintf(stderr, "The \"%s\" representing the minor site of \"%s\" should be placed on (%d, %d), but it cannot be there.\n", type->Ident.c_str(), site_iterator->second->Ident.c_str(), site_raw_pos.x, site_raw_pos.y);
					}
				}
				CUnit *unit = NULL;
				if (building_owner) {
					CPlayer *building_player = GetOrAddFactionPlayer(building_owner);
					if (!building_player) {
						continue;
					}
					if (building_player->StartPos.x == 0 && building_player->StartPos.y == 0) {
						building_player->SetStartView(site_pos - unit_offset, z);
					}
					unit = CreateUnit(site_pos - unit_offset, *type, building_player, z, true);
				} else {
					unit = CreateUnit(site_pos - unit_offset, *type, player, z, true);
				}
				if (std::get<3>(site_iterator->second->HistoricalBuildings[j])) {
					unit->SetUnique(std::get<3>(site_iterator->second->HistoricalBuildings[j]));
				}
				if (first_building) {
					if (!type->BoolFlag[TOWNHALL_INDEX].value && !unit->Unique && (!building_owner || building_owner == site_owner) && site_iterator->second->CulturalNames.find(site_owner->Civilization) != site_iterator->second->CulturalNames.end()) { //if one building is representing a minor site, make it have the site's name
						unit->Name = site_iterator->second->CulturalNames.find(site_owner->Civilization)->second;
					}
					first_building = false;
				}
				if (type->BoolFlag[TOWNHALL_INDEX].value && (!building_owner || building_owner == site_owner) && site_iterator->second->CulturalNames.find(site_owner->Civilization) != site_iterator->second->CulturalNames.end()) {
					unit->UpdateBuildingSettlementAssignment();
				}
				if (pathway_type) {
					for (int x = unit->tilePos.x - 1; x < unit->tilePos.x + unit->Type->TileSize.x + 1; ++x) {
						for (int y = unit->tilePos.y - 1; y < unit->tilePos.y + unit->Type->TileSize.y + 1; ++y) {
							if (!Map.Info.IsPointOnMap(x, y, unit->MapLayer)) {
								continue;
							}
							CMapField &mf = *Map.Field(x, y, unit->MapLayer);
							if (mf.Flags & MapFieldBuilding) { //this is a tile where the building itself is located, continue
								continue;
							}
							Vec2i pathway_pos(x, y);
							if (!UnitTypeCanBeAt(*pathway_type, pathway_pos, unit->MapLayer)) {
								continue;
							}
							
							mf.SetTerrain(pathway_type->TerrainType);
						}
					}
				}
			}
		}
		
		for (size_t j = 0; j < site_iterator->second->HistoricalUnits.size(); ++j) {
			if (
				CurrentCampaign->StartDate.ContainsDate(std::get<0>(site_iterator->second->HistoricalUnits[j]))
				&& (!CurrentCampaign->StartDate.ContainsDate(std::get<1>(site_iterator->second->HistoricalUnits[j])) || std::get<1>(site_iterator->second->HistoricalUnits[j]).Year == 0)
			) {
				int unit_quantity = std::get<3>(site_iterator->second->HistoricalUnits[j]);
						
				if (unit_quantity > 0) {
					const CUnitType *type = std::get<2>(site_iterator->second->HistoricalUnits[j]);
					if (type->BoolFlag[ORGANIC_INDEX].value) {
						unit_quantity = std::max(1, unit_quantity / PopulationPerUnit); //each organic unit represents 1,000 people
					}
							
					CPlayer *unit_player = NULL;
					CFaction *unit_owner = std::get<4>(site_iterator->second->HistoricalUnits[j]);
					if (unit_owner) {
						unit_player = GetOrAddFactionPlayer(unit_owner);
						if (!unit_player) {
							continue;
						}
						if (unit_player->StartPos.x == 0 && unit_player->StartPos.y == 0) {
							unit_player->SetStartView(site_pos, z);
						}
					} else {
						unit_player = player;
					}
					Vec2i unit_offset((type->TileSize - 1) / 2);

					for (int j = 0; j < unit_quantity; ++j) {
						CUnit *unit = CreateUnit(site_pos - unit_offset, *type, unit_player, z);
						if (!type->BoolFlag[HARVESTER_INDEX].value) { // make non-worker units not have an active AI
							unit->Active = 0;
							unit_player->ChangeUnitTypeAiActiveCount(type, -1);
						}
					}
				}
			}
		}
	}
}

void CMapTemplate::ApplyConnectors(Vec2i template_start_pos, Vec2i map_start_pos, int z, bool random)
{
	Vec2i map_end(std::min(Map.Info.MapWidths[z], map_start_pos.x + this->Width), std::min(Map.Info.MapHeights[z], map_start_pos.y + this->Height));

	for (size_t i = 0; i < this->PlaneConnectors.size(); ++i) {
		const CUnitType *type = std::get<1>(this->PlaneConnectors[i]);
		Vec2i unit_raw_pos(std::get<0>(this->PlaneConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = Map.GenerateUnitLocation(type, NULL, map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x || unit_pos.y < map_start_pos.y) {
			continue;
		}
		
		Vec2i unit_offset((type->TileSize - 1) / 2);

		if (!OnTopDetails(*type, NULL) && !UnitTypeCanBeAt(*type, unit_pos - unit_offset, z) && Map.Info.IsPointOnMap(unit_pos - unit_offset, z) && Map.Info.IsPointOnMap(unit_pos - unit_offset + Vec2i(type->TileSize - 1), z)) {
			fprintf(stderr, "Unit \"%s\" should be placed on (%d, %d), but it cannot be there.\n", type->Ident.c_str(), unit_raw_pos.x, unit_raw_pos.y);
		}

		CUnit *unit = CreateUnit(unit_pos - unit_offset, *type, &Players[PlayerNumNeutral], z, true);
		if (std::get<3>(this->PlaneConnectors[i])) {
			unit->SetUnique(std::get<3>(this->PlaneConnectors[i]));
		}
		Map.MapLayers[z]->LayerConnectors.push_back(unit);
		for (size_t second_z = 0; second_z < Map.MapLayers.size(); ++second_z) {
			bool found_other_connector = false;
			if (Map.MapLayers[second_z]->Plane == std::get<2>(this->PlaneConnectors[i])) {
				for (size_t j = 0; j < Map.MapLayers[second_z]->LayerConnectors.size(); ++j) {
					if (Map.MapLayers[second_z]->LayerConnectors[j]->Type == unit->Type && Map.MapLayers[second_z]->LayerConnectors[j]->Unique == unit->Unique && Map.MapLayers[second_z]->LayerConnectors[j]->ConnectingDestination == NULL) {
						Map.MapLayers[second_z]->LayerConnectors[j]->ConnectingDestination = unit;
						unit->ConnectingDestination = Map.MapLayers[second_z]->LayerConnectors[j];
						found_other_connector = true;
						break;
					}
				}
			}
			if (found_other_connector) {
				break;
			}
		}
	}
	
	for (size_t i = 0; i < this->WorldConnectors.size(); ++i) {
		const CUnitType *type = std::get<1>(this->WorldConnectors[i]);
		Vec2i unit_raw_pos(std::get<0>(this->WorldConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = Map.GenerateUnitLocation(type, NULL, map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x || unit_pos.y < map_start_pos.y) {
			continue;
		}
		
		Vec2i unit_offset((type->TileSize - 1) / 2);

		if (!OnTopDetails(*type, NULL) && !UnitTypeCanBeAt(*type, unit_pos - unit_offset, z) && Map.Info.IsPointOnMap(unit_pos - unit_offset, z) && Map.Info.IsPointOnMap(unit_pos - unit_offset + Vec2i(type->TileSize - 1), z)) {
			fprintf(stderr, "Unit \"%s\" should be placed on (%d, %d), but it cannot be there.\n", type->Ident.c_str(), unit_raw_pos.x, unit_raw_pos.y);
		}

		CUnit *unit = CreateUnit(unit_pos - unit_offset, *type, &Players[PlayerNumNeutral], z, true);
		if (std::get<3>(this->WorldConnectors[i])) {
			unit->SetUnique(std::get<3>(this->WorldConnectors[i]));
		}
		Map.MapLayers[z]->LayerConnectors.push_back(unit);
		for (size_t second_z = 0; second_z < Map.MapLayers.size(); ++second_z) {
			bool found_other_connector = false;
			if (Map.MapLayers[second_z]->World == std::get<2>(this->WorldConnectors[i])) {
				for (size_t j = 0; j < Map.MapLayers[second_z]->LayerConnectors.size(); ++j) {
					if (Map.MapLayers[second_z]->LayerConnectors[j]->Type == unit->Type && Map.MapLayers[second_z]->LayerConnectors[j]->Unique == unit->Unique && Map.MapLayers[second_z]->LayerConnectors[j]->ConnectingDestination == NULL) {
						Map.MapLayers[second_z]->LayerConnectors[j]->ConnectingDestination = unit;
						unit->ConnectingDestination = Map.MapLayers[second_z]->LayerConnectors[j];
						found_other_connector = true;
						break;
					}
				}
			}
			if (found_other_connector) {
				break;
			}
		}
	}
	
	for (size_t i = 0; i < this->SurfaceLayerConnectors.size(); ++i) {
		const CUnitType *type = std::get<1>(this->SurfaceLayerConnectors[i]);
		Vec2i unit_raw_pos(std::get<0>(this->SurfaceLayerConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = Map.GenerateUnitLocation(type, NULL, map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x || unit_pos.y < map_start_pos.y) {
			continue;
		}
		
		Vec2i unit_offset((type->TileSize - 1) / 2);
		
		if (!OnTopDetails(*type, NULL) && !UnitTypeCanBeAt(*type, unit_pos - unit_offset, z) && Map.Info.IsPointOnMap(unit_pos - unit_offset, z) && Map.Info.IsPointOnMap(unit_pos - unit_offset + Vec2i(type->TileSize - 1), z)) {
			fprintf(stderr, "Unit \"%s\" should be placed on (%d, %d), but it cannot be there.\n", type->Ident.c_str(), unit_raw_pos.x, unit_raw_pos.y);
		}

		CUnit *unit = CreateUnit(unit_pos - unit_offset, *type, &Players[PlayerNumNeutral], z, true);
		if (std::get<3>(this->SurfaceLayerConnectors[i])) {
			unit->SetUnique(std::get<3>(this->SurfaceLayerConnectors[i]));
		}
		Map.MapLayers[z]->LayerConnectors.push_back(unit);
		for (size_t second_z = 0; second_z < Map.MapLayers.size(); ++second_z) {
			bool found_other_connector = false;
			if (Map.MapLayers[second_z]->SurfaceLayer == std::get<2>(this->SurfaceLayerConnectors[i]) && Map.MapLayers[second_z]->World == this->World && Map.MapLayers[second_z]->Plane == this->Plane) {
				for (size_t j = 0; j < Map.MapLayers[second_z]->LayerConnectors.size(); ++j) {
					if (Map.MapLayers[second_z]->LayerConnectors[j]->Type == unit->Type && Map.MapLayers[second_z]->LayerConnectors[j]->tilePos == unit->tilePos && Map.MapLayers[second_z]->LayerConnectors[j]->Unique == unit->Unique && Map.MapLayers[second_z]->LayerConnectors[j]->ConnectingDestination == NULL) { //surface layer connectors need to be in the same X and Y coordinates as their destinations
						Map.MapLayers[second_z]->LayerConnectors[j]->ConnectingDestination = unit;
						unit->ConnectingDestination = Map.MapLayers[second_z]->LayerConnectors[j];
						found_other_connector = true;
						break;
					}
				}
			}
			if (found_other_connector) {
				break;
			}
		}
	}
}

void CMapTemplate::ApplyUnits(Vec2i template_start_pos, Vec2i map_start_pos, int z, bool random)
{
	Vec2i map_end(std::min(Map.Info.MapWidths[z], map_start_pos.x + this->Width), std::min(Map.Info.MapHeights[z], map_start_pos.y + this->Height));

	for (size_t i = 0; i < this->Units.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(this->Units[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		const CUnitType *type = std::get<1>(this->Units[i]);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = Map.GenerateUnitLocation(type, std::get<2>(this->Units[i]), map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x || unit_pos.y < map_start_pos.y) {
			continue;
		}
		
		if (
			(!CurrentCampaign && std::get<3>(this->Units[i]).Year == 0 && std::get<4>(this->Units[i]).Year == 0)
			|| (
				CurrentCampaign && (std::get<3>(this->Units[i]).Year == 0 || CurrentCampaign->StartDate.ContainsDate(std::get<3>(this->Units[i])))
				&& (std::get<4>(this->Units[i]).Year == 0 || (!CurrentCampaign->StartDate.ContainsDate(std::get<4>(this->Units[i]))))
			)
		) {
			CPlayer *player = NULL;
			if (std::get<2>(this->Units[i])) {
				if (!CurrentCampaign) { //only apply neutral units for when applying map templates for non-campaign/scenario maps
					continue;
				}
				player = GetOrAddFactionPlayer(std::get<2>(this->Units[i]));
				if (!player) {
					continue;
				}
				if (player->StartPos.x == 0 && player->StartPos.y == 0) {
					player->SetStartView(unit_pos, z);
				}
			} else {
				player = &Players[PlayerNumNeutral];
			}
			Vec2i unit_offset((type->TileSize - 1) / 2);

			CUnit *unit = CreateUnit(unit_pos - unit_offset, *std::get<1>(this->Units[i]), player, z, type->BoolFlag[BUILDING_INDEX].value && type->TileSize.x > 1 && type->TileSize.y > 1);
			if (!type->BoolFlag[BUILDING_INDEX].value && !type->BoolFlag[HARVESTER_INDEX].value) { // make non-building, non-harvester units not have an active AI
				unit->Active = 0;
				player->ChangeUnitTypeAiActiveCount(type, -1);
			}
			if (std::get<5>(this->Units[i])) {
				unit->SetUnique(std::get<5>(this->Units[i]));
			}
		}
	}

	if (!CurrentCampaign) {
		return;
	}

	for (size_t i = 0; i < this->Heroes.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(this->Heroes[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		CCharacter *hero = std::get<1>(this->Heroes[i]);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = Map.GenerateUnitLocation(hero->Type, std::get<2>(this->Heroes[i]), map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x || unit_pos.y < map_start_pos.y) {
			continue;
		}
		
		if ((!CurrentCampaign || std::get<3>(this->Heroes[i]).Year == 0 || CurrentCampaign->StartDate.ContainsDate(std::get<3>(this->Heroes[i]))) && (std::get<4>(this->Heroes[i]).Year == 0 || !CurrentCampaign->StartDate.ContainsDate(std::get<4>(this->Heroes[i])))) {
			CPlayer *player = NULL;
			if (std::get<2>(this->Heroes[i])) {
				player = GetOrAddFactionPlayer(std::get<2>(this->Heroes[i]));
				if (!player) {
					continue;
				}
				if (player->StartPos.x == 0 && player->StartPos.y == 0) {
					player->SetStartView(unit_pos, z);
				}
			} else {
				player = &Players[PlayerNumNeutral];
			}
			Vec2i unit_offset((hero->Type->TileSize - 1) / 2);
			CUnit *unit = CreateUnit(unit_pos - unit_offset, *hero->Type, player, z);
			unit->SetCharacter(hero->Ident);
			if (!unit->Type->BoolFlag[BUILDING_INDEX].value && !unit->Type->BoolFlag[HARVESTER_INDEX].value) { // make non-building, non-harvester units not have an active AI
				unit->Active = 0;
				player->ChangeUnitTypeAiActiveCount(hero->Type, -1);
			}
		}
	}
	
	if (this->IsSubtemplateArea() || random) { //don't perform the dynamic hero application if this is a subtemplate area, to avoid creating multiple copies of the same hero
		return;
	}
	
	for (std::map<std::string, CCharacter *>::iterator iterator = Characters.begin(); iterator != Characters.end(); ++iterator) {
		CCharacter *hero = iterator->second;
		
		if (!hero->CanAppear()) {
			continue;
		}
		
		if (hero->Faction == NULL && !hero->Type->BoolFlag[FAUNA_INDEX].value) { //only fauna "heroes" may have no faction
			continue;
		}
		
		if (hero->StartDate.Year == 0 || !CurrentCampaign->StartDate.ContainsDate(hero->StartDate) || CurrentCampaign->StartDate.ContainsDate(hero->DeathDate)) { //contrary to other elements, heroes aren't implemented if their date isn't set
			continue;
		}
		
		CFaction *hero_faction = hero->Faction;
		for (int i = ((int) hero->HistoricalFactions.size() - 1); i >= 0; --i) {
			if (CurrentCampaign->StartDate.ContainsDate(hero->HistoricalFactions[i].first)) {
				hero_faction = hero->HistoricalFactions[i].second;
				break;
			}
		}

		CPlayer *hero_player = hero_faction ? GetFactionPlayer(hero_faction) : NULL;
		
		Vec2i hero_pos(-1, -1);
		
		if (hero_player && hero_player->StartMapLayer == z) {
			hero_pos = hero_player->StartPos;
		}
		
		bool in_another_map_layer = false;
		for (int i = ((int) hero->HistoricalLocations.size() - 1); i >= 0; --i) {
			if (CurrentCampaign->StartDate.ContainsDate(std::get<0>(hero->HistoricalLocations[i]))) {
				if (std::get<1>(hero->HistoricalLocations[i]) == this) {
					hero_pos = map_start_pos + std::get<2>(hero->HistoricalLocations[i]) - template_start_pos;
				} else {
					in_another_map_layer = true;
				}
				break;
			}
		}
		
		if (in_another_map_layer) {
			continue;
		}
		
		if (!Map.Info.IsPointOnMap(hero_pos, z) || hero_pos.x < map_start_pos.x || hero_pos.y < map_start_pos.y) { //heroes whose faction hasn't been created already and who don't have a valid historical location set won't be created
			continue;
		}
		
		if (hero_faction) {
			hero_player = GetOrAddFactionPlayer(hero_faction);
			if (!hero_player) {
				continue;
			}
			if (hero_player->StartPos.x == 0 && hero_player->StartPos.y == 0) {
				hero_player->SetStartView(hero_pos, z);
			}
		} else {
			hero_player = &Players[PlayerNumNeutral];
		}
		Vec2i unit_offset((hero->Type->TileSize - 1) / 2);
		CUnit *unit = CreateUnit(hero_pos - unit_offset, *hero->Type, hero_player, z);
		unit->SetCharacter(hero->Ident);
		unit->Active = 0;
		hero_player->ChangeUnitTypeAiActiveCount(hero->Type, -1);
	}
}

bool CMapTemplate::IsSubtemplateArea()
{
	return this->MainTemplate != NULL;
}

//@}
