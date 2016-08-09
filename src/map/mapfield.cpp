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
/**@name mapfield.cpp - The map field. */
//
//      (c) Copyright 2013 by Joris Dauphin
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

#include "tile.h"

//Wyrmgus start
#include "editor.h"
//Wyrmgus end
#include "iolib.h"
#include "map.h"
#include "player.h"
#include "script.h"
#include "tileset.h"
#include "unit.h"
#include "unit_manager.h"

CMapField::CMapField() :
	//Wyrmgus start
	/*
#ifdef DEBUG
	tilesetTile(0),
#endif
	tile(0),
	*/
	//Wyrmgus end
	Flags(0),
	cost(0),
	Value(0),
	//Wyrmgus start
	AnimationFrame(0),
	OverlayAnimationFrame(0),
	Terrain(NULL), OverlayTerrain(NULL),
	SolidTile(0), OverlaySolidTile(0),
	OverlayTerrainDestroyed(false),
	OverlayTerrainDamaged(false),
	//Wyrmgus end
	UnitCache()
{}

bool CMapField::IsTerrainResourceOnMap(int resource) const
{
	// TODO: Hard coded stuff.
	if (resource == WoodCost) {
		return this->ForestOnMap();
	//Wyrmgus start
	} else if (resource == StoneCost) {
		return this->RockOnMap();
	//Wyrmgus end
	}
	return false;
}

bool CMapField::IsTerrainResourceOnMap() const
{
	for (int i = 0; i != MaxCosts; ++i) {
		if (IsTerrainResourceOnMap(i)) {
			return true;
		}
	}
	return false;
}

//Wyrmgus start
bool CMapField::IsSeenTileCorrect() const
{
	return this->Terrain == this->playerInfo.SeenTerrain && this->OverlayTerrain == this->playerInfo.SeenOverlayTerrain && this->SolidTile == this->playerInfo.SeenSolidTile && this->OverlaySolidTile == this->playerInfo.SeenOverlaySolidTile && this->TransitionTiles == this->playerInfo.SeenTransitionTiles && this->OverlayTransitionTiles == this->playerInfo.SeenOverlayTransitionTiles;
}
//Wyrmgus end

//Wyrmgus start
void CMapField::SetTerrain(CTerrainType *terrain)
{
	if (!terrain) {
		return;
	}
	
	//remove the flags of the old terrain type
	if (terrain->Overlay) {
		if (this->OverlayTerrain == terrain) {
			return;
		}
		if (this->OverlayTerrain) {
			this->Flags &= ~(this->OverlayTerrain->Flags);
		}
	} else {
		if (this->Terrain == terrain) {
			return;
		}
		if (this->Terrain) {
			this->Flags &= ~(this->Terrain->Flags);
			this->Flags &= ~(MapFieldCoastAllowed); // need to do this manually, since MapFieldCoast is added dynamically
		}
	}
	
	if (terrain->Overlay) {
		if (!this->Terrain || std::find(terrain->BaseTerrains.begin(), terrain->BaseTerrains.end(), this->Terrain) == terrain->BaseTerrains.end()) {
			this->SetTerrain(terrain->BaseTerrains[0]);
		}
		this->OverlayTerrain = terrain;
		if (terrain->SolidTiles.size() > 0) {
			this->OverlaySolidTile = terrain->SolidTiles[SyncRand(terrain->SolidTiles.size())];
		}
	} else {
		this->Terrain = terrain;
		if (terrain->SolidTiles.size() > 0) {
			this->SolidTile = terrain->SolidTiles[SyncRand(terrain->SolidTiles.size())];
		}
		if (this->OverlayTerrain && std::find(this->OverlayTerrain->BaseTerrains.begin(), this->OverlayTerrain->BaseTerrains.end(), terrain) == this->OverlayTerrain->BaseTerrains.end()) { //if the overlay terrain is incompatible with the new base terrain, remove the overlay
			this->Flags &= ~(this->OverlayTerrain->Flags);
			this->OverlayTerrain = NULL;
			this->OverlayTransitionTiles.clear();
		}
	}
	
	if (Editor.Running == EditorNotRunning && terrain->SolidAnimationFrames > 0) {
		if (terrain->Overlay) {
			this->OverlayAnimationFrame = SyncRand(terrain->SolidAnimationFrames);
		} else {
			this->AnimationFrame = SyncRand(terrain->SolidAnimationFrames);
		}
	} else {
		if (terrain->Overlay) {
			this->OverlayAnimationFrame = 0;
		} else {
			this->AnimationFrame = 0;
		}
	}
	
	//apply the flags from the new terrain type
	this->Flags |= terrain->Flags;
	// restore MapFieldAirUnpassable related to units (i.e. doors)
	const CUnitCache &cache = this->UnitCache;
	for (size_t i = 0; i != cache.size(); ++i) {
		CUnit &unit = *cache[i];
		if (unit.IsAliveOnMap() && unit.Type->BoolFlag[AIRUNPASSABLE_INDEX].value) {
			this->Flags |= MapFieldUnpassable;
			this->Flags |= MapFieldAirUnpassable;
		}
	}
	this->cost = 8; // default speed
}

