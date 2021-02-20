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
//      (c) Copyright 2005-2006 by Fran�ois Beerten and Jimmy Salmon
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

#include <guichan.h>
#include <guichan/gsdl.h>

#include "luacallback.h"

class CGraphic;
class CPlayerColorGraphic;

extern bool GuichanActive;
extern std::unique_ptr<gcn::Gui> Gui;

void initGuichan();
void freeGuichan();
void handleInput(const SDL_Event *event);

class LuaActionListener : public gcn::ActionListener
{
	LuaCallback callback;
public:
	LuaActionListener(lua_State *lua, lua_Object function);
	virtual void action(const std::string &eventId);
	virtual ~LuaActionListener();
};

#if defined(USE_OPENGL) || defined(USE_GLES)
class MyOpenGLGraphics : public gcn::Graphics
{
public:
	virtual void _beginDraw();
	virtual void _endDraw();

	virtual void drawImage(const gcn::Image *image, int srcX, int srcY, int dstX, int dstY, int width, int height, const wyrmgus::player_color *player_color = nullptr, unsigned int transparency = 0, bool grayscale = false) const;

	virtual void drawPoint(int x, int y);
	virtual void drawLine(int x1, int y1, int x2, int y2);
	virtual void drawRectangle(const gcn::Rectangle &rectangle);
	virtual void fillRectangle(const gcn::Rectangle &rectangle);

	virtual void setColor(const gcn::Color &color) { mColor = color; }
	virtual const gcn::Color &getColor() { return mColor; }

private:
	gcn::Color mColor;
};
#endif

class ImageWidget final : public gcn::Icon
{
public:
	explicit ImageWidget(const std::string &image_path, const int scale_factor = 1, const int image_width = -1, const int image_height = -1);
	explicit ImageWidget(const std::shared_ptr<CGraphic> &graphic, const int scale_factor = 1, const int image_width = -1, const int image_height = -1);

private:
	std::shared_ptr<CGraphic> graphic;
};

class PlayerColorImageWidget : public gcn::Icon
{
public:
	explicit PlayerColorImageWidget(const std::string &image_path, const std::string &playercolor);

	virtual void draw(gcn::Graphics *graphics);
	void setImageOrigin(int x, int y) { ImageOrigin.x = x; ImageOrigin.y = y; }

	void setGrayscale(bool grayscale)
	{
		this->grayscale = grayscale;
	}

	void set_frame(const int frame);

	std::string WidgetPlayerColor;
	Vec2i ImageOrigin;
	int frame = 0;
	bool grayscale = false;
};

class ButtonWidget : public gcn::Button
{
public:
	explicit ButtonWidget(const std::string &caption);
};

class ImageButton final : public gcn::Button
{
public:
	ImageButton();
	explicit ImageButton(const std::string &caption);
	~ImageButton();

	virtual void draw(gcn::Graphics *graphics);
	virtual void adjustSize();

	void setNormalImage(const std::string &image_path);
	void setPressedImage(const std::string &image_path);
	void setDisabledImage(const std::string &image_path);
	void setIconFrameImage();
	virtual void setPosition(int x, int y);
	void setTransparency(int alpha) { Transparency = alpha; }
	void setImageOrigin(int x, int y) { ImageOrigin.x = x; ImageOrigin.y = y; }

	std::shared_ptr<CGraphic> normalImage;
	std::shared_ptr<CGraphic> pressedImage;
	std::shared_ptr<CGraphic> disabledImage;
	std::shared_ptr<CGraphic> frameImage;
	std::shared_ptr<CGraphic> pressedframeImage;
	int Transparency = 0;
	Vec2i ImageOrigin;
};

class PlayerColorImageButton final : public gcn::Button
{
public:
	PlayerColorImageButton();
	explicit PlayerColorImageButton(const std::string &caption, const std::string &playercolor);

	virtual void draw(gcn::Graphics *graphics);
	virtual void adjustSize();

	void setNormalImage(const std::string &image_path);
	void setPressedImage(const std::string &image_path);
	void setDisabledImage(const std::string &image_path);
	void setIconFrameImage();
	virtual void setPosition(int x, int y);
	void setTransparency(int alpha) { Transparency = alpha; }
	void setImageOrigin(int x, int y) { ImageOrigin.x = x; ImageOrigin.y = y; }

	void set_frame(const int frame);

	void setGrayscale(bool grayscale)
	{
		this->grayscale = grayscale;
	}

