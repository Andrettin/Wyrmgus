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
/**@name linedraw.cpp - The general linedraw functions. */
//
//      (c) Copyright 2000-2011 by Lutz Sammer, Stephan Rasenberg,
//                                 Jimmy Salmon, Nehal Mistry and Pali Rohár
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

#include "video/video.h"

#include "intern_video.h"

#include "util/assert_util.h"
#include "video/renderer.h"

/**
** Bitmask, denoting a position left/right/above/below clip rectangle
** (mainly used by VideoDrawLineClip)
*/
static constexpr int ClipCodeInside = 0; /// Clipping inside rectangle
static constexpr int ClipCodeAbove = 1; /// Clipping above rectangle
static constexpr int ClipCodeBelow = 2; /// Clipping below rectangle
static constexpr int ClipCodeLeft = 4; /// Clipping left rectangle
static constexpr int ClipCodeRight = 8; /// Clipping right rectangle

namespace linedraw_gl
{

/**
**  Draw pixel unclipped.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
*/
void DrawPixel(uint32_t color, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands)
{
	GLubyte r, g, b, a;

	CVideo::GetRGBA(color, &r, &g, &b, &a);

	render_commands.push_back([x, y, r, g, b, a](renderer *renderer) {
		renderer->draw_pixel(QPoint(x, y), QColor(r, g, b, a));
	});
}

/**
**  Draw horizontal line unclipped.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param width  width of line (0=don't draw).
*/
void DrawHLine(uint32_t color, int x, int y, int width, std::vector<std::function<void(renderer *)>> &render_commands)
{
	GLubyte r, g, b, a;
	CVideo::GetRGBA(color, &r, &g, &b, &a);

	render_commands.push_back([x, y, width, r, g, b, a](renderer *renderer) {
		renderer->draw_horizontal_line(QPoint(x, y), width, QColor(r, g, b, a));
	});
}

/**
**  Draw vertical line unclipped.
**
**  @param color   color
**  @param x       x coordinate on the screen
**  @param y       y coordinate on the screen
**  @param height  height of line (0=don't draw).
*/
void DrawVLine(uint32_t color, int x, int y, int height, std::vector<std::function<void(renderer *)>> &render_commands)
{
	GLubyte r, g, b, a;

	CVideo::GetRGBA(color, &r, &g, &b, &a);

	render_commands.push_back([x, y, height, r, g, b, a](renderer *renderer) {
		renderer->draw_vertical_line(QPoint(x, y), height, QColor(r, g, b, a));
	});
}

/**
**  Draw line unclipped into 32bit framebuffer.
**
**  @param color  color
**  @param x1     Source x coordinate on the screen
**  @param y1     Source y coordinate on the screen
**  @param x2     Destination x coordinate on the screen
**  @param y2     Destination y coordinate on the screen
*/
void DrawLine(uint32_t color, int x1, int y1, int x2, int y2, std::vector<std::function<void(renderer *)>> &render_commands)
{
	GLubyte r, g, b, a;

	float xx1 = (float)x1;
	float xx2 = (float)x2;
	float yy1 = (float)y1;
	float yy2 = (float)y2;

	if (xx1 <= xx2) {
		xx2 += 0.5f;
	} else {
		xx1 += 0.5f;
	}
	if (yy1 <= yy2) {
		yy2 += 0.5f;
	} else {
		yy1 += 0.5f;
	}

	CVideo::GetRGBA(color, &r, &g, &b, &a);

	render_commands.push_back([xx1, yy1, xx2, yy2, r, g, b, a](renderer *renderer) {
		renderer->draw_line(QPoint(xx1, yy1), QPoint(xx2, yy2), QColor(r, g, b, a));
	});
}

/**
**  Delivers bitmask denoting given point is left/right/above/below
**      clip rectangle, used for faster determinination of clipped position.
**
**  @param x  pixel's x position (not restricted to screen width)
**  @param y  pixel's y position (not restricted to screen height)
*/
static int ClipCodeLine(int x, int y)
{
	int result;

	if (y < ClipY1) {
		result = ClipCodeAbove;
	} else if (y > ClipY2) {
		result = ClipCodeBelow;
	} else {
		result = ClipCodeInside;
	}

	if (x < ClipX1) {
		result |= ClipCodeLeft;
	} else if (x > ClipX2) {
		result |= ClipCodeRight;
	}

	return result;
}

/**
**  Denotes entire line located at the same side outside clip rectangle
**      (point 1 and 2 are both as left/right/above/below the clip rectangle)
**
**  @param code1  ClipCode of one point of line
**  @param code2  ClipCode of second point of line
*/
static int LineIsUnclippedOnSameSide(int code1, int code2)
{
	return code1 & code2;
}

/**
**  Denotes part of (or entire) line located outside clip rectangle
**      (point 1 and/or 2 is outside clip rectangle)
**
**  @param code1  ClipCode of one point of line
**  @param code2  ClipCode of second point of line
*/
static int LineIsUnclipped(int code1, int code2)
{
	return code1 | code2;
}

/**
**  Draw line clipped.
**      Based on Sutherland-Cohen clipping technique
**      (Replaces Liang/Barksy clipping algorithm in CVS version 1.18, which
**       might be faster, but that one contained some BUGs)
**
**  @param color  color
**  @param x1     Source x coordinate on the screen
**  @param y1     Source y coordinate on the screen
**  @param x2     Destination x coordinate on the screen
**  @param y2     Destination y coordinate on the screen
*/
void DrawLineClip(uint32_t color, int x1, int y1, int x2, int y2, std::vector<std::function<void(renderer *)>> &render_commands)
{
	int code1;
	int code2;

	// Make sure coordinates or on/in clipped rectangle
	while (code1 = ClipCodeLine(x1, y1), code2 = ClipCodeLine(x2, y2),
		   LineIsUnclipped(code1, code2)) {
		if (LineIsUnclippedOnSameSide(code1, code2)) {
			return;
		}

		if (!code1) {
			std::swap(x1, x2);
			std::swap(y1, y2);
			code1 = code2;
		}

		if (code1 & ClipCodeAbove) {
			int temp = ClipY1;
			x1 += (int)(((long)(temp - y1) * (x2 - x1)) / (y2 - y1));
			y1 = temp;
		} else if (code1 & ClipCodeBelow) {
			int temp = ClipY2;
			x1 += (int)(((long)(temp - y1) * (x2 - x1)) / (y2 - y1));
			y1 = temp;
		} else if (code1 & ClipCodeLeft) {
			int temp = ClipX1;
			y1 += (int)(((long)(temp - x1) * (y2 - y1)) / (x2 - x1));
			x1 = temp;
		} else {  /* code1 & ClipCodeRight */
			int temp = ClipX2;
			y1 += (int)(((long)(temp - x1) * (y2 - y1)) / (x2 - x1));
			x1 = temp;
		}
	}

	// Draw line based on clipped coordinates
	// FIXME: As the clipped coordinates are rounded to integers, the line's
	//        direction vector might be slightly off. Somehow, the sub-pixel
	//        position(s) on the clipped retangle should be denoted to the line
	//        drawing routine..
	assert_throw(x1 >= ClipX1 && x2 >= ClipX1 && x1 <= ClipX2 && x2 <= ClipX2 &&
		   y1 >= ClipY1 && y2 >= ClipY1 && y1 <= ClipY2 && y2 <= ClipY2);
	DrawLine(color, x1, y1, x2, y2, render_commands);
}

/**
**  Draw rectangle.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param h      height of rectangle (0=don't draw).
**  @param w      width of rectangle (0=don't draw).
*/
void DrawRectangle(uint32_t color, int x, int y, int w, int h, std::vector<std::function<void(renderer *)>> &render_commands)
{
	GLubyte r, g, b, a;

	CVideo::GetRGBA(color, &r, &g, &b, &a);

	render_commands.push_back([x, y, w, h, r, g, b, a](renderer *renderer) {
		renderer->draw_rect(QPoint(x, y), QSize(w, h), QColor(r, g, b, a));
	});
}

/**
**  Draw translucent rectangle.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param h      height of rectangle (0=don't draw).
**  @param w      width of rectangle (0=don't draw).
**  @param alpha  alpha value of pixel.
*/
void DrawTransRectangle(uint32_t color, int x, int y,
						int w, int h, unsigned char alpha, std::vector<std::function<void(renderer *)>> &render_commands)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	DrawRectangle(color, x, y, w, h, render_commands);
}

