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
//      (c) Copyright 1999-2021 by Lutz Sammer, Nehal Mistry, Jimmy Salmon,
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
#include "util/container_util.h"
#include "util/image_util.h"
#include "util/log_util.h"
#include "util/point_util.h"
#include "video/font.h"
#include "video/render_context.h"
#include "video/renderer.h"
#include "video/video.h"
#include "xbrz.h"

std::map<std::string, std::weak_ptr<CGraphic>> CGraphic::graphics_by_filepath;
std::list<CGraphic *> CGraphic::graphics;

void CGraphic::free_all_textures()
{
	std::shared_lock<std::shared_mutex> lock(CGraphic::mutex);

	std::vector<std::future<void>> futures;

	std::vector<std::function<void(renderer *)>> render_commands;

	for (const auto &kv_pair : CGraphic::graphics_by_filepath) {
		std::shared_ptr<CGraphic> graphic = kv_pair.second.lock();

		if (!graphic->has_textures()) {
			continue;
		}

		std::shared_ptr<std::promise<void>> promise = std::make_shared<std::promise<void>>();
		std::future<void> future = promise->get_future();

		render_commands.push_back([graphic, promise](renderer *) {
			graphic->free_textures();
			promise->set_value();
		});

		futures.push_back(std::move(future));
	}

	for (font *font : font::get_all()) {
		font->free_textures(render_commands);
	}

	render_context::get()->set_commands(std::move(render_commands));

	//wait until all textures have been freed
	for (std::future<void> &future : futures) {
		future.wait();
	}
}

CGraphic::~CGraphic()
{
	std::unique_lock<std::shared_mutex> lock(CGraphic::mutex);

	CGraphic::graphics.remove(this);

	if (!this->HashFile.empty()) {
		CGraphic::graphics_by_filepath.erase(this->HashFile);
	}

	if (this->has_textures()) {
		log::log_error("Graphic \"" + this->get_filepath().string() + "\" is being destroyed before its textures have been freed in the Qt OpenGL context.");
	}
}

