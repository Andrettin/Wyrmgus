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
/**@name video.h - The video header file. */
//
//      (c) Copyright 1999-2011 by Lutz Sammer, Nehal Mistry, Jimmy Salmon and
//                                 Pali Rohár
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

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include "SDL.h"

#ifdef USE_GLES
#include "GLES/gl.h"
#endif

#ifdef USE_OPENGL
#ifdef __APPLE__
#define GL_GLEXT_PROTOTYPES 1
#endif
#include "SDL_opengl.h"
#include "video/shaders.h"
#endif

#include "guichan.h"

#include "vec2i.h"
#include "video/color.h"

class CFont;
class CMapTemplate;
class CTimeOfDay;

extern bool ZoomNoResize;
extern bool GLShaderPipelineSupported;

class CGraphic : public gcn::Image
{

	struct frame_pos_t {
		short int x;
		short int y;
	};

protected:
	~CGraphic() {}

public:
	// Draw
	void DrawClip(int x, int y) const;
	//Wyrmgus start
	/*
	void DrawSub(int gx, int gy, int w, int h, int x, int y) const;
	void DrawSubClip(int gx, int gy, int w, int h, int x, int y) const;
	void DrawSubTrans(int gx, int gy, int w, int h, int x, int y,
					  unsigned char alpha) const;
	void DrawSubClipTrans(int gx, int gy, int w, int h, int x, int y,
						  unsigned char alpha) const;
	*/
	void DrawSub(int gx, int gy, int w, int h, int x, int y, SDL_Surface *surface = nullptr) const;
	void DrawSubClip(int gx, int gy, int w, int h, int x, int y, SDL_Surface *surface = nullptr) const;
	void DrawSubTrans(int gx, int gy, int w, int h, int x, int y,
					  unsigned char alpha, SDL_Surface *surface = nullptr) const;
	void DrawSubClipTrans(int gx, int gy, int w, int h, int x, int y,
						  unsigned char alpha, SDL_Surface *surface = nullptr) const;
	//Wyrmgus end

	// Draw frame
	void DrawFrame(unsigned frame, int x, int y) const;
	void DoDrawFrameClip(GLuint *textures, unsigned frame, int x, int y, int show_percent = 100) const;
	//Wyrmgus start
//	void DrawFrameClip(unsigned frame, int x, int y) const;
	void DrawFrameClip(unsigned frame, int x, int y, bool ignore_time_of_day = true, SDL_Surface *surface = nullptr, int show_percent = 100);
	//Wyrmgus end
	void DrawFrameTrans(unsigned frame, int x, int y, int alpha) const;
	//Wyrmgus start
//	void DrawFrameClipTrans(unsigned frame, int x, int y, int alpha) const;
	void DrawFrameClipTrans(unsigned frame, int x, int y, int alpha, bool ignore_time_of_day = true, SDL_Surface *surface = nullptr, int show_percent = 100);
	//Wyrmgus end

	// Draw frame flipped horizontally
	void DrawFrameX(unsigned frame, int x, int y) const;
	void DoDrawFrameClipX(GLuint *textures, unsigned frame, int x, int y) const;
	//Wyrmgus start
//	void DrawFrameClipX(unsigned frame, int x, int y) const;
	void DrawFrameClipX(unsigned frame, int x, int y, bool ignore_time_of_day = true, SDL_Surface *surface = nullptr);
	//Wyrmgus end
	void DrawFrameTransX(unsigned frame, int x, int y, int alpha) const;
	//Wyrmgus start
//	void DrawFrameClipTransX(unsigned frame, int x, int y, int alpha) const;
	void DrawFrameClipTransX(unsigned frame, int x, int y, int alpha, bool ignore_time_of_day = true, SDL_Surface *surface = nullptr);
	//Wyrmgus end

	static CGraphic *New(const std::string &file, int w = 0, int h = 0);
	static CGraphic *ForceNew(const std::string &file, int w = 0, int h = 0);
	static CGraphic *Get(const std::string &file);

	static void Free(CGraphic *g);

	void Load(bool grayscale = false);
	void Resize(int w, int h);
	void SetOriginalSize();
	SDL_Surface *SetTimeOfDay(CTimeOfDay *time_of_day, bool flipped = false);
	bool TransparentPixel(int x, int y);
	void MakeShadow();

	inline bool IsLoaded() const { return Surface != nullptr; }

	//guichan
	//Wyrmgus start
//	virtual void *_getData() const { return Surface; }
	virtual void *_getData(int player_color = -1) { return Surface; }
	//Wyrmgus end
	virtual int getWidth() const { return Width; }
	virtual int getHeight() const { return Height; }
	//Wyrmgus start
	virtual std::string getFile() const { return File; }
	virtual int getGraphicWidth() const { return GraphicWidth; }
	virtual int getGraphicHeight() const { return GraphicHeight; }
	//Wyrmgus end

