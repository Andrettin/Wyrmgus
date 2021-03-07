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

#pragma once

#include "guichan.h"

#include "color.h"
#include "vec2i.h"

struct SDL_Cursor;
struct SDL_Surface;
typedef float GLfloat;
typedef int GLint;
typedef unsigned int GLuint;

namespace wyrmgus {
	class font;
	class map_template;
	class player_color;
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
	static std::map<std::string, std::weak_ptr<CGraphic>> graphics_by_filepath;
	static std::list<CGraphic *> graphics;

protected:
	static inline std::shared_mutex mutex;

public:
	explicit CGraphic(const std::filesystem::path &filepath, const wyrmgus::player_color *conversible_player_color = nullptr)
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
	void DrawClip(int x, int y) const;
	void DrawSub(int gx, int gy, int w, int h, int x, int y) const;
	void DrawGrayscaleSub(int gx, int gy, int w, int h, int x, int y) const;
	void DrawSubClip(int gx, int gy, int w, int h, int x, int y) const;
	void DrawGrayscaleSubClip(int gx, int gy, int w, int h, int x, int y) const;
	void DrawSubTrans(int gx, int gy, int w, int h, int x, int y, unsigned char alpha) const;
	void DrawSubClipTrans(int gx, int gy, int w, int h, int x, int y, unsigned char alpha) const;

	// Draw frame
	void DrawFrame(unsigned frame, int x, int y) const;
	void DoDrawFrameClip(const GLuint *textures, unsigned frame, int x, int y, int show_percent = 100) const;
	void DrawFrameClip(unsigned frame, int x, int y, const wyrmgus::time_of_day *time_of_day = nullptr, int show_percent = 100);
	void DrawFrameTrans(unsigned frame, int x, int y, int alpha) const;
	void DrawFrameClipTrans(unsigned frame, int x, int y, int alpha, const wyrmgus::time_of_day *time_of_day = nullptr, int show_percent = 100);
	void DrawGrayscaleFrameClip(unsigned frame, int x, int y, int show_percent = 100);

	// Draw frame flipped horizontally
	void DrawFrameX(unsigned frame, int x, int y) const;
	void DoDrawFrameClipX(const GLuint *textures, unsigned frame, int x, int y) const;
	void DrawFrameClipX(unsigned frame, int x, int y, const wyrmgus::time_of_day *time_of_day = nullptr);
	void DrawFrameTransX(unsigned frame, int x, int y, int alpha) const;
	void DrawFrameClipTransX(unsigned frame, int x, int y, int alpha, const wyrmgus::time_of_day *time_of_day = nullptr);

	static std::shared_ptr<CGraphic> New(const std::string &filepath, const int w = 0, const int h = 0);

	static std::shared_ptr<CGraphic> New(const std::string &filepath, const QSize &size)
	{
		return CGraphic::New(filepath, size.width(), size.height());
	}

	static std::shared_ptr<CGraphic> New(const std::filesystem::path &filepath, const QSize &size)
	{
		return CGraphic::New(filepath.string(), size);
	}

	void Load(const bool create_grayscale_textures = false, const int scale_factor = 1);
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

	int get_frames_per_row() const
	{
		return this->get_width() / this->get_frame_width();
	}

	int get_frames_per_column() const
	{
		return this->get_height() / this->get_frame_height();
	}

	int get_frame_index(const QPoint &frame_pos) const;
	QPoint get_frame_pos(const int frame_index) const;

	const QImage &get_image() const
	{
		return this->image;
	}

	const QImage &get_scaled_image() const
	{
		if (!this->scaled_image.isNull()) {
			return this->scaled_image;
		}

		return this->get_image();
	}

	void set_scaled_image(const QImage &image)
	{
		this->scaled_image = image;
	}

	const wyrmgus::player_color *get_conversible_player_color() const;

	bool has_player_color() const
	{
		return this->player_color;
	}

	const GLuint *get_textures() const
	{
		return this->textures.get();
	}

	const GLuint *get_textures(const CColor &color_modification) const
	{
		auto find_iterator = this->texture_color_modifications.find(color_modification);
		if (find_iterator != this->texture_color_modifications.end()) {
			return find_iterator->second.get();
		}

		return nullptr;
	}