/**
**  Video draw the graphic clipped.
**
**  @param x   X screen position
**  @param y   Y screen position
*/
void CGraphic::DrawClip(int x, int y, std::vector<std::function<void(renderer *)>> &render_commands)
{
	int oldx = x;
	int oldy = y;
	int w = Width;
	int h = Height;
	CLIP_RECTANGLE(x, y, w, h);
	DrawSub(x - oldx, y - oldy, w, h, x, y, render_commands);
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
void CGraphic::DrawSub(const int gx, const int gy, const int w, const int h, const int x, const int y, std::vector<std::function<void(renderer *)>> &render_commands)
{
	this->render_rect(QRect(gx, gy, w, h), QPoint(x, y), color_modification(), false, 255, render_commands);
}

void CGraphic::DrawGrayscaleSub(int gx, int gy, int w, int h, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands)
{
	this->render_rect(QRect(gx, gy, w, h), QPoint(x, y), color_modification(), true, 255, render_commands);
}

void CPlayerColorGraphic::DrawPlayerColorSub(const color_modification &color_modification, int gx, int gy, int w, int h, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands)
{
	this->render_rect(QRect(gx, gy, w, h), QPoint(x, y), color_modification, false, 255, render_commands);
}

void CGraphic::DrawSubClip(const int gx, const int gy, int w, int h, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands)
{
	int oldx = x;
	int oldy = y;
	CLIP_RECTANGLE(x, y, w, h);
	DrawSub(gx + x - oldx, gy + y - oldy, w, h, x, y, render_commands);
}

void CGraphic::DrawGrayscaleSubClip(int gx, int gy, int w, int h, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands)
{
	int oldx = x;
	int oldy = y;
	CLIP_RECTANGLE(x, y, w, h);
	DrawGrayscaleSub(gx + x - oldx, gy + y - oldy, w, h, x, y, render_commands);
}

void CPlayerColorGraphic::DrawPlayerColorSubClip(const color_modification &color_modification, int gx, int gy, int w, int h, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands)
{
	int oldx = x;
	int oldy = y;
	CLIP_RECTANGLE(x, y, w, h);
	DrawPlayerColorSub(color_modification, gx + x - oldx, gy + y - oldy, w, h, x, y, render_commands);
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
void CGraphic::DrawSubTrans(const int gx, const int gy, const int w, const int h, const int x, const int y, const unsigned char alpha, std::vector<std::function<void(renderer *)>> &render_commands)
{
	this->render_rect(QRect(gx, gy, w, h), QPoint(x, y), color_modification(), false, alpha, render_commands);
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
void CGraphic::DrawSubClipTrans(const int gx, const int gy, int w, int h, int x, int y, const unsigned char alpha, std::vector<std::function<void(renderer *)>> &render_commands)
{
	int oldx = x;
	int oldy = y;
	CLIP_RECTANGLE(x, y, w, h);
	DrawSubTrans(gx + x - oldx, gy + y - oldy, w, h, x, y, alpha, render_commands);
}

/**
**  Draw graphic object unclipped.
**
**  @param frame   number of frame (object index)
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
*/
void CGraphic::DrawFrame(unsigned frame, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands)
{
	this->render_frame(frame, QPoint(x, y), render_commands);
}

/**
**  Draw graphic object clipped.
**
**  @param frame   number of frame (object index)
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
*/
void CGraphic::DrawFrameClip(const unsigned frame, const int x, const int y, const time_of_day *time_of_day, const int show_percent, std::vector<std::function<void(renderer *)>> &render_commands)
{
	this->render_frame(frame, QPoint(x, y), nullptr, time_of_day, false, false, 255, show_percent, render_commands);
}

void CGraphic::DrawFrameClipTrans(const unsigned frame, const int x, const int y, const int alpha, const time_of_day *time_of_day, const int show_percent, std::vector<std::function<void(renderer *)>> &render_commands)
{
	this->render_frame(frame, QPoint(x, y), nullptr, time_of_day, false, false, alpha, show_percent, render_commands);
}

void CGraphic::DrawGrayscaleFrameClip(unsigned frame, int x, int y, int show_percent, std::vector<std::function<void(renderer *)>> &render_commands)
{
	this->render_grayscale_frame(frame, QPoint(x, y), show_percent, render_commands);
}

void CPlayerColorGraphic::DrawPlayerColorFrameClip(const player_color *player_color, const unsigned frame, const int x, const int y, const time_of_day *time_of_day, const int show_percent, std::vector<std::function<void(renderer *)>> &render_commands)
{
	this->render_player_color_frame(frame, QPoint(x, y), player_color, time_of_day, false, 255, show_percent, render_commands);
}

//Wyrmgus start
void CPlayerColorGraphic::DrawPlayerColorFrameClipTrans(const player_color *player_color, unsigned frame, int x, int y, int alpha, const time_of_day *time_of_day, int show_percent, std::vector<std::function<void(renderer *)>> &render_commands)
{
	this->render_player_color_frame(frame, QPoint(x, y), player_color, time_of_day, false, alpha, show_percent, render_commands);
}

void CPlayerColorGraphic::DrawPlayerColorFrameClipTransX(const player_color *player_color, unsigned frame, int x, int y, int alpha, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands)
{
	this->render_player_color_frame(frame, QPoint(x, y), player_color, time_of_day, true, alpha, 100, render_commands);
}
//Wyrmgus end

/**
**  Draw graphic object clipped and flipped in X direction.
**
**  @param frame   number of frame (object index)
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
*/
//Wyrmgus start
//void CGraphic::DrawFrameClipX(unsigned frame, int x, int y) const
void CGraphic::DrawFrameClipX(unsigned frame, int x, int y, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands)
//Wyrmgus end
{
	this->render_frame(frame, QPoint(x, y), nullptr, time_of_day, false, true, 255, 100, render_commands);
}

//Wyrmgus start
//void CGraphic::DrawFrameClipTransX(unsigned frame, int x, int y, int alpha) const
void CGraphic::DrawFrameClipTransX(const unsigned frame, const int x, const int y, const int alpha, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands)
//Wyrmgus end
{
	this->render_frame(frame, QPoint(x, y), nullptr, time_of_day, false, true, alpha, 100, render_commands);
}

/**
**  Draw graphic object clipped, flipped, and with player colors.
**
**  @param player  player number
**  @param frame   number of frame (object index)
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
*/
void CPlayerColorGraphic::DrawPlayerColorFrameClipX(const player_color *player_color, unsigned frame, int x, int y, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands)
{
	this->render_player_color_frame(frame, QPoint(x, y), player_color, time_of_day, true, 255, 100, render_commands);
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
std::shared_ptr<CGraphic> CGraphic::New(const std::string &filepath, const int w, const int h)
{
	std::unique_lock<std::shared_mutex> lock(CGraphic::mutex);

	if (filepath.empty()) {
		throw std::runtime_error("CGraphic::New() called with an empty filepath.");
	}

	const std::string library_filepath = LibraryFileName(filepath.c_str());

	const auto find_iterator = CGraphic::graphics_by_filepath.find(library_filepath);
	if (find_iterator != CGraphic::graphics_by_filepath.end()) {
		std::shared_ptr<CGraphic> g = find_iterator->second.lock();
		Assert((w == 0 || g->Width == w) && (g->Height == h || h == 0));
		return g;
	}

	auto g = std::make_shared<CGraphic>(library_filepath);
	if (!g) {
		throw std::runtime_error("Out of memory");
	}

	// FIXME: use a constructor for this
	g->HashFile = library_filepath;
	g->Width = w;
	g->Height = h;
	g->original_frame_size = QSize(w, h);
	CGraphic::graphics_by_filepath[g->HashFile] = g;

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
std::shared_ptr<CPlayerColorGraphic> CPlayerColorGraphic::New(const std::string &filepath, const QSize &size, const wyrmgus::player_color *conversible_player_color)
{
	std::unique_lock<std::shared_mutex> lock(CGraphic::mutex);

	if (filepath.empty()) {
		throw std::runtime_error("CPlayerColorGraphic::New() called with an empty filepath.");
	}

	const std::string file = LibraryFileName(filepath.c_str());

	const auto find_iterator = CGraphic::graphics_by_filepath.find(file);
	if (find_iterator != CGraphic::graphics_by_filepath.end()) {
		std::shared_ptr<CGraphic> g = find_iterator->second.lock();
		Assert((size.width() == 0 || g->Width == size.width()) && (g->Height == size.height() || size.height() == 0));

		auto pcg = std::dynamic_pointer_cast<CPlayerColorGraphic>(g);

		if (pcg == nullptr) {
			throw std::runtime_error("Tried to create player color graphic \"" + file + "\", but it has already been created as a non-player color graphic.");
		}

		return pcg;
	}

	auto g = std::make_shared<CPlayerColorGraphic>(file, conversible_player_color);
	if (!g) {
		throw std::runtime_error("Out of memory");
	}

	// FIXME: use a constructor for this
	g->HashFile = file;
	g->Width = size.width();
	g->Height = size.height();
	g->original_frame_size = size;
	CGraphic::graphics_by_filepath[g->HashFile] = g;

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

	const auto find_iterator = CGraphic::graphics_by_filepath.find(file);
	if (find_iterator != CGraphic::graphics_by_filepath.end()) {
		return std::dynamic_pointer_cast<CPlayerColorGraphic>(find_iterator->second.lock()).get();
	}

	return nullptr;
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
	uint8_t r, g, b;

	for (int j = 0; j < Height; ++j) {
		for (int i = 0; i < Width; ++i) {
			uint32_t c = *reinterpret_cast<uint32_t *>(&reinterpret_cast<uint8_t *>(Surface->pixels)[i * 4 + j * Surface->pitch]);
			uint8_t a;

			CVideo::GetRGBA(c, Surface->format, &r, &g, &b, &a);
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
void CGraphic::Load(const int scale_factor)
{
	std::lock_guard lock(this->load_mutex);

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
		throw std::runtime_error("Invalid graphic (width, height) " + this->get_filepath().string() + "; Expected: (" + std::to_string(this->Width) + ", " + std::to_string(this->Height) + ")  Found: (" + std::to_string(this->GraphicWidth) + ", " + std::to_string(this->GraphicHeight) + ")");
	}

	NumFrames = GraphicWidth / Width * GraphicHeight / Height;

	this->has_player_color_value = false;
	const wyrmgus::color_set color_set = wyrmgus::image::get_colors(this->get_image());
	const wyrmgus::player_color *conversible_player_color = this->get_conversible_player_color();
	for (const QColor &color : color_set) {
		if (!this->has_player_color_value) {
			for (const QColor &player_color : conversible_player_color->get_colors()) {
				if (color.red() == player_color.red() && color.green() == player_color.green() && color.blue() == player_color.blue()) {
					this->has_player_color_value = true;
					break;
				}
			}

			if (this->has_player_color_value) {
				break;
			}
		}

		if (this->has_player_color_value) {
			break; //nothing left to check
		}
	}
	
	CGraphic::graphics.push_back(this);

	GenFramesMap();

	if (scale_factor != this->custom_scale_factor) {
		this->Resize(this->GraphicWidth * scale_factor / this->custom_scale_factor, this->GraphicHeight * scale_factor / this->custom_scale_factor);
	}
}

/**
**  Resize a graphic
**
**  @param w  New width of graphic.
**  @param h  New height of graphic.
*/
void CGraphic::Resize(int w, int h)
{
	Assert(this->IsLoaded()); // can't resize before it's been loaded

	if (this->GraphicWidth == w && this->GraphicHeight == h) {
		return;
	}

	const QSize old_size(this->GraphicWidth, this->GraphicHeight);
	const QSize old_frame_size(this->Width, this->Height);
	const QSize frame_size(this->Width * w / old_size.width(), this->Height * h / old_size.height());

	// Resizing the same image multiple times looks horrible
	// If the image has already been resized then get a clean copy first
	if (this->Resized) {
		this->SetOriginalSize();
		if (this->GraphicWidth == w && this->GraphicHeight == h) {
			return;
		}
	}

	this->Resized = true;

	this->GraphicWidth = w;
	this->GraphicHeight = h;

	this->Width = frame_size.width();
	this->Height = frame_size.height();

	this->GenFramesMap();
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
	
	this->frame_map.clear();

	this->Width = this->Height = 0;
	this->image = QImage();
	this->frame_images.clear();
	this->Load();

	this->Resized = false;
}

int CGraphic::get_frame_index(const QPoint &frame_pos) const
{
	return wyrmgus::point::to_index(frame_pos, this->get_frames_per_row());
}

QPoint CGraphic::get_frame_pos(const int frame_index) const
{
	return wyrmgus::point::from_index(frame_index, this->get_frames_per_row());
}

QImage CGraphic::create_modified_image(const color_modification &color_modification, const bool grayscale) const
{
	QImage image = this->get_image();

	if (image.format() != QImage::Format_RGBA8888) {
		image = image.convertToFormat(QImage::Format_RGBA8888);
	}

	if (grayscale) {
		if (Preference.SepiaForGrayscale) {
			ApplySepiaScale(image);
		} else {
			ApplyGrayScale(image);
		}
	} else if (!color_modification.is_null()) {
		if (color_modification.get_hue_rotation() != 0) {
			const color_set ignored_colors = container::to_set<std::vector<QColor>, color_set>(this->get_conversible_player_color()->get_colors());
			image::rotate_hue(image, color_modification.get_hue_rotation(), ignored_colors);
		}

		if (color_modification.get_player_color() != nullptr) {
			color_modification.get_player_color()->apply_to_image(image, this->get_conversible_player_color());
		}
	}

	const int scale_factor = defines::get()->get_scale_factor();
	if (scale_factor > 1 && scale_factor != this->custom_scale_factor) {
		image = image::scale(image, scale_factor, this->get_original_frame_size());
	}

	if (color_modification.has_rgb_change() && !grayscale) {
		unsigned char *image_data = image.bits();
		const int bpp = image.depth() / 8;

		const int red_change = color_modification.get_red_change();
		const int green_change = color_modification.get_green_change();
		const int blue_change = color_modification.get_blue_change();

		for (int i = 0; i < image.sizeInBytes(); i += bpp) {
			unsigned char &red = image_data[i];
			unsigned char &green = image_data[i + 1];
			unsigned char &blue = image_data[i + 2];

			red = std::clamp<int>(red + red_change, 0, 255);
			green = std::clamp<int>(green + green_change, 0, 255);
			blue = std::clamp<int>(blue + blue_change, 0, 255);
		}
	}

	return image;
}

void CGraphic::create_frame_images(const color_modification &color_modification)
{
	QImage image = this->create_modified_image(color_modification, false);

	std::vector<QImage> frames = image::to_frames(image, this->get_frame_size());

	if (color_modification.is_null()) {
		this->frame_images = std::move(frames);
	} else {
		this->modified_frame_images[color_modification] = std::move(frames);
	}
}

const wyrmgus::player_color *CGraphic::get_conversible_player_color() const
{
	if (this->conversible_player_color != nullptr) {
		return this->conversible_player_color;
	}

	return defines::get()->get_conversible_player_color();
}

void CGraphic::create_texture(const color_modification &color_modification, const bool grayscale)
{
	QImage image = this->create_modified_image(color_modification, grayscale);

	auto texture = std::make_unique<QOpenGLTexture>(image);

	if (grayscale) {
		this->grayscale_texture = std::move(texture);
	} else if (!color_modification.is_null()) {
		this->modified_textures[color_modification] = std::move(texture);
	} else {
		this->texture = std::move(texture);
	}
}

void CGraphic::render(const QPoint &pixel_pos, std::vector<std::function<void(renderer *)>> &render_commands)
{
	render_commands.push_back([this, pixel_pos](renderer *renderer) {
		const QOpenGLTexture *texture = this->get_or_create_texture(color_modification(), false);

		renderer->blit_texture(texture, pixel_pos, this->get_size(), false, 255, this->get_size());
	});
}

void CGraphic::render_frame(const int frame_index, const QPoint &pixel_pos, const color_modification &color_modification, const bool grayscale, const bool flip, const unsigned char opacity, const int show_percent, std::vector<std::function<void(renderer *)>> &render_commands)
{
	render_commands.push_back([this, frame_index, pixel_pos, color_modification, grayscale, flip, opacity, show_percent](renderer *renderer) {
		const QOpenGLTexture *texture = this->get_or_create_texture(color_modification, grayscale);

		renderer->blit_texture_frame(texture, pixel_pos, this->get_size(), frame_index, this->get_frame_size(), flip, opacity, show_percent);
	});
}

void CGraphic::render_frame(const int frame_index, const QPoint &pixel_pos, const player_color *player_color, const time_of_day *time_of_day, const bool grayscale, const bool flip, const unsigned char opacity, const int show_percent, std::vector<std::function<void(renderer *)>> &render_commands)
{
	const color_modification color_modification(0, player_color, time_of_day);

	this->render_frame(frame_index, pixel_pos, color_modification, grayscale, flip, opacity, show_percent, render_commands);
}

void CGraphic::render_rect(const QRect &rect, const QPoint &pixel_pos, const color_modification &color_modification, const bool grayscale, const unsigned char opacity, std::vector<std::function<void(renderer *)>> &render_commands)
{
	render_commands.push_back([this, rect, pixel_pos, color_modification, grayscale, opacity](renderer *renderer) {
		const QOpenGLTexture *texture = this->get_or_create_texture(color_modification, grayscale);

		renderer->blit_texture_frame(texture, pixel_pos, rect.topLeft(), rect.size(), false, opacity, 100, rect.size());
	});
}

void CGraphic::free_textures()
{
	this->texture.reset();
	this->grayscale_texture.reset();
	this->modified_textures.clear();
}

CFiller &CFiller::operator =(const CFiller &other_filler)
{
	if (other_filler.G == nullptr) {
		throw std::runtime_error("Tried to create a copy of a UI filler which has no graphics.");
	}

	this->G = other_filler.G;
	this->X = other_filler.X;
	this->Y = other_filler.Y;

	return *this;
}

void CFiller::Load()
{
	if (this->G != nullptr) {
		this->G->Load(defines::get()->get_scale_factor());
	}

	this->X *= defines::get()->get_scale_factor();
	this->Y *= defines::get()->get_scale_factor();

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
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();
	if (x >= 0 && y >= 0 && x < this->G->get_width() && y < this->G->get_height()) {
		return this->G->get_image().pixelColor(x / scale_factor, y / scale_factor).alpha() != 0;
	}
	return false;
}