/**
**  Draw rectangle clipped.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param h      height of rectangle (0=don't draw).
**  @param w      width of rectangle (0=don't draw).
*/
void DrawRectangleClip(uint32_t color, int x, int y,
					   int w, int h, std::vector<std::function<void(renderer *)>> &render_commands)
{
	// Ensure non-empty rectangle
	if (!w || !h) {
		// rectangle is `void'
		return;
	}

	// Clip rectangle boundary
	int left = 1;
	int right = 1;
	int top = 1;
	int bottom = 1;

	if (x < ClipX1) {            // no left side
		int f = ClipX1 - x;
		if (w <= f) {
			return;                    // entire rectangle left --> not visible
		}
		w -= f;
		x = ClipX1;
		left = 0;
	}
	if ((x + w) > ClipX2 + 1) {     // no right side
		if (x > ClipX2) {
			return;                    // entire rectangle right --> not visible
		}
		w = ClipX2 - x + 1;
		right = 0;
	}
	if (y < ClipY1) {               // no top
		int f = ClipY1 - y;
		if (h <= f) {
			return;                    // entire rectangle above --> not visible
		}
		h -= f;
		y = ClipY1;
		top = 0;
	}
	if ((y + h) > ClipY2 + 1) {    // no bottom
		if (y > ClipY2) {
			return;                  // entire rectangle below --> not visible
		}
		h = ClipY2 - y + 1;
		bottom = 0;
	}

	// Draw (part of) rectangle sides
	// Note: _hline and _vline should be able to handle zero width/height
	if (top) {
		//Wyrmgus start
//		DrawHLine(color, x, y, w);
		DrawHLine(color, x, y, w - 1, render_commands);
		//Wyrmgus end
		if (!--h) {
			return;                    // rectangle as horizontal line
		}
		++y;
	}
	if (bottom) {
		//Wyrmgus start
//		DrawHLine(color, x, y + h - 1, w);
		DrawHLine(color, x, y + h - 1, w - 1, render_commands);
		//Wyrmgus end
		--h;
	}
	if (left) {
		DrawVLine(color, x, y, h, render_commands);
		if (!--w) {
			return;                    // rectangle as vertical line
		}
		++x;
	}
	if (right) {
		DrawVLine(color, x + w - 1, y, h, render_commands);
	}
}