	const GLuint *get_grayscale_textures() const
	{
		return this->grayscale_textures.get();
	}

private:
	std::filesystem::path filepath;
public:
	std::string HashFile;      /// Filename used in hash
private:
	QImage image;
	QImage scaled_image;
public:
	std::vector<frame_pos_t> frame_map;
	std::vector<frame_pos_t> frameFlip_map;
	void GenFramesMap();
	int Width = 0;					/// Width of a frame
	int Height = 0;					/// Height of a frame
	QSize original_size = QSize(0, 0); //the unscaled size
	QSize original_frame_size = QSize(0, 0); //the unscaled frame size
	int NumFrames = 1;				/// Number of frames
	int GraphicWidth = 0;			/// Original graphic width
	int GraphicHeight = 0;			/// Original graphic height
private:
	const wyrmgus::player_color *conversible_player_color = nullptr;
public:
	bool Resized = false;			/// Image has been resized
public:
	GLfloat TextureWidth = 0.f;      /// Width of the texture
	GLfloat TextureHeight = 0.f;     /// Height of the texture
	std::unique_ptr<GLuint[]> textures; //texture names
	std::unique_ptr<GLuint[]> grayscale_textures;
	std::map<CColor, std::unique_ptr<GLuint[]>> texture_color_modifications; //textures with a color modification applied to them
	int NumTextures = 0;           /// Number of textures
private:
	int custom_scale_factor = 1; //the scale factor of the loaded image, if it is a custom scaled image
	bool player_color = false;

	friend wyrmgus::font;
	friend int LoadGraphicPNG(CGraphic *g, const int scale_factor);
};

class CPlayerColorGraphic final : public CGraphic
{
public:
	explicit CPlayerColorGraphic(const std::filesystem::path &filepath, const wyrmgus::player_color *conversible_player_color)
		: CGraphic(filepath, conversible_player_color)
	{
	}

	virtual ~CPlayerColorGraphic() override;

	void DrawPlayerColorSub(const wyrmgus::player_color *player_color, int gx, int gy, int w, int h, int x, int y);
	void DrawPlayerColorSubClip(const wyrmgus::player_color *player_color, int gx, int gy, int w, int h, int x, int y);
	void DrawPlayerColorFrameClipX(const wyrmgus::player_color *player_color, unsigned frame, int x, int y, const wyrmgus::time_of_day *time_of_day = nullptr);
	void DrawPlayerColorFrameClip(const wyrmgus::player_color *player_color, unsigned frame, int x, int y, const wyrmgus::time_of_day *time_of_day = nullptr, int show_percent = 100);
	void DrawPlayerColorFrameClipTransX(const wyrmgus::player_color *player_color, unsigned frame, int x, int y, int alpha, const wyrmgus::time_of_day *time_of_day = nullptr);
	void DrawPlayerColorFrameClipTrans(const wyrmgus::player_color *player_color, unsigned frame, int x, int y, int alpha, const wyrmgus::time_of_day *time_of_day = nullptr, int show_percent = 100);

	static std::shared_ptr<CPlayerColorGraphic> New(const std::string &filepath, const QSize &size, const wyrmgus::player_color *conversible_player_color);

	static std::shared_ptr<CPlayerColorGraphic> New(const std::filesystem::path &filepath, const QSize &size, const wyrmgus::player_color *conversible_player_color)
	{
		return CPlayerColorGraphic::New(filepath.string(), size, conversible_player_color);
	}

	static CPlayerColorGraphic *Get(const std::string &file);

	const GLuint *get_textures(const wyrmgus::player_color *player_color) const;
	const GLuint *get_textures(const wyrmgus::player_color *player_color, const CColor &color_modification) const;

	std::map<const wyrmgus::player_color *, std::unique_ptr<GLuint[]>> player_color_textures;
	std::map<const wyrmgus::player_color *, std::map<CColor, std::unique_ptr<GLuint[]>>> player_color_texture_color_modifications; //player color textures with a color modification applied to them
};

/**
**  Event call back.
**
**  This is placed in the video part, because it depends on the video
**  hardware driver.
*/
struct EventCallback {

