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
/**@name graphic.cpp - The general graphic functions. */
//
//      (c) Copyright 1999-2020 by Lutz Sammer, Nehal Mistry, Jimmy Salmon,
//                                 Pali Rohár and Andrettin
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

#include "stratagus.h"

#include "database/defines.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "intern_video.h"
#include "iocompat.h"
#include "iolib.h"
#include "map/map_layer.h"
#include "player.h"
#include "player_color.h"
//Wyrmgus start
#include "results.h"
//Wyrmgus end
#include "time/time_of_day.h"
#include "ui/ui.h"
//Wyrmgus start
#include "unit/unit.h" //for using CPreference
//Wyrmgus end
#include "util/image_util.h"
#include "util/point_util.h"
#include "video.h"
#include "xbrz.h"

std::map<std::string, CGraphic *> CGraphic::graphics_by_filepath;
std::list<CGraphic *> CGraphic::graphics;

CGraphic::~CGraphic()
{
	if (this->textures != nullptr) {
		glDeleteTextures(this->NumTextures, this->textures);
		delete[] this->textures;
	}

	if (this->grayscale_textures != nullptr) {
		glDeleteTextures(this->NumTextures, this->grayscale_textures);
		delete[] this->grayscale_textures;
	}

	for (const auto &kv_pair : this->texture_color_modifications) {
		glDeleteTextures(this->NumTextures, kv_pair.second);
		delete[] kv_pair.second;
	}
}

/**
**  Video draw the graphic clipped.
**
**  @param x   X screen position
**  @param y   Y screen position
*/
void CGraphic::DrawClip(int x, int y) const
{
	int oldx = x;
	int oldy = y;
	int w = Width;
	int h = Height;
	CLIP_RECTANGLE(x, y, w, h);
	DrawSub(x - oldx, y - oldy, w, h, x, y);
}

/**
**  Video draw part of graphic.
**
**  @param gx  X offset into object
**  @param gy  Y offset into object
**  @param w   width to display
**  @param h   height to display
**  @param x   X screen position
**  @param y   Y screen position
*/
//Wyrmgus start
//void CGraphic::DrawSub(int gx, int gy, int w, int h, int x, int y) const
void CGraphic::DrawSub(int gx, int gy, int w, int h, int x, int y, SDL_Surface *surface) const
//Wyrmgus end
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	DrawTexture(this, this->textures, gx, gy, gx + w, gy + h, x, y, 0);
#endif
}

void CGraphic::DrawGrayscaleSub(int gx, int gy, int w, int h, int x, int y) const
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	DrawTexture(this, this->grayscale_textures, gx, gy, gx + w, gy + h, x, y, 0);
#endif
}

CPlayerColorGraphic::~CPlayerColorGraphic()
{
	for (const auto &kv_pair : this->player_color_textures) {
		glDeleteTextures(this->NumTextures, kv_pair.second);
		delete[] kv_pair.second;
	}

	for (const auto &kv_pair : this->player_color_texture_color_modifications) {
		for (const auto &sub_kv_pair : kv_pair.second) {
			glDeleteTextures(this->NumTextures, sub_kv_pair.second);
			delete[] sub_kv_pair.second;
		}
	}
}

void CPlayerColorGraphic::DrawPlayerColorSub(const stratagus::player_color *player_color, int gx, int gy, int w, int h, int x, int y)
{
	if (this->get_textures(player_color) == nullptr) {
		MakePlayerColorTexture(this, player_color, nullptr);
	}
	DrawTexture(this, this->get_textures(player_color), gx, gy, gx + w, gy + h, x, y, 0);
}

void CGraphic::DrawSubClip(int gx, int gy, int w, int h, int x, int y, SDL_Surface *surface) const
{
	int oldx = x;
	int oldy = y;
	CLIP_RECTANGLE(x, y, w, h);
	DrawSub(gx + x - oldx, gy + y - oldy, w, h, x, y, surface);
}

void CGraphic::DrawGrayscaleSubClip(int gx, int gy, int w, int h, int x, int y) const
{
	int oldx = x;
	int oldy = y;
	CLIP_RECTANGLE(x, y, w, h);
	DrawGrayscaleSub(gx + x - oldx, gy + y - oldy, w, h, x, y);
}

void CPlayerColorGraphic::DrawPlayerColorSubClip(const stratagus::player_color *player_color, int gx, int gy, int w, int h, int x, int y)
{
	int oldx = x;
	int oldy = y;
	CLIP_RECTANGLE(x, y, w, h);
	DrawPlayerColorSub(player_color, gx + x - oldx, gy + y - oldy, w, h, x, y);
}

/**
**  Video draw part of graphic with alpha.
**
**  @param gx     X offset into object
**  @param gy     Y offset into object
**  @param w      width to display
**  @param h      height to display
**  @param x      X screen position
**  @param y      Y screen position
**  @param alpha  Alpha
*/
void CGraphic::DrawSubTrans(int gx, int gy, int w, int h, int x, int y,
							//Wyrmgus start
//							unsigned char alpha) const
							unsigned char alpha, SDL_Surface *surface) const
							//Wyrmgus end
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor4ub(255, 255, 255, alpha);
	DrawSub(gx, gy, w, h, x, y);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
}

