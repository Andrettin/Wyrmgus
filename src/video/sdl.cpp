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
//      (c) Copyright 1999-2022 by Lutz Sammer, Jimmy Salmon, Nehal Mistry,
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

#ifdef USE_BEOS
#include <sys/socket.h>
#endif

#include "editor.h"
#include "engine_interface.h"
#include "game/game.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "map/minimap.h"
#include "network/network.h"
#include "network/network_manager.h"
#include "parameters.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "translator.h"
#include "ui/cursor.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "util/assert_util.h"
#include "util/queue_util.h"
#include "video/font.h"
#include "video/video.h"
#include "widgets.h"

#pragma warning(push, 0)
#include <qcoro/core/qcorotimer.h>

#include <QWindow>
#pragma warning(pop)

static std::map<int, std::string> Key2Str;
static std::map<std::string, int> Str2Key;

double FrameTicks;     /// Frame length in ms

const EventCallback *Callbacks = nullptr;

static bool RegenerateScreen = false;
bool IsSDLWindowVisible = true;

#ifdef KeyPress
#undef KeyPress
#endif

#ifdef KeyRelease
#undef KeyRelease
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
**  Initialize SDL_Keycode to string map
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

	for (i = SDLK_KP_0; i <= SDLK_KP_9; ++i) {
		snprintf(str.data(), sizeof(str), "kp_%d", i - SDLK_KP_0);
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

	for (i = SDLK_F1; i <= SDLK_F12; ++i) {
		snprintf(str.data(), sizeof(str), "f%d", i - SDLK_F1 + 1);
		Key2Str[i] = str.data();
		snprintf(str.data(), sizeof(str), "F%d", i - SDLK_F1 + 1);
		Str2Key[str.data()] = i;
	}

	for (i = SDLK_F13; i <= SDLK_F24; ++i) {
		snprintf(str.data(), sizeof(str), "f%d", i - SDLK_F13 + 13);
		Key2Str[i] = str.data();
		snprintf(str.data(), sizeof(str), "F%d", i - SDLK_F13 + 13);
		Str2Key[str.data()] = i;
	}

	Key2Str[SDLK_HELP] = "help";
	Key2Str[SDLK_PRINTSCREEN] = "print";
	Key2Str[SDLK_SYSREQ] = "sysreq";
	Key2Str[SDLK_PAUSE] = "break";
	Key2Str[SDLK_MENU] = "menu";
	Key2Str[SDLK_POWER] = "power";
	Key2Str[SDLK_UNDO] = "undo";
}

