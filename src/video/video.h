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
/**@name video.h - The video headerfile. */
//
//      (c) Copyright 1999-2022 by Lutz Sammer, Nehal Mistry, Jimmy Salmon,
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

#pragma once

#include "color.h"
#include "guichan.h"
#include "vec2i.h"
#include "util/fractional_int.h"
#include "video/color_modification.h"

#pragma warning(push, 0)
#include <QOpenGLTexture>
#pragma warning(pop)

namespace wyrmgus {
	class font;
	class map_template;
	class player_color;
	class renderer;
	class time_of_day;
}

extern bool ZoomNoResize;

class CGraphic : public gcn::Image
{
	struct frame_pos_t final {
		short int x;
		short int y;
	};

public:
	static void unload_all();
	static void free_all_textures();

	static std::map<std::string, std::weak_ptr<CGraphic>> graphics_by_filepath;
	static std::list<CGraphic *> graphics;

protected:
	static inline std::shared_mutex mutex;

public:
	explicit CGraphic(const std::filesystem::path &filepath, const player_color *conversible_player_color = nullptr)
		: filepath(filepath), conversible_player_color(conversible_player_color)
	{
		if (filepath.empty()) {
			throw std::runtime_error("Tried to create a CGraphic instance with an empty filepath.");
		}

		if (!std::filesystem::exists(filepath)) {
			throw std::runtime_error("Tried to create a CGraphic instance with a filepath to a non-existing file: \"" + filepath.string() + "\".");
		}
	}

	virtual ~CGraphic();

	// Draw
	void DrawClip(int x, int y, std::vector<std::function<void(renderer *)>> &render_commands);
	void DrawSub(int gx, int gy, int w, int h, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands);
	void DrawGrayscaleSub(int gx, int gy, int w, int h, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands);
	void DrawSubClip(int gx, int gy, int w, int h, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands);
	void DrawGrayscaleSubClip(int gx, int gy, int w, int h, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands);
	void DrawSubTrans(int gx, int gy, int w, int h, int x, int y, unsigned char alpha, std::vector<std::function<void(renderer *)>> &render_commands);
	void DrawSubClipTrans(int gx, int gy, int w, int h, int x, int y, unsigned char alpha, std::vector<std::function<void(renderer *)>> &render_commands);

	// Draw frame
	void DrawFrame(unsigned frame, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands);

	void DrawFrameClip(const unsigned frame, const int x, const int y, const time_of_day *time_of_day, const int show_percent, std::vector<std::function<void(renderer *)>> &render_commands);

	void DrawFrameClip(const unsigned frame, const int x, const int y, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands)
	{
		this->DrawFrameClip(frame, x, y, time_of_day, 100, render_commands);
	}

	void DrawFrameClip(const unsigned frame, const int x, const int y, std::vector<std::function<void(renderer *)>> &render_commands)
	{
		this->DrawFrameClip(frame, x, y, nullptr, render_commands);
	}

	void DrawFrameClipTrans(const unsigned frame, const int x, const int y, const int alpha, const time_of_day *time_of_day, const int show_percent, std::vector<std::function<void(renderer *)>> &render_commands);

	void DrawFrameClipTrans(const unsigned frame, const int x, const int y, const int alpha, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands)
	{
		this->DrawFrameClipTrans(frame, x, y, alpha, time_of_day, 100, render_commands);
	}

	void DrawFrameClipTrans(const unsigned frame, const int x, const int y, const int alpha, std::vector<std::function<void(renderer *)>> &render_commands)
	{
		this->DrawFrameClipTrans(frame, x, y, alpha, nullptr, render_commands);
	}

	void DrawGrayscaleFrameClip(unsigned frame, int x, int y, int show_percent, std::vector<std::function<void(renderer *)>> &render_commands);

	void DrawGrayscaleFrameClip(unsigned frame, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands)
	{
		this->DrawGrayscaleFrameClip(frame, x, y, 100, render_commands);
	}

