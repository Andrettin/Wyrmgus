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
/**@name sdl.cpp - SDL video support. */
//
//      (c) Copyright 1999-2021 by Lutz Sammer, Jimmy Salmon, Nehal Mistry,
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

#ifdef DEBUG
#include <signal.h>
#endif

#ifndef USE_WIN32
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <SDL.h>
#include <SDL_syswm.h>

#ifdef USE_GLES_EGL
#include "EGL/egl.h"
#endif

#ifdef USE_GLES
#include "GLES/gl.h"
#endif

#ifdef USE_OPENGL
#include <SDL_opengl.h>
#endif

#ifdef USE_BEOS
#include <sys/socket.h>
#endif

#ifdef USE_WIN32
#include <shellapi.h>
#endif

#include "editor.h"
#include "engine_interface.h"
#include "game.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "map/minimap.h"
#include "network.h"
#include "parameters.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "translate.h"
#include "ui/cursor.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "util/queue_util.h"
#include "video/font.h"
#include "video/video.h"
#include "widgets.h"

#ifdef USE_GLES_EGL
static EGLDisplay eglDisplay;
static EGLSurface eglSurface;
#endif

SDL_Surface *TheScreen; /// Internal screen

#if defined(USE_OPENGL) || defined(USE_GLES)
GLint GLMaxTextureSize = 256;   /// Max texture size supported on the video card
GLint GLMaxTextureSizeOverride;     /// User-specified limit for ::GLMaxTextureSize
bool GLTextureCompressionSupported; /// Is OpenGL texture compression supported
bool UseGLTextureCompression;       /// Use OpenGL texture compression
#endif

static std::map<int, std::string> Key2Str;
static std::map<std::string, int> Str2Key;

double FrameTicks;     /// Frame length in ms

const EventCallback *Callbacks = nullptr;

static bool RegenerateScreen = false;
bool IsSDLWindowVisible = true;

// ARB_texture_compression
#ifdef USE_OPENGL
static PFNGLCOMPRESSEDTEXIMAGE3DARBPROC    glCompressedTexImage3DARB;
static PFNGLCOMPRESSEDTEXIMAGE2DARBPROC    glCompressedTexImage2DARB;
static PFNGLCOMPRESSEDTEXIMAGE1DARBPROC    glCompressedTexImage1DARB;
static PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glCompressedTexSubImage3DARB;
static PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glCompressedTexSubImage2DARB;
static PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glCompressedTexSubImage1DARB;
static PFNGLGETCOMPRESSEDTEXIMAGEARBPROC   glGetCompressedTexImageARB;
#endif

/*----------------------------------------------------------------------------
--  Sync
----------------------------------------------------------------------------*/

/**
**  Initialise video sync.
**  Calculate the length of video frame and any simulation skips.
**
**  @see VideoSyncSpeed @see SkipFrames @see FrameTicks
*/
void SetVideoSync()
{
	double ms;

	if (VideoSyncSpeed) {
		ms = (1000.0 * 1000.0 / CYCLES_PER_SECOND) / VideoSyncSpeed;
	} else {
		ms = (double)INT_MAX;
	}
	SkipFrames = ms / 400;
	while (SkipFrames && ms / SkipFrames < 200) {
		--SkipFrames;
	}
	ms /= SkipFrames + 1;

	FrameTicks = ms / 10;
	DebugPrint("frames %d - %5.2fms\n" _C_ SkipFrames _C_ ms / 10);
}

/*----------------------------------------------------------------------------
--  Video
----------------------------------------------------------------------------*/

#ifdef USE_OPENGL
/**
**  Check if an extension is supported
*/
static bool IsExtensionSupported(const char *extension)
{
	const GLubyte *extensions = nullptr;
	const GLubyte *start;
	GLubyte *ptr, *terminator;
	int len;

	// Extension names should not have spaces.
	ptr = (GLubyte *)strchr(extension, ' ');
	if (ptr || *extension == '\0') {
		return false;
	}

	extensions = glGetString(GL_EXTENSIONS);
	len = strlen(extension);
	start = extensions;
	while (true) {
		ptr = (GLubyte *)strstr((const char *)start, extension);
		if (!ptr) {
			break;
		}

		terminator = ptr + len;
		if (ptr == start || *(ptr - 1) == ' ') {
			if (*terminator == ' ' || *terminator == '\0') {
				return true;
			}
		}
		start = terminator;
	}
	return false;
}
#endif

#if defined(USE_OPENGL) || defined(USE_GLES)

