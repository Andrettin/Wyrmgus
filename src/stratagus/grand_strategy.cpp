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
/**@name grand_strategy.cpp - The grand strategy mode. */
//
//      (c) Copyright 2015 by Andrettin
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

#include "grand_strategy.h"

#include "character.h"
#include "game.h"	// for loading screen elements
#include "font.h"	// for grand strategy mode tooltip drawing
#include "interface.h"
#include "iolib.h"
#include "menus.h"
#include "player.h"
#include "results.h"
#include "ui.h"
#include "unit.h"
#include "unittype.h"
#include "upgrade.h"
#include "util.h"
#include "video.h"

#include <ctype.h>

#include <string>
#include <map>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

bool GrandStrategy = false;				///if the game is in grand strategy mode
bool GrandStrategyGamePaused = false;
bool GrandStrategyGameInitialized = false;
int GrandStrategyYear = 0;
std::string GrandStrategyWorld;
int WorldMapOffsetX;
int WorldMapOffsetY;
int GrandStrategyMapWidthIndent;
int GrandStrategyMapHeightIndent;
int BattalionMultiplier;
int PopulationGrowthThreshold = 1000;
std::string GrandStrategyInterfaceState;
CGrandStrategyGame GrandStrategyGame;
std::map<std::string, int> GrandStrategyHeroStringToIndex;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Clean up the GrandStrategy elements.
*/
void CGrandStrategyGame::Clean()
{
	for (int x = 0; x < WorldMapWidthMax; ++x) {
		for (int y = 0; y < WorldMapHeightMax; ++y) {
			if (this->WorldMapTiles[x][y]) {
				delete this->WorldMapTiles[x][y];
			}
		}
	}
	this->WorldMapWidth = 0;
	this->WorldMapHeight = 0;
	
	for (int i = 0; i < WorldMapTerrainTypeMax; ++i) {
		if (this->TerrainTypes[i]) {
			delete this->TerrainTypes[i];
		}
	}
	
	for (int i = 0; i < ProvinceMax; ++i) {
		if (this->Provinces[i]) {
			delete this->Provinces[i];
		}
	}
	this->ProvinceCount = 0;
	
	for (int i = 0; i < MaxCosts; ++i) {
		for (int j = 0; j < WorldMapResourceMax; ++j) {
			this->WorldMapResources[i][j].x = -1;
			this->WorldMapResources[i][j].y = -1;
		}
	}
	
	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (this->Factions[i][j]) {
				delete this->Factions[i][j];
			} else { //end of valid factions for this civilization
				break;
			}
		}
	}
	
	for (int i = 0; i < RiverMax; ++i) {
		if (this->Rivers[i]) {
			delete this->Rivers[i];
		}
	}
	
	for (size_t i = 0; i < GrandStrategyGame.Heroes.size(); ++i) {
		delete GrandStrategyGame.Heroes[i];
	}
	GrandStrategyGame.Heroes.clear();
	GrandStrategyHeroStringToIndex.clear();
	
	//destroy minimap surface
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		if (this->MinimapSurfaceGL) {
			glDeleteTextures(1, &this->MinimapTexture);
			delete[] this->MinimapSurfaceGL;
			this->MinimapSurfaceGL = NULL;
		}
	} else
#endif
	{
		if (this->MinimapSurface) {
			VideoPaletteListRemove(this->MinimapSurface);
			SDL_FreeSurface(this->MinimapSurface);
			this->MinimapSurface = NULL;
		}
	}
}

/**
**  Draw the grand strategy map.
*/
void CGrandStrategyGame::DrawMap()
{
	int grand_strategy_map_width = UI.MapArea.EndX - UI.MapArea.X;
	int grand_strategy_map_height = UI.MapArea.EndY - UI.MapArea.Y;
	
	int width_indent = GrandStrategyMapWidthIndent;
	int height_indent = GrandStrategyMapHeightIndent;
	
	for (int x = WorldMapOffsetX; x <= (WorldMapOffsetX + (grand_strategy_map_width / 64) + 1) && x < GetWorldMapWidth(); ++x) {
		for (int y = WorldMapOffsetY; y <= (WorldMapOffsetY + (grand_strategy_map_height / 64) + 1) && y < GetWorldMapHeight(); ++y) {
			if (this->WorldMapTiles[x][y]->GraphicTile) {
				if (this->TerrainTypes[this->WorldMapTiles[x][y]->Terrain]->BaseTile != -1 && this->WorldMapTiles[x][y]->BaseTile) {
					this->WorldMapTiles[x][y]->BaseTile->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
				}
				
				this->WorldMapTiles[x][y]->GraphicTile->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
			}
		}
	}
	
	// draw rivers (has to be drawn separately so that they appear over the terrain of the adjacent tiles they go over)
	for (int x = WorldMapOffsetX; x <= (WorldMapOffsetX + (grand_strategy_map_width / 64) + 1) && x < GetWorldMapWidth(); ++x) {
		for (int y = WorldMapOffsetY; y <= (WorldMapOffsetY + (grand_strategy_map_height / 64) + 1) && y < GetWorldMapHeight(); ++y) {
			if (this->WorldMapTiles[x][y]->Terrain != -1) {
				for (int i = 0; i < MaxDirections; ++i) {
					if (this->WorldMapTiles[x][y]->River[i] != -1) {
						//only draw diagonal directions if inner
						if (i == Northeast && (this->WorldMapTiles[x][y]->River[North] != -1 || this->WorldMapTiles[x][y]->River[East] != -1)) {
							continue;
						} else if (i == Southeast && (this->WorldMapTiles[x][y]->River[South] != -1 || this->WorldMapTiles[x][y]->River[East] != -1)) {
							continue;
						} else if (i == Southwest && (this->WorldMapTiles[x][y]->River[South] != -1 || this->WorldMapTiles[x][y]->River[West] != -1)) {
							continue;
						} else if (i == Northwest && (this->WorldMapTiles[x][y]->River[North] != -1 || this->WorldMapTiles[x][y]->River[West] != -1)) {
							continue;
						}
						
						if (this->WorldMapTiles[x][y]->Province != -1 && this->Provinces[this->WorldMapTiles[x][y]->Province]->Water) { //water tiles always use rivermouth graphics if they have a river
							//see from which direction the rivermouth comes
							bool flipped = false;
							if (i == North) {
								if (this->WorldMapTiles[x - 1][y] && this->WorldMapTiles[x - 1][y]->River[North] != -1) {
									flipped = true;
								}
							} else if (i == East) {
								if (this->WorldMapTiles[x][y - 1] && this->WorldMapTiles[x][y - 1]->River[East] != -1) {
									flipped = true;
								}
							} else if (i == South) {
								if (this->WorldMapTiles[x - 1][y] && this->WorldMapTiles[x - 1][y]->River[South] != -1) {
									flipped = true;
								}
							} else if (i == West) {
								if (this->WorldMapTiles[x][y - 1] && this->WorldMapTiles[x][y - 1]->River[West] != -1) {
									flipped = true;
								}
							}
							
							if (flipped) {
								this->RivermouthGraphics[i][1]->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent - 10, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 10, true);
							} else {
								this->RivermouthGraphics[i][0]->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent - 10, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 10, true);
							}
						} else if (this->WorldMapTiles[x][y]->Riverhead[i] != -1) {
							//see to which direction the riverhead runs
							bool flipped = false;
							if (i == North) {
								if (this->WorldMapTiles[x][y]->River[Northwest] != -1) {
									flipped = true;
								}
							} else if (i == East) {
								if (this->WorldMapTiles[x][y]->River[Northeast] != -1) {
									flipped = true;
								}
							} else if (i == South) {
								if (this->WorldMapTiles[x][y]->River[Southwest] != -1) {
									flipped = true;
								}
							} else if (i == West) {
								if (this->WorldMapTiles[x][y]->River[Northwest] != -1) {
									flipped = true;
								}
							}
							
							if (flipped) {
								this->RiverheadGraphics[i][1]->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent - 10, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 10, true);
							} else {
								this->RiverheadGraphics[i][0]->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent - 10, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 10, true);
							}
						} else {
							this->RiverGraphics[i]->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent - 10, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 10, true);
						}
					}
				}
			}
		}
	}
	
	// draw pathways (has to be drawn separately so that they appear over the terrain of the adjacent tiles they go over)
	for (int x = WorldMapOffsetX; x <= (WorldMapOffsetX + (grand_strategy_map_width / 64) + 1) && x < GetWorldMapWidth(); ++x) {
		for (int y = WorldMapOffsetY; y <= (WorldMapOffsetY + (grand_strategy_map_height / 64) + 1) && y < GetWorldMapHeight(); ++y) {
			if (this->WorldMapTiles[x][y]->Terrain != -1) {
				for (int i = 0; i < MaxDirections; ++i) {
					if (this->WorldMapTiles[x][y]->Pathway[i] != -1) {
						this->PathwayGraphics[this->WorldMapTiles[x][y]->Pathway[i]][i]->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
					}
				}
			}
		}
	}
	
	//draw settlement and resource graphics after rivers and pathways so that they appear over them
	for (int x = WorldMapOffsetX; x <= (WorldMapOffsetX + (grand_strategy_map_width / 64) + 1) && x < GetWorldMapWidth(); ++x) {
		for (int y = WorldMapOffsetY; y <= (WorldMapOffsetY + (grand_strategy_map_height / 64) + 1) && y < GetWorldMapHeight(); ++y) {
			if (this->WorldMapTiles[x][y]->Terrain != -1) {
				int player_color = this->WorldMapTiles[x][y]->Province != -1 && this->Provinces[this->WorldMapTiles[x][y]->Province]->Owner != NULL ? PlayerRaces.Factions[this->Provinces[this->WorldMapTiles[x][y]->Province]->Owner->Civilization][this->Provinces[this->WorldMapTiles[x][y]->Province]->Owner->Faction]->Colors[0] : 15;
				
				int province_id = this->WorldMapTiles[x][y]->Province;
				
				if (this->WorldMapTiles[x][y]->Resource != -1 && this->WorldMapTiles[x][y]->ResourceProspected) {
					this->WorldMapTiles[x][y]->ResourceBuildingGraphics->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
					
					if (this->WorldMapTiles[x][y]->ResourceBuildingGraphicsPlayerColor != NULL) {
						this->WorldMapTiles[x][y]->ResourceBuildingGraphicsPlayerColor->DrawPlayerColorFrameClip(player_color, 0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
					}
					
					if (!this->WorldMapTiles[x][y]->Worked && province_id != -1 && this->Provinces[this->WorldMapTiles[x][y]->Province]->Owner == this->PlayerFaction) {
						this->SymbolResourceNotWorked->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
					}
				}
				
				if (province_id != -1) {
					int civilization = this->Provinces[province_id]->Civilization;
					if (civilization != -1 && this->Provinces[province_id]->Owner != NULL) {
						//draw the province's settlement
						if (this->Provinces[province_id]->SettlementLocation.x == x && this->Provinces[province_id]->SettlementLocation.y == y && this->Provinces[province_id]->HasBuildingClass("town-hall")) {
							if (this->Provinces[province_id]->Owner->HasTechnologyClass("masonry") && this->SettlementMasonryGraphics[civilization]) {
								this->SettlementMasonryGraphics[civilization]->DrawPlayerColorFrameClip(player_color, 0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
							} else {
								this->SettlementGraphics[civilization]->DrawPlayerColorFrameClip(player_color, 0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
								
								if (this->BarracksGraphics[civilization] && this->Provinces[province_id]->HasBuildingClass("barracks")) {
									this->BarracksGraphics[civilization]->DrawPlayerColorFrameClip(player_color, 0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
								}
							}
						}
					}
					
					//draw symbol that the province is being attacked by the human player, if that is the case
					if (this->Provinces[province_id]->AttackedBy != NULL && this->Provinces[province_id]->AttackedBy == this->PlayerFaction && this->Provinces[province_id]->SettlementLocation.x == x && this->Provinces[province_id]->SettlementLocation.y == y) {
						this->SymbolAttack->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
					} else if (this->Provinces[province_id]->Movement && this->Provinces[province_id]->Owner != NULL && this->Provinces[province_id]->Owner == this->PlayerFaction && this->Provinces[province_id]->SettlementLocation.x == x && this->Provinces[province_id]->SettlementLocation.y == y) {
						this->SymbolMove->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
					}
					
					if (this->Provinces[province_id]->Heroes.size() > 0 && this->Provinces[province_id]->Owner != NULL && this->Provinces[province_id]->Owner == this->PlayerFaction && this->Provinces[province_id]->SettlementLocation.x == x && this->Provinces[province_id]->SettlementLocation.y == y) {
						this->SymbolHero->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent, 16 + 64 * (y - WorldMapOffsetY) + height_indent, true);
					}
				}
			}
		}
	}
	
	//draw the tile borders (they need to be drawn here, so that they appear over all tiles, as they go beyond their own tile)
	for (int x = WorldMapOffsetX; x <= (WorldMapOffsetX + (grand_strategy_map_width / 64) + 1) && x < GetWorldMapWidth(); ++x) {
		for (int y = WorldMapOffsetY; y <= (WorldMapOffsetY + (grand_strategy_map_height / 64) + 1) && y < GetWorldMapHeight(); ++y) {
			int province_id = this->WorldMapTiles[x][y]->Province;
			if (province_id != -1) {
				for (int i = 0; i < MaxDirections; ++i) {
					if (this->WorldMapTiles[x][y]->Borders[i]) {
						//only draw diagonal directions if inner
						if (i == Northeast && (this->WorldMapTiles[x][y]->Borders[North] || this->WorldMapTiles[x][y]->Borders[East])) {
							continue;
						} else if (i == Southeast && (this->WorldMapTiles[x][y]->Borders[South] || this->WorldMapTiles[x][y]->Borders[East])) {
							continue;
						} else if (i == Southwest && (this->WorldMapTiles[x][y]->Borders[South] || this->WorldMapTiles[x][y]->Borders[West])) {
							continue;
						} else if (i == Northwest && (this->WorldMapTiles[x][y]->Borders[North] || this->WorldMapTiles[x][y]->Borders[West])) {
							continue;
						}
						int sub_x = 0;
						int sub_y = 0;
						if (i == North) {
							sub_y = -1;
						} else if (i == Northeast) {
							sub_x = 1;
							sub_y = -1;
						} else if (i == East) {
							sub_x = 1;
						} else if (i == Southeast) {
							sub_x = 1;
							sub_y = 1;
						} else if (i == South) {
							sub_y = 1;
						} else if (i == Southwest) {
							sub_x = -1;
							sub_y = 1;
						} else if (i == West) {
							sub_x = -1;
						} else if (i == Northwest) {
							sub_x = -1;
							sub_y = -1;
						}
						
						int second_province_id = -1;
						if (this->WorldMapTiles[x + sub_x][y + sub_y]) {
							second_province_id = this->WorldMapTiles[x + sub_x][y + sub_y]->Province;
						}
						
						if (second_province_id == -1 || (this->Provinces[province_id]->Owner == this->Provinces[second_province_id]->Owner)) { // is not a national border
							this->BorderGraphics[i]->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent - 10, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 10, true);
						} else {
							int player_color;
							if (this->Provinces[province_id]->Owner != NULL) {
								player_color = PlayerRaces.Factions[this->Provinces[province_id]->Owner->Civilization][this->Provinces[province_id]->Owner->Faction]->Colors[0];
							} else {
								player_color = 15;
							}
										
							this->NationalBorderGraphics[i]->DrawPlayerColorFrameClip(player_color, 0, 64 * (x - WorldMapOffsetX) + width_indent - 10, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 10, true);
						}
					}
				}
			}
		}
	}
	
	//if is clicking on a tile, draw a square on its borders
	if (!GrandStrategyGamePaused && UI.MapArea.Contains(CursorScreenPos) && this->WorldMapTiles[this->GetTileUnderCursor().x][this->GetTileUnderCursor().y]->Terrain != -1 && (MouseButtons & LeftButton)) {
		int tile_screen_x = ((this->GetTileUnderCursor().x - WorldMapOffsetX) * 64) + UI.MapArea.X + width_indent;
		int tile_screen_y = ((this->GetTileUnderCursor().y - WorldMapOffsetY) * 64) + UI.MapArea.Y + height_indent;
			
//		clamp(&tile_screen_x, 0, Video.Width);
//		clamp(&tile_screen_y, 0, Video.Height);
			
		Video.DrawRectangle(ColorWhite, tile_screen_x, tile_screen_y, 64, 64);
	}
	
	//draw fog over terra incognita
	for (int x = WorldMapOffsetX; x <= (WorldMapOffsetX + (grand_strategy_map_width / 64) + 1) && x < GetWorldMapWidth(); ++x) {
		for (int y = WorldMapOffsetY; y <= (WorldMapOffsetY + (grand_strategy_map_height / 64) + 1) && y < GetWorldMapHeight(); ++y) {
			if (this->WorldMapTiles[x][y]->Terrain == -1) {
				this->FogTile->DrawFrameClip(0, 64 * (x - WorldMapOffsetX) + width_indent - 16, 16 + 64 * (y - WorldMapOffsetY) + height_indent - 16, true);
			}
		}
	}
}

/**
**  Draw the grand strategy map.
*/
void CGrandStrategyGame::DrawMinimap()
{
	#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		glBindTexture(GL_TEXTURE_2D, this->MinimapTexture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->MinimapTextureWidth, this->MinimapTextureHeight,
						GL_RGBA, GL_UNSIGNED_BYTE, this->MinimapSurfaceGL);

	#ifdef USE_GLES
		float texCoord[] = {
			0.0f, 0.0f,
			(float)this->MinimapTextureWidth / this->MinimapTextureWidth, 0.0f,
			0.0f, (float)this->MinimapTextureHeight / this->MinimapTextureHeight,
			(float)this->MinimapTextureWidth / this->MinimapTextureWidth, (float)this->MinimapTextureHeight / this->MinimapTextureHeight
		};

		float vertex[] = {
			2.0f / (GLfloat)Video.Width *(UI.Minimap.X + this->MinimapOffsetX) - 1.0f, -2.0f / (GLfloat)Video.Height *(UI.Minimap.Y + this->MinimapOffsetY) + 1.0f,
			2.0f / (GLfloat)Video.Width *(UI.Minimap.X + this->MinimapOffsetX + this->MinimapTextureWidth) - 1.0f, -2.0f / (GLfloat)Video.Height *(UI.Minimap.Y + this->MinimapOffsetY) + 1.0f,
			2.0f / (GLfloat)Video.Width *(UI.Minimap.X + this->MinimapOffsetX) - 1.0f, -2.0f / (GLfloat)Video.Height *(UI.Minimap.Y + this->MinimapOffsetY + this->MinimapTextureHeight) + 1.0f,
			2.0f / (GLfloat)Video.Width *(UI.Minimap.X + this->MinimapOffsetX + this->MinimapTextureWidth) - 1.0f, -2.0f / (GLfloat)Video.Height *(UI.Minimap.Y + this->MinimapOffsetY + this->MinimapTextureHeight) + 1.0f
		};

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);

		glTexCoordPointer(2, GL_FLOAT, 0, texCoord);
		glVertexPointer(2, GL_FLOAT, 0, vertex);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
#endif
#ifdef USE_OPENGL
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2i(UI.Minimap.X + this->MinimapOffsetX, UI.Minimap.Y + this->MinimapOffsetY);
		glTexCoord2f(0.0f, (float)this->MinimapTextureHeight / this->MinimapTextureHeight);
		glVertex2i(UI.Minimap.X + this->MinimapOffsetX, UI.Minimap.Y + this->MinimapOffsetY + this->MinimapTextureHeight);
		glTexCoord2f((float)this->MinimapTextureWidth / this->MinimapTextureWidth, (float)this->MinimapTextureHeight / this->MinimapTextureHeight);
		glVertex2i(UI.Minimap.X + this->MinimapOffsetX + this->MinimapTextureWidth, UI.Minimap.Y + this->MinimapOffsetY + this->MinimapTextureHeight);
		glTexCoord2f((float)this->MinimapTextureWidth / this->MinimapTextureWidth, 0.0f);
		glVertex2i(UI.Minimap.X + this->MinimapOffsetX + this->MinimapTextureWidth, UI.Minimap.Y + this->MinimapOffsetY);
		glEnd();
#endif
	} else
#endif
	{
		SDL_Rect drect = {Sint16(UI.Minimap.X + this->MinimapOffsetX), Sint16(UI.Minimap.Y + this->MinimapOffsetY), 0, 0};
		SDL_BlitSurface(this->MinimapSurface, NULL, TheScreen, &drect);
	}

	int start_x = UI.Minimap.X + GrandStrategyGame.MinimapOffsetX + (WorldMapOffsetX * this->MinimapTileWidth / 1000);
	int start_y = UI.Minimap.Y + GrandStrategyGame.MinimapOffsetY + (WorldMapOffsetY * this->MinimapTileHeight / 1000);
	int rectangle_width = (((UI.MapArea.EndX - UI.MapArea.X) / 64) + 1) * this->MinimapTileWidth / 1000;
	int rectangle_height = (((UI.MapArea.EndY - UI.MapArea.Y) / 64) + 1) * this->MinimapTileHeight / 1000;

	Video.DrawRectangle(ColorGray, start_x, start_y, rectangle_width, rectangle_height);
}

void CGrandStrategyGame::DrawInterface()
{
	if (this->PlayerFaction != NULL && this->PlayerFaction->ProvinceCount > 0) { //draw resource bar
		std::vector<int> stored_resources;
		stored_resources.push_back(GoldCost);
		stored_resources.push_back(WoodCost);
		stored_resources.push_back(StoneCost);
		stored_resources.push_back(ResearchCost);
		stored_resources.push_back(PrestigeCost);

		Vec2i hovered_research_icon(-1, -1);
		Vec2i hovered_prestige_icon(-1, -1);
		for (size_t i = 0; i < stored_resources.size(); ++i) {
			int x = 154 + (100 * i);
			int y = 0;
			UI.Resources[stored_resources[i]].G->DrawFrameClip(0, x, y, true);
			
			int quantity_stored = this->PlayerFaction->Resources[stored_resources[i]];
			int income = 0;
			if (stored_resources[i] == GoldCost) {
				income = this->PlayerFaction->Income[stored_resources[i]] - this->PlayerFaction->Upkeep;
			} else if (stored_resources[i] == ResearchCost) {
				income = this->PlayerFaction->Income[stored_resources[i]] / this->PlayerFaction->ProvinceCount;
			} else {
				income = this->PlayerFaction->Income[stored_resources[i]];
			}
			std::string income_string;
			if (income != 0) {
				if (income > 0) {
					income_string += "+";
				}
				income_string += std::to_string((long long) income);
			}
			std::string resource_stored_string = std::to_string((long long) quantity_stored) + income_string;
			
			if (resource_stored_string.size() <= 9) {
				CLabel(GetGameFont()).Draw(x + 18, y + 1, resource_stored_string);
			} else {
				CLabel(GetSmallFont()).Draw(x + 18, y + 1 + 2, resource_stored_string);
			}
			
			if (CursorScreenPos.x >= x && CursorScreenPos.x <= (x + UI.Resources[stored_resources[i]].G->getGraphicWidth()) && CursorScreenPos.y >= y && CursorScreenPos.y <= (y + UI.Resources[stored_resources[i]].G->getGraphicHeight())) {
				if (stored_resources[i] == ResearchCost) {
					hovered_research_icon.x = x;
					hovered_research_icon.y = y;
				} else if (stored_resources[i] == PrestigeCost) {
					hovered_prestige_icon.x = x;
					hovered_prestige_icon.y = y;
				}
			}
		}
		if (hovered_research_icon.x != -1 && hovered_research_icon.y != -1) {
			DrawGenericPopup("Gain Research by building town halls, lumber mills and smithies", hovered_research_icon.x, hovered_research_icon.y);
		} else if (hovered_prestige_icon.x != -1 && hovered_prestige_icon.y != -1) {
			DrawGenericPopup("Prestige influences trade priority between nations, among other things", hovered_prestige_icon.x, hovered_prestige_icon.y);
		}
	}
	
	int item_y = 0;
	
	if (this->SelectedProvince != -1) {
		std::string interface_state_name;
		
		if (GrandStrategyInterfaceState == "Province") {
			//draw show heroes button
			if (GrandStrategyGame.Provinces[this->SelectedProvince]->Heroes.size() > 0 && GrandStrategyGame.Provinces[this->SelectedProvince]->Owner != NULL && GrandStrategyGame.Provinces[this->SelectedProvince]->Owner == this->PlayerFaction && UI.GrandStrategyShowHeroesButton.X != -1) {
				DrawUIButton(
					UI.GrandStrategyShowHeroesButton.Style,
					(UI.GrandStrategyShowHeroesButton.Contains(CursorScreenPos) ? MI_FLAGS_ACTIVE : 0) |
					(UI.GrandStrategyShowHeroesButton.Clicked || UI.GrandStrategyShowHeroesButton.HotkeyPressed ? MI_FLAGS_CLICKED : 0),
					UI.GrandStrategyShowHeroesButton.X, UI.GrandStrategyShowHeroesButton.Y,
					UI.GrandStrategyShowHeroesButton.Text
				);
			}
		} else if (GrandStrategyInterfaceState == "town-hall" || GrandStrategyInterfaceState == "stronghold") {
			if (GrandStrategyGame.Provinces[this->SelectedProvince]->Civilization != -1) {
				std::string province_culture_string = "Province Culture: " + CapitalizeString(PlayerRaces.Name[GrandStrategyGame.Provinces[this->SelectedProvince]->Civilization]);
				CLabel(GetGameFont()).Draw(UI.InfoPanel.X + ((218 - 6) / 2) - (GetGameFont().Width(province_culture_string) / 2), UI.InfoPanel.Y + 180 - 94 + (item_y * 23), province_culture_string);
				item_y += 1;
			}
			
			std::string administrative_efficiency_string = "Administrative Efficiency: " + std::to_string((long long) 100 + GrandStrategyGame.Provinces[this->SelectedProvince]->GetAdministrativeEfficiencyModifier()) + "%";
			CLabel(GetGameFont()).Draw(UI.InfoPanel.X + ((218 - 6) / 2) - (GetGameFont().Width(administrative_efficiency_string) / 2), UI.InfoPanel.Y + 180 - 94 + (item_y * 23), administrative_efficiency_string);
			item_y += 1;
			
			std::string population_string = "Population: " + std::to_string((long long) GrandStrategyGame.Provinces[this->SelectedProvince]->GetPopulation());
			CLabel(GetGameFont()).Draw(UI.InfoPanel.X + ((218 - 6) / 2) - (GetGameFont().Width(population_string) / 2), UI.InfoPanel.Y + 180 - 94 + (item_y * 23), population_string);
			item_y += 1;
			
			std::string food_string = std::to_string((long long) GrandStrategyGame.Provinces[this->SelectedProvince]->PopulationGrowthProgress) + "/" + std::to_string((long long) PopulationGrowthThreshold);
			int food_change = GrandStrategyGame.Provinces[this->SelectedProvince]->Income[GrainCost] + GrandStrategyGame.Provinces[this->SelectedProvince]->Income[MushroomCost] + GrandStrategyGame.Provinces[this->SelectedProvince]->Income[FishCost] - GrandStrategyGame.Provinces[this->SelectedProvince]->FoodConsumption;

			if (food_change > 0) {
				food_string += "+" + std::to_string((long long) food_change);
			} else if (food_change < 0) {
				food_string += "-" + std::to_string((long long) food_change * -1);
			}
			
			UI.Resources[FoodCost].G->DrawFrameClip(0, UI.InfoPanel.X + ((218 - 6) / 2) - ((GetGameFont().Width(food_string) + 18) / 2), UI.InfoPanel.Y + 180 - 94 + (item_y * 23), true);
			CLabel(GetGameFont()).Draw(UI.InfoPanel.X + ((218 - 6) / 2) - ((GetGameFont().Width(food_string) + 18) / 2) + 18, UI.InfoPanel.Y + 180 - 94 + (item_y * 23), food_string);
			
			//draw show ruler button
			if (UI.GrandStrategyShowRulerButton.X != -1 && GrandStrategyGame.PlayerFaction != NULL && GrandStrategyGame.PlayerFaction->Ruler != NULL) {
				DrawUIButton(
					UI.GrandStrategyShowRulerButton.Style,
					(UI.GrandStrategyShowRulerButton.Contains(CursorScreenPos) ? MI_FLAGS_ACTIVE : 0) |
					(UI.GrandStrategyShowRulerButton.Clicked || UI.GrandStrategyShowRulerButton.HotkeyPressed ? MI_FLAGS_CLICKED : 0),
					UI.GrandStrategyShowRulerButton.X, UI.GrandStrategyShowRulerButton.Y,
					UI.GrandStrategyShowRulerButton.Text
				);
			}
		} else if (GrandStrategyInterfaceState == "barracks") {
			std::string revolt_risk_string = "Revolt Risk: " + std::to_string((long long) GrandStrategyGame.Provinces[this->SelectedProvince]->GetRevoltRisk()) + "%";
			CLabel(GetGameFont()).Draw(UI.InfoPanel.X + ((218 - 6) / 2) - (GetGameFont().Width(revolt_risk_string) / 2), UI.InfoPanel.Y + 180 - 94 + (item_y * 23), revolt_risk_string);
			item_y += 1;
		} else if (GrandStrategyInterfaceState == "lumber-mill" || GrandStrategyInterfaceState == "smithy") {
			std::string labor_string = std::to_string((long long) GrandStrategyGame.Provinces[this->SelectedProvince]->Labor);
			UI.Resources[LaborCost].G->DrawFrameClip(0, UI.InfoPanel.X + ((218 - 6) / 2) - ((GetGameFont().Width(labor_string) + 18) / 2), UI.InfoPanel.Y + 180 - 94 + (item_y * 23), true);
			CLabel(GetGameFont()).Draw(UI.InfoPanel.X + ((218 - 6) / 2) - ((GetGameFont().Width(labor_string) + 18) / 2) + 18, UI.InfoPanel.Y + 180 - 94 + (item_y * 23), labor_string);
		} else if (GrandStrategyInterfaceState == "Ruler") {
			interface_state_name = GrandStrategyInterfaceState;
			
			if (GrandStrategyGame.Provinces[this->SelectedProvince]->Owner != NULL && GrandStrategyGame.Provinces[this->SelectedProvince]->Owner->Ruler != NULL) {
				interface_state_name = GrandStrategyGame.Provinces[this->SelectedProvince]->Owner->GetRulerTitle();
			
				std::string ruler_name_string = GrandStrategyGame.Provinces[this->SelectedProvince]->Owner->Ruler->GetFullName();
				CLabel(GetGameFont()).Draw(UI.InfoPanel.X + ((218 - 6) / 2) - (GetGameFont().Width(ruler_name_string) / 2), UI.InfoPanel.Y + 180 - 94 + (item_y * 23), ruler_name_string);
				item_y += 1;
				
				std::string ruler_class_string = "Class: " + GrandStrategyGame.Provinces[this->SelectedProvince]->Owner->Ruler->Type->Name;
				CLabel(GetGameFont()).Draw(UI.InfoPanel.X + ((218 - 6) / 2) - (GetGameFont().Width(ruler_class_string) / 2), UI.InfoPanel.Y + 180 - 94 + (item_y * 23), ruler_class_string);
				item_y += 1;
				
				std::string ruler_effects_string = GrandStrategyGame.Provinces[this->SelectedProvince]->Owner->Ruler->GetRulerEffectsString();
				CLabel(GetGameFont()).Draw(UI.InfoPanel.X + ((218 - 6) / 2) - (GetGameFont().Width(ruler_effects_string) / 2), UI.InfoPanel.Y + 180 - 94 + (item_y * 23), ruler_effects_string);
				item_y += 1;
			}
		}
		
		if (!interface_state_name.empty()) {
			CLabel(GetGameFont()).Draw(UI.InfoPanel.X + 109 - (GetGameFont().Width(interface_state_name) / 2), UI.InfoPanel.Y + 53, interface_state_name);
		}
		
		if (GrandStrategyInterfaceState != "Province" && GrandStrategyInterfaceState != "Diplomacy") {
			//draw "OK" button to return to the province interface
			if (UI.GrandStrategyOKButton.X != -1) {
				DrawUIButton(
					UI.GrandStrategyOKButton.Style,
					(UI.GrandStrategyOKButton.Contains(CursorScreenPos) ? MI_FLAGS_ACTIVE : 0) |
					(UI.GrandStrategyOKButton.Clicked || UI.GrandStrategyOKButton.HotkeyPressed ? MI_FLAGS_CLICKED : 0),
					UI.GrandStrategyOKButton.X, UI.GrandStrategyOKButton.Y,
					UI.GrandStrategyOKButton.Text
				);
			}
		}
	}
	
	if (UI.MenuButton.X != -1) {
		DrawUIButton(
			UI.MenuButton.Style,
			(UI.MenuButton.Contains(CursorScreenPos) ? MI_FLAGS_ACTIVE : 0) |
			(UI.MenuButton.Clicked || UI.MenuButton.HotkeyPressed ? MI_FLAGS_CLICKED : 0),
			UI.MenuButton.X, UI.MenuButton.Y,
			UI.MenuButton.Text
		);
	}
	
	if (UI.GrandStrategyEndTurnButton.X != -1) {
		DrawUIButton(
			UI.GrandStrategyEndTurnButton.Style,
			(UI.GrandStrategyEndTurnButton.Contains(CursorScreenPos) ? MI_FLAGS_ACTIVE : 0) |
			(UI.GrandStrategyEndTurnButton.Clicked || UI.GrandStrategyEndTurnButton.HotkeyPressed ? MI_FLAGS_CLICKED : 0),
			UI.GrandStrategyEndTurnButton.X, UI.GrandStrategyEndTurnButton.Y,
			UI.GrandStrategyEndTurnButton.Text
		);
	}
}