	// Draw frame flipped horizontally
	void DrawFrameClipX(unsigned frame, int x, int y, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands);

	void DrawFrameClipX(unsigned frame, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands)
	{
		this->DrawFrameClipX(frame, x, y, nullptr, render_commands);
	}

	void DrawFrameClipTransX(unsigned frame, int x, int y, int alpha, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands);

	void DrawFrameClipTransX(unsigned frame, int x, int y, int alpha, std::vector<std::function<void(renderer *)>> &render_commands)
	{
		return this->DrawFrameClipTransX(frame, x, y, alpha, nullptr, render_commands);
	}

	static std::shared_ptr<CGraphic> New(const std::string &filepath, const int w = 0, const int h = 0);

	static std::shared_ptr<CGraphic> New(const std::string &filepath, const QSize &size)
	{
		return CGraphic::New(filepath, size.width(), size.height());
	}

	static std::shared_ptr<CGraphic> New(const std::filesystem::path &filepath, const QSize &size)
	{
		return CGraphic::New(filepath.string(), size);
	}

	void Load(const centesimal_int &scale_factor);
	void unload();
	void Resize(int w, int h);
	void SetOriginalSize();

	bool IsLoaded() const
	{
		return !this->image.isNull();
	}

	const std::filesystem::path &get_filepath() const
	{
		return this->filepath;
	}

	void set_filepath(const std::filesystem::path &filepath)
	{
		if (filepath.empty()) {
			throw std::runtime_error("Tried to set an empty filepath for a CGraphic instance.");
		}

		this->filepath = filepath;
	}

	//guichan
	virtual int getWidth() const override { return Width; }
	virtual int getHeight() const override { return Height; }

	QSize get_size() const
	{
		return QSize(this->GraphicWidth, this->GraphicHeight);
	}

	int get_width() const
	{
		return this->get_size().width();
	}

	int get_height() const
	{
		return this->get_size().height();
	}

	const QSize &get_original_size() const
	{
		return this->original_size;
	}

	QSize get_frame_size() const
	{
		return QSize(this->Width, this->Height);
	}

	int get_frame_width() const
	{
		return this->get_frame_size().width();
	}

	int get_frame_height() const
	{
		return this->get_frame_size().height();
	}

	const QSize &get_original_frame_size() const
	{
		return this->original_frame_size;
	}

	const QSize &get_loaded_frame_size() const
	{
		return this->loaded_frame_size;
	}

	int get_frames_per_row() const
	{
		return this->get_width() / this->get_frame_width();
	}

	int get_frames_per_column() const
	{
		return this->get_height() / this->get_frame_height();
	}

	int get_frame_count() const
	{
		return this->get_frames_per_row() * this->get_frames_per_column();
	}

	int get_frame_index(const QPoint &frame_pos) const;
	QPoint get_frame_pos(const int frame_index) const;

	const QImage &get_image() const
	{
		return this->image;
	}

	QImage create_modified_image(const color_modification &color_modification, const bool grayscale) const;

	const QImage *get_frame_image(const size_t frame_index, const color_modification &color_modification = {}, const bool grayscale = false) const
	{
		if (grayscale) {
			if (!this->grayscale_frame_images.empty()) {
				return &this->grayscale_frame_images.at(frame_index);
			}
		} else if (!color_modification.is_null()) {
			const auto find_iterator = this->modified_frame_images.find(color_modification);
			if (find_iterator != this->modified_frame_images.end()) {
				return &find_iterator->second.at(frame_index);
			}
		} else {
			if (!this->frame_images.empty()) {
				return &this->frame_images.at(frame_index);
			}
		}

		return nullptr;
	}

	void create_frame_images(const color_modification &color_modification, const bool grayscale);

