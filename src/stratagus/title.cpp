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
/**@name title.cpp - The title screen. */
//
//      (c) Copyright 2007 by Jimmy Salmon
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

#include "title.h"

#include "sound/sound_server.h"
#include "editor.h"
#include "video/font.h"
#include "video/video.h"

std::vector<TitleScreen> TitleScreens;			/// Title screens to show at startup
static bool WaitNoEvent;			/// Flag got an event

/**
**  Callback for input.
*/
static void WaitCallbackButtonPressed(unsigned, const Qt::KeyboardModifiers)
{
	WaitNoEvent = false;
}

/**
**  Callback for input.
*/
static void WaitCallbackButtonReleased(unsigned, const Qt::KeyboardModifiers)
{
}

/**
**  Callback for input.
*/
static void WaitCallbackKeyPressed(unsigned, unsigned, const Qt::KeyboardModifiers)
{
	WaitNoEvent = false;
	return;
}

/**
**  Callback for input.
*/
static void WaitCallbackKeyReleased(unsigned, unsigned, const Qt::KeyboardModifiers)
{
}

/**
**  Callback for input.
*/
static void WaitCallbackKeyRepeated(unsigned, unsigned, const Qt::KeyboardModifiers)
{
}

/**
**  Callback for input.
*/
static void WaitCallbackMouse(const PixelPos &, const Qt::KeyboardModifiers)
{
}

/**
**  Callback for exit.
*/
static void WaitCallbackExit()
{
}

void TitleScreen::ShowLabels(std::vector<std::function<void(renderer *)>> &render_commands) const
{
	const std::vector<TitleScreenLabel> &labels = this->Labels;

	if (labels.empty()) {
		return;
	}

	for (const TitleScreenLabel &title_label : labels) {
		if (!title_label.Font) {
			continue;
		}
		// offsets are for 640x480, scale up to actual resolution
		const int x = title_label.Xofs * Video.Width / 640;
		const int y = title_label.Yofs * Video.Height / 480;
		CLabel label(title_label.Font);

		if (title_label.Flags & TitleFlagCenter) {
			label.DrawCentered(x, y, title_label.Text, render_commands);
		} else {
			label.Draw(x, y, title_label.Text, render_commands);
		}
	}
}

boost::asio::awaitable<void> TitleScreen::ShowTitleImage(std::vector<std::function<void(renderer *)>> &render_commands) const
{
	const EventCallback *old_callbacks = GetCallbacks();
	EventCallback callbacks;

	WaitNoEvent = true;

	callbacks.ButtonPressed = WaitCallbackButtonPressed;
	callbacks.ButtonReleased = WaitCallbackButtonReleased;
	callbacks.MouseMoved = WaitCallbackMouse;
	callbacks.MouseExit = WaitCallbackExit;
	callbacks.KeyPressed = WaitCallbackKeyPressed;
	callbacks.KeyReleased = WaitCallbackKeyReleased;
	callbacks.KeyRepeated = WaitCallbackKeyRepeated;

	SetCallbacks(&callbacks);

	std::shared_ptr<CGraphic> g = CGraphic::New(this->File);
	g->Load(centesimal_int(1));
	if (this->StretchImage) {
		g->Resize(Video.Width, Video.Height);
	}

	int timeout = this->Timeout ? this->Timeout * CYCLES_PER_SECOND : -1;

	while (timeout-- && WaitNoEvent) {
		g->DrawClip((Video.Width - g->Width) / 2, (Video.Height - g->Height) / 2, render_commands);
		this->ShowLabels(render_commands);

		co_await WaitEventsOneFrame();
	}

	SetCallbacks(old_callbacks);
}

/**
**  Show the title screens
*/
boost::asio::awaitable<void> ShowTitleScreens(std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (TitleScreens.empty()) {
		co_return;
	}

	SetVideoSync();

	for (const TitleScreen &title_screen : TitleScreens) {
		if ((CEditor::get()->is_running() && !title_screen.Editor) || (!CEditor::get()->is_running() && title_screen.Editor)) {
			continue;
		}

		if (!title_screen.File.empty()) {
			co_await title_screen.ShowTitleImage(render_commands);
		}

		Video.ClearScreen();
	}
}