/**
**  Fill rectangle.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param h      height of rectangle (0=don't draw).
**  @param w      width of rectangle (0=don't draw).
*/
void FillRectangle(uint32_t color, int x, int y,
				   int w, int h, std::vector<std::function<void(renderer *)>> &render_commands)
{
	GLubyte r, g, b, a;

	CVideo::GetRGBA(color, &r, &g, &b, &a);

	render_commands.push_back([x, y, w, h, r, g, b, a](renderer *renderer) {
		renderer->fill_rect(QPoint(x, y), QSize(w, h), QColor(r, g, b, a));
	});
}

/**
**  Draw translucent rectangle.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param h      height of rectangle (0=don't draw).
**  @param w      width of rectangle (0=don't draw).
**  @param alpha  alpha value of pixel.
*/
void FillTransRectangle(uint32_t color, int x, int y,
						int w, int h, unsigned char alpha, std::vector<std::function<void(renderer *)>> &render_commands)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	FillRectangle(color, x, y, w, h, render_commands);
}

/**
**  Fill rectangle clipped.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param h      height of rectangle (0=don't draw).
**  @param w      width of rectangle (0=don't draw).
*/
void FillRectangleClip(uint32_t color, int x, int y,
					   int w, int h, std::vector<std::function<void(renderer *)>> &render_commands)
{
	CLIP_RECTANGLE(x, y, w, h);
	FillRectangle(color, x, y, w, h, render_commands);
}