	CPlayerColorGraphic *normalImage = nullptr;
	CPlayerColorGraphic *pressedImage = nullptr;
	CPlayerColorGraphic *disabledImage = nullptr;
	std::shared_ptr<CGraphic> frameImage;
	std::shared_ptr<CGraphic> pressedframeImage;
	std::string ButtonPlayerColor = nullptr;
	int Transparency = 0;
	Vec2i ImageOrigin;
	int frame = 0;
	bool grayscale = false;
};

class ImageRadioButton : public gcn::RadioButton
{
public:
	ImageRadioButton();
	ImageRadioButton(const std::string &caption, const std::string &group,
					 bool marked);

	virtual void drawBox(gcn::Graphics *graphics);
	virtual void draw(gcn::Graphics *graphics);

	virtual void mousePress(int x, int y, int button);
	virtual void mouseRelease(int button) override;
	virtual void mouseClick(int x, int y, int button, int count);
	virtual void adjustSize();

	void setUncheckedNormalImage(const std::string &image_path);
	void setUncheckedPressedImage(const std::string &image_path);
	void setUncheckedDisabledImage(const std::string &image_path);
	void setCheckedNormalImage(const std::string &image_path);
	void setCheckedPressedImage(const std::string &image_path);
	void setCheckedDisabledImage(const std::string &image_path);

	std::shared_ptr<CGraphic> uncheckedNormalImage;
	std::shared_ptr<CGraphic> uncheckedPressedImage;
	std::shared_ptr<CGraphic> uncheckedDisabledImage;
	std::shared_ptr<CGraphic> checkedNormalImage;
	std::shared_ptr<CGraphic> checkedPressedImage;
	std::shared_ptr<CGraphic> checkedDisabledImage;
	bool mMouseDown = false;
};

class ImageCheckBox : public gcn::CheckBox
{
public:
	ImageCheckBox();
	ImageCheckBox(const std::string &caption, bool marked = false);

	virtual void draw(gcn::Graphics *graphics);
	virtual void drawBox(gcn::Graphics *graphics);

	virtual void mousePress(int x, int y, int button);
	virtual void mouseRelease(int button) override;
	virtual void mouseClick(int x, int y, int button, int count);
	virtual void adjustSize();

	void setUncheckedNormalImage(const std::string &image_path);
	void setUncheckedPressedImage(const std::string &image_path);
	void setUncheckedDisabledImage(const std::string &image_path);
	void setCheckedNormalImage(const std::string &image_path);
	void setCheckedPressedImage(const std::string &image_path);
	void setCheckedDisabledImage(const std::string &image_path);

	std::shared_ptr<CGraphic> uncheckedNormalImage;
	std::shared_ptr<CGraphic> uncheckedPressedImage;
	std::shared_ptr<CGraphic> uncheckedDisabledImage;
	std::shared_ptr<CGraphic> checkedNormalImage;
	std::shared_ptr<CGraphic> checkedPressedImage;
	std::shared_ptr<CGraphic> checkedDisabledImage;
	bool mMouseDown = false;
};

class ImageSlider : public gcn::Slider
{
public:
	explicit ImageSlider(double scaleEnd = 1.0);
	ImageSlider(double scaleStart, double scaleEnd);

	virtual void drawMarker(gcn::Graphics *graphics);
	virtual void draw(gcn::Graphics *graphics);

	void setMarkerImage(const std::string &image_path);
	void setBackgroundImage(const std::string &image_path);
	void setDisabledBackgroundImage(const std::string &image_path);

	std::shared_ptr<CGraphic> markerImage;
	std::shared_ptr<CGraphic> backgroundImage;
	std::shared_ptr<CGraphic> disabledBackgroundImage;
};

class MultiLineLabel : public gcn::Widget
{
public:
	MultiLineLabel();
	explicit MultiLineLabel(const std::string &caption);

	virtual void setCaption(const std::string &caption);
	virtual const std::string &getCaption() const;
	virtual void setAlignment(unsigned int alignment);
	virtual unsigned int getAlignment();
	virtual void setVerticalAlignment(unsigned int alignment);
	virtual unsigned int getVerticalAlignment();
	virtual void setLineWidth(int width);
	virtual int getLineWidth();
	virtual void adjustSize();
	virtual void draw(gcn::Graphics *graphics);
	virtual void drawBorder(gcn::Graphics *graphics);

