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
/**@name map.cpp - The map. */
//
//      (c) Copyright 1998-2006 by Lutz Sammer, Vladi Shabanski and
//                                 Francois Beerten
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

#include "map.h"

//Wyrmgus start
#include <fstream>
//Wyrmgus end

//Wyrmgus start
#include "editor.h"
#include "game.h" // for the SaveGameLoading variable
//Wyrmgus end
#include "iolib.h"
#include "player.h"
//Wyrmgus start
#include "province.h"
#include "quest.h"
#include "settings.h"
//Wyrmgus end
#include "tileset.h"
//Wyrmgus start
#include "translate.h"
//Wyrmgus end
#include "unit.h"
//Wyrmgus start
#include "unit_find.h"
//Wyrmgus end
#include "unit_manager.h"
#include "ui.h"
//Wyrmgus start
#include "upgrade.h"
//Wyrmgus end
#include "version.h"
#include "video.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

//Wyrmgus start
std::vector<CMapTemplate *> MapTemplates;
std::map<std::string, CMapTemplate *> MapTemplateIdentToPointer;
std::vector<CSettlement *> Settlements;
std::map<std::string, CSettlement *> SettlementIdentToPointer;
std::vector<CTerrainFeature *> TerrainFeatures;
std::map<std::string, CTerrainFeature *> TerrainFeatureIdentToPointer;
std::map<std::tuple<int, int, int>, int> TerrainFeatureColorToIndex;
std::vector<CTimeline *> Timelines;
std::map<std::string, CTimeline *> TimelineIdentToPointer;
//Wyrmgus end
CMap Map;                   /// The current map
//Wyrmgus start
int CurrentMapLayer = 0;
//Wyrmgus end
int FlagRevealMap;          /// Flag must reveal the map
int ReplayRevealMap;        /// Reveal Map is replay
int ForestRegeneration;     /// Forest regeneration
char CurrentMapPath[1024];  /// Path of the current map

//Wyrmgus start
/**
**  Get a map template
*/
CMapTemplate *GetMapTemplate(std::string map_ident)
{
	if (map_ident.empty()) {
		return NULL;
	}
	
	if (MapTemplateIdentToPointer.find(map_ident) != MapTemplateIdentToPointer.end()) {
		return MapTemplateIdentToPointer[map_ident];
	}
	
	return NULL;
}

/**
**  Get a settlement
*/
CSettlement *GetSettlement(std::string settlement_ident)
{
	if (settlement_ident.empty()) {
		return NULL;
	}
	
	if (SettlementIdentToPointer.find(settlement_ident) != SettlementIdentToPointer.end()) {
		return SettlementIdentToPointer[settlement_ident];
	}
	
	return NULL;
}

/**
**  Get a terrain feature
*/
CTerrainFeature *GetTerrainFeature(std::string terrain_feature_ident)
{
	if (terrain_feature_ident.empty()) {
		return NULL;
	}
	
	if (TerrainFeatureIdentToPointer.find(terrain_feature_ident) != TerrainFeatureIdentToPointer.end()) {
		return TerrainFeatureIdentToPointer[terrain_feature_ident];
	}
	
	return NULL;
}

/**
**  Get a timeline
*/
CTimeline *GetTimeline(std::string timeline_ident)
{
	if (timeline_ident.empty()) {
		return NULL;
	}
	
	if (TimelineIdentToPointer.find(timeline_ident) != TimelineIdentToPointer.end()) {
		return TimelineIdentToPointer[timeline_ident];
	}
	
	return NULL;
}

std::string GetDegreeLevelNameById(int degree_level)
{
	if (degree_level == ExtremelyHighDegreeLevel) {
		return "extremely-high";
	} else if (degree_level == VeryHighDegreeLevel) {
		return "very-high";
	} else if (degree_level == HighDegreeLevel) {
		return "high";
	} else if (degree_level == MediumDegreeLevel) {
		return "medium";
	} else if (degree_level == LowDegreeLevel) {
		return "low";
	} else if (degree_level == VeryLowDegreeLevel) {
		return "very-low";
	}
	return "";
}

