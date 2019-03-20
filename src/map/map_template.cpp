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

#include "map/map_template.h"

#include "civilization.h"
#include "config.h"
#include "editor/editor.h"
#include "faction.h"
#include "game/game.h"
#include "iocompat.h"
#include "iolib.h"
#include "map/historical_location.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tileset.h"
#include "player.h"
#include "quest/campaign.h"
#include "quest/quest.h"
#include "settings.h"
#include "time/calendar.h"
#include "time/season_schedule.h"
#include "time/time_of_day_schedule.h"
#include "translate.h"
#include "unit/historical_unit.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "video/video.h"
#include "world/plane.h"
#include "world/world.h"

#include <fstream>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

const int MaxAdjacentTemplateDistance = 16;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Destructor
*/
CMapTemplate::~CMapTemplate()
{
	for (CGeneratedTerrain *generated_terrain : this->GeneratedTerrains) {
		delete generated_terrain;
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
bool CMapTemplate::ProcessConfigDataProperty(const std::string &key, std::string value)
{
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
	} else if (key == "priority") {
		this->Priority = std::stoi(value);
	} else if (key == "pixel_tile_width") {
		this->PixelTileSize.x = std::stoi(value);
	} else if (key == "pixel_tile_height") {
		this->PixelTileSize.y = std::stoi(value);
	} else if (key == "min_x") {
		this->MinPos.x = std::stoi(value);
	} else if (key == "min_y") {
		this->MinPos.y = std::stoi(value);
	} else if (key == "max_x") {
		this->MaxPos.x = std::stoi(value);
	} else if (key == "max_y") {
		this->MaxPos.y = std::stoi(value);
	} else if (key == "main_template") {
		value = FindAndReplaceString(value, "_", "-");
		CMapTemplate *main_template = CMapTemplate::Get(value);
		this->MainTemplate = main_template;
		main_template->Subtemplates.push_back(this);
		if (main_template->Plane) {
			this->Plane = main_template->Plane;
		}
		if (main_template->World) {
			this->World = main_template->World;
		}
		this->SurfaceLayer = main_template->SurfaceLayer;
	} else if (key == "upper_template") {
		value = FindAndReplaceString(value, "_", "-");
		CMapTemplate *upper_template = CMapTemplate::Get(value);
		if (upper_template) {
			this->UpperTemplate = upper_template;
			upper_template->LowerTemplate = this;
		}
	} else if (key == "lower_template") {
		value = FindAndReplaceString(value, "_", "-");
		CMapTemplate *lower_template = CMapTemplate::Get(value);
		if (lower_template) {
			this->LowerTemplate = lower_template;
			lower_template->UpperTemplate = this;
		}
	} else if (key == "adjacent_template") {
		value = FindAndReplaceString(value, "_", "-");
		CMapTemplate *adjacent_template = CMapTemplate::Get(value);
		if (adjacent_template) {
			this->AdjacentTemplates.push_back(adjacent_template);
			if (std::find(adjacent_template->AdjacentTemplates.begin(), adjacent_template->AdjacentTemplates.end(), this) == adjacent_template->AdjacentTemplates.end()) {
				adjacent_template->AdjacentTemplates.push_back(this);
			}
		}
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
	} else if (key == "output_terrain_image") {
		this->OutputTerrainImage = StringToBool(value);
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
bool CMapTemplate::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "generated_neutral_unit" || section->Tag == "player_location_generated_neutral_unit") {
		CUnitType *unit_type = nullptr;
		int quantity = 1;
			
		for (size_t j = 0; j < section->Properties.size(); ++j) {
			std::string key = section->Properties[j].first;
			std::string value = section->Properties[j].second;
			
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
			return true;
		}
		
		if (section->Tag == "generated_neutral_unit") {
			this->GeneratedNeutralUnits.push_back(std::pair<CUnitType *, int>(unit_type, quantity));
		} else if (section->Tag == "player_location_generated_neutral_unit") {
			this->PlayerLocationGeneratedNeutralUnits.push_back(std::pair<CUnitType *, int>(unit_type, quantity));
		}
	} else if (section->Tag == "generated_terrain") {
		CGeneratedTerrain *generated_terrain = new CGeneratedTerrain;
		
		generated_terrain->ProcessConfigData(section);
			
		if (!generated_terrain->TerrainType) {
			delete generated_terrain;
			return true;
		}
		
		this->GeneratedTerrains.push_back(generated_terrain);
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Initialize the map template
*/
void CMapTemplate::Initialize()
{
	this->Initialized = true;
	
	if (this->MainTemplate) { //if this is a subtemplate, re-sort the main template's subtemplates according to priority
		std::sort(this->MainTemplate->Subtemplates.begin(), this->MainTemplate->Subtemplates.end(), [](CMapTemplate *a, CMapTemplate *b) {
			if (a->Priority != b->Priority) {
				return a->Priority > b->Priority;
			} else {
				return a->Ident < b->Ident;
			}
		});
	}
}

void CMapTemplate::ApplyTerrainFile(bool overlay, Vec2i template_start_pos, Vec2i map_start_pos, int z) const
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
		if (y < template_start_pos.y || y >= (template_start_pos.y + CMap::Map.Info.MapHeights[z])) {
			y += 1;
			continue;
		}
		int x = 0;
		
		for (unsigned int i = 0; i < line_str.length(); ++i) {
			if (x < template_start_pos.x || x >= (template_start_pos.x + CMap::Map.Info.MapWidths[z])) {
				x += 1;
				continue;
			}
			std::string terrain_character = line_str.substr(i, 1);
			CTerrainType *terrain = nullptr;
			if (CTerrainType::TerrainTypesByCharacter.find(terrain_character) != CTerrainType::TerrainTypesByCharacter.end()) {
				terrain = CTerrainType::TerrainTypesByCharacter.find(terrain_character)->second;
			}
			if (terrain) {
				Vec2i real_pos(map_start_pos.x + x - template_start_pos.x, map_start_pos.y + y - template_start_pos.y);
				CMap::Map.Field(real_pos, z)->SetTerrain(terrain);
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

void CMapTemplate::ApplyTerrainImage(bool overlay, Vec2i template_start_pos, Vec2i map_start_pos, int z) const
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
		if (y < template_start_pos.y || y >= (template_start_pos.y + (CMap::Map.Info.MapHeights[z] / this->Scale))) {
			continue;
		}
		
		for (int x = 0; x < terrain_image->Width; ++x) {
			if (x < template_start_pos.x || x >= (template_start_pos.x + (CMap::Map.Info.MapWidths[z] / this->Scale))) {
				continue;
			}

			Uint32 c = *reinterpret_cast<Uint32 *>(&reinterpret_cast<Uint8 *>(terrain_image->Surface->pixels)[x * 4 + y * terrain_image->Surface->pitch]);
			Uint8 a;

			Video.GetRGBA(c, terrain_image->Surface->format, &r, &g, &b, &a);
			
			if (a == 0) { //transparent pixels means leaving the area as it is (e.g. if it is a subtemplate use the main template's terrain for this tile instead)
				continue;
			}

			CTerrainType *terrain = nullptr;
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

					if (!CMap::Map.Info.IsPointOnMap(real_pos, z)) {
						continue;
					}
					
					if (terrain) {
						CMap::Map.Field(real_pos, z)->SetTerrain(terrain);
						
						if (terrain_feature_id != -1) {
							CMap::Map.Field(real_pos, z)->TerrainFeature = TerrainFeatures[terrain_feature_id];
						}
					} else {
						if (r != 0 || g != 0 || b != 0 || !overlay) { //fully black pixels represent areas in overlay terrain files that don't have any overlays
							fprintf(stderr, "Invalid map terrain: (%d, %d) (RGB: %d/%d/%d)\n", x, y, r, g, b);
						} else if (overlay && CMap::Map.Field(real_pos, z)->OverlayTerrain) { //fully black pixel in overlay terrain map = no overlay
							CMap::Map.Field(real_pos, z)->RemoveOverlayTerrain();
						}
					}
				}
			}
		}
	}
	SDL_UnlockSurface(terrain_image->Surface);
	
	CGraphic::Free(terrain_image);
}

void CMapTemplate::Apply(Vec2i template_start_pos, Vec2i map_start_pos, int z) const
{
	if (SaveGameLoading) {
		return;
	}
	
	if (template_start_pos.x < 0 || template_start_pos.x >= this->Width || template_start_pos.y < 0 || template_start_pos.y >= this->Height) {
		fprintf(stderr, "Invalid map coordinate for map template \"%s\": (%d, %d)\n", this->Ident.c_str(), template_start_pos.x, template_start_pos.y);
		return;
	}
	
	const CCampaign *current_campaign = CCampaign::GetCurrentCampaign();
	
	if (z >= (int) CMap::Map.MapLayers.size()) {
		int width = std::min(this->Width * this->Scale, CMap::Map.Info.MapWidth);
		int height = std::min(this->Height * this->Scale, CMap::Map.Info.MapHeight);
		if (current_campaign) {
			//applies the map size set for the campaign for this map layer; for the first map layer that is already CMap::Map.Info.Width/Height, so it isn't necessary here
			width = current_campaign->MapSizes[z].x;
			height = current_campaign->MapSizes[z].y;
		}
	
		CMapLayer *map_layer = new CMapLayer(width, height);
		map_layer->ID = CMap::Map.MapLayers.size();
		CMap::Map.Info.MapWidths.push_back(map_layer->GetWidth());
		CMap::Map.Info.MapHeights.push_back(map_layer->GetHeight());
		map_layer->Plane = this->Plane;
		map_layer->World = this->World;
		map_layer->SurfaceLayer = this->SurfaceLayer;
		map_layer->PixelTileSize = this->PixelTileSize;
		map_layer->Overland = this->Overland;
		CMap::Map.MapLayers.push_back(map_layer);
	} else {
		if (!this->IsSubtemplateArea()) {
			CMap::Map.MapLayers[z]->Plane = this->Plane;
			CMap::Map.MapLayers[z]->World = this->World;
			CMap::Map.MapLayers[z]->SurfaceLayer = this->SurfaceLayer;
			CMap::Map.MapLayers[z]->PixelTileSize = this->PixelTileSize;
			CMap::Map.MapLayers[z]->Overland = this->Overland;
		}
	}

	if (!this->IsSubtemplateArea()) {
		if (Editor.Running == EditorNotRunning) {
			if (this->World && this->World->SeasonSchedule) {
				CMap::Map.MapLayers[z]->SeasonSchedule = this->World->SeasonSchedule;
			} else if (!this->World && this->Plane && this->Plane->SeasonSchedule) {
				CMap::Map.MapLayers[z]->SeasonSchedule = this->Plane->SeasonSchedule;
			} else {
				CMap::Map.MapLayers[z]->SeasonSchedule = CSeasonSchedule::DefaultSeasonSchedule;
			}
			
			CMap::Map.MapLayers[z]->SetSeasonByHours(CDate::CurrentTotalHours);
			
			CMap::Map.MapLayers[z]->TimeOfDaySchedule = nullptr;
			CMap::Map.MapLayers[z]->SetTimeOfDay(nullptr);
			
			if (
				this->SurfaceLayer == 0
				&& !GameSettings.Inside
				&& !GameSettings.NoTimeOfDay
			) {
				if (this->World && this->World->TimeOfDaySchedule) {
					CMap::Map.MapLayers[z]->TimeOfDaySchedule = this->World->TimeOfDaySchedule;
				} else if (!this->World && this->Plane && this->Plane->TimeOfDaySchedule) {
					CMap::Map.MapLayers[z]->TimeOfDaySchedule = this->Plane->TimeOfDaySchedule;
				} else {
					CMap::Map.MapLayers[z]->TimeOfDaySchedule = CTimeOfDaySchedule::DefaultTimeOfDaySchedule;
				}
				
				CMap::Map.MapLayers[z]->SetTimeOfDayByHours(CDate::CurrentTotalHours);
			}
		}
	}
	
	Vec2i map_end(std::min(CMap::Map.Info.MapWidths[z], map_start_pos.x + (this->Width * this->Scale)), std::min(CMap::Map.Info.MapHeights[z], map_start_pos.y + (this->Height * this->Scale)));
	if (!CMap::Map.Info.IsPointOnMap(map_start_pos, z)) {
		fprintf(stderr, "Invalid map coordinate for map template \"%s\": (%d, %d)\n", this->Ident.c_str(), map_start_pos.x, map_start_pos.y);
		return;
	}
	
	bool has_base_map = !this->TerrainFile.empty() || !this->TerrainImage.empty();
	
	ShowLoadProgress(_("Applying \"%s\" Map Template Terrain"), this->Name.c_str());
	
	if (this->BaseTerrainType) {
		for (int x = map_start_pos.x; x < map_end.x; ++x) {
			for (int y = map_start_pos.y; y < map_end.y; ++y) {
				Vec2i tile_pos(x, y);
				CMap::Map.Field(tile_pos, z)->SetTerrain(this->BaseTerrainType);
				
				if (this->BaseOverlayTerrainType) {
					CMap::Map.Field(tile_pos, z)->SetTerrain(this->BaseOverlayTerrainType);
				} else {
					CMap::Map.Field(tile_pos, z)->RemoveOverlayTerrain();
				}
			}
		}
	}
	
	this->ApplyTerrainImage(false, template_start_pos, map_start_pos, z);
	this->ApplyTerrainImage(true, template_start_pos, map_start_pos, z);
	
	if (this->OutputTerrainImage) {
		std::string filename = this->Ident;
		filename = FindAndReplaceString(filename, "-", "_");
		filename += ".png";
		
		std::string overlay_filename = this->Ident;
		overlay_filename = FindAndReplaceString(overlay_filename, "-", "_");
		overlay_filename += "_overlay";
		overlay_filename += ".png";
		
		SaveMapTemplatePNG(filename.c_str(), this, false);
		SaveMapTemplatePNG(overlay_filename.c_str(), this, true);
	}

	if (current_campaign) {
		for (size_t i = 0; i < HistoricalTerrains.size(); ++i) {
			Vec2i history_pos = std::get<0>(HistoricalTerrains[i]);
			if (history_pos.x < template_start_pos.x || history_pos.x >= (template_start_pos.x + (CMap::Map.Info.MapWidths[z] / this->Scale)) || history_pos.y < template_start_pos.y || history_pos.y >= (template_start_pos.y + (CMap::Map.Info.MapHeights[z] / this->Scale))) {
				continue;
			}
			if (current_campaign->GetStartDate().ContainsDate(std::get<2>(HistoricalTerrains[i])) || std::get<2>(HistoricalTerrains[i]).Year == 0) {
				CTerrainType *historical_terrain = std::get<1>(HistoricalTerrains[i]);
				
				for (int sub_x = 0; sub_x < this->Scale; ++sub_x) {
					for (int sub_y = 0; sub_y < this->Scale; ++sub_y) {
						Vec2i real_pos(map_start_pos.x + ((history_pos.x - template_start_pos.x) * this->Scale) + sub_x, map_start_pos.y + ((history_pos.y - template_start_pos.y) * this->Scale) + sub_y);

						if (!CMap::Map.Info.IsPointOnMap(real_pos, z)) {
							continue;
						}
						
						if (historical_terrain) {
							if (historical_terrain->Overlay && ((historical_terrain->Flags & MapFieldRoad) || (historical_terrain->Flags & MapFieldRailroad)) && !(CMap::Map.Field(real_pos, z)->Flags & MapFieldLandAllowed)) {
								continue;
							}
							CMap::Map.Field(real_pos, z)->SetTerrain(historical_terrain);
						} else { //if the terrain type is null, then that means a previously set overlay terrain should be removed
							CMap::Map.Field(real_pos, z)->RemoveOverlayTerrain();
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
				if (!CMap::Map.Info.IsPointOnMap(surrounding_pos, z) || CMap::Map.IsPointInASubtemplateArea(surrounding_pos, z)) {
					continue;
				}
				CMap::Map.Field(surrounding_pos, z)->SetTerrain(this->SurroundingTerrainType);
			}
		}
		for (int x = surrounding_start_pos.x; x < surrounding_end.x; x += (surrounding_end.x - surrounding_start_pos.x - 1)) {
			for (int y = surrounding_start_pos.y; y < surrounding_end.y; ++y) {
				Vec2i surrounding_pos(x, y);
				if (!CMap::Map.Info.IsPointOnMap(surrounding_pos, z) || CMap::Map.IsPointInASubtemplateArea(surrounding_pos, z)) {
					continue;
				}
				CMap::Map.Field(surrounding_pos, z)->SetTerrain(this->SurroundingTerrainType);
			}
		}
	}
	
	if (current_campaign) {
		CFaction *current_faction = current_campaign->GetFaction();
		if (current_faction && !this->IsSubtemplateArea() && CPlayer::GetThisPlayer()->GetFaction() != current_faction) {
			CPlayer::GetThisPlayer()->SetCivilization(current_faction->Civilization->GetIndex());
			CPlayer::GetThisPlayer()->SetFaction(current_faction);
			CPlayer::GetThisPlayer()->Resources[CopperCost] = 2500; // give the player enough resources to start up
			CPlayer::GetThisPlayer()->Resources[WoodCost] = 2500;
			CPlayer::GetThisPlayer()->Resources[StoneCost] = 2500;
		}
	}
	
	this->ApplySubtemplates(template_start_pos, map_start_pos, z, false);
	this->ApplySubtemplates(template_start_pos, map_start_pos, z, true);
	
	if (!has_base_map) {
		ShowLoadProgress(_("Generating \"%s\" Map Template Random Terrain"), this->Name.c_str());
		
		for (const CGeneratedTerrain *generated_terrain : this->GeneratedTerrains) {
			int map_width = (map_end.x - map_start_pos.x);
			int map_height = (map_end.y - map_start_pos.y);
			
			CMap::Map.GenerateTerrain(generated_terrain, map_start_pos, map_end - Vec2i(1, 1), has_base_map, z);
		}
	}
	
	if (!this->IsSubtemplateArea()) {
		CMap::Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		CMap::Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
		CMap::Map.AdjustTileMapTransitions(map_start_pos, map_end, z);
		CMap::Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		CMap::Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
	}
	
	ShowLoadProgress(_("Applying \"%s\" Map Template Units"), this->Name.c_str());

	for (std::map<std::pair<int, int>, std::tuple<CUnitType *, int, CUniqueItem *>>::const_iterator iterator = this->Resources.begin(); iterator != this->Resources.end(); ++iterator) {
		Vec2i unit_raw_pos(iterator->first.first, iterator->first.second);
		Vec2i unit_pos(map_start_pos.x + unit_raw_pos.x - template_start_pos.x, map_start_pos.y + unit_raw_pos.y - template_start_pos.y);
		if (!CMap::Map.Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		
		const CUnitType *type = std::get<0>(iterator->second);
		
		Vec2i unit_offset((type->TileSize - 1) / 2);
		
		if (!OnTopDetails(*type, nullptr) && !UnitTypeCanBeAt(*type, unit_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(unit_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(unit_pos - unit_offset + Vec2i(type->TileSize - 1), z)) {
			fprintf(stderr, "Unit \"%s\" should be placed on (%d, %d) for map template \"%s\", but it cannot be there.\n", type->Ident.c_str(), unit_raw_pos.x, unit_raw_pos.y, this->Ident.c_str());
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

	if (current_campaign != nullptr) {
		this->ApplyConnectors(template_start_pos, map_start_pos, z);
	}
	this->ApplySites(template_start_pos, map_start_pos, z);
	this->ApplyUnits(template_start_pos, map_start_pos, z);
	
	if (has_base_map) {
		ShowLoadProgress(_("Generating \"%s\" Map Template Random Terrain"), this->Name.c_str());
		
		for (const CGeneratedTerrain *generated_terrain : this->GeneratedTerrains) {
			int map_width = (map_end.x - map_start_pos.x);
			int map_height = (map_end.y - map_start_pos.y);
			
			CMap::Map.GenerateTerrain(generated_terrain, map_start_pos, map_end - Vec2i(1, 1), has_base_map, z);
		}
	}
	
	if (!this->IsSubtemplateArea()) {
		CMap::Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		CMap::Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
		CMap::Map.AdjustTileMapTransitions(map_start_pos, map_end, z);
		CMap::Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		CMap::Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
	}
	
	ShowLoadProgress(_("Generating \"%s\" Map Template Random Units"), this->Name.c_str());

	// now, generate the units and heroes that were set to be generated at a random position (by having their position set to {-1, -1})
	if (current_campaign != nullptr) {
		this->ApplyConnectors(template_start_pos, map_start_pos, z, true);
	}
	this->ApplySites(template_start_pos, map_start_pos, z, true);
	this->ApplyUnits(template_start_pos, map_start_pos, z, true);

	for (int i = 0; i < PlayerMax; ++i) {
		if (CPlayer::Players[i]->Type != PlayerPerson && CPlayer::Players[i]->Type != PlayerComputer && CPlayer::Players[i]->Type != PlayerRescueActive) {
			continue;
		}
		if (CMap::Map.IsPointInASubtemplateArea(CPlayer::Players[i]->StartPos, z)) {
			continue;
		}
		if (CPlayer::Players[i]->StartPos.x < map_start_pos.x || CPlayer::Players[i]->StartPos.y < map_start_pos.y || CPlayer::Players[i]->StartPos.x >= map_end.x || CPlayer::Players[i]->StartPos.y >= map_end.y || CPlayer::Players[i]->StartMapLayer != z) {
			continue;
		}
		if (CPlayer::Players[i]->StartPos.x == 0 && CPlayer::Players[i]->StartPos.y == 0) {
			continue;
		}
		// add five workers at the player's starting location
		if (CPlayer::Players[i]->NumTownHalls > 0) {
			int worker_type_id = CFaction::GetFactionClassUnitType(CPlayer::Players[i]->GetFaction(), GetUnitTypeClassIndexByName("worker"));
			if (worker_type_id != -1 && CPlayer::Players[i]->GetUnitTypeCount(CUnitType::UnitTypes[worker_type_id]) == 0) { //only create if the player doesn't have any workers created in another manner
				Vec2i worker_unit_offset((CUnitType::UnitTypes[worker_type_id]->TileSize - 1) / 2);
				
				Vec2i worker_pos(CPlayer::Players[i]->StartPos);

				bool start_pos_has_town_hall = false;
				std::vector<CUnit *> table;
				Select(worker_pos - Vec2i(4, 4), worker_pos + Vec2i(4, 4), table, z, HasSamePlayerAs(*CPlayer::Players[i]));
				for (size_t j = 0; j < table.size(); ++j) {
					if (table[j]->Type->BoolFlag[TOWNHALL_INDEX].value) {
						start_pos_has_town_hall = true;
						break;
					}
				}
				
				if (!start_pos_has_town_hall) { //if the start pos doesn't have a town hall, create the workers in the position of a town hall the player has
					for (int j = 0; j < CPlayer::Players[i]->GetUnitCount(); ++j) {
						CUnit *town_hall_unit = &CPlayer::Players[i]->GetUnit(j);
						if (!town_hall_unit->Type->BoolFlag[TOWNHALL_INDEX].value) {
							continue;
						}
						if (town_hall_unit->MapLayer->ID != z) {
							continue;
						}
						worker_pos = town_hall_unit->tilePos;
					}
				}
				
				for (int j = 0; j < 5; ++j) {
					CUnit *worker_unit = CreateUnit(worker_pos, *CUnitType::UnitTypes[worker_type_id], CPlayer::Players[i], CPlayer::Players[i]->StartMapLayer);
				}
			}
		}
		
		if (CPlayer::Players[i]->NumTownHalls > 0 || CPlayer::Players[i]->Index == CPlayer::GetThisPlayer()->Index) {
			for (size_t j = 0; j < this->PlayerLocationGeneratedNeutralUnits.size(); ++j) {
				CMap::Map.GenerateNeutralUnits(this->PlayerLocationGeneratedNeutralUnits[j].first, this->PlayerLocationGeneratedNeutralUnits[j].second, CPlayer::Players[i]->StartPos - Vec2i(8, 8), CPlayer::Players[i]->StartPos + Vec2i(8, 8), true, z);
			}
		}
	}
	
	for (size_t i = 0; i < this->GeneratedNeutralUnits.size(); ++i) {
		bool grouped = this->GeneratedNeutralUnits[i].first->GivesResource && this->GeneratedNeutralUnits[i].first->TileSize.x == 1 && this->GeneratedNeutralUnits[i].first->TileSize.y == 1; // group small resources
		CMap::Map.GenerateNeutralUnits(this->GeneratedNeutralUnits[i].first, this->GeneratedNeutralUnits[i].second, map_start_pos, map_end - Vec2i(1, 1), grouped, z);
	}
}

/**
**	@brief	Apply the subtemplates to the map
**
**	@param	template_start_pos	The start position of the map relative to the map template
**	@param	map_start_pos		The start position of the map template relative to the map
**	@param	z					The map layer
**	@param	random				Whether it is subtemplates with a random position that should be applied, or ones with a fixed one
*/
void CMapTemplate::ApplySubtemplates(const Vec2i &template_start_pos, const Vec2i &map_start_pos, const int z, const bool random) const
{
	Vec2i map_end(std::min(CMap::Map.Info.MapWidths[z], map_start_pos.x + this->Width), std::min(CMap::Map.Info.MapHeights[z], map_start_pos.y + this->Height));
	
	for (size_t i = 0; i < this->Subtemplates.size(); ++i) {
		CMapTemplate *subtemplate = this->Subtemplates[i];
		Vec2i subtemplate_pos(subtemplate->SubtemplatePosition - Vec2i((subtemplate->Width - 1) / 2, (subtemplate->Height - 1) / 2));
		bool found_location = false;
		
		if (subtemplate->UpperTemplate && (subtemplate_pos.x < 0 || subtemplate_pos.y < 0)) { //if has no given position, but has an upper template, use its coordinates instead
			subtemplate_pos = CMap::Map.GetSubtemplatePos(subtemplate->UpperTemplate);
			if (subtemplate_pos.x >= 0 && subtemplate_pos.y >= 0) {
				found_location = true;
			}
		}
		
		if (subtemplate->LowerTemplate && (subtemplate_pos.x < 0 || subtemplate_pos.y < 0)) { //if has no given position, but has a lower template, use its coordinates instead
			subtemplate_pos = CMap::Map.GetSubtemplatePos(subtemplate->LowerTemplate);
			if (subtemplate_pos.x >= 0 && subtemplate_pos.y >= 0) {
				found_location = true;
			}
		}
		
		if (!found_location) {
			if (subtemplate_pos.x < 0 || subtemplate_pos.y < 0) {
				if (!random) {
					continue;
				}
				Vec2i min_pos(map_start_pos);
				Vec2i max_pos(map_end.x - subtemplate->Width, map_end.y - subtemplate->Height);
				
				if (subtemplate->MinPos.x != -1) {
					min_pos.x += subtemplate->MinPos.x;
					min_pos.x -= template_start_pos.x;
				}
				if (subtemplate->MinPos.y != -1) {
					min_pos.y += subtemplate->MinPos.y;
					min_pos.y -= template_start_pos.y;
				}
				
				if (subtemplate->MaxPos.x != -1) {
					max_pos.x += subtemplate->MaxPos.x;
					max_pos.x -= this->Width;
				}
				if (subtemplate->MaxPos.y != -1) {
					max_pos.y += subtemplate->MaxPos.y;
					max_pos.y -= this->Height;
				}
				
				//bound the minimum and maximum positions depending on which other templates should be adjacent to this one (if they have already been applied to the map)
				for (const CMapTemplate *adjacent_template : subtemplate->AdjacentTemplates) {
					Vec2i adjacent_template_pos = CMap::Map.GetSubtemplatePos(adjacent_template);
					
					if (!CMap::Map.Info.IsPointOnMap(adjacent_template_pos, z)) {
						continue;
					}
					
					Vec2i min_adjacency_pos = adjacent_template_pos - MaxAdjacentTemplateDistance - Vec2i(subtemplate->Width, subtemplate->Height);
					Vec2i max_adjacency_pos = adjacent_template_pos + Vec2i(adjacent_template->Width, adjacent_template->Height) + MaxAdjacentTemplateDistance;
					min_pos.x = std::max(min_pos.x, min_adjacency_pos.x);
					min_pos.y = std::max(min_pos.y, min_adjacency_pos.y);
					max_pos.x = std::min(max_pos.x, max_adjacency_pos.x);
					max_pos.y = std::min(max_pos.y, max_adjacency_pos.y);
				}
				
				std::vector<Vec2i> potential_positions;
				for (int x = min_pos.x; x <= max_pos.x; ++x) {
					for (int y = min_pos.y; y <= max_pos.y; ++y) {
						potential_positions.push_back(Vec2i(x, y));
					}
				}
				
				while (!potential_positions.empty()) {
					subtemplate_pos = potential_positions[SyncRand(potential_positions.size())];
					potential_positions.erase(std::remove(potential_positions.begin(), potential_positions.end(), subtemplate_pos), potential_positions.end());

					bool on_map = CMap::Map.Info.IsPointOnMap(subtemplate_pos, z) && CMap::Map.Info.IsPointOnMap(Vec2i(subtemplate_pos.x + subtemplate->Width - 1, subtemplate_pos.y + subtemplate->Height - 1), z);

					bool on_subtemplate_area = false;
					for (int x = 0; x < subtemplate->Width; ++x) {
						for (int y = 0; y < subtemplate->Height; ++y) {
							if (CMap::Map.IsPointInASubtemplateArea(subtemplate_pos + Vec2i(x, y), z)) {
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
				}
			}
			else {
				if (random) {
					continue;
				}
				subtemplate_pos.x = map_start_pos.x + subtemplate_pos.x - template_start_pos.x;
				subtemplate_pos.y = map_start_pos.y + subtemplate_pos.y - template_start_pos.y;
				found_location = true;
			}
		}
		else {
			if (random) {
				continue;
			}
		}
		
		if (found_location) {
			if (subtemplate_pos.x >= 0 && subtemplate_pos.y >= 0 && subtemplate_pos.x < CMap::Map.Info.MapWidths[z] && subtemplate_pos.y < CMap::Map.Info.MapHeights[z]) {
				subtemplate->Apply(Vec2i(0, 0), subtemplate_pos, z);
				
				CMap::Map.MapLayers[z]->SubtemplateAreas.push_back(std::tuple<Vec2i, Vec2i, CMapTemplate *>(subtemplate_pos, Vec2i(subtemplate_pos.x + subtemplate->Width - 1, subtemplate_pos.y + subtemplate->Height - 1), subtemplate));
			}
		}
	}
}

/**
**	@brief	Apply sites to the map
**
**	@param	template_start_pos	The start position of the map relative to the map template
**	@param	map_start_pos		The start position of the map template relative to the map
**	@param	z					The map layer
**	@param	random				Whether it is sites with a random position that should be applied, or ones with a fixed one
*/
void CMapTemplate::ApplySites(const Vec2i &template_start_pos, const Vec2i &map_start_pos, const int z, const bool random) const
{
	Vec2i map_end(std::min(CMap::Map.Info.MapWidths[z], map_start_pos.x + this->Width), std::min(CMap::Map.Info.MapHeights[z], map_start_pos.y + this->Height));
	const CCampaign *current_campaign = CCampaign::GetCurrentCampaign();
	CDate start_date;
	if (current_campaign) {
		start_date = current_campaign->GetStartDate();
	}

	for (size_t site_index = 0; site_index < this->Sites.size(); ++site_index) {
		CSite *site = this->Sites[site_index];
		
		Vec2i site_raw_pos(site->Position);
		Vec2i site_pos(map_start_pos + ((site_raw_pos - template_start_pos) * this->Scale));

		Vec2i unit_offset((SettlementSiteUnitType->TileSize - 1) / 2);
			
		if (random) {
			if (site_raw_pos.x != -1 || site_raw_pos.y != -1) {
				continue;
			}
			if (SettlementSiteUnitType) {
				site_pos = CMap::Map.GenerateUnitLocation(SettlementSiteUnitType, nullptr, map_start_pos, map_end - Vec2i(1, 1), z);
				site_pos += unit_offset;
			}
		} else {
			if (site_raw_pos.x == -1 && site_raw_pos.y == -1) {
				continue;
			}
		}

		if (!CMap::Map.Info.IsPointOnMap(site_pos, z) || site_pos.x < map_start_pos.x || site_pos.y < map_start_pos.y) {
			continue;
		}

		if (site->Major && SettlementSiteUnitType) { //add a settlement site for major sites
			if (!UnitTypeCanBeAt(*SettlementSiteUnitType, site_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(site_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(site_pos - unit_offset + Vec2i(SettlementSiteUnitType->TileSize - 1), z)) {
				fprintf(stderr, "The settlement site for \"%s\" should be placed on (%d, %d), but it cannot be there.\n", site->Ident.c_str(), site_raw_pos.x, site_raw_pos.y);
			}
			CUnit *unit = CreateUnit(site_pos - unit_offset, *SettlementSiteUnitType, CPlayer::Players[PlayerNumNeutral], z, true);
			unit->Settlement = site;
			unit->Settlement->SiteUnit = unit;
			CMap::Map.SiteUnits.push_back(unit);
		}
		
		for (size_t j = 0; j < site->HistoricalResources.size(); ++j) {
			if (
				(!current_campaign && std::get<1>(site->HistoricalResources[j]).Year == 0 && std::get<1>(site->HistoricalResources[j]).Year == 0)
				|| (
					current_campaign && start_date.ContainsDate(std::get<0>(site->HistoricalResources[j]))
					&& (!start_date.ContainsDate(std::get<1>(site->HistoricalResources[j])) || std::get<1>(site->HistoricalResources[j]).Year == 0)
				)
			) {
				const CUnitType *type = std::get<2>(site->HistoricalResources[j]);
				if (!type) {
					fprintf(stderr, "Error in CMap::ApplySites (site ident \"%s\"): historical resource type is null.\n", site->Ident.c_str());
					continue;
				}
				Vec2i unit_offset((type->TileSize - 1) / 2);
				CUnit *unit = CreateResourceUnit(site_pos - unit_offset, *type, z, false); // don't generate unique resources when setting special properties, since for map templates unique resources are supposed to be explicitly indicated
				if (std::get<3>(site->HistoricalResources[j])) {
					unit->SetUnique(std::get<3>(site->HistoricalResources[j]));
				}
				int resource_quantity = std::get<4>(site->HistoricalResources[j]);
				if (resource_quantity) { //set the resource_quantity after setting the unique unit, so that unique resources can be decreased in quantity over time
					unit->SetResourcesHeld(resource_quantity);
					unit->Variable[GIVERESOURCE_INDEX].Value = resource_quantity;
					unit->Variable[GIVERESOURCE_INDEX].Max = resource_quantity;
					unit->Variable[GIVERESOURCE_INDEX].Enable = 1;
				}
			}
		}
		
		if (!current_campaign) {
			continue;
		}
		
		const CFaction *site_owner = nullptr;
		for (std::map<CDate, const CFaction *>::reverse_iterator owner_iterator = site->HistoricalOwners.rbegin(); owner_iterator != site->HistoricalOwners.rend(); ++owner_iterator) {
			if (start_date.ContainsDate(owner_iterator->first)) { // set the owner to the latest historical owner given the scenario's start date
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
			if (start_date.ContainsDate(site_owner->HistoricalCapitals[i].first) || site_owner->HistoricalCapitals[i].first.Year == 0) {
				if (site_owner->HistoricalCapitals[i].second == site->Ident) {
					is_capital = true;
				}
				break;
			}
		}
		
		if ((player->StartPos.x == 0 && player->StartPos.y == 0) || is_capital) {
			player->SetStartView(site_pos, z);
		}
		
		CUnitType *pathway_type = nullptr;
		for (size_t j = 0; j < site->HistoricalBuildings.size(); ++j) {
			if (
				start_date.ContainsDate(std::get<0>(site->HistoricalBuildings[j]))
				&& (!start_date.ContainsDate(std::get<1>(site->HistoricalBuildings[j])) || std::get<1>(site->HistoricalBuildings[j]).Year == 0)
			) {
				int unit_type_id = -1;
				unit_type_id = CFaction::GetFactionClassUnitType(site_owner, std::get<2>(site->HistoricalBuildings[j]));
				if (unit_type_id == -1) {
					continue;
				}
				const CUnitType *type = CUnitType::UnitTypes[unit_type_id];
				if (type->TerrainType) {
					if ((type->TerrainType->Flags & MapFieldRoad) || (type->TerrainType->Flags & MapFieldRailroad)) {
						pathway_type = CUnitType::UnitTypes[unit_type_id];
					}
				}
			}
		}
		
		bool first_building = true;
		for (size_t j = 0; j < site->HistoricalBuildings.size(); ++j) {
			if (
				start_date.ContainsDate(std::get<0>(site->HistoricalBuildings[j]))
				&& (!start_date.ContainsDate(std::get<1>(site->HistoricalBuildings[j])) || std::get<1>(site->HistoricalBuildings[j]).Year == 0)
			) {
				const CFaction *building_owner = std::get<4>(site->HistoricalBuildings[j]);
				int unit_type_id = -1;
				if (building_owner) {
					unit_type_id = CFaction::GetFactionClassUnitType(building_owner, std::get<2>(site->HistoricalBuildings[j]));
				} else {
					unit_type_id = CFaction::GetFactionClassUnitType(site_owner, std::get<2>(site->HistoricalBuildings[j]));
				}
				if (unit_type_id == -1) {
					continue;
				}
				const CUnitType *type = CUnitType::UnitTypes[unit_type_id];
				if (type->TerrainType) {
					continue;
				}
				if (type->BoolFlag[TOWNHALL_INDEX].value && !site->Major) {
					fprintf(stderr, "Error in CMap::ApplySites (site ident \"%s\"): site has a town hall, but isn't set as a major one.\n", site->Ident.c_str());
					continue;
				}
				Vec2i unit_offset((type->TileSize - 1) / 2);
				if (first_building) {
					if (!OnTopDetails(*type, nullptr) && !UnitTypeCanBeAt(*type, site_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(site_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(site_pos - unit_offset + Vec2i(type->TileSize - 1), z)) {
						fprintf(stderr, "The \"%s\" representing the minor site of \"%s\" should be placed on (%d, %d), but it cannot be there.\n", type->Ident.c_str(), site->Ident.c_str(), site_raw_pos.x, site_raw_pos.y);
					}
				}
				CUnit *unit = nullptr;
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
				if (std::get<3>(site->HistoricalBuildings[j])) {
					unit->SetUnique(std::get<3>(site->HistoricalBuildings[j]));
				}
				if (first_building) {
					if (!type->BoolFlag[TOWNHALL_INDEX].value && !unit->Unique && (!building_owner || building_owner == site_owner)) { //if one building is representing a minor site, make it have the site's name
						unit->Name = site->GetCulturalName(site_owner->Civilization);
					}
					first_building = false;
				}
				if (type->BoolFlag[TOWNHALL_INDEX].value && (!building_owner || building_owner == site_owner)) {
					unit->UpdateBuildingSettlementAssignment();
				}
				if (pathway_type) {
					for (int x = unit->tilePos.x - 1; x < unit->tilePos.x + unit->Type->TileSize.x + 1; ++x) {
						for (int y = unit->tilePos.y - 1; y < unit->tilePos.y + unit->Type->TileSize.y + 1; ++y) {
							if (!CMap::Map.Info.IsPointOnMap(x, y, unit->MapLayer)) {
								continue;
							}
							CMapField &mf = *unit->MapLayer->Field(x, y);
							if (mf.Flags & MapFieldBuilding) { //this is a tile where the building itself is located, continue
								continue;
							}
							Vec2i pathway_pos(x, y);
							if (!UnitTypeCanBeAt(*pathway_type, pathway_pos, unit->MapLayer->ID)) {
								continue;
							}
							
							mf.SetTerrain(pathway_type->TerrainType);
						}
					}
				}
			}
		}
		
		for (size_t j = 0; j < site->HistoricalUnits.size(); ++j) {
			if (
				start_date.ContainsDate(std::get<0>(site->HistoricalUnits[j]))
				&& (!start_date.ContainsDate(std::get<1>(site->HistoricalUnits[j])) || std::get<1>(site->HistoricalUnits[j]).Year == 0)
			) {
				int unit_quantity = std::get<3>(site->HistoricalUnits[j]);
						
				if (unit_quantity > 0) {
					const CUnitType *type = std::get<2>(site->HistoricalUnits[j]);
					if (type->BoolFlag[ORGANIC_INDEX].value) {
						unit_quantity = std::max(1, unit_quantity / PopulationPerUnit); //each organic unit represents 1,000 people
					}
							
					CPlayer *unit_player = nullptr;
					const CFaction *unit_owner = std::get<4>(site->HistoricalUnits[j]);
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

void CMapTemplate::ApplyConnectors(Vec2i template_start_pos, Vec2i map_start_pos, int z, bool random) const
{
	Vec2i map_end(std::min(CMap::Map.Info.MapWidths[z], map_start_pos.x + this->Width), std::min(CMap::Map.Info.MapHeights[z], map_start_pos.y + this->Height));

	for (size_t i = 0; i < this->PlaneConnectors.size(); ++i) {
		const CUnitType *type = std::get<1>(this->PlaneConnectors[i]);
		Vec2i unit_raw_pos(std::get<0>(this->PlaneConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		Vec2i unit_offset((type->TileSize - 1) / 2);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = CMap::Map.GenerateUnitLocation(type, nullptr, map_start_pos, map_end - Vec2i(1, 1), z);
			unit_pos += unit_offset;
		}
		if (!CMap::Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x || unit_pos.y < map_start_pos.y) {
			continue;
		}
		
		if (!OnTopDetails(*type, nullptr) && !UnitTypeCanBeAt(*type, unit_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(unit_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(unit_pos - unit_offset + Vec2i(type->TileSize - 1), z)) {
			fprintf(stderr, "Unit \"%s\" should be placed on (%d, %d) for map template \"%s\", but it cannot be there.\n", type->Ident.c_str(), unit_raw_pos.x, unit_raw_pos.y, this->Ident.c_str());
		}

		CUnit *unit = CreateUnit(unit_pos - unit_offset, *type, CPlayer::Players[PlayerNumNeutral], z, true);
		if (std::get<3>(this->PlaneConnectors[i])) {
			unit->SetUnique(std::get<3>(this->PlaneConnectors[i]));
		}
		CMap::Map.MapLayers[z]->LayerConnectors.push_back(unit);
		for (size_t second_z = 0; second_z < CMap::Map.MapLayers.size(); ++second_z) {
			bool found_other_connector = false;
			if (CMap::Map.MapLayers[second_z]->Plane == std::get<2>(this->PlaneConnectors[i])) {
				for (size_t j = 0; j < CMap::Map.MapLayers[second_z]->LayerConnectors.size(); ++j) {
					if (CMap::Map.MapLayers[second_z]->LayerConnectors[j]->Type == unit->Type && CMap::Map.MapLayers[second_z]->LayerConnectors[j]->Unique == unit->Unique && CMap::Map.MapLayers[second_z]->LayerConnectors[j]->ConnectingDestination == nullptr) {
						CMap::Map.MapLayers[second_z]->LayerConnectors[j]->ConnectingDestination = unit;
						unit->ConnectingDestination = CMap::Map.MapLayers[second_z]->LayerConnectors[j];
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
		Vec2i unit_offset((type->TileSize - 1) / 2);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = CMap::Map.GenerateUnitLocation(type, nullptr, map_start_pos, map_end - Vec2i(1, 1), z);
			unit_pos += unit_offset;
		}
		if (!CMap::Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x || unit_pos.y < map_start_pos.y) {
			continue;
		}
		
		if (!OnTopDetails(*type, nullptr) && !UnitTypeCanBeAt(*type, unit_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(unit_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(unit_pos - unit_offset + Vec2i(type->TileSize - 1), z)) {
			fprintf(stderr, "Unit \"%s\" should be placed on (%d, %d) for map template \"%s\", but it cannot be there.\n", type->Ident.c_str(), unit_raw_pos.x, unit_raw_pos.y, this->Ident.c_str());
		}

		CUnit *unit = CreateUnit(unit_pos - unit_offset, *type, CPlayer::Players[PlayerNumNeutral], z, true);
		if (std::get<3>(this->WorldConnectors[i])) {
			unit->SetUnique(std::get<3>(this->WorldConnectors[i]));
		}
		CMap::Map.MapLayers[z]->LayerConnectors.push_back(unit);
		for (size_t second_z = 0; second_z < CMap::Map.MapLayers.size(); ++second_z) {
			bool found_other_connector = false;
			if (CMap::Map.MapLayers[second_z]->World == std::get<2>(this->WorldConnectors[i])) {
				for (size_t j = 0; j < CMap::Map.MapLayers[second_z]->LayerConnectors.size(); ++j) {
					if (CMap::Map.MapLayers[second_z]->LayerConnectors[j]->Type == unit->Type && CMap::Map.MapLayers[second_z]->LayerConnectors[j]->Unique == unit->Unique && CMap::Map.MapLayers[second_z]->LayerConnectors[j]->ConnectingDestination == nullptr) {
						CMap::Map.MapLayers[second_z]->LayerConnectors[j]->ConnectingDestination = unit;
						unit->ConnectingDestination = CMap::Map.MapLayers[second_z]->LayerConnectors[j];
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
		const int surface_layer = std::get<2>(this->SurfaceLayerConnectors[i]);
		CUniqueItem *unique = std::get<3>(this->SurfaceLayerConnectors[i]);
		Vec2i unit_raw_pos(std::get<0>(this->SurfaceLayerConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		Vec2i unit_offset((type->TileSize - 1) / 2);

		//add the connecting destination
		const CMapTemplate *other_template = nullptr;
		if (surface_layer == (this->SurfaceLayer + 1)) {
			other_template = this->LowerTemplate;
		}
		else if (surface_layer == (this->SurfaceLayer - 1)) {
			other_template = this->UpperTemplate;
		}
		if (!other_template) {
			continue; //surface layer connectors must lead to an adjacent surface layer
		}

		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			
			bool already_implemented = false; //the connector could already have been implemented if it inherited its position from the connector in the destination layer (if the destination layer's map template was applied first)
			std::vector<CUnit *> other_layer_connectors = CMap::Map.GetMapTemplateLayerConnectors(other_template);
			for (const CUnit *connector : other_layer_connectors) {
				if (connector->Type == type && connector->Unique == unique && connector->ConnectingDestination != nullptr && connector->ConnectingDestination->MapLayer->Plane == this->Plane && connector->ConnectingDestination->MapLayer->World == this->World && connector->ConnectingDestination->MapLayer->SurfaceLayer == this->SurfaceLayer) {
					already_implemented = true;
					break;
				}
			}

			if (already_implemented) {
				continue;
			}
			
			unit_pos = CMap::Map.GenerateUnitLocation(type, nullptr, map_start_pos, map_end - Vec2i(1, 1), z);
			unit_pos += unit_offset;
		} else {
			if (unit_raw_pos.x == -1 && unit_raw_pos.y == -1) {
				//if the upper/lower layer has a surface layer connector to this layer that has no connecting destination, and this connector leads to that surface layer, place this connection in the same position as the other one
				std::vector<CUnit *> other_layer_connectors = CMap::Map.GetMapTemplateLayerConnectors(other_template);
				for (const CUnit *potential_connector : other_layer_connectors) {
					if (potential_connector->Type == type && potential_connector->Unique == unique && potential_connector->ConnectingDestination == nullptr) {
						unit_pos = potential_connector->GetTileCenterPos();
						break;
					}
				}
			}
		}
		if (!CMap::Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x || unit_pos.y < map_start_pos.y) {
			continue;
		}
		
		if (!OnTopDetails(*type, nullptr) && !UnitTypeCanBeAt(*type, unit_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(unit_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(unit_pos - unit_offset + Vec2i(type->TileSize - 1), z) && unit_raw_pos.x != -1 && unit_raw_pos.y != -1) {
			fprintf(stderr, "Unit \"%s\" should be placed on (%d, %d) for map template \"%s\", but it cannot be there.\n", type->Ident.c_str(), unit_raw_pos.x, unit_raw_pos.y, this->Ident.c_str());
		}

		CUnit *unit = CreateUnit(unit_pos - unit_offset, *type, CPlayer::Players[PlayerNumNeutral], z, true);
		if (unique) {
			unit->SetUnique(unique);
		}
		CMap::Map.MapLayers[z]->LayerConnectors.push_back(unit);
		
		//get the nearest compatible connector in the target map layer / template
		std::vector<CUnit *> other_layer_connectors = CMap::Map.GetMapTemplateLayerConnectors(other_template);
		CUnit *best_layer_connector = nullptr;
		int best_distance = -1;
		for (size_t j = 0; j < other_layer_connectors.size(); ++j) {
			CUnit *potential_connector = other_layer_connectors[j];
			
			if (potential_connector->Type == type && potential_connector->Unique == unique && potential_connector->ConnectingDestination == nullptr) {
				int distance = potential_connector->MapDistanceTo(unit->GetTileCenterPos(), potential_connector->MapLayer->ID);
				if (best_distance == -1 || distance < best_distance) {
					best_layer_connector = potential_connector;
					best_distance = distance;
					if (distance == 0) {
						break;
					}
				}
			}
		}
		
		if (best_layer_connector) {
			best_layer_connector->ConnectingDestination = unit;
			unit->ConnectingDestination = best_layer_connector;
		}
	}
}

void CMapTemplate::ApplyUnits(const Vec2i &template_start_pos, const Vec2i &map_start_pos, const int z, const bool random) const
{
	Vec2i map_end(std::min(CMap::Map.Info.MapWidths[z], map_start_pos.x + this->Width), std::min(CMap::Map.Info.MapHeights[z], map_start_pos.y + this->Height));
	const CCampaign *current_campaign = CCampaign::GetCurrentCampaign();
	CDate start_date;
	if (current_campaign) {
		start_date = current_campaign->GetStartDate();
	}

	for (size_t i = 0; i < this->Units.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(this->Units[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		const CUnitType *type = std::get<1>(this->Units[i]);
		Vec2i unit_offset((type->TileSize - 1) / 2);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = CMap::Map.GenerateUnitLocation(type, std::get<2>(this->Units[i]), map_start_pos, map_end - Vec2i(1, 1), z);
			unit_pos += unit_offset;
		}
		if (!CMap::Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x || unit_pos.y < map_start_pos.y) {
			continue;
		}
		
		if (
			(!current_campaign && std::get<3>(this->Units[i]).Year == 0 && std::get<4>(this->Units[i]).Year == 0)
			|| (
				current_campaign && (std::get<3>(this->Units[i]).Year == 0 || start_date.ContainsDate(std::get<3>(this->Units[i])))
				&& (std::get<4>(this->Units[i]).Year == 0 || (!start_date.ContainsDate(std::get<4>(this->Units[i]))))
			)
		) {
			CPlayer *player = nullptr;
			if (std::get<2>(this->Units[i])) {
				if (!current_campaign) { //only apply neutral units for when applying map templates for non-campaign/scenario maps
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
				player = CPlayer::Players[PlayerNumNeutral];
			}

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

	if (!current_campaign) {
		return;
	}

	for (size_t i = 0; i < this->Heroes.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(this->Heroes[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		CCharacter *hero = std::get<1>(this->Heroes[i]);
		Vec2i unit_offset((hero->Type->TileSize - 1) / 2);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = CMap::Map.GenerateUnitLocation(hero->Type, std::get<2>(this->Heroes[i]), map_start_pos, map_end - Vec2i(1, 1), z);
			unit_pos += unit_offset;
		}
		if (!CMap::Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x || unit_pos.y < map_start_pos.y) {
			continue;
		}
		
		if ((!current_campaign || std::get<3>(this->Heroes[i]).Year == 0 || start_date.ContainsDate(std::get<3>(this->Heroes[i]))) && (std::get<4>(this->Heroes[i]).Year == 0 || !start_date.ContainsDate(std::get<4>(this->Heroes[i])))) {
			CPlayer *player = nullptr;
			if (std::get<2>(this->Heroes[i])) {
				player = GetOrAddFactionPlayer(std::get<2>(this->Heroes[i]));
				if (!player) {
					continue;
				}
				if (player->StartPos.x == 0 && player->StartPos.y == 0) {
					player->SetStartView(unit_pos, z);
				}
			} else {
				player = CPlayer::Players[PlayerNumNeutral];
			}
			CUnit *unit = CreateUnit(unit_pos - unit_offset, *hero->Type, player, z);
			unit->SetCharacter(hero->Ident);
			if (!unit->Type->BoolFlag[BUILDING_INDEX].value && !unit->Type->BoolFlag[HARVESTER_INDEX].value) { // make non-building, non-harvester units not have an active AI
				unit->Active = 0;
				player->ChangeUnitTypeAiActiveCount(hero->Type, -1);
			}
		}
	}
	
	for (CHistoricalUnit *historical_unit : CHistoricalUnit::GetAll()) {
		if (historical_unit->StartDate.Year == 0 || !start_date.ContainsDate(historical_unit->StartDate) || start_date.ContainsDate(historical_unit->EndDate)) { //historical units aren't implemented if their date isn't set
			continue;
		}
		
		CFaction *unit_faction = historical_unit->Faction;

		CPlayer *unit_player = unit_faction ? GetFactionPlayer(unit_faction) : nullptr;
		
		bool in_another_map_template = false;
		Vec2i unit_pos = this->GetBestLocationMapPosition(historical_unit->HistoricalLocations, in_another_map_template, template_start_pos, map_start_pos, false);
		
		if (in_another_map_template) {
			continue;
		}
		
		if (unit_pos.x == -1 || unit_pos.y == -1) {
			if (!random) { //apply units whose position is that of a randomly-placed site, or that of their player's start position, together with randomly-placed units
				continue;
			}
			
			unit_pos = this->GetBestLocationMapPosition(historical_unit->HistoricalLocations, in_another_map_template, template_start_pos, map_start_pos, true);
			
			if (unit_pos.x == -1 || unit_pos.y == -1) {
				unit_pos = CMap::Map.GenerateUnitLocation(historical_unit->UnitType, unit_faction, map_start_pos, map_end - Vec2i(1, 1), z);
				unit_pos += historical_unit->UnitType->GetTileCenterPosOffset();
			}
		} else {
			if (random) {
				continue;
			}
		}
		
		if (!CMap::Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x || unit_pos.y < map_start_pos.y) { //units whose faction hasn't been created already and who don't have a valid historical location set won't be created
			continue;
		}
		
		if (unit_faction) {
			unit_player = GetOrAddFactionPlayer(unit_faction);
			if (!unit_player) {
				continue;
			}
			if (unit_player->StartPos.x == 0 && unit_player->StartPos.y == 0) {
				unit_player->SetStartView(unit_pos, z);
			}
		} else {
			unit_player = CPlayer::Players[PlayerNumNeutral];
		}
		for (int i = 0; i < historical_unit->Quantity; ++i) {
			CUnit *unit = CreateUnit(unit_pos - historical_unit->UnitType->GetTileCenterPosOffset(), *historical_unit->UnitType, unit_player, z);
		}
	}
	
	for (CCharacter *character : CCharacter::Characters) {
		if (!character->CanAppear()) {
			continue;
		}
		
		if (character->Faction == nullptr && !character->Type->BoolFlag[FAUNA_INDEX].value) { //only fauna "heroes" may have no faction
			continue;
		}
		
		if (character->StartDate.Year == 0 || !start_date.ContainsDate(character->StartDate) || start_date.ContainsDate(character->DeathDate)) { //contrary to other elements, heroes aren't implemented if their date isn't set
			continue;
		}
		
		const CFaction *hero_faction = character->Faction;
		for (int i = ((int) character->HistoricalFactions.size() - 1); i >= 0; --i) {
			if (start_date.ContainsDate(character->HistoricalFactions[i].first)) {
				hero_faction = character->HistoricalFactions[i].second;
				break;
			}
		}

		CPlayer *hero_player = hero_faction ? GetFactionPlayer(hero_faction) : nullptr;
		
		bool in_another_map_template = false;
		Vec2i hero_pos = this->GetBestLocationMapPosition(character->HistoricalLocations, in_another_map_template, template_start_pos, map_start_pos, false);
		
		if (in_another_map_template) {
			continue;
		}
		
		if (hero_pos.x == -1 || hero_pos.y == -1) {
			if (!random) { //apply heroes whose position is that of a randomly-placed site, or that of their player's start position, together with randomly-placed units
				continue;
			}
			
			hero_pos = this->GetBestLocationMapPosition(character->HistoricalLocations, in_another_map_template, template_start_pos, map_start_pos, true);
			
			if ((hero_pos.x == -1 || hero_pos.y == -1) && hero_player && hero_player->StartMapLayer == z) {
				hero_pos = hero_player->StartPos;
			}
		} else {
			if (random) {
				continue;
			}
		}
		
		if (!CMap::Map.Info.IsPointOnMap(hero_pos, z) || hero_pos.x < map_start_pos.x || hero_pos.y < map_start_pos.y) { //heroes whose faction hasn't been created already and who don't have a valid historical location set won't be created
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
			hero_player = CPlayer::Players[PlayerNumNeutral];
		}
		CUnit *unit = CreateUnit(hero_pos - character->Type->GetTileCenterPosOffset(), *character->Type, hero_player, z);
		unit->SetCharacter(character->Ident);
		unit->Active = 0;
		hero_player->ChangeUnitTypeAiActiveCount(character->Type, -1);
	}
}

/**
**	@brief	Get whether this map template is a subtemplate area of another one
**
**	@return	True if it is a subtemplate area, or false otherwise
*/
bool CMapTemplate::IsSubtemplateArea() const
{
	return this->MainTemplate != nullptr;
}

/**
**	@brief	Get the top map template for this one
**
**	@return	The topmost map template for this one (which can be itself if it isn't a subtemplate area)
*/
const CMapTemplate *CMapTemplate::GetTopMapTemplate() const
{
	if (this->MainTemplate != nullptr) {
		return this->MainTemplate->GetTopMapTemplate();
	} else {
		return this;
	}
}

/**
**	@brief	Gets the best map position from a list of historical locations
**
**	@param	historical_location_list	The list of historical locations
**	@param	in_another_map_template		This is set to true if there is a valid position, but it is in another map templa
**
**	@return	The best position if found, or an invalid one otherwise
*/
Vec2i CMapTemplate::GetBestLocationMapPosition(const std::vector<CHistoricalLocation *> &historical_location_list, bool &in_another_map_template, const Vec2i &template_start_pos, const Vec2i &map_start_pos, const bool random) const
{
	Vec2i pos(-1, -1);
	in_another_map_template = false;
	const CCampaign *current_campaign = CCampaign::GetCurrentCampaign();
	CDate start_date;
	if (current_campaign) {
		start_date = current_campaign->GetStartDate();
	}
	
	for (int i = ((int) historical_location_list.size() - 1); i >= 0; --i) {
		CHistoricalLocation *historical_location = historical_location_list[i];
		if (start_date.ContainsDate(historical_location->Date)) {
			if (historical_location->MapTemplate == this) {
				if (historical_location->Position.x != -1 && historical_location->Position.y != -1) { //historical unit position, could also have been inherited from a site with a fixed position
					pos = map_start_pos + historical_location->Position - template_start_pos;
				} else if (random) {
					if (historical_location->Site != nullptr && historical_location->Site->SiteUnit != nullptr) { //sites with random positions will have no valid stored fixed position, but will have had a site unit randomly placed; use that site unit's position instead for this unit then
						pos = historical_location->Site->SiteUnit->GetTileCenterPos();
					}
				}
			} else {
				in_another_map_template = true;
			}
			break;
		}
	}
	
	return pos;
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CGeneratedTerrain::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "terrain_type") {
			value = FindAndReplaceString(value, "_", "-");
			this->TerrainType = CTerrainType::GetTerrainType(value);
		} else if (key == "seed_count") {
			this->SeedCount = std::stoi(value);
		} else if (key == "expansion_chance") {
			this->ExpansionChance = std::stoi(value);
		} else if (key == "max_percent") {
			this->MaxPercent = std::stoi(value);
		} else if (key == "use_existing_as_seeds") {
			this->UseExistingAsSeeds = StringToBool(value);
		} else if (key == "use_subtemplate_borders_as_seeds") {
			this->UseSubtemplateBordersAsSeeds = StringToBool(value);
		} else if (key == "target_terrain_type") {
			value = FindAndReplaceString(value, "_", "-");
			const CTerrainType *target_terrain_type = CTerrainType::GetTerrainType(value);
			if (target_terrain_type) {
				this->TargetTerrainTypes.push_back(target_terrain_type);
			}
		} else {
			fprintf(stderr, "Invalid generated terrain property: \"%s\".\n", key.c_str());
		}
	}
	
	if (!this->TerrainType) {
		fprintf(stderr, "Generated terrain has no terrain type.\n");
	}
}

/**
**	@brief	Get whether the terrain generation can use the given tile as a seed
**
**	@param	tile	The tile
**
**	@return	True if the tile can be used as a seed, or false otherwise
*/
bool CGeneratedTerrain::CanUseTileAsSeed(const CMapField *tile) const
{
	const CTerrainType *top_terrain = tile->GetTopTerrain();
	
	if (top_terrain == this->TerrainType) { //top terrain is the same as the one for the generation, so the tile can be used as a seed
		return true;
	}
	
	if (this->TerrainType == tile->Terrain && std::find(this->TargetTerrainTypes.begin(), this->TargetTerrainTypes.end(), top_terrain) == this->TargetTerrainTypes.end()) { //the tile's base terrain is the same as the one for the generation, and its overlay terrain is not a target for the generation
		return true;
	}
	
	return false;
}

/**
**	@brief	Get whether the terrain can be generated on the given tile
**
**	@param	tile	The tile
**
**	@return	True if the terrain can be generated on the tile, or false otherwise
*/
bool CGeneratedTerrain::CanGenerateOnTile(const CMapField *tile) const
{
	if (this->TerrainType->Overlay) {
		if (std::find(this->TargetTerrainTypes.begin(), this->TargetTerrainTypes.end(), tile->GetTopTerrain()) == this->TargetTerrainTypes.end()) { //disallow generating over terrains that aren't a target for the generation
			return false;
		}
	} else {
		if (
			std::find(this->TargetTerrainTypes.begin(), this->TargetTerrainTypes.end(), tile->GetTopTerrain()) == this->TargetTerrainTypes.end()
			&& std::find(this->TargetTerrainTypes.begin(), this->TargetTerrainTypes.end(), tile->Terrain) == this->TargetTerrainTypes.end()
		) {
			return false;
		}
		
		if ( //don't allow generating the terrain on the tile if it is a base terrain, and putting it there would destroy an overlay terrain that isn't a target of the generation
			tile->OverlayTerrain
			&& !this->CanRemoveTileOverlayTerrain(tile)
			&& std::find(tile->OverlayTerrain->BaseTerrainTypes.begin(), tile->OverlayTerrain->BaseTerrainTypes.end(), this->TerrainType) == tile->OverlayTerrain->BaseTerrainTypes.end()
		) {
			return false;
		}
		
		if (std::find(this->TerrainType->BorderTerrains.begin(), this->TerrainType->BorderTerrains.end(), tile->Terrain) == this->TerrainType->BorderTerrains.end()) { //don't allow generating on the tile if it can't be a border terrain to the terrain we want to generate
			return false;
		}
	}
	
	return true;
}

/**
**	@brief	Get whether the tile can be a part of an expansion
**
**	@param	tile	The tile
**
**	@return	True if the tile can be part of an expansion, or false otherwise
*/
bool CGeneratedTerrain::CanTileBePartOfExpansion(const CMapField *tile) const
{
	if (this->CanGenerateOnTile(tile)) {
		return true;
	}
	
	if (this->TerrainType == tile->GetTopTerrain()) {
		return true;
	}
	
	if (!this->TerrainType->Overlay) {
		if (this->TerrainType == tile->Terrain) {
			return true;
		}
	}
	
	return false;
}

/**
**	@brief	Get whether the terrain generation can remove the tile's overlay terrain
**
**	@param	tile	The tile
**
**	@return	True if the terrain generation can remove the tile's overlay terrain, or false otherwise
*/
bool CGeneratedTerrain::CanRemoveTileOverlayTerrain(const CMapField *tile) const
{
	if (std::find(this->TargetTerrainTypes.begin(), this->TargetTerrainTypes.end(), tile->OverlayTerrain) == this->TargetTerrainTypes.end()) {
		return false;
	}
	
	return true;
}