/**
**  Video draw part of graphic with alpha and clipped.
**
**  @param gx     X offset into object
**  @param gy     Y offset into object
**  @param w      width to display
**  @param h      height to display
**  @param x      X screen position
**  @param y      Y screen position
**  @param alpha  Alpha
*/
void CGraphic::DrawSubClipTrans(int gx, int gy, int w, int h, int x, int y,
								//Wyrmgus start
//								unsigned char alpha) const
								unsigned char alpha, SDL_Surface *surface) const
								//Wyrmgus end
{
	int oldx = x;
	int oldy = y;
	CLIP_RECTANGLE(x, y, w, h);
	//Wyrmgus start
//	DrawSubTrans(gx + x - oldx, gy + y - oldy, w, h, x, y, alpha);
	DrawSubTrans(gx + x - oldx, gy + y - oldy, w, h, x, y, alpha, surface);
	//Wyrmgus end
}

/**
**  Draw graphic object unclipped.
**
**  @param frame   number of frame (object index)
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
*/
void CGraphic::DrawFrame(unsigned frame, int x, int y) const
{
	DrawTexture(this, this->textures, frame_map[frame].x, frame_map[frame].y,
					frame_map[frame].x +  Width, frame_map[frame].y + Height, x, y, 0);
}

#if defined(USE_OPENGL) || defined(USE_GLES)
void CGraphic::DoDrawFrameClip(const GLuint *textures,
							   unsigned frame, int x, int y, int show_percent) const
{
	int ox;
	int oy;
	int skip;
	int w = Width;
	int h = Height;

	CLIP_RECTANGLE_OFS(x, y, w, h, ox, oy, skip);
	UNUSED(skip);
	DrawTexture(this, textures, frame_map[frame].x + ox,
				frame_map[frame].y + oy,
				frame_map[frame].x + ox + (w),
				frame_map[frame].y + oy + (h * show_percent / 100), x, y, 0);
}
#endif

/**
**  Draw graphic object clipped.
**
**  @param frame   number of frame (object index)
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
*/
void CGraphic::DrawFrameClip(unsigned frame, int x, int y, const stratagus::time_of_day *time_of_day, SDL_Surface *surface, int show_percent)
{
	if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
		DoDrawFrameClip(this->textures, frame, x, y, show_percent);
	} else {
		if (this->get_textures(time_of_day->ColorModification) == nullptr) {
			MakeTexture(this, false, time_of_day);
		}
		DoDrawFrameClip(this->get_textures(time_of_day->ColorModification), frame, x, y, show_percent);
	}
}

void CGraphic::DrawFrameTrans(unsigned frame, int x, int y, int alpha) const
{
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor4ub(255, 255, 255, alpha);
	DrawFrame(frame, x, y);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

void CGraphic::DrawFrameClipTrans(unsigned frame, int x, int y, int alpha, const stratagus::time_of_day *time_of_day, SDL_Surface *surface, int show_percent)
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor4ub(255, 255, 255, alpha);
	DrawFrameClip(frame, x, y, time_of_day, surface, show_percent);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
}

void CGraphic::DrawGrayscaleFrameClip(unsigned frame, int x, int y, int show_percent)
{
	DoDrawFrameClip(this->grayscale_textures, frame, x, y, show_percent);
}

void CPlayerColorGraphic::DrawPlayerColorFrameClip(const stratagus::player_color *player_color, unsigned frame, int x, int y, const stratagus::time_of_day *time_of_day, int show_percent)
{
	if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
		if (this->get_textures(player_color) == nullptr) {
			MakePlayerColorTexture(this, player_color, nullptr);
		}
		DoDrawFrameClip(this->get_textures(player_color), frame, x, y, show_percent);
	} else {
		if (this->get_textures(player_color, time_of_day->ColorModification) == nullptr) {
			MakePlayerColorTexture(this, player_color, time_of_day);
		}
		DoDrawFrameClip(this->get_textures(player_color, time_of_day->ColorModification), frame, x, y, show_percent);
	}
}

//Wyrmgus start
void CPlayerColorGraphic::DrawPlayerColorFrameClipTrans(const stratagus::player_color *player_color, unsigned frame, int x, int y, int alpha, const stratagus::time_of_day *time_of_day, int show_percent)
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
		if (this->get_textures(player_color) == nullptr) {
			MakePlayerColorTexture(this, player_color, nullptr);
		}
	} else {
		if (this->get_textures(player_color, time_of_day->ColorModification) == nullptr) {
			MakePlayerColorTexture(this, player_color, time_of_day);
		}
	}
	
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor4ub(255, 255, 255, alpha);
	if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
		DoDrawFrameClip(this->get_textures(player_color), frame, x, y, show_percent);
	} else {
		DoDrawFrameClip(this->get_textures(player_color, time_of_day->ColorModification), frame, x, y, show_percent);
	}
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
}

void CPlayerColorGraphic::DrawPlayerColorFrameClipTransX(const stratagus::player_color *player_color, unsigned frame, int x, int y, int alpha, const stratagus::time_of_day *time_of_day)
{
	if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
		if (this->get_textures(player_color) == nullptr) {
			MakePlayerColorTexture(this, player_color, nullptr);
		}
	} else {
		if (this->get_textures(player_color, time_of_day->ColorModification) == nullptr) {
			MakePlayerColorTexture(this, player_color, time_of_day);
		}
	}
	
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor4ub(255, 255, 255, alpha);
	if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
		DoDrawFrameClipX(this->get_textures(player_color), frame, x, y);
	} else {
		DoDrawFrameClipX(this->get_textures(player_color, time_of_day->ColorModification), frame, x, y);
	}
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}
//Wyrmgus end

/**
**  Draw graphic object unclipped and flipped in X direction.
**
**  @param frame   number of frame (object index)
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
*/
void CGraphic::DrawFrameX(unsigned frame, int x, int y) const
{
	DrawTexture(this, this->textures, frame_map[frame].x, frame_map[frame].y,
				frame_map[frame].x +  Width, frame_map[frame].y + Height, x, y, 1);
}