int GetDegreeLevelIdByName(std::string degree_level)
{
	if (degree_level == "extremely-high") {
		return ExtremelyHighDegreeLevel;
	} else if (degree_level == "very-high") {
		return VeryHighDegreeLevel;
	} else if (degree_level == "high") {
		return HighDegreeLevel;
	} else if (degree_level == "medium") {
		return MediumDegreeLevel;
	} else if (degree_level == "low") {
		return LowDegreeLevel;
	} else if (degree_level == "very-low") {
		return VeryLowDegreeLevel;
	} else {
		return -1;
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
			char terrain_id = -1;
			if (TerrainTypeCharacterToIndex.find(terrain_character) != TerrainTypeCharacterToIndex.end()) {
				terrain_id = TerrainTypeCharacterToIndex.find(terrain_character)->second;
			}
			if (terrain_id != -1) {
				Vec2i real_pos(map_start_pos.x + x - template_start_pos.x, map_start_pos.y + y - template_start_pos.y);
				Map.Field(real_pos, z)->SetTerrain(TerrainTypes[terrain_id]);
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

			char terrain_id = -1;
			short terrain_feature_id = -1;
			if (TerrainFeatureColorToIndex.find(std::tuple<int, int, int>(r, g, b)) != TerrainFeatureColorToIndex.end()) {
				terrain_feature_id = TerrainFeatureColorToIndex.find(std::tuple<int, int, int>(r, g, b))->second;
				terrain_id = TerrainFeatures[terrain_feature_id]->TerrainType->ID;
			} else if (TerrainTypeColorToIndex.find(std::tuple<int, int, int>(r, g, b)) != TerrainTypeColorToIndex.end()) {
				terrain_id = TerrainTypeColorToIndex.find(std::tuple<int, int, int>(r, g, b))->second;
			}
			for (int sub_y = 0; sub_y < this->Scale; ++sub_y) {
				for (int sub_x = 0; sub_x < this->Scale; ++sub_x) {
					Vec2i real_pos(map_start_pos.x + ((x - template_start_pos.x) * this->Scale) + sub_x, map_start_pos.y + ((y - template_start_pos.y) * this->Scale) + sub_y);

					if (!Map.Info.IsPointOnMap(real_pos, z)) {
						continue;
					}
					
					if (terrain_id != -1) {
						Map.Field(real_pos, z)->SetTerrain(TerrainTypes[terrain_id]);
						
						if (terrain_feature_id != -1) {
							Map.Field(real_pos, z)->TerrainFeature = TerrainFeatures[terrain_feature_id];
						}
					} else {
						if (r != 0 || g != 0 || b != 0 || !overlay) { //fully black pixels represent areas in overlay terrain files that don't have any overlays
							fprintf(stderr, "Invalid map terrain: (%d, %d)\n", x, y);
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
	
	if (z >= (int) Map.Fields.size()) {
		Map.Info.MapWidths.push_back(std::min(this->Width * this->Scale, Map.Info.MapWidth));
		Map.Info.MapHeights.push_back(std::min(this->Height * this->Scale, Map.Info.MapHeight));
		Map.Fields.push_back(new CMapField[Map.Info.MapWidths[z] * Map.Info.MapHeights[z]]);
		Map.TimeOfDaySeconds.push_back(this->TimeOfDaySeconds);
		Map.TimeOfDay.push_back(NoTimeOfDay);
		Map.Planes.push_back(this->Plane);
		Map.Worlds.push_back(this->World);
		Map.Layers.push_back(this->Layer);
		Map.LayerConnectors.resize(z + 1);
		Map.CulturalSettlementNames.resize(z + 1);
		Map.FactionCulturalSettlementNames.resize(z + 1);
	} else {
		if (!this->IsSubtemplateArea()) {
			Map.TimeOfDaySeconds[z] = this->TimeOfDaySeconds;
			Map.Planes[z] = this->Plane;
			Map.Worlds[z] = this->World;
			Map.Layers[z] = this->Layer;
		}
	}

	if (this->TimeOfDaySeconds && !GameSettings.Inside && !GameSettings.NoTimeOfDay && Editor.Running == EditorNotRunning && !this->IsSubtemplateArea()) {
		Map.TimeOfDay[z] = SyncRand(MaxTimesOfDay - 1) + 1; // begin at a random time of day
	}
	
	Vec2i map_end(std::min(Map.Info.MapWidths[z], map_start_pos.x + (this->Width * this->Scale)), std::min(Map.Info.MapHeights[z], map_start_pos.y + (this->Height * this->Scale)));
	if (!Map.Info.IsPointOnMap(map_start_pos, z)) {
		fprintf(stderr, "Invalid map coordinate for map template \"%s\": (%d, %d)\n", this->Ident.c_str(), map_start_pos.x, map_start_pos.y);
		return;
	}
	
	ShowLoadProgress(_("Applying \"%s\" Map Template Terrain"), this->Name.c_str());
	
	this->ApplyTerrainImage(false, template_start_pos, map_start_pos, z);
	this->ApplyTerrainImage(true, template_start_pos, map_start_pos, z);

	for (size_t i = 0; i < HistoricalTerrains.size(); ++i) {
		Vec2i history_pos = std::get<0>(HistoricalTerrains[i]);
		if (history_pos.x < template_start_pos.x || history_pos.x >= (template_start_pos.x + (Map.Info.MapWidths[z] / this->Scale)) || history_pos.y < template_start_pos.y || history_pos.y >= (template_start_pos.y + (Map.Info.MapHeights[z] / this->Scale))) {
			continue;
		}
		if (CurrentCampaign->StartDate.ContainsDate(std::get<2>(HistoricalTerrains[i])) || std::get<2>(HistoricalTerrains[i]).year == 0) {
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
	
	if (this->IsSubtemplateArea() && this->SurroundingTerrain) {
		Vec2i surrounding_start_pos(map_start_pos - Vec2i(1, 1));
		Vec2i surrounding_end(map_end + Vec2i(1, 1));
		for (int x = surrounding_start_pos.x; x < surrounding_end.x; ++x) {
			for (int y = surrounding_start_pos.y; y < surrounding_end.y; y += (surrounding_end.y - surrounding_start_pos.y - 1)) {
				Vec2i surrounding_pos(x, y);
				if (!Map.Info.IsPointOnMap(surrounding_pos, z) || Map.IsPointInASubtemplateArea(surrounding_pos, z)) {
					continue;
				}
				Map.Field(surrounding_pos, z)->SetTerrain(this->SurroundingTerrain);
			}
		}
		for (int x = surrounding_start_pos.x; x < surrounding_end.x; x += (surrounding_end.x - surrounding_start_pos.x - 1)) {
			for (int y = surrounding_start_pos.y; y < surrounding_end.y; ++y) {
				Vec2i surrounding_pos(x, y);
				if (!Map.Info.IsPointOnMap(surrounding_pos, z) || Map.IsPointInASubtemplateArea(surrounding_pos, z)) {
					continue;
				}
				Map.Field(surrounding_pos, z)->SetTerrain(this->SurroundingTerrain);
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
				
			Map.SubtemplateAreas[z].push_back(std::tuple<Vec2i, Vec2i, CMapTemplate *>(subtemplate_pos, Vec2i(subtemplate_pos.x + this->Subtemplates[i]->Width - 1, subtemplate_pos.y + this->Subtemplates[i]->Height - 1), this->Subtemplates[i]));
				
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
	
	ShowLoadProgress(_("Applying \"%s\" Map Template Units"), this->Name.c_str());

	for (std::map<std::tuple<int, int, int>, std::string>::iterator iterator = this->CulturalSettlementNames.begin(); iterator != this->CulturalSettlementNames.end(); ++iterator) {
		int settlement_x = map_start_pos.x + ((std::get<0>(iterator->first) - template_start_pos.x) * this->Scale);
		int settlement_y = map_start_pos.y + ((std::get<1>(iterator->first) - template_start_pos.y) * this->Scale);
		int settlement_civilization = std::get<2>(iterator->first);
		
		if (!Map.Info.IsPointOnMap(Vec2i(settlement_x, settlement_y), z)) {
			continue;
		}

		Map.CulturalSettlementNames[z][std::tuple<int, int, int>(settlement_x, settlement_y, settlement_civilization)] = iterator->second;
	}
	
	for (std::map<std::tuple<int, int, CFaction *>, std::string>::iterator iterator = this->FactionCulturalSettlementNames.begin(); iterator != this->FactionCulturalSettlementNames.end(); ++iterator) {
		int settlement_x = map_start_pos.x + ((std::get<0>(iterator->first) - template_start_pos.x) * this->Scale);
		int settlement_y = map_start_pos.y + ((std::get<1>(iterator->first) - template_start_pos.y) * this->Scale);
		CFaction *settlement_faction = std::get<2>(iterator->first);
		
		if (!Map.Info.IsPointOnMap(Vec2i(settlement_x, settlement_y), z)) {
			continue;
		}

		Map.FactionCulturalSettlementNames[z][std::tuple<int, int, CFaction *>(settlement_x, settlement_y, settlement_faction)] = iterator->second;
	}
	
	if (!this->IsSubtemplateArea()) {
		Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
		Map.AdjustTileMapTransitions(map_start_pos, map_end, z);
		Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
	}
	
	for (std::map<std::pair<int, int>, std::tuple<CUnitType *, int, CUniqueItem *>>::iterator iterator = this->Resources.begin(); iterator != this->Resources.end(); ++iterator) {
		Vec2i unit_pos(map_start_pos.x + iterator->first.first - template_start_pos.x, map_start_pos.y + iterator->first.second - template_start_pos.y);
		if (!Map.Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		
		Vec2i unit_offset((std::get<0>(iterator->second)->TileWidth - 1) / 2, (std::get<0>(iterator->second)->TileHeight - 1) / 2);
		CUnit *unit = CreateResourceUnit(unit_pos - unit_offset, *std::get<0>(iterator->second), z);
		
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

	this->ApplyConnectors(template_start_pos, map_start_pos, z);
	if (CurrentCampaign != NULL) {
		this->ApplySettlements(template_start_pos, map_start_pos, z);
	}
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
	this->ApplyConnectors(template_start_pos, map_start_pos, z, true);
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
			int worker_type_id = PlayerRaces.GetFactionClassUnitType(Players[i].Race, Players[i].Faction, GetUnitTypeClassIndexByName("worker"));			
			if (worker_type_id != -1) {
				Vec2i worker_unit_offset((UnitTypes[worker_type_id]->TileWidth - 1) / 2, (UnitTypes[worker_type_id]->TileHeight - 1) / 2);
				
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
		bool grouped = this->GeneratedNeutralUnits[i].first->GivesResource && this->GeneratedNeutralUnits[i].first->TileWidth == 1 && this->GeneratedNeutralUnits[i].first->TileHeight == 1; // group small resources
		Map.GenerateNeutralUnits(this->GeneratedNeutralUnits[i].first, this->GeneratedNeutralUnits[i].second, map_start_pos, map_end - Vec2i(1, 1), grouped, z);
	}
}

void CMapTemplate::ApplySettlements(Vec2i template_start_pos, Vec2i map_start_pos, int z)
{
	Vec2i map_end(std::min(Map.Info.MapWidths[z], map_start_pos.x + this->Width), std::min(Map.Info.MapHeights[z], map_start_pos.y + this->Height));

	for (std::map<std::pair<int, int>, CSettlement *>::iterator settlement_iterator = this->Settlements.begin(); settlement_iterator != this->Settlements.end(); ++settlement_iterator) {
		Vec2i settlement_raw_pos(settlement_iterator->second->Position);
		Vec2i settlement_pos(map_start_pos + ((settlement_raw_pos - template_start_pos) * this->Scale));

		if (!Map.Info.IsPointOnMap(settlement_pos, z) || settlement_pos.x < map_start_pos.x || settlement_pos.y < map_start_pos.y) {
			continue;
		}

		for (std::map<int, std::string>::iterator cultural_name_iterator = settlement_iterator->second->CulturalNames.begin(); cultural_name_iterator != settlement_iterator->second->CulturalNames.end(); ++cultural_name_iterator) {
			Map.CulturalSettlementNames[z][std::tuple<int, int, int>(settlement_pos.x, settlement_pos.y, cultural_name_iterator->first)] = cultural_name_iterator->second;
		}
		
		if (settlement_iterator->second->Major && SettlementSiteUnitType) { //add a settlement site for major settlements
			Vec2i unit_offset((SettlementSiteUnitType->TileWidth - 1) / 2, (SettlementSiteUnitType->TileHeight - 1) / 2);
			CUnit *unit = CreateUnit(settlement_pos - unit_offset, *SettlementSiteUnitType, &Players[PlayerNumNeutral], z);
			unit->Settlement = settlement_iterator->second;
			Map.SettlementUnits.push_back(unit);
		}
		
		for (size_t j = 0; j < settlement_iterator->second->HistoricalResources.size(); ++j) {
			if (
				CurrentCampaign->StartDate.ContainsDate(std::get<0>(settlement_iterator->second->HistoricalResources[j]))
				&& (!CurrentCampaign->StartDate.ContainsDate(std::get<1>(settlement_iterator->second->HistoricalResources[j])) || std::get<1>(settlement_iterator->second->HistoricalResources[j]).year == 0)
			) {
				const CUnitType *type = std::get<2>(settlement_iterator->second->HistoricalResources[j]);
				if (!type) {
					fprintf(stderr, "Error in CMap::ApplySettlements (settlement ident \"%s\"): historical resource type is NULL.\n", settlement_iterator->second->Ident.c_str());
					continue;
				}
				Vec2i unit_offset((type->TileWidth - 1) / 2, (type->TileHeight - 1) / 2);
				CUnit *unit = CreateResourceUnit(settlement_pos - unit_offset, *type, z, false); // don't generate unique resources when setting special properties, since for map templates unique resources are supposed to be explicitly indicated
				if (std::get<3>(settlement_iterator->second->HistoricalResources[j])) {
					unit->SetUnique(std::get<3>(settlement_iterator->second->HistoricalResources[j]));
				}
				int resource_quantity = std::get<4>(settlement_iterator->second->HistoricalResources[j]);
				if (resource_quantity) { //set the resource_quantity after setting the unique unit, so that unique resources can be decreased in quantity over time
					unit->SetResourcesHeld(resource_quantity);
					unit->Variable[GIVERESOURCE_INDEX].Value = resource_quantity;
					unit->Variable[GIVERESOURCE_INDEX].Max = resource_quantity;
					unit->Variable[GIVERESOURCE_INDEX].Enable = 1;
				}
			}
		}
		
		CFaction *settlement_owner = NULL;
		for (std::map<CDate, CFaction *>::reverse_iterator owner_iterator = settlement_iterator->second->HistoricalOwners.rbegin(); owner_iterator != settlement_iterator->second->HistoricalOwners.rend(); ++owner_iterator) {
			if (CurrentCampaign->StartDate.ContainsDate(owner_iterator->first)) { // set the owner to the latest historical owner given the scenario's start date
				settlement_owner = owner_iterator->second;
				break;
			}
		}
		
		if (!settlement_owner) {
			continue;
		}
		
		CPlayer *player = GetOrAddFactionPlayer(settlement_owner);
		
		if (!player) {
			continue;
		}
		
		if (player->StartPos.x == 0 && player->StartPos.y == 0) {
			Vec2i default_pos(map_start_pos + ((settlement_owner->DefaultStartPos - template_start_pos) * this->Scale));
			if (settlement_owner->DefaultStartPos.x != -1 && settlement_owner->DefaultStartPos.y != -1 && Map.Info.IsPointOnMap(default_pos, z)) {
				player->SetStartView(default_pos, z);
			} else {
				player->SetStartView(settlement_pos, z);
			}
		}
		
		CUnitType *pathway_type = NULL;
		for (size_t j = 0; j < settlement_iterator->second->HistoricalBuildings.size(); ++j) {
			if (
				CurrentCampaign->StartDate.ContainsDate(std::get<0>(settlement_iterator->second->HistoricalBuildings[j]))
				&& (!CurrentCampaign->StartDate.ContainsDate(std::get<1>(settlement_iterator->second->HistoricalBuildings[j])) || std::get<1>(settlement_iterator->second->HistoricalBuildings[j]).year == 0)
			) {
				int unit_type_id = -1;
				unit_type_id = PlayerRaces.GetFactionClassUnitType(settlement_owner->Civilization, settlement_owner->ID, std::get<2>(settlement_iterator->second->HistoricalBuildings[j]));
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
		for (size_t j = 0; j < settlement_iterator->second->HistoricalBuildings.size(); ++j) {
			if (
				CurrentCampaign->StartDate.ContainsDate(std::get<0>(settlement_iterator->second->HistoricalBuildings[j]))
				&& (!CurrentCampaign->StartDate.ContainsDate(std::get<1>(settlement_iterator->second->HistoricalBuildings[j])) || std::get<1>(settlement_iterator->second->HistoricalBuildings[j]).year == 0)
			) {
				CFaction *building_owner = std::get<4>(settlement_iterator->second->HistoricalBuildings[j]);
				int unit_type_id = -1;
				if (building_owner) {
					unit_type_id = PlayerRaces.GetFactionClassUnitType(building_owner->Civilization, building_owner->ID, std::get<2>(settlement_iterator->second->HistoricalBuildings[j]));
				} else {
					unit_type_id = PlayerRaces.GetFactionClassUnitType(settlement_owner->Civilization, settlement_owner->ID, std::get<2>(settlement_iterator->second->HistoricalBuildings[j]));
				}
				if (unit_type_id == -1) {
					continue;
				}
				const CUnitType *type = UnitTypes[unit_type_id];
				if (type->TerrainType) {
					continue;
				}
				if (type->BoolFlag[TOWNHALL_INDEX].value && !settlement_iterator->second->Major) {
					fprintf(stderr, "Error in CMap::ApplySettlements (settlement ident \"%s\"): settlement has a town hall, but isn't set as a major one.\n", settlement_iterator->second->Ident.c_str());
					continue;
				}
				Vec2i unit_offset((type->TileWidth - 1) / 2, (type->TileHeight - 1) / 2);
				CUnit *unit = NULL;
				if (building_owner) {
					CPlayer *building_player = GetOrAddFactionPlayer(building_owner);
					if (!building_player) {
						continue;
					}
					if (building_player->StartPos.x == 0 && building_player->StartPos.y == 0) {
						Vec2i default_pos(map_start_pos + ((building_owner->DefaultStartPos - template_start_pos) * this->Scale));
						if (building_owner->DefaultStartPos.x != -1 && building_owner->DefaultStartPos.y != -1 && Map.Info.IsPointOnMap(default_pos, z)) {
							building_player->SetStartView(default_pos, z);
						} else {
							building_player->SetStartView(settlement_pos - unit_offset, z);
						}
					}
					unit = CreateUnit(settlement_pos - unit_offset, *type, building_player, z);
				} else {
					unit = CreateUnit(settlement_pos - unit_offset, *type, player, z);
				}
				if (std::get<3>(settlement_iterator->second->HistoricalBuildings[j])) {
					unit->SetUnique(std::get<3>(settlement_iterator->second->HistoricalBuildings[j]));
				}
				if (first_building) {
					if (!type->BoolFlag[TOWNHALL_INDEX].value && !unit->Unique && (!building_owner || building_owner == settlement_owner) && settlement_iterator->second->CulturalNames.find(settlement_owner->Civilization) != settlement_iterator->second->CulturalNames.end()) { //if one building is representing a minor settlement, make it have the settlement's name
						unit->Name = settlement_iterator->second->CulturalNames.find(settlement_owner->Civilization)->second;
					}
					first_building = false;
				}
				if (type->BoolFlag[TOWNHALL_INDEX].value && (!building_owner || building_owner == settlement_owner) && settlement_iterator->second->CulturalNames.find(settlement_owner->Civilization) != settlement_iterator->second->CulturalNames.end()) {
					unit->UpdateBuildingSettlementAssignment();
				}
				if (pathway_type) {
					for (int x = unit->tilePos.x - 1; x < unit->tilePos.x + unit->Type->TileWidth + 1; ++x) {
						for (int y = unit->tilePos.y - 1; y < unit->tilePos.y + unit->Type->TileHeight + 1; ++y) {
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
		
		for (std::map<CUnitType *, std::map<CDate, std::pair<int, CFaction *>>>::iterator unit_iterator = settlement_iterator->second->HistoricalUnits.begin(); unit_iterator != settlement_iterator->second->HistoricalUnits.end(); ++unit_iterator) {
			const CUnitType *type = unit_iterator->first;

			for (std::map<CDate, std::pair<int, CFaction *>>::reverse_iterator second_unit_iterator = unit_iterator->second.rbegin(); second_unit_iterator != unit_iterator->second.rend(); ++second_unit_iterator) {
				if (CurrentCampaign->StartDate.ContainsDate(second_unit_iterator->first)) { // set the owner to the latest historical owner given the scenario's start date
					int unit_quantity = second_unit_iterator->second.first;
					
					if (unit_quantity > 0) {
						if (type->BoolFlag[ORGANIC_INDEX].value) {
							unit_quantity = std::max(1, unit_quantity / PopulationPerUnit); //each organic unit represents 1,000 people
						}
						
						CPlayer *unit_player = NULL;
						if (second_unit_iterator->second.second) {
							unit_player = GetOrAddFactionPlayer(second_unit_iterator->second.second);
							if (!unit_player) {
								break;
							}
							if (unit_player->StartPos.x == 0 && unit_player->StartPos.y == 0) {
								Vec2i default_pos(map_start_pos + ((second_unit_iterator->second.second->DefaultStartPos - template_start_pos) * this->Scale));
								if (second_unit_iterator->second.second->DefaultStartPos.x != -1 && second_unit_iterator->second.second->DefaultStartPos.y != -1 && Map.Info.IsPointOnMap(default_pos, z)) {
									unit_player->SetStartView(default_pos, z);
								} else {
									unit_player->SetStartView(settlement_pos, z);
								}
							}
						} else {
							unit_player = player;
						}
						Vec2i unit_offset((type->TileWidth - 1) / 2, (type->TileHeight - 1) / 2);
						
						for (int j = 0; j < unit_quantity; ++j) {
							CUnit *unit = CreateUnit(settlement_pos - unit_offset, *type, unit_player, z);
							if (!type->BoolFlag[HARVESTER_INDEX].value) { // make non-worker units not have an active AI
								unit->Active = 0;
								unit_player->UnitTypesAiActiveCount[type->Slot]--;
							}
						}
					}

					break;
				}
			}
		}
	}
}

void CMapTemplate::ApplyConnectors(Vec2i template_start_pos, Vec2i map_start_pos, int z, bool random)
{
	Vec2i map_end(std::min(Map.Info.MapWidths[z], map_start_pos.x + this->Width), std::min(Map.Info.MapHeights[z], map_start_pos.y + this->Height));

	for (size_t i = 0; i < this->PlaneConnectors.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(this->PlaneConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = Map.GenerateUnitLocation(std::get<1>(this->PlaneConnectors[i]), NULL, map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x || unit_pos.y < map_start_pos.y) {
			continue;
		}
		Vec2i unit_offset((std::get<1>(this->PlaneConnectors[i])->TileWidth - 1) / 2, (std::get<1>(this->PlaneConnectors[i])->TileHeight - 1) / 2);
		CUnit *unit = CreateUnit(unit_pos - unit_offset, *std::get<1>(this->PlaneConnectors[i]), &Players[PlayerNumNeutral], z);
		if (std::get<3>(this->PlaneConnectors[i])) {
			unit->SetUnique(std::get<3>(this->PlaneConnectors[i]));
		}
		Map.LayerConnectors[z].push_back(unit);
		for (size_t second_z = 0; second_z < Map.LayerConnectors.size(); ++second_z) {
			bool found_other_connector = false;
			if (Map.Planes[second_z] == std::get<2>(this->PlaneConnectors[i])) {
				for (size_t j = 0; j < Map.LayerConnectors[second_z].size(); ++j) {
					if (Map.LayerConnectors[second_z][j]->Type == unit->Type && Map.LayerConnectors[second_z][j]->Unique == unit->Unique && Map.LayerConnectors[second_z][j]->ConnectingDestination == NULL) {
						Map.LayerConnectors[second_z][j]->ConnectingDestination = unit;
						unit->ConnectingDestination = Map.LayerConnectors[second_z][j];
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
		Vec2i unit_raw_pos(std::get<0>(this->WorldConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = Map.GenerateUnitLocation(std::get<1>(this->WorldConnectors[i]), NULL, map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x || unit_pos.y < map_start_pos.y) {
			continue;
		}
		Vec2i unit_offset((std::get<1>(this->WorldConnectors[i])->TileWidth - 1) / 2, (std::get<1>(this->WorldConnectors[i])->TileHeight - 1) / 2);
		CUnit *unit = CreateUnit(unit_pos - unit_offset, *std::get<1>(this->WorldConnectors[i]), &Players[PlayerNumNeutral], z);
		if (std::get<3>(this->WorldConnectors[i])) {
			unit->SetUnique(std::get<3>(this->WorldConnectors[i]));
		}
		Map.LayerConnectors[z].push_back(unit);
		for (size_t second_z = 0; second_z < Map.LayerConnectors.size(); ++second_z) {
			bool found_other_connector = false;
			if (Map.Worlds[second_z] == std::get<2>(this->WorldConnectors[i])) {
				for (size_t j = 0; j < Map.LayerConnectors[second_z].size(); ++j) {
					if (Map.LayerConnectors[second_z][j]->Type == unit->Type && Map.LayerConnectors[second_z][j]->Unique == unit->Unique && Map.LayerConnectors[second_z][j]->ConnectingDestination == NULL) {
						Map.LayerConnectors[second_z][j]->ConnectingDestination = unit;
						unit->ConnectingDestination = Map.LayerConnectors[second_z][j];
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
	
	for (size_t i = 0; i < this->LayerConnectors.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(this->LayerConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = Map.GenerateUnitLocation(std::get<1>(this->LayerConnectors[i]), NULL, map_start_pos, map_end - Vec2i(1, 1), z);
		}
		if (!Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x || unit_pos.y < map_start_pos.y) {
			continue;
		}
		Vec2i unit_offset((std::get<1>(this->LayerConnectors[i])->TileWidth - 1) / 2, (std::get<1>(this->LayerConnectors[i])->TileHeight - 1) / 2);
		CUnit *unit = CreateUnit(unit_pos - unit_offset, *std::get<1>(this->LayerConnectors[i]), &Players[PlayerNumNeutral], z);
		if (std::get<3>(this->LayerConnectors[i])) {
			unit->SetUnique(std::get<3>(this->LayerConnectors[i]));
		}
		Map.LayerConnectors[z].push_back(unit);
		for (size_t second_z = 0; second_z < Map.LayerConnectors.size(); ++second_z) {
			bool found_other_connector = false;
			if (Map.Layers[second_z] == std::get<2>(this->LayerConnectors[i]) && Map.Worlds[second_z] == this->World && Map.Planes[second_z] == this->Plane) {
				for (size_t j = 0; j < Map.LayerConnectors[second_z].size(); ++j) {
					if (Map.LayerConnectors[second_z][j]->Type == unit->Type && Map.LayerConnectors[second_z][j]->tilePos == unit->tilePos && Map.LayerConnectors[second_z][j]->Unique == unit->Unique && Map.LayerConnectors[second_z][j]->ConnectingDestination == NULL) { //surface layer connectors need to be in the same X and Y coordinates as their destinations
						Map.LayerConnectors[second_z][j]->ConnectingDestination = unit;
						unit->ConnectingDestination = Map.LayerConnectors[second_z][j];
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
		
		if ((!CurrentCampaign || std::get<3>(this->Units[i]).year == 0 || CurrentCampaign->StartDate.ContainsDate(std::get<3>(this->Units[i]))) && (std::get<4>(this->Units[i]).year == 0 || !CurrentCampaign->StartDate.ContainsDate(std::get<4>(this->Units[i])))) {
			CPlayer *player = NULL;
			if (std::get<2>(this->Units[i])) {
				player = GetOrAddFactionPlayer(std::get<2>(this->Units[i]));
				if (!player) {
					continue;
				}
				if (player->StartPos.x == 0 && player->StartPos.y == 0) {
					Vec2i default_pos(map_start_pos + std::get<2>(this->Units[i])->DefaultStartPos - template_start_pos);
					if (std::get<2>(this->Units[i])->DefaultStartPos.x != -1 && std::get<2>(this->Units[i])->DefaultStartPos.y != -1 && Map.Info.IsPointOnMap(default_pos, z)) {
						player->SetStartView(default_pos, z);
					} else {
						player->SetStartView(unit_pos, z);
					}
				}
			} else {
				player = &Players[PlayerNumNeutral];
			}
			Vec2i unit_offset((type->TileWidth - 1) / 2, (type->TileHeight - 1) / 2);
			CUnit *unit = CreateUnit(unit_pos - unit_offset, *std::get<1>(this->Units[i]), player, z);
			if (!type->BoolFlag[BUILDING_INDEX].value) { // make non-building units not have an active AI
				unit->Active = 0;
				player->UnitTypesAiActiveCount[type->Slot]--;
			}
			if (std::get<5>(this->Units[i])) {
				unit->SetUnique(std::get<5>(this->Units[i]));
			}
		}
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
		
		if ((!CurrentCampaign || std::get<3>(this->Heroes[i]).year == 0 || CurrentCampaign->StartDate.ContainsDate(std::get<3>(this->Heroes[i]))) && (std::get<4>(this->Heroes[i]).year == 0 || !CurrentCampaign->StartDate.ContainsDate(std::get<4>(this->Heroes[i])))) {
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
			Vec2i unit_offset((hero->Type->TileWidth - 1) / 2, (hero->Type->TileHeight - 1) / 2);
			CUnit *unit = CreateUnit(unit_pos - unit_offset, *hero->Type, player, z);
			unit->SetCharacter(hero->Ident);
			unit->Active = 0;
			player->UnitTypesAiActiveCount[hero->Type->Slot]--;
		}
	}
	
	if (this->IsSubtemplateArea() || random) { //don't perform the dynamic hero application if this is a subtemplate area, to avoid creating multiple copies of the same hero
		return;
	}
	
	for (std::map<std::string, CCharacter *>::iterator iterator = Characters.begin(); iterator != Characters.end(); ++iterator) {
		CCharacter *hero = iterator->second;
		
		if (hero->Deity != NULL) {
			continue;
		}
		
		if (hero->Faction == NULL && !hero->Type->BoolFlag[FAUNA_INDEX].value) { //only fauna "heroes" may have no faction
			continue;
		}
		
		if (hero->Date.year == 0 || !CurrentCampaign->StartDate.ContainsDate(hero->Date) || CurrentCampaign->StartDate.ContainsDate(hero->DeathDate)) { //contrary to other elements, heroes aren't implemented if their date isn't set
			continue;
		}

		CPlayer *hero_player = hero->Faction ? GetFactionPlayer(hero->Faction) : NULL;
		
		Vec2i hero_pos(-1, -1);
		
		if (hero_player && hero_player->StartMapLayer == z) {
			hero_pos = hero_player->StartPos;
		}
		
		bool has_map_template_location = false;
		for (int i = ((int) hero->HistoricalLocations.size() - 1); i >= 0; --i) {
			if (CurrentCampaign->StartDate.ContainsDate(std::get<0>(hero->HistoricalLocations[i]))) {
				if (std::get<1>(hero->HistoricalLocations[i]) == this) {
					hero_pos = map_start_pos + std::get<2>(hero->HistoricalLocations[i]) - template_start_pos;
				}

				break;
			}
		}
		
		if (!Map.Info.IsPointOnMap(hero_pos, z) || hero_pos.x < map_start_pos.x || hero_pos.y < map_start_pos.y) { //heroes whose faction hasn't been created already and who don't have a historical location set won't be created
			continue;
		}
		
		if (hero->Faction) {
			hero_player = GetOrAddFactionPlayer(hero->Faction);
			if (!hero_player) {
				continue;
			}
			if (hero_player->StartPos.x == 0 && hero_player->StartPos.y == 0) {
				hero_player->SetStartView(hero_pos, z);
			}
		} else {
			hero_player = &Players[PlayerNumNeutral];
		}
		Vec2i unit_offset((hero->Type->TileWidth - 1) / 2, (hero->Type->TileHeight - 1) / 2);
		CUnit *unit = CreateUnit(hero_pos - unit_offset, *hero->Type, hero_player, z);
		unit->SetCharacter(hero->Ident);
		unit->Active = 0;
		hero_player->UnitTypesAiActiveCount[hero->Type->Slot]--;
	}
}

bool CMapTemplate::IsSubtemplateArea()
{
	return this->MainTemplate != NULL;
}

/**
**  Get a settlement's cultural name.
*/
std::string CSettlement::GetCulturalName(int civilization)
{
	if (civilization != -1 && this->CulturalNames.find(civilization) != this->CulturalNames.end()) {
		return this->CulturalNames[civilization];
	} else {
		return this->Name;
	}
}
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Visible and explored handling
----------------------------------------------------------------------------*/

/**
**  Marks seen tile -- used mainly for the Fog Of War
**
**  @param mf  MapField-position.
*/
//Wyrmgus start
//void CMap::MarkSeenTile(CMapField &mf)
void CMap::MarkSeenTile(CMapField &mf, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	const unsigned int tile = mf.getGraphicTile();
//	const unsigned int seentile = mf.playerInfo.SeenTile;
	//Wyrmgus end

	//  Nothing changed? Seeing already the correct tile.
	//Wyrmgus start
//	if (tile == seentile) {
	if (mf.IsSeenTileCorrect()) {
	//Wyrmgus end
		return;
	}
	//Wyrmgus start
//	mf.playerInfo.SeenTile = tile;
	mf.UpdateSeenTile();
	//Wyrmgus end

#ifdef MINIMAP_UPDATE
	//rb - GRRRRRRRRRRRR
	//Wyrmgus start
//	const unsigned int index = &mf - Map.Fields;
//	const int y = index / Info.MapWidth;
//	const int x = index - (y * Info.MapWidth);
	const unsigned int index = &mf - Map.Fields[z];
	const int y = index / Info.MapWidths[z];
	const int x = index - (y * Info.MapWidths[z]);
	//Wyrmgus end
	const Vec2i pos = {x, y}
#endif

	//Wyrmgus start
	/*
	if (this->Tileset->TileTypeTable.empty() == false) {
#ifndef MINIMAP_UPDATE
		//rb - GRRRRRRRRRRRR
		const unsigned int index = &mf - Map.Fields;
		const int y = index / Info.MapWidth;
		const int x = index - (y * Info.MapWidth);
		const Vec2i pos(x, y);
#endif

		//  Handle wood changes. FIXME: check if for growing wood correct?
		if (tile == this->Tileset->getRemovedTreeTile()) {
			FixNeighbors(MapFieldForest, 1, pos);
		} else if (seentile == this->Tileset->getRemovedTreeTile()) {
			FixTile(MapFieldForest, 1, pos);
		} else if (mf.ForestOnMap()) {
			FixTile(MapFieldForest, 1, pos);
			FixNeighbors(MapFieldForest, 1, pos);

			// Handle rock changes.
		} else if (tile == Tileset->getRemovedRockTile()) {
			FixNeighbors(MapFieldRocks, 1, pos);
		} else if (seentile == Tileset->getRemovedRockTile()) {
			FixTile(MapFieldRocks, 1, pos);
		} else if (mf.RockOnMap()) {
			FixTile(MapFieldRocks, 1, pos);
			FixNeighbors(MapFieldRocks, 1, pos);

			//  Handle Walls changes.
		} else if (this->Tileset->isAWallTile(tile)
				   || this->Tileset->isAWallTile(seentile)) {
		//Wyrmgus end
			MapFixSeenWallTile(pos);
			MapFixSeenWallNeighbors(pos);
		}
	}
	*/
	//Wyrmgus end

#ifdef MINIMAP_UPDATE
	//Wyrmgus start
//	UI.Minimap.UpdateXY(pos);
	UI.Minimap.UpdateXY(pos, z);
	//Wyrmgus end
#endif
}

/**
**  Reveal the entire map.
*/
//Wyrmgus start
//void CMap::Reveal()
void CMap::Reveal(bool only_person_players)
//Wyrmgus end
{
	//  Mark every explored tile as visible. 1 turns into 2.
	//Wyrmgus start
	/*
	for (int i = 0; i != this->Info.MapWidth * this->Info.MapHeight; ++i) {
		CMapField &mf = *this->Field(i);
		CMapFieldPlayerInfo &playerInfo = mf.playerInfo;
		for (int p = 0; p < PlayerMax; ++p) {
			//Wyrmgus start
//			playerInfo.Visible[p] = std::max<unsigned short>(1, playerInfo.Visible[p]);
			if (Players[p].Type == PlayerPerson || !only_person_players) {
				playerInfo.Visible[p] = std::max<unsigned short>(1, playerInfo.Visible[p]);
			}
			//Wyrmgus end
		}
		MarkSeenTile(mf);
	}
	*/
	for (size_t z = 0; z < this->Fields.size(); ++z) {
		for (int i = 0; i != this->Info.MapWidths[z] * this->Info.MapHeights[z]; ++i) {
			CMapField &mf = *this->Field(i, z);
			CMapFieldPlayerInfo &playerInfo = mf.playerInfo;
			for (int p = 0; p < PlayerMax; ++p) {
				if (Players[p].Type == PlayerPerson || !only_person_players) {
					playerInfo.Visible[p] = std::max<unsigned short>(1, playerInfo.Visible[p]);
				}
			}
			MarkSeenTile(mf, z);
		}
	}
	//Wyrmgus end
	//  Global seen recount. Simple and effective.
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit &unit = **it;
		//  Reveal neutral buildings. Gold mines:)
		if (unit.Player->Type == PlayerNeutral) {
			for (int p = 0; p < PlayerMax; ++p) {
				//Wyrmgus start
//				if (Players[p].Type != PlayerNobody && (!(unit.Seen.ByPlayer & (1 << p)))) {
				if (Players[p].Type != PlayerNobody && (Players[p].Type == PlayerPerson || !only_person_players) && (!(unit.Seen.ByPlayer & (1 << p)))) {
				//Wyrmgus end
					UnitGoesOutOfFog(unit, Players[p]);
					UnitGoesUnderFog(unit, Players[p]);
				}
			}
		}
		UnitCountSeen(unit);
	}
}

/*----------------------------------------------------------------------------
--  Map queries
----------------------------------------------------------------------------*/

Vec2i CMap::MapPixelPosToTilePos(const PixelPos &mapPos) const
{
	const Vec2i tilePos(mapPos.x / PixelTileSize.x, mapPos.y / PixelTileSize.y);

	return tilePos;
}

PixelPos CMap::TilePosToMapPixelPos_TopLeft(const Vec2i &tilePos) const
{
	PixelPos mapPixelPos(tilePos.x * PixelTileSize.x, tilePos.y * PixelTileSize.y);

	return mapPixelPos;
}

PixelPos CMap::TilePosToMapPixelPos_Center(const Vec2i &tilePos) const
{
	return TilePosToMapPixelPos_TopLeft(tilePos) + PixelTileSize / 2;
}

//Wyrmgus start
CTerrainType *CMap::GetTileTerrain(const Vec2i &pos, bool overlay, int z) const
{
	if (!Map.Info.IsPointOnMap(pos, z)) {
		return NULL;
	}
	
	CMapField &mf = *this->Field(pos, z);
	
	if (overlay) {
		return mf.OverlayTerrain;
	} else {
		return mf.Terrain;
	}
}

CTerrainType *CMap::GetTileTopTerrain(const Vec2i &pos, bool seen, int z) const
{
	if (!Map.Info.IsPointOnMap(pos, z)) {
		return NULL;
	}
	
	CMapField &mf = *this->Field(pos, z);
	
	if (!seen) {
		if (mf.OverlayTerrain) {
			return mf.OverlayTerrain;
		} else {
			return mf.Terrain;
		}
	} else {
		if (mf.playerInfo.SeenOverlayTerrain) {
			return mf.playerInfo.SeenOverlayTerrain;
		} else {
			return mf.playerInfo.SeenTerrain;
		}
	}
}

int CMap::GetTileLandmass(const Vec2i &pos, int z) const
{
	if (!Map.Info.IsPointOnMap(pos, z)) {
		return 0;
	}
	
	CMapField &mf = *this->Field(pos, z);
	
	return mf.Landmass;
}

Vec2i CMap::GenerateUnitLocation(const CUnitType *unit_type, CFaction *faction, Vec2i min_pos, Vec2i max_pos, int z) const
{
	if (SaveGameLoading) {
		return Vec2i(-1, -1);
	}
	
	CPlayer *player = GetFactionPlayer(faction);
	if (!unit_type->BoolFlag[TOWNHALL_INDEX].value && player != NULL && player->StartMapLayer == z && player->StartPos.x >= min_pos.x && player->StartPos.x <= max_pos.x && player->StartPos.y >= min_pos.y && player->StartPos.y <= max_pos.y) {
		return player->StartPos;
	}
	
	Vec2i random_pos(-1, -1);
	
	std::vector<CTerrainType *> allowed_terrains;
	if (unit_type->BoolFlag[FAUNA_INDEX].value && unit_type->Species) { //if the unit is a fauna one, it has to start on terrain it is native to
		for (size_t i = 0; i < unit_type->Species->Terrains.size(); ++i) {
			allowed_terrains.push_back(unit_type->Species->Terrains[i]);
		}
	}
	
	for (size_t i = 0; i < unit_type->SpawnUnits.size(); ++i) {
		CUnitType *spawned_type = unit_type->SpawnUnits[i];
		if (spawned_type->BoolFlag[FAUNA_INDEX].value && spawned_type->Species) {
			for (size_t j = 0; j < spawned_type->Species->Terrains.size(); ++j) {
				allowed_terrains.push_back(spawned_type->Species->Terrains[j]);
			}
		}
	}

	int while_count = 0;
	
	while (while_count < 100) {
		random_pos.x = SyncRand(max_pos.x - (unit_type->TileWidth - 1) - min_pos.x + 1) + min_pos.x;
		random_pos.y = SyncRand(max_pos.y - (unit_type->TileHeight - 1) - min_pos.y + 1) + min_pos.y;
		
		if (!this->Info.IsPointOnMap(random_pos, z) || (this->IsPointInASubtemplateArea(random_pos, z) && GameCycle == 0)) {
			continue;
		}
		
		if (allowed_terrains.size() > 0 && std::find(allowed_terrains.begin(), allowed_terrains.end(), GetTileTopTerrain(random_pos, false, z)) == allowed_terrains.end()) { //if the unit is a fauna one, it has to start on terrain it is native to
			while_count += 1;
			continue;
		}
		
		std::vector<CUnit *> table;
		if (player != NULL) {
			Select(random_pos - Vec2i(32, 32), random_pos + Vec2i(unit_type->TileWidth - 1, unit_type->TileHeight - 1) + Vec2i(32, 32), table, z, MakeAndPredicate(HasNotSamePlayerAs(*player), HasNotSamePlayerAs(Players[PlayerNumNeutral])));
		} else if (!unit_type->GivesResource) {
			if (unit_type->BoolFlag[PREDATOR_INDEX].value || (unit_type->BoolFlag[PEOPLEAVERSION_INDEX].value && unit_type->UnitType == UnitTypeFly)) {
				Select(random_pos - Vec2i(16, 16), random_pos + Vec2i(unit_type->TileWidth - 1, unit_type->TileHeight - 1) + Vec2i(16, 16), table, z, HasNotSamePlayerAs(Players[PlayerNumNeutral]));
			} else {
				Select(random_pos - Vec2i(8, 8), random_pos + Vec2i(unit_type->TileWidth - 1, unit_type->TileHeight - 1) + Vec2i(8, 8), table, z, HasNotSamePlayerAs(Players[PlayerNumNeutral]));
			}
		} else if (unit_type->GivesResource && !unit_type->BoolFlag[BUILDING_INDEX].value) { //for non-building resources (i.e. wood piles), place them within a certain distance of player units, to prevent them from blocking the way
			Select(random_pos - Vec2i(4, 4), random_pos + Vec2i(unit_type->TileWidth - 1, unit_type->TileHeight - 1) + Vec2i(4, 4), table, z, HasNotSamePlayerAs(Players[PlayerNumNeutral]));
		}
		
		if (table.size() == 0) {
			bool passable_surroundings = true; //check if the unit won't be placed next to unpassable terrain
			for (int x = random_pos.x - 1; x < random_pos.x + unit_type->TileWidth + 1; ++x) {
				for (int y = random_pos.y - 1; y < random_pos.y + unit_type->TileHeight + 1; ++y) {
					if (Map.Info.IsPointOnMap(x, y, z) && Map.Field(x, y, z)->CheckMask(MapFieldUnpassable)) {
						passable_surroundings = false;
					}
				}
			}
			if (passable_surroundings && UnitTypeCanBeAt(*unit_type, random_pos, z) && (!unit_type->BoolFlag[BUILDING_INDEX].value || CanBuildUnitType(NULL, *unit_type, random_pos, 0, true, z))) {
				return random_pos;
			}
		}
		
		while_count += 1;
	}
	
	return Vec2i(-1, -1);
}

std::string CMap::GetSettlementName(const Vec2i &pos, int z, const Vec2i &tile_size, int civilization, int faction) const
{
	int settlement_x = pos.x + ((tile_size.x - 1) / 2);
	int settlement_y = pos.y + ((tile_size.y - 1) / 2);

	if (civilization != -1 && faction != -1 && this->FactionCulturalSettlementNames[z].find(std::tuple<int, int, CFaction *>(settlement_x, settlement_y, PlayerRaces.Factions[civilization][faction])) != this->FactionCulturalSettlementNames[z].end()) {
		return this->FactionCulturalSettlementNames[z].find(std::tuple<int, int, CFaction *>(settlement_x, settlement_y, PlayerRaces.Factions[civilization][faction]))->second;
	}

	// if didn't find a faction cultural settlement name for the exact tile, try for any of the tiles the town hall occupies
	for (int x_offset = 0; x_offset < tile_size.x; ++x_offset) {
		for (int y_offset = 0; y_offset < tile_size.y; ++y_offset) {
			int second_settlement_x = pos.x + x_offset;
			int second_settlement_y = pos.y + y_offset;
			
			if (civilization != -1 && faction != -1 && this->FactionCulturalSettlementNames[z].find(std::tuple<int, int, CFaction *>(second_settlement_x, second_settlement_y, PlayerRaces.Factions[civilization][faction])) != this->FactionCulturalSettlementNames[z].end()) {
				return this->FactionCulturalSettlementNames[z].find(std::tuple<int, int, CFaction *>(second_settlement_x, second_settlement_y, PlayerRaces.Factions[civilization][faction]))->second;
			}
		}
	}
	
	if (civilization != -1 && faction != -1 && PlayerRaces.Factions[civilization][faction]->ParentFaction != -1) {
		return this->GetSettlementName(pos, z, tile_size, civilization, PlayerRaces.Factions[civilization][faction]->ParentFaction);
	}
		
	if (civilization != -1 && this->CulturalSettlementNames[z].find(std::tuple<int, int, int>(settlement_x, settlement_y, civilization)) != this->CulturalSettlementNames[z].end()) {
		return this->CulturalSettlementNames[z].find(std::tuple<int, int, int>(settlement_x, settlement_y, civilization))->second;
	}
	
	// if didn't find a cultural settlement name for the exact tile, try for any of the tiles the town hall occupies
	for (int x_offset = 0; x_offset < tile_size.x; ++x_offset) {
		for (int y_offset = 0; y_offset < tile_size.y; ++y_offset) {
			int second_settlement_x = pos.x + x_offset;
			int second_settlement_y = pos.y + y_offset;
			
			if (civilization != -1 && this->CulturalSettlementNames[z].find(std::tuple<int, int, int>(second_settlement_x, second_settlement_y, civilization)) != this->CulturalSettlementNames[z].end()) {
				return this->CulturalSettlementNames[z].find(std::tuple<int, int, int>(second_settlement_x, second_settlement_y, civilization))->second;
			}
		}
	}

	return "";
}
//Wyrmgus end

/**
**  Wall on map tile.
**
**  @param pos  map tile position.
**
**  @return    True if wall, false otherwise.
*/
//Wyrmgus start
//bool CMap::WallOnMap(const Vec2i &pos) const
bool CMap::WallOnMap(const Vec2i &pos, int z) const
//Wyrmgus end
{
	//Wyrmgus start
//	Assert(Map.Info.IsPointOnMap(pos));
//	return Field(pos)->isAWall();
	Assert(Map.Info.IsPointOnMap(pos, z));
	return Field(pos, z)->isAWall();
	//Wyrmgus end
}

/**
**  Human wall on map tile.
**
**  @param pos  map tile position.
**
**  @return    True if human wall, false otherwise.
*/
//Wyrmgus start
/*
bool CMap::HumanWallOnMap(const Vec2i &pos) const
{
	Assert(Map.Info.IsPointOnMap(pos));
	return Field(pos)->isAHumanWall();
}
*/
//Wyrmgus end

/**
**  Orc wall on map tile.
**
**  @param pos  map tile position.
**
**  @return    True if orcish wall, false otherwise.
*/
//Wyrmgus start
/*
bool CMap::OrcWallOnMap(const Vec2i &pos) const
{
	Assert(Map.Info.IsPointOnMap(pos));
	return Field(pos)->isAOrcWall();
}
*/
//Wyrmgus end

//Wyrmgus start
bool CMap::CurrentTerrainCanBeAt(const Vec2i &pos, bool overlay, int z)
{
	CMapField &mf = *this->Field(pos, z);
	CTerrainType *terrain = NULL;
	
	if (overlay) {
		terrain = mf.OverlayTerrain;
	} else {
		terrain = mf.Terrain;
	}
	
	if (!terrain) {
		return true;
	}
	
	if (terrain->AllowSingle) {
		return true;
	}

	std::vector<int> transition_directions;
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
						
					CTerrainType *adjacent_terrain = this->GetTileTerrain(adjacent_pos, overlay, z);
					if (overlay && adjacent_terrain && this->Field(adjacent_pos, z)->OverlayTerrainDestroyed) {
						adjacent_terrain = NULL;
					}
					if (terrain != adjacent_terrain) { // also happens if terrain is NULL, so that i.e. tree transitions display correctly when adjacent to tiles without overlays
						transition_directions.push_back(GetDirectionFromOffset(x_offset, y_offset));
					}
				}
			}
		}
	}
	
	if (std::find(transition_directions.begin(), transition_directions.end(), North) != transition_directions.end() && std::find(transition_directions.begin(), transition_directions.end(), South) != transition_directions.end()) {
		return false;
	} else if (std::find(transition_directions.begin(), transition_directions.end(), West) != transition_directions.end() && std::find(transition_directions.begin(), transition_directions.end(), East) != transition_directions.end()) {
		return false;
	}

	return true;
}

bool CMap::TileBordersOnlySameTerrain(const Vec2i &pos, CTerrainType *new_terrain, int z)
{
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info.IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			if (this->IsPointInASubtemplateArea(pos, z) && !this->IsPointInASubtemplateArea(adjacent_pos, z)) {
				continue;
			}
			CTerrainType *top_terrain = GetTileTopTerrain(pos, false, z);
			CTerrainType *adjacent_top_terrain = GetTileTopTerrain(adjacent_pos, false, z);
			if (!new_terrain->Overlay) {
				if (
					adjacent_top_terrain
					&& adjacent_top_terrain != top_terrain
					&& (std::find(top_terrain->InnerBorderTerrains.begin(), top_terrain->InnerBorderTerrains.end(), adjacent_top_terrain) == top_terrain->InnerBorderTerrains.end() || std::find(new_terrain->InnerBorderTerrains.begin(), new_terrain->InnerBorderTerrains.end(), adjacent_top_terrain) == new_terrain->InnerBorderTerrains.end())
					&& adjacent_top_terrain != new_terrain
				) {
					return false;
				}
			} else {
				if (
					adjacent_top_terrain
					&& adjacent_top_terrain != top_terrain
					&& std::find(top_terrain->BaseTerrains.begin(), top_terrain->BaseTerrains.end(), adjacent_top_terrain) == top_terrain->BaseTerrains.end() && std::find(adjacent_top_terrain->BaseTerrains.begin(), adjacent_top_terrain->BaseTerrains.end(), top_terrain) == adjacent_top_terrain->BaseTerrains.end()
					&& adjacent_top_terrain != new_terrain
				) {
					return false;
				}
			}
		}
	}
		
	return true;
}

bool CMap::TileBordersFlag(const Vec2i &pos, int z, int flag, bool reverse)
{
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info.IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			CMapField &mf = *Map.Field(adjacent_pos, z);
			
			if ((!reverse && mf.CheckMask(flag)) || (reverse && !mf.CheckMask(flag))) {
				return true;
			}
		}
	}
		
	return false;
}

bool CMap::TileBordersBuilding(const Vec2i &pos, int z)
{
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info.IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			CMapField &mf = *Map.Field(adjacent_pos, z);
			
			if (mf.CheckMask(MapFieldBuilding)) {
				return true;
			}
		}
	}
		
	return false;
}

bool CMap::TileBordersPathway(const Vec2i &pos, int z, bool only_railroad)
{
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info.IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			CMapField &mf = *Map.Field(adjacent_pos, z);
			
			if (
				(!only_railroad && mf.CheckMask(MapFieldRoad))
				|| mf.CheckMask(MapFieldRailroad)
			) {
				return true;
			}
		}
	}
		
	return false;
}

bool CMap::TileBordersUnit(const Vec2i &pos, int z)
{
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info.IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			CMapField &mf = *Map.Field(adjacent_pos, z);
			
			const CUnitCache &cache = mf.UnitCache;
			for (size_t i = 0; i != cache.size(); ++i) {
				CUnit &unit = *cache[i];
				if (unit.IsAliveOnMap()) {
					return true;
				}
			}
		}
	}
		
	return false;
}

bool CMap::TileHasUnitsIncompatibleWithTerrain(const Vec2i &pos, CTerrainType *terrain, int z)
{
	CMapField &mf = *Map.Field(pos, z);
	
	const CUnitCache &cache = mf.UnitCache;
	for (size_t i = 0; i != cache.size(); ++i) {
		CUnit &unit = *cache[i];
		if (unit.IsAliveOnMap() && (terrain->Flags & unit.Type->MovementMask) != 0) {
			return true;
		}
	}

	return false;
}

bool CMap::IsPointInASubtemplateArea(const Vec2i &pos, int z) const
{
	if (this->SubtemplateAreas.find(z) == this->SubtemplateAreas.end()) {
		return false;
	}
	
	for (size_t i = 0; i < this->SubtemplateAreas.find(z)->second.size(); ++i) {
		Vec2i min_pos = std::get<0>(this->SubtemplateAreas.find(z)->second[i]);
		Vec2i max_pos = std::get<1>(this->SubtemplateAreas.find(z)->second[i]);
		if (pos.x >= min_pos.x && pos.y >= min_pos.y && pos.x <= max_pos.x && pos.y <= max_pos.y) {
			return true;
		}
	}

	return false;
}

bool CMap::IsLayerUnderground(int z) const
{
	if (GameSettings.Inside) {
		return true;
	}
	
	if (this->Layers[z] > 0) {
		return true;
	}

	return false;
}
//Wyrmgus end

/**
**  Can move to this point, applying mask.
**
**  @param pos   map tile position.
**  @param mask  Mask for movement to apply.
**
**  @return      True if could be entered, false otherwise.
*/
//Wyrmgus start
//bool CheckedCanMoveToMask(const Vec2i &pos, int mask)
bool CheckedCanMoveToMask(const Vec2i &pos, int mask, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	return Map.Info.IsPointOnMap(pos) && CanMoveToMask(pos, mask);
	return Map.Info.IsPointOnMap(pos, z) && CanMoveToMask(pos, mask, z);
	//Wyrmgus end
}

/**
**  Can a unit of unit-type be placed at this point.
**
**  @param type  unit-type to be checked.
**  @param pos   map tile position.
**
**  @return      True if could be entered, false otherwise.
*/
//Wyrmgus start
//bool UnitTypeCanBeAt(const CUnitType &type, const Vec2i &pos)
bool UnitTypeCanBeAt(const CUnitType &type, const Vec2i &pos, int z)
//Wyrmgus end
{
	const int mask = type.MovementMask;
	//Wyrmgus start
//	unsigned int index = pos.y * Map.Info.MapWidth;
	unsigned int index = pos.y * Map.Info.MapWidths[z];
	//Wyrmgus end

	for (int addy = 0; addy < type.TileHeight; ++addy) {
		for (int addx = 0; addx < type.TileWidth; ++addx) {
			//Wyrmgus start
			/*
			if (Map.Info.IsPointOnMap(pos.x + addx, pos.y + addy) == false
				|| Map.Field(pos.x + addx + index)->CheckMask(mask) == true) {
			*/
			if (Map.Info.IsPointOnMap(pos.x + addx, pos.y + addy, z) == false
				|| Map.Field(pos.x + addx + index, z)->CheckMask(mask) == true) {
			//Wyrmgus end
				return false;
			}
		}
		//Wyrmgus start
//		index += Map.Info.MapWidth;
		index += Map.Info.MapWidths[z];
		//Wyrmgus end
	}
	return true;
}

/**
**  Can a unit be placed to this point.
**
**  @param unit  unit to be checked.
**  @param pos   map tile position.
**
**  @return      True if could be placeded, false otherwise.
*/
//Wyrmgus start
//bool UnitCanBeAt(const CUnit &unit, const Vec2i &pos)
bool UnitCanBeAt(const CUnit &unit, const Vec2i &pos, int z)
//Wyrmgus end
{
	Assert(unit.Type);
	if (unit.Type->BoolFlag[NONSOLID_INDEX].value) {
		return true;
	}
	//Wyrmgus start
//	return UnitTypeCanBeAt(*unit.Type, pos);
	return UnitTypeCanBeAt(*unit.Type, pos, z);
	//Wyrmgus end
}

/**
**  Fixes initially the wood and seen tiles.
*/
void PreprocessMap()
{
	//Wyrmgus start
	/*
	for (int ix = 0; ix < Map.Info.MapWidth; ++ix) {
		for (int iy = 0; iy < Map.Info.MapHeight; ++iy) {
			CMapField &mf = *Map.Field(ix, iy);
			mf.playerInfo.SeenTile = mf.getGraphicTile();
		}
	}
	*/
	for (size_t z = 0; z < Map.Fields.size(); ++z) {
		for (int ix = 0; ix < Map.Info.MapWidths[z]; ++ix) {
			for (int iy = 0; iy < Map.Info.MapHeights[z]; ++iy) {
				CMapField &mf = *Map.Field(ix, iy, z);
				Map.CalculateTileTransitions(Vec2i(ix, iy), false, z);
				Map.CalculateTileTransitions(Vec2i(ix, iy), true, z);
				Map.CalculateTileLandmass(Vec2i(ix, iy), z);
				Map.CalculateTileOwnership(Vec2i(ix, iy), z);
				mf.UpdateSeenTile();
				UI.Minimap.UpdateXY(Vec2i(ix, iy), z);
				if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
					Map.MarkSeenTile(mf, z);
				}
			}
		}
	}
	//Wyrmgus end
	//Wyrmgus start
	/*
	// it is required for fixing the wood that all tiles are marked as seen!
	if (Map.Tileset->TileTypeTable.empty() == false) {
		Vec2i pos;
		for (pos.x = 0; pos.x < Map.Info.MapWidth; ++pos.x) {
			for (pos.y = 0; pos.y < Map.Info.MapHeight; ++pos.y) {
				MapFixWallTile(pos);
				MapFixSeenWallTile(pos);
			}
		}
	}
	*/
	//Wyrmgus end
}

//Wyrmgus start
int GetMapLayer(std::string plane_name, std::string world_name, int surface_layer)
{
	CPlane *plane = GetPlane(plane_name);
	CWorld *world = GetWorld(world_name);

	for (size_t z = 0; z < Map.Fields.size(); ++z) {
		if (Map.Planes[z] == plane && Map.Worlds[z] == world && Map.Layers[z] == surface_layer) {
			return z;
		}
	}
	
	return -1;
}

int GetSubtemplateStartX(std::string subtemplate_ident)
{
	CMapTemplate *subtemplate = GetMapTemplate(subtemplate_ident);
	
	if (!subtemplate) {
		return -1;
	}

	for (size_t z = 0; z < Map.Fields.size(); ++z) {
		if (Map.SubtemplateAreas.find(z) == Map.SubtemplateAreas.end()) {
			continue;
		}
		for (size_t i = 0; i < Map.SubtemplateAreas.find(z)->second.size(); ++i) {
			Vec2i min_pos = std::get<0>(Map.SubtemplateAreas.find(z)->second[i]);
			if (subtemplate == std::get<2>(Map.SubtemplateAreas.find(z)->second[i])) {
				return min_pos.x;
			}
		}
	}

	return -1;
}

int GetSubtemplateStartY(std::string subtemplate_ident)
{
	CMapTemplate *subtemplate = GetMapTemplate(subtemplate_ident);
	
	if (!subtemplate) {
		return -1;
	}

	for (size_t z = 0; z < Map.Fields.size(); ++z) {
		if (Map.SubtemplateAreas.find(z) == Map.SubtemplateAreas.end()) {
			continue;
		}
		for (size_t i = 0; i < Map.SubtemplateAreas.find(z)->second.size(); ++i) {
			Vec2i min_pos = std::get<0>(Map.SubtemplateAreas.find(z)->second[i]);
			if (subtemplate == std::get<2>(Map.SubtemplateAreas.find(z)->second[i])) {
				return min_pos.y;
			}
		}
	}

	return -1;
}

void ChangeCurrentMapLayer(int z)
{
	if (z < 0 || z >= (int) Map.Fields.size() || CurrentMapLayer == z) {
		return;
	}
	
	Vec2i new_viewport_map_pos(UI.SelectedViewport->MapPos.x * Map.Info.MapWidths[z] / Map.Info.MapWidths[CurrentMapLayer], UI.SelectedViewport->MapPos.y * Map.Info.MapHeights[z] / Map.Info.MapHeights[CurrentMapLayer]);
	
	CurrentMapLayer = z;
	UI.Minimap.UpdateCache = true;
	UI.SelectedViewport->Set(new_viewport_map_pos, PixelTileSize / 2);
}

void SetTimeOfDay(int time_of_day, int z)
{
	Map.TimeOfDay[z] = time_of_day;
}
//Wyrmgus end

/**
**  Clear CMapInfo.
*/
void CMapInfo::Clear()
{
	this->Description.clear();
	this->Filename.clear();
	this->MapWidth = this->MapHeight = 0;
	//Wyrmgus start
	this->MapWidths.clear();
	this->MapHeights.clear();
	//Wyrmgus end
	memset(this->PlayerSide, 0, sizeof(this->PlayerSide));
	memset(this->PlayerType, 0, sizeof(this->PlayerType));
	this->MapUID = 0;
}

//Wyrmgus start
//CMap::CMap() : Fields(NULL), NoFogOfWar(false), TileGraphic(NULL)
CMap::CMap() : NoFogOfWar(false), TileGraphic(NULL), Landmasses(0), BorderTerrain(NULL)
//Wyrmgus end
{
	Tileset = new CTileset;
}

CMap::~CMap()
{
	delete Tileset;
}

/**
**  Alocate and initialise map table
*/
void CMap::Create()
{
	//Wyrmgus start
//	Assert(!this->Fields);
	Assert(this->Fields.size() == 0);
	//Wyrmgus end

	//Wyrmgus start
//	this->Fields = new CMapField[this->Info.MapWidth * this->Info.MapHeight];
	this->Fields.push_back(new CMapField[this->Info.MapWidth * this->Info.MapHeight]);
	this->Info.MapWidths.push_back(this->Info.MapWidth);
	this->Info.MapHeights.push_back(this->Info.MapHeight);
	this->TimeOfDaySeconds.push_back(DefaultTimeOfDaySeconds);
	if (!GameSettings.Inside && !GameSettings.NoTimeOfDay && Editor.Running == EditorNotRunning) {
		this->TimeOfDay.push_back(SyncRand(MaxTimesOfDay - 1) + 1); // begin at a random time of day
	} else {
		this->TimeOfDay.push_back(NoTimeOfDay); // make indoors have no time of day setting until it is possible to make light sources change their surrounding "time of day" // indoors it is always dark (maybe would be better to allow a special setting to have bright indoor places?
	}
	this->Planes.push_back(NULL);
	this->Worlds.push_back(NULL);
	this->Layers.push_back(0);
	this->LayerConnectors.resize(1);
	this->CulturalSettlementNames.resize(1);
	this->FactionCulturalSettlementNames.resize(1);
	//Wyrmgus end
}

/**
**  Initialize the fog of war.
**  Build tables, setup functions.
*/
void CMap::Init()
{
	InitFogOfWar();
}

/**
**  Cleanup the map module.
*/
void CMap::Clean()
{
	//Wyrmgus start
	CurrentMapLayer = 0;
	this->Landmasses = 0;
	//Wyrmgus end

	//Wyrmgus start
//	delete[] this->Fields;
	for (size_t z = 0; z < this->Fields.size(); ++z) {
		delete[] this->Fields[z];
	}
	this->Fields.clear();
	this->TimeOfDaySeconds.clear();
	this->TimeOfDay.clear();
	this->BorderLandmasses.clear();
	this->Planes.clear();
	this->Worlds.clear();
	this->Layers.clear();
	this->LayerConnectors.clear();
	this->CulturalSettlementNames.clear();
	this->FactionCulturalSettlementNames.clear();
	this->SettlementUnits.clear();
	//Wyrmgus end

	// Tileset freed by Tileset?

	this->Info.Clear();
	//Wyrmgus start
//	this->Fields = NULL;
	//Wyrmgus end
	this->NoFogOfWar = false;
	this->Tileset->clear();
	this->TileModelsFileName.clear();
	CGraphic::Free(this->TileGraphic);
	this->TileGraphic = NULL;

	FlagRevealMap = 0;
	ReplayRevealMap = 0;

	UI.Minimap.Destroy();
	
	//Wyrmgus start
	SubtemplateAreas.clear();
	//Wyrmgus end
}

/**
** Save the complete map.
**
** @param file Output file.
*/
void CMap::Save(CFile &file) const
{
	file.printf("\n--- -----------------------------------------\n");
	file.printf("--- MODULE: map\n");
	file.printf("LoadTileModels(\"%s\")\n\n", this->TileModelsFileName.c_str());
	file.printf("StratagusMap(\n");
	file.printf("  \"version\", \"%s\",\n", VERSION);
	file.printf("  \"description\", \"%s\",\n", this->Info.Description.c_str());
	file.printf("  \"the-map\", {\n");
	file.printf("  \"size\", {%d, %d},\n", this->Info.MapWidth, this->Info.MapHeight);
	file.printf("  \"%s\",\n", this->NoFogOfWar ? "no-fog-of-war" : "fog-of-war");
	file.printf("  \"filename\", \"%s\",\n", this->Info.Filename.c_str());
	//Wyrmgus start
	file.printf("  \"extra-map-layers\", {\n");
	for (size_t z = 1; z < this->Fields.size(); ++z) {
		file.printf("  {%d, %d},\n", this->Info.MapWidths[z], this->Info.MapHeights[z]);
	}
	file.printf("  },\n");
	file.printf("  \"time-of-day\", {\n");
	for (size_t z = 0; z < this->TimeOfDaySeconds.size(); ++z) {
		file.printf("  {%d, %d},\n", this->TimeOfDaySeconds[z], this->TimeOfDay[z]);
	}
	file.printf("  },\n");
	file.printf("  \"layer-references\", {\n");
	for (size_t z = 0; z < this->Fields.size(); ++z) {
		file.printf("  {\"%s\", \"%s\", %d},\n", this->Planes[z] ? this->Planes[z]->Name.c_str() : "", this->Worlds[z] ? this->Worlds[z]->Name.c_str() : "", this->Layers[z]);
	}
	file.printf("  },\n");
	
	file.printf("  \"cultural-settlement-names\", {\n");
	for (size_t z = 0; z < this->CulturalSettlementNames.size(); ++z) {
		file.printf("  {\n");
		for (std::map<std::tuple<int, int, int>, std::string>::const_iterator iterator = this->CulturalSettlementNames[z].begin(); iterator != this->CulturalSettlementNames[z].end(); ++iterator) {
			file.printf("    \"%s\", %d, %d, \"%s\",\n", iterator->second.c_str(), std::get<0>(iterator->first), std::get<1>(iterator->first), PlayerRaces.Name[std::get<2>(iterator->first)].c_str());
		}
		file.printf("  },\n");
	}
	file.printf("  },\n");
	
	file.printf("  \"faction-cultural-settlement-names\", {\n");
	for (size_t z = 0; z < this->FactionCulturalSettlementNames.size(); ++z) {
		file.printf("  {\n");
		for (std::map<std::tuple<int, int, CFaction *>, std::string>::const_iterator iterator = this->FactionCulturalSettlementNames[z].begin(); iterator != this->FactionCulturalSettlementNames[z].end(); ++iterator) {
			file.printf("    \"%s\", %d, %d, \"%s\",\n", iterator->second.c_str(), std::get<0>(iterator->first), std::get<1>(iterator->first), std::get<2>(iterator->first)->Ident.c_str());
		}
		file.printf("  },\n");
	}
	file.printf("  },\n");
	//Wyrmgus end
	file.printf("  \"map-fields\", {\n");
	//Wyrmgus start
	/*
	for (int h = 0; h < this->Info.MapHeight; ++h) {
		file.printf("  -- %d\n", h);
		for (int w = 0; w < this->Info.MapWidth; ++w) {
			const CMapField &mf = *this->Field(w, h);

			mf.Save(file);
			if (w & 1) {
				file.printf(",\n");
			} else {
				file.printf(", ");
			}
		}
	}
	*/
	for (size_t z = 0; z < this->Fields.size(); ++z) {
		file.printf("  {\n");
		for (int h = 0; h < this->Info.MapHeights[z]; ++h) {
			file.printf("  -- %d\n", h);
			for (int w = 0; w < this->Info.MapWidths[z]; ++w) {
				const CMapField &mf = *this->Field(w, h, z);

				mf.Save(file);
				if (w & 1) {
					file.printf(",\n");
				} else {
					file.printf(", ");
				}
			}
		}
		file.printf("  },\n");
	}
	//Wyrmgus end
	file.printf("}})\n");
}

/*----------------------------------------------------------------------------
-- Map Tile Update Functions
----------------------------------------------------------------------------*/

/**
**  Correct the seen wood field, depending on the surrounding.
**
**  @param type  type of tile to update
**  @param seen  1 if updating seen value, 0 for real
**  @param pos   Map tile-position.
*/
//Wyrmgus start
/*
void CMap::FixTile(unsigned short type, int seen, const Vec2i &pos)
{
	Assert(type == MapFieldForest || type == MapFieldRocks);

	//  Outside of map or no wood.
	if (!Info.IsPointOnMap(pos)) {
		return;
	}
	unsigned int index = getIndex(pos);
	CMapField &mf = *this->Field(index);

	if (!((type == MapFieldForest && Tileset->isAWoodTile(mf.playerInfo.SeenTile))
		  || (type == MapFieldRocks && Tileset->isARockTile(mf.playerInfo.SeenTile)))) {
		if (seen) {
			return;
		}
	}

	if (!seen && !(mf.getFlag() & type)) {
		return;
	}

	// Select Table to lookup
	int removedtile;
	int flags;
	if (type == MapFieldForest) {
		removedtile = this->Tileset->getRemovedTreeTile();
		flags = (MapFieldForest | MapFieldUnpassable);
	} else { // (type == MapFieldRocks)
		removedtile = this->Tileset->getRemovedRockTile();
		flags = (MapFieldRocks | MapFieldUnpassable);
	}
	//  Find out what each tile has with respect to wood, or grass.
	int ttup;
	int ttdown;
	int ttleft;
	int ttright;

	if (pos.y - 1 < 0) {
		ttup = -1; //Assign trees in all directions
	} else {
		const CMapField &new_mf = *(&mf - this->Info.MapWidth);
		ttup = seen ? new_mf.playerInfo.SeenTile : new_mf.getGraphicTile();
	}
	if (pos.x + 1 >= this->Info.MapWidth) {
		ttright = -1; //Assign trees in all directions
	} else {
		const CMapField &new_mf = *(&mf + 1);
		ttright = seen ? new_mf.playerInfo.SeenTile : new_mf.getGraphicTile();
	}
	if (pos.y + 1 >= this->Info.MapHeight) {
		ttdown = -1; //Assign trees in all directions
	} else {
		const CMapField &new_mf = *(&mf + this->Info.MapWidth);
		ttdown = seen ? new_mf.playerInfo.SeenTile : new_mf.getGraphicTile();
	}
	if (pos.x - 1 < 0) {
		ttleft = -1; //Assign trees in all directions
	} else {
		const CMapField &new_mf = *(&mf - 1);
		ttleft = seen ? new_mf.playerInfo.SeenTile : new_mf.getGraphicTile();
	}
	int tile = this->Tileset->getTileBySurrounding(type, ttup, ttright, ttdown, ttleft);

	//Update seen tile.
	if (tile == -1) { // No valid wood remove it.
		if (seen) {
			mf.playerInfo.SeenTile = removedtile;
			this->FixNeighbors(type, seen, pos);
		} else {
			mf.setGraphicTile(removedtile);
			mf.Flags &= ~flags;
			mf.Value = 0;
			UI.Minimap.UpdateXY(pos);
		}
	} else if (seen && this->Tileset->isEquivalentTile(tile, mf.playerInfo.SeenTile)) { //Same Type
		return;
	} else {
		if (seen) {
			mf.playerInfo.SeenTile = tile;
		} else {
			mf.setGraphicTile(tile);
		}
	}

	//maybe isExplored
	if (mf.playerInfo.IsExplored(*ThisPlayer)) {
		UI.Minimap.UpdateSeenXY(pos);
		if (!seen) {
			MarkSeenTile(mf);
		}
	}
}
*/
//Wyrmgus end

/**
**  Correct the surrounding fields.
**
**  @param type  Tiletype of tile to adjust
**  @param seen  1 if updating seen value, 0 for real
**  @param pos   Map tile-position.
*/
//Wyrmgus start
/*
void CMap::FixNeighbors(unsigned short type, int seen, const Vec2i &pos)
{
	const Vec2i offset[] = {Vec2i(1, 0), Vec2i(-1, 0), Vec2i(0, 1), Vec2i(0, -1),
							Vec2i(-1, -1), Vec2i(-1, 1), Vec2i(1, -1), Vec2i(1, 1)
						   };

	for (unsigned int i = 0; i < sizeof(offset) / sizeof(*offset); ++i) {
		FixTile(type, seen, pos + offset[i]);
	}
}
*/
//Wyrmgus end

//Wyrmgus start
void CMap::SetTileTerrain(const Vec2i &pos, CTerrainType *terrain, int z)
{
	if (!terrain) {
		return;
	}
	
	CMapField &mf = *this->Field(pos, z);
	
	CTerrainType *old_terrain = this->GetTileTerrain(pos, terrain->Overlay, z);
	
	if (terrain->Overlay) {
		if (mf.OverlayTerrain == terrain) {
			return;
		}
	} else {
		if (mf.Terrain == terrain) {
			return;
		}
	}
	
	mf.SetTerrain(terrain);
	
	this->CalculateTileTransitions(pos, false, z); //recalculate both, since one may have changed the other
	this->CalculateTileTransitions(pos, true, z);
	
	if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
		MarkSeenTile(mf, z);
	}
	UI.Minimap.UpdateXY(pos, z);
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
						
					this->CalculateTileTransitions(adjacent_pos, false, z);
					this->CalculateTileTransitions(adjacent_pos, true, z);
					
					if (adjacent_mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
						MarkSeenTile(adjacent_mf, z);
					}
					UI.Minimap.UpdateXY(adjacent_pos, z);
				}
			}
		}
	}
	
	if (terrain->Overlay) {
		if ((terrain->Flags & MapFieldUnpassable) || (old_terrain && (old_terrain->Flags & MapFieldUnpassable))) {
			Map.CalculateTileOwnership(pos, z);
			
			for (int x_offset = -16; x_offset <= 16; ++x_offset) {
				for (int y_offset = -16; y_offset <= 16; ++y_offset) {
					if (x_offset != 0 || y_offset != 0) {
						Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
						if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
							Map.CalculateTileOwnership(adjacent_pos, z);
						}
					}
				}
			}
		}
	}
}

//Wyrmgus start
//void CMap::RemoveTileOverlayTerrain(const Vec2i &pos)
void CMap::RemoveTileOverlayTerrain(const Vec2i &pos, int z)
//Wyrmgus end
{
	CMapField &mf = *this->Field(pos, z);
	
	if (!mf.OverlayTerrain) {
		return;
	}
	
	CTerrainType *old_terrain = mf.OverlayTerrain;
	
	mf.RemoveOverlayTerrain();
	
	this->CalculateTileTransitions(pos, true, z);
	
	if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
		MarkSeenTile(mf, z);
	}
	UI.Minimap.UpdateXY(pos, z);
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
						
					this->CalculateTileTransitions(adjacent_pos, true, z);
					
					if (adjacent_mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
						MarkSeenTile(adjacent_mf, z);
					}
					UI.Minimap.UpdateXY(adjacent_pos, z);
				}
			}
		}
	}
	
	if (old_terrain->Flags & MapFieldUnpassable) {
		Map.CalculateTileOwnership(pos, z);
			
		for (int x_offset = -16; x_offset <= 16; ++x_offset) {
			for (int y_offset = -16; y_offset <= 16; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
					if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
						Map.CalculateTileOwnership(adjacent_pos, z);
					}
				}
			}
		}
	}
}

void CMap::SetOverlayTerrainDestroyed(const Vec2i &pos, bool destroyed, int z)
{
	CMapField &mf = *this->Field(pos, z);
	
	if (!mf.OverlayTerrain || mf.OverlayTerrainDestroyed == destroyed) {
		return;
	}
	
	mf.SetOverlayTerrainDestroyed(destroyed);
	
	if (destroyed) {
		if (mf.OverlayTerrain->Flags & MapFieldForest) {
			mf.Flags &= ~(MapFieldForest | MapFieldUnpassable);
			mf.Flags |= MapFieldStumps;
		} else if (mf.OverlayTerrain->Flags & MapFieldRocks) {
			mf.Flags &= ~(MapFieldRocks | MapFieldUnpassable);
			mf.Flags |= MapFieldGravel;
		} else if (mf.OverlayTerrain->Flags & MapFieldWall) {
			mf.Flags &= ~(MapFieldHuman | MapFieldWall | MapFieldUnpassable);
			if (GameSettings.Inside) {
				mf.Flags &= ~(MapFieldAirUnpassable);
			}
			mf.Flags |= MapFieldGravel;
		}
		mf.Value = 0;
	} else {
		if (mf.Flags & MapFieldStumps) { //if is a cleared tree tile regrowing trees
			mf.Flags &= ~(MapFieldStumps);
			mf.Flags |= MapFieldForest | MapFieldUnpassable;
			mf.Value = DefaultResourceAmounts[WoodCost];
		}
	}
	
	this->CalculateTileTransitions(pos, true, z);
	
	if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
		MarkSeenTile(mf, z);
	}
	UI.Minimap.UpdateXY(pos, z);
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
						
					this->CalculateTileTransitions(adjacent_pos, true, z);
					
					if (adjacent_mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
						MarkSeenTile(adjacent_mf, z);
					}
					UI.Minimap.UpdateXY(adjacent_pos, z);
				}
			}
		}
	}
	
	if (mf.OverlayTerrain->Flags & MapFieldUnpassable) {
		Map.CalculateTileOwnership(pos, z);
			
		for (int x_offset = -16; x_offset <= 16; ++x_offset) {
			for (int y_offset = -16; y_offset <= 16; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
					if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
						Map.CalculateTileOwnership(adjacent_pos, z);
					}
				}
			}
		}
	}
}