/**
**  Initialize the video part for SDL.
*/
void InitVideoSdl()
{
	if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
		// Fix tablet input in full-screen mode
		#ifndef __MORPHOS__
		SDL_setenv(strdup("SDL_MOUSE_RELATIVE"), "0", 1);
		#endif

		int res = SDL_Init(
#ifdef DEBUG
					  SDL_INIT_NOPARACHUTE |
#endif
					  SDL_INIT_AUDIO |
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
	}

	if (!Video.Width || !Video.Height) {
		Video.Width = 640;
		Video.Height = 480;
	}

	if (!Video.Depth) {
		Video.Depth = 32;
	}

	if (!Video.ViewportWidth || !Video.ViewportHeight) {
		Video.ViewportWidth = Video.Width;
		Video.ViewportHeight = Video.Height;
	}

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

static void do_mouse_warp()
{
	int xw = UI.MouseWarpPos.x;
	int yw = UI.MouseWarpPos.y;
	UI.MouseWarpPos.x = -1;
	UI.MouseWarpPos.y = -1;

	QMetaObject::invokeMethod(QApplication::instance(), [xw, yw] {
		const QWindowList windows = QApplication::topLevelWindows();

		if (windows.empty()) {
			return;
		}

		const QWindow *window = windows.at(0);

		const QPoint global_pos = window->mapToGlobal(QPoint(xw, yw));

		QCursor::setPos(global_pos);
	}, Qt::QueuedConnection);
}

/**
**  Handle interactive input event.
**
**  @param callbacks  Callback structure for events.
**  @param event      SDL event structure pointer.
*/
static void SdlDoEvent(const EventCallback &callbacks, SDL_Event &event, const Qt::KeyboardModifiers key_modifiers)
{
	switch (event.type) {
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEMOTION:
			//scale mouse-coordinates to viewport
			if (ZoomNoResize) {
				event.button.x = static_cast<Uint16>(floorf(event.button.x * float(Video.Width) / Video.ViewportWidth));
				event.button.y = static_cast<Uint16>(floorf(event.button.y * float(Video.Height) / Video.ViewportHeight));

				event.motion.x = static_cast<Uint16>(floorf(event.motion.x * float(Video.Width) / Video.ViewportWidth));
				event.motion.y = static_cast<Uint16>(floorf(event.motion.y * float(Video.Height) / Video.ViewportHeight));
			}
			break;
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			if (event.key.keysym.sym == SDLK_UNKNOWN) {
				return;
			}
			break;
		default:
			break;
	}

	switch (event.type) {
		case SDL_MOUSEBUTTONDOWN:
			InputMouseButtonPress(callbacks, SDL_GetTicks(), event.button.button, key_modifiers);
			if ((UI.MouseWarpPos.x != -1 || UI.MouseWarpPos.y != -1)
				&& (event.button.x != UI.MouseWarpPos.x || event.button.y != UI.MouseWarpPos.y)) {
				do_mouse_warp();
			}
			break;

		case SDL_MOUSEBUTTONUP:
			InputMouseButtonRelease(callbacks, SDL_GetTicks(), event.button.button, key_modifiers);
			break;

		// FIXME: check if this is only useful for the cursor
		// FIXME: if this is the case we don't need this.
		case SDL_MOUSEMOTION:
			InputMouseMove(callbacks, SDL_GetTicks(), event.motion.x, event.motion.y, key_modifiers);
			// FIXME: Same bug fix from X11
			if ((UI.MouseWarpPos.x != -1 || UI.MouseWarpPos.y != -1)
				&& (event.motion.x != UI.MouseWarpPos.x || event.motion.y != UI.MouseWarpPos.y)) {
				do_mouse_warp();
			}
			break;

		case SDL_WINDOWEVENT:
			switch (event.window.event) {
				case SDL_WINDOWEVENT_ENTER:
				case SDL_WINDOWEVENT_LEAVE: {
					static bool InMainWindow = true;

					if (InMainWindow && event.window.event == SDL_WINDOWEVENT_LEAVE) {
						InputMouseExit(callbacks, SDL_GetTicks());
					}
					InMainWindow = (event.window.event == SDL_WINDOWEVENT_ENTER);
					break;
				}
				case SDL_WINDOWEVENT_FOCUS_GAINED:
				case SDL_WINDOWEVENT_FOCUS_LOST: {
					if (!IsNetworkGame() && Preference.PauseOnLeave) {
						static bool DoTogglePause = false;

						if (IsSDLWindowVisible && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
							IsSDLWindowVisible = false;
							if (!game::get()->is_paused()) {
								DoTogglePause = true;
								game::get()->toggle_paused();
							}
						} else if (!IsSDLWindowVisible && event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
							IsSDLWindowVisible = true;
							if (game::get()->is_paused() && DoTogglePause) {
								DoTogglePause = false;
								game::get()->toggle_paused();
							}
						}
					}
					break;
				}
			}
			break;

		case SDL_KEYDOWN:
			InputKeyButtonPress(callbacks, SDL_GetTicks(), event.key.keysym.sym, event.key.keysym.sym < 128 ? event.key.keysym.sym : 0, key_modifiers);
			break;

		case SDL_KEYUP:
			InputKeyButtonRelease(callbacks, SDL_GetTicks(), event.key.keysym.sym, event.key.keysym.sym < 128 ? event.key.keysym.sym : 0, key_modifiers);
			break;

		case SDL_QUIT:
			assert_throw(false);
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

[[nodiscard]]
static int PollEvent()
{
	SDL_Event event;
	if (SDL_PollEvent(&event)) { // Handle SDL event
		SdlDoEvent(*GetCallbacks(), event, Qt::KeyboardModifiers());
		return 1;
	}

	return 0;
}

static SDL_Keycode qt_key_to_sdl_key(const Qt::Key qt_key)
{
	switch (qt_key) {
		case Qt::Key_Escape:
			return SDLK_ESCAPE;
		case Qt::Key_Tab:
		case Qt::Key_Backtab:
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
			return SDLK_PRINTSCREEN;
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
			return SDLK_LGUI;
		case Qt::Key_Alt:
			return SDLK_LALT;
		case Qt::Key_AltGr:
			return SDLK_MODE;
		case Qt::Key_CapsLock:
			return SDLK_CAPSLOCK;
		case Qt::Key_NumLock:
			return SDLK_NUMLOCKCLEAR;
		case Qt::Key_ScrollLock:
			return SDLK_SCROLLLOCK;
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
			return SDLK_LGUI;
		case Qt::Key_Super_R:
			return SDLK_RGUI;
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

static SDL_Keymod qt_key_modifier_to_sdl_key_modifier(const Qt::KeyboardModifiers qt_key_modifiers)
{
	int modifiers = KMOD_NONE;

	if (qt_key_modifiers & Qt::ShiftModifier) {
		modifiers |= KMOD_LSHIFT;
	}

	if (qt_key_modifiers & Qt::ControlModifier) {
		modifiers |= KMOD_LCTRL;
	}

	if (qt_key_modifiers & Qt::AltModifier) {
		modifiers |= KMOD_LALT;
	}

	if (qt_key_modifiers & Qt::MetaModifier) {
		modifiers |= KMOD_LGUI;
	}

	if (qt_key_modifiers & Qt::KeypadModifier) {
		modifiers |= KMOD_NUM;
	}

	return static_cast<SDL_Keymod>(modifiers);
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
		case QEvent::HoverEnter:
		case QEvent::HoverLeave: {
			sdl_event.window.type = SDL_WINDOWEVENT;
			sdl_event.window.event = qevent->type() == QEvent::HoverEnter ? SDL_WINDOWEVENT_FOCUS_GAINED : SDL_WINDOWEVENT_FOCUS_LOST;
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
QCoro::Task<void> WaitEventsOneFrame()
{
	++FrameCounter;

	Uint32 ticks = SDL_GetTicks();
	if (ticks > NextFrameTicks) { // We are too slow :(
		++SlowFrameCounter;
	}

	InputMouseTimeout(*GetCallbacks(), ticks, stored_key_modifiers);
	InputKeyTimeout(*GetCallbacks(), ticks, stored_key_modifiers);
	CursorAnimate(ticks);

	std::queue<std::unique_ptr<QInputEvent>> input_events = engine_interface::get()->take_stored_input_events();
	std::vector<std::pair<SDL_Event, Qt::KeyboardModifiers>> sdl_events;

	while (!input_events.empty()) {
		std::unique_ptr<QInputEvent> input_event = queue::take(input_events);

		switch (input_event->type()) {
			case QEvent::MouseButtonPress:
			case QEvent::MouseButtonRelease:
			case QEvent::MouseMove:
			case QEvent::HoverEnter:
			case QEvent::HoverLeave:
			case QEvent::HoverMove:
				if (engine_interface::get()->is_modal_dialog_open()) {
					//ignore mouse events if a modal dialog is opened
					continue;
				}
				break;
			default:
				break;
		}

		switch (input_event->type()) {
			case QEvent::HoverMove: {
				const QHoverEvent *hover_event = static_cast<QHoverEvent *>(input_event.get());
				if (hover_event->pos() == CursorScreenPos) {
					//ignore if the hover event has the same position as the cursor currently holds
					continue;
				}
				break;
			}
			case QEvent::KeyPress:
			case QEvent::KeyRelease: {
				const QKeyEvent *key_event = static_cast<QKeyEvent *>(input_event.get());
				if (key_event->key() == 0) {
					//can happen, but is not relevant for the engine in this case; the documentation of QKeyEvent::key() says that a value of 0 can occur as a compose sequence, keyboard macro or key event compression
					continue;
				}
				break;
			}
			default:
				break;
		}

		const Qt::KeyboardModifiers modifiers = input_event->modifiers();
		SDL_Event sdl_event = qevent_to_sdl_event(std::move(input_event));

		if (sdl_event.type == SDL_MOUSEMOTION) {
			//only process the last mouse event for this frame
			std::erase_if(sdl_events, [](const std::pair<SDL_Event, Qt::KeyboardModifiers> &event_pair) {
				return event_pair.first.type == SDL_MOUSEMOTION;
			});
		}

		sdl_events.push_back({ std::move(sdl_event), modifiers });
	}

	for (auto &[sdl_event, modifiers] : sdl_events) {
		SdlDoEvent(*GetCallbacks(), sdl_event, modifiers);
	}

	int interrupts = 0;

	for (;;) {
		// Time of frame over? This makes the CPU happy. :(
		ticks = SDL_GetTicks();
		if (!interrupts && ticks < NextFrameTicks) {
			const uint64_t ms = static_cast<uint64_t>(NextFrameTicks - ticks);
			co_await QCoro::sleepFor(std::chrono::milliseconds(ms));
			ticks = SDL_GetTicks();
		}
		while (ticks >= (unsigned long)(NextFrameTicks)) {
			++interrupts;
			NextFrameTicks += FrameTicks;
		}

		int i = PollEvent();

		// Network
		size_t s = 0;
		if (IsNetworkGame()) {
			s = network_manager::get()->get_file_descriptor()->HasDataToRead();
			if (s > 0) {
				co_await NetworkEvent();
			}
		}

		// No more input and time for frame over: return
		if (!i && s <= 0 && interrupts) {
			break;
		}
	}

	handleInput(nullptr);

	cursor::set_last_scroll_pos(QPoint(-1, -1));

	//if the current cursor changed because of input events, trigger the necessary updates
	cursor::check_current_cursor_changed();

	//update the current map viewport top left pixel pos after all mouse input events have been processed, to pool their effects together
	engine_interface::get()->update_map_view_top_left_pixel_pos();

	if (!SkipGameCycle--) {
		SkipGameCycle = SkipFrames;
	}
}

/**
**  Convert a SDL_Keycode to a string
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