	enum {
		LEFT = 0,
		CENTER,
		RIGHT,
		TOP,
		BOTTOM
	};

private:
	void wordWrap();

	std::string mCaption;
	std::vector<std::string> mTextRows;
	unsigned int mAlignment;
	unsigned int mVerticalAlignment;
	int mLineWidth;
};

class ScrollingWidget : public gcn::ScrollArea
{
public:
	ScrollingWidget(int width, int height);
	void add(gcn::Widget *widget, int x, int y);
	void restart();
	void setSpeed(float speed) { this->speedY = speed; }
	float getSpeed() const { return this->speedY; }
private:
	virtual void logic();
private:
	gcn::Container container; /// Data container
	float speedY;             /// vertical speed of the container (positive number: go up).
	float containerY;         /// Y position of the container
	bool finished;            /// True while scrolling ends.
};

class Windows : public gcn::Window
{
public:
	Windows(const std::string &text, int width, int height);
	void add(gcn::Widget *widget, int x, int y);
private:
	virtual void mouseMotion(int x, int y);
	virtual void setBackgroundColor(const gcn::Color &color);
	virtual void setBaseColor(const gcn::Color &color);
private:
	gcn::ScrollArea scroll;   /// To use scroll bar.
	gcn::Container container; /// data container.
	bool blockwholewindow;    /// Manage condition limit of moveable windows. @see mouseMotion.
	/// @todo Method to set this variable. Maybe set the variable static.
};

class ImageTextField : public gcn::TextField
{
	
public:
	ImageTextField() : TextField(), itemImage(nullptr) {}
	ImageTextField(const std::string& text) : gcn::TextField(text), itemImage(nullptr) {}
	virtual void draw(gcn::Graphics *graphics);
	virtual void drawBorder(gcn::Graphics *graphics);
	void setItemImage(CGraphic *image) { itemImage = image; }
private:
	CGraphic *itemImage;
};

class LuaListModel : public gcn::ListModel
{
	std::vector<std::string> list;
public:
	LuaListModel() {}

	void setList(lua_State *lua, lua_Object *lo);
	virtual int getNumberOfElements() {return list.size();}
	virtual std::string getElementAt(int i) {return list[i];}
};

class ImageListBox : public gcn::ListBox
{
public:
	ImageListBox();
	ImageListBox(gcn::ListModel *listModel);
	virtual void draw(gcn::Graphics *graphics);
	virtual void drawBorder(gcn::Graphics *graphics);
	void setItemImage(CGraphic *image) { itemImage = image; }
	void adjustSize();
	void mousePress(int, int y, int button);
	void setSelected(int selected);
	void setListModel(gcn::ListModel *listModel);
	void logic()
	{
		adjustSize();
	}
private:
	CGraphic *itemImage;
};

class ListBoxWidget : public gcn::ScrollArea
{
public:
	ListBoxWidget(unsigned int width, unsigned int height);
	void setList(lua_State *lua, lua_Object *lo);
	void setSelected(int i);
	int getSelected() const;
	virtual void setBackgroundColor(const gcn::Color &color);
	virtual void setFont(gcn::Font *font);
	virtual void addActionListener(gcn::ActionListener *actionListener);
private:
	void adjustSize();
private:
	LuaListModel lualistmodel;
	gcn::ListBox listbox;
};

class ImageListBoxWidget : public ListBoxWidget
{
public:
	ImageListBoxWidget(unsigned int width, unsigned int height);
	void setList(lua_State *lua, lua_Object *lo);
	void setSelected(int i);
	int getSelected() const;
	virtual void setBackgroundColor(const gcn::Color &color);
	virtual void setFont(gcn::Font *font);
	virtual void addActionListener(gcn::ActionListener *actionListener);

	void setItemImage(CGraphic *image) {
		itemImage = image;
		listbox.setItemImage(image);
	}
	void setUpButtonImage(CGraphic *image) { upButtonImage = image; }
	void setUpPressedButtonImage(CGraphic *image) { upPressedButtonImage = image; }
	void setDownButtonImage(CGraphic *image) { downButtonImage = image; }
	void setDownPressedButtonImage(CGraphic *image) { downPressedButtonImage = image; }
	void setLeftButtonImage(CGraphic *image) { leftButtonImage = image; }
	void setLeftPressedButtonImage(CGraphic *image) { leftPressedButtonImage = image; }
	void setRightButtonImage(CGraphic *image) { rightButtonImage = image; }
	void setRightPressedButtonImage(CGraphic *image) { rightPressedButtonImage = image; }
	void setHBarImage(CGraphic *image);
	void setVBarImage(CGraphic *image);
	void setMarkerImage(CGraphic *image) { markerImage = image; }

