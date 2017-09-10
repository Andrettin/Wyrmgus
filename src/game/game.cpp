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
/**@name game.cpp - The game set-up and creation. */
//
//      (c) Copyright 1998-2007 by Lutz Sammer, Andreas Arens,
//      Jimmy Salmon and Andrettin
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

#include <png.h>

#include "stratagus.h"

#include "game.h"

#include "actions.h"
#include "ai.h"
#include "animation.h"
//Wyrmgus start
#include "character.h"
//Wyrmgus end
#include "commands.h"
#include "construct.h"
#include "depend.h"
#include "editor.h"
#include "font.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "interface.h"
#include "iocompat.h"
#include "iolib.h"
#include "map.h"
#include "minimap.h"
#include "missile.h"
#include "netconnect.h"
#include "network.h"
#include "parameters.h"
#include "pathfinder.h"
#include "player.h"
//Wyrmgus start
#include "province.h"
#include "quest.h"
//Wyrmgus end
#include "replay.h"
#include "results.h"
//Wyrmgus start
#include "script.h"
//Wyrmgus end
#include "settings.h"
#include "sound.h"
#include "sound_server.h"
#include "spells.h"
//Wyrmgus start
#include "text.h"
//Wyrmgus end
#include "tileset.h"
#include "translate.h"
#include "trigger.h"
#include "ui.h"
#include "unit.h"
#include "unit_manager.h"
#include "unittype.h"
#include "upgrade.h"
//Wyrmgus start
#include "util.h"
//Wyrmgus end
#include "version.h"
#include "video.h"


extern void CleanGame();

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

Settings GameSettings;  /// Game Settings
static int LcmPreventRecurse;   /// prevent recursion through LoadGameMap
GameResults GameResult;                      /// Outcome of the game

std::string GameName;
std::string FullGameName;
//Wyrmgus start
std::string PlayerFaction;
//Wyrmgus end

unsigned long GameCycle;             /// Game simulation cycle counter
unsigned long FastForwardCycle;      /// Cycle to fastforward to in a replay

bool UseHPForXp = false;              /// true if gain XP by dealing damage, false if by killing.

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern gcn::Gui *Gui;
static std::vector<gcn::Container *> Containers;

/**
**  Save game settings.
**
**  @param file  Save file handle
*/
void SaveGameSettings(CFile &file)
{
	file.printf("\nGameSettings.NetGameType = %d\n", GameSettings.NetGameType);
	for (int i = 0; i < PlayerMax - 1; ++i) {
		file.printf("GameSettings.Presets[%d].AIScript = \"%s\"\n", i, GameSettings.Presets[i].AIScript.c_str());
		file.printf("GameSettings.Presets[%d].Race = %d\n", i, GameSettings.Presets[i].Race);
		file.printf("GameSettings.Presets[%d].Team = %d\n", i, GameSettings.Presets[i].Team);
		file.printf("GameSettings.Presets[%d].Type = %d\n", i, GameSettings.Presets[i].Type);
	}
	file.printf("GameSettings.Resources = %d\n", GameSettings.Resources);
	file.printf("GameSettings.Difficulty = %d\n", GameSettings.Difficulty);
	file.printf("GameSettings.NumUnits = %d\n", GameSettings.NumUnits);
	file.printf("GameSettings.Opponents = %d\n", GameSettings.Opponents);
	file.printf("GameSettings.GameType = %d\n", GameSettings.GameType);
	file.printf("GameSettings.NoFogOfWar = %s\n", GameSettings.NoFogOfWar ? "true" : "false");
	file.printf("GameSettings.RevealMap = %d\n", GameSettings.RevealMap);
	file.printf("GameSettings.MapRichness = %d\n", GameSettings.MapRichness);
	file.printf("GameSettings.Inside = %s\n", GameSettings.Inside ? "true" : "false");
	//Wyrmgus start
	file.printf("GameSettings.NoRandomness = %s\n", GameSettings.NoRandomness ? "true" : "false");
	file.printf("GameSettings.NoTimeOfDay = %s\n", GameSettings.NoTimeOfDay ? "true" : "false");
	//Wyrmgus end
	file.printf("\n");
}

void StartMap(const std::string &filename, bool clean)
{
	std::string nc, rc;

	gcn::Widget *oldTop = Gui->getTop();
	gcn::Container *container = new gcn::Container();
	Containers.push_back(container);
	container->setDimension(gcn::Rectangle(0, 0, Video.Width, Video.Height));
	container->setOpaque(false);
	Gui->setTop(container);

	NetConnectRunning = 0;
	InterfaceState = IfaceStateNormal;

	//  Create the game.
	DebugPrint("Creating game with map: %s\n" _C_ filename.c_str());
	if (clean) {
		CleanPlayers();
	}
	GetDefaultTextColors(nc, rc);

	//Wyrmgus start
	GameEstablishing = true;
	//Wyrmgus end
	CreateGame(filename, &Map);

	//Wyrmgus start
//	UI.StatusLine.Set(NameLine);
	//Wyrmgus end
	//Wyrmgus start
	//commented this out because it seemed superfluous
//	SetMessage("%s", _("Do it! Do it now!"));
	//Wyrmgus end
	
	//Wyrmgus start
	CurrentMapLayer = ThisPlayer->StartMapLayer;
	UI.SelectedViewport->Center(Map.TilePosToMapPixelPos_Center(ThisPlayer->StartPos));
	//Wyrmgus end

	//  Play the game.
	GameMainLoop();

	//  Clear screen
	Video.ClearScreen();
	Invalidate();

	CleanGame();
	InterfaceState = IfaceStateMenu;
	SetDefaultTextColors(nc, rc);

	Gui->setTop(oldTop);
	Containers.erase(std::find(Containers.begin(), Containers.end(), container));
	delete container;
}

void FreeAllContainers()
{
	for (size_t i = 0; i != Containers.size(); ++i) {
		delete Containers[i];
	}
}

/*----------------------------------------------------------------------------
--  Map loading/saving
----------------------------------------------------------------------------*/

/**
**  Load a Stratagus map.
**
**  @param smpname  smp filename
**  @param mapname  map filename
**  @param map      map loaded
*/
static void LoadStratagusMap(const std::string &smpname, const std::string &mapname)
{
	char mapfull[PATH_MAX];
	CFile file;

	// Try the same directory as the smp file first
	strcpy_s(mapfull, sizeof(mapfull), smpname.c_str());
	char *p = strrchr(mapfull, '/');
	if (!p) {
		p = mapfull;
	} else {
		++p;
	}
	strcpy_s(p, sizeof(mapfull) - (p - mapfull), mapname.c_str());

	if (file.open(mapfull, CL_OPEN_READ) == -1) {
		// Not found, try StratagusLibPath and the smp's dir
		strcpy_s(mapfull, sizeof(mapfull), StratagusLibPath.c_str());
		strcat_s(mapfull, sizeof(mapfull), "/");
		strcat_s(mapfull, sizeof(mapfull), smpname.c_str());
		char *p = strrchr(mapfull, '/');
		if (!p) {
			p = mapfull;
		} else {
			++p;
		}
		strcpy_s(p, sizeof(mapfull) - (p - mapfull), mapname.c_str());
		if (file.open(mapfull, CL_OPEN_READ) == -1) {
			// Not found again, try StratagusLibPath
			strcpy_s(mapfull, sizeof(mapfull), StratagusLibPath.c_str());
			strcat_s(mapfull, sizeof(mapfull), "/");
			strcat_s(mapfull, sizeof(mapfull), mapname.c_str());
			if (file.open(mapfull, CL_OPEN_READ) == -1) {
				// Not found, try mapname by itself as a last resort
				strcpy_s(mapfull, sizeof(mapfull), mapname.c_str());
			} else {
				file.close();
			}
		} else {
			file.close();
		}
	} else {
		file.close();
	}

	if (LcmPreventRecurse) {
		fprintf(stderr, "recursive use of load map!\n");
		ExitFatal(-1);
	}
	InitPlayers();
	LcmPreventRecurse = 1;
	if (LuaLoadFile(mapfull) == -1) {
		fprintf(stderr, "Can't load lua file: %s\n", mapfull);
		ExitFatal(-1);
	}
	LcmPreventRecurse = 0;

#if 0
	// Not true if multiplayer levels!
	if (!ThisPlayer) { /// ARI: bomb if nothing was loaded!
		fprintf(stderr, "%s: invalid map\n", mapname.c_str());
		ExitFatal(-1);
	}
#endif
	if (!Map.Info.Size.x || !Map.Info.Size.y) {
		fprintf(stderr, "%s: invalid map\n", mapname.c_str());
		ExitFatal(-1);
	}
	Map.Info.Filename = mapname;
}

// Write a small image of map preview
static void WriteMapPreview(const char *mapname, CMap &map)
{
	FILE *fp = fopen(mapname, "wb");
	if (fp == NULL) {
		return;
	}

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		fclose(fp);
		return;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fclose(fp);
		png_destroy_write_struct(&png_ptr, NULL);
		return;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		/* If we get here, we had a problem reading the file */
		fclose(fp);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return;
	}

	/* set up the output control if you are using standard C streams */
	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, UI.Minimap.W, UI.Minimap.H, 8,
				 PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				 PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);

	const int rectSize = 5; // size of rectange used for player start spots
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		unsigned char *pixels = new unsigned char[UI.Minimap.W * UI.Minimap.H * 3];
		if (!pixels) {
			fprintf(stderr, "Out of memory\n");
			exit(1);
		}
		// Copy GL map surface to pixel array
		for (int i = 0; i < UI.Minimap.H; ++i) {
			for (int j = 0; j < UI.Minimap.W; ++j) {
				//Wyrmgus start
//				Uint32 c = ((Uint32 *)MinimapSurfaceGL)[j + i * UI.Minimap.W];
				Uint32 c = ((Uint32 *)MinimapSurfaceGL[CurrentMapLayer])[j + i * UI.Minimap.W];
				//Wyrmgus end
				const int offset = (i * UI.Minimap.W + j) * 3;
				pixels[offset + 0] = ((c & RMASK) >> RSHIFT);
				pixels[offset + 1] = ((c & GMASK) >> GSHIFT);
				pixels[offset + 2] = ((c & BMASK) >> BSHIFT);
			}
		}
		// Add player start spots
		for (int i = 0; i < PlayerMax - 1; ++i) {
			//Wyrmgus start
//			if (Players[i].Type != PlayerNobody) {
			if (Players[i].Type != PlayerNobody && Players[i].StartMapLayer == CurrentMapLayer) {
			//Wyrmgus end
				for (int j = -rectSize / 2; j <= rectSize / 2; ++j) {
					for (int k = -rectSize / 2; k <= rectSize / 2; ++k) {
						//Wyrmgus start
						const int miniMapX = Players[i].StartPos.x * UI.Minimap.W / map.Info.LayersSizes[CurrentMapLayer].x;
						const int miniMapY = Players[i].StartPos.y * UI.Minimap.H / map.Info.LayersSizes[CurrentMapLayer].y;
						//Wyrmgus end
						if (miniMapX + j < 0 || miniMapX + j >= UI.Minimap.W) {
							continue;
						}
						if (miniMapY + k < 0 || miniMapY + k >= UI.Minimap.H) {
							continue;
						}
						const int offset = ((miniMapY + k) * UI.Minimap.H + miniMapX + j) * 3;
						pixels[offset + 0] = ((Players[i].Color & RMASK) >> RSHIFT);
						pixels[offset + 1] = ((Players[i].Color & GMASK) >> GSHIFT);
						pixels[offset + 2] = ((Players[i].Color & BMASK) >> BSHIFT);
					}
				}
			}
		}
		// Write everything in PNG
		for (int i = 0; i < UI.Minimap.H; ++i) {
			unsigned char *row = new unsigned char[UI.Minimap.W * 3];
			memcpy(row, pixels + i * UI.Minimap.W * 3, UI.Minimap.W * 3);
			png_write_row(png_ptr, row);
			delete[] row;
		}
		delete[] pixels;
	} else