void CMapField::SetOverlayTerrainDestroyed(bool destroyed)
{
	if (!this->OverlayTerrain || this->OverlayTerrainDestroyed == destroyed) {
		return;
	}
	
	this->OverlayTerrainDestroyed = destroyed;
	
	if (destroyed) {
		if (this->OverlayTerrain->DestroyedTiles.size() > 0) {
			this->OverlaySolidTile = this->OverlayTerrain->DestroyedTiles[SyncRand(this->OverlayTerrain->DestroyedTiles.size())];
		}
	} else {
		if (this->OverlayTerrain->SolidTiles.size() > 0) {
			this->OverlaySolidTile = this->OverlayTerrain->SolidTiles[SyncRand(this->OverlayTerrain->SolidTiles.size())];
		}
	}
}

void CMapField::SetOverlayTerrainDamaged(bool damaged)
{
	if (!this->OverlayTerrain || this->OverlayTerrainDamaged == damaged) {
		return;
	}
	
	this->OverlayTerrainDamaged = damaged;
	
	if (damaged) {
		if (this->OverlayTerrain->DamagedTiles.size() > 0) {
			this->OverlaySolidTile = this->OverlayTerrain->DamagedTiles[SyncRand(this->OverlayTerrain->DamagedTiles.size())];
		}
	} else {
		if (this->OverlayTerrain->SolidTiles.size() > 0) {
			this->OverlaySolidTile = this->OverlayTerrain->SolidTiles[SyncRand(this->OverlayTerrain->SolidTiles.size())];
		}
	}
}
//Wyrmgus end

void CMapField::setTileIndex(const CTileset &tileset, unsigned int tileIndex, int value)
{
	const CTile &tile = tileset.tiles[tileIndex];
	//Wyrmgus start
//	this->tile = tile.tile;
	//Wyrmgus end
	this->Value = value;
	//Wyrmgus start
	/*
#if 0
	this->Flags = tile.flag;
#else
	this->Flags &= ~(MapFieldHuman | MapFieldLandAllowed | MapFieldCoastAllowed |
					 MapFieldWaterAllowed | MapFieldNoBuilding | MapFieldUnpassable |
					 //Wyrmgus start
//					 MapFieldWall | MapFieldRocks | MapFieldForest);
					 MapFieldWall | MapFieldRocks | MapFieldForest |
					 MapFieldAirUnpassable | MapFieldDirt | MapFieldGrass |
					 MapFieldGravel | MapFieldMud | MapFieldStoneFloor | MapFieldStumps);
					 //Wyrmgus end
	this->Flags |= tile.flag;
#endif
	this->cost = 1 << (tile.flag & MapFieldSpeedMask);
#ifdef DEBUG
	this->tilesetTile = tileIndex;
#endif
	*/
	//Wyrmgus end
	
	//Wyrmgus start
	CTerrainType *terrain = GetTerrainType(tileset.getTerrainName(tile.tileinfo.BaseTerrain));
	this->SetTerrain(terrain);
	if (!terrain) {
		fprintf(stderr, "Terrain type \"%s\" doesn't exist.\n", tileset.getTerrainName(tile.tileinfo.BaseTerrain).c_str());
	}
	//Wyrmgus end
}

//Wyrmgus start
void CMapField::UpdateSeenTile()
{
	this->playerInfo.SeenTerrain = this->Terrain;
	this->playerInfo.SeenOverlayTerrain = this->OverlayTerrain;
	this->playerInfo.SeenSolidTile = this->SolidTile;
	this->playerInfo.SeenOverlaySolidTile = this->OverlaySolidTile;
	this->playerInfo.SeenTransitionTiles.clear();
	this->playerInfo.SeenTransitionTiles = this->TransitionTiles;
	this->playerInfo.SeenOverlayTransitionTiles.clear();
	this->playerInfo.SeenOverlayTransitionTiles = this->OverlayTransitionTiles;
}
//Wyrmgus end