/**
**  Initialize OpenGL extensions
*/
static void InitOpenGLExtensions()
{
	// ARB_texture_compression
	if (IsExtensionSupported("GL_ARB_texture_compression")) {
		glCompressedTexImage3DARB =
			(PFNGLCOMPRESSEDTEXIMAGE3DARBPROC)(uintptr_t)SDL_GL_GetProcAddress("glCompressedTexImage3DARB");
		glCompressedTexImage2DARB =
			(PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)(uintptr_t)SDL_GL_GetProcAddress("glCompressedTexImage2DARB");
		glCompressedTexImage1DARB =
			(PFNGLCOMPRESSEDTEXIMAGE1DARBPROC)(uintptr_t)SDL_GL_GetProcAddress("glCompressedTexImage1DARB");
		glCompressedTexSubImage3DARB =
			(PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC)(uintptr_t)SDL_GL_GetProcAddress("glCompressedTexSubImage3DARB");
		glCompressedTexSubImage2DARB =
			(PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC)(uintptr_t)SDL_GL_GetProcAddress("glCompressedTexSubImage2DARB");
		glCompressedTexSubImage1DARB =
			(PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC)(uintptr_t)SDL_GL_GetProcAddress("glCompressedTexSubImage1DARB");
		glGetCompressedTexImageARB =
			(PFNGLGETCOMPRESSEDTEXIMAGEARBPROC)(uintptr_t)SDL_GL_GetProcAddress("glGetCompressedTexImageARB");

		if (glCompressedTexImage3DARB && glCompressedTexImage2DARB &&
			glCompressedTexImage1DARB && glCompressedTexSubImage3DARB &&
			glCompressedTexSubImage2DARB && glCompressedTexSubImage1DARB &&
			glGetCompressedTexImageARB) {
			GLTextureCompressionSupported = true;
		} else {
			GLTextureCompressionSupported = false;
		}
	} else {
		GLTextureCompressionSupported = false;
	}
}

