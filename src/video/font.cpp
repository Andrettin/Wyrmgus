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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon, Nehal Mistry
//                                 and Andrettin
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

#include "video/font.h"

#include "database/defines.h"
#include "intern_video.h"
#include "util/image_util.h"
#include "util/point_util.h"
#include "util/util.h"
#include "video/font_color.h"
#include "video/video.h"

static const wyrmgus::font_color *LastTextColor;      /// Last text color

namespace wyrmgus {

//Wyrmgus start
//void font::drawString(gcn::Graphics *graphics, const std::string &txt, int x, int y)
void font::drawString(gcn::Graphics *graphics, const std::string &txt, int x, int y, bool is_normal, std::vector<std::function<void(renderer *)>> &render_commands)
//Wyrmgus end
{
	const gcn::ClipRectangle &r = graphics->getCurrentClipArea();
	int right = std::min<int>(r.x + r.width - 1, Video.Width - 1);
	int bottom = std::min<int>(r.y + r.height - 1, Video.Height - 1);

	if (r.x > right || r.y > bottom) {
		return;
	}

	PushClipping();
	SetClipping(r.x, r.y, right, bottom);
	//Wyrmgus start
//	CLabel(this).DrawClip(x + r.xOffset, y + r.yOffset, txt);
	CLabel(this).DrawClip(x + r.xOffset, y + r.yOffset, txt, is_normal, render_commands);
	//Wyrmgus end
	PopClipping();
}

}

/**
**  Draw character with current color.
**
**  @param g   Pointer to object
**  @param gx  X offset into object
**  @param gy  Y offset into object
**  @param w   width to display
**  @param h   height to display
**  @param x   X screen position
**  @param y   Y screen position
*/
static void VideoDrawChar(CGraphic &g,
						  int gx, int gy, int w, int h, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands)
{
	g.DrawSub(gx, gy, w, h, x, y, render_commands);
}

/**
**  Get the next utf8 character from a string
*/
static bool GetUTF8(const std::string &text, size_t &pos, int &utf8)
{
	// end of string
	if (pos >= text.size()) {
		return false;
	}

	int count;
	char c = text[pos++];

	// ascii
	if (!(c & 0x80)) {
		utf8 = c;
		return true;
	}

	if ((c & 0xE0) == 0xC0) {
		utf8 = (c & 0x1F);
		count = 1;
	} else if ((c & 0xF0) == 0xE0) {
		utf8 = (c & 0x0F);
		count = 2;
	} else if ((c & 0xF8) == 0xF0) {
		utf8 = (c & 0x07);
		count = 3;
	} else if ((c & 0xFC) == 0xF8) {
		utf8 = (c & 0x03);
		count = 4;
	} else if ((c & 0xFE) == 0xFC) {
		utf8 = (c & 0x01);
		count = 5;
	} else {
		DebugPrint("Invalid utf8\n");
		return false;
	}

	while (count--) {
		c = text[pos++];
		if ((c & 0xC0) != 0x80) {
			DebugPrint("Invalid utf8\n");
			return false;
		}
		utf8 <<= 6;
		utf8 |= (c & 0x3F);
	}
	return true;
}

/**
**  Get the next utf8 character from an array of chars
*/
static bool GetUTF8(const char text[], const size_t len, size_t &pos, int &utf8)
{
	// end of string
	if (pos >= len) {
		return false;
	}

	int count;
	char c = text[pos++];

	// ascii
	if (!(c & 0x80)) {
		utf8 = c;
		return true;
	}

	if ((c & 0xE0) == 0xC0) {
		utf8 = (c & 0x1F);
		count = 1;
	} else if ((c & 0xF0) == 0xE0) {
		utf8 = (c & 0x0F);
		count = 2;
	} else if ((c & 0xF8) == 0xF0) {
		utf8 = (c & 0x07);
		count = 3;
	} else if ((c & 0xFC) == 0xF8) {
		utf8 = (c & 0x03);
		count = 4;
	} else if ((c & 0xFE) == 0xFC) {
		utf8 = (c & 0x01);
		count = 5;
	} else {
		DebugPrint("Invalid utf8 I %c <%s> [%lu]\n" _C_ c _C_ text _C_(long) pos);
		return false;
	}

	while (count--) {
		c = text[pos++];
		if ((c & 0xC0) != 0x80) {
			DebugPrint("Invalid utf8 II\n");
			return false;
		}
		utf8 <<= 6;
		utf8 |= (c & 0x3F);
	}
	return true;
}