	const QImage &get_or_create_frame_image(const size_t frame_index, const color_modification &color_modification, const bool grayscale)
	{
		if (!color_modification.is_null()) {
			if (color_modification.get_player_color() != nullptr && (color_modification.get_player_color() == this->get_conversible_player_color() || !this->has_player_color())) {
				const wyrmgus::color_modification modification(color_modification.get_hue_rotation(), color_modification.get_colorization(), color_modification.get_hue_ignored_colors(), nullptr, color_modification.get_red_change(), color_modification.get_green_change(), color_modification.get_blue_change());
				return this->get_or_create_frame_image(frame_index, modification, grayscale);
			}
		}

		const QImage *image = this->get_frame_image(frame_index, color_modification, grayscale);

		if (image == nullptr) {
			this->create_frame_images(color_modification, grayscale);
			image = this->get_frame_image(frame_index, color_modification, grayscale);
		}

		if (image == nullptr) {
			throw std::runtime_error("Failed to get or create frame image.");
		}

		return *image;
	}

	const player_color *get_conversible_player_color() const;

	bool has_player_color() const
	{
		return this->has_player_color_value;
	}

	const QOpenGLTexture *get_texture(const color_modification &color_modification, const bool grayscale) const
	{
		if (grayscale) {
			return this->grayscale_texture.get();
		} else if (!color_modification.is_null()) {
			const auto find_iterator = this->modified_textures.find(color_modification);
			if (find_iterator != this->modified_textures.end()) {
				return find_iterator->second.get();
			}
		} else {
			return this->texture.get();
		}

		return nullptr;
	}

	const QOpenGLTexture *get_or_create_texture(const color_modification &color_modification, const bool grayscale)
	{
		if (!color_modification.is_null()) {
			if (color_modification.get_player_color() != nullptr && (color_modification.get_player_color() == this->get_conversible_player_color() || !this->has_player_color())) {
				const wyrmgus::color_modification modification(color_modification.get_hue_rotation(), color_modification.get_colorization(), color_modification.get_hue_ignored_colors(), nullptr, color_modification.get_red_change(), color_modification.get_green_change(), color_modification.get_blue_change());
				return this->get_or_create_texture(modification, grayscale);
			}
		}

		const QOpenGLTexture *texture = this->get_texture(color_modification, grayscale);

		if (texture == nullptr) {
			this->create_texture(color_modification, grayscale);
			texture = this->get_texture(color_modification, grayscale);
		}

		if (texture == nullptr) {
			throw std::runtime_error("Failed to get or create texture.");
		}

		return texture;
	}

	void create_texture(const color_modification &color_modification, const bool grayscale);

	void render(const QPoint &pixel_pos, std::vector<std::function<void(renderer *)>> &render_commands);

	void render_frame(const int frame_index, const QPoint &pixel_pos, const color_modification &color_modification, const bool grayscale, const bool flip, const unsigned char opacity, const int show_percent, std::vector<std::function<void(renderer *)>> &render_commands);

	void render_frame(const int frame_index, const QPoint &pixel_pos, const color_modification &color_modification, std::vector<std::function<void(renderer *)>> &render_commands)
	{
		this->render_frame(frame_index, pixel_pos, color_modification, false, false, 255, 100, render_commands);
	}

	void render_frame(const int frame_index, const QPoint &pixel_pos, const player_color *player_color, const time_of_day *time_of_day, const bool grayscale, const bool flip, const unsigned char opacity, const int show_percent, std::vector<std::function<void(renderer *)>> &render_commands);

	void render_frame(const int frame_index, const QPoint &pixel_pos, const player_color *player_color, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands)
	{
		this->render_frame(frame_index, pixel_pos, player_color, time_of_day, false, false, 255, 100, render_commands);
	}

	void render_frame(const int frame_index, const QPoint &pixel_pos, std::vector<std::function<void(renderer *)>> &render_commands)
	{
		this->render_frame(frame_index, pixel_pos, nullptr, nullptr, render_commands);
	}