#endif
	{
		unsigned char *row = new unsigned char[UI.Minimap.W * 3];
		//Wyrmgus start
//		const SDL_PixelFormat *fmt = MinimapSurface->format;
		const SDL_PixelFormat *fmt = MinimapSurface[CurrentMapLayer]->format;
		//Wyrmgus end
		SDL_Surface *preview = SDL_CreateRGBSurface(SDL_SWSURFACE,
													UI.Minimap.W, UI.Minimap.H, 32, fmt->Rmask, fmt->Gmask, fmt->Bmask, 0);
		//Wyrmgus start
//		SDL_BlitSurface(MinimapSurface, NULL, preview, NULL);
		SDL_BlitSurface(MinimapSurface[CurrentMapLayer], NULL, preview, NULL);
		//Wyrmgus end

		SDL_LockSurface(preview);

		SDL_Rect rect;
		for (int i = 0; i < PlayerMax - 1; ++i) {
			//Wyrmgus start
//			if (Players[i].Type != PlayerNobody) {
			if (Players[i].Type != PlayerNobody && Players[i].StartMapLayer == CurrentMapLayer) {
			//Wyrmgus end
				//Wyrmgus start
				rect.x = Players[i].StartPos.x * UI.Minimap.W / map.Info.LayersSizes[CurrentMapLayer].x - rectSize / 2;
				rect.y = Players[i].StartPos.y * UI.Minimap.H / map.Info.LayersSizes[CurrentMapLayer].y - rectSize / 2;
				//Wyrmgus end
				rect.w = rect.h = rectSize;
				SDL_FillRect(preview, &rect, Players[i].Color);
			}
		}

		for (int i = 0; i < UI.Minimap.H; ++i) {
			switch (preview->format->BytesPerPixel) {
				case 1:
					for (int j = 0; j < UI.Minimap.W; ++j) {
						Uint8 c = ((Uint8 *)preview->pixels)[j + i * UI.Minimap.W];
						row[j * 3 + 0] = fmt->palette->colors[c].r;
						row[j * 3 + 1] = fmt->palette->colors[c].g;
						row[j * 3 + 2] = fmt->palette->colors[c].b;
					}
					break;
				case 3:
					memcpy(row, (char *)preview->pixels + i * UI.Minimap.W, UI.Minimap.W * 3);
					break;
				case 4:
					for (int j = 0; j < UI.Minimap.W; ++j) {
						Uint32 c = ((Uint32 *)preview->pixels)[j + i * UI.Minimap.W];
						row[j * 3 + 0] = ((c & fmt->Rmask) >> fmt->Rshift);
						row[j * 3 + 1] = ((c & fmt->Gmask) >> fmt->Gshift);
						row[j * 3 + 2] = ((c & fmt->Bmask) >> fmt->Bshift);
					}
					break;
			}
			png_write_row(png_ptr, row);
		}
		delete[] row;

		SDL_UnlockSurface(preview);
		SDL_FreeSurface(preview);
	}

	png_write_end(png_ptr, info_ptr);

	/* clean up after the write, and free any memory allocated */
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);
}


// Write the map presentation file
//Wyrmgus start
//static int WriteMapPresentation(const std::string &mapname, CMap &map)
static int WriteMapPresentation(const std::string &mapname, CMap &map, bool is_mod)
//Wyrmgus end
{
	FileWriter *f = NULL;

	const char *type[] = {"", "", "neutral", "nobody",
						  "computer", "person", "rescue-passive", "rescue-active"
						 };

	int numplayers = 0;
	int topplayer = PlayerMax - 2;

	try {
		f = CreateFileWriter(mapname);
		f->printf("-- Stratagus Map Presentation\n");
		f->printf("-- File generated by the " NAME " v" VERSION " built-in map editor.\n\n");
		// MAPTODO Copyright notice in generated file

		//Wyrmgus start
		if (!is_mod) {
			f->printf("DefinePlayerTypes(");
			while (topplayer > 0 && map.Info.PlayerType[topplayer] == PlayerNobody) {
				--topplayer;
			}
			for (int i = 0; i <= topplayer; ++i) {
				f->printf("%s\"%s\"", (i ? ", " : ""), type[map.Info.PlayerType[i]]);
				if (map.Info.PlayerType[i] == PlayerPerson) {
					++numplayers;
				}
			}
			f->printf(")\n");

			f->printf("PresentMap(\"%s\", %d, %d, %d, %d)\n",
					  map.Info.Description.c_str(), numplayers, map.Info.Size.x, map.Info.Size.y,
					  map.Info.MapUID + 1);
		} else {
			f->printf("PresentMap(\"%s\")\n", map.Info.Description.c_str());
		}
		//Wyrmgus end

		//Wyrmgus start
		/*
		if (map.Info.Filename.find(".sms") == std::string::npos && !map.Info.Filename.empty()) {
			f->printf("DefineMapSetup(\"%s\")\n", map.Info.Filename.c_str());
		}
		*/
		//Wyrmgus end
	} catch (const FileException &) {
		fprintf(stderr, "ERROR: cannot write the map presentation\n");
		delete f;
		return -1;
	}

	delete f;
	return 1;
}


