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
/**@name video.cpp - The universal video functions. */
//
//      (c) Copyright 1999-2021 by Lutz Sammer, Nehal Mistry, Jimmy Salmon and Andrettin
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

/**
**   @page VideoModule Module - Video
**
** There are lots of video functions available, therefore this
** page tries to summarize these separately.
**
** @note care must be taken what to use, how to use it and where
** put new source-code. So please read the following sections
** first.
**
**
**   @section VideoMain Video main initialization
**
**   The general setup of platform dependent video and basic video
** functionalities is done with function @see InitVideo
**
** We support (depending on the platform) resolutions:
** 640x480, 800x600, 1024x768, 1600x1200
** with colors 8,15,16,24,32 bit
**
** @see video.h @see video.cpp
**
**
**   @section VideoModuleHigh High Level - video dependent functions
**
** These are the video platforms that are supported, any platform
** dependent settings/functionailty are located within each
** separate files:
**
** SDL : Simple Direct Media for Linux,
**   Win32 (Windows 95/98/2000), BeOs, MacOS
**   (visit http://www.libsdl.org)
**
** @see sdl.cpp
**
**
**   @section VideoModuleLow  Low Level - draw functions
**
** All direct drawing functions
**
** @note you might need to use Decorations (see above), to prevent
** drawing directly to screen in conflict with the video update.
**
**   @see linedraw.cpp
**   @see sprite.cpp
*/

#include "stratagus.h"

#include "video/video.h"
#include "intern_video.h"

#include "database/defines.h"
#include "database/preferences.h"
#include "iolib.h"
#include "map/map.h"
#include "map/minimap.h"
#include "script.h"
#include "ui/cursor.h"
#include "ui/ui.h"
#include "util/image_util.h"
#include "video/font.h"

#include <SDL.h>

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define RSHIFT  0
#define GSHIFT  8
#define BSHIFT  16
#define ASHIFT  24
#define RMASK   0x000000ff
#define GMASK   0x0000ff00
#define BMASK   0x00ff0000
#define AMASK   0xff000000
#else
#define RSHIFT  24
#define GSHIFT  16
#define BSHIFT  8
#define ASHIFT  0
#define RMASK   0xff000000
#define GMASK   0x00ff0000
#define BMASK   0x0000ff00
#define AMASK   0x000000ff
#endif

/**
**  Structure of pushed clippings.
*/
struct Clip {
	int X1;                             /// pushed clipping top left
	int Y1;                             /// pushed clipping top left
	int X2;                             /// pushed clipping bottom right
	int Y2;                             /// pushed clipping bottom right
};

extern void InitVideoSdl();         /// Init SDL video hardware driver

CVideo Video;

//Wyrmgus start
//bool ZoomNoResize;
bool ZoomNoResize = false;
//Wyrmgus end

double NextFrameTicks;               /// Ticks of begin of the next frame
unsigned long FrameCounter;          /// Current frame number
unsigned long SlowFrameCounter;      /// Profile, frames out of sync

int ClipX1;                          /// current clipping top left
int ClipY1;                          /// current clipping top left
int ClipX2;                          /// current clipping bottom right
int ClipY2;                          /// current clipping bottom right

static std::vector<Clip> Clips;

int VideoSyncSpeed = 100;            /// 0 disable interrupts
int SkipFrames;                      /// Skip this frames

uint32_t ColorBlack;
uint32_t ColorDarkGreen;
uint32_t ColorLightBlue;
uint32_t ColorBlue;
uint32_t ColorOrange;
uint32_t ColorWhite;
uint32_t ColorLightGray;
uint32_t ColorGray;
uint32_t ColorDarkGray;
uint32_t ColorRed;
uint32_t ColorGreen;
uint32_t ColorYellow;

/**
**  Set clipping for graphic routines.
**
**  @param left    Left X screen coordinate.
**  @param top     Top Y screen coordinate.
**  @param right   Right X screen coordinate.
**  @param bottom  Bottom Y screen coordinate.
*/
void SetClipping(int left, int top, int right, int bottom)
{
	Assert(left <= right && top <= bottom && left >= 0 && left < Video.Width
		   && top >= 0 && top < Video.Height && right >= 0
		   && right < Video.Width && bottom >= 0 && bottom < Video.Height);

	ClipX1 = left;
	ClipY1 = top;
	ClipX2 = right;
	ClipY2 = bottom;
}

/**
**  Push current clipping.
*/
void PushClipping()
{
	Clip clip = {ClipX1, ClipY1, ClipX2, ClipY2};
	Clips.push_back(clip);
}

/**
**  Pop current clipping.
*/
void PopClipping()
{
	Clip clip = Clips.back();
	ClipX1 = clip.X1;
	ClipY1 = clip.Y1;
	ClipX2 = clip.X2;
	ClipY2 = clip.Y2;
	Clips.pop_back();
}

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Clear the video screen.
*/
void CVideo::ClearScreen()
{
	std::vector<std::function<void(renderer *)>> render_commands;
	FillRectangle(ColorBlack, 0, 0, Video.Width, Video.Height, render_commands);

	//FIXME: do something with the render commands, or remove this function altogether
}

/**
**  Resize the video screen.
**
**  @return  True if the resolution changed, false otherwise
*/
bool CVideo::ResizeScreen(int w, int h)
{
	ViewportWidth = w;
	ViewportHeight = h;
	if (!ZoomNoResize) {
		Width = w;
		Height = h;
		SetClipping(0, 0, Video.Width - 1, Video.Height - 1);
	}

	//Wyrmgus start
	if (GameRunning) {
		InitUserInterface();
		UI.Load();
	}
	//Wyrmgus end
	return true;
}

uint32_t CVideo::MapRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return ((r << RSHIFT) | (g << GSHIFT) | (b << BSHIFT) | (a << ASHIFT));
}

void CVideo::GetRGB(uint32_t c, uint8_t *r, uint8_t *g, uint8_t *b)
{
	*r = (c >> RSHIFT) & 0xff;
	*g = (c >> GSHIFT) & 0xff;
	*b = (c >> BSHIFT) & 0xff;
}

void CVideo::GetRGBA(uint32_t c, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a)
{
	*r = (c >> RSHIFT) & 0xff;
	*g = (c >> GSHIFT) & 0xff;
	*b = (c >> BSHIFT) & 0xff;
	*a = (c >> ASHIFT) & 0xff;
}

QColor CVideo::GetRGBA(const uint32_t c)
{
	QColor color;
	color.setRed((c >> RSHIFT) & 0xff);
	color.setGreen((c >> GSHIFT) & 0xff);
	color.setBlue((c >> BSHIFT) & 0xff);
	color.setAlpha((c >> ASHIFT) & 0xff);
	return color;
}

/**
**  Return ticks in ms since start.
*/
unsigned long GetTicks()
{
	return SDL_GetTicks();
}

/**
**  Video initialize.
*/
void InitVideo()
{
	InitVideoSdl();
}

void DeInitVideo()
{
}

int get_scale_factor()
{
	return defines::get()->get_scale_factor();
}

void pack_image_folder(const std::string &dir_path, const int frames_per_row)
{
	try {
		image::pack_folder(dir_path, image::frame_order::top_to_bottom, frames_per_row);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to pack image folder \"" + dir_path + "\"."));
	}
}

void index_image_to_image_palette(const std::string &image_path, const std::string &other_image_path)
{
	const QString image_path_qstring = QString::fromStdString(image_path);
	QImage image(image_path_qstring);
	const QImage other_image(QString::fromStdString(other_image_path));
	wyrmgus::image::index_to_image_palette(image, other_image);
	image.save(image_path_qstring);
}