	void render_player_color_frame(const int frame_index, const QPoint &pixel_pos, const player_color *player_color, const time_of_day *time_of_day, const bool flip, const unsigned char opacity, const int show_percent, std::vector<std::function<void(renderer *)>> &render_commands)
	{
		this->render_frame(frame_index, pixel_pos, player_color, time_of_day, false, flip, opacity, show_percent, render_commands);
	}

	void render_grayscale_frame(const int frame_index, const QPoint &pixel_pos, const int show_percent, std::vector<std::function<void(renderer *)>> &render_commands)
	{
		this->render_frame(frame_index, pixel_pos, nullptr, nullptr, true, false, 255, show_percent, render_commands);
	}

	void render_rect(const QRect &rect, const QPoint &pixel_pos, const color_modification &color_modification, const bool grayscale, const unsigned char opacity, std::vector<std::function<void(renderer *)>> &render_commands);

	bool has_textures() const
	{
		return this->texture != nullptr || this->grayscale_texture != nullptr || !this->modified_textures.empty();
	}

	void free_textures();

private:
	std::filesystem::path filepath;
public:
	std::string HashFile;      /// Filename used in hash
private:
	QImage image;
	std::vector<QImage> frame_images;
	std::vector<QImage> grayscale_frame_images;
	std::map<color_modification, std::vector<QImage>> modified_frame_images;
public:
	std::vector<frame_pos_t> frame_map;
	std::vector<frame_pos_t> frameFlip_map;
	void GenFramesMap();
	int Width = 0;					/// Width of a frame
	int Height = 0;					/// Height of a frame
	QSize original_size = QSize(0, 0); //the unscaled size
	QSize original_frame_size = QSize(0, 0); //the unscaled frame size
	QSize loaded_frame_size = QSize(0, 0); //the size of the image as loaded, before being scaled
	int NumFrames = 1;				/// Number of frames
	int GraphicWidth = 0;			/// Original graphic width
	int GraphicHeight = 0;			/// Original graphic height
private:
	const player_color *conversible_player_color = nullptr;
public:
	bool Resized = false;			/// Image has been resized
private:
	std::unique_ptr<QOpenGLTexture> texture;
	std::unique_ptr<QOpenGLTexture> grayscale_texture;
	std::map<color_modification, std::unique_ptr<QOpenGLTexture>> modified_textures;
	centesimal_int custom_scale_factor = centesimal_int(1); //the scale factor of the loaded image, if it is a custom scaled image
	bool has_player_color_value = false;
	std::mutex load_mutex;

	friend wyrmgus::font;
	friend int LoadGraphicPNG(CGraphic *g, const centesimal_int &scale_factor);
};

class CPlayerColorGraphic final : public CGraphic
{
public:
	explicit CPlayerColorGraphic(const std::filesystem::path &filepath, const player_color *conversible_player_color)
		: CGraphic(filepath, conversible_player_color)
	{
	}

	void DrawPlayerColorSub(const color_modification &color_modification, int gx, int gy, int w, int h, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands);
	void DrawPlayerColorSubClip(const color_modification &color_modification, int gx, int gy, int w, int h, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands);
	void DrawPlayerColorFrameClipX(const player_color *player_color, unsigned frame, int x, int y, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands);

	void DrawPlayerColorFrameClip(const player_color *player_color, const unsigned frame, const int x, const int y, const time_of_day *time_of_day, const int show_percent, std::vector<std::function<void(renderer *)>> &render_commands);

	void DrawPlayerColorFrameClip(const player_color *player_color, const unsigned frame, const int x, const int y, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands)
	{
		this->DrawPlayerColorFrameClip(player_color, frame, x, y, time_of_day, 100, render_commands);
	}

	void DrawPlayerColorFrameClipTransX(const player_color *player_color, unsigned frame, int x, int y, int alpha, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands);
	void DrawPlayerColorFrameClipTrans(const player_color *player_color, unsigned frame, int x, int y, int alpha, const time_of_day *time_of_day, int show_percent, std::vector<std::function<void(renderer *)>> &render_commands);