/**
**  Draw the grand strategy tile tooltip.
*/
void CGrandStrategyGame::DrawTileTooltip(int x, int y)
{
	std::string tile_tooltip;
	
	int province_id = GrandStrategyGame.WorldMapTiles[x][y]->Province;
	int res = GrandStrategyGame.WorldMapTiles[x][y]->Resource;
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]->Owner != NULL && GrandStrategyGame.Provinces[province_id]->SettlementLocation.x == x && GrandStrategyGame.Provinces[province_id]->SettlementLocation.y == y && GrandStrategyGame.Provinces[province_id]->HasBuildingClass("town-hall")) {
		tile_tooltip += "Settlement";
		if (!GrandStrategyGame.Provinces[province_id]->GetCulturalSettlementName().empty()) {
			tile_tooltip += " of ";
			tile_tooltip += GrandStrategyGame.Provinces[province_id]->GetCulturalSettlementName();
		}
		tile_tooltip += " (";
		tile_tooltip += GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name;
		tile_tooltip += ")";
		if (GrandStrategyGame.Provinces[province_id]->ProductionCapacityFulfilled[FishCost] > 0 && GrandStrategyGame.Provinces[province_id]->Owner == GrandStrategyGame.PlayerFaction) {
			tile_tooltip += " (COST_";
			tile_tooltip += std::to_string((long long) FishCost);
			tile_tooltip += " ";
			tile_tooltip += std::to_string((long long) GrandStrategyGame.Provinces[province_id]->Income[FishCost]);
			tile_tooltip += ")";
		}
	} else if (res != -1 && GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected) {
		if (!GrandStrategyGame.WorldMapTiles[x][y]->Worked && province_id != -1 && GrandStrategyGame.Provinces[province_id]->Owner == GrandStrategyGame.PlayerFaction) {
			tile_tooltip += "Unused ";
		}
		
		if (res == GoldCost) {
			tile_tooltip += "Gold Mine";
		} else if (res == SilverCost) {
			tile_tooltip += "Silver Mine";
		} else if (res == CopperCost) {
			tile_tooltip += "Copper Mine";
		} else if (res == WoodCost) {
			tile_tooltip += "Timber Lodge";
		} else if (res == StoneCost) {
			tile_tooltip += "Quarry";
		} else if (res == GrainCost) {
			tile_tooltip += "Grain Farm";
		} else if (res == MushroomCost) {
			tile_tooltip += "Mushroom Farm";
		}
		tile_tooltip += " (";
		tile_tooltip += GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name;
		tile_tooltip += ")";
		if (GrandStrategyGame.WorldMapTiles[x][y]->Worked && province_id != -1 && GrandStrategyGame.Provinces[province_id]->Owner == GrandStrategyGame.PlayerFaction) {
			tile_tooltip += " (COST_";
			tile_tooltip += std::to_string((long long) res);
			tile_tooltip += " ";
			tile_tooltip += std::to_string((long long) GrandStrategyGame.Provinces[province_id]->Income[res] / GrandStrategyGame.Provinces[province_id]->ProductionCapacityFulfilled[res]);
			tile_tooltip += ")";
		}
	} else if (GrandStrategyGame.WorldMapTiles[x][y]->Terrain != -1) {
		if (!GrandStrategyGame.WorldMapTiles[x][y]->GetCulturalName().empty()) { //if the terrain feature has a particular name, use it
			tile_tooltip += GrandStrategyGame.WorldMapTiles[x][y]->GetCulturalName();
			tile_tooltip += " (";
			tile_tooltip += GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name;
			tile_tooltip += ")";
		} else {
			tile_tooltip += GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name;
		}
	} else {
		tile_tooltip += "Unexplored";
	}
	
	if (province_id != -1 && !GrandStrategyGame.Provinces[province_id]->Water) {
		int tooltip_rivers[MaxDirections];
		memset(tooltip_rivers, -1, sizeof(tooltip_rivers));
		int tooltip_river_count = 0;
		for (int i = 0; i < MaxDirections; ++i) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->River[i] != -1) {
				bool already_in_tooltip = false;
				for (int j = 0; j < MaxDirections; ++j) {
					if (tooltip_rivers[j] == -1) { //reached blank spot, no need to continue the loop
						break;
					}

					if (tooltip_rivers[j] == GrandStrategyGame.WorldMapTiles[x][y]->River[i]) {
						already_in_tooltip = true;
						break;
					}
				}
				if (!already_in_tooltip) {
					tooltip_rivers[tooltip_river_count] = GrandStrategyGame.WorldMapTiles[x][y]->River[i];
					tooltip_river_count += 1;
					tile_tooltip += " (";
					if (!GrandStrategyGame.Rivers[GrandStrategyGame.WorldMapTiles[x][y]->River[i]]->GetCulturalName(GrandStrategyGame.Provinces[province_id]->Civilization).empty()) {
						tile_tooltip += GrandStrategyGame.Rivers[GrandStrategyGame.WorldMapTiles[x][y]->River[i]]->GetCulturalName(GrandStrategyGame.Provinces[province_id]->Civilization) + " ";
					}
					tile_tooltip += "River";
					tile_tooltip += ")";
				}
			}
		}
	}
				
	if (province_id != -1 && !GrandStrategyGame.Provinces[province_id]->Water) {
		int tooltip_pathways[MaxDirections];
		memset(tooltip_pathways, -1, sizeof(tooltip_pathways));
		int tooltip_pathway_count = 0;
		for (int i = 0; i < MaxDirections; ++i) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->Pathway[i] != -1) {
				bool already_in_tooltip = false;
				for (int j = 0; j < MaxDirections; ++j) {
					if (tooltip_pathways[j] == -1) { //reached blank spot, no need to continue the loop
						break;
					}

					if (tooltip_pathways[j] == GrandStrategyGame.WorldMapTiles[x][y]->Pathway[i]) {
						already_in_tooltip = true;
						break;
					}
				}
				if (!already_in_tooltip) {
					tooltip_pathways[tooltip_pathway_count] = GrandStrategyGame.WorldMapTiles[x][y]->Pathway[i];
					tooltip_pathway_count += 1;
					tile_tooltip += " (";
					if (GrandStrategyGame.WorldMapTiles[x][y]->Pathway[i] == PathwayTrail) {
						tile_tooltip += "Trail";
					} else if (GrandStrategyGame.WorldMapTiles[x][y]->Pathway[i] == PathwayRoad) {
						tile_tooltip += "Road";
					}
					tile_tooltip += ")";
				}
			}
		}
	}

	/*
	if (GrandStrategyGame.WorldMapTiles[x][y]->Port) { //deactivated for now, since there aren't proper port graphics yet
		tile_tooltip += " (";
		tile_tooltip += "Port";
		tile_tooltip += ")";
	}
	*/
	
	if (province_id != -1) {
		tile_tooltip += ",\n";
		tile_tooltip += GrandStrategyGame.Provinces[province_id]->GetCulturalName();
		
		if (GrandStrategyGame.Provinces[province_id]->Owner != NULL) {
			tile_tooltip += ",\n";
			tile_tooltip += GrandStrategyGame.Provinces[province_id]->Owner->GetFullName();
		}
	}
	tile_tooltip += "\n(";
	tile_tooltip += std::to_string((long long) x);
	tile_tooltip += ", ";
	tile_tooltip += std::to_string((long long) y);
	tile_tooltip += ")";

	if (!Preference.NoStatusLineTooltips) {
		CLabel(GetGameFont()).Draw(UI.StatusLine.TextX, UI.StatusLine.TextY, tile_tooltip);
	}
	
	//draw tile tooltip as a popup
	DrawGenericPopup(tile_tooltip, 0, UI.InfoPanel.Y);
}

void CGrandStrategyGame::DoTurn()
{
	//allocate labor
	for (int i = 0; i < this->ProvinceCount; ++i) {
		if (this->Provinces[i] && !this->Provinces[i]->Name.empty()) { //if this is a valid province
			if (this->Provinces[i]->Civilization != -1 && this->Provinces[i]->Owner != NULL && this->Provinces[i]->Labor > 0) { // if this province has a culture and an owner, and has surplus labor
				this->Provinces[i]->AllocateLabor();
			}
		} else { //if a somehow invalid province is reached
			break;
		}
	}
	
	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (this->Factions[i][j]) {
				if (this->Factions[i][j]->IsAlive()) {
					//faction income
					for (int k = 0; k < MaxCosts; ++k) {
						if (k == GrainCost || k == MushroomCost || k == FishCost || k == SilverCost || k == CopperCost) { //food resources are not added to the faction's storage, being stored at the province level instead, and silver and copper are converted to gold
							continue;
						} else if (k == ResearchCost) {
							this->Factions[i][j]->Resources[k] += this->Factions[i][j]->Income[k] / this->Factions[i][j]->ProvinceCount;
						} else {
							this->Factions[i][j]->Resources[k] += this->Factions[i][j]->Income[k];
						}
					}
					
					// try to perform ruler succession for existent factions without rulers
					if (this->Factions[i][j]->Ruler == NULL) {
						this->Factions[i][j]->RulerSuccession();
					}
				} else {
					if (this->Factions[i][j]->Ruler != NULL) {
						this->Factions[i][j]->SetRuler(""); //"dead" factions should have no ruler
					}
				}
			} else { //end of valid factions for this civilization
				break;
			}
		}
	}
	
	//this function takes care only of some things for now, move the rest from Lua later
	this->DoTrade();
	
	for (int i = 0; i < this->ProvinceCount; ++i) {
		if (this->Provinces[i] && !this->Provinces[i]->Name.empty()) { //if this is a valid province
			if (this->Provinces[i]->Civilization != -1) { // if this province has a culture
				// construct buildings
				if (this->Provinces[i]->CurrentConstruction != -1) {
					this->Provinces[i]->SetSettlementBuilding(this->Provinces[i]->CurrentConstruction, true);
					this->Provinces[i]->CurrentConstruction = -1;
				}
					
				// if the province has a town hall, a barracks and a smithy, give it a mercenary camp; not for Earth for now, since there are no recruitable mercenaries for Earth yet
				int mercenary_camp_id = UnitTypeIdByIdent("unit-mercenary-camp");
				if (mercenary_camp_id != -1 && this->Provinces[i]->SettlementBuildings[mercenary_camp_id] == false && GrandStrategyWorld != "Earth") {
					if (this->Provinces[i]->HasBuildingClass("town-hall") && this->Provinces[i]->HasBuildingClass("barracks") && this->Provinces[i]->HasBuildingClass("smithy")) {
						this->Provinces[i]->SetSettlementBuilding(mercenary_camp_id, true);
					}
				}
				
				if (this->Provinces[i]->Owner != NULL) {
					//check revolt risk and potentially trigger a revolt
					if (
						this->Provinces[i]->GetRevoltRisk() > 0
						&& SyncRand(100) < this->Provinces[i]->GetRevoltRisk()
						&& this->Provinces[i]->AttackedBy == NULL
						&& this->Provinces[i]->TotalWorkers > 0
					) { //if a revolt is triggered this turn (a revolt can only happen if the province is not being attacked that turn, and the quantity of revolting units is based on the quantity of workers in the province)
						int possible_revolters[FactionMax];
						int possible_revolter_count = 0;
						for (size_t j = 0; j < this->Provinces[i]->Claims.size(); ++j) {
							if (
								this->Provinces[i]->Claims[j]->Civilization == this->Provinces[i]->Civilization
								&& PlayerRaces.Factions[this->Provinces[i]->Civilization][this->Provinces[i]->Claims[j]->Faction]->Type == PlayerRaces.Factions[this->Provinces[i]->Owner->Civilization][this->Provinces[i]->Owner->Faction]->Type
								&& !(this->Provinces[i]->Claims[j] == this->Provinces[i]->Owner)
								&& PlayerRaces.Factions[GrandStrategyGame.Provinces[i]->Claims[j]->Civilization][GrandStrategyGame.Provinces[i]->Claims[j]->Faction]->Name != PlayerRaces.Factions[GrandStrategyGame.Provinces[i]->Owner->Civilization][GrandStrategyGame.Provinces[i]->Owner->Faction]->Name // they can't have the same name (this is needed because some of the Lua code identifies factions by name)
							) { //if faction which has a claim on this province has the same civilization as the province, and if it is of the same faction type as the province's owner
								possible_revolters[possible_revolter_count] = this->Provinces[i]->Claims[j]->Faction;
								possible_revolter_count += 1;
							}
						}
						if (possible_revolter_count > 0) {
							int revolter_faction = possible_revolters[SyncRand(possible_revolter_count)];
							this->Provinces[i]->AttackedBy = const_cast<CGrandStrategyFaction *>(&(*GrandStrategyGame.Factions[this->Provinces[i]->Civilization][revolter_faction]));
							
							int militia_id = this->Provinces[i]->GetClassUnitType(GetUnitTypeClassIndexByName("militia"));
							int infantry_id = this->Provinces[i]->GetClassUnitType(GetUnitTypeClassIndexByName("infantry"));
							
							if (militia_id != -1 && this->Provinces[i]->TotalWorkers >= 2) {
								this->Provinces[i]->SetAttackingUnitQuantity(militia_id, (this->Provinces[i]->TotalWorkers / 2) + (SyncRand(this->Provinces[i]->TotalWorkers / 2)));
							} else if (infantry_id != -1 && this->Provinces[i]->TotalWorkers >= 4) { //if the province's civilization doesn't have militia units, use infantry instead (but with half the quantity)
								this->Provinces[i]->SetAttackingUnitQuantity(infantry_id, (this->Provinces[i]->TotalWorkers / 4) + (SyncRand(this->Provinces[i]->TotalWorkers * 3 / 4)));
							}
						}
					}
					
					if (!this->Provinces[i]->HasFactionClaim(this->Provinces[i]->Owner->Civilization, this->Provinces[i]->Owner->Faction) && SyncRand(100) < 1) { // 1% chance the owner of this province will get a claim on it
						this->Provinces[i]->AddFactionClaim(this->Provinces[i]->Owner->Civilization, this->Provinces[i]->Owner->Faction);
					}
					
					
					//population growth
	//				this->Provinces[i]->PopulationGrowthProgress += (this->Provinces[i]->GetPopulation() / 2) * BasePopulationGrowthPermyriad / 10000;
					int province_food_income = this->Provinces[i]->Income[GrainCost] + this->Provinces[i]->Income[MushroomCost] + this->Provinces[i]->Income[FishCost] - this->Provinces[i]->FoodConsumption;
					this->Provinces[i]->PopulationGrowthProgress += province_food_income;
					if (this->Provinces[i]->PopulationGrowthProgress >= PopulationGrowthThreshold) { //if population growth progress is greater than or equal to the population growth threshold, create a new worker unit
						if (province_food_income >= 100) { //if province food income is enough to support a new unit
							int worker_unit_type = this->Provinces[i]->GetClassUnitType(GetUnitTypeClassIndexByName("worker"));
							int new_units = this->Provinces[i]->PopulationGrowthProgress / PopulationGrowthThreshold;
							this->Provinces[i]->PopulationGrowthProgress -= PopulationGrowthThreshold * new_units;
							
							this->Provinces[i]->ChangeUnitQuantity(worker_unit_type, new_units);
						} else { //if the province's food income is positive, but not enough to sustain a new unit, keep it at the population growth threshold
							this->Provinces[i]->PopulationGrowthProgress = PopulationGrowthThreshold;
						}
					} else if (province_food_income < 0) { // if the province's food income is negative, then try to reallocate labor
						this->Provinces[i]->ReallocateLabor();
						province_food_income = this->Provinces[i]->Income[GrainCost] + this->Provinces[i]->Income[MushroomCost] + this->Provinces[i]->Income[FishCost] - this->Provinces[i]->FoodConsumption;
						//if the food income is still negative after reallocating labor (this shouldn't happen most of the time!) then decrease the population by 1 due to starvation; only do this if the population is above 1 (to prevent provinces from being entirely depopulated and unable to grow a population afterwards)
						if (province_food_income < 0 && this->Provinces[i]->TotalWorkers > 1) {
							int worker_unit_type = this->Provinces[i]->GetClassUnitType(GetUnitTypeClassIndexByName("worker"));
							this->Provinces[i]->ChangeUnitQuantity(worker_unit_type, -1);
							if (this->Provinces[i]->Owner == this->PlayerFaction) { //if this is one of the player's provinces, display a message about the population starving
								char buf[256];
								snprintf(
									buf, sizeof(buf), "if (GenericDialog ~= nil) then GenericDialog(\"%s\", \"%s\") end;",
									("Starvation in " + this->Provinces[i]->GetCulturalName()).c_str(),
									("My lord, food stocks have been depleted in " + this->Provinces[i]->GetCulturalName() + ". The local population is starving!").c_str()
								);
								CclCommand(buf);
							}
						}
					}
					this->Provinces[i]->PopulationGrowthProgress = std::max(0, this->Provinces[i]->PopulationGrowthProgress);
				}
			}
			this->Provinces[i]->Movement = false; //after processing the turn, always set the movement to false
		} else { //if a somehow invalid province is reached
			break;
		}
	}
	
	//research technologies
	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (this->Factions[i][j]) {
				if (this->Factions[i][j]->IsAlive()) {
					if (this->Factions[i][j]->CurrentResearch != -1) {
						this->Factions[i][j]->SetTechnology(this->Factions[i][j]->CurrentResearch, true);
						this->Factions[i][j]->CurrentResearch = -1;
					}
					
					//see if this faction can form a faction
					this->Factions[i][j]->CheckFormableFactions(i);
				}
			} else { //end of valid factions for this civilization
				break;
			}
		}
	}
	
	//check if any heroes should begin activity this year
	for (size_t i = 0; i < this->Heroes.size(); ++i) {
		if (
			// for historical personages to appear, they require three things: the year of their historical rise to prominence, ownership of the province in which they were born or raised, and that that province be of the correct culture for them, if they belonged to the cultural majority (or if the civilization of the province's owner is the same as the hero, as undoubtedly administrators and the like would exist from the faction's culture in any of its provinces)
			this->Heroes[i]->Year == GrandStrategyYear
			&& this->Heroes[i]->State == 0
			&& this->Heroes[i]->ProvinceOfOrigin != NULL
			&& this->Heroes[i]->ProvinceOfOrigin->Owner != NULL
			&& (this->Heroes[i]->ProvinceOfOrigin->Civilization == this->Heroes[i]->Civilization || this->Heroes[i]->ProvinceOfOrigin->Owner->Civilization == this->Heroes[i]->Civilization)
		) {
			//make heroes appear in their start year
			this->Heroes[i]->Create();
		}
	}
	
	//check if any heroes should die this year (this needs to be done as its own loop to allow new rulers to appear in the same year their predecessor dies, and succeede him)
	for (size_t i = 0; i < this->Heroes.size(); ++i) {
		if (
			this->Heroes[i]->DeathYear == GrandStrategyYear
			&& this->Heroes[i]->State != 0
		) {
			this->Heroes[i]->Die();
		}
	}
}

void CGrandStrategyGame::DoTrade()
{
	//store the human player's trade settings
	int player_trade_preferences[MaxCosts];
	if (this->PlayerFaction != NULL) {
		for (int i = 0; i < MaxCosts; ++i) {
			player_trade_preferences[i] = this->PlayerFaction->Trade[i];
		}
	}
	
	bool province_consumed_commodity[MaxCosts][ProvinceMax];
	for (int i = 0; i < MaxCosts; ++i) {
		for (int j = 0; j < this->ProvinceCount; ++j) {
			province_consumed_commodity[i][j] = false;
		}
	}
	
	// first sell to domestic provinces, then to other factions, and only then to foreign provinces
	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (this->Factions[i][j]) {
				if (this->Factions[i][j]->IsAlive()) {
					for (int k = 0; k < this->Factions[i][j]->ProvinceCount; ++k) {
						int province_id = this->Factions[i][j]->OwnedProvinces[k];
						for (int res = 0; res < MaxCosts; ++res) {
							if (res == GoldCost || res == SilverCost || res == CopperCost || res == ResearchCost || res == PrestigeCost || res == LaborCost || res == GrainCost || res == MushroomCost || res == FishCost) {
								continue;
							}
							
							if (province_consumed_commodity[res][province_id] == false && this->Factions[i][j]->Trade[res] >= this->Provinces[province_id]->GetResourceDemand(res) && this->Provinces[province_id]->HasBuildingClass("town-hall")) {
								this->Factions[i][j]->Resources[res] -= this->Provinces[province_id]->GetResourceDemand(res);
								this->Factions[i][j]->Resources[GoldCost] += this->Provinces[province_id]->GetResourceDemand(res) * this->CommodityPrices[res] / 100;
								this->Factions[i][j]->Trade[res] -= this->Provinces[province_id]->GetResourceDemand(res);
								province_consumed_commodity[res][province_id] = true;
							}
						}
					}
				}
			} else { //end of valid factions for this civilization
				break;
			}
		}
	}
	
	CGrandStrategyFaction *factions_by_prestige[MAX_RACES * FactionMax];
	int factions_by_prestige_count = 0;
	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (this->Factions[i][j]) {
				if (this->Factions[i][j]->IsAlive()) {
					factions_by_prestige[factions_by_prestige_count] = const_cast<CGrandStrategyFaction *>(&(*this->Factions[i][j]));
					factions_by_prestige_count += 1;
				}
			} else { //end of valid factions for this civilization
				break;
			}
		}
	}
	
	//sort factions by prestige
	bool swapped = true;
	for (int passes = 0; passes < factions_by_prestige_count && swapped; ++passes) {
		swapped = false;
		for (int i = 0; i < factions_by_prestige_count - 1; ++i) {
			if (factions_by_prestige[i] && factions_by_prestige[i + 1]) {
				if (!TradePriority(*factions_by_prestige[i], *factions_by_prestige[i + 1])) {
					CGrandStrategyFaction *temp_faction = const_cast<CGrandStrategyFaction *>(&(*factions_by_prestige[i]));
					factions_by_prestige[i] = const_cast<CGrandStrategyFaction *>(&(*factions_by_prestige[i + 1]));
					factions_by_prestige[i + 1] = const_cast<CGrandStrategyFaction *>(&(*temp_faction));
					swapped = true;
				}
			} else {
				break;
			}
		}
	}

	for (int i = 0; i < factions_by_prestige_count; ++i) {
		if (factions_by_prestige[i]) {
			for (int res = 0; res < MaxCosts; ++res) {
				if (res == GoldCost || res == SilverCost || res == CopperCost || res == ResearchCost || res == PrestigeCost || res == LaborCost || res == GrainCost || res == MushroomCost || res == FishCost) {
					continue;
				}
				
				if (factions_by_prestige[i]->Trade[res] < 0) { // if wants to import lumber
					for (int j = 0; j < factions_by_prestige_count; ++j) {
						if (j != i && factions_by_prestige[j]) {
							if (factions_by_prestige[j]->Trade[res] > 0) { // if second faction wants to export lumber
								this->PerformTrade(*factions_by_prestige[i], *factions_by_prestige[j], res);
							}
						} else {
							break;
						}
					}
				} else if (factions_by_prestige[i]->Trade[res] > 0) { // if wants to export lumber
					for (int j = 0; j < factions_by_prestige_count; ++j) {
						if (j != i && factions_by_prestige[j]) {
							if (factions_by_prestige[j]->Trade[res] < 0) { // if second faction wants to import lumber
								this->PerformTrade(*factions_by_prestige[j], *factions_by_prestige[i], res);
							}
						} else {
							break;
						}
					}
				}
			}
		} else {
			break;
		}
	}
	
	//sell to foreign provinces
	for (int i = 0; i < factions_by_prestige_count; ++i) {
		if (factions_by_prestige[i]) {
			for (int j = 0; j < factions_by_prestige_count; ++j) {
				if (j != i && factions_by_prestige[j]) {
					for (int k = 0; k < factions_by_prestige[j]->ProvinceCount; ++k) {
						int province_id = factions_by_prestige[j]->OwnedProvinces[k];
						
						for (int res = 0; res < MaxCosts; ++res) {
							if (res == GoldCost || res == SilverCost || res == CopperCost || res == ResearchCost || res == PrestigeCost || res == LaborCost || res == GrainCost || res == MushroomCost || res == FishCost) {
								continue;
							}
							
							if (province_consumed_commodity[res][province_id] == false && factions_by_prestige[i]->Trade[res] >= this->Provinces[province_id]->GetResourceDemand(res) && this->Provinces[province_id]->HasBuildingClass("town-hall")) {
								factions_by_prestige[i]->Resources[res] -= this->Provinces[province_id]->GetResourceDemand(res);
								factions_by_prestige[i]->Resources[GoldCost] += this->Provinces[province_id]->GetResourceDemand(res) * this->CommodityPrices[res] / 100;
								factions_by_prestige[i]->Trade[res] -= this->Provinces[province_id]->GetResourceDemand(res);
								province_consumed_commodity[res][province_id] = true;
							}
						}
					}
				} else {
					break;
				}
			}
		} else {
			break;
		}
	}

	// check whether offers or bids have been greater, and change the commodity's price accordingly
	int remaining_wanted_trade[MaxCosts];
	memset(remaining_wanted_trade, 0, sizeof(remaining_wanted_trade));
	for (int res = 0; res < MaxCosts; ++res) {
		if (res == GoldCost || res == SilverCost || res == CopperCost || res == ResearchCost || res == PrestigeCost || res == LaborCost || res == GrainCost || res == MushroomCost || res == FishCost) {
			continue;
		}
		
		for (int i = 0; i < factions_by_prestige_count; ++i) {
			if (factions_by_prestige[i]) {
				remaining_wanted_trade[res] += factions_by_prestige[i]->Trade[res];
			} else {
				break;
			}
		}
		
		for (int i = 0; i < this->ProvinceCount; ++i) {
			if (this->Provinces[i] && !this->Provinces[i]->Name.empty()) { //if this is a valid province
				if (this->Provinces[i]->HasBuildingClass("town-hall") && this->Provinces[i]->Owner != NULL) {
					if (province_consumed_commodity[res][i] == false) {
						remaining_wanted_trade[res] -= this->Provinces[i]->GetResourceDemand(res);
					}
				}
			} else { //if a somehow invalid province is reached
				break;
			}
		}
	
		if (remaining_wanted_trade[res] > 0 && this->CommodityPrices[res] > 1) { // more offers than bids
			this->CommodityPrices[res] -= 1;
		} else if (remaining_wanted_trade[res] < 0) { // more bids than offers
			this->CommodityPrices[res] += 1;
		}
	}
	
	//now restore the human player's trade settings
	if (this->PlayerFaction != NULL) {
		for (int i = 0; i < MaxCosts; ++i) {
			if (i == GoldCost || i == SilverCost || i == CopperCost || i == ResearchCost || i == PrestigeCost || i == LaborCost || i == GrainCost || i == MushroomCost || i == FishCost) {
				continue;
			}
		
			if (player_trade_preferences[i] > 0 && this->PlayerFaction->Resources[i] < player_trade_preferences[i]) {
				player_trade_preferences[i] = this->PlayerFaction->Resources[i];
			} else if (player_trade_preferences[i] < 0 && this->PlayerFaction->Resources[GoldCost] < 0) {
				player_trade_preferences[i] = 0;
			} else if (player_trade_preferences[i] < 0 && this->PlayerFaction->Resources[GoldCost] < (player_trade_preferences[i] * -1 * this->CommodityPrices[i] / 100)) {
				player_trade_preferences[i] = (this->PlayerFaction->Resources[GoldCost] / this->CommodityPrices[i] * 100) * -1;
			}
			this->PlayerFaction->Trade[i] = player_trade_preferences[i];
		}
	}
}

void CGrandStrategyGame::DoProspection()
{
	for (int i = 0; i < MaxCosts; ++i) {
		for (int j = 0; j < WorldMapResourceMax; ++j) {
			if (GrandStrategyGame.WorldMapResources[i][j].x != -1 && GrandStrategyGame.WorldMapResources[i][j].y != -1) {
				int x = GrandStrategyGame.WorldMapResources[i][j].x;
				int y = GrandStrategyGame.WorldMapResources[i][j].y;
				
				if (GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected) { //already discovered
					continue;
				}

				int province_id = GrandStrategyGame.WorldMapTiles[x][y]->Province;
						
				if (province_id != -1 && GrandStrategyGame.Provinces[province_id]->Owner != NULL && GrandStrategyGame.Provinces[province_id]->HasBuildingClass("town-hall")) {
					if (SyncRand(100) < 1) { // 1% chance of discovery per turn
						GrandStrategyGame.WorldMapTiles[x][y]->SetResourceProspected(i, true);
						if (GrandStrategyGame.PlayerFaction == GrandStrategyGame.Provinces[province_id]->Owner) {
							char buf[256];
							snprintf(
								buf, sizeof(buf), "if (GenericDialog ~= nil) then GenericDialog(\"%s\", \"%s\") end;",
								(CapitalizeString(DefaultResourceNames[i]) + " found in " + GrandStrategyGame.Provinces[province_id]->GetCulturalName()).c_str(),
								("My lord, " + CapitalizeString(DefaultResourceNames[i]) + " has been found in the " + DecapitalizeString(GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name) + " of " + GrandStrategyGame.Provinces[province_id]->GetCulturalName() + "!").c_str()
							);
							CclCommand(buf);
							
							/*
							if (wyr.preferences.ShowTips) {
								Tip("Gold Discovery in Province", "Congratulations! You have found gold in one of your provinces. Each gold mine provides you with 200 gold per turn, if a town hall is built in its province.")
							}
							*/
						}
					}
				}
			} else {
				break;
			}
		}
	}
}

void CGrandStrategyGame::PerformTrade(CGrandStrategyFaction &importer_faction, CGrandStrategyFaction &exporter_faction, int resource)
{
	if (abs(importer_faction.Trade[resource]) > exporter_faction.Trade[resource]) {
		importer_faction.Resources[resource] += exporter_faction.Trade[resource];
		exporter_faction.Resources[resource] -= exporter_faction.Trade[resource];

		importer_faction.Resources[GoldCost] -= exporter_faction.Trade[resource] * this->CommodityPrices[resource] / 100;
		exporter_faction.Resources[GoldCost] += exporter_faction.Trade[resource] * this->CommodityPrices[resource] / 100;
		
		importer_faction.Trade[resource] += exporter_faction.Trade[resource];
		exporter_faction.Trade[resource] = 0;
	} else {
		importer_faction.Resources[resource] += abs(importer_faction.Trade[resource]);
		exporter_faction.Resources[resource] -= abs(importer_faction.Trade[resource]);

		importer_faction.Resources[GoldCost] -= abs(importer_faction.Trade[resource]) * this->CommodityPrices[resource] / 100;
		exporter_faction.Resources[GoldCost] += abs(importer_faction.Trade[resource]) * this->CommodityPrices[resource] / 100;
		
		exporter_faction.Trade[resource] += importer_faction.Trade[resource];
		importer_faction.Trade[resource] = 0;
	}
}

bool CGrandStrategyGame::IsPointOnMap(int x, int y)
{
	if (x < 0 || x >= this->WorldMapWidth || y < 0 || y >= this->WorldMapHeight || !WorldMapTiles[x][y]) {
		return false;
	}
	return true;
}

bool CGrandStrategyGame::TradePriority(CGrandStrategyFaction &faction_a, CGrandStrategyFaction &faction_b)
{
	return faction_a.Resources[PrestigeCost] > faction_b.Resources[PrestigeCost];
}