	/// Callback for mouse button press
	void (*ButtonPressed)(unsigned buttons);
	/// Callback for mouse button release
	void (*ButtonReleased)(unsigned buttons);
	/// Callback for mouse move
	void (*MouseMoved)(const PixelPos &screenPos);
	/// Callback for mouse exit of game window
	void (*MouseExit)();

	/// Callback for key press
	void (*KeyPressed)(unsigned keycode, unsigned keychar);
	/// Callback for key release
	void (*KeyReleased)(unsigned keycode, unsigned keychar);
	/// Callback for key repeated
	void (*KeyRepeated)(unsigned keycode, unsigned keychar);

	/// Callback for network event
	void (*NetworkEvent)();

};

class CVideo
{
public:
	CVideo() : Width(0), Height(0), Depth(0), FullScreen(false) {}

	void ClearScreen();
	bool ResizeScreen(int width, int height);

	void DrawPixelClip(uint32_t color, int x, int y);
	void DrawTransPixelClip(uint32_t color, int x, int y, unsigned char alpha);

	void DrawVLine(uint32_t color, int x, int y, int height);
	void DrawTransVLine(uint32_t color, int x, int y, int height, unsigned char alpha);
	void DrawVLineClip(uint32_t color, int x, int y, int height);
	void DrawTransVLineClip(uint32_t color, int x, int y, int height, unsigned char alpha);

	void DrawHLine(uint32_t color, int x, int y, int width);
	void DrawTransHLine(uint32_t color, int x, int y, int width, unsigned char alpha);
	void DrawHLineClip(uint32_t color, int x, int y, int width);
	void DrawTransHLineClip(uint32_t color, int x, int y, int width, unsigned char alpha);

	void DrawLine(uint32_t color, int sx, int sy, int dx, int dy);
	void DrawTransLine(uint32_t color, int sx, int sy, int dx, int dy, unsigned char alpha);
	void DrawLineClip(uint32_t color, const PixelPos &pos1, const PixelPos &pos2);
	void DrawTransLineClip(uint32_t color, int sx, int sy, int dx, int dy, unsigned char alpha);

	void DrawRectangle(uint32_t color, int x, int y, int w, int h);
	void DrawTransRectangle(uint32_t color, int x, int y, int w, int h, unsigned char alpha);
	void DrawRectangleClip(uint32_t color, int x, int y, int w, int h);
	void DrawTransRectangleClip(uint32_t color, int x, int y, int w, int h, unsigned char alpha);

	void FillRectangle(uint32_t color, int x, int y, int w, int h);
	void FillTransRectangle(uint32_t color, int x, int y, int w, int h, unsigned char alpha);
	void FillRectangleClip(uint32_t color, int x, int y, int w, int h);
	void FillTransRectangleClip(uint32_t color, int x, int y, int w, int h, unsigned char alpha);

	void DrawCircle(uint32_t color, int x, int y, int r);
	void DrawTransCircle(uint32_t color, int x, int y, int r, unsigned char alpha);
	void DrawCircleClip(uint32_t color, int x, int y, int r);
	void DrawTransCircleClip(uint32_t color, int x, int y, int r, unsigned char alpha);

	void FillCircle(uint32_t color, int x, int y, int radius);
	void FillTransCircle(uint32_t color, int x, int y, int radius, unsigned char alpha);
	void FillCircleClip(uint32_t color, const PixelPos &screenPos, int radius);
	void FillTransCircleClip(uint32_t color, int x, int y, int radius, unsigned char alpha);

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