void CMap::SetOverlayTerrainDamaged(const Vec2i &pos, bool damaged, int z)
{
	CMapField &mf = *this->Field(pos, z);
	
	if (!mf.OverlayTerrain || mf.OverlayTerrainDamaged == damaged) {
		return;
	}
	
	mf.SetOverlayTerrainDamaged(damaged);
	
	this->CalculateTileTransitions(pos, true, z);
	
	if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
		MarkSeenTile(mf, z);
	}
	UI.Minimap.UpdateXY(pos, z);
}

static int GetTransitionType(std::vector<int> &adjacent_directions, bool allow_single = false)
{
	if (adjacent_directions.size() == 0) {
		return -1;
	}
	
	int transition_type = -1;

	if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end()) {
		transition_type = SingleTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end()) {
		transition_type = NorthSingleTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end()) {
		transition_type = SouthSingleTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = WestSingleTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end()) {
		transition_type = EastSingleTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthSouthTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end()) {
		transition_type = WestEastTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end()) {
		transition_type = NorthSouthwestInnerSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end()) {
		transition_type = NorthSouthwestInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end()) {
		transition_type = NorthSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end()) {
		transition_type = SouthNorthwestInnerNortheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end()) {
		transition_type = SouthNorthwestInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end()) {
		transition_type = SouthNortheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end()) {
		transition_type = WestNortheastInnerSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end()) {
		transition_type = WestNortheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end()) {
		transition_type = WestSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end()) {
		transition_type = EastNorthwestInnerSouthwestInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end()) {
		transition_type = EastNorthwestInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end()) {
		transition_type = EastSouthwestInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end()) {
		transition_type = NorthwestOuterSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end()) {
		transition_type = NortheastOuterSouthwestInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end()) {
		transition_type = SouthwestOuterNortheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end()) {
		transition_type = SoutheastOuterNorthwestInnerTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = SouthTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end()) {
		transition_type = WestTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end()) {
		transition_type = EastTransitionType;
	} else if ((std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() || std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end()) && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end()) {
		transition_type = NorthwestOuterTransitionType;
	} else if ((std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() || std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end()) && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end()) {
		transition_type = NortheastOuterTransitionType;
	} else if ((std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() || std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end()) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end()) {
		transition_type = SouthwestOuterTransitionType;
	} else if ((std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() || std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end()) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end()) {
		transition_type = SoutheastOuterTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthwestNortheastSouthwestSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthwestNortheastSouthwestInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthwestNortheastSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthwestSouthwestSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NortheastSouthwestSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthwestNortheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = SouthwestSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthwestSouthwestInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NortheastSoutheastInnerTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthwestSoutheastInnerTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NortheastSouthwestInnerTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthwestInnerTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NortheastInnerTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = SouthwestInnerTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = SoutheastInnerTransitionType;
	}

	return transition_type;
}