	std::string File;			/// Filename
	std::string HashFile;		/// Filename used in hash
	SDL_Surface *Surface = nullptr;	/// Surface
	SDL_Surface *SurfaceFlip = nullptr;	/// Flipped surface
	//Wyrmgus start
	SDL_Surface *DawnSurface = nullptr;	/// Surface
	SDL_Surface *DawnSurfaceFlip = nullptr;	/// Flipped surface
	SDL_Surface *DuskSurface = nullptr;	/// Surface
	SDL_Surface *DuskSurfaceFlip = nullptr;	/// Flipped surface
	SDL_Surface *NightSurface = nullptr;	/// Surface
	SDL_Surface *NightSurfaceFlip = nullptr;	/// Flipped surface
	//Wyrmgus end
	frame_pos_t *frame_map = nullptr;
	frame_pos_t *frameFlip_map = nullptr;
	void GenFramesMap();
	int Width = 0;				/// Width of a frame
	int Height = 0;				/// Height of a frame
	int NumFrames = 1;			/// Number of frames
	int GraphicWidth = 0;		/// Original graphic width
	int GraphicHeight = 0;		/// Original graphic height
	int Refs = 1;				/// Uses of this graphic
	CTimeOfDay *TimeOfDay = nullptr;	/// Time of day for this graphic
	bool Resized = false;		/// Whether the image has been resized
	//Wyrmgus start
	bool Grayscale = false;
	//Wyrmgus end

	GLfloat TextureWidth = 0.f;		/// Width of the texture
	GLfloat TextureHeight = 0.f;	/// Height of the texture
	GLuint *Textures = nullptr;		/// Texture names
	std::map<CColor, GLuint *> TextureColorModifications;	/// Textures with a color modification applied to them
	int NumTextures = 0;			/// Number of textures

	friend class CFont;
};

class CPlayerColorGraphic : public CGraphic
{
protected:
	CPlayerColorGraphic()
	{
		//Wyrmgus start
		for (int i = 0; i < PlayerColorMax; ++i) {
			PlayerColorSurfaces[i] = nullptr;
			PlayerColorSurfacesFlip[i] = nullptr;
			PlayerColorSurfacesDawn[i] = nullptr;
			PlayerColorSurfacesDawnFlip[i] = nullptr;
			PlayerColorSurfacesDusk[i] = nullptr;
			PlayerColorSurfacesDuskFlip[i] = nullptr;
			PlayerColorSurfacesNight[i] = nullptr;
			PlayerColorSurfacesNightFlip[i] = nullptr;
		}
		//Wyrmgus end
		
		//Wyrmgus start
//		memset(PlayerColorTextures, 0, sizeof(PlayerColorTextures));
		for (int i = 0; i < PlayerColorMax; ++i) {
			PlayerColorTextures[i] = nullptr;
		}
		//Wyrmgus end
	}

public:
	//Wyrmgus start
	void DrawPlayerColorSub(int player, int gx, int gy, int w, int h, int x, int y);
	void DrawPlayerColorSubClip(int player, int gx, int gy, int w, int h, int x, int y);
//	void DrawPlayerColorFrameClipX(int player, unsigned frame, int x, int y);
//	void DrawPlayerColorFrameClip(int player, unsigned frame, int x, int y);
	void DrawPlayerColorFrameClipX(int player, unsigned frame, int x, int y, bool ignore_time_of_day = true);
	void DrawPlayerColorFrameClip(int player, unsigned frame, int x, int y, bool ignore_time_of_day = true, int show_percent = 100);
	void DrawPlayerColorFrameClipTransX(int player, unsigned frame, int x, int y, int alpha, bool ignore_time_of_day = true);
	void DrawPlayerColorFrameClipTrans(int player, unsigned frame, int x, int y, int alpha, bool ignore_time_of_day = true, int show_percent = 100);
	//Wyrmgus end

	static CPlayerColorGraphic *New(const std::string &file, int w = 0, int h = 0);
	static CPlayerColorGraphic *ForceNew(const std::string &file, int w = 0, int h = 0);
	static CPlayerColorGraphic *Get(const std::string &file);

	CPlayerColorGraphic *Clone(bool grayscale = false) const;
	
	//Wyrmgus start
	virtual void *_getData(int player_color = -1) {
		if (player_color == -1) {
			return Surface;
		}
		
		return PlayerColorSurfaces[player_color];
	}
	//Wyrmgus end