	int Width;
	int Height;
#if defined(USE_OPENGL) || defined(USE_GLES)
	int ViewportWidth;         /// Actual width of the window
	int ViewportHeight;        /// Actual height of the window
#endif
//Wyrmgus start
//#if defined(USE_TOUCHSCREEN) && defined(USE_WIN32)
//Wyrmgus end
	SDL_Cursor *blankCursor;
//Wyrmgus start
//#endif
//Wyrmgus end
	int Depth;
	bool FullScreen;
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

/// Fullscreen or windowed set from commandline.
extern char VideoForceFullScreen;

/// Next frame ticks
extern double NextFrameTicks;

/// Counts frames
extern unsigned long FrameCounter;

/// Counts quantity of slow frames
extern unsigned long SlowFrameCounter;

/// Initialize Pixels[] for all players.
/// (bring Players[] in sync with Pixels[])
extern void SetPlayersPalette();

/// The SDL screen
extern SDL_Surface *TheScreen;

#if defined(USE_OPENGL) || defined(USE_GLES)
/// Max texture size supported on the video card
extern GLint GLMaxTextureSize;
/// User-specified limit for ::GLMaxTextureSize
extern GLint GLMaxTextureSizeOverride;
/// Is OpenGL texture compression supported
extern bool GLTextureCompressionSupported;
/// Use OpenGL texture compression
extern bool UseGLTextureCompression;
#endif

/// register lua function
extern void VideoCclRegister();

/// initialize the video part
extern void InitVideo();

/// deinitialize the video part
void DeInitVideo();

/// Check if a resolution is valid
extern int VideoValidResolution(int w, int h);

/// Load graphic from PNG file
extern int LoadGraphicPNG(CGraphic *g, const int scale_factor);

#if defined(USE_OPENGL) || defined(USE_GLES)

/// Make an OpenGL texture
extern void MakeTexture(CGraphic *graphic, const bool grayscale, const wyrmgus::time_of_day *time_of_day);
//Wyrmgus start
extern void MakeTextures2(const QImage &image, GLuint texture, const int ow, const int oh, const wyrmgus::time_of_day *time_of_day = nullptr);
//Wyrmgus end
extern void MakePlayerColorTexture(CPlayerColorGraphic *graphic, const wyrmgus::player_color *player_color, const wyrmgus::time_of_day *time_of_day = nullptr);

/// Regenerate Window screen if needed
extern void ValidateOpenGLScreen();

/// Free OpenGL graphics
extern void FreeOpenGLGraphics();
/// Reload OpenGL graphics
extern void ReloadGraphics();
/// Reload OpenGL
extern void ReloadOpenGL();

#endif

/// Initializes video synchronization.
extern void SetVideoSync();

/// Init line draw
extern void InitLineDraw();

/// Set clipping for nearly all vector primitives. Functions which support
/// clipping will be marked Clip. Set the system-wide clipping rectangle.
extern void SetClipping(int left, int top, int right, int bottom);

/// Realize video memory.
extern void RealizeVideoMemory();

/// Save a screenshot to a PNG file
extern void SaveScreenshotPNG(const char *name);

/// Set the current callbacks
extern void SetCallbacks(const EventCallback *callbacks);
/// Get the current callbacks
extern const EventCallback *GetCallbacks();

/// Process all system events. Returns if the time for a frame is over
extern void WaitEventsOneFrame();

/// Poll all sdl events
extern void PollEvents();

/// Toggle full screen mode
extern void ToggleFullScreen();

/// Push current clipping.
extern void PushClipping();

/// Pop current clipping.
extern void PopClipping();

/// Returns the ticks in ms since start
extern unsigned long GetTicks();

/// Convert a SDLKey to a string
extern const char *SdlKey2Str(int key);

/// Check if the mouse is grabbed
extern bool SdlGetGrabMouse();
/// Toggle mouse grab mode
extern void ToggleGrabMouse(int mode);

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

#if defined(USE_OPENGL) || defined(USE_GLES)
void DrawTexture(const CGraphic *g, const GLuint *textures, int sx, int sy,
				 int ex, int ey, int x, int y, int flip);
#endif

//  Color Cycling stuff

extern void VideoPaletteListAdd(SDL_Surface *surface);
extern void VideoPaletteListRemove(SDL_Surface *surface);
extern void ClearAllColorCyclingRange();
extern void AddColorCyclingRange(unsigned int begin, unsigned int end);
extern void SetColorCycleAll(bool value);

//called by tolua++
extern int get_scale_factor();
extern int get_scale_factor_preference();
extern void set_scale_factor(const int factor);
extern void pack_image_folder(const std::string &dir_path, const int frames_per_row = 5);
extern void index_image_to_image_palette(const std::string &image_path, const std::string &other_image_path);