/**
**  Initialize OpenGL
*/
static void InitOpenGL()
{
	InitOpenGLExtensions();

	glViewport(0, 0, (GLsizei)Video.ViewportWidth, (GLsizei)Video.ViewportHeight);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

#ifdef USE_GLES
	glOrthof(0.0f, (GLfloat)Video.Width, (GLfloat)Video.Height, 0.0f, -1.0f, 1.0f);
#endif

	glOrtho(0, Video.Width, Video.Height, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(0.375, 0.375, 0.);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

#ifdef USE_GLES
	glClearDepthf(1.0f);
#endif

	glClearDepth(1.0f);

	glShadeModel(GL_FLAT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &GLMaxTextureSize);
	if (GLMaxTextureSize == 0) {
		// FIXME: try to use GL_PROXY_TEXTURE_2D to get a valid size
#if 0
		glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA, size, size, 0,
					 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glGetTexLevelParameterfv(GL_PROXY_TEXTURE_2D, 0,
								 GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
#endif
		fprintf(stderr, "GL_MAX_TEXTURE_SIZE is 0, using 256 by default\n");
		GLMaxTextureSize = 256;
	}
	if (GLMaxTextureSize > GLMaxTextureSizeOverride
		&& GLMaxTextureSizeOverride > 0) {
		GLMaxTextureSize = GLMaxTextureSizeOverride;
	}
}

void ReloadOpenGL()
{
	InitOpenGL();
	ReloadGraphics();
	ReloadFonts();
	UI.get_minimap()->Reload();
}

#endif

#if defined(DEBUG) && !defined(USE_WIN32)
static void CleanExit(int)
{
	// Clean SDL
	SDL_Quit();
	// Reestablish normal behaviour for next abort call
	signal(SIGABRT, SIG_DFL);
	// Generates a core dump
	abort();
}
#endif

/**
**  Initialize SDLKey to string map
*/
static void InitKey2Str()
{
	Str2Key[_("esc")] = SDLK_ESCAPE;

	if (!Key2Str.empty()) {
		return;
	}

	int i;
	std::array<char, 20> str{};

	Key2Str[SDLK_BACKSPACE] = "backspace";
	Key2Str[SDLK_TAB] = "tab";
	Key2Str[SDLK_CLEAR] = "clear";
	Key2Str[SDLK_RETURN] = "return";
	Key2Str[SDLK_PAUSE] = "pause";
	Key2Str[SDLK_ESCAPE] = "escape";
	Key2Str[SDLK_SPACE] = " ";
	Key2Str[SDLK_EXCLAIM] = "!";
	Key2Str[SDLK_QUOTEDBL] = "\"";
	Key2Str[SDLK_HASH] = "#";
	Key2Str[SDLK_DOLLAR] = "$";
	Key2Str[SDLK_AMPERSAND] = "&";
	Key2Str[SDLK_QUOTE] = "'";
	Key2Str[SDLK_LEFTPAREN] = "(";
	Key2Str[SDLK_RIGHTPAREN] = ")";
	Key2Str[SDLK_ASTERISK] = "*";
	Key2Str[SDLK_PLUS] = "+";
	Key2Str[SDLK_COMMA] = ",";
	Key2Str[SDLK_MINUS] = "-";
	Key2Str[SDLK_PERIOD] = ".";
	Key2Str[SDLK_SLASH] = "/";

	str[1] = '\0';
	for (i = SDLK_0; i <= SDLK_9; ++i) {
		str[0] = i;
		Key2Str[i] = str.data();
	}

	Key2Str[SDLK_COLON] = ":";
	Key2Str[SDLK_SEMICOLON] = ";";
	Key2Str[SDLK_LESS] = "<";
	Key2Str[SDLK_EQUALS] = "=";
	Key2Str[SDLK_GREATER] = ">";
	Key2Str[SDLK_QUESTION] = "?";
	Key2Str[SDLK_AT] = "@";
	Key2Str[SDLK_LEFTBRACKET] = "[";
	Key2Str[SDLK_BACKSLASH] = "\\";
	Key2Str[SDLK_RIGHTBRACKET] = "]";
	Key2Str[SDLK_BACKQUOTE] = "`";

	str[1] = '\0';
	for (i = SDLK_a; i <= SDLK_z; ++i) {
		str[0] = i;
		Key2Str[i] = str.data();
	}

	Key2Str[SDLK_DELETE] = "delete";

	for (i = SDLK_KP0; i <= SDLK_KP9; ++i) {
		snprintf(str.data(), sizeof(str), "kp_%d", i - SDLK_KP0);
		Key2Str[i] = str.data();
	}

	Key2Str[SDLK_KP_PERIOD] = "kp_period";
	Key2Str[SDLK_KP_DIVIDE] = "kp_divide";
	Key2Str[SDLK_KP_MULTIPLY] = "kp_multiply";
	Key2Str[SDLK_KP_MINUS] = "kp_minus";
	Key2Str[SDLK_KP_PLUS] = "kp_plus";
	Key2Str[SDLK_KP_ENTER] = "kp_enter";
	Key2Str[SDLK_KP_EQUALS] = "kp_equals";
	Key2Str[SDLK_UP] = "up";
	Key2Str[SDLK_DOWN] = "down";
	Key2Str[SDLK_RIGHT] = "right";
	Key2Str[SDLK_LEFT] = "left";
	Key2Str[SDLK_INSERT] = "insert";
	Key2Str[SDLK_HOME] = "home";
	Key2Str[SDLK_END] = "end";
	Key2Str[SDLK_PAGEUP] = "pageup";
	Key2Str[SDLK_PAGEDOWN] = "pagedown";

	for (i = SDLK_F1; i <= SDLK_F15; ++i) {
		snprintf(str.data(), sizeof(str), "f%d", i - SDLK_F1 + 1);
		Key2Str[i] = str.data();
		snprintf(str.data(), sizeof(str), "F%d", i - SDLK_F1 + 1);
		Str2Key[str.data()] = i;
	}

	Key2Str[SDLK_HELP] = "help";
	Key2Str[SDLK_PRINT] = "print";
	Key2Str[SDLK_SYSREQ] = "sysreq";
	Key2Str[SDLK_BREAK] = "break";
	Key2Str[SDLK_MENU] = "menu";
	Key2Str[SDLK_POWER] = "power";
	Key2Str[SDLK_EURO] = "euro";
	Key2Str[SDLK_UNDO] = "undo";
}

/**
**  Initialize the video part for SDL.
*/
void InitVideoSdl()
{
	Uint32 flags = 0;

	if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
//Wyrmgus start
//#ifndef USE_WIN32
//Wyrmgus end
		// Fix tablet input in full-screen mode
		#ifndef __MORPHOS__
		SDL_putenv(strdup("SDL_MOUSE_RELATIVE=0"));
		#endif
//Wyrmgus start
//#endif
//Wyrmgus end
		int res = SDL_Init(
#ifdef DEBUG
					  SDL_INIT_NOPARACHUTE |
#endif
					  SDL_INIT_AUDIO | SDL_INIT_VIDEO |
					  SDL_INIT_TIMER);
		if (res < 0) {
			throw std::runtime_error("Couldn't initialize SDL: " + std::string(SDL_GetError()));
		}

		// Clean up on exit
		atexit(SDL_Quit);

		// If debug is enabled, Stratagus disable SDL Parachute.
		// So we need gracefully handle segfaults and aborts.
#if defined(DEBUG) && !defined(USE_WIN32)
		signal(SIGSEGV, CleanExit);
		signal(SIGABRT, CleanExit);
#endif
		// Set WindowManager Title
		if (!FullGameName.empty()) {
			SDL_WM_SetCaption(FullGameName.c_str(), FullGameName.c_str());
		} else {
			const std::string name = QApplication::applicationName().toStdString();
			SDL_WM_SetCaption(name.c_str(), name.c_str());
		}

#if ! defined(USE_WIN32)

#ifndef __MORPHOS__	
		SDL_Surface *icon = nullptr;
		std::shared_ptr<CGraphic> g = nullptr;
		struct stat st;

		std::string FullGameNameL = FullGameName;
		for (size_t i = 0; i < FullGameNameL.size(); ++i) {
			FullGameNameL[i] = tolower(FullGameNameL[i]);
		}

		std::string ApplicationName = QApplication::applicationName().toStdString();
		std::string ApplicationNameL = ApplicationName;
		for (size_t i = 0; i < ApplicationNameL.size(); ++i) {
			ApplicationNameL[i] = tolower(ApplicationNameL[i]);
		}
#endif
		
#endif
#ifdef USE_WIN32
		HWND hwnd = nullptr;
		HICON hicon = nullptr;
		SDL_SysWMinfo info{};
		SDL_VERSION(&info.version);

		if (SDL_GetWMInfo(&info)) {
			hwnd = info.window;
		}

		if (hwnd) {
			hicon = ExtractIcon(GetModuleHandle(nullptr), BINARY_NAME ".exe", 0);
		}

		if (hicon) {
			SendMessage(hwnd, (UINT)WM_SETICON, ICON_SMALL, (LPARAM)hicon);
			SendMessage(hwnd, (UINT)WM_SETICON, ICON_BIG, (LPARAM)hicon);
		}
#endif
	}

	// Initialize the display

#if !defined(USE_OPENGL) && !defined(USE_GLES)
	#ifdef __MORPHOS__
	flags = SDL_SWSURFACE;
	#else
	flags = SDL_HWSURFACE | SDL_HWPALETTE;
	#endif
#endif

	// Sam said: better for windows.
	/* SDL_HWSURFACE|SDL_HWPALETTE | */
	if (Video.FullScreen) {
		flags |= SDL_FULLSCREEN;
	}

#ifdef USE_GLES_NATIVE
	flags |= SDL_OPENGLES;
#endif
#ifdef USE_OPENGL
	flags |= SDL_OPENGL | SDL_GL_DOUBLEBUFFER;
#endif

	if (!Video.Width || !Video.Height) {
		Video.Width = 640;
		Video.Height = 480;
	}

	if (!Video.Depth) {
		Video.Depth = 32;
	}

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!Video.ViewportWidth || !Video.ViewportHeight) {
		Video.ViewportWidth = Video.Width;
		Video.ViewportHeight = Video.Height;
	}
	TheScreen = SDL_SetVideoMode(Video.ViewportWidth, Video.ViewportHeight, Video.Depth, flags);
#else
	TheScreen = SDL_SetVideoMode(Video.Width, Video.Height, Video.Depth, flags);
#endif
	if (TheScreen && (TheScreen->format->BitsPerPixel != 16
					  && TheScreen->format->BitsPerPixel != 32)) {
		// Only support 16 and 32 bpp, default to 16
#if defined(USE_OPENGL) || defined(USE_GLES)
		TheScreen = SDL_SetVideoMode(Video.ViewportWidth, Video.ViewportHeight, 16, flags);
#else
		TheScreen = SDL_SetVideoMode(Video.Width, Video.Height, 16, flags);
#endif
	}
	if (TheScreen == nullptr) {
		throw std::runtime_error("Couldn't set " + std::to_string(Video.Width) + "x" + std::to_string(Video.Height) + "x" + std::to_string(Video.Depth) + " video mode: " + std::string(SDL_GetError()));
	}

	Video.FullScreen = (TheScreen->flags & SDL_FULLSCREEN) ? 1 : 0;
	Video.Depth = TheScreen->format->BitsPerPixel;

//Wyrmgus start
//#if defined(USE_TOUCHSCREEN) && defined(USE_WIN32)
//Wyrmgus end
	// Must not allow SDL to switch to relative mouse coordinates
	// with touchscreen when going fullscreen. So we don't hide the
	// cursor, but instead set a transparent 1px cursor
	Uint8 emptyCursor[] = {'\0'};
	Video.blankCursor = SDL_CreateCursor(emptyCursor, emptyCursor, 1, 1, 0, 0);
	SDL_SetCursor(Video.blankCursor);
//Wyrmgus start
//#else
//Wyrmgus end
	// Turn cursor off, we use our own.
	//Wyrmgus start
//	SDL_ShowCursor(SDL_DISABLE);
	//Wyrmgus end
//Wyrmgus start
//#endif
//Wyrmgus end

	// Make default character translation easier
	SDL_EnableUNICODE(1);

#ifdef USE_GLES_EGL
	// Get the SDL window handle
	SDL_SysWMinfo sysInfo; //Will hold our Window information
	SDL_VERSION(&sysInfo.version); //Set SDL version
	if (SDL_GetWMInfo(&sysInfo) <= 0) {
		throw std::runtime_error("Unable to get window handle.");
	}

	eglDisplay = eglGetDisplay((EGLNativeDisplayType)sysInfo.info.x11.display);
	if (!eglDisplay) {
		throw std::runtime_error("Couldn't open EGL Display.");
	}

	if (!eglInitialize(eglDisplay, nullptr, nullptr)) {
		throw std::runtime_error("Couldn't initialize EGL Display.");
	}

	// Find a matching config
	EGLint configAttribs[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE};
	EGLint numConfigsOut = 0;
	EGLConfig eglConfig;
	if (eglChooseConfig(eglDisplay, configAttribs, &eglConfig, 1, &numConfigsOut) != EGL_TRUE || numConfigsOut == 0) {
		throw std::runtime_error("Unable to find appropriate EGL config.");
	}

	eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (EGLNativeWindowType)sysInfo.info.x11.window, 0);
	if (eglSurface == EGL_NO_SURFACE) {
		throw std::runtime_error("Unable to create EGL surface.");
	}

	// Bind GLES and create the context
	eglBindAPI(EGL_OPENGL_ES_API);
	EGLint contextParams[] = {EGL_CONTEXT_CLIENT_VERSION, 1, EGL_NONE};
	EGLContext eglContext = eglCreateContext(eglDisplay, eglConfig, nullptr, nullptr);
	if (eglContext == EGL_NO_CONTEXT) {
		throw std::runtime_error("Unable to create GLES context.");
	}

	if (eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext) == EGL_FALSE) {
		throw std::runtime_error("Unable to make GLES context current.");
	}