/**
**  Fill rectangle translucent clipped.
**
**  @param color  color
**  @param x      x coordinate on the screen
**  @param y      y coordinate on the screen
**  @param h      height of rectangle (0=don't draw).
**  @param w      width of rectangle (0=don't draw).
**  @param alpha  alpha value of pixels.
*/
void FillTransRectangleClip(uint32_t color, int x, int y,
							int w, int h, unsigned char alpha, std::vector<std::function<void(renderer *)>> &render_commands)
{
	GLubyte r, g, b;

	CVideo::GetRGB(color, &r, &g, &b);
	color = CVideo::MapRGBA(r, g, b, alpha);
	FillRectangleClip(color, x, y, w, h, render_commands);
}

}

void CVideo::DrawVLine(uint32_t color, int x, int y, int height, std::vector<std::function<void(renderer *)>> &render_commands)
{
	linedraw_gl::DrawVLine(color, x, y, height, render_commands);
}

void CVideo::DrawHLine(uint32_t color, int x, int y, int width, std::vector<std::function<void(renderer *)>> &render_commands)
{
	linedraw_gl::DrawHLine(color, x, y, width, render_commands);
}

void CVideo::DrawLine(uint32_t color, int sx, int sy, int dx, int dy, std::vector<std::function<void(renderer *)>> &render_commands)
{
	linedraw_gl::DrawLine(color, sx, sy, dx, dy, render_commands);
}

void CVideo::DrawLineClip(uint32_t color, const PixelPos &pos1, const PixelPos &pos2, std::vector<std::function<void(renderer *)>> &render_commands)
{
	linedraw_gl::DrawLineClip(color, pos1.x, pos1.y, pos2.x, pos2.y, render_commands);
}

void CVideo::DrawRectangle(uint32_t color, int x, int y, int w, int h, std::vector<std::function<void(renderer *)>> &render_commands)
{
	linedraw_gl::DrawRectangle(color, x, y, w, h, render_commands);
}

void CVideo::DrawTransRectangle(uint32_t color, int x, int y, int w, int h, unsigned char alpha, std::vector<std::function<void(renderer *)>> &render_commands)
{
	linedraw_gl::DrawTransRectangle(color, x, y, w, h, alpha, render_commands);
}

void CVideo::DrawRectangleClip(uint32_t color, int x, int y, int w, int h, std::vector<std::function<void(renderer *)>> &render_commands)
{
	linedraw_gl::DrawRectangleClip(color, x, y, w, h, render_commands);
}

void CVideo::FillRectangle(uint32_t color, int x, int y, int w, int h, std::vector<std::function<void(renderer *)>> &render_commands)
{
	linedraw_gl::FillRectangle(color, x, y, w, h, render_commands);
}

void CVideo::FillTransRectangle(uint32_t color, int x, int y, int w, int h, unsigned char alpha, std::vector<std::function<void(renderer *)>> &render_commands)
{
	linedraw_gl::FillTransRectangle(color, x, y, w, h, alpha, render_commands);
}

void CVideo::FillRectangleClip(uint32_t color, int x, int y, int w, int h, std::vector<std::function<void(renderer *)>> &render_commands)
{
	linedraw_gl::FillRectangleClip(color, x, y, w, h, render_commands);
}

void CVideo::FillTransRectangleClip(uint32_t color, int x, int y, int w, int h, unsigned char alpha, std::vector<std::function<void(renderer *)>> &render_commands)
{
	linedraw_gl::FillTransRectangleClip(color, x, y, w, h, alpha, render_commands);
}