/**
**  Write the map setup file.
**
**  @param mapsetup      map filename
**  @param map           map to save
**  @param writeTerrain  write the tiles map in the .sms
*/
//Wyrmgus start
//int WriteMapSetup(const char *mapSetup, CMap &map, int writeTerrain)
int WriteMapSetup(const char *mapSetup, CMap &map, int writeTerrain, bool is_mod)
//Wyrmgus end
{
	FileWriter *f = NULL;

	try {
		f = CreateFileWriter(mapSetup);

		f->printf("-- Stratagus Map Setup\n");
		f->printf("-- File generated by the " NAME " v" VERSION " built-in map editor.\n\n");
		// MAPTODO Copyright notice in generated file
		
		//Wyrmgus start
		std::string mod_file(mapSetup);
		mod_file = FindAndReplaceStringBeginning(mod_file, StratagusLibPath + "/", "");
		
		for (int i = 0; i < MAX_RACES; ++i) {
			for (size_t j = 0; j < PlayerRaces.Factions[i].size(); ++j) {
				if (PlayerRaces.Factions[i][j]->Mod != Map.Info.Filename) {
					continue;
				}
				
				f->printf("DefineFaction(\"%s\", {\n", PlayerRaces.Factions[i][j]->Ident.c_str());
				f->printf("\tName = \"%s\",\n", PlayerRaces.Factions[i][j]->Name.c_str());
				f->printf("\tCivilization = \"%s\",\n", PlayerRaces.Name[i].c_str());
				if (PlayerRaces.Factions[i][j]->Type != FactionTypeNoFactionType) {
					f->printf("\tType = \"%s\",\n", GetFactionTypeNameById(PlayerRaces.Factions[i][j]->Type).c_str());
				}
				if (PlayerRaces.Factions[i][j]->ParentFaction != -1) {
					f->printf("\tParentFaction = \"%s\",\n", PlayerRaces.Factions[i][PlayerRaces.Factions[i][j]->ParentFaction]->Ident.c_str());
				}
				if (PlayerRaces.Factions[i][j]->Colors.size() > 0) {
					f->printf("\tColors = {");
					for (size_t k = 0; k < PlayerRaces.Factions[i][j]->Colors.size(); ++k) {
						f->printf("\"%s\", ", PlayerColorNames[PlayerRaces.Factions[i][j]->Colors[k]].c_str());
					}
					f->printf("},\n");
				}
				if (!PlayerRaces.Factions[i][j]->FactionUpgrade.empty()) {
					f->printf("\tFactionUpgrade = \"%s\",\n", PlayerRaces.Factions[i][j]->FactionUpgrade.c_str());
				}
				f->printf("\tMod = \"%s\"\n", mod_file.c_str());
				f->printf("})\n\n");
			}
		}
		
		for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
			CUnitType &type = *UnitTypes[i];
			
			if (type.Mod != Map.Info.Filename) {
				continue;
			}
			
			f->printf("DefineUnitType(\"%s\", {\n", type.Ident.c_str());
			if (!type.Parent.empty()) {
				f->printf("\tParent = \"%s\",\n", type.Parent.c_str());
			}
			CUnitType *parent_type = UnitTypeByIdent(type.Parent);
			if (!type.Name.empty() && (parent_type == NULL || type.Name != parent_type->Name)) {
				f->printf("\tName = \"%s\",\n", type.Name.c_str());
			}
			if (type.Civilization != -1) {
				f->printf("\tCivilization = \"%s\",\n", PlayerRaces.Name[type.Civilization].c_str());
			}
			if (type.Faction != -1) {
				f->printf("\tFaction = \"%s\",\n", PlayerRaces.Factions[type.Civilization][type.Faction]->Ident.c_str());
			}
			if (type.Class != -1) {
				f->printf("\tClass = \"%s\",\n", UnitTypeClasses[type.Class].c_str());
			}
			if (!type.File.empty() && (parent_type == NULL || type.File != parent_type->File)) {
				f->printf("\tImage = {\"file\", \"%s\", \"size\", {%d, %d}},\n", type.File.c_str(), type.Width, type.Height);
			}
			if (!type.Icon.Name.empty() && (parent_type == NULL || type.Icon.Name != parent_type->Icon.Name)) {
				f->printf("\tIcon = \"%s\",\n", type.Icon.Name.c_str());
			}
			if (type.Animations != NULL && (parent_type == NULL || type.Animations != parent_type->Animations)) {
				f->printf("\tAnimations = \"%s\",\n", type.Animations->Ident.c_str());
			}
			
			f->printf("\tCosts = {");
			for (unsigned int j = 0; j < MaxCosts; ++j) {
				if (type.DefaultStat.Costs[j] != 0 && (parent_type == NULL || type.DefaultStat.Costs[j] != parent_type->DefaultStat.Costs[j])) {
					f->printf("\"%s\", ", DefaultResourceNames[j].c_str());
					f->printf("%d, ", type.DefaultStat.Costs[j]);
				}
			}
			f->printf("},\n");
			f->printf("\tImproveProduction = {");
			for (unsigned int j = 0; j < MaxCosts; ++j) {
				if (type.DefaultStat.ImproveIncomes[j] != 0 && (parent_type == NULL || type.DefaultStat.ImproveIncomes[j] != parent_type->DefaultStat.ImproveIncomes[j])) {
					f->printf("\"%s\", ", DefaultResourceNames[j].c_str());
					f->printf("%d, ", type.DefaultStat.ImproveIncomes[j]);
				}
			}
			f->printf("},\n");
			f->printf("\tUnitStock = {");
			for (size_t j = 0; j < UnitTypes.size(); ++j) {
				if (type.DefaultStat.UnitStock[j] != 0 && (parent_type == NULL || type.DefaultStat.UnitStock[j] != parent_type->DefaultStat.UnitStock[j])) {
					f->printf("\"%s\", ", UnitTypes[j]->Ident.c_str());
					f->printf("%d, ", type.DefaultStat.UnitStock[j]);
				}
			}
			f->printf("},\n");
			
			for (size_t j = 0; j < UnitTypeVar.GetNumberVariable(); ++j) {
				if (parent_type == NULL || type.DefaultStat.Variables[j] != parent_type->DefaultStat.Variables[j]) {
					if (type.DefaultStat.Variables[j].Increase != 0 || type.DefaultStat.Variables[j].Value != type.DefaultStat.Variables[j].Max) {
						f->printf("\t%s = {", UnitTypeVar.VariableNameLookup[j]);
						f->printf("Enable = %s, ", type.DefaultStat.Variables[j].Enable ? "true" : "false");
						f->printf("Max = %d, ", type.DefaultStat.Variables[j].Max);
						f->printf("Value = %d, ", type.DefaultStat.Variables[j].Value);
						f->printf("Increase = %d", type.DefaultStat.Variables[j].Increase);
						f->printf("},\n");
					} else if (type.DefaultStat.Variables[j].Value != 0) {
						f->printf("\t%s = %d,\n", UnitTypeVar.VariableNameLookup[j], type.DefaultStat.Variables[j].Value);
					}
				}
			}
			
			if (type.ButtonPos != 0 && (parent_type == NULL || type.ButtonPos != parent_type->ButtonPos)) {
				f->printf("\tButtonPos = %d,\n", type.ButtonPos);
			}
			if (!type.ButtonKey.empty() && (parent_type == NULL || type.ButtonKey != parent_type->ButtonKey)) {
				f->printf("\tButtonKey = \"%s\",\n", type.ButtonKey.c_str());
			}
			if (!type.ButtonHint.empty() && (parent_type == NULL || type.ButtonKey != parent_type->ButtonHint)) {
				f->printf("\tButtonHint = \"%s\",\n", type.ButtonHint.c_str());
			}
			
			f->printf("\tSounds = {\n");
			if (!type.Sound.Selected.Name.empty() && (parent_type == NULL || type.Sound.Selected.Name != parent_type->Sound.Selected.Name)) {
				f->printf("\t\t\"selected\", \"%s\",\n", type.Sound.Selected.Name.c_str());
			}
			if (!type.Sound.Acknowledgement.Name.empty() && (parent_type == NULL || type.Sound.Acknowledgement.Name != parent_type->Sound.Acknowledgement.Name)) {
				f->printf("\t\t\"acknowledge\", \"%s\",\n", type.Sound.Acknowledgement.Name.c_str());
			}
			if (!type.Sound.Attack.Name.empty() && (parent_type == NULL || type.Sound.Attack.Name != parent_type->Sound.Attack.Name)) {
				f->printf("\t\t\"attack\", \"%s\",\n", type.Sound.Attack.Name.c_str());
			}
			if (!type.Sound.Idle.Name.empty() && (parent_type == NULL || type.Sound.Idle.Name != parent_type->Sound.Idle.Name)) {
				f->printf("\t\t\"idle\", \"%s\",\n", type.Sound.Idle.Name.c_str());
			}
			if (!type.Sound.Hit.Name.empty() && (parent_type == NULL || type.Sound.Hit.Name != parent_type->Sound.Hit.Name)) {
				f->printf("\t\t\"hit\", \"%s\",\n", type.Sound.Hit.Name.c_str());
			}
			if (!type.Sound.Miss.Name.empty() && (parent_type == NULL || type.Sound.Miss.Name != parent_type->Sound.Miss.Name)) {
				f->printf("\t\t\"miss\", \"%s\",\n", type.Sound.Miss.Name.c_str());
			}
			if (!type.Sound.Step.Name.empty() && (parent_type == NULL || type.Sound.Step.Name != parent_type->Sound.Step.Name)) {
				f->printf("\t\t\"step\", \"%s\",\n", type.Sound.Step.Name.c_str());
			}
			if (!type.Sound.StepDirt.Name.empty() && (parent_type == NULL || type.Sound.StepDirt.Name != parent_type->Sound.StepDirt.Name)) {
				f->printf("\t\t\"step-dirt\", \"%s\",\n", type.Sound.StepDirt.Name.c_str());
			}
			if (!type.Sound.StepGrass.Name.empty() && (parent_type == NULL || type.Sound.StepGrass.Name != parent_type->Sound.StepGrass.Name)) {
				f->printf("\t\t\"step-grass\", \"%s\",\n", type.Sound.StepGrass.Name.c_str());
			}
			if (!type.Sound.StepGravel.Name.empty() && (parent_type == NULL || type.Sound.StepGravel.Name != parent_type->Sound.StepGravel.Name)) {
				f->printf("\t\t\"step-gravel\", \"%s\",\n", type.Sound.StepGravel.Name.c_str());
			}
			if (!type.Sound.StepMud.Name.empty() && (parent_type == NULL || type.Sound.StepMud.Name != parent_type->Sound.StepMud.Name)) {
				f->printf("\t\t\"step-mud\", \"%s\",\n", type.Sound.StepMud.Name.c_str());
			}
			if (!type.Sound.StepStone.Name.empty() && (parent_type == NULL || type.Sound.StepStone.Name != parent_type->Sound.StepStone.Name)) {
				f->printf("\t\t\"step-stone\", \"%s\",\n", type.Sound.StepStone.Name.c_str());
			}
			if (!type.Sound.Used.Name.empty() && (parent_type == NULL || type.Sound.Used.Name != parent_type->Sound.Used.Name)) {
				f->printf("\t\t\"used\", \"%s\",\n", type.Sound.Used.Name.c_str());
			}
			if (!type.Sound.Build.Name.empty() && (parent_type == NULL || type.Sound.Build.Name != parent_type->Sound.Build.Name)) {
				f->printf("\t\t\"build\", \"%s\",\n", type.Sound.Build.Name.c_str());
			}
			if (!type.Sound.Ready.Name.empty() && (parent_type == NULL || type.Sound.Ready.Name != parent_type->Sound.Ready.Name)) {
				f->printf("\t\t\"ready\", \"%s\",\n", type.Sound.Ready.Name.c_str());
			}
			if (!type.Sound.Repair.Name.empty() && (parent_type == NULL || type.Sound.Repair.Name != parent_type->Sound.Repair.Name)) {
				f->printf("\t\t\"repair\", \"%s\",\n", type.Sound.Repair.Name.c_str());
			}
			for (unsigned int j = 0; j < MaxCosts; ++j) {
				if (!type.Sound.Harvest[j].Name.empty() && (parent_type == NULL || type.Sound.Harvest[j].Name != parent_type->Sound.Harvest[j].Name)) {
					f->printf("\t\t\"harvest\", \"%s\", \"%s\",\n", DefaultResourceNames[j].c_str(), type.Sound.Harvest[j].Name.c_str());
				}
			}
			if (!type.Sound.Help.Name.empty() && (parent_type == NULL || type.Sound.Help.Name != parent_type->Sound.Help.Name)) {
				f->printf("\t\t\"help\", \"%s\",\n", type.Sound.Help.Name.c_str());
			}
			if (!type.Sound.Dead[ANIMATIONS_DEATHTYPES].Name.empty() && (parent_type == NULL || type.Sound.Dead[ANIMATIONS_DEATHTYPES].Name != parent_type->Sound.Dead[ANIMATIONS_DEATHTYPES].Name)) {
				f->printf("\t\t\"dead\", \"%s\",\n", type.Sound.Dead[ANIMATIONS_DEATHTYPES].Name.c_str());
			}
			int death;
			for (death = 0; death < ANIMATIONS_DEATHTYPES; ++death) {
				if (!type.Sound.Dead[death].Name.empty() && (parent_type == NULL || type.Sound.Dead[death].Name != parent_type->Sound.Dead[death].Name)) {
					f->printf("\t\t\"dead\", \"%s\", \"%s\",\n", ExtraDeathTypes[death].c_str(), type.Sound.Dead[death].Name.c_str());
				}
			}
			f->printf("\t},\n");
			
			f->printf("\tMod = \"%s\"\n", mod_file.c_str());
			f->printf("})\n\n");
		}
		
		//save the definition of trained unit types separately, to avoid issues like a trained unit being defined after the unit that trains it
		for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
			CUnitType &type = *UnitTypes[i];
			
			if (type.Mod != Map.Info.Filename) {
				continue;
			}
			
			if (type.Trains.size() > 0) {
				f->printf("DefineUnitType(\"%s\", {\n", type.Ident.c_str());
				
				f->printf("\tTrains = {");
				for (size_t j = 0; j < type.Trains.size(); ++j) {
					f->printf("\"%s\", ", type.Trains[j]->Ident.c_str());
				}
				f->printf("}\n");
				
				f->printf("})\n\n");
			}
		}		

		for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
			CUnitType &type = *UnitTypes[i];
			
			if (type.ModTrains.find(Map.Info.Filename) != type.ModTrains.end()) {
				f->printf("SetModTrains(\"%s\", \"%s\", {", mod_file.c_str(), type.Ident.c_str());
				for (size_t j = 0; j < type.ModTrains[Map.Info.Filename].size(); ++j) {
					f->printf("\"%s\", ", type.ModTrains[Map.Info.Filename][j]->Ident.c_str());
				}
				f->printf("})\n\n");
			}
			
			if (type.ModAiDrops.find(Map.Info.Filename) != type.ModAiDrops.end()) {
				f->printf("SetModAiDrops(\"%s\", \"%s\", {", mod_file.c_str(), type.Ident.c_str());
				for (size_t j = 0; j < type.ModAiDrops[Map.Info.Filename].size(); ++j) {
					f->printf("\"%s\", ", type.ModAiDrops[Map.Info.Filename][j]->Ident.c_str());
				}
				f->printf("})\n\n");
			}
		}
		//Wyrmgus end

		//Wyrmgus start
		/*
		f->printf("-- player configuration\n");
		for (int i = 0; i < PlayerMax; ++i) {
			if (Map.Info.PlayerType[i] == PlayerNobody) {
				continue;
			}
			f->printf("SetStartView(%d, %d, %d)\n", i, Players[i].StartPos.x, Players[i].StartPos.y);
			f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
					  i, DefaultResourceNames[WoodCost].c_str(),
					  Players[i].Resources[WoodCost]);
			f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
					  i, DefaultResourceNames[GoldCost].c_str(),
					  Players[i].Resources[GoldCost]);
			f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
					  i, DefaultResourceNames[OilCost].c_str(),
					  Players[i].Resources[OilCost]);
			f->printf("SetPlayerData(%d, \"RaceName\", \"%s\")\n",
					  i, PlayerRaces.Name[Players[i].Race].c_str());
			f->printf("SetAiType(%d, \"%s\")\n",
					  i, Players[i].AiName.c_str());
		}
		f->printf("\n");

		f->printf("-- load tilesets\n");
		f->printf("LoadTileModels(\"%s\")\n\n", map.TileModelsFileName.c_str());
		*/
		if (!is_mod) {
			f->printf("-- player configuration\n");
			for (int i = 0; i < PlayerMax; ++i) {
				if (Map.Info.PlayerType[i] == PlayerNobody) {
					continue;
				}
				f->printf("SetStartView(%d, %d, %d)\n", i, Players[i].StartPos.x, Players[i].StartPos.y);
				f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
						  i, DefaultResourceNames[WoodCost].c_str(),
						  Players[i].Resources[WoodCost]);
				f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
						  i, DefaultResourceNames[CopperCost].c_str(),
						  Players[i].Resources[CopperCost]);
				if (Players[i].Resources[OilCost]) {
					f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
							  i, DefaultResourceNames[OilCost].c_str(),
							  Players[i].Resources[OilCost]);
				}
				f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
						  i, DefaultResourceNames[StoneCost].c_str(),
						  Players[i].Resources[StoneCost]);
				f->printf("SetPlayerData(%d, \"RaceName\", \"%s\")\n",
						  i, PlayerRaces.Name[Players[i].Race].c_str());
				if (Players[i].Faction != -1) {
					f->printf("SetPlayerData(%d, \"Faction\", \"%s\")\n",
							  i, PlayerRaces.Factions[Players[i].Race][Players[i].Faction]->Ident.c_str());
				}
				f->printf("SetAiType(%d, \"%s\")\n",
						  i, Players[i].AiName.c_str());
			}
			f->printf("\n");

			f->printf("-- load tilesets\n");
			f->printf("LoadTileModels(\"%s\")\n\n", map.TileModelsFileName.c_str());
		}
		//Wyrmgus end

		if (writeTerrain) {
			f->printf("-- Tiles Map\n");
			//Wyrmgus start
			for (size_t z = 0; z < map.Fields.size(); ++z) {
				//Wyrmgus start
				for (int i = 0; i < map.Info.LayersSizes[z].y; ++i) {
				//Wyrmgus end
					//Wyrmgus start
					for (int j = 0; j < map.Info.LayersSizes[z].x; ++j) {
					//Wyrmgus end
						//Wyrmgus start
						const CMapField &mf = map.Fields[z][map.getIndex(j, i, z)];
	//					const int tile = mf.getGraphicTile();
	//					const int n = map.Tileset->findTileIndexByTile(tile);
						//Wyrmgus end
						const int value = mf.Value;
						//Wyrmgus start
	//					f->printf("SetTile(%3d, %d, %d, %d)\n", n, j, i, value);
						f->printf("SetTileTerrain(\"%s\", %d, %d, %d, %d)\n", mf.Terrain->Ident.c_str(), j, i, 0, z);
						if (mf.OverlayTerrain) {
							f->printf("SetTileTerrain(\"%s\", %d, %d, %d, %d)\n", mf.OverlayTerrain->Ident.c_str(), j, i, value, z);
						}
						//Wyrmgus end
					}
				}
			}
			//Wyrmgus end
		}
			
		f->printf("\n-- set map default stat and map sound for unit types\n");
		for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
			CUnitType &type = *UnitTypes[i];
			
			if (type.ModDefaultStats.find(Map.Info.Filename) != type.ModDefaultStats.end()) {
				for (unsigned int j = 0; j < MaxCosts; ++j) {
					if (type.ModDefaultStats[Map.Info.Filename].Costs[j] != 0) {
						f->printf("SetModStat(\"%s\", \"%s\", \"Costs\", %d, \"%s\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModDefaultStats[Map.Info.Filename].Costs[j], DefaultResourceNames[j].c_str());
					}
				}
				for (unsigned int j = 0; j < MaxCosts; ++j) {
					if (type.ModDefaultStats[Map.Info.Filename].ImproveIncomes[j] != 0) {
						f->printf("SetModStat(\"%s\", \"%s\", \"ImproveProduction\", %d, \"%s\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModDefaultStats[Map.Info.Filename].ImproveIncomes[j], DefaultResourceNames[j].c_str());
					}
				}
				for (size_t j = 0; j < UnitTypes.size(); ++j) {
					if (type.ModDefaultStats[Map.Info.Filename].UnitStock[j] != 0) {
						f->printf("SetModStat(\"%s\", \"%s\", \"UnitStock\", %d, \"%s\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModDefaultStats[Map.Info.Filename].UnitStock[j], UnitTypes[j]->Ident.c_str());
					}
				}
				for (size_t j = 0; j < UnitTypeVar.GetNumberVariable(); ++j) {
					if (type.ModDefaultStats[Map.Info.Filename].Variables[j].Value != 0) {
						f->printf("SetModStat(\"%s\", \"%s\", \"%s\", %d, \"Value\")\n", mod_file.c_str(), type.Ident.c_str(), UnitTypeVar.VariableNameLookup[j], type.ModDefaultStats[Map.Info.Filename].Variables[j].Value);
					}
					if (type.ModDefaultStats[Map.Info.Filename].Variables[j].Max != 0) {
						f->printf("SetModStat(\"%s\", \"%s\", \"%s\", %d, \"Max\")\n", mod_file.c_str(), type.Ident.c_str(), UnitTypeVar.VariableNameLookup[j], type.ModDefaultStats[Map.Info.Filename].Variables[j].Max);
					}
					if (type.ModDefaultStats[Map.Info.Filename].Variables[j].Enable != 0 && (type.ModDefaultStats[Map.Info.Filename].Variables[j].Value != 0 || type.ModDefaultStats[Map.Info.Filename].Variables[j].Max != 0 || type.ModDefaultStats[Map.Info.Filename].Variables[j].Increase != 0)) {
						f->printf("SetModStat(\"%s\", \"%s\", \"%s\", %d, \"Enable\")\n", mod_file.c_str(), type.Ident.c_str(), UnitTypeVar.VariableNameLookup[j], type.ModDefaultStats[Map.Info.Filename].Variables[j].Enable);
					}
					if (type.ModDefaultStats[Map.Info.Filename].Variables[j].Increase != 0) {
						f->printf("SetModStat(\"%s\", \"%s\", \"%s\", %d, \"Increase\")\n", mod_file.c_str(), type.Ident.c_str(), UnitTypeVar.VariableNameLookup[j], type.ModDefaultStats[Map.Info.Filename].Variables[j].Increase);
					}
				}
			}
			
			if (!type.ModSounds[Map.Info.Filename].Selected.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"selected\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].Selected.Name.c_str());
			}
			if (!type.ModSounds[Map.Info.Filename].Acknowledgement.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"acknowledge\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].Acknowledgement.Name.c_str());
			}
			if (!type.ModSounds[Map.Info.Filename].Attack.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"attack\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].Attack.Name.c_str());
			}
			//Wyrmgus start
			if (!type.ModSounds[Map.Info.Filename].Idle.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"idle\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].Idle.Name.c_str());
			}
			if (!type.ModSounds[Map.Info.Filename].Hit.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"hit\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].Hit.Name.c_str());
			}
			if (!type.ModSounds[Map.Info.Filename].Miss.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"miss\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].Miss.Name.c_str());
			}
			if (!type.ModSounds[Map.Info.Filename].Step.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"step\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].Step.Name.c_str());
			}
			if (!type.ModSounds[Map.Info.Filename].StepDirt.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"step-dirt\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].StepDirt.Name.c_str());
			}
			if (!type.ModSounds[Map.Info.Filename].StepGrass.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"step-grass\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].StepGrass.Name.c_str());
			}
			if (!type.ModSounds[Map.Info.Filename].StepGravel.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"step-gravel\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].StepGravel.Name.c_str());
			}
			if (!type.ModSounds[Map.Info.Filename].StepMud.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"step-mud\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].StepMud.Name.c_str());
			}
			if (!type.ModSounds[Map.Info.Filename].StepStone.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"step-stone\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].StepStone.Name.c_str());
			}
			if (!type.ModSounds[Map.Info.Filename].Used.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"used\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].Used.Name.c_str());
			}
			//Wyrmgus end
			if (!type.ModSounds[Map.Info.Filename].Build.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"build\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].Build.Name.c_str());
			}
			if (!type.ModSounds[Map.Info.Filename].Ready.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"ready\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].Ready.Name.c_str());
			}
			if (!type.ModSounds[Map.Info.Filename].Repair.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"repair\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].Repair.Name.c_str());
			}
			for (unsigned int j = 0; j < MaxCosts; ++j) {
				if (!type.ModSounds[Map.Info.Filename].Harvest[j].Name.empty()) {
					f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"harvest\", \"%s\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].Harvest[j].Name.c_str(), DefaultResourceNames[j].c_str());
				}
			}
			if (!type.ModSounds[Map.Info.Filename].Help.Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"help\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].Help.Name.c_str());
			}
			if (!type.ModSounds[Map.Info.Filename].Dead[ANIMATIONS_DEATHTYPES].Name.empty()) {
				f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"dead\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].Dead[ANIMATIONS_DEATHTYPES].Name.c_str());
			}
			int death;
			for (death = 0; death < ANIMATIONS_DEATHTYPES; ++death) {
				if (!type.ModSounds[Map.Info.Filename].Dead[death].Name.empty()) {
					f->printf("SetModSound(\"%s\", \"%s\", \"%s\", \"dead\", \"%s\")\n", mod_file.c_str(), type.Ident.c_str(), type.ModSounds[Map.Info.Filename].Dead[death].Name.c_str(), ExtraDeathTypes[death].c_str());
				}
			}
		}
		
		//Wyrmgus start
		/*
		f->printf("\n-- place units\n");
		f->printf("if (MapUnitsInit ~= nil) then MapUnitsInit() end\n");
		std::vector<CUnit *> teleporters;
		for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
			const CUnit &unit = **it;
			f->printf("unit = CreateUnit(\"%s\", %d, {%d, %d})\n",
					  unit.Type->Ident.c_str(),
					  unit.Player->Index,
					  unit.tilePos.x, unit.tilePos.y);
			if (unit.Type->GivesResource) {
				f->printf("SetResourcesHeld(unit, %d)\n", unit.ResourcesHeld);
			}
			if (!unit.Active) { //Active is true by default
				f->printf("SetUnitVariable(unit, \"Active\", false)\n");
			}
			if (unit.Type->BoolFlag[TELEPORTER_INDEX].value && unit.Goal) {
				teleporters.push_back(*it);
			}
		}
		f->printf("\n\n");
		for (std::vector<CUnit *>::iterator it = teleporters.begin(); it != teleporters.end(); ++it) {
			CUnit &unit = **it;
			f->printf("SetTeleportDestination(%d, %d)\n", UnitNumber(unit), UnitNumber(*unit.Goal));
		}
		f->printf("\n\n");
		*/
		if (!is_mod) {
			f->printf("\n-- place units\n");
			f->printf("if (MapUnitsInit ~= nil) then MapUnitsInit() end\n");
			std::vector<CUnit *> teleporters;
			for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
				const CUnit &unit = **it;
				f->printf("unit = CreateUnit(\"%s\", %d, {%d, %d})\n",
						  unit.Type->Ident.c_str(),
						  unit.Player->Index,
						  unit.tilePos.x, unit.tilePos.y);
				if (unit.Type->GivesResource) {
					f->printf("SetResourcesHeld(unit, %d)\n", unit.ResourcesHeld);
				}
				if (!unit.Active) { //Active is true by default
					f->printf("SetUnitVariable(unit, \"Active\", false)\n");
				}
				if (unit.Character != NULL) {
					if (!unit.Character->Custom) {
						f->printf("SetUnitVariable(unit, \"Character\", \"%s\")\n", unit.Character->Ident.c_str());
					} else {
						f->printf("SetUnitVariable(unit, \"CustomHero\", \"%s\")\n", unit.Character->Ident.c_str());
					}
				} else {
					if (!unit.Name.empty()) {
						f->printf("SetUnitVariable(unit, \"Name\", \"%s\")\n", unit.Name.c_str());
					}
				}
				if (unit.Trait != NULL) {
					f->printf("AcquireTrait(unit, \"%s\")\n", unit.Trait->Ident.c_str());
				}
				if (unit.Prefix != NULL) {
					f->printf("SetUnitVariable(unit, \"Prefix\", \"%s\")\n", unit.Prefix->Ident.c_str());
				}
				if (unit.Suffix != NULL) {
					f->printf("SetUnitVariable(unit, \"Suffix\", \"%s\")\n", unit.Suffix->Ident.c_str());
				}
				if (unit.Work != NULL) {
					f->printf("SetUnitVariable(unit, \"Work\", \"%s\")\n", unit.Work->Ident.c_str());
				}
				if (unit.Elixir != NULL) {
					f->printf("SetUnitVariable(unit, \"Elixir\", \"%s\")\n", unit.Elixir->Ident.c_str());
				}
				if (unit.Variable[HP_INDEX].Value != unit.GetModifiedVariable(HP_INDEX, VariableMax)) {
					f->printf("SetUnitVariable(unit, \"HitPoints\", %d)\n", unit.Variable[HP_INDEX].Value);
				}
				if (unit.Type->BoolFlag[TELEPORTER_INDEX].value && unit.Goal) {
					teleporters.push_back(*it);
				}
			}
			f->printf("\n\n");
			for (std::vector<CUnit *>::iterator it = teleporters.begin(); it != teleporters.end(); ++it) {
				CUnit &unit = **it;
				f->printf("SetTeleportDestination(%d, %d)\n", UnitNumber(unit), UnitNumber(*unit.Goal));
			}
			f->printf("\n\n");
		}
		//Wyrmgus end
	} catch (const FileException &) {
		fprintf(stderr, "Can't save map setup : '%s' \n", mapSetup);
		delete f;
		return -1;
	}

	delete f;
	return 1;
}