#if defined(USE_OPENGL) || defined(USE_GLES)
void CGraphic::DoDrawFrameClipX(const GLuint *textures, unsigned frame,
								int x, int y) const
{
	int ox;
	int oy;
	int skip;
	int w = Width;
	int h = Height;
	CLIP_RECTANGLE_OFS(x, y, w, h, ox, oy, skip);
	UNUSED(skip);

	if (w < Width) {
		if (ox == 0) {
			ox = Width - w;
		} else {
			ox = 0;
		}
	}

	DrawTexture(this, textures, frame_map[frame].x + ox,
				frame_map[frame].y + oy,
				frame_map[frame].x + ox + w,
				frame_map[frame].y + oy + h, x, y, 1);
}
#endif

/**
**  Draw graphic object clipped and flipped in X direction.
**
**  @param frame   number of frame (object index)
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
*/
//Wyrmgus start
//void CGraphic::DrawFrameClipX(unsigned frame, int x, int y) const
void CGraphic::DrawFrameClipX(unsigned frame, int x, int y, const stratagus::time_of_day *time_of_day, SDL_Surface *surface)
//Wyrmgus end
{
	if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
		DoDrawFrameClipX(this->textures, frame, x, y);
	} else {
		if (this->get_textures(time_of_day->ColorModification) == nullptr) {
			MakeTexture(this, false, time_of_day);
		}
		DoDrawFrameClipX(this->get_textures(time_of_day->ColorModification), frame, x, y);
	}
}

void CGraphic::DrawFrameTransX(unsigned frame, int x, int y, int alpha) const
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor4ub(255, 255, 255, alpha);
	DrawFrameX(frame, x, y);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
}

//Wyrmgus start
//void CGraphic::DrawFrameClipTransX(unsigned frame, int x, int y, int alpha) const
void CGraphic::DrawFrameClipTransX(unsigned frame, int x, int y, int alpha, const stratagus::time_of_day *time_of_day, SDL_Surface *surface)
//Wyrmgus end
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glColor4ub(255, 255, 255, alpha);
	//Wyrmgus start
//	DrawFrameClipX(frame, x, y);
	DrawFrameClipX(frame, x, y, time_of_day);
	//Wyrmgus end
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif
}

/**
**  Draw graphic object clipped, flipped, and with player colors.
**
**  @param player  player number
**  @param frame   number of frame (object index)
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
*/
void CPlayerColorGraphic::DrawPlayerColorFrameClipX(const stratagus::player_color *player_color, unsigned frame, int x, int y, const stratagus::time_of_day *time_of_day)
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (time_of_day == nullptr || !time_of_day->HasColorModification()) {
		if (this->get_textures(player_color) == nullptr) {
			MakePlayerColorTexture(this, player_color, nullptr);
		}
		DoDrawFrameClipX(this->get_textures(player_color), frame, x, y);
	} else {
		if (this->get_textures(player_color, time_of_day->ColorModification) == nullptr) {
			MakePlayerColorTexture(this, player_color, time_of_day);
		}
		DoDrawFrameClipX(this->get_textures(player_color, time_of_day->ColorModification), frame, x, y);
	}
#endif
}

/*----------------------------------------------------------------------------
--  Global functions
----------------------------------------------------------------------------*/

/**
**  Make a new graphic object.
**
**  @param filename  Filename
**  @param w     Width of a frame (optional)
**  @param h     Height of a frame (optional)
**
**  @return      New graphic object
*/
CGraphic *CGraphic::New(const std::string &filepath, const int w, const int h)
{
	std::unique_lock<std::shared_mutex> lock(CGraphic::mutex);

	if (filepath.empty()) {
		throw std::runtime_error("CGraphic::New() called with an empty filepath.");
	}

	const std::string library_filepath = LibraryFileName(filepath.c_str());
	CGraphic *g = CGraphic::graphics_by_filepath[library_filepath];
	if (g == nullptr) {
		g = new CGraphic(library_filepath);
		if (!g) {
			fprintf(stderr, "Out of memory\n");
			ExitFatal(-1);
		}
		// FIXME: use a constructor for this
		g->HashFile = library_filepath;
		g->Width = w;
		g->Height = h;
		g->original_frame_size = QSize(w, h);
		CGraphic::graphics_by_filepath[g->HashFile] = g;
	} else {
		++g->refs;
		Assert((w == 0 || g->Width == w) && (g->Height == h || h == 0));
	}

	return g;
}

/**
**  Make a new player color graphic object.
**
**  @param filename  Filename
**  @param w     Width of a frame (optional)
**  @param h     Height of a frame (optional)
**
**  @return      New graphic object
*/
CPlayerColorGraphic *CPlayerColorGraphic::New(const std::string &filepath, const int w, const int h)
{
	std::unique_lock<std::shared_mutex> lock(CGraphic::mutex);

	if (filepath.empty()) {
		throw std::runtime_error("CPlayerColorGraphic::New() called with an empty filepath.");
	}

	const std::string file = LibraryFileName(filepath.c_str());
	CPlayerColorGraphic *g = dynamic_cast<CPlayerColorGraphic *>(CGraphic::graphics_by_filepath[file]);
	if (g == nullptr) {
		g = new CPlayerColorGraphic(file);
		if (!g) {
			fprintf(stderr, "Out of memory\n");
			ExitFatal(-1);
		}
		// FIXME: use a constructor for this
		g->HashFile = file;
		g->Width = w;
		g->Height = h;
		g->original_frame_size = QSize(w, h);
		CGraphic::graphics_by_filepath[g->HashFile] = g;
	} else {
		++g->refs;
		Assert((w == 0 || g->Width == w) && (g->Height == h || h == 0));
	}

	return g;
}