namespace wyrmgus {

int font::Height()
{
	if (!this->is_loaded()) {
		this->load();
	}

	return G->Height;
}

/**
**  Returns the pixel width of text.
**
**  @param text  Text to calculate the width of.
**
**  @return      The width in pixels of the text.
*/
int font::Width(const int number)
{
	if (!this->is_loaded()) {
		this->load();
	}

	int width = 0;
#if 0
	bool isformat = false;
#endif
	int utf8;
	size_t pos = 0;
	std::string text = FormatNumber(number);
	const int len = text.length();

	while (GetUTF8(text.c_str(), len, pos, utf8)) {
		width += this->char_width[utf8 - 32] + 1;
	}
	return width;
}

/**
**  Returns the pixel width of text.
**
**  @param text  Text to calculate the width of.
**
**  @return      The width in pixels of the text.
*/
int font::Width(const std::string &text)
{
	if (!this->is_loaded()) {
		this->load();
	}

	int width = 0;
	bool isformat = false;
	int utf8;
	size_t pos = 0;

	while (GetUTF8(text, pos, utf8)) {
		if (utf8 == '~') {
			if (text[pos] == '|') {
				++pos;
				continue;
			}
			if (pos >= text.size()) {  // bad formatted string
				break;
			}
			if (text[pos] == '<' || text[pos] == '>') {
				isformat = false;
				++pos;
				continue;
			}
			if (text[pos] == '!') {
				++pos;
				continue;
			}
			if (text[pos] != '~') { // ~~ -> ~
				isformat = !isformat;
				continue;
			}
		}
		if (!isformat) {
			width += this->char_width[utf8 - 32] + 1;
		}
	}
	return width;
}

}

extern int convertKey(const char *key);

/**
**  Get the hot key from a string
*/
int GetHotKey(const std::string &text)
{
	int hotkey = 0;
	size_t pos = 0;

	if (text.length() > 1) {
		hotkey = convertKey(text.c_str());
	} else if (text.length() == 1) {
		GetUTF8(text, pos, hotkey);
	}

	return hotkey;
}

namespace wyrmgus {

font::font(const std::string &identifier) : data_entry(identifier)
{
}

font::~font()
{
}

}

/**
**  Draw character with current color clipped into 8 bit framebuffer.
**
**  @param g   Pointer to object
**  @param gx  X offset into object
**  @param gy  Y offset into object
**  @param w   width to display
**  @param h   height to display
**  @param x   X screen position
**  @param y   Y screen position
*/
static void VideoDrawCharClip(CGraphic &g, int gx, int gy, int w, int h,
							  int x, int y, std::vector<std::function<void(renderer *)>> &render_commands)
{
	int ox;
	int oy;
	int ex;
	CLIP_RECTANGLE_OFS(x, y, w, h, ox, oy, ex);
	UNUSED(ex);
	VideoDrawChar(g, gx + ox, gy + oy, w, h, x, y, render_commands);
}

namespace wyrmgus {

template<bool CLIP>
unsigned int font::DrawChar(CGraphic &g, int utf8, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	int c = utf8 - 32;
	Assert(c >= 0);
	const int ipr = this->G->GraphicWidth / this->G->Width;

	if (c < 0 || ipr * this->G->GraphicHeight / this->G->Height <= c) {
		c = 0;
	}
	const int w = this->char_width[c];
	const int gx = (c % ipr) * this->G->Width;
	const int gy = (c / ipr) * this->G->Height;

	if constexpr (CLIP) {
		VideoDrawCharClip(g, gx, gy, w, this->G->Height, x , y, render_commands);
	} else {
		VideoDrawChar(g, gx, gy, w, this->G->Height, x, y, render_commands);
	}
	return w + 1;
}

CGraphic *font::get_font_color_graphic(const wyrmgus::font_color *font_color)
{
	if (!this->font_color_graphics.contains(font_color)) {
		//load the font color graphics on demand
		this->make_font_color_texture(font_color);
	}

	const auto find_iterator = this->font_color_graphics.find(font_color);

	if (find_iterator != this->font_color_graphics.end()) {
		return find_iterator->second.get();
	}

	throw std::runtime_error("Could not load font color \"" + font_color->get_identifier()  + "\" for font \"" + this->get_identifier() + "\".");
}

}