/**
**  Save a Stratagus map.
**
**  @param mapName   map filename
**  @param map       map to save
**  @param writeTerrain   write the tiles map in the .sms
*/
//Wyrmgus start
//int SaveStratagusMap(const std::string &mapName, CMap &map, int writeTerrain)
int SaveStratagusMap(const std::string &mapName, CMap &map, int writeTerrain, bool is_mod)
//Wyrmgus end
{
	if (!map.Info.Size.x || !map.Info.Size.y) {
		fprintf(stderr, "%s: invalid Stratagus map\n", mapName.c_str());
		ExitFatal(-1);
	}

	char mapSetup[PATH_MAX];
	strcpy_s(mapSetup, sizeof(mapSetup), mapName.c_str());
	char *setupExtension = strstr(mapSetup, ".smp");
	if (!setupExtension) {
		fprintf(stderr, "%s: invalid Stratagus map filename\n", mapName.c_str());
	}

	//Wyrmgus start
	//don't write map previews
	/*
	char previewName[PATH_MAX];
	strcpy_s(previewName, sizeof(previewName), mapName.c_str());
	char *previewExtension = strstr(previewName, ".smp");
	memcpy(previewExtension, ".png\0", 5 * sizeof(char));
	WriteMapPreview(previewName, map);
	*/
	//Wyrmgus end

	memcpy(setupExtension, ".sms", 4 * sizeof(char));
	//Wyrmgus start
//	if (WriteMapPresentation(mapName, map) == -1) {
	if (WriteMapPresentation(mapName, map, is_mod) == -1) {
	//Wyrmgus end
		return -1;
	}

	//Wyrmgus start
//	return WriteMapSetup(mapSetup, map, writeTerrain);
	return WriteMapSetup(mapSetup, map, writeTerrain, is_mod);
	//Wyrmgus end
}