#endif
	InitOpenGL();

	InitKey2Str();

	ColorBlack = CVideo::MapRGB(0, 0, 0);
	ColorDarkGreen = CVideo::MapRGB(48, 100, 4);
	ColorLightBlue = CVideo::MapRGB(52, 113, 166);
	ColorBlue = CVideo::MapRGB(0, 0, 252);
	ColorOrange = CVideo::MapRGB(248, 140, 20);
	ColorWhite = CVideo::MapRGB(252, 248, 240);
	ColorLightGray = CVideo::MapRGB(192, 192, 192);
	ColorGray = CVideo::MapRGB(128, 128, 128);
	ColorDarkGray = CVideo::MapRGB(64, 64, 64);
	ColorRed = CVideo::MapRGB(252, 0, 0);
	ColorGreen = CVideo::MapRGB(0, 252, 0);
	ColorYellow = CVideo::MapRGB(252, 252, 0);

	UI.MouseWarpPos.x = UI.MouseWarpPos.y = -1;
}

/**
**  Check if a resolution is valid
**
**  @param w  Width
**  @param h  Height
*/
int VideoValidResolution(int w, int h)
{
	return SDL_VideoModeOK(w, h, TheScreen->format->BitsPerPixel, TheScreen->flags);
}

static void do_mouse_warp()
{
	int xw = UI.MouseWarpPos.x;
	int yw = UI.MouseWarpPos.y;
	UI.MouseWarpPos.x = -1;
	UI.MouseWarpPos.y = -1;
	SDL_WarpMouse(xw, yw);
}