void CMapField::Save(CFile &file) const
{
	//Wyrmgus start
//	file.printf("  {%3d, %3d, %2d, %2d", tile, playerInfo.SeenTile, Value, cost);
	file.printf("  {\"%s\", \"%s\", %s, %s, \"%s\", \"%s\", %d, %d, %d, %d, %2d, %2d", Terrain ? Terrain->Ident.c_str() : "", OverlayTerrain ? OverlayTerrain->Ident.c_str() : "", OverlayTerrainDamaged ? "true" : "false", OverlayTerrainDestroyed ? "true" : "false", playerInfo.SeenTerrain ? playerInfo.SeenTerrain->Ident.c_str() : "", playerInfo.SeenOverlayTerrain ? playerInfo.SeenOverlayTerrain->Ident.c_str() : "", SolidTile, OverlaySolidTile, playerInfo.SeenSolidTile, playerInfo.SeenOverlaySolidTile, Value, cost);
	
	for (size_t i = 0; i != TransitionTiles.size(); ++i) {
		file.printf(", \"transition-tile\", \"%s\", %d", TransitionTiles[i].first->Ident.c_str(), TransitionTiles[i].second);
	}
	
	for (size_t i = 0; i != OverlayTransitionTiles.size(); ++i) {
		file.printf(", \"overlay-transition-tile\", \"%s\", %d", OverlayTransitionTiles[i].first->Ident.c_str(), OverlayTransitionTiles[i].second);
	}
	
	for (size_t i = 0; i != playerInfo.SeenTransitionTiles.size(); ++i) {
		file.printf(", \"seen-transition-tile\", \"%s\", %d", playerInfo.SeenTransitionTiles[i].first->Ident.c_str(), playerInfo.SeenTransitionTiles[i].second);
	}
	
	for (size_t i = 0; i != playerInfo.SeenOverlayTransitionTiles.size(); ++i) {
		file.printf(", \"seen-overlay-transition-tile\", \"%s\", %d", playerInfo.SeenOverlayTransitionTiles[i].first->Ident.c_str(), playerInfo.SeenOverlayTransitionTiles[i].second);
	}
	//Wyrmgus end
	for (int i = 0; i != PlayerMax; ++i) {
		if (playerInfo.Visible[i] == 1) {
			file.printf(", \"explored\", %d", i);
		}
	}
	if (Flags & MapFieldHuman) {
		file.printf(", \"human\"");
	}
	if (Flags & MapFieldLandAllowed) {
		file.printf(", \"land\"");
	}
	if (Flags & MapFieldCoastAllowed) {
		file.printf(", \"coast\"");
	}
	if (Flags & MapFieldWaterAllowed) {
		file.printf(", \"water\"");
	}
	if (Flags & MapFieldNoBuilding) {
		//Wyrmgus start
//		file.printf(", \"mud\"");
		file.printf(", \"no-building\"");
		//Wyrmgus end
	}
	if (Flags & MapFieldUnpassable) {
		file.printf(", \"block\"");
	}
	if (Flags & MapFieldWall) {
		file.printf(", \"wall\"");
	}
	if (Flags & MapFieldRocks) {
		file.printf(", \"rock\"");
	}
	if (Flags & MapFieldForest) {
		file.printf(", \"wood\"");
	}
	//Wyrmgus start
	if (Flags & MapFieldAirUnpassable) {
		file.printf(", \"air-unpassable\"");
	}
	if (Flags & MapFieldDirt) {
		file.printf(", \"dirt\"");
	}
	if (Flags & MapFieldGrass) {
		file.printf(", \"grass\"");
	}
	if (Flags & MapFieldGravel) {
		file.printf(", \"gravel\"");
	}
	if (Flags & MapFieldMud) {
		file.printf(", \"mud\"");
	}
	if (Flags & MapFieldStoneFloor) {
		file.printf(", \"stone-floor\"");
	}
	if (Flags & MapFieldStumps) {
		file.printf(", \"stumps\"");
	}
	//Wyrmgus end
#if 1
	// Not Required for save
	// These are required for now, UnitType::FieldFlags is 0 until
	// UpdateStats is called which is after the game is loaded
	if (Flags & MapFieldLandUnit) {
		file.printf(", \"ground\"");
	}
	if (Flags & MapFieldAirUnit) {
		file.printf(", \"air\"");
	}
	if (Flags & MapFieldSeaUnit) {
		file.printf(", \"sea\"");
	}
	if (Flags & MapFieldBuilding) {
		file.printf(", \"building\"");
	}
	//Wyrmgus start
	if (Flags & MapFieldItem) {
		file.printf(", \"item\"");
	}
	if (Flags & MapFieldBridge) {
		file.printf(", \"bridge\"");
	}
	//Wyrmgus end
#endif
	file.printf("}");
}