/**
**  Load any map.
**
**  @param filename  map filename
**  @param map       map loaded
*/
//Wyrmgus start
//static void LoadMap(const std::string &filename, CMap &map)
static void LoadMap(const std::string &filename, CMap &map, bool is_mod)
//Wyrmgus end
{
	const char *name = filename.c_str();
	const char *tmp = strrchr(name, '.');
	if (tmp) {
#ifdef USE_ZLIB
		if (!strcmp(tmp, ".gz")) {
			while (tmp - 1 > name && *--tmp != '.') {
			}
		}
#endif
#ifdef USE_BZ2LIB
		if (!strcmp(tmp, ".bz2")) {
			while (tmp - 1 > name && *--tmp != '.') {
			}
		}
#endif
		if (!strcmp(tmp, ".smp")
#ifdef USE_ZLIB
			|| !strcmp(tmp, ".smp.gz")
#endif
#ifdef USE_BZ2LIB
			|| !strcmp(tmp, ".smp.bz2")
#endif
		   ) {
			if (map.Info.Filename.empty()) {
				// The map info hasn't been loaded yet => do it now
				LoadStratagusMapInfo(filename);
			}
			Assert(!map.Info.Filename.empty());
			//Wyrmgus start
//			map.Create();
//			LoadStratagusMap(filename, map.Info.Filename);
			if (!is_mod) {
				map.Create();
				LoadStratagusMap(filename, map.Info.Filename);
			} else {
				DisableMod(LibraryFileName(map.Info.Filename.c_str()));
				LuaLoadFile(LibraryFileName(map.Info.Filename.c_str()));
			}
			//Wyrmgus end
			return;
		}
	}

	fprintf(stderr, "Unrecognized map format\n");
	ExitFatal(-1);
}

/**
**  Set the game paused or unpaused
**
**  @param paused  True to pause game, false to unpause.
*/
void SetGamePaused(bool paused)
{
	//Wyrmgus start
	KeyScrollState = MouseScrollState = ScrollNone;
	//Wyrmgus end

	GamePaused = paused;
}

/**
**  Get the game paused or unpaused
**
**  @return  True if the game is paused, false otherwise
*/
bool GetGamePaused()
{
	return GamePaused;
}

/**
**  Set the game speed
**
**  @param speed  New game speed.
*/
void SetGameSpeed(int speed)
{
	if (GameCycle == 0 || FastForwardCycle < GameCycle) {
		VideoSyncSpeed = speed * 100 / CYCLES_PER_SECOND;
		SetVideoSync();
	}
}

/**
**  Get the game speed
**
**  @return  Game speed
*/
int GetGameSpeed()
{
	return CYCLES_PER_SECOND * VideoSyncSpeed / 100;
}

/*----------------------------------------------------------------------------
--  Game types
----------------------------------------------------------------------------*/

//Wyrmgus start
/**
**  Melee
*/
static void GameTypeMelee()
{
	for (int i = 0; i < PlayerMax - 1; ++i) {
		for (int j = i + 1; j < PlayerMax - 1; ++j) {
			if (Players[i].Type == PlayerComputer && Players[j].Type == PlayerComputer) {
				CommandDiplomacy(i, DiplomacyAllied, j);
				Players[i].ShareVisionWith(Players[j]);
				CommandDiplomacy(j, DiplomacyAllied, i);
				Players[j].ShareVisionWith(Players[i]);
			} else {
				CommandDiplomacy(i, DiplomacyEnemy, j);
				CommandDiplomacy(j, DiplomacyEnemy, i);
			}
		}
	}
}
//Wyrmgus end

/**
**  Free for all
*/
static void GameTypeFreeForAll()
{
	for (int i = 0; i < PlayerMax - 1; ++i) {
		for (int j = i + 1; j < PlayerMax - 1; ++j) {
			CommandDiplomacy(i, DiplomacyEnemy, j);
			CommandDiplomacy(j, DiplomacyEnemy, i);
		}
	}
}