void CMap::CalculateTileTransitions(const Vec2i &pos, bool overlay, int z)
{
	CMapField &mf = *this->Field(pos, z);
	CTerrainType *terrain = NULL;
	if (overlay) {
		terrain = mf.OverlayTerrain;
		mf.OverlayTransitionTiles.clear();
	} else {
		terrain = mf.Terrain;
		mf.TransitionTiles.clear();
	}
	
	if (!terrain || (overlay && mf.OverlayTerrainDestroyed)) {
		return;
	}
	
	int terrain_id = terrain->ID;
	
	std::map<int, std::vector<int>> adjacent_terrain_directions;
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CTerrainType *adjacent_terrain = this->GetTileTerrain(adjacent_pos, overlay, z);
					if (overlay && adjacent_terrain && this->Field(adjacent_pos, z)->OverlayTerrainDestroyed) {
						adjacent_terrain = NULL;
					}
					if (adjacent_terrain && terrain != adjacent_terrain) {
						if (std::find(terrain->InnerBorderTerrains.begin(), terrain->InnerBorderTerrains.end(), adjacent_terrain) != terrain->InnerBorderTerrains.end()) {
							adjacent_terrain_directions[adjacent_terrain->ID].push_back(GetDirectionFromOffset(x_offset, y_offset));
						} else if (std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), adjacent_terrain) == terrain->BorderTerrains.end()) { //if the two terrain types can't border, look for a third terrain type which can border both, and which treats both as outer border terrains, and then use for transitions between both tiles
							for (size_t i = 0; i < terrain->BorderTerrains.size(); ++i) {
								CTerrainType *border_terrain = terrain->BorderTerrains[i];
								if (std::find(terrain->InnerBorderTerrains.begin(), terrain->InnerBorderTerrains.end(), border_terrain) != terrain->InnerBorderTerrains.end() && std::find(adjacent_terrain->InnerBorderTerrains.begin(), adjacent_terrain->InnerBorderTerrains.end(), border_terrain) != adjacent_terrain->InnerBorderTerrains.end()) {
									adjacent_terrain_directions[border_terrain->ID].push_back(GetDirectionFromOffset(x_offset, y_offset));
									break;
								}
							}
						}
					}
					if (!adjacent_terrain || (overlay && terrain != adjacent_terrain && std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), adjacent_terrain) == terrain->BorderTerrains.end())) { // happens if terrain is NULL or if it is an overlay tile which doesn't have a border with this one, so that i.e. tree transitions display correctly when adjacent to tiles without overlays
						adjacent_terrain_directions[TerrainTypes.size()].push_back(GetDirectionFromOffset(x_offset, y_offset));
					}
				}
			}
		}
	}
	
	for (std::map<int, std::vector<int>>::iterator iterator = adjacent_terrain_directions.begin(); iterator != adjacent_terrain_directions.end(); ++iterator) {
		int adjacent_terrain_id = iterator->first;
		CTerrainType *adjacent_terrain = adjacent_terrain_id < (int) TerrainTypes.size() ? TerrainTypes[adjacent_terrain_id] : NULL;
		int transition_type = GetTransitionType(iterator->second, terrain->AllowSingle);
		
		if (transition_type != -1) {
			bool found_transition = false;
			
			if (!overlay) {
				if (adjacent_terrain) {
					if (terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)].size() > 0) {
						mf.TransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)][SyncRand(terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)].size())]));
						found_transition = true;
					} else if (adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)].size() > 0) {
						mf.TransitionTiles.push_back(std::pair<CTerrainType *, int>(adjacent_terrain, adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)][SyncRand(adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)].size())]));
						found_transition = true;
					} else if (adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)].size() > 0) {
						mf.TransitionTiles.push_back(std::pair<CTerrainType *, int>(adjacent_terrain, adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)][SyncRand(adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)].size())]));
						found_transition = true;
					}
				} else {
					if (terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)].size() > 0) {
						mf.TransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)][SyncRand(terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)].size())]));
					}
				}
			} else {
				if (adjacent_terrain) {
					if (adjacent_terrain && terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)].size() > 0) {
						mf.OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)][SyncRand(terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)].size())]));
						found_transition = true;
					} else if (adjacent_terrain && adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)].size() > 0) {
						mf.OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(adjacent_terrain, adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)][SyncRand(adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)].size())]));
						found_transition = true;
					} else if (adjacent_terrain && adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)].size() > 0) {
						mf.OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(adjacent_terrain, adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)][SyncRand(adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)].size())]));
						found_transition = true;
					}
				} else {
					if (terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)].size() > 0) {
						mf.OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)][SyncRand(terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)].size())]));
					}
				}
				
				if ((mf.Flags & MapFieldWaterAllowed) && (!adjacent_terrain || !(adjacent_terrain->Flags & MapFieldWaterAllowed))) { //if this is a water tile adjacent to a non-water tile, replace the water flag with a coast one
					mf.Flags &= ~(MapFieldWaterAllowed);
					mf.Flags |= MapFieldCoastAllowed;
				}
			}
			
			if (adjacent_terrain && found_transition) {
				for (size_t i = 0; i != iterator->second.size(); ++i) {
					adjacent_terrain_directions[TerrainTypes.size()].erase(std::remove(adjacent_terrain_directions[TerrainTypes.size()].begin(), adjacent_terrain_directions[TerrainTypes.size()].end(), iterator->second[i]), adjacent_terrain_directions[TerrainTypes.size()].end());
				}
			}
		}
	}
	
	//sort the transitions so that they will be displayed in the correct order
	if (overlay) {
		bool swapped = true;
		for (int passes = 0; passes < (int) mf.OverlayTransitionTiles.size() && swapped; ++passes) {
			swapped = false;
			for (int i = 0; i < ((int) mf.OverlayTransitionTiles.size()) - 1; ++i) {
				bool change_order = false;
				if (std::find(mf.OverlayTransitionTiles[i + 1].first->InnerBorderTerrains.begin(), mf.OverlayTransitionTiles[i + 1].first->InnerBorderTerrains.end(), mf.OverlayTransitionTiles[i].first) != mf.OverlayTransitionTiles[i + 1].first->InnerBorderTerrains.end()) {
					std::pair<CTerrainType *, int> temp_transition = mf.OverlayTransitionTiles[i];
					mf.OverlayTransitionTiles[i] = mf.OverlayTransitionTiles[i + 1];
					mf.OverlayTransitionTiles[i + 1] = temp_transition;
					swapped = true;
				}
			}
		}
	} else {
		bool swapped = true;
		for (int passes = 0; passes < (int) mf.TransitionTiles.size() && swapped; ++passes) {
			swapped = false;
			for (int i = 0; i < ((int) mf.TransitionTiles.size()) - 1; ++i) {
				bool change_order = false;
				if (std::find(mf.TransitionTiles[i + 1].first->InnerBorderTerrains.begin(), mf.TransitionTiles[i + 1].first->InnerBorderTerrains.end(), mf.TransitionTiles[i].first) != mf.TransitionTiles[i + 1].first->InnerBorderTerrains.end()) {
					std::pair<CTerrainType *, int> temp_transition = mf.TransitionTiles[i];
					mf.TransitionTiles[i] = mf.TransitionTiles[i + 1];
					mf.TransitionTiles[i + 1] = temp_transition;
					swapped = true;
				}
			}
		}
	}
}