Vec2i CGrandStrategyGame::GetTileUnderCursor()
{
	Vec2i tile_under_cursor(0, 0);

	if (UI.MapArea.Contains(CursorScreenPos)) {
		int width_indent = GrandStrategyMapWidthIndent;
		int height_indent = GrandStrategyMapHeightIndent;

		tile_under_cursor.x = WorldMapOffsetX + ((CursorScreenPos.x - UI.MapArea.X - width_indent) / 64);
		tile_under_cursor.y = WorldMapOffsetY + ((CursorScreenPos.y - UI.MapArea.Y - height_indent) / 64);
	} else if (
		CursorScreenPos.x >= UI.Minimap.X + GrandStrategyGame.MinimapOffsetX
		&& CursorScreenPos.x < UI.Minimap.X + GrandStrategyGame.MinimapOffsetX + GrandStrategyGame.MinimapTextureWidth
		&& CursorScreenPos.y >= UI.Minimap.Y + GrandStrategyGame.MinimapOffsetY
		&& CursorScreenPos.y < UI.Minimap.Y + GrandStrategyGame.MinimapOffsetY + GrandStrategyGame.MinimapTextureHeight
	) {
		tile_under_cursor.x = (CursorScreenPos.x - UI.Minimap.X - GrandStrategyGame.MinimapOffsetX) * 1000 / this->MinimapTileWidth;
		tile_under_cursor.y = (CursorScreenPos.y - UI.Minimap.Y - GrandStrategyGame.MinimapOffsetY) * 1000 / this->MinimapTileHeight;
	}
	
	return tile_under_cursor;
}

CGrandStrategyHero *CGrandStrategyGame::GetHero(std::string hero_full_name)
{
	if (!hero_full_name.empty() && GrandStrategyHeroStringToIndex.find(hero_full_name) != GrandStrategyHeroStringToIndex.end()) {
		return this->Heroes[GrandStrategyHeroStringToIndex[hero_full_name]];
	} else {
		return NULL;
	}
}

#if defined(USE_OPENGL) || defined(USE_GLES)
/**
**  Create the minimap texture
*/
void CGrandStrategyGame::CreateMinimapTexture()
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &this->MinimapTexture);
	glBindTexture(GL_TEXTURE_2D, this->MinimapTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->MinimapTextureWidth,
				 this->MinimapTextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
				 this->MinimapSurfaceGL);
}
#endif

void CGrandStrategyGame::UpdateMinimap()
{
	// Clear Minimap background if not transparent
	#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		memset(this->MinimapSurfaceGL, 0, this->MinimapTextureWidth * this->MinimapTextureHeight * 4);
	} else
	#endif
	{
		SDL_FillRect(this->MinimapSurface, NULL, SDL_MapRGB(this->MinimapSurface->format, 0, 0, 0));
	}

	int bpp;
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		bpp = 0;
	} else
#endif
	{
		bpp = this->MinimapSurface->format->BytesPerPixel;
	}

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		SDL_LockSurface(this->MinimapSurface);
	}

	for (int my = 0; my < this->MinimapTextureHeight; ++my) {
		for (int mx = 0; mx < this->MinimapTextureWidth; ++mx) {
			int tile_x = mx * 1000 / this->MinimapTileWidth;
			int tile_y = my * 1000 / this->MinimapTileHeight;
#if defined(USE_OPENGL) || defined(USE_GLES)
			if (UseOpenGL) {
				if (GrandStrategyGame.WorldMapTiles[tile_x][tile_y] && GrandStrategyGame.WorldMapTiles[tile_x][tile_y]->Province != -1) {
					int province_id = GrandStrategyGame.WorldMapTiles[tile_x][tile_y]->Province;
					if (GrandStrategyGame.Provinces[province_id]->Owner != NULL) {
						int player_color = PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->Owner->Civilization][GrandStrategyGame.Provinces[province_id]->Owner->Faction]->Colors[0];
						*(Uint32 *)&(this->MinimapSurfaceGL[(mx + my * this->MinimapTextureWidth) * 4]) = Video.MapRGB(TheScreen->format, PlayerColorsRGB[player_color][0]);
					} else if (GrandStrategyGame.Provinces[province_id]->Water) {
						*(Uint32 *)&(this->MinimapSurfaceGL[(mx + my * this->MinimapTextureWidth) * 4]) = Video.MapRGB(TheScreen->format, 171, 198, 217);
					} else {
						*(Uint32 *)&(this->MinimapSurfaceGL[(mx + my * this->MinimapTextureWidth) * 4]) = Video.MapRGB(TheScreen->format, 255, 255, 255);
					}
				} else {
					*(Uint32 *)&(this->MinimapSurfaceGL[(mx + my * this->MinimapTextureWidth) * 4]) = Video.MapRGB(0, 0, 0, 0);
				}
			} else
#endif
			{
				const int index = mx * bpp + my * this->MinimapSurface->pitch;
				if (GrandStrategyGame.WorldMapTiles[tile_x][tile_y] && GrandStrategyGame.WorldMapTiles[tile_x][tile_y]->Province != -1) {
					int province_id = GrandStrategyGame.WorldMapTiles[tile_x][tile_y]->Province;
					if (GrandStrategyGame.Provinces[province_id]->Owner != NULL) {
						int player_color = PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->Owner->Civilization][GrandStrategyGame.Provinces[province_id]->Owner->Faction]->Colors[0];
						if (bpp == 2) {
							*(Uint16 *)&((Uint8 *)this->MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, PlayerColorsRGB[player_color][0]);
						} else {
							*(Uint32 *)&((Uint8 *)this->MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, PlayerColorsRGB[player_color][0]);
						}
					} else if (GrandStrategyGame.Provinces[province_id]->Water) {
						if (bpp == 2) {
							*(Uint16 *)&((Uint8 *)this->MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, 171, 198, 217);
						} else {
							*(Uint32 *)&((Uint8 *)this->MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, 171, 198, 217);
						}
					} else {
						if (bpp == 2) {
							*(Uint16 *)&((Uint8 *)this->MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, 255, 255, 255);
						} else {
							*(Uint32 *)&((Uint8 *)this->MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, 255, 255, 255);
						}
					}
				} else {
					if (bpp == 2) {
						*(Uint16 *)&((Uint8 *)this->MinimapSurface->pixels)[index] = ColorBlack;
					} else {
						*(Uint32 *)&((Uint8 *)this->MinimapSurface->pixels)[index] = ColorBlack;
					}
				}
			}
		}
	}

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		SDL_UnlockSurface(this->MinimapSurface);
	}
}

void WorldMapTile::UpdateMinimap()
{
	if (!(
		(GetWorldMapWidth() <= UI.Minimap.X && GetWorldMapHeight() <= UI.Minimap.Y)
		|| (
			(this->Position.x % std::max(1000 / GrandStrategyGame.MinimapTileWidth, 1)) == 0
			&& (this->Position.y % std::max(1000 / GrandStrategyGame.MinimapTileHeight, 1)) == 0
		)
	)) {
		return;
	}
	
	int bpp;
	#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		bpp = 0;
	} else
	#endif
	{
		bpp = GrandStrategyGame.MinimapSurface->format->BytesPerPixel;
	}

	#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
	#endif
	{
		SDL_LockSurface(GrandStrategyGame.MinimapSurface);
	}

	int minimap_tile_width = std::max(GrandStrategyGame.MinimapTileWidth / 1000, 1);
	int minimap_tile_height = std::max(GrandStrategyGame.MinimapTileHeight / 1000, 1);
	for (int sub_x = 0; sub_x < minimap_tile_width; ++sub_x) {
		for (int sub_y = 0; sub_y < minimap_tile_height; ++sub_y) {
			int mx = (this->Position.x * GrandStrategyGame.MinimapTileWidth / 1000) + sub_x;
			int my = (this->Position.y * GrandStrategyGame.MinimapTileHeight / 1000) + sub_y;
#if defined(USE_OPENGL) || defined(USE_GLES)
			if (UseOpenGL) {
				if (this->Province != -1) {
					int province_id = this->Province;
					if (GrandStrategyGame.Provinces[province_id]->Owner != NULL) {
						int player_color = PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->Owner->Civilization][GrandStrategyGame.Provinces[province_id]->Owner->Faction]->Colors[0];
						*(Uint32 *)&(GrandStrategyGame.MinimapSurfaceGL[(mx + my * GrandStrategyGame.MinimapTextureWidth) * 4]) = Video.MapRGB(TheScreen->format, PlayerColorsRGB[player_color][0]);
					} else if (GrandStrategyGame.Provinces[province_id]->Water) {
						*(Uint32 *)&(GrandStrategyGame.MinimapSurfaceGL[(mx + my * GrandStrategyGame.MinimapTextureWidth) * 4]) = Video.MapRGB(TheScreen->format, 171, 198, 217);
					} else {
						*(Uint32 *)&(GrandStrategyGame.MinimapSurfaceGL[(mx + my * GrandStrategyGame.MinimapTextureWidth) * 4]) = Video.MapRGB(TheScreen->format, 255, 255, 255);
					}
				} else {
					*(Uint32 *)&(GrandStrategyGame.MinimapSurfaceGL[(mx + my * GrandStrategyGame.MinimapTextureWidth) * 4]) = Video.MapRGB(0, 0, 0, 0);
				}
			} else
#endif
			{
				const int index = mx * bpp + my * GrandStrategyGame.MinimapSurface->pitch;
				if (this->Province != -1) {
					int province_id = this->Province;
					if (GrandStrategyGame.Provinces[province_id]->Owner != NULL) {
						int player_color = PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->Owner->Civilization][GrandStrategyGame.Provinces[province_id]->Owner->Faction]->Colors[0];
						if (bpp == 2) {
							*(Uint16 *)&((Uint8 *)GrandStrategyGame.MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, PlayerColorsRGB[player_color][0]);
						} else {
							*(Uint32 *)&((Uint8 *)GrandStrategyGame.MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, PlayerColorsRGB[player_color][0]);
						}
					} else if (GrandStrategyGame.Provinces[province_id]->Water) {
						if (bpp == 2) {
							*(Uint16 *)&((Uint8 *)GrandStrategyGame.MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, 171, 198, 217);
						} else {
							*(Uint32 *)&((Uint8 *)GrandStrategyGame.MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, 171, 198, 217);
						}
					} else {
						if (bpp == 2) {
							*(Uint16 *)&((Uint8 *)GrandStrategyGame.MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, 255, 255, 255);
						} else {
							*(Uint32 *)&((Uint8 *)GrandStrategyGame.MinimapSurface->pixels)[index] = Video.MapRGB(TheScreen->format, 255, 255, 255);
						}
					}
				} else {
					if (bpp == 2) {
						*(Uint16 *)&((Uint8 *)GrandStrategyGame.MinimapSurface->pixels)[index] = ColorBlack;
					} else {
						*(Uint32 *)&((Uint8 *)GrandStrategyGame.MinimapSurface->pixels)[index] = ColorBlack;
					}
				}
			}
		}
	}

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		SDL_UnlockSurface(GrandStrategyGame.MinimapSurface);
	}
}

void WorldMapTile::SetResourceProspected(int resource_id, bool discovered)
{
	if (this->ResourceProspected == discovered) { //no change, return
		return;
	}
	
	if (resource_id != -1 && this->Resource == resource_id) {
		this->ResourceProspected = discovered;
		
		if (this->Province != -1) {
			if (this->ResourceProspected) {
				GrandStrategyGame.Provinces[this->Province]->ProductionCapacity[resource_id] += 1;
				GrandStrategyGame.Provinces[this->Province]->AllocateLabor(); //allocate labor so that the 
			} else {
				GrandStrategyGame.Provinces[this->Province]->ProductionCapacity[resource_id] -= 1;
				GrandStrategyGame.Provinces[this->Province]->ReallocateLabor();
			}
		}
	}
}

void WorldMapTile::SetPort(bool has_port)
{
	if (this->Port == has_port) {
		return;
	}
	
	this->Port = has_port;
	
	//if the tile is the same as the province's settlement location, create a dock for the province's settlement, if its civilization has one
	if (this->Province != -1 && GrandStrategyGame.Provinces[this->Province]->SettlementLocation == this->Position) {
		int civilization = GrandStrategyGame.Provinces[this->Province]->Civilization;
		if (civilization != -1) {
			int building_type = GrandStrategyGame.Provinces[this->Province]->GetClassUnitType(GetUnitTypeClassIndexByName("dock"));
			if (building_type != -1) {
				GrandStrategyGame.Provinces[this->Province]->SetSettlementBuilding(building_type, has_port);
			}
		}
	}
}

bool WorldMapTile::IsWater()
{
	if (this->Terrain != -1) {
		return GrandStrategyGame.TerrainTypes[this->Terrain]->Water;
	}
	return false;
}

/**
**  Get whether the tile has a resource
*/
bool WorldMapTile::HasResource(int resource, bool ignore_prospection)
{
	if (resource == this->Resource && (this->ResourceProspected || ignore_prospection)) {
		return true;
	}
	return false;
}

/**
**  Get the tile's cultural name.
*/
std::string WorldMapTile::GetCulturalName()
{
	if (this->Province != -1 && !GrandStrategyGame.Provinces[this->Province]->Water && GrandStrategyGame.Provinces[this->Province]->Civilization != -1 && !this->CulturalNames[GrandStrategyGame.Provinces[this->Province]->Civilization].empty()) {
		return this->CulturalNames[GrandStrategyGame.Provinces[this->Province]->Civilization];
	} else if (
		this->Province != -1
		&& GrandStrategyGame.Provinces[this->Province]->Water && GrandStrategyGame.Provinces[this->Province]->ReferenceProvince != -1
		&& GrandStrategyGame.Provinces[GrandStrategyGame.Provinces[this->Province]->ReferenceProvince]->Civilization != -1
		&& !this->CulturalNames[GrandStrategyGame.Provinces[GrandStrategyGame.Provinces[this->Province]->ReferenceProvince]->Civilization].empty()
	) {
		return this->CulturalNames[GrandStrategyGame.Provinces[GrandStrategyGame.Provinces[this->Province]->ReferenceProvince]->Civilization];
	} else {
		return this->Name;
	}
}

/**
**  Get a river's cultural name.
*/
std::string CRiver::GetCulturalName(int civilization)
{
	if (civilization != -1 && !this->CulturalNames[civilization].empty()) {
		return this->CulturalNames[civilization];
	} else {
		return this->Name;
	}
}

void CProvince::UpdateMinimap()
{
	for (size_t i = 0; i < this->Tiles.size(); ++i) {
		int x = this->Tiles[i].x;
		int y = this->Tiles[i].y;
		if (GrandStrategyGame.WorldMapTiles[x][y]) {
			GrandStrategyGame.WorldMapTiles[x][y]->UpdateMinimap();
		}
	}
}

void CProvince::SetOwner(int civilization_id, int faction_id)
{
	//if new owner is the same as the current owner, return
	if (
		(this->Owner != NULL && this->Owner->Civilization == civilization_id && this->Owner->Faction == faction_id)
		|| (this->Owner == NULL && civilization_id == -1 && faction_id == -1)
	) {
		return;
	}
	
	if (this->Owner != NULL) { //if province has a previous owner, remove it from the owner's province list
		for (int i = 0; i < this->Owner->ProvinceCount; ++i) {
			if (this->Owner->OwnedProvinces[i] == this->ID) {
				//if this owned province is the one we are changing the owner of, push every element of the array after it back one step
				for (int j = i; j < this->Owner->ProvinceCount; ++j) {
					this->Owner->OwnedProvinces[j] = this->Owner->OwnedProvinces[j + 1];
				}
				break;
			}
		}
		this->Owner->ProvinceCount -= 1;
		
		//also remove its resource incomes from the owner's incomes, and reset the province's income so it won't be deduced from the new owner's income when recalculating it
		this->DeallocateLabor();
		for (int i = 0; i < MaxCosts; ++i) {
			if (this->Income[i] != 0) {
				this->Owner->Income[i] -= this->Income[i];
				this->Income[i] = 0;
			}
		}
		
		//set the province's faction-specific units back to the corresponding units of the province's civilization
		if (this->Owner->Civilization == this->Civilization) {
			for (size_t i = 0; i < UnitTypes.size(); ++i) { //change the province's military score to be appropriate for the new faction's technologies
				if (
					IsGrandStrategyUnit(*UnitTypes[i])
					&& !UnitTypes[i]->Class.empty()
					&& !UnitTypes[i]->Civilization.empty()
					&& !UnitTypes[i]->Faction.empty()
					&& PlayerRaces.GetFactionClassUnitType(this->Owner->Civilization, this->Owner->Faction, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) == i
					&& PlayerRaces.GetCivilizationClassUnitType(this->Civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) != -1
					&& PlayerRaces.GetCivilizationClassUnitType(this->Civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) != PlayerRaces.GetFactionClassUnitType(this->Owner->Civilization, this->Owner->Faction, GetUnitTypeClassIndexByName(UnitTypes[i]->Class))
				) {
					this->ChangeUnitQuantity(PlayerRaces.GetCivilizationClassUnitType(this->Civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)), this->Units[i]);
					this->UnderConstructionUnits[PlayerRaces.GetCivilizationClassUnitType(this->Civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class))] += this->UnderConstructionUnits[i];
					this->SetUnitQuantity(i, 0);
					this->UnderConstructionUnits[i] = 0;
				}
			}
		}
	}
	
	for (size_t i = 0; i < UnitTypes.size(); ++i) { //change the province's military score to be appropriate for the new faction's technologies
		if (IsMilitaryUnit(*UnitTypes[i])) {
			int old_owner_military_score_bonus = (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[i] : 0);
			int new_owner_military_score_bonus = (faction_id != -1 ? GrandStrategyGame.Factions[civilization_id][faction_id]->MilitaryScoreBonus[i] : 0);
			if (old_owner_military_score_bonus != new_owner_military_score_bonus) {
				this->MilitaryScore += this->Units[i] * (new_owner_military_score_bonus - old_owner_military_score_bonus);
				this->OffensiveMilitaryScore += this->Units[i] * new_owner_military_score_bonus - old_owner_military_score_bonus;
			}
		} else if (UnitTypes[i]->Class == "worker") {
			int militia_unit_type = PlayerRaces.GetCivilizationClassUnitType(PlayerRaces.GetRaceIndexByName(UnitTypes[i]->Civilization.c_str()), GetUnitTypeClassIndexByName("militia"));
			if (militia_unit_type != -1) {
				int old_owner_military_score_bonus = (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[militia_unit_type] : 0);
				int new_owner_military_score_bonus = (faction_id != -1 ? GrandStrategyGame.Factions[civilization_id][faction_id]->MilitaryScoreBonus[militia_unit_type] : 0);
				if (old_owner_military_score_bonus != new_owner_military_score_bonus) {
					this->MilitaryScore += this->Units[i] * ((new_owner_military_score_bonus - old_owner_military_score_bonus) / 2);
				}
			}
		}
	}

	if (civilization_id != -1 && faction_id != -1) {
		this->Owner = const_cast<CGrandStrategyFaction *>(&(*GrandStrategyGame.Factions[civilization_id][faction_id]));
		this->Owner->OwnedProvinces[this->Owner->ProvinceCount] = this->ID;
		this->Owner->ProvinceCount += 1;
		
		if (this->Civilization == -1) { // if province has no civilization/culture defined, then make its culture that of its owner
			this->SetCivilization(this->Owner->Civilization);
			
			//if province's settlement location tile has a port but not the province, create a dock for the province's settlement, if its civilization has one
			if (GrandStrategyGame.WorldMapTiles[this->SettlementLocation.x][this->SettlementLocation.y]->Port) {
				int dock_building_type = this->GetClassUnitType(GetUnitTypeClassIndexByName("dock"));
				if (dock_building_type != -1) {
					this->SetSettlementBuilding(dock_building_type, true);
				}
			}
		}
		
		//set the province's units to their faction-specific equivalents (if any)
		if (this->Owner->Civilization == this->Civilization) {
			for (size_t i = 0; i < UnitTypes.size(); ++i) { //change the province's military score to be appropriate for the new faction's technologies
				if (
					IsGrandStrategyUnit(*UnitTypes[i])
					&& !UnitTypes[i]->Class.empty()
					&& !UnitTypes[i]->Civilization.empty()
					&& UnitTypes[i]->Faction.empty()
					&& PlayerRaces.GetCivilizationClassUnitType(this->Civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) == i
					&& PlayerRaces.GetFactionClassUnitType(this->Owner->Civilization, this->Owner->Faction, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) != -1
					&& PlayerRaces.GetFactionClassUnitType(this->Owner->Civilization, this->Owner->Faction, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) != PlayerRaces.GetCivilizationClassUnitType(this->Civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class))
				) {
					this->ChangeUnitQuantity(PlayerRaces.GetFactionClassUnitType(this->Owner->Civilization, this->Owner->Faction, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)), this->Units[i]);
					this->UnderConstructionUnits[PlayerRaces.GetFactionClassUnitType(this->Owner->Civilization, this->Owner->Faction, GetUnitTypeClassIndexByName(UnitTypes[i]->Class))] += this->UnderConstructionUnits[i];
					this->SetUnitQuantity(i, 0);
					this->UnderConstructionUnits[i] = 0;
				}
			}
		}
		
		// if the province's new owner is a faction that has a language different from that of its civilization, generate a faction-specific name for the province
		if (PlayerRaces.GetFactionLanguage(civilization_id, faction_id) != -1 && PlayerRaces.GetFactionLanguage(civilization_id, faction_id) != PlayerRaces.GetCivilizationLanguage(civilization_id)) {
			if (this->FactionCulturalNames[civilization_id][faction_id].empty()) {
				std::string new_province_name = "";
				// first see if can translate the cultural name of the province's current civilization
				if (this->Civilization != -1 && !this->CulturalNames[this->Civilization].empty()) {
					new_province_name = PlayerRaces.TranslateName(this->CulturalNames[this->Civilization], PlayerRaces.GetFactionLanguage(civilization_id, faction_id));
				}
				if (new_province_name == "") { // try to translate any cultural name
					for (int i = 0; i < MAX_RACES; ++i) {
						if (!this->CulturalNames[i].empty()) {
							new_province_name = PlayerRaces.TranslateName(this->CulturalNames[i], PlayerRaces.GetFactionLanguage(civilization_id, faction_id)); 
							if (!new_province_name.empty()) {
								break;
							}
						}
					}
				}
				if (new_province_name == "") { // if trying to translate all cultural names failed, generate a new name
					new_province_name = this->GenerateProvinceName(civilization_id, faction_id);
				}
				if (!new_province_name.empty()) {
					this->FactionCulturalNames[civilization_id][faction_id] = new_province_name;
				}
			}
			
			if (this->FactionCulturalSettlementNames[civilization_id][faction_id].empty()) {
				std::string new_settlement_name = "";
				// first see if can translate the cultural name of the province's current civilization
				if (this->Civilization != -1 && !this->CulturalSettlementNames[this->Civilization].empty()) {
					new_settlement_name = PlayerRaces.TranslateName(this->CulturalSettlementNames[this->Civilization], PlayerRaces.GetFactionLanguage(civilization_id, faction_id));
				}
				if (new_settlement_name == "") { // try to translate any cultural name
					for (int i = 0; i < MAX_RACES; ++i) {
						if (!this->CulturalSettlementNames[i].empty()) {
							new_settlement_name = PlayerRaces.TranslateName(this->CulturalSettlementNames[i], PlayerRaces.GetFactionLanguage(civilization_id, faction_id)); 
							if (!new_settlement_name.empty()) {
								break;
							}
						}
					}
				}
				if (new_settlement_name == "") { // if trying to translate all cultural names failed, generate a new name
					new_settlement_name = this->GenerateSettlementName(civilization_id, faction_id);
				}
				if (!new_settlement_name.empty()) {
					this->FactionCulturalSettlementNames[civilization_id][faction_id] = new_settlement_name;
				}
			}
		}
	} else {
		this->Owner = NULL;
		this->SetCivilization(-1); //if there is no owner, change the civilization to none
	}
	
	if (this->Owner != NULL && this->Labor > 0 && this->HasBuildingClass("town-hall")) {
		this->AllocateLabor();
	}
	
	this->CalculateIncomes();
}

void CProvince::SetCivilization(int civilization)
{
	int old_civilization = this->Civilization;
	
	this->Civilization = civilization;
	
	if (civilization != -1) {
		// create new cultural names for the province's terrain features, if there aren't any
		for (size_t i = 0; i < this->Tiles.size(); ++i) {
			int x = this->Tiles[i].x;
			int y = this->Tiles[i].y;
			if (GrandStrategyGame.WorldMapTiles[x][y]->CulturalNames[civilization].empty()) {
				std::string new_tile_name = "";
				// first see if can translate the cultural name of the old civilization
				if (old_civilization != -1 && !GrandStrategyGame.WorldMapTiles[x][y]->CulturalNames[old_civilization].empty()) {
					new_tile_name = PlayerRaces.TranslateName(GrandStrategyGame.WorldMapTiles[x][y]->CulturalNames[old_civilization], PlayerRaces.GetCivilizationLanguage(civilization));
				}
				if (new_tile_name == "") { // try to translate any cultural name
					for (int j = 0; j < MAX_RACES; ++j) {
						if (!GrandStrategyGame.WorldMapTiles[x][y]->CulturalNames[j].empty()) {
							new_tile_name = PlayerRaces.TranslateName(GrandStrategyGame.WorldMapTiles[x][y]->CulturalNames[j], PlayerRaces.GetCivilizationLanguage(civilization));
							if (!new_tile_name.empty()) {
								break;
							}
						}
					}
				}
				if (new_tile_name == "") { // if trying to translate all cultural names failed, generate a new name
					new_tile_name = GenerateName(PlayerRaces.GetCivilizationLanguage(civilization), "terrain-" + NameToIdent(GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name));
				}
				if (new_tile_name != "") {
					GrandStrategyGame.WorldMapTiles[x][y]->CulturalNames[civilization] = new_tile_name;
				}
			}
		}
			
		// create a new cultural name for the province's settlement, if there isn't any
		if (this->CulturalSettlementNames[civilization].empty()) {
			std::string new_settlement_name = "";
			// first see if can translate the cultural name of the old civilization
			if (old_civilization != -1 && !this->CulturalSettlementNames[old_civilization].empty()) {
				new_settlement_name = PlayerRaces.TranslateName(this->CulturalSettlementNames[old_civilization], PlayerRaces.GetCivilizationLanguage(civilization));
			}
			if (new_settlement_name == "") { // try to translate any cultural name
				for (int i = 0; i < MAX_RACES; ++i) {
					if (!this->CulturalSettlementNames[i].empty()) {
						new_settlement_name = PlayerRaces.TranslateName(this->CulturalSettlementNames[i], PlayerRaces.GetCivilizationLanguage(civilization));
						if (!new_settlement_name.empty()) {
							break;
						}
					}
				}
			}
			if (new_settlement_name == "") { // if trying to translate all cultural names failed, generate a new name
				new_settlement_name = this->GenerateSettlementName(civilization);
			}
			if (new_settlement_name != "") {
				this->CulturalSettlementNames[civilization] = new_settlement_name;
			}
		}
			
		// create a new cultural name for the province, if there isn't any
		if (this->CulturalNames[civilization].empty()) {
			std::string new_province_name = "";
			// first see if can translate the cultural name of the old civilization
			if (old_civilization != -1 && !this->CulturalNames[old_civilization].empty()) {
				new_province_name = PlayerRaces.TranslateName(this->CulturalNames[old_civilization], PlayerRaces.GetCivilizationLanguage(civilization));
			}
			if (new_province_name == "") { // try to translate any cultural name
				for (int i = 0; i < MAX_RACES; ++i) {
					if (!this->CulturalNames[i].empty()) {
						new_province_name = PlayerRaces.TranslateName(this->CulturalNames[i], PlayerRaces.GetCivilizationLanguage(civilization));
						if (!new_province_name.empty()) {
							break;
						}
					}
				}
			}
			if (new_province_name == "") { // if trying to translate all cultural names failed, generate a new name
				new_province_name = this->GenerateProvinceName(civilization);
			}
			if (new_province_name != "") {
				this->CulturalNames[civilization] = new_province_name;
			}
		}
		
		for (size_t i = 0; i < UnitTypes.size(); ++i) {
			// replace existent buildings from other civilizations with buildings of the new civilization
			if (IsGrandStrategyBuilding(*UnitTypes[i]) && !UnitTypes[i]->Civilization.empty()) {
				if (this->SettlementBuildings[i] && this->GetClassUnitType(GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) != i) {
					this->SetSettlementBuilding(i, false); // remove building from other civilization
					if (this->GetClassUnitType(GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) != -1) {
						this->SetSettlementBuilding(this->GetClassUnitType(GetUnitTypeClassIndexByName(UnitTypes[i]->Class)), true);
					}
				}
			// replace existent units from the previous civilization with units of the new civilization
			} else if (
				IsGrandStrategyUnit(*UnitTypes[i])
				&& !UnitTypes[i]->Class.empty()
				&& !UnitTypes[i]->Civilization.empty()
				&& PlayerRaces.GetCivilizationClassUnitType(old_civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) == i
				&& this->GetClassUnitType(GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) != -1
				&& this->GetClassUnitType(GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) != PlayerRaces.GetCivilizationClassUnitType(old_civilization, GetUnitTypeClassIndexByName(UnitTypes[i]->Class)) // don't replace if both civilizations use the same unit type
			) {
				this->ChangeUnitQuantity(this->GetClassUnitType(GetUnitTypeClassIndexByName(UnitTypes[i]->Class)), this->Units[i]);
				this->UnderConstructionUnits[this->GetClassUnitType(GetUnitTypeClassIndexByName(UnitTypes[i]->Class))] += this->UnderConstructionUnits[i];
				this->SetUnitQuantity(i, 0);
				this->UnderConstructionUnits[i] = 0;
			}
		}
		
		if (old_civilization == -1 && this->TotalWorkers == 0) {
			//if the province had no culture set and thus has no worker, give it one worker
			this->SetUnitQuantity(this->GetClassUnitType(GetUnitTypeClassIndexByName("worker")), 1);
		}
	} else {
		//if province is being set to no culture, remove all workers
		if (old_civilization != -1) {
			this->SetUnitQuantity(PlayerRaces.GetCivilizationClassUnitType(old_civilization, GetUnitTypeClassIndexByName("worker")), 0);
		}
	}
	
	this->CurrentConstruction = -1; // under construction buildings get canceled
	
	this->CalculateIncomes();
}

void CProvince::SetSettlementBuilding(int building_id, bool has_settlement_building)
{
	if (building_id == -1) {
		fprintf(stderr, "Invalid building type being set for the settlement of \"%s\".\n", this->Name.c_str());
		return;
	}
	
	if (this->SettlementBuildings[building_id] == has_settlement_building) {
		return;
	}
	
	this->SettlementBuildings[building_id] = has_settlement_building;
	
	int change = has_settlement_building ? 1 : -1;
	for (int i = 0; i < MaxCosts; ++i) {
		if (UnitTypes[building_id]->GrandStrategyProductionEfficiencyModifier[i] != 0) {
			this->ProductionEfficiencyModifier[i] += UnitTypes[building_id]->GrandStrategyProductionEfficiencyModifier[i] * change;
			if (this->Owner != NULL) {
				this->CalculateIncome(i);
			}
		}
	}
	
	//recalculate the faction incomes if a town hall or a building that provides research was constructed
	if (this->Owner != NULL) {
		if (UnitTypes[building_id]->Class == "town-hall") {
			this->CalculateIncomes();
		} else if (UnitTypes[building_id]->Class == "lumber-mill") {
			this->CalculateIncome(ResearchCost);
		} else if (UnitTypes[building_id]->Class == "smithy") {
			this->CalculateIncome(ResearchCost);
		}
	}
	
	if (UnitTypes[building_id]->Class == "stronghold") { //increase the military score of the province, if this building is a stronghold
		this->MilitaryScore += (100 * 2) * change; // two guard towers if has a stronghold
	} else if (UnitTypes[building_id]->Class == "dock") {
		//place a port in the province's settlement location, if the building is a dock
		GrandStrategyGame.WorldMapTiles[this->SettlementLocation.x][this->SettlementLocation.y]->Port = has_settlement_building;
		
		//allow the province to fish if it has a dock
		this->ProductionCapacity[FishCost] = 0;
		if (has_settlement_building) {
			for (int sub_x = -1; sub_x <= 1; ++sub_x) { //add 1 capacity in fish production for every water tile adjacent to the settlement location
				if ((this->SettlementLocation.x + sub_x) < 0 || (this->SettlementLocation.x + sub_x) >= GrandStrategyGame.WorldMapWidth) {
					continue;
				}
				for (int sub_y = -1; sub_y <= 1; ++sub_y) {
					if ((this->SettlementLocation.y + sub_y) < 0 || (this->SettlementLocation.y + sub_y) >= GrandStrategyGame.WorldMapHeight) {
						continue;
					}
					if (!(sub_x == 0 && sub_y == 0)) {
						if (GrandStrategyGame.WorldMapTiles[this->SettlementLocation.x + sub_x][this->SettlementLocation.y + sub_y]->IsWater()) {
							this->ProductionCapacity[FishCost] += 1;
						}
					}
				}
			}
				
			for (int i = 0; i < MaxDirections; ++i) { //if the settlement location has a river, add one fish production capacity
				if (GrandStrategyGame.WorldMapTiles[this->SettlementLocation.x][this->SettlementLocation.y]->River[i] != -1) {
					this->ProductionCapacity[FishCost] += 1;
					break;
				}
			}
		}
	}

	// allocate labor (in case building a town hall or another building may have allowed a new sort of production)
	if (this->Owner != NULL && this->Labor > 0 && this->HasBuildingClass("town-hall")) {
		this->AllocateLabor();
	}
}