void CMapField::parse(lua_State *l)
{
	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	const int len = lua_rawlen(l, -1);
	//Wyrmgus start
//	if (len < 4) {
	if (len < 12) {
	//Wyrmgus end
		LuaError(l, "incorrect argument");
	}

	//Wyrmgus start
	/*
	this->tile = LuaToNumber(l, -1, 1);
	this->playerInfo.SeenTile = LuaToNumber(l, -1, 2);
	this->Value = LuaToNumber(l, -1, 3);
	this->cost = LuaToNumber(l, -1, 4);
	*/
	this->Terrain = GetTerrainType(LuaToString(l, -1, 1));
	this->OverlayTerrain = GetTerrainType(LuaToString(l, -1, 2));
	this->SetOverlayTerrainDamaged(LuaToBoolean(l, -1, 3));
	this->SetOverlayTerrainDestroyed(LuaToBoolean(l, -1, 4));
	this->playerInfo.SeenTerrain = GetTerrainType(LuaToString(l, -1, 5));
	this->playerInfo.SeenOverlayTerrain = GetTerrainType(LuaToString(l, -1, 6));
	this->SolidTile = LuaToNumber(l, -1, 7);
	this->OverlaySolidTile = LuaToNumber(l, -1, 8);
	this->playerInfo.SeenSolidTile = LuaToNumber(l, -1, 9);
	this->playerInfo.SeenOverlaySolidTile = LuaToNumber(l, -1, 10);
	this->Value = LuaToNumber(l, -1, 11);
	this->cost = LuaToNumber(l, -1, 12);
	//Wyrmgus end

	//Wyrmgus start
//	for (int j = 4; j < len; ++j) {
	for (int j = 12; j < len; ++j) {
	//Wyrmgus end
		const char *value = LuaToString(l, -1, j + 1);

		//Wyrmgus start
//		if (!strcmp(value, "explored")) {
		if (!strcmp(value, "transition-tile")) {
			++j;
			CTerrainType *terrain = GetTerrainType(LuaToString(l, -1, j + 1));
			++j;
			int tile_number = LuaToNumber(l, -1, j + 1);
			this->TransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, tile_number));
		} else if (!strcmp(value, "overlay-transition-tile")) {
			++j;
			CTerrainType *terrain = GetTerrainType(LuaToString(l, -1, j + 1));
			++j;
			int tile_number = LuaToNumber(l, -1, j + 1);
			this->OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, tile_number));
		} else if (!strcmp(value, "seen-transition-tile")) {
			++j;
			CTerrainType *terrain = GetTerrainType(LuaToString(l, -1, j + 1));
			++j;
			int tile_number = LuaToNumber(l, -1, j + 1);
			this->playerInfo.SeenTransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, tile_number));
		} else if (!strcmp(value, "seen-overlay-transition-tile")) {
			++j;
			CTerrainType *terrain = GetTerrainType(LuaToString(l, -1, j + 1));
			++j;
			int tile_number = LuaToNumber(l, -1, j + 1);
			this->playerInfo.SeenOverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, tile_number));
		} else if (!strcmp(value, "explored")) {
		//Wyrmgus end
			++j;
			this->playerInfo.Visible[LuaToNumber(l, -1, j + 1)] = 1;
		} else if (!strcmp(value, "human")) {
			this->Flags |= MapFieldHuman;
		} else if (!strcmp(value, "land")) {
			this->Flags |= MapFieldLandAllowed;
		} else if (!strcmp(value, "coast")) {
			this->Flags |= MapFieldCoastAllowed;
		} else if (!strcmp(value, "water")) {
			this->Flags |= MapFieldWaterAllowed;
		//Wyrmgus start
//		} else if (!strcmp(value, "mud")) {
		} else if (!strcmp(value, "no-building")) {
		//Wyrmgus end
			this->Flags |= MapFieldNoBuilding;
		} else if (!strcmp(value, "block")) {
			this->Flags |= MapFieldUnpassable;
		} else if (!strcmp(value, "wall")) {
			this->Flags |= MapFieldWall;
		} else if (!strcmp(value, "rock")) {
			this->Flags |= MapFieldRocks;
		} else if (!strcmp(value, "wood")) {
			this->Flags |= MapFieldForest;
		} else if (!strcmp(value, "ground")) {
			this->Flags |= MapFieldLandUnit;
		} else if (!strcmp(value, "air")) {
			this->Flags |= MapFieldAirUnit;
		} else if (!strcmp(value, "sea")) {
			this->Flags |= MapFieldSeaUnit;
		} else if (!strcmp(value, "building")) {
			this->Flags |= MapFieldBuilding;
		//Wyrmgus start
		} else if (!strcmp(value, "item")) {
			this->Flags |= MapFieldItem;
		} else if (!strcmp(value, "bridge")) {
			this->Flags |= MapFieldBridge;
		} else if (!strcmp(value, "air-unpassable")) {
			this->Flags |= MapFieldAirUnpassable;
		} else if (!strcmp(value, "dirt")) {
			this->Flags |= MapFieldDirt;
		} else if (!strcmp(value, "grass")) {
			this->Flags |= MapFieldGrass;
		} else if (!strcmp(value, "gravel")) {
			this->Flags |= MapFieldGravel;
		} else if (!strcmp(value, "mud")) {
			this->Flags |= MapFieldMud;
		} else if (!strcmp(value, "stone-floor")) {
			this->Flags |= MapFieldStoneFloor;
		} else if (!strcmp(value, "stumps")) {
			this->Flags |= MapFieldStumps;
		//Wyrmgus end
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
}

