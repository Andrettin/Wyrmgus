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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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
#include "database/data_entry.h"
#include "database/data_type.h"
#include "guichan/font.h"

class CGraphic;

namespace wyrmgus {

class font_color;
class renderer;

class font final : public data_entry, public gcn::Font, public data_type<font>
{
	Q_OBJECT

	Q_PROPERTY(QSize size MEMBER size)

public:
	static constexpr const char *class_identifier = "font";
	static constexpr const char *database_folder = "fonts";

	//this function is kept since it is still used in tolua++
	static font *Get(const std::string &identifier)
	{
		font *font = font::get(identifier);

		return font;
	}

	explicit font(const std::string &ident);
	virtual ~font();

	virtual void process_sml_property(const sml_property &property) override;
	virtual void initialize() override;

	bool is_loaded() const;
	void load();

	int Height();
	int Width(const std::string &text);
	int Width(const int number);

	virtual int getHeight() override { return Height(); }
	virtual int getWidth(const std::string &text) override { return Width(text); }
	//Wyrmgus start
//	virtual void drawString(gcn::Graphics *graphics, const std::string &text, int x, int y);
	virtual void drawString(gcn::Graphics *graphics, const std::string &text, int x, int y, bool is_normal, std::vector<std::function<void(renderer *)>> &render_commands) override;
	//Wyrmgus end

	CGraphic *get_font_color_graphic(const wyrmgus::font_color *font_color);

	template<bool CLIP>
	unsigned int DrawChar(CGraphic &g, int utf8, int x, int y, std::vector<std::function<void(renderer *)>> &render_commands) const;

	void free_textures(std::vector<std::function<void(renderer *)>> &render_commands);

private:
	void make_font_color_texture(const font_color *fc);
	void MeasureWidths();

private:
	std::filesystem::path filepath;
	QSize size;
	std::vector<char> char_width; //real font width (starting with ' ')
	std::shared_ptr<CGraphic> G; /// Graphic object used to draw
	std::map<const font_color *, std::shared_ptr<CGraphic>> font_color_graphics;
};

}

///  Return the 'line' line of the string 's'.
extern std::string GetLineFont(unsigned int line, const std::string &s, unsigned int maxlen, wyrmgus::font *font);

/// Get the hot key from a string
extern int GetHotKey(const std::string &text);

class CLabel
{
public:
	explicit CLabel(wyrmgus::font *f, const wyrmgus::font_color *nc, const wyrmgus::font_color *rc);
	explicit CLabel(wyrmgus::font *f);

	int Height() const { return font->Height(); }

	void SetFont(wyrmgus::font *f)
	{
		this->font = f;
	}

	void SetNormalColor(const wyrmgus::font_color *nc);

	/// Draw text/number unclipped
	int Draw(int x, int y, const char *const text, std::vector<std::function<void(renderer *)>> &render_commands) const;
	int Draw(int x, int y, const std::string &text, std::vector<std::function<void(renderer *)>> &render_commands) const;
	int Draw(int x, int y, int number, std::vector<std::function<void(renderer *)>> &render_commands) const;
	/// Draw text/number clipped
	int DrawClip(int x, int y, const char *const text, std::vector<std::function<void(renderer *)>> &render_commands) const;
	//Wyrmgus start
//	int DrawClip(int x, int y, const std::string &text) const;
	int DrawClip(int x, int y, const std::string &text, bool is_normal, std::vector<std::function<void(renderer *)>> &render_commands) const;
	//Wyrmgus end

	int DrawClip(int x, int y, const std::string &text, std::vector<std::function<void(renderer *)>> &render_commands) const
	{
		return this->DrawClip(x, y, text, true, render_commands);
	}

	int DrawClip(int x, int y, int number, std::vector<std::function<void(renderer *)>> &render_commands) const;
	/// Draw reverse text/number unclipped
	int DrawReverse(int x, int y, const char *const text, std::vector<std::function<void(renderer *)>> &render_commands) const;
	int DrawReverse(int x, int y, const std::string &text, std::vector<std::function<void(renderer *)>> &render_commands) const;
	int DrawReverse(int x, int y, int number, std::vector<std::function<void(renderer *)>> &render_commands) const;
	/// Draw reverse text/number clipped
	int DrawReverseClip(int x, int y, const char *const text, std::vector<std::function<void(renderer *)>> &render_commands) const;
	int DrawReverseClip(int x, int y, const std::string &text, std::vector<std::function<void(renderer *)>> &render_commands) const;
	int DrawReverseClip(int x, int y, int number, std::vector<std::function<void(renderer *)>> &render_commands) const;

	int DrawCentered(int x, int y, const std::string &text, std::vector<std::function<void(renderer *)>> &render_commands) const;
	int DrawReverseCentered(int x, int y, const std::string &text, std::vector<std::function<void(renderer *)>> &render_commands) const;
private:
	template <const bool CLIP>
	int DoDrawText(int x, int y, const char *const text, const size_t len, const font_color *fc, std::vector<std::function<void(renderer *)>> &render_commands) const;
private:
	const wyrmgus::font_color *normal;
	const wyrmgus::font_color *reverse;
	wyrmgus::font *font;
};