	void DrawPlayerColorFrameClipTrans(const player_color *player_color, unsigned frame, int x, int y, int alpha, const time_of_day *time_of_day, std::vector<std::function<void(renderer *)>> &render_commands)
	{
		this->DrawPlayerColorFrameClipTrans(player_color, frame, x, y, alpha, time_of_day, 100, render_commands);
	}

	static std::shared_ptr<CPlayerColorGraphic> New(const std::string &filepath, const QSize &size, const player_color *conversible_player_color);

	static std::shared_ptr<CPlayerColorGraphic> New(const std::filesystem::path &filepath, const QSize &size, const player_color *conversible_player_color)
	{
		return CPlayerColorGraphic::New(filepath.string(), size, conversible_player_color);
	}

	static CPlayerColorGraphic *Get(const std::string &file);
};

/**
**  Event call back.
**
**  This is placed in the video part, because it depends on the video
**  hardware driver.
*/
struct EventCallback final
{
	/// Callback for mouse button press
	void (*ButtonPressed)(unsigned buttons, const Qt::KeyboardModifiers key_modifiers) = nullptr;
	/// Callback for mouse button release
	void (*ButtonReleased)(unsigned buttons, const Qt::KeyboardModifiers key_modifiers) = nullptr;
	/// Callback for mouse move
	void (*MouseMoved)(const PixelPos &screenPos, const Qt::KeyboardModifiers key_modifiers) = nullptr;
	/// Callback for mouse exit of game window
	void (*MouseExit)() = nullptr;

	/// Callback for key press
	boost::asio::awaitable<void> (*KeyPressed)(unsigned keycode, unsigned keychar, const Qt::KeyboardModifiers key_modifiers) = nullptr;
	/// Callback for key release
	void (*KeyReleased)(unsigned keycode, unsigned keychar, const Qt::KeyboardModifiers key_modifiers) = nullptr;
	/// Callback for key repeated
	void (*KeyRepeated)(unsigned keycode, unsigned keychar, const Qt::KeyboardModifiers key_modifiers) = nullptr;
};

class CVideo
{
public:
	void ClearScreen();
	bool ResizeScreen(int width, int height);

	void DrawVLine(uint32_t color, int x, int y, int height, std::vector<std::function<void(renderer *)>> &render_commands);

	void DrawHLine(uint32_t color, int x, int y, int width, std::vector<std::function<void(renderer *)>> &render_commands);

	void DrawLine(uint32_t color, int sx, int sy, int dx, int dy, std::vector<std::function<void(renderer *)>> &render_commands);
	void DrawLineClip(uint32_t color, const PixelPos &pos1, const PixelPos &pos2, std::vector<std::function<void(renderer *)>> &render_commands);

	void DrawRectangle(uint32_t color, int x, int y, int w, int h, std::vector<std::function<void(renderer *)>> &render_commands);
	void DrawTransRectangle(uint32_t color, int x, int y, int w, int h, unsigned char alpha, std::vector<std::function<void(renderer *)>> &render_commands);
	void DrawRectangleClip(uint32_t color, int x, int y, int w, int h, std::vector<std::function<void(renderer *)>> &render_commands);

	void FillRectangle(uint32_t color, int x, int y, int w, int h, std::vector<std::function<void(renderer *)>> &render_commands);
	void FillTransRectangle(uint32_t color, int x, int y, int w, int h, unsigned char alpha, std::vector<std::function<void(renderer *)>> &render_commands);
	void FillRectangleClip(uint32_t color, int x, int y, int w, int h, std::vector<std::function<void(renderer *)>> &render_commands);
	void FillTransRectangleClip(uint32_t color, int x, int y, int w, int h, unsigned char alpha, std::vector<std::function<void(renderer *)>> &render_commands);