/**
**  Draw text with font at x,y clipped/unclipped.
**
**  ~    is special prefix.
**  ~~   is the ~ character self.
**  ~!   print next character reverse.
**  ~<   start reverse.
**  ~>   switch back to last used color.
**
**  @param x     X screen position
**  @param y     Y screen position
**  @param font  Font number
**  @param text  Text to be displayed.
**  @param clip  Flag if TRUE clip, otherwise not.
**
**  @return      The length of the printed text.
*/
template <const bool CLIP>
int CLabel::DoDrawText(int x, int y, const char *const text, const size_t len, const font_color *fc, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	int widths = 0;
	int utf8;
	bool tab;
	const int tabSize = 4; // FIXME: will be removed when text system will be rewritten
	size_t pos = 0;
	const wyrmgus::font_color *backup = fc;
	bool isColor = false;
	//Wyrmgus start
//	CGraphic *g = font->get_font_color_graphic(FontColor);
	CGraphic *g = this->font->get_font_color_graphic(fc);
	//Wyrmgus end

	while (GetUTF8(text, len, pos, utf8)) {
		tab = false;
		if (utf8 == '\t') {
			tab = true;
		} else if (utf8 == '~') {
			switch (text[pos]) {
				case '\0':  // wrong formatted string.
					DebugPrint("oops, format your ~\n");
					return widths;
				case '~':
					++pos;
					break;
				case '|':
					++pos;
					continue;
				case '!':
					if (fc != reverse) {
						fc = reverse;
						g = font->get_font_color_graphic(fc);
					}
					++pos;
					continue;
				case '<':
					LastTextColor = fc;
					if (fc != reverse) {
						isColor = true;
						fc = reverse;
						g = font->get_font_color_graphic(fc);
					}
					++pos;
					continue;
				case '>':
					if (fc != LastTextColor) {
						std::swap(fc, LastTextColor);
						isColor = false;
						g = font->get_font_color_graphic(fc);
					}
					++pos;
					continue;

				default: {
					const char *p = text + pos;
					while (*p && *p != '~') {
						++p;
					}
					if (!*p) {
						DebugPrint("oops, format your ~\n");
						return widths;
					}
					std::string color;

					color.insert(0, text + pos, p - (text + pos));
					pos = p - text + 1;
					LastTextColor = fc;
					const wyrmgus::font_color *fc_tmp = wyrmgus::font_color::get(color);
					if (fc_tmp) {
						isColor = true;
						fc = fc_tmp;
						g = font->get_font_color_graphic(fc);
					}
					continue;
				}
			}
		}
		if (tab) {
			for (int tabs = 0; tabs < tabSize; ++tabs) {
				widths += font->DrawChar<CLIP>(*g, ' ', x + widths, y, render_commands);
			}
		} else {
			widths += font->DrawChar<CLIP>(*g, utf8, x + widths, y, render_commands);
		}

		if (isColor == false && fc != backup) {
			fc = backup;
			g = font->get_font_color_graphic(fc);
		}
	}
	return widths;
}

CLabel::CLabel(wyrmgus::font *f, const wyrmgus::font_color *nc, const wyrmgus::font_color *rc) : font(f)
{
	if (!f->is_loaded()) {
		f->load();
	}

	normal = nc;
	reverse = rc;
}

CLabel::CLabel(wyrmgus::font *f)
	: CLabel(f, wyrmgus::defines::get()->get_default_font_color(), wyrmgus::defines::get()->get_default_highlight_font_color())
{
}

void CLabel::SetNormalColor(const wyrmgus::font_color *nc)
{
	this->normal = nc;
}

/// Draw text/number unclipped
int CLabel::Draw(int x, int y, const char *const text, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	return DoDrawText<false>(x, y, text, strlen(text), normal, render_commands);
}

int CLabel::Draw(int x, int y, const std::string &text, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	return DoDrawText<false>(x, y, text.c_str(), text.size(), normal, render_commands);
}