void CMap::CalculateTileLandmass(const Vec2i &pos, int z)
{
	if (!this->Info.IsPointOnMap(pos, z)) {
		return;
	}
	
	if (Editor.Running != EditorNotRunning) { //no need to assign landmasses while in the editor
		return;
	}
	
	CMapField &mf = *this->Field(pos, z);

	if (mf.Landmass != 0) {
		return; //already calculated
	}
	
	bool is_water = (mf.Flags & MapFieldWaterAllowed) || (mf.Flags & MapFieldCoastAllowed);

	//doesn't have a landmass ID, and hasn't inherited one from another tile yet, so add a new one
	mf.Landmass = this->Landmasses + 1;
	this->Landmasses += 1;
	this->BorderLandmasses.resize(this->Landmasses + 1);
	//now, spread the new landmass ID to neighboring land tiles
	std::vector<Vec2i> landmass_tiles;
	landmass_tiles.push_back(pos);
	//calculate the landmass of any neighboring land tiles with no set landmass as well
	for (size_t i = 0; i < landmass_tiles.size(); ++i) {
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(landmass_tiles[i].x + x_offset, landmass_tiles[i].y + y_offset);
					if (this->Info.IsPointOnMap(adjacent_pos, z)) {
						CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
						bool adjacent_is_water = (adjacent_mf.Flags & MapFieldWaterAllowed) || (adjacent_mf.Flags & MapFieldCoastAllowed);
									
						if (adjacent_is_water == is_water && adjacent_mf.Landmass == 0) {
							adjacent_mf.Landmass = mf.Landmass;
							landmass_tiles.push_back(adjacent_pos);
						} else if (adjacent_is_water != is_water && adjacent_mf.Landmass != 0 && std::find(this->BorderLandmasses[mf.Landmass].begin(), this->BorderLandmasses[mf.Landmass].end(), adjacent_mf.Landmass) == this->BorderLandmasses[mf.Landmass].end()) {
							this->BorderLandmasses[mf.Landmass].push_back(adjacent_mf.Landmass);
							this->BorderLandmasses[adjacent_mf.Landmass].push_back(mf.Landmass);
						}
					}
				}
			}
		}
	}
}