	//Wyrmgus start
	SDL_Surface *PlayerColorSurfaces[PlayerColorMax];			/// Surface
	SDL_Surface *PlayerColorSurfacesFlip[PlayerColorMax];		/// Flipped surface
	SDL_Surface *PlayerColorSurfacesDawn[PlayerColorMax];		/// Surface
	SDL_Surface *PlayerColorSurfacesDawnFlip[PlayerColorMax];	/// Flipped surface
	SDL_Surface *PlayerColorSurfacesDusk[PlayerColorMax];		/// Surface
	SDL_Surface *PlayerColorSurfacesDuskFlip[PlayerColorMax];	/// Flipped surface
	SDL_Surface *PlayerColorSurfacesNight[PlayerColorMax];		/// Surface
	SDL_Surface *PlayerColorSurfacesNightFlip[PlayerColorMax];	/// Flipped surface
	//Wyrmgus end
	
	GLuint *PlayerColorTextures[PlayerColorMax];				/// Textures with player colors
	std::map<int, std::map<CColor, GLuint *>> PlayerColorTextureColorModifications;	/// Player color textures with a color modification applied to them
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


class CVideo
{
public:
	void ClearScreen();
	bool ResizeScreen(int width, int height);

	void DrawPixelClip(Uint32 color, int x, int y);
	void DrawTransPixelClip(Uint32 color, int x, int y, unsigned char alpha);

	void DrawVLine(Uint32 color, int x, int y, int height);
	void DrawTransVLine(Uint32 color, int x, int y, int height, unsigned char alpha);
	void DrawVLineClip(Uint32 color, int x, int y, int height);
	void DrawTransVLineClip(Uint32 color, int x, int y, int height, unsigned char alpha);

	void DrawHLine(Uint32 color, int x, int y, int width);
	void DrawTransHLine(Uint32 color, int x, int y, int width, unsigned char alpha);
	void DrawHLineClip(Uint32 color, int x, int y, int width);
	void DrawTransHLineClip(Uint32 color, int x, int y, int width, unsigned char alpha);

	void DrawLine(Uint32 color, int sx, int sy, int dx, int dy);
	void DrawTransLine(Uint32 color, int sx, int sy, int dx, int dy, unsigned char alpha);
	void DrawLineClip(Uint32 color, const PixelPos &pos1, const PixelPos &pos2);
	void DrawTransLineClip(Uint32 color, int sx, int sy, int dx, int dy, unsigned char alpha);

	void DrawRectangle(Uint32 color, int x, int y, int w, int h);
	void DrawTransRectangle(Uint32 color, int x, int y, int w, int h, unsigned char alpha);
	void DrawRectangleClip(Uint32 color, int x, int y, int w, int h);
	void DrawTransRectangleClip(Uint32 color, int x, int y, int w, int h, unsigned char alpha);

	void FillRectangle(Uint32 color, int x, int y, int w, int h);
	void FillTransRectangle(Uint32 color, int x, int y, int w, int h, unsigned char alpha);
	void FillRectangleClip(Uint32 color, int x, int y, int w, int h);
	void FillTransRectangleClip(Uint32 color, int x, int y, int w, int h, unsigned char alpha);

	void DrawCircle(Uint32 color, int x, int y, int r);
	void DrawTransCircle(Uint32 color, int x, int y, int r, unsigned char alpha);
	void DrawCircleClip(Uint32 color, int x, int y, int r);
	void DrawTransCircleClip(Uint32 color, int x, int y, int r, unsigned char alpha);

	void FillCircle(Uint32 color, int x, int y, int radius);
	void FillTransCircle(Uint32 color, int x, int y, int radius, unsigned char alpha);
	void FillCircleClip(Uint32 color, const PixelPos &screenPos, int radius);
	void FillTransCircleClip(Uint32 color, int x, int y, int radius, unsigned char alpha);

	inline Uint32 MapRGB(SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b)
	{
		return MapRGBA(f, r, g, b, 0xFF);
	}
	
	inline Uint32 MapRGB(SDL_PixelFormat *f, const CColor &color)
	{
		return MapRGB(f, color.R, color.G, color.B);
	}
	
