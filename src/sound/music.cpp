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
/**@name music.cpp - Background music support */
//
//      (c) Copyright 2002-2006 by Lutz Sammer, Nehal Mistry, and Jimmy Salmon
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
-- Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"


#include "SDL.h"

#include "sound_server.h"
#include "script.h"
#include "iolib.h"

/*----------------------------------------------------------------------------
-- Declaration
----------------------------------------------------------------------------*/

#define SoundFrequency 44100 // sample rate of dsp

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

static SDL_mutex *MusicFinishedMutex;     /// Mutex for MusicFinished
static bool MusicFinished;                /// Music ended and we need a new file

bool CallbackMusic;                       /// flag true callback ccl if stops

#ifdef USE_OAML
#include <oaml.h>

oamlApi *oaml = nullptr;
bool enableOAML = false;
#endif

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

/**
**  Callback for when music has finished
**  Note: we are in the sdl audio thread
*/
static void MusicFinishedCallback()
{
	SDL_LockMutex(MusicFinishedMutex);
	MusicFinished = true;
	SDL_UnlockMutex(MusicFinishedMutex);
}

/**
**  Check if music is finished and play the next song
*/
void CheckMusicFinished(bool force)
{
	bool proceed;

	SDL_LockMutex(MusicFinishedMutex);
	proceed = MusicFinished;
	MusicFinished = false;
	SDL_UnlockMutex(MusicFinishedMutex);

	if ((proceed || force) && SoundEnabled() && IsMusicEnabled() && CallbackMusic) {
		lua_getglobal(Lua, "MusicStopped");
		if (!lua_isfunction(Lua, -1)) {
			fprintf(stderr, "No MusicStopped function in Lua\n");
			StopMusic();
		} else {
			LuaCall(0, 1);
		}
	}
}

/**
**  Init music
*/
void InitMusic()
{
	MusicFinished = false;
	MusicFinishedMutex = SDL_CreateMutex();
	SetMusicFinishedCallback(MusicFinishedCallback);
}

#ifdef USE_OAML
static void* oamlOpen(const char *filename) {
	CFile *f = new CFile;
	const std::string name = LibraryFileName(filename);
	if (f->open(name.c_str(), CL_OPEN_READ) == -1) {
		delete f;
		return nullptr;
	}
	return (void*)f;
}

static size_t oamlRead(void *ptr, size_t size, size_t nitems, void *fd) {
	CFile *f = (CFile*)fd;
	return f->read(ptr, size*nitems);
}

static int oamlSeek(void *fd, long offset, int whence) {
	CFile *f = (CFile*)fd;
	return f->seek(offset, whence);
}

static long oamlTell(void *fd) {
	CFile *f = (CFile*)fd;
	return f->tell();
}

static int oamlClose(void *fd) {
	CFile *f = (CFile*)fd;
	int ret = f->close();
	delete f;
	return ret;
}


static oamlFileCallbacks fileCbs = {
	&oamlOpen,
	&oamlRead,
	&oamlSeek,
	&oamlTell,
	&oamlClose
};

void InitMusicOAML()
{
	const std::string filename = LibraryFileName("oaml.defs");
	oaml = new oamlApi();
	oaml->SetFileCallbacks(&fileCbs);
	oaml->Init(filename.c_str());

	enableOAML = true;
	SetMusicVolume(GetMusicVolume());
}

#endif
void LoadOAMLDefinitionsFile(const std::string &file_path)
{
#ifdef USE_OAML
	const std::string filename = LibraryFileName(file_path.c_str());
	oaml->ReadDefsFile(filename.c_str());
#endif
}

#ifdef USE_OAML
void ShutdownMusicOAML()
{
	if (oaml) {
		oaml->Shutdown();
		delete oaml;
		oaml = nullptr;
	}
	enableOAML = false;
}
#else

void InitMusicOAML()
{
}

void ShutdownMusicOAML()
{
}

#endif

//@}