	static uint32_t MapRGB(const uint8_t r, const uint8_t g, const uint8_t b)
	{
		return CVideo::MapRGBA(r, g, b, 0xFF);
	}

	static uint32_t MapRGB(const CColor &color)
	{
		return CVideo::MapRGB(color.R, color.G, color.B);
	}

	static uint32_t MapRGB(const QColor &color)
	{
		return CVideo::MapRGB(color.red(), color.green(), color.blue());
	}

	static uint32_t MapRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

	static uint32_t MapRGBA(const CColor &color)
	{
		return CVideo::MapRGBA(color.R, color.G, color.B, color.A);
	}

	static uint32_t MapRGBA(const QColor &color)
	{
		return CVideo::MapRGBA(color.red(), color.green(), color.blue(), color.alpha());
	}

	static void GetRGB(uint32_t c, uint8_t *r, uint8_t *g, uint8_t *b);
	static void GetRGBA(uint32_t c, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a);

	static QColor GetRGBA(const uint32_t c);

	int Width = 0;
	int Height = 0;
	int ViewportWidth;         /// Actual width of the window
	int ViewportHeight;        /// Actual height of the window
	int Depth = 0;
};

extern CVideo Video;

/**
**  Video synchronization speed. Synchronization time in percent.
**  If =0, video framerate is not synchronized. 100 is exact
**  CYCLES_PER_SECOND (30). Game will try to redraw screen within
**  intervals of VideoSyncSpeed, not more, not less.
**  @see CYCLES_PER_SECOND
*/
extern int VideoSyncSpeed;

extern int SkipFrames;

/// Next frame ticks
extern double NextFrameTicks;

/// Counts frames
extern unsigned long FrameCounter;

/// Counts quantity of slow frames
extern unsigned long SlowFrameCounter;

/// Initialize Pixels[] for all players.
/// (bring Players[] in sync with Pixels[])
extern void SetPlayersPalette();

/// initialize the video part
extern void InitVideo();

/// deinitialize the video part
void DeInitVideo();

/// Load graphic from PNG file
extern int LoadGraphicPNG(CGraphic *g, const centesimal_int &scale_factor);

/// Regenerate Window screen if needed
extern void ValidateOpenGLScreen();

/// Initializes video synchronization.
extern void SetVideoSync();

/// Set clipping for nearly all vector primitives. Functions which support
/// clipping will be marked Clip. Set the system-wide clipping rectangle.
extern void SetClipping(int left, int top, int right, int bottom);

/// Save a screenshot to a PNG file
extern void SaveScreenshotPNG(const char *name);

/// Set the current callbacks
extern void SetCallbacks(const EventCallback *callbacks);
/// Get the current callbacks
extern const EventCallback *GetCallbacks();

/// Process all system events. Returns if the time for a frame is over
[[nodiscard]]
extern boost::asio::awaitable<void> WaitEventsOneFrame();

/// Push current clipping.
extern void PushClipping();

/// Pop current clipping.
extern void PopClipping();

/// Returns the ticks in ms since start
extern unsigned long GetTicks();

/// Convert a SDL_Keycode to a string
extern const char *SdlKey2Str(int key);

extern EventCallback GameCallbacks;   /// Game callbacks
extern EventCallback EditorCallbacks; /// Editor callbacks

extern uint32_t ColorBlack;
extern uint32_t ColorDarkGreen;
extern uint32_t ColorLightBlue;
extern uint32_t ColorBlue;
extern uint32_t ColorOrange;
extern uint32_t ColorWhite;
extern uint32_t ColorLightGray;
extern uint32_t ColorGray;
extern uint32_t ColorDarkGray;
extern uint32_t ColorRed;
extern uint32_t ColorGreen;
extern uint32_t ColorYellow;

//called by tolua++
extern double get_scale_factor();
extern void pack_image_folder(const std::string &dir_path, const int frames_per_row = 5);
extern void index_image_to_image_palette(const std::string &image_path, const std::string &other_image_path);