void CMap::CalculateTileOwnership(const Vec2i &pos, int z)
{
	if (!this->Info.IsPointOnMap(pos, z)) {
		return;
	}

	CMapField &mf = *this->Field(pos, z);
	
	int new_owner = -1;
	bool must_have_no_owner = false;
	
	if (mf.Flags & MapFieldBuilding) { //make sure the place a building is located is set to be owned by its player; this is necessary for scenarios, since when they start buildings could be on another player's territory (i.e. if a farm starts next to a town hall)
		const CUnitCache &cache = mf.UnitCache;
		for (size_t i = 0; i != cache.size(); ++i) {
			CUnit *unit = cache[i];
			if (!unit) {
				fprintf(stderr, "Error in CMap::CalculateTileOwnership (pos %d, %d): a unit in the tile's unit cache is NULL.\n", pos.x, pos.y);
			}
			if (unit->IsAliveOnMap() && unit->Type->BoolFlag[BUILDING_INDEX].value) {
				if (unit->Variable[OWNERSHIPINFLUENCERANGE_INDEX].Value && unit->Player->Index != PlayerNumNeutral) {
					new_owner = unit->Player->Index;
					break;
				} else if (unit->Player->Index == PlayerNumNeutral && (unit->Type == SettlementSiteUnitType || (unit->Type->GivesResource && !unit->Type->BoolFlag[CANHARVEST_INDEX].value))) { //there cannot be an owner for the tile of a (neutral) settlement site or deposit, otherwise players might not be able to build over them
					must_have_no_owner = true;
					break;
				}
			}
		}
	}

	if (new_owner == -1 && !must_have_no_owner) { //if no building is on the tile, set it to the first unit to have influence on it, if that isn't blocked by an obstacle
		std::vector<unsigned long> obstacle_flags;
		obstacle_flags.push_back(MapFieldCoastAllowed);
		obstacle_flags.push_back(MapFieldUnpassable);

		std::vector<CUnit *> table;
		Select(pos - Vec2i(16, 16), pos + Vec2i(16, 16), table, z);
		for (size_t i = 0; i != table.size(); ++i) {
			CUnit *unit = table[i];
			if (!unit) {
				fprintf(stderr, "Error in CMap::CalculateTileOwnership (pos %d, %d): a unit within the tile's range is NULL.\n", pos.x, pos.y);
			}
			if (unit->IsAliveOnMap() && unit->Variable[OWNERSHIPINFLUENCERANGE_INDEX].Value > 0 && unit->MapDistanceTo(pos, z) <= unit->Variable[OWNERSHIPINFLUENCERANGE_INDEX].Value) {
				bool obstacle_check = true;
				for (size_t j = 0; j < obstacle_flags.size(); ++j) {
					bool obstacle_subcheck = false;
					for (int x = 0; x < unit->Type->TileWidth; ++x) {
						for (int y = 0; y < unit->Type->TileHeight; ++y) {
							if (CheckObstaclesBetweenTiles(unit->tilePos + Vec2i(x, y), pos, obstacle_flags[j], z, 0, NULL, unit->Player->Index)) { //the obstacle must be avoidable from at least one of the unit's tiles
								obstacle_subcheck = true;
								break;
							}
						}
						if (obstacle_subcheck) {
							break;
						}
					}
					if (!obstacle_subcheck) {
						obstacle_check = false;
						break;
					}
				}
				if (!obstacle_check) {
					continue;
				}
				new_owner = unit->Player->Index;
				break;
			}
		}
	}
	
	if (new_owner != mf.Owner) {
		mf.Owner = new_owner;
		
		this->CalculateTileOwnershipTransition(pos, z);
		
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
					if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
						CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
							
						this->CalculateTileOwnershipTransition(adjacent_pos, z);
					}
				}
			}
		}
	}
}