void CProvince::SetUnitQuantity(int unit_type_id, int quantity)
{
	quantity = std::max(0, quantity);
	
	int change = quantity - this->Units[unit_type_id];
	
	this->TotalUnits += change;
	
	if (IsMilitaryUnit(*UnitTypes[unit_type_id])) {
		this->MilitaryScore += change * (UnitTypes[unit_type_id]->DefaultStat.Variables[POINTS_INDEX].Value + (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[unit_type_id] : 0));
		this->OffensiveMilitaryScore += change * (UnitTypes[unit_type_id]->DefaultStat.Variables[POINTS_INDEX].Value + (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[unit_type_id] : 0));
	}
	
	if (UnitTypes[unit_type_id]->Class == "worker") {
		this->TotalWorkers += change;
		
		//if this unit's civilization can change workers into militia, add half of the militia's points to the military score (one in every two workers becomes a militia when the province is attacked)
		int militia_unit_type = PlayerRaces.GetCivilizationClassUnitType(PlayerRaces.GetRaceIndexByName(UnitTypes[unit_type_id]->Civilization.c_str()), GetUnitTypeClassIndexByName("militia"));
		if (militia_unit_type != -1) {
			this->MilitaryScore += change * ((UnitTypes[militia_unit_type]->DefaultStat.Variables[POINTS_INDEX].Value + (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[militia_unit_type] : 0)) / 2);
		}
		
		if (GrandStrategyGameInitialized) {
			int labor_change = change * 100;
			if (labor_change >= 0) {
				this->Labor += labor_change;
				this->AllocateLabor();
			} else { //if workers are being removed from the province, reallocate labor
				this->ReallocateLabor();
			}		
		}
	}
	
	this->Units[unit_type_id] = quantity;
}

void CProvince::ChangeUnitQuantity(int unit_type_id, int quantity)
{
	this->SetUnitQuantity(unit_type_id, this->Units[unit_type_id] + quantity);
}

void CProvince::SetAttackingUnitQuantity(int unit_type_id, int quantity)
{
	quantity = std::max(0, quantity);
	
	int change = quantity - this->AttackingUnits[unit_type_id];
	
	if (IsMilitaryUnit(*UnitTypes[unit_type_id])) {
		this->AttackingMilitaryScore += change * (UnitTypes[unit_type_id]->DefaultStat.Variables[POINTS_INDEX].Value + (this->AttackedBy != NULL ? this->AttackedBy->MilitaryScoreBonus[unit_type_id] : 0));
	}
		
	this->AttackingUnits[unit_type_id] = quantity;
}

void CProvince::ChangeAttackingUnitQuantity(int unit_type_id, int quantity)
{
	this->SetAttackingUnitQuantity(unit_type_id, this->AttackingUnits[unit_type_id] + quantity);
}

void CProvince::SetHero(std::string hero_full_name, int value)
{
	if (value == 1) {
		this->Movement = true;
	}
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		//update the hero
		if (value == 0) {
			hero->Die();
			return;
		}
		if (hero->Province != NULL) {
			if (hero->State == 2) {
				hero->Province->MilitaryScore -= (hero->Type->DefaultStat.Variables[POINTS_INDEX].Value + (hero->Province->Owner != NULL ? hero->Province->Owner->MilitaryScoreBonus[hero->Type->Slot] : 0));
			} else if (hero->State == 3) {
				hero->Province->AttackingMilitaryScore -= (hero->Type->DefaultStat.Variables[POINTS_INDEX].Value + (hero->Province->AttackedBy != NULL ? hero->Province->AttackedBy->MilitaryScoreBonus[hero->Type->Slot] : 0));
			}
		}
		hero->State = value;
			
		if (this != hero->Province || value == 0) { //if the new province is different from the hero's current province
			if (hero->Province != NULL) {
				hero->Province->Heroes.erase(std::remove(hero->Province->Heroes.begin(), hero->Province->Heroes.end(), hero), hero->Province->Heroes.end());  //remove the hero from the last province
			}
			hero->Province = value != 0 ? const_cast<CProvince *>(&(*this)) : NULL;
			if (hero->Province != NULL) {
				hero->Province->Heroes.push_back(hero); //add the hero to the new province
			}
		}
	} else {
		//if the hero hasn't been defined yet, give an error message
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	
	
	if (value == 2) {
		this->MilitaryScore += (hero->Type->DefaultStat.Variables[POINTS_INDEX].Value + (this->Owner != NULL ? this->Owner->MilitaryScoreBonus[hero->Type->Slot] : 0));
	} else if (value == 3) {
		this->AttackingMilitaryScore += (hero->Type->DefaultStat.Variables[POINTS_INDEX].Value + (this->AttackedBy != NULL ? this->AttackedBy->MilitaryScoreBonus[hero->Type->Slot] : 0));
	}
}
		
void CProvince::AllocateLabor()
{
	if (this->Owner == NULL || !this->HasBuildingClass("town-hall")) { //no production if no town hall is in place, or if the province has no owner
		return;
	}
	
	if (this->Labor == 0) { //if there's no labor, nothing to allocate
		return;
	}
	
	//first, try to allocate as many workers as possible in food production, to increase the population, then allocate workers to gold, and then to other goods
//	std::vector<int> resources_by_priority = {GrainCost, MushroomCost, FishCost, GoldCost, WoodCost, StoneCost};
	std::vector<int> resources_by_priority;
	resources_by_priority.push_back(GrainCost);
	resources_by_priority.push_back(MushroomCost);
	resources_by_priority.push_back(FishCost);
	resources_by_priority.push_back(GoldCost);
	resources_by_priority.push_back(SilverCost);
	resources_by_priority.push_back(WoodCost);
	resources_by_priority.push_back(StoneCost);
	resources_by_priority.push_back(CopperCost);

	for (size_t i = 0; i < resources_by_priority.size(); ++i) {
		this->AllocateLaborToResource(resources_by_priority[i]);
		if (this->Labor == 0) { //labor depleted
			return;
		}
	}
}

void CProvince::AllocateLaborToResource(int resource)
{
	if (this->Owner == NULL || !this->HasBuildingClass("town-hall")) { //no production if no town hall is in place, or if the province has no owner
		return;
	}
	
	if (this->Labor == 0) { //if there's no labor, nothing to allocate
		return;
	}
	
	if (this->ProductionCapacity[resource] > this->ProductionCapacityFulfilled[resource] && this->Labor >= DefaultResourceLaborInputs[resource]) {
		int employment_change = std::min(this->Labor / DefaultResourceLaborInputs[resource], this->ProductionCapacity[resource] - ProductionCapacityFulfilled[resource]);
		this->Labor -= employment_change * DefaultResourceLaborInputs[resource];
		ProductionCapacityFulfilled[resource] += employment_change;
		this->CalculateIncome(resource);
		
		//set new worked tiles
		int new_worked_tiles = employment_change;
		for (size_t i = 0; i < this->ResourceTiles[resource].size(); ++i) {
			int x = this->ResourceTiles[resource][i].x;
			int y = this->ResourceTiles[resource][i].y;
			if (GrandStrategyGame.WorldMapTiles[x][y] && GrandStrategyGame.WorldMapTiles[x][y]->Worked == false) {
				GrandStrategyGame.WorldMapTiles[x][y]->Worked = true;
				new_worked_tiles -= 1;
			}
			if (new_worked_tiles <= 0) {
				break;
			}
		}	
	}
	
	//recalculate food consumption (workers employed in producing food don't need to consume food)
	FoodConsumption = this->TotalWorkers * 100;
	FoodConsumption -= (this->ProductionCapacityFulfilled[GrainCost] * DefaultResourceLaborInputs[GrainCost]);
	FoodConsumption -= (this->ProductionCapacityFulfilled[MushroomCost] * DefaultResourceLaborInputs[MushroomCost]);
	FoodConsumption -= (this->ProductionCapacityFulfilled[FishCost] * DefaultResourceLaborInputs[FishCost]);
}

void CProvince::DeallocateLabor()
{
	for (int i = 0; i < MaxCosts; ++i) {
		this->ProductionCapacityFulfilled[i] = 0;
		
		for (size_t j = 0; j < this->ResourceTiles[i].size(); ++j) {
			int x = this->ResourceTiles[i][j].x;
			int y = this->ResourceTiles[i][j].y;
			if (GrandStrategyGame.WorldMapTiles[x][y]) {
				GrandStrategyGame.WorldMapTiles[x][y]->Worked = false;
			}
		}	
	}
	this->Labor = this->TotalWorkers * 100;
}

void CProvince::ReallocateLabor()
{
	this->DeallocateLabor();
	this->AllocateLabor();
}

void CProvince::CalculateIncome(int resource)
{
	if (resource == -1) {
		return;
	}
	
	if (this->Owner == NULL || !this->HasBuildingClass("town-hall")) { //don't produce resources if no town hall is in place
		this->Income[resource] = 0;
		return;
	}
	
	this->Owner->Income[resource] -= this->Income[resource]; //first, remove the old income from the owner's income
	if (resource == SilverCost) { //silver and copper are converted to gold
		this->Owner->Income[GoldCost] -= this->Income[resource] / 2;
	} else if (resource == CopperCost) {
		this->Owner->Income[GoldCost] -= this->Income[resource] / 4;
	}
	
	int income = 0;
	
	if (resource == ResearchCost) {
		// faction's research is 10 if all provinces have town halls, lumber mills and smithies
		if (this->HasBuildingClass("town-hall")) {
			income += 6;
		}
		if (this->HasBuildingClass("lumber-mill")) {
			income += 2;
		}
		if (this->HasBuildingClass("smithy")) {
			income += 2;
		}
			
		income *= 100 + this->Owner->ProductionEfficiencyModifier[resource] + this->ProductionEfficiencyModifier[resource] + this->GetAdministrativeEfficiencyModifier();
		income /= 100;
	} else {
		if (this->ProductionCapacityFulfilled[resource] > 0) {
			income = DefaultResourceOutputs[resource] * this->ProductionCapacityFulfilled[resource];
			int production_modifier = this->Owner->ProductionEfficiencyModifier[resource] + this->ProductionEfficiencyModifier[resource];
			if (resource != GrainCost && resource != MushroomCost && resource != FishCost) { //food resources don't lose production efficiency if administrative efficiency is lower than 100%, to prevent provinces from starving when conquered
				production_modifier += this->GetAdministrativeEfficiencyModifier();
			}
			income *= 100 + production_modifier;
			income /= 100;
		}
	}
	
	this->Income[resource] = income;
	
	this->Owner->Income[resource] += this->Income[resource]; //add the new income to the owner's income
	if (resource == SilverCost) { //silver and copper are converted to gold
		this->Owner->Income[GoldCost] += this->Income[resource] / 2;
	} else if (resource == CopperCost) {
		this->Owner->Income[GoldCost] += this->Income[resource] / 4;
	}
}

void CProvince::CalculateIncomes()
{
	for (int i = 0; i < MaxCosts; ++i) {
		this->CalculateIncome(i);
	}
}

void CProvince::AddFactionClaim(int civilization_id, int faction_id)
{
	this->Claims.push_back(GrandStrategyGame.Factions[civilization_id][faction_id]);
	GrandStrategyGame.Factions[civilization_id][faction_id]->Claims.push_back(this);
}

void CProvince::RemoveFactionClaim(int civilization_id, int faction_id)
{
	this->Claims.erase(std::remove(this->Claims.begin(), this->Claims.end(), GrandStrategyGame.Factions[civilization_id][faction_id]), this->Claims.end());
	GrandStrategyGame.Factions[civilization_id][faction_id]->Claims.erase(std::remove(GrandStrategyGame.Factions[civilization_id][faction_id]->Claims.begin(), GrandStrategyGame.Factions[civilization_id][faction_id]->Claims.end(), this), GrandStrategyGame.Factions[civilization_id][faction_id]->Claims.end());
}

bool CProvince::HasBuildingClass(std::string building_class_name)
{
	if (this->Civilization == -1 || building_class_name.empty()) {
		return false;
	}
	
	int building_type = this->GetClassUnitType(GetUnitTypeClassIndexByName(building_class_name));
	
	if (building_type == -1 && building_class_name == "mercenary-camp") { //special case for mercenary camps, which are a neutral building
		building_type = UnitTypeIdByIdent("unit-mercenary-camp");
	}
	
	if (building_type != -1 && this->SettlementBuildings[building_type] == true) {
		return true;
	}

	return false;
}

bool CProvince::HasFactionClaim(int civilization_id, int faction_id)
{
	for (size_t i = 0; i < this->Claims.size(); ++i) {
		if (this->Claims[i]->Civilization == civilization_id && this->Claims[i]->Faction == faction_id) {
			return true;
		}
	}
	return false;
}

bool CProvince::HasResource(int resource, bool ignore_prospection)
{
	for (size_t i = 0; i < this->Tiles.size(); ++i) {
		int x = this->Tiles[i].x;
		int y = this->Tiles[i].y;
		if (GrandStrategyGame.WorldMapTiles[x][y] && GrandStrategyGame.WorldMapTiles[x][y]->HasResource(resource, ignore_prospection)) {
			return true;
		}
	}
	return false;
}

bool CProvince::BordersProvince(int province_id)
{
	for (int i = 0; i < ProvinceMax; ++i) {
		if (this->BorderProvinces[i] != -1) {
			if (this->BorderProvinces[i] == province_id) {
				return true;
			}
		} else {
			break;
		}
	}
	return false;
}

bool CProvince::BordersFaction(int faction_civilization, int faction)
{
	for (int i = 0; i < ProvinceMax; ++i) {
		if (this->BorderProvinces[i] != -1) {
			if (GrandStrategyGame.Provinces[this->BorderProvinces[i]]->Owner == NULL) {
				continue;
			}
			if (GrandStrategyGame.Provinces[this->BorderProvinces[i]]->Owner->Civilization == faction_civilization && GrandStrategyGame.Provinces[this->BorderProvinces[i]]->Owner->Faction == faction) {
				return true;
			}
		} else {
			break;
		}
	}
	return false;
}

int CProvince::GetPopulation()
{
	return (this->TotalWorkers * 10000) * 2;
}

int CProvince::GetResourceDemand(int resource)
{
	int quantity = 0;
	if (resource == WoodCost) {
		quantity = 50;
		if (this->HasBuildingClass("lumber-mill")) {
			quantity += 50; // increase the province's lumber demand if it has a lumber mill built
		}
	} else if (resource == StoneCost) {
		quantity = 25;
	}
	
	if (quantity > 0 && GrandStrategyGame.CommodityPrices[resource] > 0) {
		quantity *= DefaultResourcePrices[resource];
		quantity /= GrandStrategyGame.CommodityPrices[resource];
	}

	return quantity;
}

int CProvince::GetAdministrativeEfficiencyModifier()
{
	int modifier = 0;
	
	if (this->Civilization != -1 && this->Owner != NULL) {
		modifier += this->Civilization == this->Owner->Civilization ? 0 : -25; //if the province is of a different culture than its owner, it gets a cultural penalty to its administrative efficiency modifier
	}
	
	
	if (this->Owner != NULL && this->Owner->Ruler != NULL) {
		modifier += this->Owner->Ruler->GetAdministrativeEfficiencyModifier();
	}
	
	return modifier;
}

int CProvince::GetRevoltRisk()
{
	int revolt_risk = 0;
	
	if (this->Civilization != -1 && this->Owner != NULL) {
		if (this->Civilization != this->Owner->Civilization) {
			revolt_risk += 2; //if the province is of a different culture than its owner, it gets plus 2% revolt risk
		}
		
		if (!this->HasFactionClaim(this->Owner->Civilization, this->Owner->Faction)) {
			revolt_risk += 1; //if the owner does not have a claim to the province, it gets plus 1% revolt risk
		}
	}
	
	return revolt_risk;
}

int CProvince::GetClassUnitType(int class_id)
{
	if (this->Owner != NULL && this->Civilization == this->Owner->Civilization) {
		return PlayerRaces.GetFactionClassUnitType(this->Owner->Civilization, this->Owner->Faction, class_id);
	} else {
		return PlayerRaces.GetCivilizationClassUnitType(this->Civilization, class_id);
	}
}

int CProvince::GetFoodCapacity(bool subtract_non_food)
{
	int food_capacity = 0;
	food_capacity += this->ProductionCapacity[GrainCost];
	food_capacity += this->ProductionCapacity[MushroomCost];
	food_capacity += this->ProductionCapacity[FishCost];
	
	if (subtract_non_food) {
		food_capacity -= this->ProductionCapacity[GoldCost];
		food_capacity -= this->ProductionCapacity[SilverCost];
		food_capacity -= this->ProductionCapacity[CopperCost];
		food_capacity -= this->ProductionCapacity[WoodCost];
		food_capacity -= this->ProductionCapacity[StoneCost];
	}
	
	return food_capacity;
}

/**
**  Get the province's cultural name.
*/
std::string CProvince::GetCulturalName()
{
	if (this->Owner != NULL && !this->Water && !this->FactionCulturalNames[this->Owner->Civilization][this->Owner->Faction].empty() && this->Civilization == this->Owner->Civilization) {
		return this->FactionCulturalNames[this->Owner->Civilization][this->Owner->Faction];
	} else if (!this->Water && this->Civilization != -1 && !this->CulturalNames[this->Civilization].empty()) {
		return this->CulturalNames[this->Civilization];
	} else if (
		this->Water && this->ReferenceProvince != -1
		&& GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner != NULL
		&& !GrandStrategyGame.Provinces[this->ReferenceProvince]->Water
		&& !this->FactionCulturalNames[GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner->Civilization][GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner->Faction].empty()
		&& GrandStrategyGame.Provinces[this->ReferenceProvince]->Civilization == GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner->Civilization
	) {
		return this->FactionCulturalNames[GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner->Civilization][GrandStrategyGame.Provinces[this->ReferenceProvince]->Owner->Faction];
	} else if (
		this->Water && this->ReferenceProvince != -1
		&& GrandStrategyGame.Provinces[this->ReferenceProvince]->Civilization != -1
		&& !this->CulturalNames[GrandStrategyGame.Provinces[this->ReferenceProvince]->Civilization].empty()
	) {
		return this->CulturalNames[GrandStrategyGame.Provinces[this->ReferenceProvince]->Civilization];
	} else {
		return this->Name;
	}
}

/**
**  Get the province's cultural settlement name.
*/
std::string CProvince::GetCulturalSettlementName()
{
	if (!this->Water && this->Owner != NULL && !this->FactionCulturalSettlementNames[this->Owner->Civilization][this->Owner->Faction].empty() && this->Civilization == this->Owner->Civilization) {
		return this->FactionCulturalSettlementNames[this->Owner->Civilization][this->Owner->Faction];
	} else if (!this->Water && this->Civilization != -1 && !this->CulturalSettlementNames[this->Civilization].empty()) {
		return this->CulturalSettlementNames[this->Civilization];
	} else {
		return this->SettlementName;
	}
}

/**
**  Generate a province name for the civilization.
*/
std::string CProvince::GenerateProvinceName(int civilization, int faction)
{
	//10% chance that the province will be named after its settlement
	if (civilization != -1 && faction != -1 && !this->FactionCulturalSettlementNames[civilization][faction].empty() && SyncRand(100) < 10) {
		return this->FactionCulturalSettlementNames[civilization][faction];
	} else if (civilization != -1 && faction == -1 && !this->CulturalSettlementNames[civilization].empty() && SyncRand(100) < 10) {
		return this->CulturalSettlementNames[civilization];
	}
	
	if (faction != -1) {
		return GenerateName(PlayerRaces.GetFactionLanguage(civilization, faction), "province");
	} else {
		return GenerateName(PlayerRaces.GetCivilizationLanguage(civilization), "province");
	}
}

/**
**  Generate a settlement name for the civilization.
**
**  @param l  Lua state.
*/
std::string CProvince::GenerateSettlementName(int civilization, int faction)
{
	//10% chance that the settlement will be named after a named terrain feature in its tile, if there is any
	if (civilization != -1 && faction == -1 && this->SettlementLocation.x != -1 && this->SettlementLocation.y != -1 && !GrandStrategyGame.WorldMapTiles[this->SettlementLocation.x][this->SettlementLocation.y]->CulturalNames[civilization].empty() && SyncRand(100) < 10) {
		return GrandStrategyGame.WorldMapTiles[this->SettlementLocation.x][this->SettlementLocation.y]->CulturalNames[civilization];
	}
	
	if (faction != -1) {
		return GenerateName(PlayerRaces.GetFactionLanguage(civilization, faction), "settlement");
	} else {
		return GenerateName(PlayerRaces.GetCivilizationLanguage(civilization), "settlement");
	}
}

void CGrandStrategyFaction::SetTechnology(int upgrade_id, bool has_technology, bool secondary_setting)
{
	if (this->Technologies[upgrade_id] == has_technology) {
		return;
	}
	
	this->Technologies[upgrade_id] = has_technology;
	
	int change = has_technology ? 1 : -1;
		
	//add military score bonuses
	for (int z = 0; z < NumUpgradeModifiers; ++z) {
		if (UpgradeModifiers[z]->UpgradeId == upgrade_id) {
			for (size_t i = 0; i < UnitTypes.size(); ++i) {
				
				Assert(UpgradeModifiers[z]->ApplyTo[i] == '?' || UpgradeModifiers[z]->ApplyTo[i] == 'X');

				if (UpgradeModifiers[z]->ApplyTo[i] == 'X') {
					if (UpgradeModifiers[z]->Modifier.Variables[POINTS_INDEX].Value) {
						this->MilitaryScoreBonus[i] += UpgradeModifiers[z]->Modifier.Variables[POINTS_INDEX].Value * change;
					}
				}
			}
		}
	}
	
	if (!secondary_setting) { //if this technology is not being set as a result of another technology of the same class being researched
		if (has_technology) { //if value is true, mark technologies from other civilizations that are of the same class as researched too, so that the player doesn't need to research the same type of technology every time
			if (!AllUpgrades[upgrade_id]->Class.empty()) {
				for (size_t i = 0; i < AllUpgrades.size(); ++i) {
					if (AllUpgrades[upgrade_id]->Class == AllUpgrades[i]->Class) {
						this->SetTechnology(i, has_technology, true);
					}
				}
			}
		}
		
		for (int i = 0; i < MaxCosts; ++i) {
			if (AllUpgrades[upgrade_id]->GrandStrategyProductionEfficiencyModifier[i] != 0) {
				this->ProductionEfficiencyModifier[i] += AllUpgrades[upgrade_id]->GrandStrategyProductionEfficiencyModifier[i] * change;
				this->CalculateIncome(i);
			}
		}
	}
}

void CGrandStrategyFaction::CalculateIncome(int resource)
{
	if (resource == -1) {
		return;
	}
	
	if (this->ProvinceCount == 0) {
		this->Income[resource] = 0;
		return;
	}
	
	for (int i = 0; i < this->ProvinceCount; ++i) {
		int province_id = this->OwnedProvinces[i];
		GrandStrategyGame.Provinces[province_id]->CalculateIncome(resource);
	}
}

void CGrandStrategyFaction::CalculateIncomes()
{
	for (int i = 0; i < MaxCosts; ++i) {
		this->CalculateIncome(i);
	}
}

void CGrandStrategyFaction::CalculateUpkeep()
{
	this->Upkeep = 0;
	
	if (this->ProvinceCount == 0) {
		return;
	}
	
	for (int i = 0; i < this->ProvinceCount; ++i) {
		int province_id = this->OwnedProvinces[i];
		for (size_t j = 0; j < UnitTypes.size(); ++j) {
			if (GrandStrategyGame.Provinces[province_id]->Units[j] > 0 && UnitTypes[j]->Upkeep > 0) {
				this->Upkeep += GrandStrategyGame.Provinces[province_id]->Units[j] * UnitTypes[j]->Upkeep;
			}
		}
	}
}

void CGrandStrategyFaction::CheckFormableFactions(int civilization)
{
	for (size_t i = 0; i < PlayerRaces.Factions[this->Civilization][this->Faction]->DevelopsTo.size(); ++i) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, PlayerRaces.Factions[this->Civilization][this->Faction]->DevelopsTo[i]);
		if (faction != -1 && GrandStrategyGame.Factions[civilization][faction]) {
			if (GrandStrategyGame.Factions[civilization][faction] != this && !GrandStrategyGame.Factions[civilization][faction]->IsAlive()) {
				if (CanFormFaction(civilization, faction)) {
					this->FormFaction(civilization, faction);
				}
			}
		}
	}
}

void CGrandStrategyFaction::FormFaction(int civilization, int faction)
{
	int old_civilization = this->Civilization;
	int old_faction = this->Faction;
	
	int new_civilization = civilization;
	int new_faction = faction;
	
	GrandStrategyGame.Factions[new_civilization][new_faction]->AcquireFactionTechnologies(old_civilization, old_faction);
	
	//set the ruler from the old faction
	if (GrandStrategyGame.Factions[old_civilization][old_faction]->Ruler != NULL) {
		GrandStrategyGame.Factions[new_civilization][new_faction]->SetRuler(GrandStrategyGame.Factions[old_civilization][old_faction]->Ruler->GetFullName());
	}

	
	if (this->ProvinceCount > 0) {
		for (int i = (this->ProvinceCount - 1); i >= 0; --i) {
			int province_id = this->OwnedProvinces[i];

			//GrandStrategyGame.Provinces[province_id]->SetOwner(new_civilization, new_faction);
			char buf[256];
			snprintf(
				buf, sizeof(buf), "AcquireProvince(GetProvinceFromName(\"%s\"), \"%s\");",
				(GrandStrategyGame.Provinces[province_id]->Name).c_str(),
				(PlayerRaces.Factions[new_civilization][new_faction]->Name).c_str()
			);
			CclCommand(buf);
			
			// replace existing units from the previous civilization with units of the new civilization, if the civilizations are different
			if (old_civilization != new_civilization) {
				for (size_t j = 0; j < UnitTypes.size(); ++j) {
					if (
						!UnitTypes[j]->Class.empty()
						&& !UnitTypes[j]->Civilization.empty()
						&& !UnitTypes[j]->BoolFlag[BUILDING_INDEX].value
						&& UnitTypes[j]->DefaultStat.Variables[DEMAND_INDEX].Value > 0
						&& UnitTypes[j]->Civilization == PlayerRaces.Name[old_civilization]
						&& PlayerRaces.GetCivilizationClassUnitType(new_civilization, GetUnitTypeClassIndexByName(UnitTypes[j]->Class)) != -1
						&& PlayerRaces.GetCivilizationClassUnitType(new_civilization, GetUnitTypeClassIndexByName(UnitTypes[j]->Class)) != PlayerRaces.GetCivilizationClassUnitType(old_civilization, GetUnitTypeClassIndexByName(UnitTypes[j]->Class)) // don't replace if both civilizations use the same unit type
					) {
						GrandStrategyGame.Provinces[province_id]->ChangeUnitQuantity(PlayerRaces.GetCivilizationClassUnitType(new_civilization, GetUnitTypeClassIndexByName(UnitTypes[j]->Class)), GrandStrategyGame.Provinces[province_id]->Units[j]);
						GrandStrategyGame.Provinces[province_id]->UnderConstructionUnits[PlayerRaces.GetCivilizationClassUnitType(new_civilization, GetUnitTypeClassIndexByName(UnitTypes[j]->Class))] += GrandStrategyGame.Provinces[province_id]->UnderConstructionUnits[j];
						GrandStrategyGame.Provinces[province_id]->SetUnitQuantity(j, 0);
						GrandStrategyGame.Provinces[province_id]->UnderConstructionUnits[j] = 0;
					}
				}
			}
		}
	}
	
	for (int i = 0; i < MaxCosts; ++i) {
		GrandStrategyGame.Factions[new_civilization][new_faction]->Resources[i] = this->Resources[i];
	}
	
	GrandStrategyGame.Factions[new_civilization][new_faction]->CurrentResearch = GrandStrategyGame.Factions[old_civilization][old_faction]->CurrentResearch;

	for (size_t i = 0; i < this->Claims.size(); ++i) { // the new faction gets the claims of the old one
		this->Claims[i]->AddFactionClaim(new_civilization, new_faction);
	}

	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (GrandStrategyGame.Factions[i][j]) {
				GrandStrategyGame.Factions[old_civilization][old_faction]->DiplomacyState[i][j] = DiplomacyStatePeace;
				GrandStrategyGame.Factions[i][j]->DiplomacyState[old_civilization][old_faction] = DiplomacyStatePeace;
				GrandStrategyGame.Factions[old_civilization][old_faction]->DiplomacyStateProposal[i][j] = -1;
				GrandStrategyGame.Factions[i][j]->DiplomacyStateProposal[old_civilization][old_faction] = -1;
			}
		}
	}
	
	GrandStrategyGame.Factions[old_civilization][old_faction]->CalculateIncomes();
	GrandStrategyGame.Factions[new_civilization][new_faction]->CalculateIncomes();

	//if the faction is civilizing, grant 10 prestige
	if (PlayerRaces.Factions[old_civilization][old_faction]->Type == "tribe" && PlayerRaces.Factions[new_civilization][new_faction]->Type == "polity") {
		GrandStrategyGame.Factions[new_civilization][new_faction]->Resources[PrestigeCost] += 10;
	}
		
	if (this == GrandStrategyGame.PlayerFaction) {
		GrandStrategyGame.PlayerFaction = const_cast<CGrandStrategyFaction *>(&(*GrandStrategyGame.Factions[new_civilization][new_faction]));
		
		char buf[256];
		snprintf(
			buf, sizeof(buf), "GrandStrategyFaction = GetFactionFromName(\"%s\");",
			(PlayerRaces.Factions[new_civilization][new_faction]->Name).c_str()
		);
		CclCommand(buf);
		
		std::string dialog_tooltip = "Our faction becomes the " + GrandStrategyGame.Factions[new_civilization][new_faction]->GetFullName();
		if (PlayerRaces.Factions[old_civilization][old_faction]->Type == "tribe" && PlayerRaces.Factions[new_civilization][new_faction]->Type == "polity") {
			dialog_tooltip += ", +10 Prestige";
		}
		std::string dialog_text;
		if (PlayerRaces.Factions[new_civilization][new_faction]->Type == "polity") {
			dialog_text = "From the halls of our capital the formation of a new realm has been declared, the ";
		} else if (PlayerRaces.Factions[new_civilization][new_faction]->Type == "tribe") {
			dialog_text = "Our council of elders has declared the formation of a new tribe, the ";
		}
		dialog_text += GrandStrategyGame.Factions[new_civilization][new_faction]->GetFullName() + "!";
		char buf_2[256];
		snprintf(
			buf_2, sizeof(buf_2), "if (GenericDialog ~= nil) then GenericDialog(\"%s\", \"%s\", \"%s\") end;",
			("The " + GrandStrategyGame.Factions[new_civilization][new_faction]->GetFullName()).c_str(),
			dialog_text.c_str(),
			dialog_tooltip.c_str()
		);
		CclCommand(buf_2);
	}
	
	if (GrandStrategyGame.Factions[old_civilization][old_faction]->Ruler != NULL) {
		GrandStrategyGame.Factions[old_civilization][old_faction]->SetRuler(""); //do this after changing the PlayerFaction to prevent ruler death/rise to power messages, since the ruler is the same
	}
}