int CLabel::Draw(int x, int y, int number, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	std::string str = FormatNumber(number);
	return DoDrawText<false>(x, y, str.c_str(), str.length(), normal, render_commands);
}

/// Draw text/number clipped
int CLabel::DrawClip(int x, int y, const char *const text, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	return DoDrawText<true>(x, y, text, strlen(text), normal, render_commands);
}

//Wyrmgus start
//int CLabel::DrawClip(int x, int y, const std::string &text) const
int CLabel::DrawClip(int x, int y, const std::string &text, bool is_normal, std::vector<std::function<void(renderer *)>> &render_commands) const
//Wyrmgus end
{
	//Wyrmgus start
//	return DoDrawText<true>(x, y, text.c_str(), text.size(), normal);
	if (is_normal) {
		return DoDrawText<true>(x, y, text.c_str(), text.size(), normal, render_commands);
	} else {
		return DoDrawText<true>(x, y, text.c_str(), text.size(), reverse, render_commands);
	}
	//Wyrmgus end
}

int CLabel::DrawClip(int x, int y, int number, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	std::string str = FormatNumber(number);
	return DoDrawText<true>(x, y, str.c_str(), str.length(), normal, render_commands);
}


/// Draw reverse text/number unclipped
int CLabel::DrawReverse(int x, int y, const char *const text, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	return DoDrawText<false>(x, y, text, strlen(text), reverse, render_commands);
}

int CLabel::DrawReverse(int x, int y, const std::string &text, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	return DoDrawText<false>(x, y, text.c_str(), text.size(), reverse, render_commands);
}

int CLabel::DrawReverse(int x, int y, int number, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	std::string str = FormatNumber(number);
	return DoDrawText<false>(x, y, str.c_str(), str.length(), reverse, render_commands);
}

/// Draw reverse text/number clipped
int CLabel::DrawReverseClip(int x, int y, const char *const text, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	return DoDrawText<true>(x, y, text, strlen(text), reverse, render_commands);
}

int CLabel::DrawReverseClip(int x, int y, const std::string &text, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	return DoDrawText<true>(x, y, text.c_str(), text.size(), reverse, render_commands);
}

int CLabel::DrawReverseClip(int x, int y, int number, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	std::string str = FormatNumber(number);
	return DoDrawText<true>(x, y, str.c_str(), str.length(), reverse, render_commands);
}

int CLabel::DrawCentered(int x, int y, const std::string &text, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	int dx = font->Width(text);
	DoDrawText<false>(x - dx / 2, y, text.c_str(), text.size(), normal, render_commands);
	return dx / 2;
}

int CLabel::DrawReverseCentered(int x, int y, const std::string &text, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	int dx = font->Width(text);
	DoDrawText<false>(x - dx / 2, y, text.c_str(), text.size(), reverse, render_commands);
	return dx / 2;
}

/**
**  Return the index of first occurrence of c in [s- s + maxlen]
**
**  @param s       original string.
**  @param c       character to find.
**  @param maxlen  size limit of the search. (0 means unlimited). (in char if font == null else in pixels).
**  @param font    if specified use font->Width() instead of strlen.
**
**  @return computed value.
*/
static int strchrlen(const std::string &s, char c, unsigned int maxlen, wyrmgus::font *font)
{
	if (s.empty()) {
		return 0;
	}
	int res = s.find(c);
	res = (res == -1) ? s.size() : res;

	if (!maxlen || (!font && (unsigned int) res < maxlen) || (font && (unsigned int) font->Width(s.substr(0, res)) < maxlen)) {
		return res;
	}
	if (!font) {
		res = s.rfind(' ', maxlen);
		if (res == -1) {
			// line too long
			return maxlen;
		}
	} else {
		res = s.rfind(' ', res);
		while (res != -1 && (unsigned int) font->Width(s.substr(0, res)) > maxlen) {
			res = s.rfind(' ', res - 1);
		}
		if (res == -1) {
			// Line too long.
			// FIXME.
			//Wyrmgus start
			return maxlen / font->Width(1);
			//Wyrmgus end
		}
	}

	return res;
}