/**
**  Handle interactive input event.
**
**  @param callbacks  Callback structure for events.
**  @param event      SDL event structure pointer.
*/
static void SdlDoEvent(const EventCallback &callbacks, SDL_Event &event)
{
#if (defined(USE_OPENGL) || defined(USE_GLES))
	// Scale mouse-coordinates to viewport
	if (ZoomNoResize && (event.type & (SDL_MOUSEBUTTONUP | SDL_MOUSEBUTTONDOWN | SDL_MOUSEMOTION))) {
		event.button.x = (Uint16)floorf(event.button.x * float(Video.Width) / Video.ViewportWidth);
		event.button.y = (Uint16)floorf(event.button.y * float(Video.Height) / Video.ViewportHeight);
		//Wyrmgus start
		event.motion.x = (Uint16)floorf(event.motion.x * float(Video.Width) / Video.ViewportWidth);
		event.motion.y = (Uint16)floorf(event.motion.y * float(Video.Height) / Video.ViewportHeight);
		//Wyrmgus end
	}
#endif
	switch (event.type) {
		case SDL_MOUSEBUTTONDOWN:
			InputMouseButtonPress(callbacks, SDL_GetTicks(), event.button.button);
			if ((UI.MouseWarpPos.x != -1 || UI.MouseWarpPos.y != -1)
				&& (event.button.x != UI.MouseWarpPos.x || event.button.y != UI.MouseWarpPos.y)) {
				do_mouse_warp();
			}
			break;

		case SDL_MOUSEBUTTONUP:
			InputMouseButtonRelease(callbacks, SDL_GetTicks(), event.button.button);
			break;

		// FIXME: check if this is only useful for the cursor
		// FIXME: if this is the case we don't need this.
		case SDL_MOUSEMOTION:
			InputMouseMove(callbacks, SDL_GetTicks(), event.motion.x, event.motion.y);
			// FIXME: Same bug fix from X11
			if ((UI.MouseWarpPos.x != -1 || UI.MouseWarpPos.y != -1)
				&& (event.motion.x != UI.MouseWarpPos.x || event.motion.y != UI.MouseWarpPos.y)) {
				do_mouse_warp();
			}
			break;

		case SDL_ACTIVEEVENT:
			if (event.active.state & SDL_APPMOUSEFOCUS) {
				static bool InMainWindow = true;

				if (InMainWindow && !event.active.gain) {
					InputMouseExit(callbacks, SDL_GetTicks());
				}
				InMainWindow = (event.active.gain != 0);
			}
			if (!IsNetworkGame() && Preference.PauseOnLeave && (event.active.state & SDL_APPACTIVE || SDL_GetAppState() & SDL_APPACTIVE)) {
				static bool DoTogglePause = false;

				if (IsSDLWindowVisible && !event.active.gain) {
					IsSDLWindowVisible = false;
					if (!GamePaused) {
						DoTogglePause = true;
						UiTogglePause();
					}
				} else if (!IsSDLWindowVisible && event.active.gain) {
					IsSDLWindowVisible = true;
					if (GamePaused && DoTogglePause) {
						DoTogglePause = false;
						UiTogglePause();
					}
				}
			}
			break;

		case SDL_KEYDOWN:
			InputKeyButtonPress(callbacks, SDL_GetTicks(),
								event.key.keysym.sym, event.key.keysym.unicode);
			break;

		case SDL_KEYUP:
			InputKeyButtonRelease(callbacks, SDL_GetTicks(),
								  event.key.keysym.sym, event.key.keysym.unicode);
			break;

		case SDL_QUIT:
			Exit(0);
			break;
	}

	if (&callbacks == GetCallbacks()) {
		handleInput(&event);
	}
}

void ValidateOpenGLScreen()
{
	if (RegenerateScreen) {
		Video.ResizeScreen(Video.Width, Video.Height);
		RegenerateScreen = false;
	}
}

/**
**  Set the current callbacks
*/
void SetCallbacks(const EventCallback *callbacks)
{
	Callbacks = callbacks;
}

/**
**  Get the current callbacks
*/
const EventCallback *GetCallbacks()
{
	return Callbacks;
}

int PollEvent()
{
	SDL_Event event;
	if (SDL_PollEvent(&event)) { // Handle SDL event
		SdlDoEvent(*GetCallbacks(), event);
		return 1;
	}

	return 0;
}

void PollEvents()
{
	if (Callbacks == nullptr) return;

	while (PollEvent()) { }
}