void CGrandStrategyFaction::AcquireFactionTechnologies(int civilization, int faction)
{
	for (size_t i = 0; i < AllUpgrades.size(); ++i) {
		if (GrandStrategyGame.Factions[civilization][faction]->Technologies[i] && AllUpgrades[i]->Ident != PlayerRaces.Factions[civilization][faction]->FactionUpgrade) { //don't acquire the faction's faction-specific upgrade
			this->SetTechnology(i, true);
		}
	}
}

void CGrandStrategyFaction::SetRuler(std::string hero_full_name)
{
	if (hero_full_name.empty()) {
		if (this->IsAlive() && GrandStrategyGameInitialized) {
			this->RulerSuccession();
		} else {
			this->Ruler = NULL;
		}
	} else {
		CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
		if (hero) {
			if (hero->State == 0) {
				hero->Create();
			}
			this->Ruler = const_cast<CGrandStrategyHero *>(&(*hero));
		} else {
			fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
		}
		
		if (this == GrandStrategyGame.PlayerFaction && GrandStrategyGameInitialized) {
			char buf[256];
			snprintf(
				buf, sizeof(buf), "if (GenericDialog ~= nil) then GenericDialog(\"%s\", \"%s\") end;",
				(this->GetRulerTitle() + " " + this->Ruler->GetFullName()).c_str(),
				("A new " + DecapitalizeString(this->GetRulerTitle()) + " has come to power in our realm, " + this->Ruler->GetFullName() + "!\\n\\n" + this->Ruler->GetRulerEffectsString()).c_str()
			);
			CclCommand(buf);	
		}
	}
	
	this->CalculateIncomes(); //recalculate incomes, as administrative efficiency may have changed
}

void CGrandStrategyFaction::RulerSuccession()
{
	if (
		this->Ruler != NULL
		&& (PlayerRaces.Factions[this->Civilization][this->Faction]->Type == "tribe" || this->GovernmentType == GovernmentTypeMonarchy)
	) { //if is a tribe or a monarchical polity, try to perform ruler succession by descent
		for (size_t i = 0; i < this->Ruler->Children.size(); ++i) {
			if (this->Ruler->Children[i]->State != 0 && this->Ruler->Children[i]->Gender == MaleGender) { //historically males have generally been given priority in throne inheritance (if not exclusivity), specially in the cultures currently playable in the game
				this->SetRuler(this->Ruler->Children[i]->GetFullName());
				return;
			}
		}
		for (size_t i = 0; i < this->Ruler->Siblings.size(); ++i) { // now check for male siblings of the current ruler
			if (this->Ruler->Siblings[i]->State != 0 && this->Ruler->Siblings[i]->Gender == MaleGender) {
				this->SetRuler(this->Ruler->Siblings[i]->GetFullName());
				return;
			}
		}		
		for (size_t i = 0; i < this->Ruler->Children.size(); ++i) { //check again for children, but now allow for inheritance regardless of gender
			if (this->Ruler->Children[i]->State != 0) {
				this->SetRuler(this->Ruler->Children[i]->GetFullName());
				return;
			}
		}
		for (size_t i = 0; i < this->Ruler->Siblings.size(); ++i) { //check again for siblings, but now allow for inheritance regardless of gender
			if (this->Ruler->Siblings[i]->State != 0) {
				this->SetRuler(this->Ruler->Siblings[i]->GetFullName());
				return;
			}
		}		
	}
	
	std::vector<CGrandStrategyHero *> ruler_candidates;
	for (size_t i = 0; i < GrandStrategyGame.Heroes.size(); ++i) {
		if (
			GrandStrategyGame.Heroes[i]->State != 0
			&& GrandStrategyGame.Heroes[i]->Gender == MaleGender
			&& (
				(GrandStrategyGame.Heroes[i]->Province != NULL && GrandStrategyGame.Heroes[i]->Province->Owner == this)
				|| (GrandStrategyGame.Heroes[i]->Province == NULL && GrandStrategyGame.Heroes[i]->ProvinceOfOrigin != NULL && GrandStrategyGame.Heroes[i]->ProvinceOfOrigin->Owner == this)
			)
			&& !GrandStrategyGame.Heroes[i]->Custom
		) {
			ruler_candidates.push_back(GrandStrategyGame.Heroes[i]);
		}
	}
	if (ruler_candidates.size() == 0) {
		//if the list of male ruler candidates is empty, see if there are heroes available without taking gender in regard
		for (size_t i = 0; i < GrandStrategyGame.Heroes.size(); ++i) {
			if (
				GrandStrategyGame.Heroes[i]->State != 0
				&& (
					(GrandStrategyGame.Heroes[i]->Province != NULL && GrandStrategyGame.Heroes[i]->Province->Owner == this)
					|| (GrandStrategyGame.Heroes[i]->Province == NULL && GrandStrategyGame.Heroes[i]->ProvinceOfOrigin != NULL && GrandStrategyGame.Heroes[i]->ProvinceOfOrigin->Owner == this)
				)
				&& !GrandStrategyGame.Heroes[i]->Custom
			) {
				ruler_candidates.push_back(GrandStrategyGame.Heroes[i]);
			}
		}
	}
	if (ruler_candidates.size() > 0) {
		this->SetRuler(ruler_candidates[SyncRand(ruler_candidates.size())]->GetFullName());
		return;
	}
		
	this->GenerateRuler(); //if all else failed, try to generate a ruler for the faction
}

void CGrandStrategyFaction::GenerateRuler()
{
	this->Ruler = NULL;
	std::vector<int> potential_ruler_unit_types;
	if (PlayerRaces.Factions[this->Civilization][this->Faction]->Type == "tribe" || this->GovernmentType != GovernmentTypeTheocracy) { //exclude priests from ruling non-theocracies
		if (PlayerRaces.GetFactionClassUnitType(this->Civilization, this->Faction, GetUnitTypeClassIndexByName("heroic-infantry")) != -1) {
			potential_ruler_unit_types.push_back(PlayerRaces.GetFactionClassUnitType(this->Civilization, this->Faction, GetUnitTypeClassIndexByName("heroic-infantry")));
		} else if (PlayerRaces.GetFactionClassUnitType(this->Civilization, this->Faction, GetUnitTypeClassIndexByName("veteran-infantry")) != -1) {
			potential_ruler_unit_types.push_back(PlayerRaces.GetFactionClassUnitType(this->Civilization, this->Faction, GetUnitTypeClassIndexByName("veteran-infantry")));
		} else if (PlayerRaces.GetFactionClassUnitType(this->Civilization, this->Faction, GetUnitTypeClassIndexByName("infantry")) != -1) {
			potential_ruler_unit_types.push_back(PlayerRaces.GetFactionClassUnitType(this->Civilization, this->Faction, GetUnitTypeClassIndexByName("infantry")));
		}
		if (PlayerRaces.GetFactionClassUnitType(this->Civilization, this->Faction, GetUnitTypeClassIndexByName("shooter")) != -1) {
			potential_ruler_unit_types.push_back(PlayerRaces.GetFactionClassUnitType(this->Civilization, this->Faction, GetUnitTypeClassIndexByName("shooter")));
		}
		if (PlayerRaces.GetFactionClassUnitType(this->Civilization, this->Faction, GetUnitTypeClassIndexByName("cavalry")) != -1) {
			potential_ruler_unit_types.push_back(PlayerRaces.GetFactionClassUnitType(this->Civilization, this->Faction, GetUnitTypeClassIndexByName("cavalry")));
		}
		if (PlayerRaces.GetFactionClassUnitType(this->Civilization, this->Faction, GetUnitTypeClassIndexByName("flying-rider")) != -1) {
			potential_ruler_unit_types.push_back(PlayerRaces.GetFactionClassUnitType(this->Civilization, this->Faction, GetUnitTypeClassIndexByName("flying-rider")));
		}
	} else { //only allow priests to rule theocracies
		if (PlayerRaces.GetFactionClassUnitType(this->Civilization, this->Faction, GetUnitTypeClassIndexByName("priest")) != -1) {
			potential_ruler_unit_types.push_back(PlayerRaces.GetFactionClassUnitType(this->Civilization, this->Faction, GetUnitTypeClassIndexByName("priest")));
		}
	}
	
	int unit_type_id;
	if (potential_ruler_unit_types.size() > 0) {
		unit_type_id = potential_ruler_unit_types[SyncRand(potential_ruler_unit_types.size())];
	} else {
		return;
	}
	
	int civilization = PlayerRaces.GetRaceIndexByName(UnitTypes[unit_type_id]->Civilization.c_str()); //use unit type's civilization, so that names can be generated even for civilizations for which we don't have personal name language data defined
	int language = PlayerRaces.GetCivilizationLanguage(civilization);
	if (civilization != -1 && this->Civilization == civilization && this->Faction != -1) {
		language = PlayerRaces.GetFactionLanguage(civilization, this->Faction);
	}
	std::string hero_name = GeneratePersonalName(language, unit_type_id);
	
	if (hero_name.empty()) { //if civilization can't generate personal names, return
		return;
	}
	
	std::string hero_extra_name;
	if (GrandStrategyGame.GetHero(hero_name) != NULL) { // generate extra given names if this name is already used by an existing hero
		hero_extra_name = GeneratePersonalName(language, unit_type_id);
		while (GrandStrategyGame.GetHero(hero_name + " " + hero_extra_name) != NULL) {
			hero_extra_name += " " + GeneratePersonalName(language, unit_type_id);
		}
	}
	CGrandStrategyHero *hero = new CGrandStrategyHero;
	GrandStrategyGame.Heroes.push_back(hero);
	hero->Name = hero_name;
	hero->ExtraName = hero_extra_name;
	hero->State = 2;
	hero->Type = const_cast<CUnitType *>(&(*UnitTypes[unit_type_id]));
	if (hero->Type->Traits.size() > 0) { //generate a trait
		hero->Trait = const_cast<CUpgrade *>(&(*hero->Type->Traits[SyncRand(hero->Type->Traits.size())]));
	}
	hero->Year = GrandStrategyYear;
	hero->DeathYear = GrandStrategyYear + (SyncRand(45) + 1); //average + 30 years after initially appearing
	hero->Civilization = this->Civilization;
	if (this->ProvinceCount == 0) {
		fprintf(stderr, "Faction \"%s\" is generating a ruler, but has no provinces.\n", PlayerRaces.Factions[this->Civilization][this->Faction]->Name.c_str());
	}
	hero->ProvinceOfOrigin = const_cast<CProvince *>(&(*GrandStrategyGame.Provinces[this->OwnedProvinces[SyncRand(this->ProvinceCount)]]));
	hero->ProvinceOfOriginName = hero->ProvinceOfOrigin->Name;
	hero->Gender = MaleGender;
	GrandStrategyHeroStringToIndex[hero->GetFullName()] = GrandStrategyGame.Heroes.size() - 1;
	this->SetRuler(hero->GetFullName());
}

bool CGrandStrategyFaction::IsAlive()
{
	return this->ProvinceCount > 0;
}

bool CGrandStrategyFaction::HasTechnologyClass(std::string technology_class_name)
{
	if (this->Civilization == -1 || technology_class_name.empty()) {
		return false;
	}
	
	int technology_id = PlayerRaces.GetFactionClassUpgrade(this->Civilization, this->Faction, GetUpgradeClassIndexByName(technology_class_name));
	
	if (technology_id != -1 && this->Technologies[technology_id] == true) {
		return true;
	}

	return false;
}

bool CGrandStrategyFaction::CanFormFaction(int civilization, int faction)
{
	bool civilized = this->HasTechnologyClass("writing") && this->HasTechnologyClass("masonry");
	
	if ((PlayerRaces.Factions[civilization][faction]->Type == "polity") != civilized) {
		return false;
	}
	
	if (this->FactionTier > GrandStrategyGame.Factions[civilization][faction]->FactionTier && !(PlayerRaces.Factions[this->Civilization][this->Faction]->Type == "tribe" && civilized)) { //only form factions of the same tier or higher, unless is civilizing
		return false;
	}
	
	//check if owns the majority of the formable faction's claims
	if (GrandStrategyGame.Factions[civilization][faction]->Claims.size() > 0) {
		size_t owned_claims = 0;
		for (size_t i = 0; i < GrandStrategyGame.Factions[civilization][faction]->Claims.size(); ++i) {
			if (GrandStrategyGame.Factions[civilization][faction]->Claims[i]->Owner == this) {
				owned_claims += 1;
			}
		}
		
		if (owned_claims <= (GrandStrategyGame.Factions[civilization][faction]->Claims.size() / 2)) {
			return false;
		}
	}
	
	return true;
}

std::string CGrandStrategyFaction::GetFullName()
{
	if (PlayerRaces.Factions[this->Civilization][this->Faction]->Type == "tribe") {
		return PlayerRaces.Factions[this->Civilization][this->Faction]->Name;
	} else if (PlayerRaces.Factions[this->Civilization][this->Faction]->Type == "polity") {
		return this->GetTitle() + " of " + PlayerRaces.Factions[this->Civilization][this->Faction]->Name;
	}
	
	return "";
}

std::string CGrandStrategyFaction::GetTitle()
{
	std::string faction_title;
	
	if (PlayerRaces.Factions[this->Civilization][this->Faction]->Type == "polity") {
		if (!PlayerRaces.Factions[this->Civilization][this->Faction]->Titles[this->GovernmentType][this->FactionTier].empty()) {
			faction_title = PlayerRaces.Factions[this->Civilization][this->Faction]->Titles[this->GovernmentType][this->FactionTier];
		} else {
			if (this->GovernmentType == GovernmentTypeMonarchy) {
				if (this->FactionTier == FactionTierBarony) {
					faction_title = "Barony";
				} else if (this->FactionTier == FactionTierCounty) {
					faction_title = "County";
				} else if (this->FactionTier == FactionTierDuchy) {
					faction_title = "Duchy";
				} else if (this->FactionTier == FactionTierGrandDuchy) {
					faction_title = "Grand Duchy";
				} else if (this->FactionTier == FactionTierKingdom) {
					faction_title = "Kingdom";
				} else if (this->FactionTier == FactionTierEmpire) {
					faction_title = "Empire";
				}
			} else if (this->GovernmentType == GovernmentTypeRepublic) {
				faction_title = "Republic";
			} else if (this->GovernmentType == GovernmentTypeTheocracy) {
				faction_title = "Theocracy";
			}
		}
	}
	
	return faction_title;
}

std::string CGrandStrategyFaction::GetRulerTitle()
{
	if (this->Ruler == NULL) {
		return "";
	}
	
	if (PlayerRaces.Factions[this->Civilization][this->Faction]->Type == "tribe") {
		if (this->Ruler->Gender != FemaleGender) {
			return "Chieftain";
		} else {
			return "Chieftess";
		}
	} else if (PlayerRaces.Factions[this->Civilization][this->Faction]->Type == "polity") {
		std::string faction_title = this->GetTitle();
		
		if (faction_title == "Barony") {
			if (this->Ruler->Gender != FemaleGender) {
				return "Baron";
			} else {
				return "Baroness";
			}
		} else if (faction_title == "Lordship") {
			if (this->Ruler->Gender != FemaleGender) {
				return "Lord";
			} else {
				return "Lady";
			}
		} else if (faction_title == "County") {
			if (this->Ruler->Gender != FemaleGender) {
				return "Count";
			} else {
				return "Countess";
			}
		} else if (faction_title == "City-State") {
			return "Archon";
		} else if (faction_title == "Duchy") {
			if (this->Ruler->Gender != FemaleGender) {
				return "Duke";
			} else {
				return "Duchess";
			}
		} else if (faction_title == "Principality") {
			if (this->Ruler->Gender != FemaleGender) {
				return "Prince";
			} else {
				return "Princess";
			}
		} else if (faction_title == "Margraviate") {
			return "Margrave";
		} else if (faction_title == "Landgraviate") {
			return "Landgrave";
		} else if (faction_title == "Grand Duchy") {
			if (this->Ruler->Gender != FemaleGender) {
				return "Grand Duke";
			} else {
				return "Grand Duchess";
			}
		} else if (faction_title == "Archduchy") {
			if (this->Ruler->Gender != FemaleGender) {
				return "Archduke";
			} else {
				return "Archduchess";
			}
		} else if (faction_title == "Kingdom") {
			if (this->Ruler->Gender != FemaleGender) {
				return "King";
			} else {
				return "Queen";
			}
		} else if (faction_title == "Khanate") {
			return "Khan";
		} else if (faction_title == "Empire") {
			if (this->Ruler->Gender != FemaleGender) {
				return "Emperor";
			} else {
				return "Empress";
			}
		} else if (faction_title == "Republic") {
			return "Consul";
		} else if (faction_title == "Confederation") {
			return "Chancellor";
		} else if (faction_title == "Theocracy") {
			if (this->Ruler->Gender != FemaleGender) {
				return "High Priest";
			} else {
				return "High Priestess";
			}
		} else if (faction_title == "Bishopric") {
			return "Bishop";
		} else if (faction_title == "Archbishopric") {
			return "Archbishop";
		}
	}
	
	return "";
}

void CGrandStrategyHero::Initialize()
{
	if (this->Trait == NULL) { //if no trait was set, have the hero be the same trait as the unit type (if the unit type has it predefined)
		if (this->Type != NULL && this->Type->Traits.size() > 0) {
			this->Trait = const_cast<CUpgrade *>(&(*this->Type->Traits[SyncRand(this->Type->Traits.size())]));
		}
	}
	int province_of_origin_id;
	if (!this->Custom) {
		province_of_origin_id = GetProvinceId(this->ProvinceOfOriginName);
	} else {
		province_of_origin_id = GrandStrategyGame.PlayerFaction->OwnedProvinces[0];
	}
	
	this->ProvinceOfOrigin = const_cast<CProvince *>(&(*GrandStrategyGame.Provinces[province_of_origin_id]));
	
	if (!this->Icon.Name.empty()) {
		this->Icon.Load();
	}
	
	if (!this->HeroicIcon.Name.empty()) {
		this->HeroicIcon.Load();
	}
}

void CGrandStrategyHero::Create()
{
	//show message that the hero has appeared
	if (
		this->ProvinceOfOrigin != NULL
		&& this->ProvinceOfOrigin->Owner != NULL
		&& this->ProvinceOfOrigin->Owner == GrandStrategyGame.PlayerFaction
		&& !this->Icon.Name.empty()
		&& GrandStrategyGameInitialized
	) {
		char buf[256];
		snprintf(
			buf, sizeof(buf), "if (GenericDialog ~= nil) then GenericDialog(\"%s\", \"%s\") end;",
			(this->Type->Name + " " + this->GetFullName()).c_str(),
			("My lord, the hero " + this->GetFullName() + " has come to renown in " + this->ProvinceOfOrigin->GetCulturalName() + " and has entered our service.").c_str()
		);
		CclCommand(buf);	
	}
	
	this->State = 2;
	
	if (this->ProvinceOfOrigin != NULL && (!this->Icon.Name.empty() || this->Custom)) { //if the hero has its own icon or is a custom hero
		this->ProvinceOfOrigin->SetHero(this->GetFullName(), 2);
	}
}

void CGrandStrategyHero::Die()
{
	//show message that the hero has died
	if (GrandStrategyGameInitialized) {
		if (GrandStrategyGame.PlayerFaction != NULL && GrandStrategyGame.PlayerFaction->Ruler == this) {
			char buf[256];
			snprintf(
				buf, sizeof(buf), "if (GenericDialog ~= nil) then GenericDialog(\"%s\", \"%s\") end;",
				(GrandStrategyGame.PlayerFaction->GetRulerTitle() + " " + this->GetFullName() + " Dies").c_str(),
				("Tragic news spread throughout our realm. Our " + DecapitalizeString(GrandStrategyGame.PlayerFaction->GetRulerTitle()) + ", " + this->GetFullName() + ", has died! May his soul rest in peace.").c_str()
			);
			CclCommand(buf);	
		} else if (
			this->Province != NULL
			&& (
				((this->State == 1 || this->State == 2) && this->Province->Owner == GrandStrategyGame.PlayerFaction)
				|| (this->State == 3 && this->Province->AttackedBy == GrandStrategyGame.PlayerFaction)
			)
		) {
			char buf[256];
			snprintf(
				buf, sizeof(buf), "if (GenericDialog ~= nil) then GenericDialog(\"%s\", \"%s\") end;",
				(this->GetFullName() + " Dies").c_str(),
				("My lord, the hero " + this->GetFullName() + " has died!").c_str()
			);
			CclCommand(buf);	
		}
	}
	
	if (this->Province != NULL) {
		if (this->State == 2) {
			this->Province->MilitaryScore -= (this->Type->DefaultStat.Variables[POINTS_INDEX].Value + (this->Province->Owner != NULL ? this->Province->Owner->MilitaryScoreBonus[this->Type->Slot] : 0));
		} else if (this->State == 3) {
			this->Province->AttackingMilitaryScore -= (this->Type->DefaultStat.Variables[POINTS_INDEX].Value + (this->Province->AttackedBy != NULL ? this->Province->AttackedBy->MilitaryScoreBonus[this->Type->Slot] : 0));
		}
		
		this->Province->Heroes.erase(std::remove(this->Province->Heroes.begin(), this->Province->Heroes.end(), this), this->Province->Heroes.end());  //remove the hero from its province
	}
	
	this->Province = NULL;
	
	this->State = 0;

	//check if the hero is the ruler of a faction, and if so, remove it from that position
	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (GrandStrategyGame.Factions[i][j]) {
				if (GrandStrategyGame.Factions[i][j]->Ruler == this) {
					GrandStrategyGame.Factions[i][j]->SetRuler("");
				}
			} else {
				break;
			}
		}
	}
}

void CGrandStrategyHero::SetType(int unit_type_id)
{
	if (this->Province != NULL) {
		if (this->State == 2) {
			this->Province->MilitaryScore -= (this->Type->DefaultStat.Variables[POINTS_INDEX].Value + (this->Province->Owner != NULL ? this->Province->Owner->MilitaryScoreBonus[this->Type->Slot] : 0));
		} else if (this->State == 3) {
			this->Province->AttackingMilitaryScore -= (this->Type->DefaultStat.Variables[POINTS_INDEX].Value + (this->Province->AttackedBy != NULL ? this->Province->AttackedBy->MilitaryScoreBonus[this->Type->Slot] : 0));
		}
	}
	
	//if the hero's unit type changed
	if (unit_type_id != this->Type->Slot) {
		this->Type = const_cast<CUnitType *>(&(*UnitTypes[unit_type_id]));
	}	
	
	if (this->Province != NULL) {
		if (this->State == 2) {
			this->Province->MilitaryScore += (this->Type->DefaultStat.Variables[POINTS_INDEX].Value + (this->Province->Owner != NULL ? this->Province->Owner->MilitaryScoreBonus[this->Type->Slot] : 0));
		} else if (this->State == 3) {
			this->Province->AttackingMilitaryScore += (this->Type->DefaultStat.Variables[POINTS_INDEX].Value + (this->Province->AttackedBy != NULL ? this->Province->AttackedBy->MilitaryScoreBonus[this->Type->Slot] : 0));
		}
	}
}

int CGrandStrategyHero::GetAdministrativeEfficiencyModifier()
{
	int modifier = 0;
	
	if (this->Type != NULL) {
		int intelligence = this->Type->DefaultStat.Variables[INTELLIGENCE_INDEX].Value;
		if (intelligence == 10) {
			intelligence += 1; //grant +1 intelligence to heroes with 10 intelligence, to prevent them from having no bonus
		}
		modifier += (intelligence - 10) * 5; //+2.5% administrative efficiency for every intelligence point above 10, and -2.5% for every point below 10; changed that to 5 for now since heroes don't get intelligence scores which are particularly high
	}
	
	return modifier;
}

std::string CGrandStrategyHero::GetRulerEffectsString()
{
	std::string ruler_effects_string;
	
	int administrative_modifier = this->GetAdministrativeEfficiencyModifier();
	if (administrative_modifier != 0) {
		if (administrative_modifier > 0) {
			ruler_effects_string += "+";
		}
		ruler_effects_string += std::to_string((long long) administrative_modifier) + "% Administrative Efficiency";
	}
	
	return ruler_effects_string;
}

/**
**  Get the width of the world map.
*/
int GetWorldMapWidth()
{
	return GrandStrategyGame.WorldMapWidth;
}

/**
**  Get the height of the world map.
*/
int GetWorldMapHeight()
{
	return GrandStrategyGame.WorldMapHeight;
}

/**
**  Get the terrain type of a world map tile.
*/
std::string GetWorldMapTileTerrain(int x, int y)
{
	
	clamp(&x, 0, GrandStrategyGame.WorldMapWidth - 1);
	clamp(&y, 0, GrandStrategyGame.WorldMapHeight - 1);

	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	if (GrandStrategyGame.WorldMapTiles[x][y]->Terrain == -1) {
		return "";
	}
	
	return GrandStrategyGame.TerrainTypes[GrandStrategyGame.WorldMapTiles[x][y]->Terrain]->Name;
}

/**
**  Get the terrain variation of a world map tile.
*/
int GetWorldMapTileTerrainVariation(int x, int y)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	Assert(GrandStrategyGame.WorldMapTiles[x][y]->Terrain != -1);
	Assert(GrandStrategyGame.WorldMapTiles[x][y]->Variation != -1);
	
	return GrandStrategyGame.WorldMapTiles[x][y]->Variation + 1;
}

std::string GetWorldMapTileProvinceName(int x, int y)
{
	
	clamp(&x, 0, GrandStrategyGame.WorldMapWidth - 1);
	clamp(&y, 0, GrandStrategyGame.WorldMapHeight - 1);

	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	if (GrandStrategyGame.WorldMapTiles[x][y]->Province != -1) {
		return GrandStrategyGame.Provinces[GrandStrategyGame.WorldMapTiles[x][y]->Province]->Name;
	} else {
		return "";
	}
}

bool WorldMapTileHasResource(int x, int y, std::string resource_name, bool ignore_prospection)
{
	clamp(&x, 0, GrandStrategyGame.WorldMapWidth - 1);
	clamp(&y, 0, GrandStrategyGame.WorldMapHeight - 1);

	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	if (resource_name == "any") {
		return GrandStrategyGame.WorldMapTiles[x][y]->Resource != -1 && (GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected || ignore_prospection);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return false;
	}
	
	return GrandStrategyGame.WorldMapTiles[x][y]->HasResource(resource, ignore_prospection);
}

/**
**  Get the ID of a world map terrain type
*/
int GetWorldMapTerrainTypeId(std::string terrain_type_name)
{
	for (int i = 0; i < WorldMapTerrainTypeMax; ++i) {
		if (!GrandStrategyGame.TerrainTypes[i]) {
			break;
		}
		
		if (GrandStrategyGame.TerrainTypes[i]->Name == terrain_type_name) {
			return i;
		}
	}
	
	if (!terrain_type_name.empty()) {
		fprintf(stderr, "Can't find terrain type \"%s\".\n", terrain_type_name.c_str());
	}
	
	return -1;
}

/**
**  Get the ID of a province
*/
int GetProvinceId(std::string province_name)
{
	if (!province_name.empty()) {
		for (int i = 0; i < ProvinceMax; ++i) {
			if (!GrandStrategyGame.Provinces[i]) {
				break;
			}
			
			if (!GrandStrategyGame.Provinces[i]->Name.empty() && GrandStrategyGame.Provinces[i]->Name == province_name) {
				return i;
			}
		}
	
//		fprintf(stderr, "Can't find %s province.\n", province_name.c_str());
	}
	
	return -1;
}

/**
**  Set the size of the world map.
*/
void SetWorldMapSize(int width, int height)
{
	Assert(width <= WorldMapWidthMax);
	Assert(height <= WorldMapHeightMax);
	GrandStrategyGame.WorldMapWidth = width;
	GrandStrategyGame.WorldMapHeight = height;
	
	//create new world map tile objects for the size, if necessary
	if (!GrandStrategyGame.WorldMapTiles[width - 1][height - 1]) {
		for (int x = 0; x < GrandStrategyGame.WorldMapWidth; ++x) {
			for (int y = 0; y < GrandStrategyGame.WorldMapHeight; ++y) {
				if (!GrandStrategyGame.WorldMapTiles[x][y]) {
					WorldMapTile *world_map_tile = new WorldMapTile;
					GrandStrategyGame.WorldMapTiles[x][y] = world_map_tile;
					GrandStrategyGame.WorldMapTiles[x][y]->Position.x = x;
					GrandStrategyGame.WorldMapTiles[x][y]->Position.y = y;
				}
			}
		}
	}
}

/**
**  Set the terrain type of a world map tile.
*/
void SetWorldMapTileTerrain(int x, int y, int terrain)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	//if tile doesn't exist, create it now
	if (!GrandStrategyGame.WorldMapTiles[x][y]) {
		WorldMapTile *world_map_tile = new WorldMapTile;
		GrandStrategyGame.WorldMapTiles[x][y] = world_map_tile;
		GrandStrategyGame.WorldMapTiles[x][y]->Position.x = x;
		GrandStrategyGame.WorldMapTiles[x][y]->Position.y = y;
	}
	
	GrandStrategyGame.WorldMapTiles[x][y]->Terrain = terrain;
	
	if (terrain != -1 && GrandStrategyGame.TerrainTypes[terrain]) {
		//randomly select a variation for the world map tile
		if (GrandStrategyGame.TerrainTypes[terrain]->Variations > 0) {
			GrandStrategyGame.WorldMapTiles[x][y]->Variation = SyncRand(GrandStrategyGame.TerrainTypes[terrain]->Variations);
		} else {
			GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
		}
		
		int base_terrain = GrandStrategyGame.TerrainTypes[terrain]->BaseTile;
		if (base_terrain != -1 && GrandStrategyGame.TerrainTypes[base_terrain]) {
			//randomly select a variation for the world map tile
			if (GrandStrategyGame.TerrainTypes[base_terrain]->Variations > 0) {
				GrandStrategyGame.WorldMapTiles[x][y]->BaseTileVariation = SyncRand(GrandStrategyGame.TerrainTypes[base_terrain]->Variations);
			} else {
				GrandStrategyGame.WorldMapTiles[x][y]->BaseTileVariation = -1;
			}
		}
		
		if (GrandStrategyGame.TerrainTypes[terrain]->Water) { //if is a water terrain, remove already-placed rivers, if any
			for (int i = 0; i < MaxDirections; ++i) {
				GrandStrategyGame.WorldMapTiles[x][y]->River[i] = -1;
			}
		}
	}
}

void SetWorldMapTileProvince(int x, int y, std::string province_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	int old_province_id = GrandStrategyGame.WorldMapTiles[x][y]->Province;
	if (old_province_id != -1) { //if the tile is already assigned to a province, remove it from that province's tile arrays
		GrandStrategyGame.Provinces[old_province_id]->Tiles.erase(std::remove(GrandStrategyGame.Provinces[old_province_id]->Tiles.begin(), GrandStrategyGame.Provinces[old_province_id]->Tiles.end(), Vec2i(x, y)), GrandStrategyGame.Provinces[old_province_id]->Tiles.end());
		
		if (GrandStrategyGame.WorldMapTiles[x][y]->Resource != -1) {
			int res = GrandStrategyGame.WorldMapTiles[x][y]->Resource;
			if (GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected) {
				GrandStrategyGame.Provinces[old_province_id]->ProductionCapacity[res] -= 1;
			}
			GrandStrategyGame.Provinces[old_province_id]->ResourceTiles[res].erase(std::remove(GrandStrategyGame.Provinces[old_province_id]->ResourceTiles[res].begin(), GrandStrategyGame.Provinces[old_province_id]->ResourceTiles[res].end(), Vec2i(x, y)), GrandStrategyGame.Provinces[old_province_id]->ResourceTiles[res].end());
		}
	}

	int province_id = GetProvinceId(province_name);
	GrandStrategyGame.WorldMapTiles[x][y]->Province = province_id;
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		//now add the tile to the province's tile arrays
		GrandStrategyGame.Provinces[province_id]->Tiles.push_back(Vec2i(x, y));
		
		if (GrandStrategyGame.WorldMapTiles[x][y]->Resource != -1) {
			int res = GrandStrategyGame.WorldMapTiles[x][y]->Resource;
			if (GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected) {
				GrandStrategyGame.Provinces[province_id]->ProductionCapacity[res] += 1;
			}
			GrandStrategyGame.Provinces[province_id]->ResourceTiles[res].push_back(Vec2i(x, y));
		}
	}
}

/**
**  Set the name of a world map tile.
*/
void SetWorldMapTileName(int x, int y, std::string name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	GrandStrategyGame.WorldMapTiles[x][y]->Name = name;
}

/**
**  Set the cultural name of a world map tile for a particular civilization.
*/
void SetWorldMapTileCulturalName(int x, int y, std::string civilization_name, std::string cultural_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	if (civilization != -1) {
		GrandStrategyGame.WorldMapTiles[x][y]->CulturalNames[civilization] = TransliterateText(cultural_name);
	}
}