/**
**  Return the 'line' line of the string 's'.
**
**  @param line    line number.
**  @param s       multiline string.
**  @param maxlen  max length of the string (0 : unlimited) (in char if font == null else in pixels).
**  @param font    if specified use font->Width() instead of strlen.
**
**  @return computed value.
*/
std::string GetLineFont(unsigned int line, const std::string &s, unsigned int maxlen, wyrmgus::font *font)
{
	unsigned int res;
	std::string s1 = s;

	Assert(0 < line);

	for (unsigned int i = 1; i < line; ++i) {
		res = strchrlen(s1, '\n', maxlen, font);
		if (!res || res >= s1.size()) {
			return "";
		}
		//Wyrmgus start
//		s1 = s1.substr(res + 1);
		if (s1.substr(res, 1).find(' ') != -1 || s1.substr(res, 1).find('\n') != -1) {
			s1 = s1.substr(res + 1);
		} else {
			s1 = s1.substr(res);
		}
		//Wyrmgus end
	}
	res = strchrlen(s1, '\n', maxlen, font);
	return s1.substr(0, res);
}

namespace wyrmgus {

/**
**  Calculate the width of each character
*/
void font::MeasureWidths()
{
	const QImage image(QString::fromStdString(this->G->get_filepath().string()));
	const QSize &frame_size = G->get_original_frame_size();
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	const int maxy = image.width() / frame_size.width() * image.height() / frame_size.height();

	this->char_width = std::vector<char>(maxy, 0);
	this->char_width[0] = frame_size.width() / 2 * scale_factor;  // a reasonable value for SPACE
	const int ipr = image.width() / frame_size.width(); // images per row

	for (int y = 1; y < maxy; ++y) {
		const unsigned char *sp = (const unsigned char *)image.constBits() +
								  (y / ipr) * image.bytesPerLine() * frame_size.height() +
								  (y % ipr) * frame_size.width() - 1;
		const unsigned char *gp = sp + image.bytesPerLine() * frame_size.height();
		// Bail out if no letters left
		if (gp >= ((const unsigned char *) image.constBits() +
			image.bytesPerLine() * image.height())) {
			break;
		}
		while (sp < gp) {
			const unsigned char *lp = sp + frame_size.width();

			for (; sp < lp; --lp) {
				if (*lp != 0 && *lp != 7) {
					this->char_width[y] = std::max<char>(this->char_width[y], (lp - sp) * scale_factor);
				}
			}
			sp += image.bytesPerLine();
		}
	}
}

void font::make_font_color_texture(const wyrmgus::font_color *fc)
{
	if (this->font_color_graphics.contains(fc)) {
		// already loaded
		return;
	}

	if (!this->is_loaded()) {
		this->load();
	}

	const CGraphic &g = *this->G;

	auto newg = std::make_unique<CGraphic>(g.get_filepath());

	newg->Width = g.Width;
	newg->Height = g.Height;
	newg->NumFrames = g.NumFrames;
	newg->GraphicWidth = g.GraphicWidth;
	newg->GraphicHeight = g.GraphicHeight;
	newg->image = g.get_image();
	newg->original_size = g.get_original_size();
	newg->original_frame_size = g.get_original_frame_size();

	for (int j = 0; j < newg->image.colorCount(); ++j) {
		if (static_cast<size_t>(j) >= fc->get_colors().size()) {
			break;
		}

		const QColor &color = fc->get_colors()[j];

		newg->image.setColor(j, qRgba(color.red(), color.green(), color.blue(), j == 0 ? 0 : 255));
	}

	this->font_color_graphics[fc] = std::move(newg);
}

void font::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "file") {
		this->filepath = database::get()->get_graphics_path(this->get_module()) / value;
	} else {
		data_entry::process_sml_property(property);
	}
}

void font::initialize()
{
	this->G = CGraphic::New(this->filepath, this->size);

	data_entry::initialize();
}

bool font::is_loaded() const
{
	return this->G->IsLoaded();
}

void font::load()
{
	if (this->is_loaded()) {
		return;
	}

	this->G->Load(defines::get()->get_scale_factor());
	this->MeasureWidths();
}

void font::free_textures(std::vector<std::function<void()>> &render_commands)
{
	for (const auto &kv_pair : this->font_color_graphics) {
		std::shared_ptr<CGraphic> graphic = kv_pair.second;

		if (!graphic->has_textures()) {
			continue;
		}

		render_commands.push_back([graphic]() {
			graphic->free_textures();
		});
	}
}

}