/**
**  Top vs Bottom
*/
static void GameTypeTopVsBottom()
{
	const int middle = Map.Info.Size.y / 2;

	for (int i = 0; i < PlayerMax - 1; ++i) {
		const bool top_i = Players[i].StartPos.y <= middle;

		for (int j = i + 1; j < PlayerMax - 1; ++j) {
			const bool top_j = Players[j].StartPos.y <= middle;

			if (top_i == top_j) {
				CommandDiplomacy(i, DiplomacyAllied, j);
				Players[i].ShareVisionWith(Players[j]);
				CommandDiplomacy(j, DiplomacyAllied, i);
				Players[j].ShareVisionWith(Players[i]);
			} else {
				CommandDiplomacy(i, DiplomacyEnemy, j);
				CommandDiplomacy(j, DiplomacyEnemy, i);
			}
		}
	}
}

/**
**  Left vs Right
*/
static void GameTypeLeftVsRight()
{
	const int middle = Map.Info.Size.x / 2;

	for (int i = 0; i < PlayerMax - 1; ++i) {
		const bool left_i = Players[i].StartPos.x <= middle;

		for (int j = i + 1; j < PlayerMax - 1; ++j) {
			const bool left_j = Players[j].StartPos.x <= middle;

			if (left_i == left_j) {
				CommandDiplomacy(i, DiplomacyAllied, j);
				Players[i].ShareVisionWith(Players[j]);
				CommandDiplomacy(j, DiplomacyAllied, i);
				Players[j].ShareVisionWith(Players[i]);
			} else {
				CommandDiplomacy(i, DiplomacyEnemy, j);
				CommandDiplomacy(j, DiplomacyEnemy, i);
			}
		}
	}
}

/**
**  Man vs Machine
*/
static void GameTypeManVsMachine()
{
	for (int i = 0; i < PlayerMax - 1; ++i) {
		if (Players[i].Type != PlayerPerson && Players[i].Type != PlayerComputer) {
			continue;
		}
		for (int j = i + 1; j < PlayerMax - 1; ++j) {
			if (Players[j].Type != PlayerPerson && Players[j].Type != PlayerComputer) {
				continue;
			}
			if (Players[i].Type == Players[j].Type) {
				CommandDiplomacy(i, DiplomacyAllied, j);
				Players[i].ShareVisionWith(Players[j]);
				CommandDiplomacy(j, DiplomacyAllied, i);
				Players[j].ShareVisionWith(Players[i]);
			} else {
				CommandDiplomacy(i, DiplomacyEnemy, j);
				CommandDiplomacy(j, DiplomacyEnemy, i);
			}
		}
	}
}

/**
**  Man vs Machine with Humans on a Team
*/
static void GameTypeManTeamVsMachine()
{
	for (int i = 0; i < PlayerMax - 1; ++i) {
		if (Players[i].Type != PlayerPerson && Players[i].Type != PlayerComputer) {
			continue;
		}
		for (int j = 0; j < PlayerMax - 1; ++j) {
			if (i != j) {
				if (Players[i].Type == Players[j].Type) {
					CommandDiplomacy(i, DiplomacyAllied, j);
					Players[i].ShareVisionWith(Players[j]);
				} else {
					CommandDiplomacy(i, DiplomacyEnemy, j);
				}
			}
		}
		if (Players[i].Type == PlayerPerson) {
			Players[i].Team = 2;
		} else {
			Players[i].Team = 1;
		}
	}
}

//
// LoadingBar
// TODO: move this to a new class

static int itemsToLoad;
static int itemsLoaded;
static CGraphic *loadingEmpty = NULL;
static CGraphic *loadingFull = NULL;
static std::vector<std::string> loadingBackgrounds;
CGraphic *loadingBackground = NULL;
static CFont *loadingFont = NULL;
static std::vector<std::string> loadingTips;
static std::vector<std::string> loadingTip;

static int CclLoadingBarSetTips(lua_State *l)
{
	loadingTips.clear();
	
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	const char *str;
	int i = 1;
	do {
		lua_pushinteger(l, i++);
		lua_gettable(l, -2);
		str = lua_tostring(l, -1);
		if (str) {
			loadingTips.push_back(str);
		}
		lua_pop(l, 1);
	} while (str != NULL);

	return 0;
}

static int CclLoadingBarSetBackgrounds(lua_State *l)
{
	loadingBackgrounds.clear();
	
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	const char *str;
	int i = 1;
	do {
		lua_pushinteger(l, i++);
		lua_gettable(l, -2);
		str = lua_tostring(l, -1);
		if (str) {
			loadingBackgrounds.push_back(str);
		}
		lua_pop(l, 1);
	} while (str != NULL);

	return 0;
}

void CalculateItemsToLoad(bool grand_strategy)
{
	ResetItemsToLoad();

	if (loadingBackgrounds.size() > 0) {
		std::string loading_background_string = loadingBackgrounds[rand()%loadingBackgrounds.size()];
		if (CanAccessFile(loading_background_string.c_str())) {
			loadingBackground = CGraphic::New(loading_background_string);
			loadingBackground->Load();
			loadingBackground->Resize(Video.Width, Video.Height);
		}
	}
	
	if (CanAccessFile("ui/loadingEmpty.png") && CanAccessFile("ui/loadingFull.png")) {
		if (!grand_strategy) {
			itemsToLoad+= GetIconsCount();
			if (ThisPlayer) {
				itemsToLoad+= GetCursorsCount(PlayerRaces.Name[ThisPlayer->Race]);
			}
			itemsToLoad+= GetUnitTypesCount();
			itemsToLoad+= GetDecorationsCount();
			itemsToLoad+= GetConstructionsCount();
			itemsToLoad+= GetMissileSpritesCount();
		}
		
		loadingEmpty = CGraphic::New("ui/loadingEmpty.png");
		loadingEmpty->Load();
		loadingFull = CGraphic::New("ui/loadingFull.png");
		loadingFull->Load();
	}

	if (loadingTips.size() > 0) {
		std::string base_loadingTip = loadingTips[rand()%loadingTips.size()];
		
		int str_width_per_total_width = 1;
		str_width_per_total_width += GetGameFont().Width(base_loadingTip) / Video.Width;
		
	//	int line_length = Video.Width / GetGameFont().Width(1);
		int line_length = base_loadingTip.size() / str_width_per_total_width;
		
		int begin = 0;
		for (int i = 0; i < str_width_per_total_width; ++i) {
			int end = base_loadingTip.size();
			
			if (i != (str_width_per_total_width - 1)) {
				end = (i + 1) * line_length;
				while (base_loadingTip.substr(end - 1, 1) != " ") {
					end -= 1;
				}
			}
			
			loadingTip.push_back(base_loadingTip.substr(begin, end - begin));
			
			begin = end;
		}
	}
}

void UpdateLoadingBar()
{
	int y = Video.Height/2;
	
	if (loadingBackground != NULL) {
		loadingBackground->DrawClip(0, 0);
	}
	
	if (itemsToLoad > 0) {
		int x = Video.Width/2 - loadingEmpty->Width/2;
		int pct = (itemsLoaded * 100) / itemsToLoad;
		
		loadingEmpty->DrawClip(x, y - loadingEmpty->Height/2);
		loadingFull->DrawSub(0, 0, (loadingFull->Width * pct) / 100, loadingFull->Height, x, y - loadingEmpty->Height/2);
		y+= loadingEmpty->Height/2;
	}

	if (loadingFont == NULL) {
		loadingFont = CFont::Get("game");
	}

	if (loadingFont != NULL) {
		CLabel label(*loadingFont);
		for (size_t i = 0; i < loadingTip.size(); ++i) {
			label.DrawCentered(Video.Width/2, y + 10 + (GetGameFont().Height() * i), loadingTip[i]);
		}
	}
}

void IncItemsLoaded()
{
	if (itemsToLoad == 0 || itemsLoaded >= itemsToLoad)
		return;

	itemsLoaded++;
}

void ResetItemsToLoad()
{
	itemsLoaded = 0;
	itemsToLoad = 0;
	loadingTip.clear();
	loadingBackground = NULL;
}

/*----------------------------------------------------------------------------
--  Game creation
----------------------------------------------------------------------------*/