int GetRiverId(std::string river_name)
{
	for (int i = 0; i < RiverMax; ++i) {
		if (!GrandStrategyGame.Rivers[i] || GrandStrategyGame.Rivers[i]->Name.empty()) {
			if (!river_name.empty()) { //if the name is not empty and reached a blank spot, create a new river and return its ID
				if (!GrandStrategyGame.Rivers[i]) { //if river doesn't exist, create it now
					CRiver *river = new CRiver;
					GrandStrategyGame.Rivers[i] = river;
				}
				GrandStrategyGame.Rivers[i]->Name = river_name;
				return i;
			}
			break;
		}
		
		if (!GrandStrategyGame.Rivers[i]->Name.empty() && GrandStrategyGame.Rivers[i]->Name == river_name) {
			return i;
		}
	}
	
	return -1;
}

/**
**  Set river data for a world map tile.
*/
void SetWorldMapTileRiver(int x, int y, std::string direction_name, std::string river_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	int river_id = GetRiverId(river_name);
	
	return; //deactivate this for now, while there aren't proper graphics for the rivers
	
	if (direction_name == "north") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[North] = river_id;
		if (!GrandStrategyGame.WorldMapTiles[x][y]->IsWater()) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->River[Northwest] == -1) {
				GrandStrategyGame.WorldMapTiles[x][y]->River[Northwest] = river_id;
			}
			if (GrandStrategyGame.WorldMapTiles[x][y]->River[Northeast] == -1) {
				GrandStrategyGame.WorldMapTiles[x][y]->River[Northeast] = river_id;
			}
			if (GrandStrategyGame.IsPointOnMap(x + 1, y) && !GrandStrategyGame.WorldMapTiles[x + 1][y]->IsWater() && GrandStrategyGame.WorldMapTiles[x + 1][y]->River[Northwest] == -1) {
				GrandStrategyGame.WorldMapTiles[x + 1][y]->River[Northwest] = river_id;
			}
			if (GrandStrategyGame.IsPointOnMap(x - 1, y) && !GrandStrategyGame.WorldMapTiles[x - 1][y]->IsWater() && GrandStrategyGame.WorldMapTiles[x - 1][y]->River[Northeast] == -1) {
				GrandStrategyGame.WorldMapTiles[x - 1][y]->River[Northeast] = river_id;
			}
		}
	} else if (direction_name == "northeast") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[Northeast] = river_id;
	} else if (direction_name == "east") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[East] = river_id;
		if (!GrandStrategyGame.WorldMapTiles[x][y]->IsWater()) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->River[Northeast] == -1) {
				GrandStrategyGame.WorldMapTiles[x][y]->River[Northeast] = river_id;
			}
			if (GrandStrategyGame.WorldMapTiles[x][y]->River[Southeast] == -1) {
				GrandStrategyGame.WorldMapTiles[x][y]->River[Southeast] = river_id;
			}
			if (GrandStrategyGame.IsPointOnMap(x, y + 1) && !GrandStrategyGame.WorldMapTiles[x][y + 1]->IsWater() && GrandStrategyGame.WorldMapTiles[x][y + 1]->River[Northeast] == -1) {
				GrandStrategyGame.WorldMapTiles[x][y + 1]->River[Northeast] = river_id;
			}
			if (GrandStrategyGame.IsPointOnMap(x, y - 1) && !GrandStrategyGame.WorldMapTiles[x][y - 1]->IsWater() && GrandStrategyGame.WorldMapTiles[x][y - 1]->River[Southeast] == -1) {
				GrandStrategyGame.WorldMapTiles[x][y - 1]->River[Southeast] = river_id;
			}
		}
	} else if (direction_name == "southeast") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[Southeast] = river_id;
	} else if (direction_name == "south") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[South] = river_id;
		if (!GrandStrategyGame.WorldMapTiles[x][y]->IsWater()) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->River[Southwest] == -1) {
				GrandStrategyGame.WorldMapTiles[x][y]->River[Southwest] = river_id;
			}
			if (GrandStrategyGame.WorldMapTiles[x][y]->River[Southeast] == -1) {
				GrandStrategyGame.WorldMapTiles[x][y]->River[Southeast] = river_id;
			}
			if (GrandStrategyGame.IsPointOnMap(x + 1, y) && !GrandStrategyGame.WorldMapTiles[x + 1][y]->IsWater() && GrandStrategyGame.WorldMapTiles[x + 1][y]->River[Southwest] == -1) {
				GrandStrategyGame.WorldMapTiles[x + 1][y]->River[Southwest] = river_id;
			}
			if (GrandStrategyGame.IsPointOnMap(x - 1, y) && !GrandStrategyGame.WorldMapTiles[x - 1][y]->IsWater() && GrandStrategyGame.WorldMapTiles[x - 1][y]->River[Southeast] == -1) {
				GrandStrategyGame.WorldMapTiles[x - 1][y]->River[Southeast] = river_id;
			}
		}
	} else if (direction_name == "southwest") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[Southwest] = river_id;
	} else if (direction_name == "west") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[West] = river_id;
		if (!GrandStrategyGame.WorldMapTiles[x][y]->IsWater()) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->River[Northwest] == -1) {
				GrandStrategyGame.WorldMapTiles[x][y]->River[Northwest] = river_id;
			}
			if (GrandStrategyGame.WorldMapTiles[x][y]->River[Southwest] == -1) {
				GrandStrategyGame.WorldMapTiles[x][y]->River[Southwest] = river_id;
			}
			if (GrandStrategyGame.IsPointOnMap(x, y + 1) && !GrandStrategyGame.WorldMapTiles[x][y + 1]->IsWater() && GrandStrategyGame.WorldMapTiles[x][y + 1]->River[Northwest] == -1) {
				GrandStrategyGame.WorldMapTiles[x][y + 1]->River[Northwest] = river_id;
			}
			if (GrandStrategyGame.IsPointOnMap(x, y - 1) && !GrandStrategyGame.WorldMapTiles[x][y - 1]->IsWater() && GrandStrategyGame.WorldMapTiles[x][y - 1]->River[Southwest] == -1) {
				GrandStrategyGame.WorldMapTiles[x][y - 1]->River[Southwest] = river_id;
			}
		}
	} else if (direction_name == "northwest") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[Northwest] = river_id;
	} else {
		fprintf(stderr, "Error: Wrong direction set for river.\n");
	}
	
}

/**
**  Set riverhead data for a world map tile.
*/
void SetWorldMapTileRiverhead(int x, int y, std::string direction_name, std::string river_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	int river_id = GetRiverId(river_name);
	
	return; //deactivate this for now, while there aren't proper graphics for the rivers
	
	if (direction_name == "north") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[North] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[North] = river_id;
	} else if (direction_name == "northeast") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[Northeast] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[Northeast] = river_id;
	} else if (direction_name == "east") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[East] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[East] = river_id;
	} else if (direction_name == "southeast") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[Southeast] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[Southeast] = river_id;
	} else if (direction_name == "south") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[South] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[South] = river_id;
	} else if (direction_name == "southwest") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[Southwest] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[Southwest] = river_id;
	} else if (direction_name == "west") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[West] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[West] = river_id;
	} else if (direction_name == "northwest") {
		GrandStrategyGame.WorldMapTiles[x][y]->River[Northwest] = river_id;
		GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[Northwest] = river_id;
	} else {
		fprintf(stderr, "Error: Wrong direction set for river.\n");
	}
}

void SetRiverCulturalName(std::string river_name, std::string civilization_name, std::string cultural_name)
{
	int river_id = GetRiverId(river_name);
	
	if (river_id != -1 && GrandStrategyGame.Rivers[river_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			GrandStrategyGame.Rivers[river_id]->CulturalNames[civilization] = TransliterateText(cultural_name);
		}
	}
}

/**
**  Set pathway data for a world map tile.
*/
void SetWorldMapTilePathway(int x, int y, std::string direction_name, std::string pathway_name)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	return; //deactivate this for now, while there aren't proper graphics for the pathways
	
	int direction;
	if (direction_name == "north") {
		direction = North;
	} else if (direction_name == "northeast") {
		direction = Northeast;
	} else if (direction_name == "east") {
		direction = East;
	} else if (direction_name == "southeast") {
		direction = Southeast;
	} else if (direction_name == "south") {
		direction = South;
	} else if (direction_name == "southwest") {
		direction = Southwest;
	} else if (direction_name == "west") {
		direction = West;
	} else if (direction_name == "northwest") {
		direction = Northwest;
	} else {
		fprintf(stderr, "Wrong direction (\"%s\") set for pathway.\n", direction_name.c_str());
		return;
	}
	
	int pathway_id;
	
	if (pathway_name == "none") {
		pathway_id = -1;
	} else if (pathway_name == "trail") {
		pathway_id = PathwayTrail;
	} else if (pathway_name == "road") {
		pathway_id = PathwayRoad;
	} else {
		fprintf(stderr, "Pathway \"%s\" does not exist.\n", pathway_name.c_str());
		return;
	}
	
	GrandStrategyGame.WorldMapTiles[x][y]->Pathway[direction] = pathway_id;
}

void SetWorldMapTilePort(int x, int y, bool has_port)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	
	GrandStrategyGame.WorldMapTiles[x][y]->SetPort(has_port);
}

/**
**  Calculate the graphic tile for a world map tile.
*/
void CalculateWorldMapTileGraphicTile(int x, int y)
{
	Assert(GrandStrategyGame.WorldMapTiles[x][y]);
	Assert(GrandStrategyGame.WorldMapTiles[x][y]->Terrain != -1);
	
	int terrain = GrandStrategyGame.WorldMapTiles[x][y]->Terrain;
	
	if (terrain != -1 && GrandStrategyGame.TerrainTypes[terrain]) {
		//set the GraphicTile for this world map tile
		std::string graphic_tile = "tilesets/world/terrain/";
		graphic_tile += GrandStrategyGame.TerrainTypes[terrain]->Tag;
		
		std::string base_tile_filename;
		if (GrandStrategyGame.TerrainTypes[terrain]->BaseTile != -1) {
			if (GrandStrategyWorld == "Nidavellir") {
				base_tile_filename = "tilesets/world/terrain/dark_plains";
			} else {
				base_tile_filename = "tilesets/world/terrain/plains";
			}
		}
		
		if (GrandStrategyGame.TerrainTypes[terrain]->HasTransitions) {
			graphic_tile += "/";
			graphic_tile += GrandStrategyGame.TerrainTypes[terrain]->Tag;
			
			if (
				GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_north";
			}
			if (
				GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_south";
			}
			if (
				GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_west";
			}
			if (
				GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_east";
			}
			if (
				GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_northwest_outer";
			}
			if (
				GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_northeast_outer";
			}
			if (
				GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_southwest_outer";
			}
			if (
				GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_southeast_outer";
			}
			if (
				GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_northwest_inner";
			}
			if (
				GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_northeast_inner";
			}
			if (
				GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_southwest_inner";
			}
			if (
				GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_southeast_inner";
			}
			if (
				GetWorldMapTileTerrain(x, y - 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y + 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y - 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y + 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y - 1) == GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y + 1) == GetWorldMapTileTerrain(x, y)
			) {
				graphic_tile += "_inner";
			}
			if (
				/*
				GetWorldMapTileTerrain(x, y - 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x, y + 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y - 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x - 1, y + 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y - 1) != GetWorldMapTileTerrain(x, y)
				&& GetWorldMapTileTerrain(x + 1, y + 1) != GetWorldMapTileTerrain(x, y)
				*/
				graphic_tile.find("north", 0) == std::string::npos
				&& graphic_tile.find("south", 0) == std::string::npos
				&& graphic_tile.find("west", 0) == std::string::npos
				&& graphic_tile.find("east", 0) == std::string::npos
				&& graphic_tile.find("inner", 0) == std::string::npos
			) {
				graphic_tile += "_outer";
			}
			graphic_tile = FindAndReplaceString(graphic_tile, "_northwest_outer_northeast_outer_southwest_outer_southeast_outer", "_outer");
			graphic_tile = FindAndReplaceString(graphic_tile, "_northwest_outer_northeast_outer_southwest_outer", "_outer");
			graphic_tile = FindAndReplaceString(graphic_tile, "_northwest_outer_northeast_outer_southeast_outer", "_outer");
			graphic_tile = FindAndReplaceString(graphic_tile, "_northeast_outer_southwest_outer_southeast_outer", "_outer");
			graphic_tile = FindAndReplaceString(graphic_tile, "_northeast_outer_southwest_outer_southeast_outer", "_outer");
		}
		
		if (GrandStrategyGame.WorldMapTiles[x][y]->Variation != -1) {
			graphic_tile += "_";
			graphic_tile += std::to_string((long long) GrandStrategyGame.WorldMapTiles[x][y]->Variation + 1);
		}
		
		if (GrandStrategyGame.TerrainTypes[terrain]->BaseTile != -1 && GrandStrategyGame.WorldMapTiles[x][y]->BaseTileVariation != -1) {
			base_tile_filename += "_";
			base_tile_filename += std::to_string((long long) GrandStrategyGame.WorldMapTiles[x][y]->BaseTileVariation + 1);
		}
			
		graphic_tile += ".png";
		if (GrandStrategyGame.TerrainTypes[terrain]->BaseTile != -1) {
			base_tile_filename += ".png";
		}
		
		if (!CanAccessFile(graphic_tile.c_str()) && GrandStrategyGame.WorldMapTiles[x][y]->Variation != -1) {
			for (int i = GrandStrategyGame.WorldMapTiles[x][y]->Variation; i > -1; --i) {
				if (i >= 1) {
					graphic_tile = FindAndReplaceString(graphic_tile, std::to_string((long long) i + 1), std::to_string((long long) i));
				} else {
					graphic_tile = FindAndReplaceString(graphic_tile, "_1", "");
				}
				
				if (CanAccessFile(graphic_tile.c_str())) {
					break;
				}
			}
		}
		if (GrandStrategyGame.TerrainTypes[terrain]->BaseTile != -1) {
			if (!CanAccessFile(base_tile_filename.c_str()) && GrandStrategyGame.WorldMapTiles[x][y]->BaseTileVariation != -1) {
				for (int i = GrandStrategyGame.WorldMapTiles[x][y]->BaseTileVariation; i > -1; --i) {
					if (i >= 1) {
						base_tile_filename = FindAndReplaceString(base_tile_filename, std::to_string((long long) i + 1), std::to_string((long long) i));
					} else {
						base_tile_filename = FindAndReplaceString(base_tile_filename, "_1", "");
					}
					
					if (CanAccessFile(base_tile_filename.c_str())) {
						break;
					}
				}
			}
		}
		
		if (CGraphic::Get(graphic_tile) == NULL) {
			CGraphic *tile_graphic = CGraphic::New(graphic_tile, 64, 64);
			tile_graphic->Load();
		}
		GrandStrategyGame.WorldMapTiles[x][y]->GraphicTile = CGraphic::Get(graphic_tile);
		
		if (GrandStrategyGame.TerrainTypes[terrain]->BaseTile != -1) {
			if (CGraphic::Get(base_tile_filename) == NULL) {
				CGraphic *base_tile_graphic = CGraphic::New(base_tile_filename, 64, 64);
				base_tile_graphic->Load();
			}
			GrandStrategyGame.WorldMapTiles[x][y]->BaseTile = CGraphic::Get(base_tile_filename);
		}
		
		if (GrandStrategyGame.WorldMapTiles[x][y]->Resource != -1) {
			std::string resource_building_filename = "tilesets/world/sites/resource_building_";
			resource_building_filename += DefaultResourceNames[GrandStrategyGame.WorldMapTiles[x][y]->Resource];
			
			//see if the resource has a graphic specific for this tile's terrain, and if so, use it
			if (ResourceGrandStrategyBuildingTerrainSpecificGraphic[GrandStrategyGame.WorldMapTiles[x][y]->Resource][terrain]) {
				resource_building_filename += "_";
				resource_building_filename += GrandStrategyGame.TerrainTypes[terrain]->Tag;
			}
			
			if (ResourceGrandStrategyBuildingVariations[GrandStrategyGame.WorldMapTiles[x][y]->Resource] > 0 && CanAccessFile((resource_building_filename + "_" + std::to_string((long long) ResourceGrandStrategyBuildingVariations[GrandStrategyGame.WorldMapTiles[x][y]->Resource]) + ".png").c_str())) { //see if the resource has variations, and if so, choose a random one
				resource_building_filename += "_";
				resource_building_filename += std::to_string((long long) SyncRand(ResourceGrandStrategyBuildingVariations[GrandStrategyGame.WorldMapTiles[x][y]->Resource]) + 1);
			}
			
			std::string resource_building_player_color_filename = resource_building_filename + "_player_color.png";
			resource_building_filename += ".png";
			
			if (CGraphic::Get(resource_building_filename) == NULL) {
				CGraphic *resource_building_graphics = CGraphic::New(resource_building_filename, 64, 64);
				resource_building_graphics->Load();
			}
			GrandStrategyGame.WorldMapTiles[x][y]->ResourceBuildingGraphics = CGraphic::Get(resource_building_filename);
			
			if (CanAccessFile(resource_building_player_color_filename.c_str())) {
				if (CPlayerColorGraphic::Get(resource_building_player_color_filename) == NULL) {
					CPlayerColorGraphic *resource_building_graphics_player_color = CPlayerColorGraphic::New(resource_building_player_color_filename, 64, 64);
					resource_building_graphics_player_color->Load();
				}
				GrandStrategyGame.WorldMapTiles[x][y]->ResourceBuildingGraphicsPlayerColor = CPlayerColorGraphic::Get(resource_building_player_color_filename);
			}
		}
	}
}

void AddWorldMapResource(std::string resource_name, int x, int y, bool discovered)
{
	int province_id = GrandStrategyGame.WorldMapTiles[x][y]->Province;
	if (GrandStrategyGame.WorldMapTiles[x][y]->Resource != -1) { //if tile already has a resource, remove it from the old resource's arrays
		int old_resource = GrandStrategyGame.WorldMapTiles[x][y]->Resource;
		for (int i = 0; i < WorldMapResourceMax; ++i) { // remove it from the world map resources array
			if (GrandStrategyGame.WorldMapResources[old_resource][i].x == x && GrandStrategyGame.WorldMapResources[old_resource][i].y == y) { //if tile was found, push every element of the array after it back one step
				for (int j = i; j < WorldMapResourceMax; ++j) {
					GrandStrategyGame.WorldMapResources[old_resource][j].x = GrandStrategyGame.WorldMapResources[old_resource][j + 1].x;
					GrandStrategyGame.WorldMapResources[old_resource][j].y = GrandStrategyGame.WorldMapResources[old_resource][j + 1].y;
					if (GrandStrategyGame.WorldMapResources[old_resource][j].x == -1 && GrandStrategyGame.WorldMapResources[old_resource][j].y == -1) { // if this is a blank tile slot
						break;
					}
				}
				break;
			}
			if (GrandStrategyGame.WorldMapResources[old_resource][i].x == -1 && GrandStrategyGame.WorldMapResources[old_resource][i].y == -1) { // if this is a blank tile slot
				break;
			}
		}
	
		if (province_id != -1) {
			if (GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected) {
				GrandStrategyGame.Provinces[province_id]->ProductionCapacity[old_resource] -= 1;
			}
			GrandStrategyGame.Provinces[province_id]->ResourceTiles[old_resource].erase(std::remove(GrandStrategyGame.Provinces[province_id]->ResourceTiles[old_resource].begin(), GrandStrategyGame.Provinces[province_id]->ResourceTiles[old_resource].end(), Vec2i(x, y)), GrandStrategyGame.Provinces[province_id]->ResourceTiles[old_resource].end());
		}
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource != -1) {
		for (int i = 0; i < WorldMapResourceMax; ++i) {
			if (GrandStrategyGame.WorldMapResources[resource][i].x == -1 && GrandStrategyGame.WorldMapResources[resource][i].y == -1) { //if this spot for a world map resource is blank
				GrandStrategyGame.WorldMapResources[resource][i].x = x;
				GrandStrategyGame.WorldMapResources[resource][i].y = y;
				GrandStrategyGame.WorldMapTiles[x][y]->Resource = resource;
				GrandStrategyGame.WorldMapTiles[x][y]->SetResourceProspected(resource, discovered);
				break;
			}
		}
		if (province_id != -1) {
			GrandStrategyGame.Provinces[province_id]->ResourceTiles[resource].push_back(Vec2i(x, y));
		}
	}
}

void SetWorldMapResourceProspected(std::string resource_name, int x, int y, bool discovered)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource != -1) {
		GrandStrategyGame.WorldMapTiles[x][y]->SetResourceProspected(resource, discovered);
	}
}

/**
**  Get the cultural name of a province
*/
std::string GetProvinceCulturalName(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		return GrandStrategyGame.Provinces[province_id]->GetCulturalName();
	}
	
	return "";
}

/**
**  Get the cultural name of a province pertaining to a particular civilization
*/
std::string GetProvinceCivilizationCulturalName(std::string province_name, std::string civilization_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			return GrandStrategyGame.Provinces[province_id]->CulturalNames[civilization];
		}
	}
	
	return "";
}

/**
**  Get the cultural name of a province pertaining to a particular faction
*/
std::string GetProvinceFactionCulturalName(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
			if (faction != -1) {
				return GrandStrategyGame.Provinces[province_id]->FactionCulturalNames[civilization][faction];
			}
		}
	}
	
	return "";
}

/**
**  Get the cultural name of a province
*/
std::string GetProvinceCulturalSettlementName(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		return GrandStrategyGame.Provinces[province_id]->GetCulturalSettlementName();
	}
	
	return "";
}

/**
**  Get the cultural settlement name of a province pertaining to a particular civilization
*/
std::string GetProvinceCivilizationCulturalSettlementName(std::string province_name, std::string civilization_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			return GrandStrategyGame.Provinces[province_id]->CulturalSettlementNames[civilization];
		}
	}
	
	return "";
}

/**
**  Get the cultural settlement name of a province pertaining to a particular faction
*/
std::string GetProvinceFactionCulturalSettlementName(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
			if (faction != -1) {
				return GrandStrategyGame.Provinces[province_id]->FactionCulturalSettlementNames[civilization][faction];
			}
		}
	}
	
	return "";
}

std::string GetProvinceAttackedBy(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		if (GrandStrategyGame.Provinces[province_id]->AttackedBy != NULL) {
			return PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->AttackedBy->Civilization][GrandStrategyGame.Provinces[province_id]->AttackedBy->Faction]->Name;
		}
	}
	
	return "";
}

void SetProvinceName(std::string old_province_name, std::string new_province_name)
{
	int province_id = GetProvinceId(old_province_name);

	if (province_id == -1 || !GrandStrategyGame.Provinces[province_id]) { //if province doesn't exist, create it now
		province_id = GrandStrategyGame.ProvinceCount;
		if (GrandStrategyGame.ProvinceCount >= ProvinceMax) {
			fprintf(stderr, "Max province limit of %d reached.\n", ProvinceMax);
		}
		if (!GrandStrategyGame.Provinces[province_id]) {
			CProvince *province = new CProvince;
			GrandStrategyGame.Provinces[province_id] = province;
		}
		GrandStrategyGame.Provinces[province_id]->ID = province_id;
		GrandStrategyGame.ProvinceCount += 1;
	}
	GrandStrategyGame.Provinces[province_id]->Name = new_province_name;
}

void SetProvinceWater(std::string province_name, bool water)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->Water = water;
	}
}

void SetProvinceOwner(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	int civilization_id = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction_id = PlayerRaces.GetFactionIndexByName(civilization_id, faction_name);
	
	if (province_id == -1 || !GrandStrategyGame.Provinces[province_id]) {
		return;
	}
	
	GrandStrategyGame.Provinces[province_id]->SetOwner(civilization_id, faction_id);
}

void SetProvinceCivilization(std::string province_name, std::string civilization_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		GrandStrategyGame.Provinces[province_id]->SetCivilization(civilization);
	}
}

void SetProvinceSettlementName(std::string province_name, std::string settlement_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->SettlementName = TransliterateText(settlement_name);
	}
}

void SetProvinceSettlementLocation(std::string province_name, int x, int y)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->SettlementLocation.x = x;
		GrandStrategyGame.Provinces[province_id]->SettlementLocation.y = y;
	}
}

void SetProvinceCulturalName(std::string province_name, std::string civilization_name, std::string province_cultural_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			GrandStrategyGame.Provinces[province_id]->CulturalNames[civilization] = TransliterateText(province_cultural_name);
		}
	}
}

void SetProvinceFactionCulturalName(std::string province_name, std::string civilization_name, std::string faction_name, std::string province_cultural_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
			if (faction != -1) {
				GrandStrategyGame.Provinces[province_id]->FactionCulturalNames[civilization][faction] = TransliterateText(province_cultural_name);
			}
		}
	}
}

void SetProvinceCulturalSettlementName(std::string province_name, std::string civilization_name, std::string province_cultural_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			GrandStrategyGame.Provinces[province_id]->CulturalSettlementNames[civilization] = TransliterateText(province_cultural_name);
		}
	}
}

void SetProvinceFactionCulturalSettlementName(std::string province_name, std::string civilization_name, std::string faction_name, std::string province_cultural_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
			if (faction != -1) {
				GrandStrategyGame.Provinces[province_id]->FactionCulturalSettlementNames[civilization][faction] = TransliterateText(province_cultural_name);
			}
		}
	}
}

void SetProvinceReferenceProvince(std::string province_name, std::string reference_province_name)
{
	int province_id = GetProvinceId(province_name);
	int reference_province_id = GetProvinceId(reference_province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && reference_province_id != -1) {
		GrandStrategyGame.Provinces[province_id]->ReferenceProvince = reference_province_id;
	}
}

void SetProvinceSettlementBuilding(std::string province_name, std::string settlement_building_ident, bool has_settlement_building)
{
	int province_id = GetProvinceId(province_name);
	int settlement_building = UnitTypeIdByIdent(settlement_building_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && settlement_building != -1) {
		GrandStrategyGame.Provinces[province_id]->SetSettlementBuilding(settlement_building, has_settlement_building);
	}
}

void SetProvinceCurrentConstruction(std::string province_name, std::string settlement_building_ident)
{
	int province_id = GetProvinceId(province_name);
	int settlement_building;
	if (!settlement_building_ident.empty()) {
		settlement_building = UnitTypeIdByIdent(settlement_building_ident);
	} else {
		settlement_building = -1;
	}
	if (province_id != -1) {
		GrandStrategyGame.Provinces[province_id]->CurrentConstruction = settlement_building;
	}
}

void SetProvincePopulation(std::string province_name, int quantity)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		if (GrandStrategyGame.Provinces[province_id]->Civilization == -1) {
			return;
		}
		int worker_unit_type = GrandStrategyGame.Provinces[province_id]->GetClassUnitType(GetUnitTypeClassIndexByName("worker"));
	
		if (quantity > 0) {
			quantity /= 10000; // each population unit represents 10,000 people
			quantity /= 2; // only (working-age) adults are represented, so around half of the total population
			quantity = std::max(1, quantity);
		}
	
//		quantity -= GrandStrategyGame.Provinces[province_id]->TotalUnits - GrandStrategyGame.Provinces[province_id]->Units[worker_unit_type]; // decrease from the quantity to be set the population that isn't composed of workers
		// don't take military units in consideration for this population count for now (since they don't cost food anyway)
		quantity -= GrandStrategyGame.Provinces[province_id]->TotalWorkers - GrandStrategyGame.Provinces[province_id]->Units[worker_unit_type]; // decrease from the quantity to be set the population that isn't composed of workers
		quantity = std::max(0, quantity);
			
		GrandStrategyGame.Provinces[province_id]->SetUnitQuantity(worker_unit_type, quantity);
	}
}

void SetProvinceUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && unit_type != -1) {
		GrandStrategyGame.Provinces[province_id]->SetUnitQuantity(unit_type, quantity);
	}
}

void ChangeProvinceUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && unit_type != -1) {
		GrandStrategyGame.Provinces[province_id]->ChangeUnitQuantity(unit_type, quantity);
	}
}

void SetProvinceUnderConstructionUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && unit_type != -1) {
		GrandStrategyGame.Provinces[province_id]->UnderConstructionUnits[unit_type] = std::max(0, quantity);
	}
}

void SetProvinceMovingUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && unit_type != -1) {
		if (quantity > 0) {
			GrandStrategyGame.Provinces[province_id]->Movement = true;
		}
		GrandStrategyGame.Provinces[province_id]->MovingUnits[unit_type] = std::max(0, quantity);
	}
}

void SetProvinceAttackingUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id] && unit_type != -1) {
		GrandStrategyGame.Provinces[province_id]->SetAttackingUnitQuantity(unit_type, quantity);
	}
}

void SetProvinceHero(std::string province_name, std::string hero_full_name, int value)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->SetHero(hero_full_name, value);
	}
}

void SetProvinceFood(std::string province_name, int quantity)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->PopulationGrowthProgress = std::max(0, quantity);
	}
}

void ChangeProvinceFood(std::string province_name, int quantity)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->PopulationGrowthProgress += quantity;
		GrandStrategyGame.Provinces[province_id]->PopulationGrowthProgress = std::max(0, GrandStrategyGame.Provinces[province_id]->PopulationGrowthProgress);
	}
}

void SetProvinceAttackedBy(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization_id = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		int faction_id = PlayerRaces.GetFactionIndexByName(civilization_id, faction_name);
		if (civilization_id != -1 && faction_id != -1) {
			GrandStrategyGame.Provinces[province_id]->AttackedBy = const_cast<CGrandStrategyFaction *>(&(*GrandStrategyGame.Factions[civilization_id][faction_id]));
		} else {
			GrandStrategyGame.Provinces[province_id]->AttackedBy = NULL;
		}
	}
}

void SetSelectedProvince(std::string province_name)
{
	int province_id = GetProvinceId(province_name);

	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.SelectedProvince = province_id;
	}
}

void AddProvinceClaim(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
			if (faction != -1) {
				GrandStrategyGame.Provinces[province_id]->AddFactionClaim(civilization, faction);
			} else {
				fprintf(stderr, "Can't find %s faction (%s) to add claim to province %s.\n", faction_name.c_str(), civilization_name.c_str(), province_name.c_str());
			}
		} else {
			fprintf(stderr, "Can't find %s civilization to add the claim of its %s faction claim to province %s.\n", civilization_name.c_str(), faction_name.c_str(), province_name.c_str());
		}
	} else {
		fprintf(stderr, "Can't find %s province to add %s faction (%s) claim to.\n", province_name.c_str(), faction_name.c_str(), civilization_name.c_str());
	}
}

void RemoveProvinceClaim(std::string province_name, std::string civilization_name, std::string faction_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
			if (faction != -1) {
				GrandStrategyGame.Provinces[province_id]->RemoveFactionClaim(civilization, faction);
			}
		}
	}
}

void UpdateProvinceMinimap(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id != -1 && GrandStrategyGame.Provinces[province_id]) {
		GrandStrategyGame.Provinces[province_id]->UpdateMinimap();
	}
}