	inline Uint32 MapRGBA(SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
	{
		return ((r << RSHIFT) | (g << GSHIFT) | (b << BSHIFT) | (a << ASHIFT));
	}
	
	inline Uint32 MapRGBA(SDL_PixelFormat *f, const CColor &color)
	{
		return MapRGBA(f, color.R, color.G, color.B, color.A);
	}
	
	inline void GetRGB(Uint32 c, SDL_PixelFormat *f, Uint8 *r, Uint8 *g, Uint8 *b)
	{
		*r = (c >> RSHIFT) & 0xff;
		*g = (c >> GSHIFT) & 0xff;
		*b = (c >> BSHIFT) & 0xff;
	}
	
	inline void GetRGBA(Uint32 c, SDL_PixelFormat *f, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a)
	{
		*r = (c >> RSHIFT) & 0xff;
		*g = (c >> GSHIFT) & 0xff;
		*b = (c >> BSHIFT) & 0xff;
		*a = (c >> ASHIFT) & 0xff;
	}

	int Width = 0;
	int Height = 0;
	int ViewportWidth;         /// Actual width of the window
	int ViewportHeight;        /// Actual height of the window
//Wyrmgus start
//#if defined(USE_TOUCHSCREEN) && defined(_WIN32)
//Wyrmgus end
	SDL_Cursor *blankCursor = nullptr;
//Wyrmgus start
//#endif
//Wyrmgus end
	int Depth = 0;
	bool FullScreen = false;
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

/// Max texture size supported on the video card
extern GLint GLMaxTextureSize;
/// User-specified limit for ::GLMaxTextureSize
extern GLint GLMaxTextureSizeOverride;
/// Is OpenGL texture compression supported
extern bool GLTextureCompressionSupported;
/// Use OpenGL texture compression
extern bool UseGLTextureCompression;

/// register lua function
extern void VideoCclRegister();

/// initialize the video part
extern void InitVideo();

/// deinitliaize the video part
void DeInitVideo();

/// Check if a resolution is valid
extern int VideoValidResolution(int w, int h);

/// Load graphic from PNG file
extern int LoadGraphicPNG(CGraphic *g);

/// Make an OpenGL texture
//Wyrmgus start
//extern void MakeTexture(CGraphic *graphic);
extern void MakeTexture(CGraphic *graphic, const CTimeOfDay *time_of_day = nullptr);
//Wyrmgus end
//Wyrmgus start
extern void MakeTextures2(CGraphic *g, GLuint texture, CUnitColors *colors, const int ow, const int oh, const CTimeOfDay *time_of_day = nullptr);
//Wyrmgus end
/// Make an OpenGL texture of the player color pixels only.
//Wyrmgus start
//extern void MakePlayerColorTexture(CPlayerColorGraphic *graphic, int player);
extern void MakePlayerColorTexture(CPlayerColorGraphic *graphic, const int player, const CTimeOfDay *time_of_day = nullptr);
//Wyrmgus end

/// Regenerate Window screen if needed
extern void ValidateOpenGLScreen();

/// Free OpenGL graphics
extern void FreeOpenGLGraphics();
/// Reload OpenGL graphics
extern void ReloadGraphics();
/// Reload OpenGL
extern void ReloadOpenGL();

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

/// Save a screenshot to a PNG file
extern void SaveMapPNG(const char *name);

//Wyrmgus start
/// Save a map template's terrain to a PNG file
extern void SaveMapTemplatePNG(const char *name, const CMapTemplate *map_template, const bool overlay);
//Wyrmgus end

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

extern Uint32 ColorBlack;
extern Uint32 ColorDarkGreen;
extern Uint32 ColorLightBlue;
extern Uint32 ColorBlue;
extern Uint32 ColorOrange;
extern Uint32 ColorWhite;
extern Uint32 ColorLightGray;
extern Uint32 ColorGray;
extern Uint32 ColorDarkGray;
extern Uint32 ColorRed;
extern Uint32 ColorGreen;
extern Uint32 ColorYellow;

void DrawTexture(const CGraphic *g, GLuint *textures, int sx, int sy,
				 int ex, int ey, int x, int y, int flip);

extern void FreeGraphics();


// ARB_texture_compression
#if defined(USE_OPENGL) && !defined(__APPLE__)
extern PFNGLCOMPRESSEDTEXIMAGE3DARBPROC    glCompressedTexImage3DARB;
extern PFNGLCOMPRESSEDTEXIMAGE2DARBPROC    glCompressedTexImage2DARB;
extern PFNGLCOMPRESSEDTEXIMAGE1DARBPROC    glCompressedTexImage1DARB;
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glCompressedTexSubImage3DARB;
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glCompressedTexSubImage2DARB;
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glCompressedTexSubImage1DARB;
extern PFNGLGETCOMPRESSEDTEXIMAGEARBPROC   glGetCompressedTexImageARB;
#endif

//
//  Color Cycling stuff
//

extern void VideoPaletteListAdd(SDL_Surface *surface);
extern void VideoPaletteListRemove(SDL_Surface *surface);
extern void ClearAllColorCyclingRange();
extern void AddColorCyclingRange(unsigned int begin, unsigned int end);
extern void SetColorCycleAll(bool value);
extern void RestoreColorCyclingSurface();

/// Does ColorCycling..
extern void ColorCycle();

#endif