static SDLKey qt_key_to_sdl_key(const Qt::Key qt_key)
{
	switch (qt_key) {
		case Qt::Key_Escape:
			return SDLK_ESCAPE;
		case Qt::Key_Tab:
			return SDLK_TAB;
		case Qt::Key_Backspace:
			return SDLK_BACKSPACE;
		case Qt::Key_Return:
		case Qt::Key_Enter:
			return SDLK_RETURN;
		case Qt::Key_Insert:
			return SDLK_INSERT;
		case Qt::Key_Delete:
			return SDLK_DELETE;
		case Qt::Key_Pause:
			return SDLK_PAUSE;
		case Qt::Key_Print:
			return SDLK_PRINT;
		case Qt::Key_SysReq:
			return SDLK_SYSREQ;
		case Qt::Key_Clear:
			return SDLK_CLEAR;
		case Qt::Key_Home:
			return SDLK_HOME;
		case Qt::Key_End:
			return SDLK_END;
		case Qt::Key_Left:
			return SDLK_LEFT;
		case Qt::Key_Up:
			return SDLK_UP;
		case Qt::Key_Right:
			return SDLK_RIGHT;
		case Qt::Key_Down:
			return SDLK_DOWN;
		case Qt::Key_PageUp:
			return SDLK_PAGEUP;
		case Qt::Key_PageDown:
			return SDLK_PAGEDOWN;
		case Qt::Key_Shift:
			return SDLK_LSHIFT;
		case Qt::Key_Control:
			return SDLK_LCTRL;
		case Qt::Key_Meta:
			return SDLK_LMETA;
		case Qt::Key_Alt:
			return SDLK_LALT;
		case Qt::Key_AltGr:
			return SDLK_MODE;
		case Qt::Key_CapsLock:
			return SDLK_CAPSLOCK;
		case Qt::Key_NumLock:
			return SDLK_NUMLOCK;
		case Qt::Key_ScrollLock:
			return SDLK_SCROLLOCK;
		case Qt::Key_F1:
			return SDLK_F1;
		case Qt::Key_F2:
			return SDLK_F2;
		case Qt::Key_F3:
			return SDLK_F3;
		case Qt::Key_F4:
			return SDLK_F4;
		case Qt::Key_F5:
			return SDLK_F5;
		case Qt::Key_F6:
			return SDLK_F6;
		case Qt::Key_F7:
			return SDLK_F7;
		case Qt::Key_F8:
			return SDLK_F8;
		case Qt::Key_F9:
			return SDLK_F9;
		case Qt::Key_F10:
			return SDLK_F10;
		case Qt::Key_F11:
			return SDLK_F11;
		case Qt::Key_F12:
			return SDLK_F12;
		case Qt::Key_F13:
			return SDLK_F13;
		case Qt::Key_F14:
			return SDLK_F14;
		case Qt::Key_F15:
			return SDLK_F15;
		case Qt::Key_Super_L:
			return SDLK_LSUPER;
		case Qt::Key_Super_R:
			return SDLK_RSUPER;
		case Qt::Key_Menu:
			return SDLK_MENU;
		case Qt::Key_Help:
			return SDLK_HELP;
		case Qt::Key_Space:
			return SDLK_SPACE;
		case Qt::Key_Exclam:
			return SDLK_EXCLAIM;
		case Qt::Key_QuoteDbl:
			return SDLK_QUOTEDBL;
		case Qt::Key_NumberSign:
			return SDLK_HASH;
		case Qt::Key_Dollar:
			return SDLK_DOLLAR;
		case Qt::Key_Ampersand:
			return SDLK_AMPERSAND;
		case Qt::Key_ParenLeft:
			return SDLK_LEFTPAREN;
		case Qt::Key_ParenRight:
			return SDLK_RIGHTPAREN;
		case Qt::Key_Asterisk:
			return SDLK_ASTERISK;
		case Qt::Key_Plus:
			return SDLK_PLUS;
		case Qt::Key_Comma:
			return SDLK_COMMA;
		case Qt::Key_Minus:
			return SDLK_MINUS;
		case Qt::Key_Period:
			return SDLK_PERIOD;
		case Qt::Key_Slash:
			return SDLK_SLASH;
		case Qt::Key_0:
			return SDLK_0;
		case Qt::Key_1:
			return SDLK_1;
		case Qt::Key_2:
			return SDLK_2;
		case Qt::Key_3:
			return SDLK_3;
		case Qt::Key_4:
			return SDLK_4;
		case Qt::Key_5:
			return SDLK_5;
		case Qt::Key_6:
			return SDLK_6;
		case Qt::Key_7:
			return SDLK_7;
		case Qt::Key_8:
			return SDLK_8;
		case Qt::Key_9:
			return SDLK_9;
		case Qt::Key_Colon:
			return SDLK_COLON;
		case Qt::Key_Semicolon:
			return SDLK_SEMICOLON;
		case Qt::Key_Less:
			return SDLK_LESS;
		case Qt::Key_Equal:
			return SDLK_EQUALS;
		case Qt::Key_Greater:
			return SDLK_GREATER;
		case Qt::Key_Question:
			return SDLK_QUESTION;
		case Qt::Key_At:
			return SDLK_AT;
		case Qt::Key_A:
			return SDLK_a;
		case Qt::Key_B:
			return SDLK_b;
		case Qt::Key_C:
			return SDLK_c;
		case Qt::Key_D:
			return SDLK_d;
		case Qt::Key_E:
			return SDLK_e;
		case Qt::Key_F:
			return SDLK_f;
		case Qt::Key_G:
			return SDLK_g;
		case Qt::Key_H:
			return SDLK_h;
		case Qt::Key_I:
			return SDLK_i;
		case Qt::Key_J:
			return SDLK_j;
		case Qt::Key_K:
			return SDLK_k;
		case Qt::Key_L:
			return SDLK_l;
		case Qt::Key_M:
			return SDLK_m;
		case Qt::Key_N:
			return SDLK_n;
		case Qt::Key_O:
			return SDLK_o;
		case Qt::Key_P:
			return SDLK_p;
		case Qt::Key_Q:
			return SDLK_q;
		case Qt::Key_R:
			return SDLK_r;
		case Qt::Key_S:
			return SDLK_s;
		case Qt::Key_T:
			return SDLK_t;
		case Qt::Key_U:
			return SDLK_u;
		case Qt::Key_V:
			return SDLK_v;
		case Qt::Key_W:
			return SDLK_w;
		case Qt::Key_X:
			return SDLK_x;
		case Qt::Key_Y:
			return SDLK_y;
		case Qt::Key_Z:
			return SDLK_z;
		case Qt::Key_BracketLeft:
			return SDLK_LEFTBRACKET;
		case Qt::Key_Backslash:
			return SDLK_BACKSLASH;
		case Qt::Key_BracketRight:
			return SDLK_RIGHTBRACKET;
		case Qt::Key_AsciiCircum:
			return SDLK_CARET;
		case Qt::Key_Underscore:
			return SDLK_UNDERSCORE;
		case Qt::Key_QuoteLeft:
			return SDLK_BACKQUOTE;
		case Qt::Key_Undo:
			return SDLK_UNDO;
		default:
			return SDLK_UNKNOWN;
	}
}