void CMap::CalculateTileOwnershipTransition(const Vec2i &pos, int z)
{
	if (!this->Info.IsPointOnMap(pos, z)) {
		return;
	}
	
	if (Editor.Running != EditorNotRunning) { //no need to assign ownership transitions while in the editor
		return;
	}
	
	CMapField &mf = *this->Field(pos, z);
	
	mf.OwnershipBorderTile = -1;

	if (mf.Owner == -1) {
		return;
	}
	
	std::vector<int> adjacent_directions;
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
					if (adjacent_mf.Owner != mf.Owner) {
						adjacent_directions.push_back(GetDirectionFromOffset(x_offset, y_offset));
					}
				}
			}
		}
	}
	
	int transition_type = GetTransitionType(adjacent_directions, true);
	
	if (transition_type != -1) {
		if (Map.BorderTerrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)].size() > 0) {
			mf.OwnershipBorderTile = Map.BorderTerrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)][SyncRand(Map.BorderTerrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)].size())];
		}
	}
}

void CMap::AdjustMap()
{
	for (size_t z = 0; z < this->Fields.size(); ++z) {
		Vec2i map_start_pos(0, 0);
		Vec2i map_end(this->Info.MapWidths[z], this->Info.MapHeights[z]);
		
		this->AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		this->AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
		this->AdjustTileMapTransitions(map_start_pos, map_end, z);
		this->AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		this->AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
	}
}

void CMap::AdjustTileMapIrregularities(bool overlay, const Vec2i &min_pos, const Vec2i &max_pos, int z)
{
	bool no_irregularities_found = false;
	while (!no_irregularities_found) {
		no_irregularities_found = true;
		for (int x = min_pos.x; x < max_pos.x; ++x) {
			for (int y = min_pos.y; y < max_pos.y; ++y) {
				CMapField &mf = *this->Field(x, y, z);
				CTerrainType *terrain = overlay ? mf.OverlayTerrain : mf.Terrain;
				if (!terrain || terrain->AllowSingle) {
					continue;
				}
				std::vector<CTerrainType *> acceptable_adjacent_tile_types;
				acceptable_adjacent_tile_types.push_back(terrain);
				for (size_t i = 0; i < terrain->OuterBorderTerrains.size(); ++i) {
					acceptable_adjacent_tile_types.push_back(terrain->OuterBorderTerrains[i]);
				}
				
				int horizontal_adjacent_tiles = 0;
				int vertical_adjacent_tiles = 0;
				int nw_quadrant_adjacent_tiles = 0; //should be 4 if the wrong tile types are present in X-1,Y; X-1,Y-1; X,Y-1; and X+1,Y+1
				int ne_quadrant_adjacent_tiles = 0;
				int sw_quadrant_adjacent_tiles = 0;
				int se_quadrant_adjacent_tiles = 0;
				
				if ((x - 1) >= 0 && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), this->GetTileTerrain(Vec2i(x - 1, y), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					horizontal_adjacent_tiles += 1;
					nw_quadrant_adjacent_tiles += 1;
					sw_quadrant_adjacent_tiles += 1;
				}
				if ((x + 1) < this->Info.MapWidths[z] && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), this->GetTileTerrain(Vec2i(x + 1, y), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					horizontal_adjacent_tiles += 1;
					ne_quadrant_adjacent_tiles += 1;
					se_quadrant_adjacent_tiles += 1;
				}
				
				if ((y - 1) >= 0 && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), this->GetTileTerrain(Vec2i(x, y - 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					vertical_adjacent_tiles += 1;
					nw_quadrant_adjacent_tiles += 1;
					ne_quadrant_adjacent_tiles += 1;
				}
				if ((y + 1) < this->Info.MapHeights[z] && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), this->GetTileTerrain(Vec2i(x, y + 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					vertical_adjacent_tiles += 1;
					sw_quadrant_adjacent_tiles += 1;
					se_quadrant_adjacent_tiles += 1;
				}

				if ((x - 1) >= 0 && (y - 1) >= 0 && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), this->GetTileTerrain(Vec2i(x - 1, y - 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					nw_quadrant_adjacent_tiles += 1;
					se_quadrant_adjacent_tiles += 1;
				}
				if ((x - 1) >= 0 && (y + 1) < this->Info.MapHeights[z] && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), GetTileTerrain(Vec2i(x - 1, y + 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					sw_quadrant_adjacent_tiles += 1;
					ne_quadrant_adjacent_tiles += 1;
				}
				if ((x + 1) < this->Info.MapWidths[z] && (y - 1) >= 0 && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), GetTileTerrain(Vec2i(x + 1, y - 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					ne_quadrant_adjacent_tiles += 1;
					sw_quadrant_adjacent_tiles += 1;
				}
				if ((x + 1) < this->Info.MapWidths[z] && (y + 1) < this->Info.MapHeights[z] && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), GetTileTerrain(Vec2i(x + 1, y + 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					se_quadrant_adjacent_tiles += 1;
					nw_quadrant_adjacent_tiles += 1;
				}
				
				
				if (horizontal_adjacent_tiles >= 2 || vertical_adjacent_tiles >= 2 || nw_quadrant_adjacent_tiles >= 4 || ne_quadrant_adjacent_tiles >= 4 || sw_quadrant_adjacent_tiles >= 4 || se_quadrant_adjacent_tiles >= 4) {
					if (overlay) {
						mf.RemoveOverlayTerrain();
					} else {
						bool changed_terrain = false;
						for (int sub_x = -1; sub_x <= 1; ++sub_x) {
							for (int sub_y = -1; sub_y <= 1; ++sub_y) {
								if ((x + sub_x) < min_pos.x || (x + sub_x) >= max_pos.x || (y + sub_y) < min_pos.y || (y + sub_y) >= max_pos.y || (sub_x == 0 && sub_y == 0)) {
									continue;
								}
								CTerrainType *tile_terrain = GetTileTerrain(Vec2i(x + sub_x, y + sub_y), false, z);
								if (mf.Terrain != tile_terrain) {
									if (std::find(mf.Terrain->InnerBorderTerrains.begin(), mf.Terrain->InnerBorderTerrains.end(), tile_terrain) != mf.Terrain->InnerBorderTerrains.end()) {
										mf.SetTerrain(tile_terrain);
										changed_terrain = true;
										break;
									}
								}
							}
							if (changed_terrain) {
								break;
							}
						}
						if (!changed_terrain && terrain->InnerBorderTerrains.size() > 0) {
							mf.SetTerrain(terrain->InnerBorderTerrains[0]);
						}
					}
					no_irregularities_found = false;
				}
			}
		}
	}
}

void CMap::AdjustTileMapTransitions(const Vec2i &min_pos, const Vec2i &max_pos, int z)
{
	for (int x = min_pos.x; x < max_pos.x; ++x) {
		for (int y = min_pos.y; y < max_pos.y; ++y) {
			CMapField &mf = *this->Field(x, y, z);

			for (int sub_x = -1; sub_x <= 1; ++sub_x) {
				for (int sub_y = -1; sub_y <= 1; ++sub_y) {
					if ((x + sub_x) < min_pos.x || (x + sub_x) >= max_pos.x || (y + sub_y) < min_pos.y || (y + sub_y) >= max_pos.y || (sub_x == 0 && sub_y == 0)) {
						continue;
					}
					CTerrainType *tile_terrain = GetTileTerrain(Vec2i(x + sub_x, y + sub_y), false, z);
					CTerrainType *tile_top_terrain = GetTileTopTerrain(Vec2i(x + sub_x, y + sub_y), false, z);
					if (mf.Terrain != tile_terrain && tile_top_terrain->Overlay && tile_top_terrain != mf.OverlayTerrain && std::find(tile_terrain->OuterBorderTerrains.begin(), tile_terrain->OuterBorderTerrains.end(), mf.Terrain) == tile_terrain->OuterBorderTerrains.end() && std::find(tile_top_terrain->BaseTerrains.begin(), tile_top_terrain->BaseTerrains.end(), mf.Terrain) == tile_top_terrain->BaseTerrains.end()) {
						mf.SetTerrain(tile_terrain);
					}
				}
			}
		}
	}

	for (int x = min_pos.x; x < max_pos.x; ++x) {
		for (int y = min_pos.y; y < max_pos.y; ++y) {
			CMapField &mf = *this->Field(x, y, z);

			for (int sub_x = -1; sub_x <= 1; ++sub_x) {
				for (int sub_y = -1; sub_y <= 1; ++sub_y) {
					if ((x + sub_x) < min_pos.x || (x + sub_x) >= max_pos.x || (y + sub_y) < min_pos.y || (y + sub_y) >= max_pos.y || (sub_x == 0 && sub_y == 0)) {
						continue;
					}
					CTerrainType *tile_terrain = GetTileTerrain(Vec2i(x + sub_x, y + sub_y), false, z);
					if (mf.Terrain != tile_terrain && std::find(mf.Terrain->BorderTerrains.begin(), mf.Terrain->BorderTerrains.end(), tile_terrain) == mf.Terrain->BorderTerrains.end()) {
						for (size_t i = 0; i < mf.Terrain->BorderTerrains.size(); ++i) {
							CTerrainType *border_terrain = mf.Terrain->BorderTerrains[i];
							if (std::find(border_terrain->BorderTerrains.begin(), border_terrain->BorderTerrains.end(), mf.Terrain) != border_terrain->BorderTerrains.end() && std::find(border_terrain->BorderTerrains.begin(), border_terrain->BorderTerrains.end(), tile_terrain) != border_terrain->BorderTerrains.end()) {
								mf.SetTerrain(border_terrain);
								break;
							}
						}
					}
				}
			}
		}
	}
}

void CMap::GenerateTerrain(CTerrainType *terrain, int seed_number, int expansion_number, const Vec2i &min_pos, const Vec2i &max_pos, bool preserve_coastline, int z)
{
	if (SaveGameLoading) {
		return;
	}
	
	Vec2i random_pos(0, 0);
	int count = seed_number;
	int while_count = 0;
	
	// create initial seeds
	while (count > 0 && while_count < seed_number * 100) {
		random_pos.x = SyncRand(max_pos.x - min_pos.x + 1) + min_pos.x;
		random_pos.y = SyncRand(max_pos.y - min_pos.y + 1) + min_pos.y;
		
		if (!this->Info.IsPointOnMap(random_pos, z) || this->IsPointInASubtemplateArea(random_pos, z)) {
			continue;
		}
		
		CTerrainType *tile_terrain = GetTileTerrain(random_pos, false, z);
		
		if (
			(
				(
					!terrain->Overlay
					&& std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), tile_terrain) != terrain->BorderTerrains.end() && this->TileBordersOnlySameTerrain(random_pos, terrain, z)
				)
				|| (terrain->Overlay && std::find(terrain->BaseTerrains.begin(), terrain->BaseTerrains.end(), tile_terrain) != terrain->BaseTerrains.end() && this->TileBordersOnlySameTerrain(random_pos, terrain, z))
			)
			&& (!GetTileTopTerrain(random_pos, false, z)->Overlay || GetTileTopTerrain(random_pos, false, z) == terrain)
			&& (!preserve_coastline || (terrain->Flags & MapFieldWaterAllowed) == (tile_terrain->Flags & MapFieldWaterAllowed))
			&& !this->TileHasUnitsIncompatibleWithTerrain(random_pos, terrain, z)
			&& (!(terrain->Flags & MapFieldUnpassable) || !this->TileBordersUnit(random_pos, z)) // if the terrain is unpassable, don't expand to spots adjacent to units
		) {
			std::vector<Vec2i> adjacent_positions;
			for (int sub_x = -1; sub_x <= 1; sub_x += 2) { // +2 so that only diagonals are used
				for (int sub_y = -1; sub_y <= 1; sub_y += 2) {
					Vec2i diagonal_pos(random_pos.x + sub_x, random_pos.y + sub_y);
					Vec2i vertical_pos(random_pos.x, random_pos.y + sub_y);
					Vec2i horizontal_pos(random_pos.x + sub_x, random_pos.y);
					if (!this->Info.IsPointOnMap(diagonal_pos, z)) {
						continue;
					}
					
					CTerrainType *diagonal_tile_terrain = GetTileTerrain(diagonal_pos, false, z);
					CTerrainType *vertical_tile_terrain = GetTileTerrain(vertical_pos, false, z);
					CTerrainType *horizontal_tile_terrain = GetTileTerrain(horizontal_pos, false, z);
					
					if (
						(
							(
								!terrain->Overlay
								&& std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), diagonal_tile_terrain) != terrain->BorderTerrains.end() && this->TileBordersOnlySameTerrain(diagonal_pos, terrain, z)
								&& std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), vertical_tile_terrain) != terrain->BorderTerrains.end() && this->TileBordersOnlySameTerrain(vertical_pos, terrain, z)
								&& std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), horizontal_tile_terrain) != terrain->BorderTerrains.end() && this->TileBordersOnlySameTerrain(horizontal_pos, terrain, z)
							)
							|| (
								terrain->Overlay
								&& std::find(terrain->BaseTerrains.begin(), terrain->BaseTerrains.end(), diagonal_tile_terrain) != terrain->BaseTerrains.end() && this->TileBordersOnlySameTerrain(diagonal_pos, terrain, z)
								&& std::find(terrain->BaseTerrains.begin(), terrain->BaseTerrains.end(), vertical_tile_terrain) != terrain->BaseTerrains.end() && this->TileBordersOnlySameTerrain(vertical_pos, terrain, z)
								&& std::find(terrain->BaseTerrains.begin(), terrain->BaseTerrains.end(), horizontal_tile_terrain) != terrain->BaseTerrains.end() && this->TileBordersOnlySameTerrain(horizontal_pos, terrain, z)
							)
						)
						&& (!GetTileTopTerrain(diagonal_pos, false, z)->Overlay || GetTileTopTerrain(diagonal_pos, false, z) == terrain) && (!GetTileTopTerrain(vertical_pos, false, z)->Overlay || GetTileTopTerrain(vertical_pos, false, z) == terrain) && (!GetTileTopTerrain(horizontal_pos, false, z)->Overlay || GetTileTopTerrain(horizontal_pos, false, z) == terrain)
						&& (!preserve_coastline || ((terrain->Flags & MapFieldWaterAllowed) == (diagonal_tile_terrain->Flags & MapFieldWaterAllowed) && (terrain->Flags & MapFieldWaterAllowed) == (vertical_tile_terrain->Flags & MapFieldWaterAllowed) && (terrain->Flags & MapFieldWaterAllowed) == (horizontal_tile_terrain->Flags & MapFieldWaterAllowed)))
						&& !this->TileHasUnitsIncompatibleWithTerrain(diagonal_pos, terrain, z) && !this->TileHasUnitsIncompatibleWithTerrain(vertical_pos, terrain, z) && !this->TileHasUnitsIncompatibleWithTerrain(horizontal_pos, terrain, z)
						&& (!(terrain->Flags & MapFieldUnpassable) || (!this->TileBordersUnit(diagonal_pos, z) && !this->TileBordersUnit(vertical_pos, z) && !this->TileBordersUnit(horizontal_pos, z))) // if the terrain is unpassable, don't expand to spots adjacent to buildings
						&& !this->IsPointInASubtemplateArea(diagonal_pos, z) && !this->IsPointInASubtemplateArea(vertical_pos, z) && !this->IsPointInASubtemplateArea(horizontal_pos, z)
					) {
						adjacent_positions.push_back(diagonal_pos);
					}
				}
			}
			
			if (adjacent_positions.size() > 0) {
				Vec2i adjacent_pos = adjacent_positions[SyncRand(adjacent_positions.size())];
				this->Field(random_pos, z)->SetTerrain(terrain);
				this->Field(adjacent_pos, z)->SetTerrain(terrain);
				this->Field(Vec2i(random_pos.x, adjacent_pos.y), z)->SetTerrain(terrain);
				this->Field(Vec2i(adjacent_pos.x, random_pos.y), z)->SetTerrain(terrain);
				count -= 1;
			}
		}
		
		while_count += 1;
	}
	
	// expand seeds
	count = expansion_number;
	while_count = 0;
	
	while (count > 0 && while_count < expansion_number * 100) {
		random_pos.x = SyncRand(max_pos.x - min_pos.x + 1) + min_pos.x;
		random_pos.y = SyncRand(max_pos.y - min_pos.y + 1) + min_pos.y;
		
		if (
			this->Info.IsPointOnMap(random_pos, z)
			&& GetTileTerrain(random_pos, terrain->Overlay, z) == terrain
			&& (!terrain->Overlay || this->TileBordersOnlySameTerrain(random_pos, terrain, z))
		) {
			std::vector<Vec2i> adjacent_positions;
			for (int sub_x = -1; sub_x <= 1; sub_x += 2) { // +2 so that only diagonals are used
				for (int sub_y = -1; sub_y <= 1; sub_y += 2) {
					Vec2i diagonal_pos(random_pos.x + sub_x, random_pos.y + sub_y);
					Vec2i vertical_pos(random_pos.x, random_pos.y + sub_y);
					Vec2i horizontal_pos(random_pos.x + sub_x, random_pos.y);
					if (!this->Info.IsPointOnMap(diagonal_pos, z)) {
						continue;
					}
					
					CTerrainType *diagonal_tile_terrain = GetTileTerrain(diagonal_pos, false, z);
					CTerrainType *vertical_tile_terrain = GetTileTerrain(vertical_pos, false, z);
					CTerrainType *horizontal_tile_terrain = GetTileTerrain(horizontal_pos, false, z);
					
					if (
						(
							(
								!terrain->Overlay
								&& (diagonal_tile_terrain == terrain || (std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), diagonal_tile_terrain) != terrain->BorderTerrains.end() && this->TileBordersOnlySameTerrain(diagonal_pos, terrain, z)))
								&& (vertical_tile_terrain == terrain || (std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), vertical_tile_terrain) != terrain->BorderTerrains.end() && this->TileBordersOnlySameTerrain(vertical_pos, terrain, z)))
								&& (horizontal_tile_terrain == terrain || (std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), horizontal_tile_terrain) != terrain->BorderTerrains.end() && this->TileBordersOnlySameTerrain(horizontal_pos, terrain, z)))
								&& (diagonal_tile_terrain != terrain || vertical_tile_terrain != terrain || horizontal_tile_terrain != terrain)
							)
							|| (
								terrain->Overlay
								&& ((std::find(terrain->BaseTerrains.begin(), terrain->BaseTerrains.end(), diagonal_tile_terrain) != terrain->BaseTerrains.end() && this->TileBordersOnlySameTerrain(diagonal_pos, terrain, z)) || GetTileTerrain(diagonal_pos, terrain->Overlay, z) == terrain)
								&& ((std::find(terrain->BaseTerrains.begin(), terrain->BaseTerrains.end(), vertical_tile_terrain) != terrain->BaseTerrains.end() && this->TileBordersOnlySameTerrain(vertical_pos, terrain, z)) || GetTileTerrain(vertical_pos, terrain->Overlay, z) == terrain)
								&& ((std::find(terrain->BaseTerrains.begin(), terrain->BaseTerrains.end(), horizontal_tile_terrain) != terrain->BaseTerrains.end() && this->TileBordersOnlySameTerrain(horizontal_pos, terrain, z)) || GetTileTerrain(horizontal_pos, terrain->Overlay, z) == terrain)
								&& (GetTileTerrain(diagonal_pos, terrain->Overlay, z) != terrain || GetTileTerrain(vertical_pos, terrain->Overlay, z) != terrain || GetTileTerrain(horizontal_pos, terrain->Overlay, z) != terrain)
							)
						)
						&& (!GetTileTopTerrain(diagonal_pos, false, z)->Overlay || GetTileTopTerrain(diagonal_pos, false, z) == terrain) && (!GetTileTopTerrain(vertical_pos, false, z)->Overlay || GetTileTopTerrain(vertical_pos, false, z) == terrain) && (!GetTileTopTerrain(horizontal_pos, false, z)->Overlay || GetTileTopTerrain(horizontal_pos, false, z) == terrain) // don't expand into tiles with overlays
						&& (!preserve_coastline || ((terrain->Flags & MapFieldWaterAllowed) == (diagonal_tile_terrain->Flags & MapFieldWaterAllowed) && (terrain->Flags & MapFieldWaterAllowed) == (vertical_tile_terrain->Flags & MapFieldWaterAllowed) && (terrain->Flags & MapFieldWaterAllowed) == (horizontal_tile_terrain->Flags & MapFieldWaterAllowed)))
						&& !this->TileHasUnitsIncompatibleWithTerrain(diagonal_pos, terrain, z) && !this->TileHasUnitsIncompatibleWithTerrain(vertical_pos, terrain, z) && !this->TileHasUnitsIncompatibleWithTerrain(horizontal_pos, terrain, z)
						&& (!(terrain->Flags & MapFieldUnpassable) || (!this->TileBordersUnit(diagonal_pos, z) && !this->TileBordersUnit(vertical_pos, z) && !this->TileBordersUnit(horizontal_pos, z))) // if the terrain is unpassable, don't expand to spots adjacent to buildings
						&& (!this->IsPointInASubtemplateArea(diagonal_pos, z) || GetTileTerrain(diagonal_pos, terrain->Overlay, z) == terrain) && (!this->IsPointInASubtemplateArea(vertical_pos, z) || GetTileTerrain(vertical_pos, terrain->Overlay, z) == terrain) && (!this->IsPointInASubtemplateArea(horizontal_pos, z) || GetTileTerrain(horizontal_pos, terrain->Overlay, z) == terrain)
					) {
						adjacent_positions.push_back(diagonal_pos);
					}
				}
			}
			
			if (adjacent_positions.size() > 0) {
				Vec2i adjacent_pos = adjacent_positions[SyncRand(adjacent_positions.size())];
				this->Field(adjacent_pos, z)->SetTerrain(terrain);
				this->Field(Vec2i(random_pos.x, adjacent_pos.y), z)->SetTerrain(terrain);
				this->Field(Vec2i(adjacent_pos.x, random_pos.y), z)->SetTerrain(terrain);
				count -= 1;
			}
		}
		
		while_count += 1;
	}
}