const GLuint *CPlayerColorGraphic::get_textures(const stratagus::player_color *player_color) const
{
	if (!this->has_player_color() || player_color == nullptr || player_color == stratagus::defines::get()->get_conversible_player_color()) {
		return CGraphic::get_textures();
	}

	auto find_iterator = this->player_color_textures.find(player_color);
	if (find_iterator != this->player_color_textures.end()) {
		return find_iterator->second;
	}

	return nullptr;
}

const GLuint *CPlayerColorGraphic::get_textures(const stratagus::player_color *player_color, const CColor &color_modification) const
{
	if (!this->has_player_color() || player_color == nullptr || player_color == stratagus::defines::get()->get_conversible_player_color()) {
		return CGraphic::get_textures(color_modification);
	}

	auto find_iterator = this->player_color_texture_color_modifications.find(player_color);
	if (find_iterator != this->player_color_texture_color_modifications.end()) {
		auto sub_find_iterator = find_iterator->second.find(color_modification);
		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	return nullptr;
}

/**
**  Get a graphic object.
**
**  @param filename  Filename
**
**  @return      Graphic object
*/
CGraphic *CGraphic::Get(const std::string &filename)
{
	std::shared_lock<std::shared_mutex> lock(CGraphic::mutex);

	if (filename.empty()) {
		return nullptr;
	}

	const std::string file = LibraryFileName(filename.c_str());
	CGraphic *&g = CGraphic::graphics_by_filepath[file];

	return g;
}

/**
**  Get a player color graphic object.
**
**  @param filename  Filename
**
**  @return      Graphic object
*/
CPlayerColorGraphic *CPlayerColorGraphic::Get(const std::string &filename)
{
	std::shared_lock<std::shared_mutex> lock(CGraphic::mutex);

	if (filename.empty()) {
		return nullptr;
	}

	const std::string file = LibraryFileName(filename.c_str());
	CPlayerColorGraphic *g = dynamic_cast<CPlayerColorGraphic *>(CGraphic::graphics_by_filepath[file]);

	return g;
}

void CGraphic::GenFramesMap()
{
	Assert(NumFrames != 0);
#if defined(USE_OPENGL) || defined(USE_GLES)
	Assert(GraphicWidth != 0);
#endif
	Assert(Width != 0);
	Assert(Height != 0);

	frame_map.resize(NumFrames);

	for (int frame = 0; frame < NumFrames; ++frame) {
#if defined(USE_OPENGL) || defined(USE_GLES)
		frame_map[frame].x = (frame % (GraphicWidth / Width)) * Width;
		frame_map[frame].y = (frame / (GraphicWidth / Width)) * Height;
#endif
	}
}

static void ApplyGrayScale(QImage &image)
{
	const int bpp = image.depth() / 8;
	const double redGray = 0.21;
	const double greenGray = 0.72;
	const double blueGray = 0.07;

	switch (bpp) {
		case 1: {
			for (int i = 0; i < image.colorCount(); ++i) {
				const QRgb rgb = image.color(i);
				QColor color(rgb);
				const int gray = redGray * color.red() + greenGray * color.green() + blueGray * color.blue();
				color.setRed(gray);
				color.setGreen(gray);
				color.setBlue(gray);
				image.setColor(i, color.rgba());
			}
			break;
		}
		case 3:
		case 4:
			if (bpp == 3 && image.format() != QImage::Format_RGB888) {
				image = image.convertToFormat(QImage::Format_RGB888);
			} else if (bpp == 4 && image.format() != QImage::Format_RGBA8888) {
				image = image.convertToFormat(QImage::Format_RGBA8888);
			}

			unsigned char *image_data = image.bits();
			for (int x = 0; x < image.width(); ++x) {
				for (int y = 0; y < image.height(); ++y) {
					const int pixel_index = (y * image.width() + x) * bpp;
					unsigned char &red = image_data[pixel_index];
					unsigned char &green = image_data[pixel_index + 1];
					unsigned char &blue = image_data[pixel_index + 2];

					const int gray = redGray * red + greenGray * green + blueGray * blue;
					red = gray;
					green = gray;
					blue = gray;
				}
			}
			break;
	}
}

//Wyrmgus start
static void ApplySepiaScale(QImage &image)
{
	const int bpp = image.depth() / 8;

	switch (bpp) {
		case 1: {
			for (int i = 0; i < image.colorCount(); ++i) {
				const QRgb rgb = image.color(i);
				QColor color(rgb);

				const int input_red = color.red();
				const int input_green = color.green();
				const int input_blue = color.blue();

				color.setRed(std::min<int>(255, (input_red * .393) + (input_green * .769) + (input_blue * .189)));
				color.setGreen(std::min<int>(255, (input_red * .349) + (input_green * .686) + (input_blue * .168)));
				color.setBlue(std::min<int>(255, (input_red * .272) + (input_green * .534) + (input_blue * .131)));

				image.setColor(i, color.rgba());
			}
			break;
		}
		case 3:
		case 4:
			if (bpp == 3 && image.format() != QImage::Format_RGB888) {
				image = image.convertToFormat(QImage::Format_RGB888);
			} else if (bpp == 4 && image.format() != QImage::Format_RGBA8888) {
				image = image.convertToFormat(QImage::Format_RGBA8888);
			}

			unsigned char *image_data = image.bits();
			for (int x = 0; x < image.width(); ++x) {
				for (int y = 0; y < image.height(); ++y) {
					const int pixel_index = (y * image.width() + x) * bpp;
					unsigned char &red = image_data[pixel_index];
					unsigned char &green = image_data[pixel_index + 1];
					unsigned char &blue = image_data[pixel_index + 2];
					
					const int input_red = red;
					const int input_green = green;
					const int input_blue = blue;
					
					red = std::min<int>(255, (input_red * .393) + (input_green * .769) + (input_blue * .189));
					green = std::min<int>(255, (input_red * .349) + (input_green * .686) + (input_blue * .168));
					blue = std::min<int>(255, (input_red * .272) + (input_green * .534) + (input_blue * .131));
				}
			}
			break;
	}
}

/*
static std::string map_terrains[8192][4096];

static void ConvertImageToMap(SDL_Surface *Surface, int Width, int Height)
{
	SDL_LockSurface(Surface);
	const SDL_PixelFormat *f = Surface->format;
	const int bpp = Surface->format->BytesPerPixel;
	Uint8 r, g, b;

	for (int j = 0; j < Height; ++j) {
		for (int i = 0; i < Width; ++i) {
			Uint32 c = *reinterpret_cast<Uint32 *>(&reinterpret_cast<Uint8 *>(Surface->pixels)[i * 4 + j * Surface->pitch]);
			Uint8 a;

			Video.GetRGBA(c, Surface->format, &r, &g, &b, &a);
			if (a >= 128) {
				if (r == 235 && g == 235 && b == 235) {
					map_terrains[i][j] = "s";
				} else if (r == 138 && g == 11 && b == 26) {
					map_terrains[i][j] = "r";
				} else if (r == 69 && g == 91 && b == 186) {
					map_terrains[i][j] = "w";
				} else if (r == 178 && g == 34 && b == 34) {
					map_terrains[i][j] = "m";
				} else if (r == 0 && g == 255 && b == 255) {
					map_terrains[i][j] = "e";
				} else {
					map_terrains[i][j] = "d";
				}
			} else {
				map_terrains[i][j] = "0";
			}
		}
	}
	SDL_UnlockSurface(Surface);
	
	FileWriter *fw = nullptr;
	std::string map_filename = "scripts/map_templates/new.map";

	try {
		fw = CreateFileWriter(map_filename);

		for (int y = 0; y < Height; ++y) {
			for (int x = 0; x < Width; ++x) {
				fw->printf("%s", map_terrains[x][y].c_str());
			}
			fw->printf("\n");
		}
			
		fw->printf("\n");
	} catch (const FileException &) {
		fprintf(stderr, "Couldn't write the map setup: \"%s\"\n", map_filename.c_str());
		delete fw;
		return;
	}
	
	delete fw;
}
*/
//Wyrmgus end

/**
**  Load a graphic
**
**  @param grayscale  Make a grayscale surface
*/
void CGraphic::Load(const bool create_grayscale_textures, const int scale_factor)
{
	if (this->IsLoaded()) {
		return;
	}

	// TODO: More formats?
	if (LoadGraphicPNG(this, scale_factor) == -1) {
		throw std::runtime_error("Can't load the graphic \"" + this->get_filepath().string() + "\".");
	}

	if (this->custom_scale_factor != 1) {
		//update the frame size for the custom scale factor of the loaded image
		this->Width *= this->custom_scale_factor;
		this->Height *= this->custom_scale_factor;
		this->original_frame_size = QSize(this->Width, this->Height);
	}

	if (!Width) {
		Width = GraphicWidth;
		this->original_frame_size.setWidth(Width);
	}
	if (!Height) {
		Height = GraphicHeight;
		this->original_frame_size.setHeight(Height);
	}

	Assert(Width <= GraphicWidth && Height <= GraphicHeight);

	if ((GraphicWidth / Width) * Width != GraphicWidth ||
		(GraphicHeight / Height) * Height != GraphicHeight) {
		fprintf(stderr, "Invalid graphic (width, height) %s\n", this->get_filepath().string().c_str());
		fprintf(stderr, "Expected: (%d,%d)  Found: (%d,%d)\n",
				Width, Height, GraphicWidth, GraphicHeight);
		ExitFatal(-1);
	}

	NumFrames = GraphicWidth / Width * GraphicHeight / Height;

	this->player_color = false;
	const stratagus::color_set color_set = stratagus::image::get_colors(this->get_image());
	for (const QColor &color : color_set) {
		if (!this->player_color) {
			const stratagus::player_color *conversible_player_color = stratagus::defines::get()->get_conversible_player_color();
			for (const QColor &player_color : conversible_player_color->get_colors()) {
				if (color.red() == player_color.red() && color.green() == player_color.green() && color.blue() == player_color.blue()) {
					this->player_color = true;
					break;
				}
			}
		}

		if (this->player_color) {
			break; //nothing left to check
		}
	}
	
	MakeTexture(this, false, nullptr);

	if (create_grayscale_textures) {
		MakeTexture(this, true, nullptr);
	}

	CGraphic::graphics.push_back(this);

	GenFramesMap();

	if (scale_factor != this->custom_scale_factor) {
		this->Resize(this->GraphicWidth * scale_factor / this->custom_scale_factor, this->GraphicHeight * scale_factor / this->custom_scale_factor);
	}
}

/**
**  Free a SDL surface
**
**  @param surface  SDL surface to free
*/
static void FreeSurface(SDL_Surface **surface)
{
	if (!*surface) {
		return;
	}
	VideoPaletteListRemove(*surface);

	unsigned char *pixels = nullptr;

	if ((*surface)->flags & SDL_PREALLOC) {
		pixels = (unsigned char *)(*surface)->pixels;
	}

	SDL_FreeSurface(*surface);
	delete[] pixels;
	*surface = nullptr;
}

/**
**  Free a graphic
**
**  @param g  Pointer to the graphic
*/
void CGraphic::Free(CGraphic *g)
{
	if (!g) {
		return;
	}

	std::unique_lock<std::shared_mutex> lock(CGraphic::mutex);

	if (g->refs <= 0) {
		throw std::runtime_error("Tried to free an already-freed graphic.");
	}

	--g->refs;
	if (g->refs == 0) {
		// No more uses of this graphic
		CGraphic::graphics.remove(g);

		if (!g->HashFile.empty()) {
			CGraphic::graphics_by_filepath.erase(g->HashFile);
		}

		delete g;
	}
}

#if defined(USE_OPENGL) || defined(USE_GLES)

/**
**  Free OpenGL graphics
*/
void FreeOpenGLGraphics()
{
	for (CGraphic *graphic : CGraphic::graphics) {
		if (graphic->textures) {
			glDeleteTextures(graphic->NumTextures, graphic->textures);
			delete[] graphic->textures;
			graphic->textures = nullptr;
		}
		
		if (graphic->grayscale_textures) {
			glDeleteTextures(graphic->NumTextures, graphic->grayscale_textures);
			delete[] graphic->grayscale_textures;
			graphic->grayscale_textures = nullptr;
		}
		
		for (const auto &kv_pair : graphic->texture_color_modifications) {
			glDeleteTextures(graphic->NumTextures, kv_pair.second);
		}
		graphic->texture_color_modifications.clear();
		
		CPlayerColorGraphic *cg = dynamic_cast<CPlayerColorGraphic *>(graphic);
		if (cg) {
			for (const auto &kv_pair : cg->player_color_textures) {
				glDeleteTextures(cg->NumTextures, kv_pair.second);
			}
			cg->player_color_textures.clear();
			
			for (const auto &kv_pair : cg->player_color_texture_color_modifications) {
				for (const auto &sub_kv_pair : kv_pair.second) {
					glDeleteTextures(cg->NumTextures, sub_kv_pair.second);
				}
			}
			cg->player_color_texture_color_modifications.clear();
		}
	}
}

/**
**	@brief	Reload OpenGL graphics
*/
void ReloadGraphics()
{
	for (CGraphic *graphic : CGraphic::graphics) {
		if (graphic->textures) {
			delete[] graphic->textures;
			graphic->textures = nullptr;
			MakeTexture(graphic, false, nullptr);
		}
		
		if (graphic->grayscale_textures) {
			delete[] graphic->grayscale_textures;
			graphic->grayscale_textures = nullptr;
			MakeTexture(graphic, true, nullptr);
		}
		
		for (const auto &kv_pair : graphic->texture_color_modifications) {
			delete[] kv_pair.second;
		}
		graphic->texture_color_modifications.clear();
				
		CPlayerColorGraphic *cg = dynamic_cast<CPlayerColorGraphic *>(graphic);
		if (cg) {
			for (const auto &kv_pair : cg->player_color_texture_color_modifications) {
				for (const auto &sub_kv_pair : kv_pair.second) {
					delete[] sub_kv_pair.second;
				}
			}
			cg->player_color_texture_color_modifications.clear();
			
			for (const auto &kv_pair : cg->player_color_textures) {
				delete[] kv_pair.second;
			}
			cg->player_color_textures.clear();

			for (const stratagus::player_color *player_color : stratagus::player_color::get_all()) {
				MakePlayerColorTexture(cg, player_color, nullptr);
			}
		}
	}
}

#endif

#if defined(USE_OPENGL) || defined(USE_GLES)

/**
**  Find the next power of 2 >= x
*/
static int PowerOf2(int x)
{
	int i;
	for (i = 1; i < x; i <<= 1) ;
	return i;
}

/**
**  Make an OpenGL texture or textures out of a graphic object.
**
**  @param g        The graphic object.
**  @param texture  Texture.
**  @param colors   Unit colors.
**  @param ow       Offset width.
**  @param oh       Offset height.
*/
void MakeTextures2(const CGraphic *g, const QImage &image, GLuint texture, const int ow, const int oh, const stratagus::time_of_day *time_of_day)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	int maxw = std::min<int>(image.width() - ow, GLMaxTextureSize);
	int maxh = std::min<int>(image.height() - oh, GLMaxTextureSize);
	int w = PowerOf2(maxw);
	int h = PowerOf2(maxh);
	std::vector<unsigned char> tex(w * h * 4, 0);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	//Wyrmgus start
	int time_of_day_red = 0;
	int time_of_day_green = 0;
	int time_of_day_blue = 0;
	
	if (time_of_day && time_of_day->HasColorModification()) {
		time_of_day_red = time_of_day->ColorModification.R;
		time_of_day_green = time_of_day->ColorModification.G;
		time_of_day_blue = time_of_day->ColorModification.B;
	}
	//Wyrmgus end

	if (image.isNull()) {
		throw std::runtime_error("Cannot generate a texture for a null image.");
	}

	const int depth = image.depth();
	const int bpp = depth / 8;

	if (bpp != 4) {
		throw std::runtime_error("The image BPP must be 4 for generating textures.");
	}

	const unsigned char *image_data = image.constBits();
	for (int x = 0; x < maxw; ++x) {
		for (int y = 0; y < maxh; ++y) {
			const int src_index = ((oh + y) * image.width() + ow + x) * bpp;
			const unsigned char &src_red = image_data[src_index];
			const unsigned char &src_green = image_data[src_index + 1];
			const unsigned char &src_blue = image_data[src_index + 2];
			const unsigned char &src_alpha = image_data[src_index + 3];

			const int dst_index = (y * w + x) * 4;
			unsigned char &dst_red = tex[dst_index];
			unsigned char &dst_green = tex[dst_index + 1];
			unsigned char &dst_blue = tex[dst_index + 2];
			unsigned char &dst_alpha = tex[dst_index + 3];

			dst_red = src_red;
			dst_green = src_green;
			dst_blue = src_blue;
			dst_alpha = src_alpha;

			if (time_of_day_red != 0) {
				dst_red = std::max<int>(0, std::min<int>(255, dst_red + time_of_day_red));
			}
			if (time_of_day_green != 0) {
				dst_green = std::max<int>(0, std::min<int>(255, dst_green + time_of_day_green));
			}
			if (time_of_day_blue != 0) {
				dst_blue = std::max<int>(0, std::min<int>(255, dst_blue + time_of_day_blue));
			}
		}
	}

	GLint internalformat = GL_RGBA;
#ifdef USE_OPENGL
	if (GLTextureCompressionSupported && UseGLTextureCompression) {
		internalformat = GL_COMPRESSED_RGBA;
	}
#endif

	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.data());

	const int error_code = glGetError();
	if (error_code != GL_NO_ERROR) {
		throw std::runtime_error("glTexImage2D failed with error code " + std::to_string(error_code) + ".");
	}
}