	virtual void draw(gcn::Graphics *graphics);
	virtual void drawBorder(gcn::Graphics *graphics);
	virtual gcn::Rectangle getVerticalMarkerDimension();
	virtual gcn::Rectangle getHorizontalMarkerDimension();
private:
	void adjustSize();

	void drawUpButton(gcn::Graphics *graphics);
	void drawDownButton(gcn::Graphics *graphics);
	void drawLeftButton(gcn::Graphics *graphics);
	void drawRightButton(gcn::Graphics *graphics);
	void drawUpPressedButton(gcn::Graphics *graphics);
	void drawDownPressedButton(gcn::Graphics *graphics);
	void drawLeftPressedButton(gcn::Graphics *graphics);
	void drawRightPressedButton(gcn::Graphics *graphics);
	void drawHMarker(gcn::Graphics *graphics);
	void drawVMarker(gcn::Graphics *graphics);
	void drawHBar(gcn::Graphics *graphics);
	void drawVBar(gcn::Graphics *graphics);
private:
	CGraphic *itemImage;
	CGraphic *upButtonImage;
	CGraphic *upPressedButtonImage;
	CGraphic *downButtonImage;
	CGraphic *downPressedButtonImage;
	CGraphic *leftButtonImage;
	CGraphic *leftPressedButtonImage;
	CGraphic *rightButtonImage;
	CGraphic *rightPressedButtonImage;
	CGraphic *hBarButtonImage;
	CGraphic *vBarButtonImage;
	CGraphic *markerImage;

	LuaListModel lualistmodel;
	ImageListBox listbox;
};

class DropDownWidget : public gcn::DropDown
{
	LuaListModel listmodel;
public:
	DropDownWidget() {}
	void setList(lua_State *lua, lua_Object *lo);
	virtual void setSize(int width, int height);
};

class ImageDropDownWidget : public DropDownWidget
{
public:
	ImageDropDownWidget() {
		mListBox.addActionListener(this);
		setListModel(&listmodel);
		mScrollArea->setContent(&mListBox);
	}

	void setItemImage(const std::string &image_path);
	void setDownNormalImage(const std::string &image_path);
	void setDownPressedImage(const std::string &image_path);

	virtual ImageListBox *getListBox() { return &mListBox; }
	virtual void draw(gcn::Graphics *graphics);
	virtual void drawBorder(gcn::Graphics *graphics);
	void drawButton(gcn::Graphics *graphics);
	void setList(lua_State *lua, lua_Object *lo);
	virtual void setSize(int width, int height);
	void setListModel(LuaListModel *listModel);
	int getSelected();
	void setSelected(int selected);
	void adjustHeight();
	void setListBox(ImageListBox *listBox);
	void setFont(gcn::Font *font);
	void _mouseInputMessage(const gcn::MouseInput &mouseInput);
private:
	std::shared_ptr<CGraphic> itemImage;
	std::shared_ptr<CGraphic> DownNormalImage;
	std::shared_ptr<CGraphic> DownPressedImage;
	ImageListBox mListBox;
	LuaListModel listmodel;
};

class StatBoxWidget : public gcn::Widget
{
public:
	StatBoxWidget(int width, int height);

	virtual void draw(gcn::Graphics *graphics);
	void setCaption(const std::string &s);
	const std::string &getCaption() const;
	void setPercent(const int percent);
	int getPercent() const;

private:
	std::string caption;  /// caption of the widget.
	unsigned int percent; /// percent value of the widget.
};

class MenuScreen : public gcn::Container
{
public:
	MenuScreen();

	int run(bool loop = true);
	void stop(int result = 0, bool stopAll = false);
	void stopAll(int result = 0) { stop(result, true); }
	void addLogicCallback(LuaActionListener *listener);
	virtual void draw(gcn::Graphics *graphics);
	virtual void logic();
	void setDrawMenusUnder(bool drawUnder) { this->drawUnder = drawUnder; }
	bool getDrawMenusUnder() const { return this->drawUnder; }

private:
	bool runLoop;
	int loopResult;
	gcn::Widget *oldtop;
	LuaActionListener *logiclistener;
	bool drawUnder;
	bool running;
};