/**
**  Clean the grand strategy variables.
*/
void CleanGrandStrategyGame()
{
	for (int x = 0; x < WorldMapWidthMax; ++x) {
		for (int y = 0; y < WorldMapHeightMax; ++y) {
			if (GrandStrategyGame.WorldMapTiles[x][y]) {
				GrandStrategyGame.WorldMapTiles[x][y]->Terrain = -1;
				GrandStrategyGame.WorldMapTiles[x][y]->Province = -1;
				GrandStrategyGame.WorldMapTiles[x][y]->BaseTileVariation = -1;
				GrandStrategyGame.WorldMapTiles[x][y]->Variation = -1;
				GrandStrategyGame.WorldMapTiles[x][y]->Resource = -1;
				GrandStrategyGame.WorldMapTiles[x][y]->ResourceProspected = false;
				GrandStrategyGame.WorldMapTiles[x][y]->Port = false;
				GrandStrategyGame.WorldMapTiles[x][y]->Worked = false;
				GrandStrategyGame.WorldMapTiles[x][y]->Name = "";
				GrandStrategyGame.WorldMapTiles[x][y]->BaseTile = NULL;
				GrandStrategyGame.WorldMapTiles[x][y]->GraphicTile = NULL;
				GrandStrategyGame.WorldMapTiles[x][y]->ResourceBuildingGraphics = NULL;
				GrandStrategyGame.WorldMapTiles[x][y]->ResourceBuildingGraphicsPlayerColor = NULL;
				for (int i = 0; i < MAX_RACES; ++i) {
					GrandStrategyGame.WorldMapTiles[x][y]->CulturalNames[i] = "";
				}
				for (int i = 0; i < MaxDirections; ++i) {
					GrandStrategyGame.WorldMapTiles[x][y]->Borders[i] = false;
					GrandStrategyGame.WorldMapTiles[x][y]->River[i] = -1;
					GrandStrategyGame.WorldMapTiles[x][y]->Riverhead[i] = -1;
					GrandStrategyGame.WorldMapTiles[x][y]->Pathway[i] = -1;
				}
			} else {
				break;
			}
		}
	}
	
	for (int i = 0; i < ProvinceMax; ++i) {
		if (GrandStrategyGame.Provinces[i] && !GrandStrategyGame.Provinces[i]->Name.empty()) {
			GrandStrategyGame.Provinces[i]->Name = "";
			GrandStrategyGame.Provinces[i]->SettlementName = "";
			GrandStrategyGame.Provinces[i]->ID = -1;
			GrandStrategyGame.Provinces[i]->Civilization = -1;
			GrandStrategyGame.Provinces[i]->Owner = NULL;
			GrandStrategyGame.Provinces[i]->ReferenceProvince = -1;
			GrandStrategyGame.Provinces[i]->CurrentConstruction = -1;
			GrandStrategyGame.Provinces[i]->AttackedBy = NULL;
			GrandStrategyGame.Provinces[i]->TotalUnits = 0;
			GrandStrategyGame.Provinces[i]->TotalWorkers = 0;
			GrandStrategyGame.Provinces[i]->PopulationGrowthProgress = 0;
			GrandStrategyGame.Provinces[i]->FoodConsumption = 0;
			GrandStrategyGame.Provinces[i]->Labor = 0;
			GrandStrategyGame.Provinces[i]->MilitaryScore = 0;
			GrandStrategyGame.Provinces[i]->OffensiveMilitaryScore = 0;
			GrandStrategyGame.Provinces[i]->AttackingMilitaryScore = 0;
			GrandStrategyGame.Provinces[i]->Water = false;
			GrandStrategyGame.Provinces[i]->Coastal = false;
			GrandStrategyGame.Provinces[i]->Movement = false;
			GrandStrategyGame.Provinces[i]->SettlementLocation.x = -1;
			GrandStrategyGame.Provinces[i]->SettlementLocation.y = -1;
			for (int j = 0; j < MAX_RACES; ++j) {
				GrandStrategyGame.Provinces[i]->CulturalNames[j] = "";
				GrandStrategyGame.Provinces[i]->CulturalSettlementNames[j] = "";
				for (int k = 0; k < FactionMax; ++k) {
					GrandStrategyGame.Provinces[i]->FactionCulturalNames[j][k] = "";
					GrandStrategyGame.Provinces[i]->FactionCulturalSettlementNames[j][k] = "";
				}
			}
			for (size_t j = 0; j < UnitTypes.size(); ++j) {
				GrandStrategyGame.Provinces[i]->SettlementBuildings[j] = false;
				GrandStrategyGame.Provinces[i]->Units[j] = 0;
				GrandStrategyGame.Provinces[i]->UnderConstructionUnits[j] = 0;
				GrandStrategyGame.Provinces[i]->MovingUnits[j] = 0;
				GrandStrategyGame.Provinces[i]->AttackingUnits[j] = 0;
			}
			for (int j = 0; j < ProvinceMax; ++j) {
				GrandStrategyGame.Provinces[i]->BorderProvinces[j] = -1;
			}
			for (int j = 0; j < MaxCosts; ++j) {
				GrandStrategyGame.Provinces[i]->Income[j] = 0;
				GrandStrategyGame.Provinces[i]->ProductionCapacity[j] = 0;
				GrandStrategyGame.Provinces[i]->ProductionCapacityFulfilled[j] = 0;
				GrandStrategyGame.Provinces[i]->ProductionEfficiencyModifier[j] = 0;
				GrandStrategyGame.Provinces[i]->ResourceTiles[j].clear();
			}
			GrandStrategyGame.Provinces[i]->Claims.clear();
			GrandStrategyGame.Provinces[i]->Heroes.clear();
			GrandStrategyGame.Provinces[i]->Tiles.clear();
		} else {
			break;
		}
	}
	
	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (GrandStrategyGame.Factions[i][j]) {
				if (PlayerRaces.Factions[i][j]->Type == "tribe") {
					GrandStrategyGame.Factions[i][j]->GovernmentType = -1;
				} else if (PlayerRaces.Factions[i][j]->Type == "polity") {
					GrandStrategyGame.Factions[i][j]->GovernmentType = GovernmentTypeMonarchy; //monarchy is the default government type for polities
				}
				GrandStrategyGame.Factions[i][j]->FactionTier = PlayerRaces.Factions[i][j]->DefaultTier;
				GrandStrategyGame.Factions[i][j]->CurrentResearch = -1;
				GrandStrategyGame.Factions[i][j]->ProvinceCount = 0;
				GrandStrategyGame.Factions[i][j]->Upkeep = 0;
				GrandStrategyGame.Factions[i][j]->Ruler = NULL;
				for (size_t k = 0; k < AllUpgrades.size(); ++k) {
					GrandStrategyGame.Factions[i][j]->Technologies[k] = false;
				}
				for (int k = 0; k < MaxCosts; ++k) {
					GrandStrategyGame.Factions[i][j]->Resources[k] = 0;
					GrandStrategyGame.Factions[i][j]->Income[k] = 0;
					GrandStrategyGame.Factions[i][j]->ProductionEfficiencyModifier[k] = 0;
					GrandStrategyGame.Factions[i][j]->Trade[k] = 0;
				}
				for (int k = 0; k < ProvinceMax; ++k) {
					GrandStrategyGame.Factions[i][j]->OwnedProvinces[k] = -1;
				}
				for (size_t k = 0; k < UnitTypes.size(); ++k) {
					GrandStrategyGame.Factions[i][j]->MilitaryScoreBonus[k] = 0;
				}
				for (int k = 0; k < MAX_RACES; ++k) {
					for (int n = 0; n < FactionMax; ++n) {
						GrandStrategyGame.Factions[i][j]->DiplomacyState[k][n] = DiplomacyStatePeace;
						GrandStrategyGame.Factions[i][j]->DiplomacyStateProposal[k][n] = -1;
					}
				}
				GrandStrategyGame.Factions[i][j]->Claims.clear();
			} else {
				break;
			}
		}
	}
	
	for (int i = 0; i < RiverMax; ++i) {
		if (GrandStrategyGame.Rivers[i] && !GrandStrategyGame.Rivers[i]->Name.empty()) {
			GrandStrategyGame.Rivers[i]->Name = "";
			for (int j = 0; j < MAX_RACES; ++j) {
				GrandStrategyGame.Rivers[i]->CulturalNames[j] = "";
			}
		} else {
			break;
		}
	}
	
	for (int i = 0; i < MaxCosts; ++i) {
		GrandStrategyGame.CommodityPrices[i] = 0;
		for (int j = 0; j < WorldMapResourceMax; ++j) {
			if (GrandStrategyGame.WorldMapResources[i][j].x != -1 || GrandStrategyGame.WorldMapResources[i][j].y != -1) {
				GrandStrategyGame.WorldMapResources[i][j].x = -1;
				GrandStrategyGame.WorldMapResources[i][j].y = -1;
			} else {
				break;
			}
		}
	}
	
	for (size_t i = 0; i < GrandStrategyGame.Heroes.size(); ++i) {
		GrandStrategyGame.Heroes[i]->State = 0;
		GrandStrategyGame.Heroes[i]->Province = NULL;
		GrandStrategyGame.Heroes[i]->Type = NULL;
		GrandStrategyGame.Heroes[i]->ProvinceOfOrigin = NULL;
		GrandStrategyGame.Heroes[i]->Children.clear();
		GrandStrategyGame.Heroes[i]->Siblings.clear();
		GrandStrategyGame.Heroes[i]->Father = NULL;
		GrandStrategyGame.Heroes[i]->Mother = NULL;
	}

	for (size_t i = 0; i < GrandStrategyGame.Heroes.size(); ++i) {
		delete GrandStrategyGame.Heroes[i];
	}
	GrandStrategyGame.Heroes.clear();
	GrandStrategyHeroStringToIndex.clear();
	
	GrandStrategyGame.WorldMapWidth = 0;
	GrandStrategyGame.WorldMapHeight = 0;
	GrandStrategyGame.ProvinceCount = 0;
	GrandStrategyGame.SelectedProvince = -1;
	GrandStrategyGame.PlayerFaction = NULL;
	
	//destroy minimap surface
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		if (GrandStrategyGame.MinimapSurfaceGL) {
			glDeleteTextures(1, &GrandStrategyGame.MinimapTexture);
			delete[] GrandStrategyGame.MinimapSurfaceGL;
			GrandStrategyGame.MinimapSurfaceGL = NULL;
		}
	} else
#endif
	{
		if (GrandStrategyGame.MinimapSurface) {
			VideoPaletteListRemove(GrandStrategyGame.MinimapSurface);
			SDL_FreeSurface(GrandStrategyGame.MinimapSurface);
			GrandStrategyGame.MinimapSurface = NULL;
		}
	}
	
	GrandStrategyGame.MinimapTextureWidth = 0;
	GrandStrategyGame.MinimapTextureHeight = 0;
	GrandStrategyGame.MinimapTileWidth = 0;
	GrandStrategyGame.MinimapTileHeight = 0;
	GrandStrategyGame.MinimapOffsetX = 0;
	GrandStrategyGame.MinimapOffsetY = 0;
	
	WorldMapOffsetX = 0;
	WorldMapOffsetY = 0;
	GrandStrategyMapWidthIndent = 0;
	GrandStrategyMapHeightIndent = 0;
}

void InitializeGrandStrategyGame(bool show_loading)
{
	if (show_loading) {
		CalculateItemsToLoad(true);
		UpdateLoadingBar();
	}
	
	//do the same for the fog tile now
	std::string fog_graphic_tile = "tilesets/world/terrain/fog.png";
	if (CGraphic::Get(fog_graphic_tile) == NULL) {
		CGraphic *fog_tile_graphic = CGraphic::New(fog_graphic_tile, 96, 96);
		fog_tile_graphic->Load();
	}
	GrandStrategyGame.FogTile = CGraphic::Get(fog_graphic_tile);
	
	// set the settlement graphics
	for (int i = 0; i < MAX_RACES; ++i) {
		std::string settlement_graphics_file = "tilesets/world/sites/";
		settlement_graphics_file += PlayerRaces.Name[i];
		settlement_graphics_file += "_settlement";
		std::string settlement_masonry_graphics_file = settlement_graphics_file + "_masonry" + ".png";
		settlement_graphics_file += ".png";
		if (!CanAccessFile(settlement_graphics_file.c_str()) && PlayerRaces.ParentCivilization[i] != -1) {
			settlement_graphics_file = FindAndReplaceString(settlement_graphics_file, PlayerRaces.Name[i], PlayerRaces.Name[PlayerRaces.ParentCivilization[i]]);
		}
		if (!CanAccessFile(settlement_masonry_graphics_file.c_str()) && PlayerRaces.ParentCivilization[i] != -1) {
			settlement_masonry_graphics_file = FindAndReplaceString(settlement_masonry_graphics_file, PlayerRaces.Name[i], PlayerRaces.Name[PlayerRaces.ParentCivilization[i]]);
		}
		if (CanAccessFile(settlement_graphics_file.c_str())) {
			if (CPlayerColorGraphic::Get(settlement_graphics_file) == NULL) {
				CPlayerColorGraphic *settlement_graphics = CPlayerColorGraphic::New(settlement_graphics_file, 64, 64);
				settlement_graphics->Load();
			}
			GrandStrategyGame.SettlementGraphics[i] = CPlayerColorGraphic::Get(settlement_graphics_file);
		}
		if (CanAccessFile(settlement_masonry_graphics_file.c_str())) {
			if (CPlayerColorGraphic::Get(settlement_masonry_graphics_file) == NULL) {
				CPlayerColorGraphic *settlement_graphics = CPlayerColorGraphic::New(settlement_masonry_graphics_file, 64, 64);
				settlement_graphics->Load();
			}
			GrandStrategyGame.SettlementMasonryGraphics[i] = CPlayerColorGraphic::Get(settlement_masonry_graphics_file);
		}
		
		std::string barracks_graphics_file = "tilesets/world/sites/";
		barracks_graphics_file += PlayerRaces.Name[i];
		barracks_graphics_file += "_barracks.png";
		if (!CanAccessFile(barracks_graphics_file.c_str()) && PlayerRaces.ParentCivilization[i] != -1) {
			barracks_graphics_file = FindAndReplaceString(barracks_graphics_file, PlayerRaces.Name[i], PlayerRaces.Name[PlayerRaces.ParentCivilization[i]]);
		}
		if (CanAccessFile(barracks_graphics_file.c_str())) {
			if (CPlayerColorGraphic::Get(barracks_graphics_file) == NULL) {
				CPlayerColorGraphic *barracks_graphics = CPlayerColorGraphic::New(barracks_graphics_file, 64, 64);
				barracks_graphics->Load();
			}
			GrandStrategyGame.BarracksGraphics[i] = CPlayerColorGraphic::Get(barracks_graphics_file);
		}
	}
	
	// set the border graphics
	for (int i = 0; i < MaxDirections; ++i) {
		std::string border_graphics_file = "tilesets/world/terrain/";
		border_graphics_file += "province_border_";
		
		std::string national_border_graphics_file = "tilesets/world/terrain/";
		national_border_graphics_file += "province_national_border_";
		
		if (i == North) {
			border_graphics_file += "north";
			national_border_graphics_file += "north";
		} else if (i == Northeast) {
			border_graphics_file += "northeast_inner";
			national_border_graphics_file += "northeast_inner";
		} else if (i == East) {
			border_graphics_file += "east";
			national_border_graphics_file += "east";
		} else if (i == Southeast) {
			border_graphics_file += "southeast_inner";
			national_border_graphics_file += "southeast_inner";
		} else if (i == South) {
			border_graphics_file += "south";
			national_border_graphics_file += "south";
		} else if (i == Southwest) {
			border_graphics_file += "southwest_inner";
			national_border_graphics_file += "southwest_inner";
		} else if (i == West) {
			border_graphics_file += "west";
			national_border_graphics_file += "west";
		} else if (i == Northwest) {
			border_graphics_file += "northwest_inner";
			national_border_graphics_file += "northwest_inner";
		}
		
		border_graphics_file += ".png";
		national_border_graphics_file += ".png";
		
		if (CGraphic::Get(border_graphics_file) == NULL) {
			CGraphic *border_graphics = CGraphic::New(border_graphics_file, 84, 84);
			border_graphics->Load();
		}
		GrandStrategyGame.BorderGraphics[i] = CGraphic::Get(border_graphics_file);
		
		if (CPlayerColorGraphic::Get(national_border_graphics_file) == NULL) {
			CPlayerColorGraphic *national_border_graphics = CPlayerColorGraphic::New(national_border_graphics_file, 84, 84);
			national_border_graphics->Load();
		}
		GrandStrategyGame.NationalBorderGraphics[i] = CPlayerColorGraphic::Get(national_border_graphics_file);
	}
	
	// set the river and road graphics
	for (int i = 0; i < MaxDirections; ++i) {
		std::string river_graphics_file = "tilesets/world/terrain/";
		river_graphics_file += "river_";
		
		std::string rivermouth_graphics_file = "tilesets/world/terrain/";
		rivermouth_graphics_file += "rivermouth_";
		
		std::string riverhead_graphics_file = "tilesets/world/terrain/";
		riverhead_graphics_file += "riverhead_";
		
		std::string trail_graphics_file = "tilesets/world/terrain/";
		trail_graphics_file += "trail_";
		
		std::string road_graphics_file = "tilesets/world/terrain/";
		road_graphics_file += "road_";
		
		if (i == North) {
			river_graphics_file += "north";
			rivermouth_graphics_file += "north";
			riverhead_graphics_file += "north";
			trail_graphics_file += "north";
			road_graphics_file += "north";
		} else if (i == Northeast) {
			river_graphics_file += "northeast_inner";
			rivermouth_graphics_file += "northeast";
			riverhead_graphics_file += "northeast";
			trail_graphics_file += "northeast";
			road_graphics_file += "northeast";
		} else if (i == East) {
			river_graphics_file += "east";
			rivermouth_graphics_file += "east";
			riverhead_graphics_file += "east";
			trail_graphics_file += "east";
			road_graphics_file += "east";
		} else if (i == Southeast) {
			river_graphics_file += "southeast_inner";
			rivermouth_graphics_file += "southeast";
			riverhead_graphics_file += "southeast";
			trail_graphics_file += "southeast";
			road_graphics_file += "southeast";
		} else if (i == South) {
			river_graphics_file += "south";
			rivermouth_graphics_file += "south";
			riverhead_graphics_file += "south";
			trail_graphics_file += "south";
			road_graphics_file += "south";
		} else if (i == Southwest) {
			river_graphics_file += "southwest_inner";
			rivermouth_graphics_file += "southwest";
			riverhead_graphics_file += "southwest";
			trail_graphics_file += "southwest";
			road_graphics_file += "southwest";
		} else if (i == West) {
			river_graphics_file += "west";
			rivermouth_graphics_file += "west";
			riverhead_graphics_file += "west";
			trail_graphics_file += "west";
			road_graphics_file += "west";
		} else if (i == Northwest) {
			river_graphics_file += "northwest_inner";
			rivermouth_graphics_file += "northwest";
			riverhead_graphics_file += "northwest";
			trail_graphics_file += "northwest";
			road_graphics_file += "northwest";
		}
		
		std::string rivermouth_flipped_graphics_file;
		if (i == North || i == East || i == South || i == West) { //only non-diagonal directions get flipped rivermouth graphics
			rivermouth_flipped_graphics_file = rivermouth_graphics_file + "_flipped" + ".png";
		}
		
		std::string riverhead_flipped_graphics_file;
		if (i == North || i == East || i == South || i == West) { //only non-diagonal directions get flipped riverhead graphics
			riverhead_flipped_graphics_file = riverhead_graphics_file + "_flipped" + ".png";
		}
		
		river_graphics_file += ".png";
		rivermouth_graphics_file += ".png";
		riverhead_graphics_file += ".png";
		trail_graphics_file += ".png";
		road_graphics_file += ".png";
		
		if (CGraphic::Get(river_graphics_file) == NULL) {
			CGraphic *river_graphics = CGraphic::New(river_graphics_file, 84, 84);
			river_graphics->Load();
		}
		GrandStrategyGame.RiverGraphics[i] = CGraphic::Get(river_graphics_file);
		
		if (CGraphic::Get(rivermouth_graphics_file) == NULL) {
			CGraphic *rivermouth_graphics = CGraphic::New(rivermouth_graphics_file, 84, 84);
			rivermouth_graphics->Load();
		}
		GrandStrategyGame.RivermouthGraphics[i][0] = CGraphic::Get(rivermouth_graphics_file);
		
		if (!rivermouth_flipped_graphics_file.empty()) {
			if (CGraphic::Get(rivermouth_flipped_graphics_file) == NULL) {
				CGraphic *rivermouth_flipped_graphics = CGraphic::New(rivermouth_flipped_graphics_file, 84, 84);
				rivermouth_flipped_graphics->Load();
			}
			GrandStrategyGame.RivermouthGraphics[i][1] = CGraphic::Get(rivermouth_flipped_graphics_file);
		}
		
		if (CGraphic::Get(riverhead_graphics_file) == NULL) {
			CGraphic *riverhead_graphics = CGraphic::New(riverhead_graphics_file, 84, 84);
			riverhead_graphics->Load();
		}
		GrandStrategyGame.RiverheadGraphics[i][0] = CGraphic::Get(riverhead_graphics_file);
		
		if (!riverhead_flipped_graphics_file.empty()) {
			if (CGraphic::Get(riverhead_flipped_graphics_file) == NULL) {
				CGraphic *riverhead_flipped_graphics = CGraphic::New(riverhead_flipped_graphics_file, 84, 84);
				riverhead_flipped_graphics->Load();
			}
			GrandStrategyGame.RiverheadGraphics[i][1] = CGraphic::Get(riverhead_flipped_graphics_file);
		}
		
		if (CGraphic::Get(road_graphics_file) == NULL) { //use road graphics file for trails for now
			CGraphic *trail_graphics = CGraphic::New(road_graphics_file, 64, 64);
			trail_graphics->Load();
		}
		GrandStrategyGame.PathwayGraphics[PathwayTrail][i] = CGraphic::Get(road_graphics_file);
		
		if (CGraphic::Get(road_graphics_file) == NULL) {
			CGraphic *road_graphics = CGraphic::New(road_graphics_file, 64, 64);
			road_graphics->Load();
		}
		GrandStrategyGame.PathwayGraphics[PathwayRoad][i] = CGraphic::Get(road_graphics_file);
	}
	
	//load the move symbol
	std::string move_symbol_filename = "tilesets/world/sites/move.png";
	if (CGraphic::Get(move_symbol_filename) == NULL) {
		CGraphic *move_symbol_graphic = CGraphic::New(move_symbol_filename, 64, 64);
		move_symbol_graphic->Load();
	}
	GrandStrategyGame.SymbolMove = CGraphic::Get(move_symbol_filename);
	
	//load the attack symbol
	std::string attack_symbol_filename = "tilesets/world/sites/attack.png";
	if (CGraphic::Get(attack_symbol_filename) == NULL) {
		CGraphic *attack_symbol_graphic = CGraphic::New(attack_symbol_filename, 64, 64);
		attack_symbol_graphic->Load();
	}
	GrandStrategyGame.SymbolAttack = CGraphic::Get(attack_symbol_filename);
	
	//load the hero symbol
	std::string hero_symbol_filename = "tilesets/world/sites/hero.png";
	if (CGraphic::Get(hero_symbol_filename) == NULL) {
		CGraphic *hero_symbol_graphic = CGraphic::New(hero_symbol_filename, 64, 64);
		hero_symbol_graphic->Load();
	}
	GrandStrategyGame.SymbolHero = CGraphic::Get(hero_symbol_filename);
	
	//load the resource not worked symbol
	std::string resource_not_worked_symbol_filename = "tilesets/world/sites/resource_not_worked.png";
	if (CGraphic::Get(resource_not_worked_symbol_filename) == NULL) {
		CGraphic *resource_not_worked_symbol_graphic = CGraphic::New(resource_not_worked_symbol_filename, 64, 64);
		resource_not_worked_symbol_graphic->Load();
	}
	GrandStrategyGame.SymbolResourceNotWorked = CGraphic::Get(resource_not_worked_symbol_filename);
	
	//create grand strategy faction instances for all defined factions
	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (!GrandStrategyGame.Factions[i][j]) { // no need to create a grand strategy instance for an already-created faction again
				if (PlayerRaces.Factions[i][j] && !PlayerRaces.Factions[i][j]->Name.empty()) { //if the faction is defined
					CGrandStrategyFaction *faction = new CGrandStrategyFaction;
					GrandStrategyGame.Factions[i][j] = faction;
					
					GrandStrategyGame.Factions[i][j]->Civilization = i;
					GrandStrategyGame.Factions[i][j]->Faction = j;
					GrandStrategyGame.Factions[i][j]->FactionTier = PlayerRaces.Factions[i][j]->DefaultTier;
				} else {
					break;
				}
			}
			
			if (!PlayerRaces.Factions[i][j]->FactionUpgrade.empty()) { //if faction has a faction upgrade, apply it
				GrandStrategyGame.Factions[i][j]->SetTechnology(UpgradeIdByIdent(PlayerRaces.Factions[i][j]->FactionUpgrade), true);
			}
		}
	}
	
	//set resource prices to base prices
	for (int i = 0; i < MaxCosts; ++i) {
		GrandStrategyGame.CommodityPrices[i] = DefaultResourcePrices[i];
	}
	
	//initialize heroes
	for (size_t i = 0; i < Characters.size(); ++i) {
		if (Characters[i]->Civilization == -1 || Characters[i]->Year == 0 || Characters[i]->DeathYear == 0 || Characters[i]->ProvinceOfOriginName.empty()) {
			continue;
		}
		CGrandStrategyHero *hero = new CGrandStrategyHero;
		GrandStrategyGame.Heroes.push_back(hero);
		hero->Name = Characters[i]->Name;
		hero->ExtraName = Characters[i]->ExtraName;
		hero->Dynasty = Characters[i]->Dynasty;
		if (Characters[i]->Type != NULL) {
			hero->Type = const_cast<CUnitType *>(&(*Characters[i]->Type));
		}
		if (Characters[i]->Trait != NULL) {
			hero->Trait = const_cast<CUpgrade *>(&(*Characters[i]->Trait));
		} else if (hero->Type != NULL && hero->Type->Traits.size() > 0) {
			hero->Trait = const_cast<CUpgrade *>(&(*hero->Type->Traits[SyncRand(hero->Type->Traits.size())]));
		}
		hero->Year = Characters[i]->Year;
		hero->DeathYear = Characters[i]->DeathYear;
		hero->Civilization = Characters[i]->Civilization;
		hero->ProvinceOfOriginName = Characters[i]->ProvinceOfOriginName;
		hero->Gender = Characters[i]->Gender;
		if (Characters[i]->Father != NULL) {
			hero->Father = const_cast<CGrandStrategyHero *>(&(*GrandStrategyGame.GetHero(Characters[i]->Father->GetFullName())));
			hero->Father->Children.push_back(hero);
		}
		if (Characters[i]->Mother != NULL) {
			hero->Mother = const_cast<CGrandStrategyHero *>(&(*GrandStrategyGame.GetHero(Characters[i]->Mother->GetFullName())));
			hero->Mother->Children.push_back(hero);
		}
		for (size_t j = 0; j < Characters[i]->Siblings.size(); ++j) { // now check for male siblings of the current ruler
			CGrandStrategyHero *sibling = GrandStrategyGame.GetHero(Characters[i]->Siblings[j]->GetFullName());
			if (sibling != NULL) {
				hero->Siblings.push_back(sibling);
				sibling->Siblings.push_back(hero); //when the sibling was defined, the hero wasn't, since by virtue of not being NULL, the sibling was necessarily defined before the hero
			}
		}
		if (!Characters[i]->Icon.Name.empty()) {
			hero->Icon.Name = Characters[i]->Icon.Name;
			hero->Icon.Icon = NULL;
		}
		if (!Characters[i]->HeroicIcon.Name.empty()) {
			hero->HeroicIcon.Name = Characters[i]->HeroicIcon.Name;
			hero->HeroicIcon.Icon = NULL;
		}
		GrandStrategyHeroStringToIndex[hero->GetFullName()] = GrandStrategyGame.Heroes.size() - 1;
	}
	
	if (CurrentCustomHero != NULL) { //if a custom hero has been selected, create the hero as a grand strategy hero
		CGrandStrategyHero *hero = new CGrandStrategyHero;
		GrandStrategyGame.Heroes.push_back(hero);
		hero->Name = CurrentCustomHero->Name;
		hero->ExtraName = CurrentCustomHero->ExtraName;
		hero->Dynasty = CurrentCustomHero->Dynasty;
		if (CurrentCustomHero->Type != NULL) {
			hero->Type = const_cast<CUnitType *>(&(*CurrentCustomHero->Type));
		}
		if (CurrentCustomHero->Trait != NULL) {
			hero->Trait = const_cast<CUpgrade *>(&(*CurrentCustomHero->Trait));
		} else if (hero->Type != NULL && hero->Type->Traits.size() > 0) {
			hero->Trait = const_cast<CUpgrade *>(&(*hero->Type->Traits[SyncRand(hero->Type->Traits.size())]));
		}
		hero->Civilization = CurrentCustomHero->Civilization;
		hero->Gender = CurrentCustomHero->Gender;
		hero->Custom = CurrentCustomHero->Custom;
		GrandStrategyHeroStringToIndex[hero->GetFullName()] = GrandStrategyGame.Heroes.size() - 1;
	}
}

void InitializeGrandStrategyMinimap()
{
	//calculate the minimap texture width and height
	if (GrandStrategyGame.WorldMapWidth >= GrandStrategyGame.WorldMapHeight) {
		GrandStrategyGame.MinimapTextureWidth = UI.Minimap.W;
		GrandStrategyGame.MinimapTextureHeight = UI.Minimap.H * GrandStrategyGame.WorldMapHeight / GrandStrategyGame.WorldMapWidth;
	} else {
		GrandStrategyGame.MinimapTextureWidth = UI.Minimap.W * GrandStrategyGame.WorldMapWidth / GrandStrategyGame.WorldMapHeight;
		GrandStrategyGame.MinimapTextureHeight = UI.Minimap.H;
	}

	//calculate the minimap tile width and height
	GrandStrategyGame.MinimapTileWidth = UI.Minimap.W * 1000 / GetWorldMapWidth();
	GrandStrategyGame.MinimapTileHeight = UI.Minimap.H * 1000 / GetWorldMapHeight();
	if (GetWorldMapWidth() >= GetWorldMapHeight()) {
		GrandStrategyGame.MinimapTileHeight = GrandStrategyGame.MinimapTileWidth;
	} else {
		GrandStrategyGame.MinimapTileWidth = GrandStrategyGame.MinimapTileHeight;
	}

	// create minimap surface
	#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		if (!GrandStrategyGame.MinimapSurfaceGL) {
			GrandStrategyGame.MinimapSurfaceGL = new unsigned char[GrandStrategyGame.MinimapTextureWidth * GrandStrategyGame.MinimapTextureHeight * 4];
			memset(GrandStrategyGame.MinimapSurfaceGL, 0, GrandStrategyGame.MinimapTextureWidth * GrandStrategyGame.MinimapTextureHeight * 4);
		}
		GrandStrategyGame.CreateMinimapTexture();
	} else
	#endif
	{
		if (!GrandStrategyGame.MinimapSurface) {
			GrandStrategyGame.MinimapSurface = SDL_CreateRGBSurface(SDL_SWSURFACE,  GrandStrategyGame.MinimapTextureWidth, GrandStrategyGame.MinimapTextureHeight, 32, TheScreen->format->Rmask, TheScreen->format->Gmask, TheScreen->format->Bmask, 0);
		}
	}

	GrandStrategyGame.UpdateMinimap();
	
	GrandStrategyGame.MinimapOffsetX = 0;
	GrandStrategyGame.MinimapOffsetY = 0;
	if (GetWorldMapWidth() <= UI.Minimap.W && GetWorldMapHeight() <= UI.Minimap.H) {
		if (GetWorldMapWidth() >= GetWorldMapHeight()) {
			GrandStrategyGame.MinimapOffsetY = (UI.Minimap.H - (GetWorldMapHeight() * std::max(GrandStrategyGame.MinimapTileHeight / 1000, 1))) / 2;
		} else {
			GrandStrategyGame.MinimapOffsetX = (UI.Minimap.W - (GetWorldMapWidth() * std::max(GrandStrategyGame.MinimapTileWidth / 1000, 1))) / 2;
		}
	} else {
		if (GetWorldMapWidth() >= GetWorldMapHeight()) {
			GrandStrategyGame.MinimapOffsetY = (UI.Minimap.H - ((GetWorldMapHeight() / std::max(1000 / GrandStrategyGame.MinimapTileHeight, 1)) * std::max(GrandStrategyGame.MinimapTileHeight / 1000, 1))) / 2;
		} else {
			GrandStrategyGame.MinimapOffsetX = (UI.Minimap.H - ((GetWorldMapWidth() / std::max(1000 / GrandStrategyGame.MinimapTileWidth, 1)) * std::max(GrandStrategyGame.MinimapTileWidth / 1000, 1))) / 2;
		}
	}
}