/// Check if a field flags.
bool CMapField::CheckMask(int mask) const
{
	//Wyrmgus start
	//for purposes of this check, don't count MapFieldWaterAllowed and MapFieldCoastAllowed if there is a bridge present
	int check_flags = this->Flags;
	if (check_flags & MapFieldBridge) {
		check_flags &= ~(MapFieldWaterAllowed | MapFieldCoastAllowed);
	}
//	return (this->Flags & mask) != 0;
	return (check_flags & mask) != 0;
	//Wyrmgus end
}

/// Returns true, if water on the map tile field
bool CMapField::WaterOnMap() const
{
	return CheckMask(MapFieldWaterAllowed);
}

/// Returns true, if coast on the map tile field
bool CMapField::CoastOnMap() const
{
	return CheckMask(MapFieldCoastAllowed);
}

/// Returns true, if water on the map tile field
bool CMapField::ForestOnMap() const
{
	return CheckMask(MapFieldForest);
}

/// Returns true, if coast on the map tile field
bool CMapField::RockOnMap() const
{
	return CheckMask(MapFieldRocks);
}

bool CMapField::isAWall() const
{
	return Flags & MapFieldWall;
}
bool CMapField::isHuman() const
{
	return Flags & MapFieldHuman;
}

//Wyrmgus start
/*
bool CMapField::isAHumanWall() const
{
	const unsigned int humanWallFlag = (MapFieldWall | MapFieldHuman);
	return (Flags & humanWallFlag) == humanWallFlag;
}
bool CMapField::isAOrcWall() const
{
	const unsigned int humanWallFlag = (MapFieldWall | MapFieldHuman);
	return (Flags & humanWallFlag) == MapFieldWall;
}
*/
//Wyrmgus end

//
//  CMapFieldPlayerInfo
//

unsigned char CMapFieldPlayerInfo::TeamVisibilityState(const CPlayer &player) const
{
	if (IsVisible(player)) {
		return 2;
	}
	unsigned char maxVision = 0;
	if (IsExplored(player)) {
		maxVision = 1;
	}
	for (int i = 0; i != PlayerMax ; ++i) {
		//Wyrmgus start
//		if (player.IsBothSharedVision(Players[i])) {
		if (player.IsBothSharedVision(Players[i]) || Players[i].Revealed) {
		//Wyrmgus end
			//Wyrmgus start
			if (Players[i].Revealed && !player.IsBothSharedVision(Players[i]) && Visible[i] < 2) { //don't show a revealed player's explored tiles, only the currently visible ones
				continue;
			}
			//Wyrmgus end
			maxVision = std::max<unsigned char>(maxVision, Visible[i]);
			if (maxVision >= 2) {
				return 2;
			}
		}
	}
	if (maxVision == 1 && Map.NoFogOfWar) {
		return 2;
	}
	return maxVision;
}

bool CMapFieldPlayerInfo::IsExplored(const CPlayer &player) const
{
	return Visible[player.Index] != 0;
}

//Wyrmgus start
bool CMapFieldPlayerInfo::IsTeamExplored(const CPlayer &player) const
{
	return Visible[player.Index] != 0 || TeamVisibilityState(player) != 0;
}
//Wyrmgus end

bool CMapFieldPlayerInfo::IsVisible(const CPlayer &player) const
{
	const bool fogOfWar = !Map.NoFogOfWar;
	return Visible[player.Index] >= 2 || (!fogOfWar && IsExplored(player));
}

bool CMapFieldPlayerInfo::IsTeamVisible(const CPlayer &player) const
{
	return TeamVisibilityState(player) == 2;
}

//@}