/**
**  CreateGame.
**
**  Load map, graphics, sounds, etc
**
**  @param filename  map filename
**  @param map       map loaded
**
**  @todo FIXME: use in this function InitModules / LoadModules!!!
*/
//Wyrmgus start
//void CreateGame(const std::string &filename, CMap *map)
void CreateGame(const std::string &filename, CMap *map, bool is_mod)
//Wyrmgus end
{
	if (SaveGameLoading) {
		SaveGameLoading = false;
		// Load game, already created game with Init/LoadModules
		CommandLog(NULL, NoUnitP, FlushCommands, -1, -1, NoUnitP, NULL, -1);
		return;
	}

	InitPlayers();
	
	//Wyrmgus start
	if (IsNetworkGame()) { // if is a network game, it is necessary to reinitialize the syncrand variables before beginning to load the map, due to random map generation
		SyncHash = 0;
		InitSyncRand();
	}
	//Wyrmgus end

	if (Map.Info.Filename.empty() && !filename.empty()) {
		const std::string path = LibraryFileName(filename.c_str());

		if (strcasestr(filename.c_str(), ".smp")) {
			LuaLoadFile(path);
		}
	}

	for (int i = 0; i < PlayerMax; ++i) {
		int playertype = (PlayerTypes)Map.Info.PlayerType[i];
		// Network games only:
		if (GameSettings.Presets[i].Type != SettingsPresetMapDefault) {
			playertype = GameSettings.Presets[i].Type;
		}
		CreatePlayer(playertype);
	}
	
	//Wyrmgus start
	CalculateItemsToLoad();
	//Wyrmgus end

	if (!filename.empty()) {
		if (CurrentMapPath != filename) {
			strcpy_s(CurrentMapPath, sizeof(CurrentMapPath), filename.c_str());
		}

		//
		// Load the map.
		//
		//Wyrmgus start
//		InitUnitTypes(1);
		if (!is_mod) {
			InitUnitTypes(1);
		}
//		LoadMap(filename, *map);
		LoadMap(filename, *map, is_mod);
		//Wyrmgus end
		ApplyUpgrades();
	}
	CclCommand("if (MapLoaded ~= nil) then MapLoaded() end");

	GameCycle = 0;
	FastForwardCycle = 0;
	SyncHash = 0;
	InitSyncRand();

	if (IsNetworkGame()) { // Prepare network play
		NetworkOnStartGame();
	//Wyrmgus start
	/*
	} else {
		const std::string &localPlayerName = Parameters::Instance.LocalPlayerName;

		if (!localPlayerName.empty() && localPlayerName != "Anonymous") {
			ThisPlayer->SetName(localPlayerName);
		}
	*/
	//Wyrmgus end
	}

	CallbackMusicOn();

#if 0
	GamePaused = true;
#endif

	if (FlagRevealMap) {
		Map.Reveal();
	}

	//
	// Setup game types
	//
	// FIXME: implement more game types
	if (GameSettings.GameType != SettingsGameTypeMapDefault) {
		switch (GameSettings.GameType) {
			case SettingsGameTypeMelee:
				break;
			case SettingsGameTypeFreeForAll:
				GameTypeFreeForAll();
				break;
			case SettingsGameTypeTopVsBottom:
				GameTypeTopVsBottom();
				break;
			case SettingsGameTypeLeftVsRight:
				GameTypeLeftVsRight();
				break;
			case SettingsGameTypeManVsMachine:
				GameTypeManVsMachine();
				break;
			case SettingsGameTypeManTeamVsMachine:
				GameTypeManTeamVsMachine();

				// Future game type ideas
#if 0
			case SettingsGameTypeOneOnOne:
				break;
			case SettingsGameTypeCaptureTheFlag:
				break;
			case SettingsGameTypeGreed:
				break;
			case SettingsGameTypeSlaughter:
				break;
			case SettingsGameTypeSuddenDeath:
				break;
			case SettingsGameTypeTeamMelee:
				break;
			case SettingsGameTypeTeamCaptureTheFlag:
				break;
#endif
		}
	}

	//
	// Graphic part
	//
	SetPlayersPalette();
	LoadIcons();

	//Wyrmgus start
//	LoadCursors(PlayerRaces.Name[ThisPlayer->Race]);
	LoadCursors();
	//Wyrmgus end
	UnitUnderCursor = NoUnitP;

	//Wyrmgus start
	LoadTerrainTypes();
	//Wyrmgus end
	InitMissileTypes();
#ifndef DYNAMIC_LOAD
	LoadMissileSprites();
#endif
	InitConstructions();
	LoadConstructions();
	LoadUnitTypes();
	LoadDecorations();

	InitUserInterface();
	UI.Load();

	Map.Init();
	UI.Minimap.Create();
	PreprocessMap();
	
	//Wyrmgus start
	//update the sight of all units
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit &unit = **it;
		if (!unit.Destroyed) {
			MapUnmarkUnitSight(unit);
			UpdateUnitSightRange(unit);
			MapMarkUnitSight(unit);
		}
	}
	//Wyrmgus end

	//
	// Sound part
	//
	LoadUnitSounds();
	MapUnitSounds();
	if (SoundEnabled()) {
		InitSoundClient();
	}

	//
	// Spells
	//
	InitSpells();

	//
	// Init players?
	//
	DebugPlayers();
	PlayersInitAi();

	//
	// Upgrades
	//
	InitUpgrades();

	//
	// Dependencies
	//
	InitDependencies();

	//
	// Buttons (botpanel)
	//
	InitButtons();

	//
	// Triggers
	//
	InitTriggers();

	SetDefaultTextColors(UI.NormalFontColor, UI.ReverseFontColor);

#if 0
	if (!UI.SelectedViewport) {
		UI.SelectedViewport = UI.Viewports;
	}
#endif
	//Wyrmgus start
	CurrentMapLayer = ThisPlayer->StartMapLayer;
	//Wyrmgus end
	UI.SelectedViewport->Center(Map.TilePosToMapPixelPos_Center(ThisPlayer->StartPos));

	//
	// Various hacks which must be done after the map is loaded.
	//
	// FIXME: must be done after map is loaded
	InitPathfinder();
	//
	// FIXME: The palette is loaded after the units are created.
	// FIXME: This loops fixes the colors of the units.
	//
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit &unit = **it;
		// I don't really think that there can be any rescued units at this point.
		if (unit.RescuedFrom) {
			unit.Colors = &unit.RescuedFrom->UnitColors;
		} else {
			unit.Colors = &unit.Player->UnitColors;
		}
	}

	GameResult = GameNoResult;

	CommandLog(NULL, NoUnitP, FlushCommands, -1, -1, NoUnitP, NULL, -1);
	Video.ClearScreen();
	
	//Wyrmgus start
	ResetItemsToLoad();
	//Wyrmgus end
}

/**
**  Init Game Setting to default values
**
**  @todo  FIXME: this should not be executed for restart levels!
*/
void InitSettings()
{
	for (int i = 0; i < PlayerMax; ++i) {
		GameSettings.Presets[i].AIScript = "ai-passive";
		GameSettings.Presets[i].Race = SettingsPresetMapDefault;
		GameSettings.Presets[i].Team = SettingsPresetMapDefault;
		GameSettings.Presets[i].Type = SettingsPresetMapDefault;
	}
	GameSettings.Resources = SettingsPresetMapDefault;
	GameSettings.NumUnits = SettingsPresetMapDefault;
	GameSettings.Opponents = SettingsPresetMapDefault;
	GameSettings.Difficulty = SettingsPresetMapDefault;
	GameSettings.GameType = SettingsPresetMapDefault;
	GameSettings.MapRichness = SettingsPresetMapDefault;
	GameSettings.NetGameType = SettingsSinglePlayerGame;
}

// call the lua function: CleanGame_Lua.
static void CleanGame_Lua()
{
	lua_getglobal(Lua, "CleanGame_Lua");
	if (lua_isfunction(Lua, -1)) {
		LuaCall(0, 1);
	} else {
		lua_pop(Lua, 1);
	}
}

/**
**  Cleanup game.
**
**  Call each module to clean up.
**  Contrary to CleanModules, maps can be restarted
**  without reloading all lua files.
*/
void CleanGame()
{
	EndReplayLog();
	CleanMessages();

	RestoreColorCyclingSurface();
	CleanGame_Lua();
	CleanTriggers();
	CleanAi();
	CleanGroups();
	CleanMissiles();
	CleanUnits();
	CleanSelections();
	//Wyrmgus start
	DisableMod(Map.Info.Filename);
	//Wyrmgus end
	Map.Clean();
	CleanReplayLog();
	FreePathfinder();
	CursorBuilding = NULL;
	UnitUnderCursor = NULL;
	GameEstablishing = false;
}

/**
**  Return of game name.
**
**  @param l  Lua state.
*/
static int CclSetGameName(lua_State *l)
{
	const int args = lua_gettop(l);
	if (args > 1 || (args == 1 && (!lua_isnil(l, 1) && !lua_isstring(l, 1)))) {
		LuaError(l, "incorrect argument");
	}
	if (args == 1 && !lua_isnil(l, 1)) {
		GameName = lua_tostring(l, 1);
	}

	if (!GameName.empty()) {
		std::string path = Parameters::Instance.GetUserDirectory() + "/" + GameName;
		makedir(path.c_str(), 0777);
	}
	return 0;
}

static int CclSetFullGameName(lua_State *l)
{
	const int args = lua_gettop(l);
	if (args > 1 || (args == 1 && (!lua_isnil(l, 1) && !lua_isstring(l, 1)))) {
		LuaError(l, "incorrect argument");
	}
	if (args == 1 && !lua_isnil(l, 1)) {
		FullGameName = lua_tostring(l, 1);
	}
	return 0;
}

/**
**  Set God mode.
**
**  @param l  Lua state.
**
**  @return   The old mode.
*/
static int CclSetGodMode(lua_State *l)
{
	LuaCheckArgs(l, 1);
	GodMode = LuaToBoolean(l, 1);
	return 0;
}

/**
**  Get God mode.
**
**  @param l  Lua state.
**
**  @return   God mode.
*/
static int CclGetGodMode(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushboolean(l, GodMode);
	return 1;
}

/**
**  Set resource harvesting speed (deprecated).
**
**  @param l  Lua state.
*/
static int CclSetSpeedResourcesHarvest(lua_State *l)
{
	LuaCheckArgs(l, 3);

	const int player = LuaToNumber(l, 1);
	const std::string resource = LuaToString(l, 2);
	const int resId = GetResourceIdByName(l, resource.c_str());

	Players[player].SpeedResourcesHarvest[resId] = LuaToNumber(l, 3);
	return 0;
}

/**
**  Set resource returning speed (deprecated).
**
**  @param l  Lua state.
*/
static int CclSetSpeedResourcesReturn(lua_State *l)
{
	LuaCheckArgs(l, 3);

	const int player = LuaToNumber(l, 1);
	const std::string resource = LuaToString(l, 2);
	const int resId = GetResourceIdByName(l, resource.c_str());

	Players[player].SpeedResourcesReturn[resId] = LuaToNumber(l, 3);
	return 0;
}

/**
**  Set building speed (deprecated).
**
**  @param l  Lua state.
*/
static int CclSetSpeedBuild(lua_State *l)
{
	LuaCheckArgs(l, 2);
	const int player = LuaToNumber(l, 1);
	Players[player].SpeedBuild = LuaToNumber(l, 2);
	return 0;
}

/**
**  Get building speed (deprecated).
**
**  @param l  Lua state.
**
**  @return   Building speed.
*/
static int CclGetSpeedBuild(lua_State *l)
{
	LuaCheckArgs(l, 1);
	const int player = LuaToNumber(l, 1);
	lua_pushnumber(l, Players[player].SpeedBuild);
	return 1;
}

/**
**  Set training speed (deprecated).
**
**  @param l  Lua state.
*/
static int CclSetSpeedTrain(lua_State *l)
{
	LuaCheckArgs(l, 2);
	const int player = LuaToNumber(l, 1);
	Players[player].SpeedTrain = LuaToNumber(l, 2);
	return 0;
}

/**
**  Get training speed (deprecated).
**
**  @param l  Lua state.
**
**  @return   Training speed.
*/
static int CclGetSpeedTrain(lua_State *l)
{
	LuaCheckArgs(l, 1);
	const int player = LuaToNumber(l, 1);
	lua_pushnumber(l, Players[player].SpeedTrain);
	return 1;
}

/**
**  For debug increase upgrading speed (deprecated).
**
**  @param l  Lua state.
*/
static int CclSetSpeedUpgrade(lua_State *l)
{
	LuaCheckArgs(l, 2);
	const int player = LuaToNumber(l, 1);
	Players[player].SpeedUpgrade = LuaToNumber(l, 2);

	lua_pushnumber(l, Players[player].SpeedUpgrade);
	return 1;
}

/**
**  For debug increase researching speed (deprecated).
**
**  @param l  Lua state.
*/
static int CclSetSpeedResearch(lua_State *l)
{
	LuaCheckArgs(l, 2);
	const int player = LuaToNumber(l, 1);
	Players[player].SpeedResearch = LuaToNumber(l, 2);

	lua_pushnumber(l, Players[player].SpeedResearch);
	return 1;
}