void CMap::GenerateNeutralUnits(CUnitType *unit_type, int quantity, const Vec2i &min_pos, const Vec2i &max_pos, bool grouped, int z)
{
	if (SaveGameLoading) {
		return;
	}
	
	Vec2i unit_pos(-1, -1);
	
	for (int i = 0; i < quantity; ++i) {
		if (i == 0 || !grouped) {
			unit_pos = GenerateUnitLocation(unit_type, NULL, min_pos, max_pos, z);
		}
		if (!this->Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		if (unit_type->GivesResource) {
			CUnit *unit = CreateResourceUnit(unit_pos, *unit_type, z);
		} else {
			CUnit *unit = CreateUnit(unit_pos, *unit_type, &Players[PlayerNumNeutral], z);
		}
	}
}
//Wyrmgus end

//Wyrmgus start
void CMap::ClearOverlayTile(const Vec2i &pos, int z)
{
	CMapField &mf = *this->Field(pos, z);

	if (!mf.OverlayTerrain) {
		return;
	}
	
	this->SetOverlayTerrainDestroyed(pos, true, z);

	//remove decorations if a wall, tree or rock was removed from the tile
	std::vector<CUnit *> table;
	Select(pos, pos, table, z);
	for (size_t i = 0; i != table.size(); ++i) {
		if (table[i]->Type->UnitType == UnitTypeLand && table[i]->Type->BoolFlag[DECORATION_INDEX].value) {
			LetUnitDie(*table[i]);			
		}
	}

	//check if any further tile should be removed with the clearing of this one
	if (!mf.OverlayTerrain->AllowSingle) {
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
					if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
						CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
						
						if (adjacent_mf.OverlayTerrain == mf.OverlayTerrain && !adjacent_mf.OverlayTerrainDestroyed && !this->CurrentTerrainCanBeAt(adjacent_pos, true, z)) {
							this->ClearOverlayTile(adjacent_pos, z);
						}
					}
				}
			}
		}
	}
}
//Wyrmgus end

//Wyrmgus start
/*
/// Remove wood from the map.
void CMap::ClearWoodTile(const Vec2i &pos)
{
	CMapField &mf = *this->Field(pos);

	mf.setGraphicTile(this->Tileset->getRemovedTreeTile());
	mf.Flags &= ~(MapFieldForest | MapFieldUnpassable);
	mf.Value = 0;

	UI.Minimap.UpdateXY(pos);
	FixNeighbors(MapFieldForest, 0, pos);

	//maybe isExplored
	if (mf.playerInfo.IsExplored(*ThisPlayer)) {
		UI.Minimap.UpdateSeenXY(pos);
		MarkSeenTile(mf);
	}
}

/// Remove rock from the map.
void CMap::ClearRockTile(const Vec2i &pos)
{
	CMapField &mf = *this->Field(pos);

	mf.setGraphicTile(this->Tileset->getRemovedRockTile());
	mf.Flags &= ~(MapFieldRocks | MapFieldUnpassable);
	mf.Value = 0;
	
	UI.Minimap.UpdateXY(pos);
	FixNeighbors(MapFieldRocks, 0, pos);

	//maybe isExplored
	if (mf.playerInfo.IsExplored(*ThisPlayer)) {
		UI.Minimap.UpdateSeenXY(pos);
		MarkSeenTile(mf);
	}
}
*/
//Wyrmgus end

/**
**  Regenerate forest.
**
**  @param pos  Map tile pos
*/
//Wyrmgus start
//void CMap::RegenerateForestTile(const Vec2i &pos)
void CMap::RegenerateForestTile(const Vec2i &pos, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	Assert(Map.Info.IsPointOnMap(pos));
//	CMapField &mf = *this->Field(pos);
	Assert(Map.Info.IsPointOnMap(pos, z));
	CMapField &mf = *this->Field(pos, z);
	//Wyrmgus end

	//Wyrmgus start
//	if (mf.getGraphicTile() != this->Tileset->getRemovedTreeTile()) {
	if ((mf.OverlayTerrain && !mf.OverlayTerrainDestroyed) || !(mf.getFlag() & MapFieldStumps)) {
	//Wyrmgus end
		return;
	}

	//  Increment each value of no wood.
	//  If grown up, place new wood.
	//  FIXME: a better looking result would be fine
	//    Allow general updates to any tiletype that regrows

	//Wyrmgus start
	/*
	const unsigned int occupedFlag = (MapFieldWall | MapFieldUnpassable | MapFieldLandUnit | MapFieldBuilding);
	++mf.Value;
	if (mf.Value < ForestRegeneration) {
		return;
	}
	mf.Value = ForestRegeneration;
	if ((mf.Flags & occupedFlag) || pos.y == 0) {
		return;
	}
	*/
	
	const unsigned int permanent_occupied_flag = (MapFieldWall | MapFieldUnpassable | MapFieldBuilding);
	const unsigned int occupedFlag = (MapFieldWall | MapFieldUnpassable | MapFieldLandUnit | MapFieldBuilding | MapFieldItem); // don't regrow forests under items
	
	if ((mf.Flags & permanent_occupied_flag)) { //if the tree tile is occupied by buildings and the like, reset the regeneration process
		mf.Value = 0;
		return;
	}

	if ((mf.Flags & occupedFlag) || pos.y == 0) {
		return;
	}
	
	++mf.Value;
	if (mf.Value < ForestRegeneration) {
		return;
	}
	mf.Value = ForestRegeneration;
	//Wyrmgus end
	
	//Wyrmgus start
//	const Vec2i offset(0, -1);
//	CMapField &topMf = *(&mf - this->Info.MapWidth);

	for (int x_offset = -1; x_offset <= 1; x_offset+=2) { //increment by 2 to avoid instances where it is 0
		for (int y_offset = -1; y_offset <= 1; y_offset+=2) {
			const Vec2i verticalOffset(0, y_offset);
			CMapField &verticalMf = *this->Field(pos + verticalOffset, z);
			const Vec2i horizontalOffset(x_offset, 0);
			CMapField &horizontalMf = *this->Field(pos + horizontalOffset, z);
			const Vec2i diagonalOffset(x_offset, y_offset);
			CMapField &diagonalMf = *this->Field(pos + diagonalOffset, z);
			
			if (
				this->Info.IsPointOnMap(pos + verticalOffset, z)
				&& ((verticalMf.OverlayTerrain && verticalMf.OverlayTerrainDestroyed && (verticalMf.getFlag() & MapFieldStumps) && verticalMf.Value >= ForestRegeneration && !(verticalMf.Flags & occupedFlag)) || (verticalMf.getFlag() & MapFieldForest))
				&& this->Info.IsPointOnMap(pos + diagonalOffset, z)
				&& ((diagonalMf.OverlayTerrain && diagonalMf.OverlayTerrainDestroyed && (diagonalMf.getFlag() & MapFieldStumps) && diagonalMf.Value >= ForestRegeneration && !(diagonalMf.Flags & occupedFlag)) || (diagonalMf.getFlag() & MapFieldForest))
				&& this->Info.IsPointOnMap(pos + horizontalOffset, z)
				&& ((horizontalMf.OverlayTerrain && horizontalMf.OverlayTerrainDestroyed && (horizontalMf.getFlag() & MapFieldStumps) && horizontalMf.Value >= ForestRegeneration && !(horizontalMf.Flags & occupedFlag)) || (horizontalMf.getFlag() & MapFieldForest))
			) {
				DebugPrint("Real place wood\n");
				this->SetOverlayTerrainDestroyed(pos + verticalOffset, false, z);
				this->SetOverlayTerrainDestroyed(pos + diagonalOffset, false, z);
				this->SetOverlayTerrainDestroyed(pos + horizontalOffset, false, z);
				this->SetOverlayTerrainDestroyed(pos, false, z);
				
				return;
			}
		}
	}

	/*
	if (topMf.getGraphicTile() == this->Tileset->getRemovedTreeTile()
		&& topMf.Value >= ForestRegeneration
		&& !(topMf.Flags & occupedFlag)) {
		DebugPrint("Real place wood\n");
		topMf.setTileIndex(*Map.Tileset, Map.Tileset->getTopOneTreeTile(), 0);
		topMf.setGraphicTile(Map.Tileset->getTopOneTreeTile());
		topMf.playerInfo.SeenTile = topMf.getGraphicTile();
		topMf.Value = 0;
		topMf.Flags |= MapFieldForest | MapFieldUnpassable;
		UI.Minimap.UpdateSeenXY(pos + offset);
		UI.Minimap.UpdateXY(pos + offset);
		
		mf.setTileIndex(*Map.Tileset, Map.Tileset->getBottomOneTreeTile(), 0);
		mf.setGraphicTile(Map.Tileset->getBottomOneTreeTile());
		mf.playerInfo.SeenTile = mf.getGraphicTile();
		mf.Value = 0;
		mf.Flags |= MapFieldForest | MapFieldUnpassable;
		UI.Minimap.UpdateSeenXY(pos);
		UI.Minimap.UpdateXY(pos);
		
		if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(mf);
		}
		if (Map.Field(pos + offset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(topMf);
		}
		FixNeighbors(MapFieldForest, 0, pos + offset);
		FixNeighbors(MapFieldForest, 0, pos);
	}
	*/
}

/**
**  Regenerate forest.
*/
void CMap::RegenerateForest()
{
	if (!ForestRegeneration) {
		return;
	}
	Vec2i pos;
	//Wyrmgus start
	/*
	for (pos.y = 0; pos.y < Info.MapHeight; ++pos.y) {
		for (pos.x = 0; pos.x < Info.MapWidth; ++pos.x) {
			RegenerateForestTile(pos);
		}
	}
	*/
	for (size_t z = 0; z < this->Fields.size(); ++z) {
		for (pos.y = 0; pos.y < Info.MapHeights[z]; ++pos.y) {
			for (pos.x = 0; pos.x < Info.MapWidths[z]; ++pos.x) {
				RegenerateForestTile(pos, z);
			}
		}
	}
	//Wyrmgus end
}


/**
**  Load the map presentation
**
**  @param mapname  map filename
*/
void LoadStratagusMapInfo(const std::string &mapname)
{
	// Set the default map setup by replacing .smp with .sms
	size_t loc = mapname.find(".smp");
	if (loc != std::string::npos) {
		Map.Info.Filename = mapname;
		Map.Info.Filename.replace(loc, 4, ".sms");
	}

	const std::string filename = LibraryFileName(mapname.c_str());
	LuaLoadFile(filename);
}

//@}
