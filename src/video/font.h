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
//      (c) Copyright 1998-2020 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#pragma once

#include "color.h"
#include "guichan/font.h"

class CGraphic;

namespace stratagus {
	class font_color;
}

/// Font definition
class CFont : public gcn::Font
{
private:
	explicit CFont(const std::string &ident);

public:
	virtual ~CFont();

	static CFont *New(const std::string &ident, CGraphic *g);
	static CFont *Get(const std::string &ident);

	int Height() const;
	int Width(const std::string &text) const;
	int Width(const int number) const;
	bool IsLoaded() const;

	virtual int getHeight() const { return Height(); }
	virtual int getWidth(const std::string &text) const { return Width(text); }
	//Wyrmgus start
//	virtual void drawString(gcn::Graphics *graphics, const std::string &text, int x, int y);
	virtual void drawString(gcn::Graphics *graphics, const std::string &text, int x, int y, bool is_normal = true);
	//Wyrmgus end

	void Load();
	void Reload();
#if defined(USE_OPENGL) || defined(USE_GLES)
	void FreeOpenGL();
#endif
	void Clean();

	CGraphic *GetFontColorGraphic(const stratagus::font_color &fontColor) const;

	template<bool CLIP>
	unsigned int DrawChar(CGraphic &g, int utf8, int x, int y, const stratagus::font_color &fc) const;

	void DynamicLoad() const;

private:
#if defined(USE_OPENGL) || defined(USE_GLES)
	void make_font_color_textures();
#endif
	void MeasureWidths();

private:
	std::string Ident;    /// Ident of the font.
	char *CharWidth;      /// Real font width (starting with ' ')
	CGraphic *G;          /// Graphic object used to draw
	std::map<const stratagus::font_color *, std::unique_ptr<CGraphic>> font_color_graphics;
};

/**
**  Font selector for the font functions.
**  FIXME: should be moved to lua
*/
extern CFont &GetSmallFont();  /// Small font used in stats
extern CFont &GetGameFont();   /// Normal font used in game
extern bool IsGameFontReady(); /// true when GameFont is provided

/// Set the default text colors for normal and reverse text
extern void SetDefaultTextColors(const std::string &normal, const std::string &reverse);
/// Get the default text colors for normal and reverse text
extern void GetDefaultTextColors(std::string &normalp, std::string &reversep);
///  Return the 'line' line of the string 's'.
extern std::string GetLineFont(unsigned int line, const std::string &s, unsigned int maxlen, const CFont *font);

/// Get the hot key from a string
extern int GetHotKey(const std::string &text);

/// Load and initialize the fonts
extern void LoadFonts();

#if defined(USE_OPENGL) || defined(USE_GLES)
/// Free OpenGL fonts
extern void FreeOpenGLFonts();
/// Reload OpenGL fonts
extern void ReloadFonts();
#endif

/// Cleanup the font module
extern void CleanFonts();

class CLabel
{
public:
	explicit CLabel(const CFont &f, const std::string &nc, const std::string &rc);
	explicit CLabel(const CFont &f);

	int Height() const { return font->Height(); }

	void SetFont(const CFont &f) { font = &f; }

	void SetNormalColor(const std::string &nc);

	/// Draw text/number unclipped
	int Draw(int x, int y, const char *const text) const;
	int Draw(int x, int y, const std::string &text) const;
	int Draw(int x, int y, int number) const;
	/// Draw text/number clipped
	int DrawClip(int x, int y, const char *const text) const;
	//Wyrmgus start
//	int DrawClip(int x, int y, const std::string &text) const;
	int DrawClip(int x, int y, const std::string &text, bool is_normal = true) const;
	//Wyrmgus end
	int DrawClip(int x, int y, int number) const;
	/// Draw reverse text/number unclipped
	int DrawReverse(int x, int y, const char *const text) const;
	int DrawReverse(int x, int y, const std::string &text) const;
	int DrawReverse(int x, int y, int number) const ;
	/// Draw reverse text/number clipped
	int DrawReverseClip(int x, int y, const char *const text) const;
	int DrawReverseClip(int x, int y, const std::string &text) const;
	int DrawReverseClip(int x, int y, int number) const;

	int DrawCentered(int x, int y, const std::string &text) const;
	int DrawReverseCentered(int x, int y, const std::string &text) const;
private:
	template <const bool CLIP>
	int DoDrawText(int x, int y, const char *const text,
				   const size_t len, const stratagus::font_color *fc) const;
private:
	const stratagus::font_color *normal;
	const stratagus::font_color *reverse;
	const CFont *font;
};