/**
**  For debug increase all speeds (deprecated).
**
**  @param l  Lua state.
*/
static int CclSetSpeeds(lua_State *l)
{
	LuaCheckArgs(l, 1);
	const int speed = LuaToNumber(l, 1);
	for (int i = 0; i < PlayerMax; ++i) {
		for (int j = 0; j < MaxCosts; ++j) {
			Players[i].SpeedResourcesHarvest[j] = speed;
			Players[i].SpeedResourcesReturn[j] = speed;
		}
		Players[i].SpeedBuild = Players[i].SpeedTrain = Players[i].SpeedUpgrade = Players[i].SpeedResearch = speed;
	}

	lua_pushnumber(l, speed);
	return 1;
}

/**
**  Define default incomes for a new player.
**
**  @param l  Lua state.
*/
static int CclDefineDefaultIncomes(lua_State *l)
{
	const int args = lua_gettop(l);
	for (int i = 0; i < MaxCosts && i < args; ++i) {
		DefaultIncomes[i] = LuaToNumber(l, i + 1);
	}
	return 0;
}

/**
**  Define default action for the resources.
**
**  @param l  Lua state.
*/
static int CclDefineDefaultActions(lua_State *l)
{
	for (unsigned int i = 0; i < MaxCosts; ++i) {
		DefaultActions[i].clear();
	}
	const unsigned int args = lua_gettop(l);
	for (unsigned int i = 0; i < MaxCosts && i < args; ++i) {
		DefaultActions[i] = LuaToString(l, i + 1);
	}
	return 0;
}

/**
**  Define default names for the resources.
**
**  @param l  Lua state.
*/
static int CclDefineDefaultResourceNames(lua_State *l)
{
	for (unsigned int i = 0; i < MaxCosts; ++i) {
		DefaultResourceNames[i].clear();
	}
	const unsigned int args = lua_gettop(l);
	for (unsigned int i = 0; i < MaxCosts && i < args; ++i) {
		DefaultResourceNames[i] = LuaToString(l, i + 1);
	}
	
	//Wyrmgus start
	//initialize these variables here, for lack of a better place
	for (int i = 0; i < MaxCosts; ++i) {
		LuxuryResources[i] = false;
		DefaultResourceFinalResources[i] = i;
		DefaultResourceFinalResourceConversionRates[i] = 100;
		DefaultResourceInputResources[i] = 0;
		DefaultResourcePrices[i] = 0;
	}
	//Wyrmgus end
	
	return 0;
}

/**
**  Define default names for the resources.
**
**  @param l  Lua state.
*/
static int CclDefineDefaultResourceAmounts(lua_State *l)
{
	const unsigned int args = lua_gettop(l);

	if (args & 1) {
		LuaError(l, "incorrect argument");
	}
	for (unsigned int j = 0; j < args; ++j) {
		const std::string resource = LuaToString(l, j + 1);
		const int resId = GetResourceIdByName(l, resource.c_str());

		++j;
		DefaultResourceAmounts[resId] = LuaToNumber(l, j + 1);
	}
	return 0;
}

/**
**  Define max amounts for the resources.
**
**  @param l  Lua state.
*/
static int CclDefineDefaultResourceMaxAmounts(lua_State *l)
{
	const int args = std::min<int>(lua_gettop(l), MaxCosts);

	for (int i = 0; i < args; ++i) {
		DefaultResourceMaxAmounts[i] = LuaToNumber(l, i + 1);
	}
	for (int i = args; i < MaxCosts; ++i) {
		DefaultResourceMaxAmounts[i] = -1;
	}
	return 0;
}

//Wyrmgus start
/**
**  Defines which resources are luxury resources.
**
**  @param l  Lua state.
*/
static int CclDefineLuxuryResources(lua_State *l)
{
	const unsigned int args = lua_gettop(l);

	for (unsigned int j = 0; j < args; ++j) {
		const std::string resource = LuaToString(l, j + 1);
		const int resId = GetResourceIdByName(l, resource.c_str());

		LuxuryResources[resId] = true;
	}
	return 0;
}

//Wyrmgus end

/**
**  Affect UseHPForXp.
**
**  @param l  Lua state.
**
**  @return 0.
*/
static int ScriptSetUseHPForXp(lua_State *l)
{
	LuaCheckArgs(l, 1);
	UseHPForXp = LuaToBoolean(l, 1);
	return 0;
}

/**
**  Set the local player name
**
**  @param l  Lua state.
*/
static int CclSetLocalPlayerName(lua_State *l)
{
	LuaCheckArgs(l, 1);
	Parameters::Instance.LocalPlayerName = LuaToString(l, 1);
	return 0;
}

/**
**  Get the local player name
**
**  @param l  Lua state.
*/
static int CclGetLocalPlayerName(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushstring(l, Parameters::Instance.LocalPlayerName.c_str());
	return 1;
}

/**
**  Get Stratagus Version
*/
static int CclGetStratagusVersion(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushstring(l, VERSION);
	return 1;
}

/**
**  Get Stratagus Homepage
*/
static int CclGetStratagusHomepage(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushstring(l, HOMEPAGE);
	return 1;
}

static int CclSetMenuRace(lua_State *l)
{
	LuaCheckArgs(l, 1);
	MenuRace = LuaToString(l, 1);
	return 0;
}

/**
**  Load the SavedGameInfo Header
**
**  @param l  Lua state.
*/
static int CclSavedGameInfo(lua_State *l)
{
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}

	for (lua_pushnil(l); lua_next(l, 1); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);

		if (!strcmp(value, "SaveFile")) {
			if (strcpy_s(CurrentMapPath, sizeof(CurrentMapPath), LuaToString(l, -1)) != 0) {
				LuaError(l, "SaveFile too long");
			}
			std::string buf = StratagusLibPath;
			buf += "/";
			buf += LuaToString(l, -1);
			if (LuaLoadFile(buf) == -1) {
				DebugPrint("Load failed: %s\n" _C_ value);
			}
		} else if (!strcmp(value, "SyncHash")) {
			SyncHash = LuaToNumber(l, -1);
		} else if (!strcmp(value, "SyncRandSeed")) {
			SyncRandSeed = LuaToNumber(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	return 0;
}

//Wyrmgus start
void SetResourceFinalResource(std::string resource_name, std::string final_resource_name)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	int final_resource = GetResourceIdByName(final_resource_name.c_str());
	
	if (resource == -1 || final_resource == -1) {
		return;
	}
	
	DefaultResourceFinalResources[resource] = final_resource;
}

void SetResourceFinalResourceConversionRate(std::string resource_name, int conversion_rate)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	
	if (resource == -1) {
		return;
	}
	
	DefaultResourceFinalResourceConversionRates[resource] = conversion_rate;
}

void SetResourceInputResource(std::string resource_name, std::string input_resource_name)
{
	int resource = GetResourceIdByName(resource_name.c_str());
	int input_resource = GetResourceIdByName(input_resource_name.c_str());
	
	if (resource == -1 || input_resource == -1) {
		return;
	}
	
	DefaultResourceInputResources[resource] = input_resource;
}
//Wyrmgus end

void LuaRegisterModules()
{
	lua_register(Lua, "SetGameName", CclSetGameName);
	lua_register(Lua, "SetFullGameName", CclSetFullGameName);

	lua_register(Lua, "SetGodMode", CclSetGodMode);
	lua_register(Lua, "GetGodMode", CclGetGodMode);

	lua_register(Lua, "SetSpeedResourcesHarvest", CclSetSpeedResourcesHarvest);
	lua_register(Lua, "SetSpeedResourcesReturn", CclSetSpeedResourcesReturn);
	lua_register(Lua, "SetSpeedBuild", CclSetSpeedBuild);
	lua_register(Lua, "GetSpeedBuild", CclGetSpeedBuild);
	lua_register(Lua, "SetSpeedTrain", CclSetSpeedTrain);
	lua_register(Lua, "GetSpeedTrain", CclGetSpeedTrain);
	lua_register(Lua, "SetSpeedUpgrade", CclSetSpeedUpgrade);
	lua_register(Lua, "SetSpeedResearch", CclSetSpeedResearch);
	lua_register(Lua, "SetSpeeds", CclSetSpeeds);

	lua_register(Lua, "DefineDefaultIncomes", CclDefineDefaultIncomes);
	lua_register(Lua, "DefineDefaultActions", CclDefineDefaultActions);
	lua_register(Lua, "DefineDefaultResourceNames", CclDefineDefaultResourceNames);
	lua_register(Lua, "DefineDefaultResourceAmounts", CclDefineDefaultResourceAmounts);
	lua_register(Lua, "DefineDefaultResourceMaxAmounts", CclDefineDefaultResourceMaxAmounts);
	//Wyrmgus start
	lua_register(Lua, "DefineLuxuryResources", CclDefineLuxuryResources);
	//Wyrmgus end

	lua_register(Lua, "SetUseHPForXp", ScriptSetUseHPForXp);
	lua_register(Lua, "SetLocalPlayerName", CclSetLocalPlayerName);
	lua_register(Lua, "GetLocalPlayerName", CclGetLocalPlayerName);

	lua_register(Lua, "SetMenuRace", CclSetMenuRace);

	lua_register(Lua, "GetStratagusVersion", CclGetStratagusVersion);
	lua_register(Lua, "GetStratagusHomepage", CclGetStratagusHomepage);

	lua_register(Lua, "SavedGameInfo", CclSavedGameInfo);

	//Wyrmgus start
	lua_register(Lua, "LoadingBarSetTips", CclLoadingBarSetTips);
	lua_register(Lua, "LoadingBarSetBackgrounds", CclLoadingBarSetBackgrounds);
	//Wyrmgus end

	AiCclRegister();
	AnimationCclRegister();
	//Wyrmgus start
	CharacterCclRegister();
	//Wyrmgus end
	ConstructionCclRegister();
	DecorationCclRegister();
	DependenciesCclRegister();
	EditorCclRegister();
	//Wyrmgus start
	GrandStrategyCclRegister();
	//Wyrmgus end
	GroupCclRegister();
	//Wyrmgus start
	ItemCclRegister();
	//Wyrmgus end
	MapCclRegister();
	MissileCclRegister();
	NetworkCclRegister();
	PathfinderCclRegister();
	PlayerCclRegister();
	//Wyrmgus start
	ProvinceCclRegister();
	QuestCclRegister();
	//Wyrmgus end
	ReplayCclRegister();
	ScriptRegister();
	SelectionCclRegister();
	SoundCclRegister();
	SpellCclRegister();
	//Wyrmgus start
	TextCclRegister();
	//Wyrmgus end
	TriggerCclRegister();
	UnitCclRegister();
	UnitTypeCclRegister();
	UpgradesCclRegister();
	UserInterfaceCclRegister();
	VideoCclRegister();
}


//@}