static void MakeTextures(CGraphic *g, const bool grayscale, const stratagus::player_color *player_color, const stratagus::time_of_day *time_of_day)
{
	int tw = (g->get_width() - 1) / GLMaxTextureSize + 1;
	const int th = (g->get_height() - 1) / GLMaxTextureSize + 1;

	int w = g->get_width() % GLMaxTextureSize;
	if (w == 0) {
		w = GLMaxTextureSize;
	}
	g->TextureWidth = (GLfloat)w / PowerOf2(w);

	int h = g->get_height() % GLMaxTextureSize;
	if (h == 0) {
		h = GLMaxTextureSize;
	}
	g->TextureHeight = (GLfloat)h / PowerOf2(h);

	g->NumTextures = tw * th;

	CPlayerColorGraphic *cg = dynamic_cast<CPlayerColorGraphic *>(g);
	GLuint *textures;
	if (!player_color || !cg) {
		if (time_of_day && time_of_day->HasColorModification()) {
			textures = g->texture_color_modifications[time_of_day->ColorModification] = new GLuint[g->NumTextures];
			glGenTextures(g->NumTextures, g->texture_color_modifications[time_of_day->ColorModification]);
		} else if (grayscale) {
			textures = g->grayscale_textures = new GLuint[g->NumTextures];
			glGenTextures(g->NumTextures, g->grayscale_textures);
		} else {
			textures = g->textures = new GLuint[g->NumTextures];
			glGenTextures(g->NumTextures, g->textures);
		}
	} else if (time_of_day && time_of_day->HasColorModification()) {
		textures = cg->player_color_texture_color_modifications[player_color][time_of_day->ColorModification] = new GLuint[cg->NumTextures];
		glGenTextures(cg->NumTextures, cg->player_color_texture_color_modifications[player_color][time_of_day->ColorModification]);
	} else {
		textures = cg->player_color_textures[player_color] = new GLuint[cg->NumTextures];
		glGenTextures(cg->NumTextures, cg->player_color_textures[player_color]);
	}

	QImage image = g->get_image();
	if (image.format() != QImage::Format_RGBA8888) {
		image = image.convertToFormat(QImage::Format_RGBA8888);
	}

	if (grayscale) {
		if (Preference.SepiaForGrayscale) {
			ApplySepiaScale(image);
		} else {
			ApplyGrayScale(image);
		}
	} else if (player_color != nullptr && g->has_player_color()) {
		const int bpp = image.depth() / 8;
		unsigned char *image_data = image.bits();
		const stratagus::player_color *conversible_player_color = stratagus::defines::get()->get_conversible_player_color();
		const std::vector<QColor> &conversible_colors = conversible_player_color->get_colors();
		const std::vector<QColor> &colors = player_color->get_colors();

		for (int i = 0; i < image.sizeInBytes(); i += bpp) {
			unsigned char &red = image_data[i];
			unsigned char &green = image_data[i + 1];
			unsigned char &blue = image_data[i + 2];

			for (size_t z = 0; z < conversible_colors.size(); ++z) {
				const QColor &color = conversible_colors[z];
				if (red == color.red() && green == color.green() && blue == color.blue()) {
					red = colors[z].red();
					green = colors[z].green();
					blue = colors[z].blue();
				}
			}
		}
	}

	if (image.size() != g->get_size()) {
		//the texture needs to be rescaled compared to the source image
		if (g->get_width() > image.width() && g->get_height() > image.height() && (g->get_width() % image.width()) == 0 && (g->get_height() % image.height()) == 0 && (g->get_width() / image.width()) == (g->get_height() / image.height())) {
			//if a simple scale factor is being used for the resizing, then use xBRZ for the rescaling
			const int scale_factor = g->get_width() / image.width();
			image = stratagus::image::scale(image, scale_factor, g->get_original_frame_size());
		} else {
			image = image.scaled(g->get_size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			if (image.format() != QImage::Format_RGBA8888) {
				//the image's format could have changed due to the rescaling
				image = image.convertToFormat(QImage::Format_RGBA8888);
			}
		}
	}

	for (int j = 0; j < th; ++j) {
		for (int i = 0; i < tw; ++i) {
			MakeTextures2(g, image, textures[j * tw + i], GLMaxTextureSize * i, GLMaxTextureSize * j, time_of_day);
		}
	}
}

/**
**  Make an OpenGL texture or textures out of a graphic object.
**
**  @param g  The graphic object.
*/
void MakeTexture(CGraphic *g, const bool grayscale, const stratagus::time_of_day *time_of_day)
{
	if (time_of_day && time_of_day->HasColorModification()) {
		if (g->get_textures(time_of_day->ColorModification) != nullptr) {
			return;
		}
	} else if (grayscale) {
		if (g->grayscale_textures) {
			return;
		}
	} else {
		if (g->textures) {
			return;
		}
	}

	MakeTextures(g, grayscale, nullptr, time_of_day);
}

void MakePlayerColorTexture(CPlayerColorGraphic *g, const stratagus::player_color *player_color, const stratagus::time_of_day *time_of_day)
{
	if (time_of_day && time_of_day->HasColorModification()) {
		if (g->get_textures(player_color, time_of_day->ColorModification) != nullptr) {
			return;
		}

		if (!g->has_player_color() || player_color == nullptr || player_color == stratagus::defines::get()->get_conversible_player_color()) {
			MakeTextures(g, false, nullptr, time_of_day);
			return;
		}
	} else {
		if (g->get_textures(player_color) != nullptr) {
			return;
		}
	}

	MakeTextures(g, false, player_color, time_of_day);
}

#endif

/**
**  Resize a graphic
**
**  @param w  New width of graphic.
**  @param h  New height of graphic.
*/
void CGraphic::Resize(int w, int h)
{
	Assert(this->IsLoaded()); // can't resize before it's been loaded

	if (GraphicWidth == w && GraphicHeight == h) {
		return;
	}

	const QSize old_size(this->GraphicWidth, this->GraphicHeight);
	const QSize old_frame_size(this->Width, this->Height);
	const QSize frame_size(this->Width * w / old_size.width(), this->Height * h / old_size.height());

	// Resizing the same image multiple times looks horrible
	// If the image has already been resized then get a clean copy first
	if (Resized) {
		this->SetOriginalSize();
		if (GraphicWidth == w && GraphicHeight == h) {
			return;
		}
	}

	this->Resized = true;

	GraphicWidth = w;
	GraphicHeight = h;

	this->Width = frame_size.width();
	this->Height = frame_size.height();

	if (this->textures) {
		glDeleteTextures(NumTextures, this->textures);
		delete[] this->textures;
		this->textures = nullptr;
		MakeTexture(this, false, nullptr);
	}

	if (this->grayscale_textures) {
		glDeleteTextures(NumTextures, this->grayscale_textures);
		delete[] this->grayscale_textures;
		this->grayscale_textures = nullptr;
		MakeTexture(this, true, nullptr);
	}

	GenFramesMap();
}

/**
**  Sets the original size for a graphic
**
*/
void CGraphic::SetOriginalSize()
{
	Assert(this->IsLoaded()); // can't resize before it's been loaded

	if (!Resized) {
		return;
	}
	
	if (!this->get_image().isNull()) {
		this->image = QImage();
	}
	this->frame_map.clear();

	if (this->textures) {
		glDeleteTextures(NumTextures, this->textures);
		delete[] this->textures;
		this->textures = nullptr;
	}

	if (this->grayscale_textures) {
		glDeleteTextures(NumTextures, this->grayscale_textures);
		delete[] this->grayscale_textures;
		this->grayscale_textures = nullptr;
	}

	this->Width = this->Height = 0;
	this->image = QImage();
	this->Load();

	this->Resized = false;
}

int CGraphic::get_frame_index(const QPoint &frame_pos) const
{
	return stratagus::point::to_index(frame_pos, this->get_frames_per_row());
}

QPoint CGraphic::get_frame_pos(const int frame_index) const
{
	return stratagus::point::from_index(frame_index, this->get_frames_per_row());
}

void FreeGraphics()
{
	std::shared_lock<std::shared_mutex> lock(CGraphic::mutex);

	std::map<std::string, CGraphic *>::iterator i;
	while (!CGraphic::graphics_by_filepath.empty()) {
		i = CGraphic::graphics_by_filepath.begin();
		lock.unlock();
		CGraphic::Free((*i).second);
		lock.lock();
	}
}

CFiller &CFiller::operator =(const CFiller &other_filler)
{
	if (other_filler.G == nullptr) {
		throw std::runtime_error("Tried to create a copy of a UI filler which has no graphics.");
	}

	this->G = CGraphic::New(other_filler.G->get_filepath().string());
	this->X = other_filler.X;
	this->Y = other_filler.Y;

	return *this;
}

CFiller::~CFiller()
{
	if (this->G != nullptr) {
		CGraphic::Free(this->G);
	}
}

void CFiller::Load()
{
	if (this->G != nullptr) {
		this->G->Load(false, stratagus::defines::get()->get_scale_factor());
	}

	this->X *= stratagus::defines::get()->get_scale_factor();
	this->Y *= stratagus::defines::get()->get_scale_factor();

	if (this->X < 0) {
		this->X = Video.Width + this->X;
	}
	
	if (this->Y < 0) {
		this->Y = Video.Height + this->Y;
	}

	this->loaded = true;
}

bool CFiller::OnGraphic(int x, int y) const
{
	x -= X;
	y -= Y;
	const int scale_factor = stratagus::defines::get()->get_scale_factor();
	if (x >= 0 && y >= 0 && x < this->G->get_width() && y < this->G->get_height()) {
		return this->G->get_image().pixelColor(x / scale_factor, y / scale_factor).alpha() != 0;
	}
	return false;
}