void InitializeGrandStrategyFactions()
{
	//initialize heroes
	for (size_t i = 0; i < GrandStrategyGame.Heroes.size(); ++i) {
		GrandStrategyGame.Heroes[i]->Initialize();
		
		if (
			(GrandStrategyGame.Heroes[i]->State == 0 && GrandStrategyYear >= GrandStrategyGame.Heroes[i]->Year && GrandStrategyYear < GrandStrategyGame.Heroes[i]->DeathYear)
			|| GrandStrategyGame.Heroes[i]->Custom //create custom hero regardless of date
		) {
			GrandStrategyGame.Heroes[i]->Create();
		} else if (GrandStrategyGame.Heroes[i]->State != 0 && GrandStrategyYear >= GrandStrategyGame.Heroes[i]->DeathYear) {
			GrandStrategyGame.Heroes[i]->Die();
		}
	}
	
	for (int i = 0; i < GrandStrategyGame.ProvinceCount; ++i) {
		if (GrandStrategyGame.Provinces[i] && !GrandStrategyGame.Provinces[i]->Name.empty()) { //if this is a valid province
			if (GrandStrategyGame.Provinces[i]->Civilization != -1 && GrandStrategyGame.Provinces[i]->Owner != NULL) { // if this province has a culture and an owner
				GrandStrategyGame.Provinces[i]->ReallocateLabor(); // allocate labor for provinces
			}
			if (GrandStrategyGame.Provinces[i]->Coastal && GrandStrategyGame.Provinces[i]->Tiles.size() == 1) { //if the province is a 1-tile island, it has to start with a port in its capital to feed itself
				GrandStrategyGame.WorldMapTiles[GrandStrategyGame.Provinces[i]->SettlementLocation.x][GrandStrategyGame.Provinces[i]->SettlementLocation.y]->SetPort(true);
			}
		} else { //if a somehow invalid province is reached
			break;
		}
	}

	// calculate income and upkeep, and set initial ruler (if none is preset) for factions
	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (GrandStrategyGame.Factions[i][j]) {
				if (GrandStrategyGame.Factions[i][j]->IsAlive()) {
					// try to perform ruler succession for existent factions without rulers
					if (GrandStrategyGame.Factions[i][j]->Ruler == NULL) {
						GrandStrategyGame.Factions[i][j]->RulerSuccession();
					}
					GrandStrategyGame.Factions[i][j]->CalculateIncomes();
					GrandStrategyGame.Factions[i][j]->CalculateUpkeep();
				}
			} else { //end of valid factions for this civilization
				break;
			}
		}
	}
}

void SetGrandStrategyWorld(std::string world)
{
	GrandStrategyWorld = world;
}

void DoGrandStrategyTurn()
{
	GrandStrategyGame.DoTurn();
}

void DoProspection()
{
	GrandStrategyGame.DoProspection();
}

void CalculateProvinceBorders()
{
	for (int i = 0; i < ProvinceMax; ++i) {
		if (GrandStrategyGame.Provinces[i] && !GrandStrategyGame.Provinces[i]->Name.empty()) {
			for (size_t j = 0; j < GrandStrategyGame.Provinces[i]->Tiles.size(); ++j) {
				GrandStrategyGame.WorldMapTiles[GrandStrategyGame.Provinces[i]->Tiles[j].x][GrandStrategyGame.Provinces[i]->Tiles[j].y]->Province = i; //tell the tile it belongs to this province
			}
			
			for (int j = 0; j < ProvinceMax; ++j) { //clean border provinces
				if (GrandStrategyGame.Provinces[i]->BorderProvinces[j] == -1) {
					break;
				}
				GrandStrategyGame.Provinces[i]->BorderProvinces[j] = -1;
			}
			
			//calculate which of the province's tiles are border tiles, and which provinces it borders; also whether the province borders water (is coastal) or not
			int border_province_count = 0;
			for (size_t j = 0; j < GrandStrategyGame.Provinces[i]->Tiles.size(); ++j) {
				int x = GrandStrategyGame.Provinces[i]->Tiles[j].x;
				int y = GrandStrategyGame.Provinces[i]->Tiles[j].y;
				for (int sub_x = -1; sub_x <= 1; ++sub_x) {
					if ((x + sub_x) < 0 || (x + sub_x) >= GrandStrategyGame.WorldMapWidth) {
						continue;
					}
							
					for (int sub_y = -1; sub_y <= 1; ++sub_y) {
						if ((y + sub_y) < 0 || (y + sub_y) >= GrandStrategyGame.WorldMapHeight) {
							continue;
						}
							
						int second_province_id = GrandStrategyGame.WorldMapTiles[x + sub_x][y + sub_y]->Province;
						if (!(sub_x == 0 && sub_y == 0) && second_province_id != i && GrandStrategyGame.WorldMapTiles[x + sub_x][y + sub_y]->Terrain != -1) {
							if (second_province_id == -1 || GrandStrategyGame.Provinces[i]->Water == GrandStrategyGame.Provinces[second_province_id]->Water) {
								int direction = DirectionToHeading(Vec2i(x + sub_x, y + sub_y) - Vec2i(x, y)) + (32 / 2);
								if (direction % 32 != 0) {
									direction = direction - (direction % 32);
								}
								direction = direction / 32;
									
								GrandStrategyGame.WorldMapTiles[x][y]->Borders[direction] = true;
							}
								
							if (second_province_id != -1 && !GrandStrategyGame.Provinces[i]->BordersProvince(second_province_id)) { //if isn't added yet to the border provinces, do so now
								GrandStrategyGame.Provinces[i]->BorderProvinces[border_province_count] = second_province_id;
								border_province_count += 1;
							}
								
							if (second_province_id != -1 && GrandStrategyGame.Provinces[i]->Water == false && GrandStrategyGame.Provinces[second_province_id]->Water == true) {
								GrandStrategyGame.Provinces[i]->Coastal = true;
							}
						}
					}
				}
			}
		}
	}				
}

void CenterGrandStrategyMapOnTile(int x, int y)
{
	WorldMapOffsetX = x - (((UI.MapArea.EndX - UI.MapArea.X) / 64) / 2);
	if (WorldMapOffsetX < 0) {
		WorldMapOffsetX = 0;
	} else if (WorldMapOffsetX > GetWorldMapWidth() - 1 - ((UI.MapArea.EndX - UI.MapArea.X) / 64)) {
		WorldMapOffsetX = GetWorldMapWidth() - 1 - ((UI.MapArea.EndX - UI.MapArea.X) / 64);
	}

	WorldMapOffsetY = y - (((UI.MapArea.EndY - UI.MapArea.Y) / 64) / 2);
	if (WorldMapOffsetY < 0) {
		WorldMapOffsetY = 0;
	} else if (WorldMapOffsetY > GetWorldMapHeight() - 1 - ((UI.MapArea.EndY - UI.MapArea.Y) / 64)) {
		WorldMapOffsetY = GetWorldMapHeight() - 1 - ((UI.MapArea.EndY - UI.MapArea.Y) / 64);
	}
}

bool ProvinceBordersProvince(std::string province_name, std::string second_province_name)
{
	int province = GetProvinceId(province_name);
	int second_province = GetProvinceId(second_province_name);
	
	return GrandStrategyGame.Provinces[province]->BordersProvince(second_province);
}

bool ProvinceBordersFaction(std::string province_name, std::string faction_civilization_name, std::string faction_name)
{
	int province = GetProvinceId(province_name);
	int civilization = PlayerRaces.GetRaceIndexByName(faction_civilization_name.c_str());
	int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	
	if (civilization == -1 || faction == -1) {
		return false;
	}
	
	return GrandStrategyGame.Provinces[province]->BordersFaction(civilization, faction);
}

bool ProvinceHasBuildingClass(std::string province_name, std::string building_class)
{
	int province_id = GetProvinceId(province_name);
	
	return GrandStrategyGame.Provinces[province_id]->HasBuildingClass(building_class);
}

bool ProvinceHasClaim(std::string province_name, std::string faction_civilization_name, std::string faction_name)
{
	int province = GetProvinceId(province_name);
	int civilization = PlayerRaces.GetRaceIndexByName(faction_civilization_name.c_str());
	int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	
	if (civilization == -1 || faction == -1) {
		return false;
	}
	
	return GrandStrategyGame.Provinces[province]->HasFactionClaim(civilization, faction);
}

bool ProvinceHasResource(std::string province_name, std::string resource_name, bool ignore_prospection)
{
	int province_id = GetProvinceId(province_name);
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return false;
	}
	
	return GrandStrategyGame.Provinces[province_id]->HasResource(resource, ignore_prospection);
}

bool IsGrandStrategyBuilding(const CUnitType &type)
{
	if (type.BoolFlag[BUILDING_INDEX].value && !type.Class.empty() && type.Class != "farm" && type.Class != "watch-tower" && type.Class != "guard-tower") {
		return true;
	}
	return false;
}

std::string GetProvinceCivilization(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (GrandStrategyGame.Provinces[province_id]->Civilization != -1) {
		return PlayerRaces.Name[GrandStrategyGame.Provinces[province_id]->Civilization];
	} else {
		return "";
	}
}

bool GetProvinceSettlementBuilding(std::string province_name, std::string building_ident)
{
	int province_id = GetProvinceId(province_name);
	int building_id = UnitTypeIdByIdent(building_ident);
	
	return GrandStrategyGame.Provinces[province_id]->SettlementBuildings[building_id];
}

std::string GetProvinceCurrentConstruction(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	if (province_id != -1) {
		if (GrandStrategyGame.Provinces[province_id]->CurrentConstruction != -1) {
			return UnitTypes[GrandStrategyGame.Provinces[province_id]->CurrentConstruction]->Ident;
		}
	}
	
	return "";
}

int GetProvinceUnitQuantity(std::string province_name, std::string unit_type_ident)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	return GrandStrategyGame.Provinces[province_id]->Units[unit_type];
}

int GetProvinceUnderConstructionUnitQuantity(std::string province_name, std::string unit_type_ident)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	return GrandStrategyGame.Provinces[province_id]->UnderConstructionUnits[unit_type];
}

int GetProvinceMovingUnitQuantity(std::string province_name, std::string unit_type_ident)
{
	int province_id = GetProvinceId(province_name);
	int unit_type = UnitTypeIdByIdent(unit_type_ident);
	
	return GrandStrategyGame.Provinces[province_id]->MovingUnits[unit_type];
}

int GetProvinceAttackingUnitQuantity(std::string province_name, std::string unit_type_ident)
{
	int province_id = GetProvinceId(province_name);
	int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
	
	return GrandStrategyGame.Provinces[province_id]->AttackingUnits[unit_type_id];
}

int GetProvinceHero(std::string province_name, std::string hero_full_name)
{
	int province_id = GetProvinceId(province_name);

	if (province_id == -1) {
		fprintf(stderr, "Can't find %s province.\n", province_name.c_str());
		return 0;
	}
	
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		if (hero->Province != NULL && hero->Province->ID == province_id) {
			return hero->State;
		}
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	
	return 0;
}

int GetProvinceLabor(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	return GrandStrategyGame.Provinces[province_id]->Labor;
}

int GetProvinceAvailableWorkersForTraining(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	return GrandStrategyGame.Provinces[province_id]->Labor / 100;
}

int GetProvinceTotalWorkers(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	return GrandStrategyGame.Provinces[province_id]->TotalWorkers;
}

int GetProvinceMilitaryScore(std::string province_name, bool attacker, bool count_defenders)
{
	int province_id = GetProvinceId(province_name);
	
	int military_score = 0;
	if (province_id != -1) {
		if (attacker) {
			military_score = GrandStrategyGame.Provinces[province_id]->AttackingMilitaryScore;
		} else if (count_defenders) {
			military_score = GrandStrategyGame.Provinces[province_id]->MilitaryScore;
		} else {
			military_score = GrandStrategyGame.Provinces[province_id]->OffensiveMilitaryScore;
		}
	}
	
	return std::max(1, military_score); // military score must be at least one, since it is a divider in some instances, and we don't want to divide by 0
}

std::string GetProvinceOwner(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id == -1 || GrandStrategyGame.Provinces[province_id]->Owner == NULL) {
		return "";
	}
	
	return PlayerRaces.Factions[GrandStrategyGame.Provinces[province_id]->Owner->Civilization][GrandStrategyGame.Provinces[province_id]->Owner->Faction]->Name;
}

bool GetProvinceWater(std::string province_name)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id == -1) {
		return false;
	}
	
	return GrandStrategyGame.Provinces[province_id]->Water;
}

int GetProvinceFoodCapacity(std::string province_name, bool subtract_non_food)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id == -1) {
		return false;
	}
	
	return GrandStrategyGame.Provinces[province_id]->GetFoodCapacity(subtract_non_food);
}

void SetPlayerFaction(std::string civilization_name, std::string faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	if (faction == -1) {
		return;
	}
	
	GrandStrategyGame.PlayerFaction = const_cast<CGrandStrategyFaction *>(&(*GrandStrategyGame.Factions[civilization][faction]));
}

std::string GetPlayerFactionName()
{
	if (GrandStrategyGame.PlayerFaction != NULL) {
		return PlayerRaces.Factions[GrandStrategyGame.PlayerFaction->Civilization][GrandStrategyGame.PlayerFaction->Faction]->Name;
	} else {
		return "";
	}
}

void SetFactionResource(std::string civilization_name, std::string faction_name, std::string resource_name, int resource_quantity)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization][faction]->Resources[resource] = resource_quantity;
}

void ChangeFactionResource(std::string civilization_name, std::string faction_name, std::string resource_name, int resource_quantity)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization][faction]->Resources[resource] += resource_quantity;
}

int GetFactionResource(std::string civilization_name, std::string faction_name, std::string resource_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return 0;
	}
	
	return GrandStrategyGame.Factions[civilization][faction]->Resources[resource];
}

void CalculateFactionIncomes(std::string civilization_name, std::string faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	if (faction == -1 || GrandStrategyGame.Factions[civilization][faction]->ProvinceCount == 0) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization][faction]->CalculateIncomes();
}

void CalculateFactionUpkeeps()
{
	for (int i = 0; i < MAX_RACES; ++i) {
		for (int j = 0; j < FactionMax; ++j) {
			if (GrandStrategyGame.Factions[i][j]) {
				GrandStrategyGame.Factions[i][j]->CalculateUpkeep();
			} else { //end of valid factions for this civilization
				break;
			}
		}
	}
}

int GetFactionIncome(std::string civilization_name, std::string faction_name, std::string resource_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return 0;
	}
	
	return GrandStrategyGame.Factions[civilization][faction]->Income[resource];
}

int GetFactionUpkeep(std::string civilization_name, std::string faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	if (faction == -1) {
		return 0;
	}
	
	return GrandStrategyGame.Factions[civilization][faction]->Upkeep;
}

void SetFactionTechnology(std::string civilization_name, std::string faction_name, std::string upgrade_ident, bool has_technology)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int upgrade_id = UpgradeIdByIdent(upgrade_ident);
	if (civilization != -1 && upgrade_id != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		if (faction != -1) {
			GrandStrategyGame.Factions[civilization][faction]->SetTechnology(upgrade_id, has_technology);
		}
	}
}

bool GetFactionTechnology(std::string civilization_name, std::string faction_name, std::string upgrade_ident)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int upgrade_id = UpgradeIdByIdent(upgrade_ident);
	if (civilization != -1 && upgrade_id != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		if (faction != -1) {
			return GrandStrategyGame.Factions[civilization][faction]->Technologies[upgrade_id];
		}
	}
	
	return false;
}

void SetFactionGovernmentType(std::string civilization_name, std::string faction_name, std::string government_type_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	
	int government_type_id = GetGovernmentTypeIdByName(government_type_name);
	
	if (government_type_id == -1) {
		return;
	}

	if (civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		if (faction != -1) {
			GrandStrategyGame.Factions[civilization][faction]->GovernmentType = government_type_id;
		}
	}
}

void SetFactionDiplomacyState(std::string civilization_name, std::string faction_name, std::string second_civilization_name, std::string second_faction_name, std::string diplomacy_state_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int second_civilization = PlayerRaces.GetRaceIndexByName(second_civilization_name.c_str());
	
	int diplomacy_state_id = GetDiplomacyStateIdByName(diplomacy_state_name);
	
	if (diplomacy_state_id == -1) {
		return;
	}

	int second_diplomacy_state_id; // usually the second diplomacy state is the same as the first, but there are asymmetrical diplomacy states (such as vassal/sovereign relationships)
	if (diplomacy_state_id == DiplomacyStateVassal) {
		second_diplomacy_state_id = DiplomacyStateSovereign;
	} else if (diplomacy_state_id == DiplomacyStateSovereign) {
		second_diplomacy_state_id = DiplomacyStateVassal;
	} else {
		second_diplomacy_state_id = diplomacy_state_id;
	}

	if (civilization != -1 && second_civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		int second_faction = PlayerRaces.GetFactionIndexByName(second_civilization, second_faction_name);
		if (faction != -1 && second_faction != -1) {
			GrandStrategyGame.Factions[civilization][faction]->DiplomacyState[second_civilization][second_faction] = diplomacy_state_id;
			GrandStrategyGame.Factions[second_civilization][second_faction]->DiplomacyState[civilization][faction] = second_diplomacy_state_id;
		}
	}
}

std::string GetFactionDiplomacyState(std::string civilization_name, std::string faction_name, std::string second_civilization_name, std::string second_faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int second_civilization = PlayerRaces.GetRaceIndexByName(second_civilization_name.c_str());
	
	if (civilization != -1 && second_civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		int second_faction = PlayerRaces.GetFactionIndexByName(second_civilization, second_faction_name);
		if (faction != -1 && second_faction != -1) {
			return GetDiplomacyStateNameById(GrandStrategyGame.Factions[civilization][faction]->DiplomacyState[second_civilization][second_faction]);
		}
	}
	
	return "";
}

void SetFactionDiplomacyStateProposal(std::string civilization_name, std::string faction_name, std::string second_civilization_name, std::string second_faction_name, std::string diplomacy_state_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int second_civilization = PlayerRaces.GetRaceIndexByName(second_civilization_name.c_str());
	
	int diplomacy_state_id = GetDiplomacyStateIdByName(diplomacy_state_name);
	
	if (civilization != -1 && second_civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		int second_faction = PlayerRaces.GetFactionIndexByName(second_civilization, second_faction_name);
		if (faction != -1 && second_faction != -1) {
			GrandStrategyGame.Factions[civilization][faction]->DiplomacyStateProposal[second_civilization][second_faction] = diplomacy_state_id;
		}
	}
}

std::string GetFactionDiplomacyStateProposal(std::string civilization_name, std::string faction_name, std::string second_civilization_name, std::string second_faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int second_civilization = PlayerRaces.GetRaceIndexByName(second_civilization_name.c_str());
	
	if (civilization != -1 && second_civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		int second_faction = PlayerRaces.GetFactionIndexByName(second_civilization, second_faction_name);
		if (faction != -1 && second_faction != -1) {
			return GetDiplomacyStateNameById(GrandStrategyGame.Factions[civilization][faction]->DiplomacyStateProposal[second_civilization][second_faction]);
		}
	}
	
	return "";
}

void SetFactionTier(std::string civilization_name, std::string faction_name, std::string faction_tier_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	
	int faction_tier_id = GetFactionTierIdByName(faction_tier_name);
	
	if (faction_tier_id == -1) {
		return;
	}

	if (civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		if (faction != -1) {
			GrandStrategyGame.Factions[civilization][faction]->FactionTier = faction_tier_id;
		}
	}
}

std::string GetFactionTier(std::string civilization_name, std::string faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	if (civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		if (faction != -1) {
			return GetFactionTierNameById(GrandStrategyGame.Factions[civilization][faction]->FactionTier);
		}
	}
	
	return "";
}

void SetFactionCurrentResearch(std::string civilization_name, std::string faction_name, std::string upgrade_ident)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int upgrade_id;
	if (!upgrade_ident.empty()) {
		upgrade_id = UpgradeIdByIdent(upgrade_ident);
	} else {
		upgrade_id = -1;
	}
	if (civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		if (faction != -1) {
			GrandStrategyGame.Factions[civilization][faction]->CurrentResearch = upgrade_id;
		}
	}
}

std::string GetFactionCurrentResearch(std::string civilization_name, std::string faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	if (civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		if (faction != -1) {
			if (GrandStrategyGame.Factions[civilization][faction]->CurrentResearch != -1) {
				return AllUpgrades[GrandStrategyGame.Factions[civilization][faction]->CurrentResearch]->Ident;
			}
		}
	}
	
	return "";
}

std::string GetFactionFullName(std::string civilization_name, std::string faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	if (civilization != -1) {
		int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
		if (faction != -1) {
			return GrandStrategyGame.Factions[civilization][faction]->GetFullName();
		}
	}
	
	return "";
}

void AcquireFactionTechnologies(std::string civilization_from_name, std::string faction_from_name, std::string civilization_to_name, std::string faction_to_name)
{
	int civilization_from = PlayerRaces.GetRaceIndexByName(civilization_from_name.c_str());
	int civilization_to = PlayerRaces.GetRaceIndexByName(civilization_to_name.c_str());
	if (civilization_from != -1 && civilization_to != -1) {
		int faction_from = PlayerRaces.GetFactionIndexByName(civilization_from, faction_from_name);
		int faction_to = PlayerRaces.GetFactionIndexByName(civilization_to, faction_to_name);
		if (faction_from != -1 && faction_to != -1) {
			GrandStrategyGame.Factions[civilization_to][faction_to]->AcquireFactionTechnologies(civilization_from, faction_from);
		}
	}
}

bool IsGrandStrategyUnit(const CUnitType &type)
{
	if (!type.BoolFlag[BUILDING_INDEX].value && type.DefaultStat.Variables[DEMAND_INDEX].Value > 0 && type.Class != "caravan") {
		return true;
	}
	return false;
}

bool IsMilitaryUnit(const CUnitType &type)
{
	if (IsGrandStrategyUnit(type) && type.Class != "worker") {
		return true;
	}
	return false;
}

void CreateProvinceUnits(std::string province_name, int player, int divisor, bool attacking_units, bool ignore_militia)
{
	int province_id = GetProvinceId(province_name);
	
	if (province_id == -1) {
		return;
	}
	
	for (size_t i = 0; i < UnitTypes.size(); ++i) {
		int units_to_be_created = 0;
		if (IsMilitaryUnit(*UnitTypes[i]) && UnitTypes[i]->Class != "militia") {
			if (!attacking_units) {
				units_to_be_created = GrandStrategyGame.Provinces[province_id]->Units[i] / divisor;
				GrandStrategyGame.Provinces[province_id]->ChangeUnitQuantity(i, - units_to_be_created);
			} else {
				units_to_be_created = GrandStrategyGame.Provinces[province_id]->AttackingUnits[i] / divisor;
				GrandStrategyGame.Provinces[province_id]->ChangeAttackingUnitQuantity(i, - units_to_be_created);
			}
		} else if (!attacking_units && UnitTypes[i]->Class == "worker" && !ignore_militia && !UnitTypes[i]->Civilization.empty()) { // create militia in the province depending on the amount of workers
			
			int militia_unit_type = PlayerRaces.GetCivilizationClassUnitType(PlayerRaces.GetRaceIndexByName(UnitTypes[i]->Civilization.c_str()), GetUnitTypeClassIndexByName("militia"));
			
			if (militia_unit_type != -1) {
				units_to_be_created = GrandStrategyGame.Provinces[province_id]->Units[militia_unit_type] / 2 / divisor; //half of the worker population as militia
			}
		}
		
		if (units_to_be_created > 0) {
			units_to_be_created *= BattalionMultiplier;
			for (int j = 0; j < units_to_be_created; ++j) {
				CUnit *unit = MakeUnit(*UnitTypes[i], &Players[player]);
				if (unit == NULL) {
					DebugPrint("Unable to allocate unit");
					return;
				} else {
					if (UnitCanBeAt(*unit, Players[player].StartPos)) {
						unit->Place(Players[player].StartPos);
					} else {
						const int heading = SyncRand() % 256;

						unit->tilePos = Players[player].StartPos;
						DropOutOnSide(*unit, heading, NULL);
					}
					UpdateForNewUnit(*unit, 0);
				}
			}
		}
	}
}

void FormFaction(std::string old_civilization_name, std::string old_faction_name, std::string new_civilization_name, std::string new_faction_name)
{
	int old_civilization = PlayerRaces.GetRaceIndexByName(old_civilization_name.c_str());
	int old_faction = -1;
	if (old_civilization != -1) {
		old_faction = PlayerRaces.GetFactionIndexByName(old_civilization, old_faction_name);
	}
	
	int new_civilization = PlayerRaces.GetRaceIndexByName(new_civilization_name.c_str());
	int new_faction = -1;
	if (new_civilization != -1) {
		new_faction = PlayerRaces.GetFactionIndexByName(new_civilization, new_faction_name);
	}
	
	if (old_faction == -1 || new_faction == -1) {
		return;
	}
	
	GrandStrategyGame.Factions[old_civilization][old_faction]->FormFaction(new_civilization, new_faction);
}

void SetFactionCommodityTrade(std::string civilization_name, std::string faction_name, std::string resource_name, int quantity)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization][faction]->Trade[resource] = quantity;
}

void ChangeFactionCommodityTrade(std::string civilization_name, std::string faction_name, std::string resource_name, int quantity)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization][faction]->Trade[resource] += quantity;
}

int GetFactionCommodityTrade(std::string civilization_name, std::string faction_name, std::string resource_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (faction == -1 || resource == -1) {
		return 0;
	}
	
	return GrandStrategyGame.Factions[civilization][faction]->Trade[resource];
}

bool FactionHasHero(std::string civilization_name, std::string faction_name, std::string hero_full_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	if (faction == -1) {
		return false;
	}
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		if (hero->State == 0) {
			return false;
		}
		if (hero->Province != NULL) {
			if (hero->State != 3 && hero->Province->Owner != NULL && hero->Province->Owner->Civilization == civilization && hero->Province->Owner->Faction == faction) {
				return true;
			} else if (hero->State == 3 && hero->Province->AttackedBy != NULL && hero->Province->AttackedBy->Civilization == civilization && hero->Province->AttackedBy->Faction == faction) {
				return true;
			}
		}
		//check if the hero is the ruler of a faction
		for (int i = 0; i < MAX_RACES; ++i) {
			for (int j = 0; j < FactionMax; ++j) {
				if (GrandStrategyGame.Factions[i][j]) {
					if (GrandStrategyGame.Factions[i][j]->Ruler == hero) {
						if (civilization == i && faction == j) {
							return true;
						} else {
							return false;
						}
					}
				} else {
					break;
				}
			}
		}
		if (
			hero->Province == NULL
			&& hero->ProvinceOfOrigin != NULL
			&& hero->ProvinceOfOrigin->Owner != NULL
			&& hero->ProvinceOfOrigin->Owner->Civilization == civilization
			&& hero->ProvinceOfOrigin->Owner->Faction == faction
		) {
			return true;
		}
	}
	return false;
}

void SetFactionRuler(std::string civilization_name, std::string faction_name, std::string hero_full_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	if (faction == -1) {
		return;
	}
	
	GrandStrategyGame.Factions[civilization][faction]->SetRuler(hero_full_name);
}

std::string GetFactionRuler(std::string civilization_name, std::string faction_name)
{
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	int faction = -1;
	if (civilization != -1) {
		faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	}
	
	if (faction == -1) {
		return "";
	}
	
	if (GrandStrategyGame.Factions[civilization][faction]->Ruler != NULL) {
		return GrandStrategyGame.Factions[civilization][faction]->Ruler->GetFullName();
	} else {
		return "";
	}
}

void CreateGrandStrategyHero(std::string hero_full_name)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		hero->Create();
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
}

void CreateGrandStrategyCustomHero(std::string hero_full_name)
{
	CCharacter *custom_hero = GetCustomHero(hero_full_name);
	CGrandStrategyHero *hero = new CGrandStrategyHero;
	GrandStrategyGame.Heroes.push_back(hero);
	hero->Name = custom_hero->Name;
	hero->ExtraName = custom_hero->ExtraName;
	hero->Dynasty = custom_hero->Dynasty;
	if (custom_hero->Type != NULL) {
		hero->Type = const_cast<CUnitType *>(&(*custom_hero->Type));
	}
	if (custom_hero->Trait != NULL) {
		hero->Trait = const_cast<CUpgrade *>(&(*custom_hero->Trait));
	} else if (hero->Type != NULL && hero->Type->Traits.size() > 0) {
		hero->Trait = const_cast<CUpgrade *>(&(*hero->Type->Traits[SyncRand(hero->Type->Traits.size())]));
	}
	hero->Civilization = custom_hero->Civilization;
	hero->Gender = custom_hero->Gender;
	hero->Custom = custom_hero->Custom;
	GrandStrategyHeroStringToIndex[hero->GetFullName()] = GrandStrategyGame.Heroes.size() - 1;

	hero->Initialize();
	hero->Create();
}

void KillGrandStrategyHero(std::string hero_full_name)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		hero->Die();
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
}

void SetGrandStrategyHeroUnitType(std::string hero_full_name, std::string unit_type_ident)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
		if (unit_type_id != -1) {
			hero->SetType(unit_type_id);
		}
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
}

std::string GetGrandStrategyHeroUnitType(std::string hero_full_name)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		if (hero->Type != NULL) {
			return hero->Type->Ident;
		}
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	return "";
}

bool GrandStrategyHeroIsAlive(std::string hero_full_name)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		if (hero->State != 0) {
			return true;
		}
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	return false;
}


bool GrandStrategyHeroIsCustom(std::string hero_full_name)
{
	CGrandStrategyHero *hero = GrandStrategyGame.GetHero(hero_full_name);
	if (hero) {
		return hero->Custom;
	} else {
		fprintf(stderr, "Hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	return false;
}

void SetCommodityPrice(std::string resource_name, int price)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return;
	}
	
	GrandStrategyGame.CommodityPrices[resource] = price;
}

int GetCommodityPrice(std::string resource_name)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return 0;
	}
	
	return GrandStrategyGame.CommodityPrices[resource];
}

void SetResourceBasePrice(std::string resource_name, int price)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return;
	}
	
	DefaultResourcePrices[resource] = price;
}

void SetResourceBaseLaborInput(std::string resource_name, int input)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return;
	}
	
	DefaultResourceLaborInputs[resource] = input;
}

void SetResourceBaseOutput(std::string resource_name, int output)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return;
	}
	
	DefaultResourceOutputs[resource] = output;
}

void SetResourceGrandStrategyBuildingVariations(std::string resource_name, int variation_quantity)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return;
	}
	
	ResourceGrandStrategyBuildingVariations[resource] = variation_quantity;
}

void SetResourceGrandStrategyBuildingTerrainSpecificGraphic(std::string resource_name, std::string terrain_type_name, bool has_terrain_specific_graphic)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	int terrain_type = GetWorldMapTerrainTypeId(terrain_type_name);
	if (resource == -1 || terrain_type == -1) {
		return;
	}
	
	ResourceGrandStrategyBuildingTerrainSpecificGraphic[resource][terrain_type] = has_terrain_specific_graphic;
}

std::string GetDiplomacyStateNameById(int diplomacy_state)
{
	if (diplomacy_state == DiplomacyStatePeace) {
		return "peace";
	} else if (diplomacy_state == DiplomacyStateWar) {
		return "war";
	} else if (diplomacy_state == DiplomacyStateAlliance) {
		return "alliance";
	} else if (diplomacy_state == DiplomacyStateVassal) {
		return "vassal";
	} else if (diplomacy_state == DiplomacyStateSovereign) {
		return "sovereign";
	} else if (diplomacy_state == -1) {
		return "";
	}

	return "";
}

int GetDiplomacyStateIdByName(std::string diplomacy_state)
{
	if (diplomacy_state == "peace") {
		return DiplomacyStatePeace;
	} else if (diplomacy_state == "war") {
		return DiplomacyStateWar;
	} else if (diplomacy_state == "alliance") {
//		return DiplomacyStateAlliance; //deactivated for now until this diplomacy state is properly implemented
		return DiplomacyStatePeace;
	} else if (diplomacy_state == "vassal") {
//		return DiplomacyStateVassal;
		return DiplomacyStatePeace;
	} else if (diplomacy_state == "sovereign") {
//		return DiplomacyStateSovereign;
		return DiplomacyStatePeace;
	}

	return -1;
}

std::string GetFactionTierNameById(int faction_tier)
{
	if (faction_tier == FactionTierBarony) {
		return "barony";
	} else if (faction_tier == FactionTierCounty) {
		return "county";
	} else if (faction_tier == FactionTierDuchy) {
		return "duchy";
	} else if (faction_tier == FactionTierGrandDuchy) {
		return "grand duchy";
	} else if (faction_tier == FactionTierKingdom) {
		return "kingdom";
	} else if (faction_tier == FactionTierEmpire) {
		return "empire";
	}

	return "";
}

int GetFactionTierIdByName(std::string faction_tier)
{
	if (faction_tier == "barony") {
		return FactionTierBarony;
	} else if (faction_tier == "county") {
		return FactionTierCounty;
	} else if (faction_tier == "duchy") {
		return FactionTierDuchy;
	} else if (faction_tier == "grand duchy") {
		return FactionTierGrandDuchy;
	} else if (faction_tier == "kingdom") {
		return FactionTierKingdom;
	} else if (faction_tier == "empire") {
		return FactionTierEmpire;
	}

	return -1;
}

//@}