static SDLMod qt_key_modifier_to_sdl_key_modifier(const Qt::KeyboardModifiers qt_key_modifier)
{
	int modifier = KMOD_NONE;

	if (qt_key_modifier & Qt::ShiftModifier) {
		modifier |= KMOD_LSHIFT;
	}

	if (qt_key_modifier & Qt::ControlModifier) {
		modifier |= KMOD_LCTRL;
	}

	if (qt_key_modifier & Qt::AltModifier) {
		modifier |= KMOD_LALT;
	}

	if (qt_key_modifier & Qt::MetaModifier) {
		modifier |= KMOD_LMETA;
	}

	if (qt_key_modifier & Qt::KeypadModifier) {
		modifier |= KMOD_NUM;
	}

	return static_cast<SDLMod>(modifier);
}

static SDL_Event qevent_to_sdl_event(std::unique_ptr<QInputEvent> &&qevent)
{
	SDL_Event sdl_event{};

	switch (qevent->type()) {
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease: {
			if (qevent->type() == QEvent::MouseButtonPress) {
				sdl_event.button.type = SDL_MOUSEBUTTONDOWN;
				sdl_event.button.state = SDL_PRESSED;
			} else if (qevent->type() == QEvent::MouseButtonRelease) {
				sdl_event.button.type = SDL_MOUSEBUTTONUP;
				sdl_event.button.state = SDL_RELEASED;
			}

			const QMouseEvent *mouse_event = static_cast<QMouseEvent *>(qevent.get());

			switch (mouse_event->button()) {
				case Qt::LeftButton:
					sdl_event.button.button = SDL_BUTTON_LEFT;
					break;
				case Qt::MiddleButton:
					sdl_event.button.button = SDL_BUTTON_MIDDLE;
					break;
				case Qt::RightButton:
					sdl_event.button.button = SDL_BUTTON_RIGHT;
					break;
				default:
					break;
			}

			sdl_event.button.x = mouse_event->pos().x();
			sdl_event.button.y = mouse_event->pos().y();
			break;
		}
		case QEvent::MouseMove: {
			const QMouseEvent *mouse_event = static_cast<QMouseEvent *>(qevent.get());

			sdl_event.motion.type = SDL_MOUSEMOTION;
			sdl_event.motion.x = mouse_event->pos().x();
			sdl_event.motion.y = mouse_event->pos().y();
			break;
		}
		case QEvent::HoverMove: {
			const QHoverEvent *hover_event = static_cast<QHoverEvent *>(qevent.get());

			sdl_event.motion.type = SDL_MOUSEMOTION;
			sdl_event.motion.x = hover_event->pos().x();
			sdl_event.motion.y = hover_event->pos().y();
			break;
		}
		case QEvent::KeyPress:
		case QEvent::KeyRelease: {
			const QKeyEvent *key_event = static_cast<QKeyEvent *>(qevent.get());

			if (qevent->type() == QEvent::KeyPress) {
				sdl_event.key.type = SDL_KEYDOWN;
				sdl_event.key.state = SDL_PRESSED;
			} else if (qevent->type() == QEvent::KeyRelease) {
				sdl_event.key.type = SDL_KEYUP;
				sdl_event.key.state = SDL_RELEASED;
			}

			sdl_event.key.keysym.sym = qt_key_to_sdl_key(static_cast<Qt::Key>(key_event->key()));
			sdl_event.key.keysym.unicode = qt_key_to_sdl_key(static_cast<Qt::Key>(key_event->key()));
			sdl_event.key.keysym.mod = qt_key_modifier_to_sdl_key_modifier(key_event->modifiers());
			break;
		}
		default:
			break;
	}

	return sdl_event;
}

/**
**  Wait for interactive input event for one frame.
**
**  Handles system events, joystick, keyboard, mouse.
**  Handles the network messages.
**  Handles the sound queue.
**
**  All events available are fetched. Sound and network only if available.
**  Returns if the time for one frame is over.
*/
void WaitEventsOneFrame()
{
	++FrameCounter;

	Uint32 ticks = SDL_GetTicks();
	if (ticks > NextFrameTicks) { // We are too slow :(
		++SlowFrameCounter;
	}

	InputMouseTimeout(*GetCallbacks(), ticks);
	InputKeyTimeout(*GetCallbacks(), ticks);
	CursorAnimate(ticks);

	std::queue<std::unique_ptr<QInputEvent>> input_events = engine_interface::get()->take_stored_input_events();
	while (!input_events.empty()) {
		std::unique_ptr<QInputEvent> input_event = queue::take(input_events);
		SDL_Event sdl_event = qevent_to_sdl_event(std::move(input_event));
		SdlDoEvent(*GetCallbacks(), sdl_event);
	}

	int interrupts = 0;

	for (;;) {
		// Time of frame over? This makes the CPU happy. :(
		ticks = SDL_GetTicks();
		if (!interrupts && ticks < NextFrameTicks) {
			SDL_Delay(NextFrameTicks - ticks);
			ticks = SDL_GetTicks();
		}
		while (ticks >= (unsigned long)(NextFrameTicks)) {
			++interrupts;
			NextFrameTicks += FrameTicks;
		}

		int i = PollEvent();

		// Network
		int s = 0;
		if (IsNetworkGame()) {
			s = NetworkFildes.HasDataToRead(0);
			if (s > 0) {
				GetCallbacks()->NetworkEvent();
			}
		}
		// No more input and time for frame over: return
		if (!i && s <= 0 && interrupts) {
			break;
		}
	}
	handleInput(nullptr);

	if (!SkipGameCycle--) {
		SkipGameCycle = SkipFrames;
	}
}

/**
**  Realize video memory.
*/
void RealizeVideoMemory()
{
#ifdef USE_GLES_EGL
	eglSwapBuffers(eglDisplay, eglSurface);
#endif
#if defined(USE_OPENGL) || defined(USE_GLES_NATIVE)
	SDL_GL_SwapBuffers();
#endif
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/**
**  Convert a SDLKey to a string
*/
const char *SdlKey2Str(int key)
{
	return Key2Str[key].c_str();
}

/**
**  Convert a string to SDLKey
*/
int Str2SdlKey(const char *str)
{
	InitKey2Str();

	std::map<int, std::string>::iterator i;
	for (i = Key2Str.begin(); i != Key2Str.end(); ++i) {
		if (!strcasecmp(str, (*i).second.c_str())) {
			return (*i).first;
		}
	}
	std::map<std::string, int>::iterator i2;
	for (i2 = Str2Key.begin(); i2 != Str2Key.end(); ++i2) {
		if (!strcasecmp(str, (*i2).first.c_str())) {
			return (*i2).second;
		}
	}
	return 0;
}

/**
**  Check if the mouse is grabbed
*/
bool SdlGetGrabMouse()
{
	return SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_ON;
}

/**
**  Toggle grab mouse.
**
**  @param mode  Wanted mode, 1 grab, -1 not grab, 0 toggle.
*/
void ToggleGrabMouse(int mode)
{
	bool grabbed = SdlGetGrabMouse();

	if (mode <= 0 && grabbed) {
		SDL_WM_GrabInput(SDL_GRAB_OFF);
	} else if (mode >= 0 && !grabbed) {
		SDL_WM_GrabInput(SDL_GRAB_ON);
	}
}

/**
**  Toggle full screen mode.
*/
void ToggleFullScreen()
{
#if defined(USE_WIN32) || defined(__APPLE__)
	long framesize;
	SDL_Rect clip;

	if (!TheScreen) { // don't bother if there's no surface.
		return;
	}

	Uint32 flags = TheScreen->flags;
	int w = TheScreen->w;
	int h = TheScreen->h;
	int bpp = TheScreen->format->BitsPerPixel;

	if (!SDL_VideoModeOK(w, h, bpp,	flags ^ SDL_FULLSCREEN)) {
		return;
	}

	SDL_GetClipRect(TheScreen, &clip);

	// save the contents of the screen.
	framesize = w * h * TheScreen->format->BytesPerPixel;

	TheScreen = SDL_SetVideoMode(w, h, bpp, flags ^ SDL_FULLSCREEN);
	if (!TheScreen) {
		TheScreen = SDL_SetVideoMode(w, h, bpp, flags);
		if (!TheScreen) { // completely screwed.
			throw std::runtime_error("Toggle to fullscreen, crashed all.");
		}
	}

#ifndef USE_TOUCHSCREEN
	// Cannot hide cursor on Windows with touchscreen, as it switches
	// to relative mouse coordinates in fullscreen. See above initial
	// call to ShowCursor
	//
	// Windows shows the SDL cursor when starting in fullscreen mode
	// then switching to window mode.  This hides the cursor again.
	//Wyrmgus start
//	SDL_ShowCursor(SDL_ENABLE);
//	SDL_ShowCursor(SDL_DISABLE);
	//Wyrmgus end
#endif

	ReloadOpenGL();

	SDL_SetClipRect(TheScreen, &clip);

#else // !USE_WIN32
	SDL_WM_ToggleFullScreen(TheScreen);
#endif

	Video.FullScreen = (TheScreen->flags & SDL_FULLSCREEN) ? 1 : 0;
}
