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
//      (c) Copyright 2005-2022 by Francois Beerten, Jimmy Salmon and
//                                 Andrettin
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

#include "widgets.h"

#include "database/defines.h"
#include "database/preferences.h"
#include "editor.h"
#include "engine_interface.h"
#include "game/game.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "netconnect.h"
#include "network.h"
#include "player/player_color.h"
//Wyrmgus start
#include "results.h"
//Wyrmgus end
#include "script.h"
#include "sound/sound.h"
#include "ui/cursor.h"
#include "ui/cursor_type.h"
#include "ui/ui.h"
#include "util/assert_util.h"
#include "util/colorization_type.h"
#include "util/thread_pool.h"
#include "video/font.h"
#include "video/video.h"

// Guichan stuff we need
std::unique_ptr<gcn::Gui> Gui;         /// A Gui object - binds it all together
static std::unique_ptr<gcn::SDLInput> Input;  /// Input driver

EventCallback GuichanCallbacks;

static std::stack<MenuScreen *> MenuStack;

static void MenuHandleButtonDown(unsigned, const Qt::KeyboardModifiers)
{
}

static void MenuHandleButtonUp(unsigned, const Qt::KeyboardModifiers)
{
}

static void MenuHandleMouseMove(const PixelPos &screenPos, const Qt::KeyboardModifiers key_modifiers)
{
	Q_UNUSED(key_modifiers)

	PixelPos pos(screenPos);
	HandleCursorMove(&pos.x, &pos.y);
}

static void MenuHandleKeyDown(unsigned key, unsigned keychar, const Qt::KeyboardModifiers key_modifiers)
{
	Q_UNUSED(keychar)
	Q_UNUSED(key_modifiers)

	switch (key) {
		case SDLK_SYSREQ:
		case SDLK_PRINTSCREEN:
		case SDLK_F11:
			Screenshot();
			return;
		default:
			break;
	}

	HandleKeyModifiersDown(key);
}

static void MenuHandleKeyUp(unsigned key, unsigned keychar, const Qt::KeyboardModifiers key_modifiers)
{
	Q_UNUSED(keychar)
	Q_UNUSED(key_modifiers)
	
	HandleKeyModifiersUp(key);
}

static void MenuHandleKeyRepeat(unsigned key, unsigned keychar, const Qt::KeyboardModifiers key_modifiers)
{
	Q_UNUSED(keychar)
	Q_UNUSED(key_modifiers)

	Input->processKeyRepeat();
	HandleKeyModifiersDown(key);
}

/**
**  Initializes the GUI stuff
*/
void initGuichan()
{
	std::unique_ptr<gcn::Graphics> graphics = std::make_unique<MyOpenGLGraphics>();

	Input = std::make_unique<gcn::SDLInput>();

	Gui = std::make_unique<gcn::Gui>();
	Gui->setGraphics(std::move(graphics));
	Gui->setInput(Input.get());
	Gui->setTop(nullptr);

	Gui->setUseDirtyDrawing(false);

	GuichanCallbacks.ButtonPressed = &MenuHandleButtonDown;
	GuichanCallbacks.ButtonReleased = &MenuHandleButtonUp;
	GuichanCallbacks.MouseMoved = &MenuHandleMouseMove;
	GuichanCallbacks.MouseExit = &HandleMouseExit;
	GuichanCallbacks.KeyPressed = &MenuHandleKeyDown;
	GuichanCallbacks.KeyReleased = &MenuHandleKeyUp;
	GuichanCallbacks.KeyRepeated = &MenuHandleKeyRepeat;
	GuichanCallbacks.NetworkEvent = NetworkEvent;
}

/**
**  Free all guichan infrastructure
*/
void freeGuichan()
{
	if (Gui != nullptr) {
		Gui.reset();
	}

	Input.reset();
}

/**
**  Handle input events
**
**  @param event  event to handle, null if no more events for this frame
*/
void handleInput(const SDL_Event *event)
{
	if (event) {
		if (Input) {
			try {
				Input->pushInput(*event);
			} catch (const gcn::Exception &) {
				// ignore unhandled buttons
			}
		}
	} else {
		if (Gui && Gui->getTop() != nullptr) {
			Gui->logic();
		}
	}
}

void DrawGuichanWidgets(std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (Gui && Gui->getTop() != nullptr) {
		Gui->setUseDirtyDrawing(false);
		Gui->draw(render_commands);
	}
}


/*----------------------------------------------------------------------------
--  LuaActionListener
----------------------------------------------------------------------------*/


/**
**  LuaActionListener constructor
**
**  @param l  Lua state
**  @param f  Listener function
*/
LuaActionListener::LuaActionListener(lua_State *l, lua_Object f) :
	callback(l, f)
{
}

/**
**  Called when an action is received from a Widget. It is used
**  to be able to receive a notification that an action has
**  occurred.
**
**  @param eventId  the identifier of the Widget
*/
void LuaActionListener::action(const std::string &eventId)
{
	callback.pushPreamble();
	callback.pushString(eventId.c_str());
	callback.run();
}

/**
**  LuaActionListener destructor
*/
LuaActionListener::~LuaActionListener()
{
}

/*----------------------------------------------------------------------------
--  MyOpenGLGraphics
----------------------------------------------------------------------------*/

void MyOpenGLGraphics::_beginDraw()
{
	gcn::Rectangle area(0, 0, Video.Width, Video.Height);
	pushClipArea(area);
}

void MyOpenGLGraphics::_endDraw()
{
	popClipArea();
}

void MyOpenGLGraphics::drawImage(const gcn::Image *image, int srcX, int srcY, int dstX, int dstY, int width, int height, const color_modification &color_modification, unsigned int transparency, bool grayscale, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	Q_UNUSED(transparency)

	const gcn::ClipRectangle &r = this->getCurrentClipArea();
	int right = std::min<int>(r.x + r.width - 1, Video.Width - 1);
	int bottom = std::min<int>(r.y + r.height - 1, Video.Height - 1);

	if (r.x > right || r.y > bottom) {
		return;
	}

	PushClipping();
	SetClipping(r.x, r.y, right, bottom);
	//Wyrmgus start
//	((CGraphic *)image)->DrawSubClip(srcX, srcY, width, height,
//									 dstX + mClipStack.top().xOffset, dstY + mClipStack.top().yOffset);
	if (!color_modification.is_null()) {
		((CPlayerColorGraphic *)image)->DrawPlayerColorSubClip(color_modification, srcX, srcY, width, height,
										 dstX + mClipStack.top().xOffset, dstY + mClipStack.top().yOffset, render_commands);
	} else if (grayscale) {
		((CGraphic *) image)->DrawGrayscaleSubClip(srcX, srcY, width, height,
										 dstX + mClipStack.top().xOffset, dstY + mClipStack.top().yOffset, render_commands);
	} else {
		((CGraphic *)image)->DrawSubClip(srcX, srcY, width, height,
										 dstX + mClipStack.top().xOffset, dstY + mClipStack.top().yOffset, render_commands);
	}
	//Wyrmgus end
	PopClipping();
}

void MyOpenGLGraphics::drawLine(int x1, int y1, int x2, int y2, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Color c = this->getColor();
	const PixelPos pos1(x1 + mClipStack.top().xOffset, y1 + mClipStack.top().yOffset);
	const PixelPos pos2(x2 + mClipStack.top().xOffset, y2 + mClipStack.top().yOffset);

	Video.DrawLineClip(CVideo::MapRGBA(c.r, c.g, c.b, c.a), pos1, pos2, render_commands);
}

void MyOpenGLGraphics::drawRectangle(const gcn::Rectangle &rectangle, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Color c = this->getColor();
	if (c.a == 0) {
		return;
	}

	const gcn::ClipRectangle top = mClipStack.top();
	gcn::Rectangle area = gcn::Rectangle(rectangle.x + top.xOffset,
										 rectangle.y + top.yOffset,
										 rectangle.width, rectangle.height);

	if (!area.intersect(top)) {
		return;
	}

	int x1 = std::max<int>(area.x, top.x);
	int y1 = std::max<int>(area.y, top.y);
	int x2 = std::min<int>(area.x + area.width, top.x + top.width);
	int y2 = std::min<int>(area.y + area.height, top.y + top.height);

	Video.DrawTransRectangle(CVideo::MapRGB(c.r, c.g, c.b),
							 x1, y1, x2 - x1, y2 - y1, mColor.a, render_commands);
}

void MyOpenGLGraphics::fillRectangle(const gcn::Rectangle &rectangle, std::vector<std::function<void(renderer *)>> &render_commands)
{
	const gcn::Color c = this->getColor();

	if (c.a == 0) {
		return;
	}

	const gcn::ClipRectangle top = mClipStack.top();
	gcn::Rectangle area = gcn::Rectangle(rectangle.x + top.xOffset,
										 rectangle.y + top.yOffset,
										 rectangle.width, rectangle.height);

	if (!area.intersect(top)) {
		return;
	}

	int x1 = std::max<int>(area.x, top.x);
	int y1 = std::max<int>(area.y, top.y);
	int x2 = std::min<int>(area.x + area.width, top.x + top.width);
	int y2 = std::min<int>(area.y + area.height, top.y + top.height);

	Video.FillTransRectangle(CVideo::MapRGB(c.r, c.g, c.b),
							 x1, y1, x2 - x1, y2 - y1, c.a, render_commands);
}


ImageWidget::ImageWidget(const std::string &image_path, const int scale_factor, const int image_width, const int image_height)
	: ImageWidget(CGraphic::New(image_path), centesimal_int(scale_factor), image_width, image_height)
{
}

ImageWidget::ImageWidget(const std::string &image_path)
	: ImageWidget(CGraphic::New(image_path), preferences::get()->get_scale_factor(), -1, -1)
{
}

ImageWidget::ImageWidget(const std::shared_ptr<CGraphic> &graphic, const centesimal_int &scale_factor, const int image_width, const int image_height) : gcn::Icon(graphic.get()), graphic(graphic)
{
	this->graphic->Load(scale_factor);

	if (image_width != -1 && image_height != -1) {
		this->graphic->Resize(image_width, image_height);
	}

	setHeight(this->graphic->getHeight());
	setWidth(this->graphic->getWidth());
}

/*----------------------------------------------------------------------------
--  PlayerColorImageWidget
----------------------------------------------------------------------------*/

PlayerColorImageWidget::PlayerColorImageWidget(const std::string &image_path, const std::string &playercolor)
	: gcn::Icon(CPlayerColorGraphic::Get(image_path)), WidgetPlayerColor(playercolor)
{
	ImageOrigin.x = 0;
	ImageOrigin.y = 0;

	CPlayerColorGraphic *graphic = static_cast<CPlayerColorGraphic *>(this->mImage);
	graphic->Load(preferences::get()->get_scale_factor());

	setHeight(graphic->getHeight());
	setWidth(graphic->getWidth());
}

void PlayerColorImageWidget::draw(gcn::Graphics* graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	const player_color *player_color = nullptr;
	if (!this->WidgetPlayerColor.empty()) {
		player_color = player_color::get(WidgetPlayerColor);
	}
	
	graphics->drawImage(mImage, ImageOrigin.x, ImageOrigin.y, 0, 0, mImage->getWidth(), mImage->getHeight(), color_modification(0, colorization_type::none, color_set(), player_color), 0, this->grayscale, render_commands);
}

void PlayerColorImageWidget::set_frame(const int frame)
{
	this->frame = frame;

	if (this->mImage != nullptr) {
		CPlayerColorGraphic *graphic = static_cast<CPlayerColorGraphic *>(this->mImage);

		const int x_origin = (this->frame * graphic->get_frame_width()) % graphic->get_width();
		const int y_origin = this->frame * graphic->get_frame_width() / graphic->get_width() * graphic->get_frame_height();
		this->setImageOrigin(x_origin, y_origin);
		this->setSize(graphic->get_frame_width(), graphic->get_frame_height());
	}
}

ButtonWidget::ButtonWidget(const std::string &caption) : Button(caption)
{
	//Wyrmgus start
//		this->setHotKey(GetHotKey(caption));
	if (!caption.empty()) {
		this->setHotKey(GetHotKey(caption));
	}
	//Wyrmgus end
}

ImageButton::ImageButton() : Button()
{
	setForegroundColor(0xffffff);
	//Wyrmgus start
	ImageOrigin.x = 0;
	ImageOrigin.y = 0;
	//Wyrmgus end
}

/**
**  ImageButton constructor
**
**  @param caption  Caption text
*/
ImageButton::ImageButton(const std::string &caption) : Button(caption)
{
	setForegroundColor(0xffffff);
	//Wyrmgus start
	ImageOrigin.x = 0;
	ImageOrigin.y = 0;
	//Wyrmgus end
}

ImageButton::~ImageButton()
{
}

/**
**  Draw the image button
**
**  @param graphics  Graphics object to draw with
*/
void ImageButton::draw(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (!normalImage) {
		Button::draw(graphics, render_commands);
		return;
	}

	std::shared_ptr<CGraphic> img;

	if (!isEnabled()) {
		img = disabledImage ? disabledImage : normalImage;
	} else if (isPressed()) {
		img = pressedImage ? pressedImage : normalImage;
	} else {
		img = normalImage;
	}
	
	//Wyrmgus start
//	graphics->drawImage(img, 0, 0, 0, 0,
//						img->getWidth(), img->getHeight());

	if (frameImage) {
        graphics->setColor(ColorBlack);
		graphics->fillRectangle(gcn::Rectangle((frameImage->getWidth() - img->getWidth()) / 2, (frameImage->getHeight() - img->getHeight()) / 2, img->getWidth(), img->getHeight()), render_commands);
		graphics->drawImage(frameImage.get(), 0, 0, 0, 0,
							frameImage->getWidth(), frameImage->getHeight(), render_commands);
		if (isPressed()) {
			if (Transparency) {
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				glColor4ub(255, 255, 255, int(256 - 2.56 * Transparency));
			}
			graphics->drawImage(img.get(), ImageOrigin.x, ImageOrigin.y, ((frameImage->getWidth() - img->getWidth()) / 2) + 1, ((frameImage->getHeight() - img->getHeight()) / 2) + 1,
								img->getWidth() - 1, img->getHeight() - 1, render_commands);
			if (Transparency) {
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			}
			if (pressedframeImage) {
				graphics->drawImage(pressedframeImage.get(), 0, 0, 0, 0,
									pressedframeImage->getWidth(), pressedframeImage->getHeight(), render_commands);
			}
		} else {
			if (Transparency) {
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				glColor4ub(255, 255, 255, int(256 - 2.56 * Transparency));
			}
			graphics->drawImage(img.get(), ImageOrigin.x, ImageOrigin.y, (frameImage->getWidth() - img->getWidth()) / 2, (frameImage->getHeight() - img->getHeight()) / 2,
								img->getWidth(), img->getHeight(), render_commands);
			if (Transparency) {
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			}
		}
	} else {
		if (Transparency) {
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glColor4ub(255, 255, 255, int(256 - 2.56 * Transparency));
		}
		graphics->drawImage(img.get(), ImageOrigin.x, ImageOrigin.y, 0, 0,
							img->getWidth(), img->getHeight(), render_commands);
		if (Transparency) {
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		}
	}
	//Wyrmgus end

	graphics->setColor(getForegroundColor());

	int textX;
	int textY = getHeight() / 2 - getFont()->getHeight() / 2;

	switch (getAlignment()) {
		case gcn::Graphics::LEFT:
			textX = 4;
			break;
		case gcn::Graphics::CENTER:
			textX = getWidth() / 2;
			break;
		case gcn::Graphics::RIGHT:
			textX = getWidth() - 4;
			break;
		default:
			textX = 0;
			throw std::runtime_error("Unknown alignment.");
	}

	graphics->setFont(getFont());
	//Wyrmgus start
	/*
	if (isPressed()) {
		graphics->drawText(getCaption(), textX + 4, textY + 4, getAlignment());
	} else {
		graphics->drawText(getCaption(), textX + 2, textY + 2, getAlignment());
	}
	*/
	bool is_normal = true;
	if (hasMouse()) {
		is_normal = false;
	}
	if (isPressed()) {
		textX += 4;
		textY += 4;
		if ((textY + 11) > (getHeight() - 2)) {
			textY += (getHeight() - 2) - (textY + 11);
		}
		if (textY < 1) {
			textY = 1;
		}
	} else {
		textX += 2;
		textY += 2;
		if ((textY + 11) > (getHeight() - 3)) {
			textY += (getHeight() - 3) - (textY + 11);
		}
		if (textY < 0) {
			textY = 0;
		}
	}
	graphics->drawText(getCaption(), textX, textY, getAlignment(), is_normal, render_commands);
	//Wyrmgus end

	//Wyrmgus start
//	if (hasFocus()) {
	if (isPressed() && !frameImage) {
	//Wyrmgus end
		//Wyrmgus start
//		graphics->drawRectangle(gcn::Rectangle(0, 0, getWidth(), getHeight()));
		if (getWidth() == getHeight() && getWidth() > 64 && getHeight() > 64) {
			graphics->drawRectangle(gcn::Rectangle(0 + ((getWidth() - 64) / 2), 0 + ((getHeight() - 64) / 2), getWidth() - (getWidth() - 64), getHeight() - (getHeight() - 64)), render_commands); //hack to make it appear properly in grand strategy mode
		} else {
			graphics->drawRectangle(gcn::Rectangle(0, 0, getWidth(), getHeight()), render_commands);
		}
		//Wyrmgus end
	}
}

/**
**  Automatically adjust the size of an image button
*/
void ImageButton::adjustSize()
{
	//Wyrmgus start
//	if (normalImage) {
	if (frameImage) {
		setWidth(frameImage->getWidth());
		setHeight(frameImage->getHeight());
		setPosition(getX(), getY()); //reset position, to make it appropriate for the frame image
	} else if (normalImage) {
	//Wyrmgus end
		setWidth(normalImage->getWidth());
		setHeight(normalImage->getHeight());
	} else {
		Button::adjustSize();
	}
}

void ImageButton::setNormalImage(const std::string &image_path)
{ 
	normalImage = CGraphic::New(image_path); 
	normalImage->Load(preferences::get()->get_scale_factor());
	adjustSize();
}

void ImageButton::setPressedImage(const std::string &image_path) 
{ 
	pressedImage = CGraphic::New(image_path);
	pressedImage->Load(preferences::get()->get_scale_factor());
}

void ImageButton::setDisabledImage(const std::string &image_path) 
{ 
	disabledImage = CGraphic::New(image_path);
	disabledImage->Load(preferences::get()->get_scale_factor());
}

void ImageButton::setIconFrameImage()
{
	this->frameImage = defines::get()->get_icon_frame_graphics();
	this->pressedframeImage = defines::get()->get_pressed_icon_frame_graphics();
	adjustSize();
}

//Wyrmgus start
void ImageButton::setPosition(int x, int y)
{
	if (frameImage) {
		mDimension.x = x - ((frameImage->getWidth() - normalImage->getWidth()) / 2);
		mDimension.y = y - ((frameImage->getHeight() - normalImage->getWidth()) / 2);
	} else {
		mDimension.x = x;
		mDimension.y = y;
	}
}

/*----------------------------------------------------------------------------
--  PlayerColorImageButtonImageButton
----------------------------------------------------------------------------*/


/**
**  PlayerColorImageButton constructor
*/
PlayerColorImageButton::PlayerColorImageButton() : Button()
{
	setForegroundColor(0xffffff);
	ImageOrigin.x = 0;
	ImageOrigin.y = 0;
}

/**
**  PlayerColorImageButton constructor
**
**  @param caption  Caption text
*/
PlayerColorImageButton::PlayerColorImageButton(const std::string &caption, const std::string &playercolor)
	: Button(caption), ButtonPlayerColor(playercolor)
{
	setForegroundColor(0xffffff);
	ImageOrigin.x = 0;
	ImageOrigin.y = 0;
}

/**
**  Draw the image button
**
**  @param graphics  Graphics object to draw with
*/
void PlayerColorImageButton::draw(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (!normalImage) {
		Button::draw(graphics, render_commands);
		return;
	}

	gcn::Image *img;

	if (!isEnabled()) {
		img = disabledImage ? disabledImage : normalImage;
	} else if (isPressed()) {
		img = pressedImage ? pressedImage : normalImage;
	} else {
		img = normalImage;
	}

	const wyrmgus::player_color *player_color = nullptr;
	if (!this->ButtonPlayerColor.empty()) {
		player_color = wyrmgus::player_color::get(ButtonPlayerColor);
	}
		
	if (frameImage) {
        graphics->setColor(ColorBlack);
		graphics->fillRectangle(gcn::Rectangle((frameImage->getWidth() - img->getWidth()) / 2, (frameImage->getHeight() - img->getHeight()) / 2, img->getWidth(), img->getHeight()), render_commands);
		graphics->drawImage(frameImage.get(), 0, 0, 0, 0,
							frameImage->getWidth(), frameImage->getHeight(), render_commands);
		if (isPressed()) {
			if (Transparency) {
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				glColor4ub(255, 255, 255, int(256 - 2.56 * Transparency));
			}
			graphics->drawImage(img, ImageOrigin.x, ImageOrigin.y, ((frameImage->getWidth() - img->getWidth()) / 2) + 1, ((frameImage->getHeight() - img->getHeight()) / 2) + 1,
								img->getWidth() - 1, img->getHeight() - 1, color_modification(0, colorization_type::none, color_set(), player_color), Transparency, this->grayscale, render_commands);
			if (Transparency) {
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			}
			if (pressedframeImage) {
				graphics->drawImage(pressedframeImage.get(), 0, 0, 0, 0,
									pressedframeImage->getWidth(), pressedframeImage->getHeight(), render_commands);
			}
		} else {
			if (Transparency) {
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				glColor4ub(255, 255, 255, int(256 - 2.56 * Transparency));
			}
			graphics->drawImage(img, ImageOrigin.x, ImageOrigin.y, (frameImage->getWidth() - img->getWidth()) / 2, (frameImage->getHeight() - img->getHeight()) / 2,
								img->getWidth(), img->getHeight(), color_modification(0, colorization_type::none, color_set(), player_color), Transparency, this->grayscale, render_commands);
			if (Transparency) {
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			}
		}
	} else {
		if (Transparency) {
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glColor4ub(255, 255, 255, int(256 - 2.56 * Transparency));
		}
		graphics->drawImage(img, ImageOrigin.x, ImageOrigin.y, 0, 0,
							img->getWidth(), img->getHeight(), color_modification(0, colorization_type::none, color_set(), player_color), Transparency, this->grayscale, render_commands);
		if (Transparency) {
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		}
	}

	graphics->setColor(getForegroundColor());

	int textX;
	int textY = getHeight() / 2 - getFont()->getHeight() / 2;

	switch (getAlignment()) {
		case gcn::Graphics::LEFT:
			textX = 4;
			break;
		case gcn::Graphics::CENTER:
			textX = getWidth() / 2;
			break;
		case gcn::Graphics::RIGHT:
			textX = getWidth() - 4;
			break;
		default:
			textX = 0;
			throw std::runtime_error("Unknown alignment.");
	}

	graphics->setFont(getFont());
	bool is_normal = true;
	if (hasMouse()) {
		is_normal = false;
	}
	if (isPressed()) {
		graphics->drawText(getCaption(), textX + 4, textY + 4, getAlignment(), is_normal, render_commands);
	} else {
		graphics->drawText(getCaption(), textX + 2, textY + 2, getAlignment(), is_normal, render_commands);
	}

	if (isPressed() && !frameImage) {
//		graphics->drawRectangle(gcn::Rectangle(0, 0, getWidth(), getHeight()));
		if (getWidth() == getHeight() && getWidth() > 64 && getHeight() > 64) {
			graphics->drawRectangle(gcn::Rectangle(0 + ((getWidth() - 64) / 2), 0 + ((getHeight() - 64) / 2), getWidth() - (getWidth() - 64), getHeight() - (getHeight() - 64)), render_commands); //hack to make it appear properly in grand strategy mode
		} else {
			graphics->drawRectangle(gcn::Rectangle(0, 0, getWidth(), getHeight()), render_commands);
		}
	}
}

/**
**  Automatically adjust the size of an image button
*/
void PlayerColorImageButton::adjustSize()
{
	if (frameImage) {
		setWidth(frameImage->getWidth());
		setHeight(frameImage->getHeight());
		setPosition(getX(), getY()); //reset position, to make it appropriate for the frame image
	} else if (normalImage) {
		setWidth(normalImage->getWidth());
		setHeight(normalImage->getHeight());
	} else {
		Button::adjustSize();
	}
}

void PlayerColorImageButton::setNormalImage(const std::string &image_path)
{
	normalImage = CPlayerColorGraphic::Get(image_path);
	normalImage->Load(preferences::get()->get_scale_factor());
	adjustSize();
}

void PlayerColorImageButton::setPressedImage(const std::string &image_path)
{
	pressedImage = CPlayerColorGraphic::Get(image_path);
	pressedImage->Load(preferences::get()->get_scale_factor());
}

void PlayerColorImageButton::setDisabledImage(const std::string &image_path)
{
	disabledImage = CPlayerColorGraphic::Get(image_path);
	disabledImage->Load(preferences::get()->get_scale_factor());
}


void PlayerColorImageButton::setIconFrameImage()
{
	this->frameImage = defines::get()->get_icon_frame_graphics();
	this->pressedframeImage = defines::get()->get_pressed_icon_frame_graphics();
	adjustSize();
}

void PlayerColorImageButton::setPosition(int x, int y)
{
	if (frameImage) {
		mDimension.x = x - ((frameImage->getWidth() - normalImage->getWidth()) / 2);
		mDimension.y = y - ((frameImage->getHeight() - normalImage->getWidth()) / 2);
	} else {
		mDimension.x = x;
		mDimension.y = y;
	}
}
//Wyrmgus end

void PlayerColorImageButton::set_frame(const int frame)
{
	this->frame = frame;

	if (this->normalImage != nullptr) {
		const int x_origin = (this->frame * this->normalImage->get_frame_width()) % this->normalImage->get_width();
		const int y_origin = this->frame * this->normalImage->get_frame_width() / this->normalImage->get_width() * this->normalImage->get_frame_height();
		this->setImageOrigin(x_origin, y_origin);
		this->setSize(this->normalImage->get_frame_width(), this->normalImage->get_frame_height());
	}
}

/*----------------------------------------------------------------------------
--  ImageCheckbox
----------------------------------------------------------------------------*/

/**
**  Image checkbox constructor
*/
ImageCheckBox::ImageCheckBox() : gcn::CheckBox()
{
}

/**
**  Image checkbox constructor
*/
ImageCheckBox::ImageCheckBox(const std::string &caption, bool marked) :
	gcn::CheckBox(caption, marked)
{
}

/**
**  Draw the image checkbox
*/
void ImageCheckBox::draw(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	drawBox(graphics, render_commands);

	graphics->setFont(getFont());
	graphics->setColor(getForegroundColor());

	int width;
	if (uncheckedNormalImage) {
		width = uncheckedNormalImage->getWidth();
		width += width / 2;
	} else {
		width = getHeight();
		width += width / 2;
	}

	graphics->drawText(getCaption(), width - 2, 0, render_commands);

	if (hasFocus()) {
		graphics->drawRectangle(gcn::Rectangle(width - 4, 0, getWidth() - width + 3, getHeight()), render_commands);
	}
}

/**
**  Draw the checkbox (not the caption)
*/
void ImageCheckBox::drawBox(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	std::shared_ptr<CGraphic> img;

	if (mMarked) {
		if (isEnabled() == false) {
			img = checkedDisabledImage;
		} else if (mMouseDown) {
			img = checkedPressedImage;
		} else {
			img = checkedNormalImage;
		}
	} else {
		if (isEnabled() == false) {
			img = uncheckedDisabledImage;
		} else if (mMouseDown) {
			img = uncheckedPressedImage;
		} else {
			img = uncheckedNormalImage;
		}
	}

	if (img) {
		graphics->drawImage(img.get(), 0, 0, 0, (getHeight() - img->getHeight()) / 2,
							img->getWidth(), img->getHeight(), render_commands);
	} else {
		CheckBox::drawBox(graphics, render_commands);
	}
}

/**
**  Mouse button pressed callback
*/
void ImageCheckBox::mousePress(int, int, int button)
{
	if (button == gcn::MouseInput::LEFT && hasMouse()) {
		mMouseDown = true;
	}
}

/**
**  Mouse button released callback
*/
void ImageCheckBox::mouseRelease(int button)
{
	if (button == gcn::MouseInput::LEFT) {
		mMouseDown = false;
	}
}

/**
**  Mouse clicked callback
*/
void ImageCheckBox::mouseClick(int, int, int button, int)
{
	if (button == gcn::MouseInput::LEFT) {
		toggle();
	}
}

/**
**  Adjusts the size to fit the image and font size
*/
void ImageCheckBox::adjustSize()
{
	int width, height;

	height = getFont()->getHeight();
	if (uncheckedNormalImage) {
		width = uncheckedNormalImage->getWidth();
		width += width / 2;
		height = std::max(height, uncheckedNormalImage->getHeight());
	} else {
		width = getFont()->getHeight();
		width += width / 2;
	}

	setHeight(height);
	setWidth(getFont()->getWidth(mCaption) + width);
}


void ImageCheckBox::setUncheckedNormalImage(const std::string &image_path)
{
	uncheckedNormalImage = CGraphic::New(image_path);
	uncheckedNormalImage->Load(preferences::get()->get_scale_factor());
}

void ImageCheckBox::setUncheckedPressedImage(const std::string &image_path)
{
	uncheckedPressedImage = CGraphic::New(image_path);
	uncheckedPressedImage->Load(preferences::get()->get_scale_factor());
}

void ImageCheckBox::setUncheckedDisabledImage(const std::string &image_path)
{
	uncheckedDisabledImage = CGraphic::New(image_path);
	uncheckedDisabledImage->Load(preferences::get()->get_scale_factor());
}

void ImageCheckBox::setCheckedNormalImage(const std::string &image_path)
{
	checkedNormalImage = CGraphic::New(image_path);
	checkedNormalImage->Load(preferences::get()->get_scale_factor());
}

void ImageCheckBox::setCheckedPressedImage(const std::string &image_path)
{
	checkedPressedImage = CGraphic::New(image_path);
	checkedPressedImage->Load(preferences::get()->get_scale_factor());
}

void ImageCheckBox::setCheckedDisabledImage(const std::string &image_path)
{
	checkedDisabledImage = CGraphic::New(image_path);
	checkedDisabledImage->Load(preferences::get()->get_scale_factor());
}

/*----------------------------------------------------------------------------
--  MultiLineLabel
----------------------------------------------------------------------------*/


/**
**  MultiLineLabel constructor
*/
MultiLineLabel::MultiLineLabel()
{
	this->mAlignment = LEFT;
	this->mVerticalAlignment = TOP;
	this->mLineWidth = 0;
}

/**
**  MultiLineLabel constructor
*/
MultiLineLabel::MultiLineLabel(const std::string &caption)
{
	this->mCaption = caption;
	this->mAlignment = LEFT;
	this->mVerticalAlignment = TOP;

	this->mLineWidth = 999999;
	this->wordWrap();
	this->adjustSize();
}

/**
**  Set the caption
*/
void MultiLineLabel::setCaption(const std::string &caption)
{
	this->mCaption = caption;
	this->wordWrap();
	this->setDirty(true);
}

/**
**  Get the caption
*/
const std::string &MultiLineLabel::getCaption() const
{
	return this->mCaption;
}

/**
**  Set the horizontal alignment
*/
void MultiLineLabel::setAlignment(unsigned int alignment)
{
	this->mAlignment = alignment;
}

/**
**  Get the horizontal alignment
*/
unsigned int MultiLineLabel::getAlignment()
{
	return this->mAlignment;
}

/**
**  Set the vertical alignment
*/
void MultiLineLabel::setVerticalAlignment(unsigned int alignment)
{
	this->mVerticalAlignment = alignment;
}

/**
**  Get the vertical alignment
*/
unsigned int MultiLineLabel::getVerticalAlignment()
{
	return this->mVerticalAlignment;
}

/**
**  Set the line width
*/
void MultiLineLabel::setLineWidth(int width)
{
	this->mLineWidth = width;
	this->wordWrap();
}

/**
**  Get the line width
*/
int MultiLineLabel::getLineWidth()
{
	return this->mLineWidth;
}

/**
**  Adjust the size
*/
void MultiLineLabel::adjustSize()
{
	int width = 0;
	for (int i = 0; i < (int)this->mTextRows.size(); ++i) {
		int w = this->getFont()->getWidth(this->mTextRows[i]);
		if (width < w) {
			width = std::min(w, this->mLineWidth);
		}
	}
	this->setWidth(width);
	this->setHeight(this->getFont()->getHeight() * this->mTextRows.size());
}

/**
**  Draw the label
*/
void MultiLineLabel::draw(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	graphics->setFont(getFont());
	graphics->setColor(getForegroundColor());

	int textX, textY;
	switch (this->getAlignment()) {
		case LEFT:
			textX = 0;
			break;
		case CENTER:
			textX = this->getWidth() / 2;
			break;
		case RIGHT:
			textX = this->getWidth();
			break;
		default:
			textX = 0;
			throw std::runtime_error("Unknown alignment.");
	}
	switch (this->getVerticalAlignment()) {
		case TOP:
			textY = 0;
			break;
		case CENTER:
			textY = (this->getHeight() - (int)this->mTextRows.size() * this->getFont()->getHeight()) / 2;
			break;
		case BOTTOM:
			textY = this->getHeight() - (int)this->mTextRows.size() * this->getFont()->getHeight();
			break;
		default:
			textY = 0;
			throw std::runtime_error("Unknown alignment.");
	}

	for (int i = 0; i < (int)this->mTextRows.size(); ++i) {
		graphics->drawText(this->mTextRows[i], textX, textY + i * this->getFont()->getHeight(),
						   this->getAlignment(), true, render_commands);
	}
}

/**
**  Draw the border
*/
void MultiLineLabel::drawBorder(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Color faceColor = getBaseColor();
	gcn::Color highlightColor, shadowColor;
	int alpha = getBaseColor().a;
	int width = getWidth() + getBorderSize() * 2 - 1;
	int height = getHeight() + getBorderSize() * 2 - 1;
	highlightColor = faceColor + 0x303030;
	highlightColor.a = alpha;
	shadowColor = faceColor - 0x303030;
	shadowColor.a = alpha;

	for (unsigned int i = 0; i < getBorderSize(); ++i) {
		graphics->setColor(shadowColor);
		graphics->drawLine(i, i, width - i, i, render_commands);
		graphics->drawLine(i, i + 1, i, height - i - 1, render_commands);
		graphics->setColor(highlightColor);
		graphics->drawLine(width - i, i + 1, width - i, height - i, render_commands);
		graphics->drawLine(i, height - i, width - i - 1, height - i, render_commands);
	}
}

/**
**  Do word wrap
*/
void MultiLineLabel::wordWrap()
{
	gcn::Font *font = this->getFont();
	int lineWidth = this->getLineWidth();
	std::string str = this->getCaption();
	size_t pos, lastPos;
	std::string substr;
	bool done = false;
	bool first = true;

	this->mTextRows.clear();

	while (!done) {
		if (str.find('\n') != std::string::npos || font->getWidth(str) > lineWidth) {
			// string too wide or has a newline, split it up
			first = true;
			lastPos = 0;
			while (1) {
				// look for any whitespace
				pos = str.find_first_of(" \t\n", first ? 0 : lastPos + 1);
				if (pos != std::string::npos) {
					// found space, now check width
					substr = str.substr(0, pos);
					if (font->getWidth(substr) > lineWidth) {
						// sub-string is too big, use last good position
						if (first) {
							// didn't find a good last position
							substr = str.substr(0, pos);
							this->mTextRows.push_back(substr);
							str = str.substr(pos + 1);
							break;
						} else {
							substr = str.substr(0, lastPos);
							this->mTextRows.push_back(substr);
							// If we stopped at a space then skip any extra spaces but stop at a newline
							if (str[lastPos] != '\n') {
								while (str[lastPos + 1] == ' ' || str[lastPos + 1] == '\t' || str[lastPos + 1] == '\n') {
									++lastPos;
									if (str[lastPos] == '\n') {
										break;
									}
								}
							}
							str = str.substr(lastPos + 1);
							break;
						}
					} else {
						// sub-string is small enough
						// stop if we found a newline, otherwise look for next space
						if (str[pos] == '\n') {
							substr = str.substr(0, pos);
							this->mTextRows.push_back(substr);
							str = str.substr(pos + 1);
							break;
						}
					}
				} else {
					// no space found
					if (first) {
						// didn't find a good last position, we're done
						this->mTextRows.push_back(str);
						done = true;
						break;
					} else {
						substr = str.substr(0, lastPos);
						this->mTextRows.push_back(substr);
						str = str.substr(lastPos + 1);
						break;
					}
				}
				lastPos = pos;
				first = false;
			}
		} else {
			// string small enough
			this->mTextRows.push_back(str);
			done = true;
		}
	}
}


/*----------------------------------------------------------------------------
--  ScrollingWidget
----------------------------------------------------------------------------*/


/**
**  ScrollingWidget constructor.
**
**  @param width   Width of the widget.
**  @param height  Height of the widget.
*/
ScrollingWidget::ScrollingWidget(int width, int height) :
	gcn::ScrollArea(nullptr, gcn::ScrollArea::SHOW_NEVER, gcn::ScrollArea::SHOW_NEVER),
	speedY(1.f), containerY(0.f), finished(false)
{
	container.setDimension(gcn::Rectangle(0, 0, width, height));
	container.setOpaque(false);
	setContent(&container);
	setDimension(gcn::Rectangle(0, 0, width, height));
}

/**
**  Add a widget in the window.
**
**  @param widget  Widget to add.
**  @param x       Position of the widget in the window.
**  @param y       Position of the widget in the window.
*/
void ScrollingWidget::add(gcn::Widget *widget, int x, int y)
{
	container.add(widget, x, y);
	if (x + widget->getWidth() > container.getWidth()) {
		container.setWidth(x + widget->getWidth());
	}
	if (y + widget->getHeight() > container.getHeight()) {
		container.setHeight(y + widget->getHeight());
	}
}

/**
**  Scrolling the content when possible.
*/
void ScrollingWidget::logic()
{
	setDirty(true);
	if (container.getHeight() + containerY - speedY > 0) {
		// the bottom of the container is lower than the top
		// of the widget. It is thus still visible.
		containerY -= speedY;
		container.setY((int)containerY);
	} else if (!finished) {
		finished = true;
		generateAction();
	}
}

/**
**  Restart animation to the beginning.
*/
void ScrollingWidget::restart()
{
	container.setY(0);
	containerY = 0.f;
	finished = (container.getHeight() == getHeight());
}

/*----------------------------------------------------------------------------
--  ImageTextField
----------------------------------------------------------------------------*/

void ImageTextField::draw(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Font *font;
	int x, y;
	CGraphic *img = this->itemImage;
	if (!img) {
		throw std::runtime_error("Not all graphics for ImageTextField were set.");
	}
	img->Resize(getWidth(), img->getHeight());
	graphics->drawImage(img, 0, 0, 0, 0, getWidth(), img->getHeight(), render_commands);

	if (hasFocus())
	{
		drawCaret(graphics, getFont()->getWidth(mText.substr(0, mCaretPosition)) - mXScroll, render_commands);
	}

	graphics->setColor(getForegroundColor());
	font = getFont();
	graphics->setFont(font);

	x = 1 - mXScroll;
	y = 1;

	if (mSelectEndOffset != 0)
	{
		unsigned int first;
		unsigned int len;
		int selX;
		int selW;
		std::string tmpStr;

		getTextSelectionPositions(&first, &len);

		tmpStr = std::string(mText.substr(0, first));
		selX = font->getWidth(tmpStr);

		tmpStr = std::string(mText.substr(first, len));
		selW = font->getWidth(tmpStr);

		graphics->setColor(gcn::Color(127, 127, 127));
		graphics->fillRectangle(gcn::Rectangle(x + selX, y, selW, font->getHeight()), render_commands);
	}

	graphics->drawText(mText, x, y, render_commands);
}

void ImageTextField::drawBorder(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Color faceColor = getBaseColor();
	gcn::Color highlightColor, shadowColor;
	int alpha = getBaseColor().a;
	int width = getWidth() + getBorderSize() * 2 - 1;
	int height = getHeight() + getBorderSize() * 2 - 1;
	height = itemImage ? std::max<int>(height, itemImage->getHeight()) : height;
	highlightColor = faceColor + 0x303030;
	highlightColor.a = alpha;
	shadowColor = faceColor - 0x303030;
	shadowColor.a = alpha;

	unsigned int i;
	for (i = 0; i < getBorderSize(); ++i)
	{
		graphics->setColor(shadowColor);
		graphics->drawLine(i,i, width - i, i, render_commands);
		graphics->drawLine(i,i + 1, i, height - i - 1, render_commands);
		graphics->setColor(highlightColor);
		graphics->drawLine(width - i,i + 1, width - i, height - i, render_commands);
		graphics->drawLine(i,height - i, width - i - 1, height - i, render_commands);
	}
}

/*----------------------------------------------------------------------------
--  LuaListModel
----------------------------------------------------------------------------*/

/**
**  Set the list
*/
void LuaListModel::setList(lua_State *lua, lua_Object *lo)
{
	list.clear();

	const int args = lua_rawlen(lua, *lo);
	for (int j = 0; j < args; ++j) {
		list.push_back(std::string(LuaToString(lua, *lo, j + 1)));
	}
}

/*----------------------------------------------------------------------------
--  ImageListBox
----------------------------------------------------------------------------*/

ImageListBox::ImageListBox() : gcn::ListBox(), itemImage(nullptr)
{
}

ImageListBox::ImageListBox(gcn::ListModel *listModel) : gcn::ListBox(listModel), itemImage(nullptr)
{
}

void ImageListBox::draw(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (mListModel == nullptr) {
		return;
	}

	graphics->setColor(getForegroundColor());
	graphics->setFont(getFont());

	int i, fontHeight;
	int y = 0;
	CGraphic *img = itemImage;
	//Wyrmgus start
//	img->Resize(getWidth(), img->getHeight());
	//Wyrmgus end

	fontHeight = std::max<int>(getFont()->getHeight(), img->getHeight());

    /**
        * @todo Check cliprects so we do not have to iterate over elements in the list model
        */
	for (i = 0; i < mListModel->getNumberOfElements(); ++i) {
		graphics->drawImage(img, 0, 0, 0, y, getWidth(), img->getHeight(), render_commands);
		if (i == mSelected) {
			graphics->drawText("~<" + mListModel->getElementAt(i) + "~>", 1, y + (fontHeight - getFont()->getHeight()) / 2, render_commands);
		} else {
			graphics->drawText(mListModel->getElementAt(i), 1, y + (fontHeight - getFont()->getHeight()) / 2, render_commands);
		}

		y += fontHeight;
	}
	//img->SetOriginalSize();
}

void ImageListBox::drawBorder(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Color faceColor = getBaseColor();
	gcn::Color highlightColor, shadowColor;
	int alpha = getBaseColor().a;
	int width = getWidth() + getBorderSize() * 2 - 1;
	int height = getHeight() + getBorderSize() * 2 - 1;
	highlightColor = faceColor + 0x303030;
	highlightColor.a = alpha;
	shadowColor = faceColor - 0x303030;
	shadowColor.a = alpha;

	unsigned int i;
	for (i = 0; i < getBorderSize(); ++i)
	{
		graphics->setColor(shadowColor);
		graphics->drawLine(i,i, width - i, i, render_commands);
		graphics->drawLine(i,i + 1, i, height - i - 1, render_commands);
		graphics->setColor(highlightColor);
		graphics->drawLine(width - i,i + 1, width - i, height - i, render_commands);
		graphics->drawLine(i,height - i, width - i - 1, height - i, render_commands);
	}
}

void ImageListBox::adjustSize()
{
	if (mListModel != nullptr)
	{
		setHeight((itemImage ? std::max<int>(getFont()->getHeight(), itemImage->getHeight()) : getFont()->getHeight()) * mListModel->getNumberOfElements());
	}
}

void ImageListBox::mousePress(int, int y, int button)
{
	if (button == gcn::MouseInput::LEFT && hasMouse())
	{
		setSelected(y / (itemImage ? std::max<int>(getFont()->getHeight(), itemImage->getHeight()) : getFont()->getHeight()));
		generateAction();
	}
}

void ImageListBox::setSelected(int selected)
{
	if (mListModel == nullptr)
	{
		mSelected = -1;
	}
	else
	{
		if (selected < 0)
		{
			mSelected = -1;
		}
		else if (selected >= mListModel->getNumberOfElements())
		{
			mSelected = mListModel->getNumberOfElements() - 1;
		}
		else
		{
			mSelected = selected;
		}

		Widget *par = getParent();
		if (par == nullptr)
		{
			return;
		}

		gcn::ScrollArea* scrollArea = dynamic_cast<gcn::ScrollArea *>(par);
		if (scrollArea != nullptr)
		{
			gcn::Rectangle scroll;
			scroll.y = (itemImage ? std::max<int>(getFont()->getHeight(), itemImage->getHeight()) : getFont()->getHeight()) * mSelected;
			scroll.height = (itemImage ? std::max<int>(getFont()->getHeight(), itemImage->getHeight()) : getFont()->getHeight());
			scrollArea->scrollToRectangle(scroll);
		}
	}
}

void ImageListBox::setListModel(gcn::ListModel *listModel)
{
	mSelected = -1;
	mListModel = listModel;
	adjustSize();
}


/*----------------------------------------------------------------------------
--  ListBoxWidget
----------------------------------------------------------------------------*/


/**
**  ListBoxWidget constructor.
**
**  @todo  Size should be parametrable, maybe remove default constructor?
*/
ListBoxWidget::ListBoxWidget(unsigned int width, unsigned int height)
{
	setDimension(gcn::Rectangle(0, 0, width, height));
	setContent(&listbox);
	setBackgroundColor(gcn::Color(128, 128, 128));
}

/**
**  ImageListBoxWidget constructor.
**
**  @todo  Size should be parametrable, maybe remove default constructor?
*/
ImageListBoxWidget::ImageListBoxWidget(unsigned int width, unsigned int height) : ListBoxWidget(width, height),
	upButtonImage(nullptr), downButtonImage(nullptr), leftButtonImage(nullptr), rightButtonImage(nullptr), hBarButtonImage(nullptr), 
	vBarButtonImage(nullptr),	markerImage(nullptr)
{
	setDimension(gcn::Rectangle(0, 0, width, height));
	setContent(&listbox);
}


/**
**  Set the list
*/
void ListBoxWidget::setList(lua_State *lua, lua_Object *lo)
{
	lualistmodel.setList(lua, lo);
	listbox.setListModel(&lualistmodel);
	adjustSize();
}

/**
**  Set the list
*/
void ImageListBoxWidget::setList(lua_State *lua, lua_Object *lo)
{
	lualistmodel.setList(lua, lo);
	listbox.setListModel(&lualistmodel);
	adjustSize();
}

/**
**  Sets the ListModel index of the selected element.
**
**  @param selected  The ListModel index of the selected element.
**
**  @see gcn::ListBox
*/
void ListBoxWidget::setSelected(int selected)
{
	listbox.setSelected(selected);
}

/**
**  Sets the ListModel index of the selected element.
**
**  @param selected  The ListModel index of the selected element.
**
**  @see gcn::ListBox
*/
void ImageListBoxWidget::setSelected(int selected)
{
	listbox.setSelected(selected);
}

/**
**  Gets the ListModel index of the selected element.
**
**  @return  The ListModel index of the selected element.
**
**  @see gcn::ListBox
*/
int ListBoxWidget::getSelected() const
{
	return const_cast<gcn::ListBox &>(listbox).getSelected();
}

/**
**  Gets the ListModel index of the selected element.
**
**  @return  The ListModel index of the selected element.
**
**  @see gcn::ListBox
*/
int ImageListBoxWidget::getSelected() const
{
	return const_cast<ImageListBox &>(listbox).getSelected();
}

/**
**  Set background color of the ListBoxWidget.
**
**  @param color  Color to set.
*/
void ListBoxWidget::setBackgroundColor(const gcn::Color &color)
{
	ScrollArea::setBackgroundColor(color);
	ScrollArea::setBaseColor(color);
	listbox.setBackgroundColor(color);
}

/**
**  Set background color of the ListBoxWidget.
**
**  @param color  Color to set.
*/
void ImageListBoxWidget::setBackgroundColor(const gcn::Color &color)
{
	ScrollArea::setBackgroundColor(color);
	ScrollArea::setBaseColor(color);
	listbox.setBackgroundColor(color);
}

/**
**  Set font of the ListBox.
**
**  @param font  Font to set.
*/
void ListBoxWidget::setFont(gcn::Font *font)
{
	listbox.setFont(font);
	listbox.setWidth(getWidth());
	adjustSize();
}

/**
**  Set font of the ListBox.
**
**  @param font  Font to set.
*/
void ImageListBoxWidget::setFont(gcn::Font *font)
{
	listbox.setFont(font);
	listbox.setWidth(getWidth());
	adjustSize();
}

/**
**  Adjust size of the listBox.
**
**  @todo Fix width of the scroll area (depend of v-scroll or not).
*/
void ListBoxWidget::adjustSize()
{
	int i;
	int width;
	gcn::ListModel *listmodel;

	width = listbox.getWidth();
	assert_throw(listbox.getListModel() != nullptr);
	listmodel = listbox.getListModel();
	for (i = 0; i < listmodel->getNumberOfElements(); ++i) {
		if (width < listbox.getFont()->getWidth(listmodel->getElementAt(i))) {
			width = listbox.getFont()->getWidth(listmodel->getElementAt(i));
		}
	}
	if (width != listbox.getWidth()) {
		listbox.setWidth(width);
	}
}

/**
**  Adjust size of the listBox.
**
**  @todo Fix width of the scroll area (depend of v-scroll or not).
*/
void ImageListBoxWidget::adjustSize()
{
	int i;
	int width;
	gcn::ListModel *listmodel;

	width = listbox.getWidth();
	assert_throw(listbox.getListModel() != nullptr);
	listmodel = listbox.getListModel();
	for (i = 0; i < listmodel->getNumberOfElements(); ++i) {
		if (width < listbox.getFont()->getWidth(listmodel->getElementAt(i))) {
			width = listbox.getFont()->getWidth(listmodel->getElementAt(i));
		}
	}
	if (width != listbox.getWidth()) {
		listbox.setWidth(width);
	}
}

/**
**  Add an action listener
*/
void ListBoxWidget::addActionListener(gcn::ActionListener *actionListener)
{
	listbox.addActionListener(actionListener);
}

/**
**  Add an action listener
*/
void ImageListBoxWidget::addActionListener(gcn::ActionListener *actionListener)
{
	listbox.addActionListener(actionListener);
}

void ImageListBoxWidget::setHBarImage(CGraphic *image) {
	hBarButtonImage = image;
	mScrollbarWidth = std::min<int>(image->getWidth(), image->getHeight());
}
void ImageListBoxWidget::setVBarImage(CGraphic *image) {
	vBarButtonImage = image;
	mScrollbarWidth = std::min<int>(image->getWidth(), image->getHeight());
}

/**
**  Draw the list box  
**
**  @param  graphics Graphics to use
*/
void ImageListBoxWidget::draw(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	CGraphic *img = nullptr;

	// Check if we have all required graphics
	if (!this->upButtonImage || !this->downButtonImage || !this->leftButtonImage || !this->rightButtonImage
		|| !this->upPressedButtonImage || !this->downPressedButtonImage || !this->leftPressedButtonImage || !this->rightPressedButtonImage
		|| !this->markerImage || !this->hBarButtonImage || !this->vBarButtonImage) {
		throw std::runtime_error("Not all graphics for ImageListBoxWidget were set.");
	}

	gcn::Rectangle rect = getContentDimension();
	img = itemImage;
	img->Resize(rect.width, img->getHeight());
	int y = 0;
	while (y + img->getHeight() <= rect.height) {
		graphics->drawImage(img, 0, 0, 0, y, getWidth(), img->getHeight(), render_commands);
		y += img->getHeight();
	}
	img->SetOriginalSize();
	
	if (mVBarVisible)
	{
		if (mUpButtonPressed) {
			this->drawUpPressedButton(graphics, render_commands);
		} else {
			this->drawUpButton(graphics, render_commands);
		}
		if (mDownButtonPressed) {
			this->drawDownPressedButton(graphics, render_commands);
		} else {
			this->drawDownButton(graphics, render_commands);
		}
		this->drawVBar(graphics, render_commands);
		this->drawVMarker(graphics, render_commands);
	}
	if (mHBarVisible)
	{
		if (mLeftButtonPressed) {
			this->drawLeftPressedButton(graphics, render_commands);
		} else {
			this->drawLeftButton(graphics, render_commands);
		}
		if (mRightButtonPressed) {
			this->drawRightPressedButton(graphics, render_commands);
		} else {
			this->drawRightButton(graphics, render_commands);
		}
		this->drawHBar(graphics, render_commands);
		this->drawHMarker(graphics, render_commands);
	}
	if (mContent)
	{
		gcn::Rectangle contdim = mContent->getDimension();
		graphics->pushClipArea(getContentDimension());

		if (mContent->getBorderSize() > 0)
		{
			img = this->itemImage;
			gcn::Rectangle rec = mContent->getDimension();
			rec.x -= mContent->getBorderSize();
			rec.y -= mContent->getBorderSize();
			rec.width += 2 * mContent->getBorderSize();
			rec.height += 2 * mContent->getBorderSize();
			graphics->pushClipArea(rec);
			mContent->drawBorder(graphics, render_commands);
			graphics->popClipArea();
		}

		graphics->pushClipArea(contdim);
		mContent->draw(graphics, render_commands);
		graphics->popClipArea();
		graphics->popClipArea();
	}
}

/**
**  Draw the list box border 
**
**  @param  graphics Graphics to use
*/
void ImageListBoxWidget::drawBorder(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Color faceColor = getBaseColor();
	gcn::Color highlightColor, shadowColor;
	int alpha = getBaseColor().a;
	int width = getWidth() + getBorderSize() * 2 - 1;
	int height = getHeight() + getBorderSize() * 2 - 1;
	highlightColor = faceColor + 0x303030;
	highlightColor.a = alpha;
	shadowColor = faceColor - 0x303030;
	shadowColor.a = alpha;

	unsigned int i;
	for (i = 0; i < getBorderSize(); ++i)
	{
		graphics->setColor(shadowColor);
		graphics->drawLine(i,i, width - i, i, render_commands);
		graphics->drawLine(i,i + 1, i, height - i - 1, render_commands);
		graphics->setColor(highlightColor);
		graphics->drawLine(width - i,i + 1, width - i, height - i, render_commands);
		graphics->drawLine(i,height - i, width - i - 1, height - i, render_commands);
	}
}

void ImageListBoxWidget::drawUpButton(gcn::Graphics* graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Rectangle dim = getUpButtonDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = nullptr;

	img = upButtonImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight(), render_commands);
	graphics->popClipArea();
}

void ImageListBoxWidget::drawDownButton(gcn::Graphics* graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Rectangle dim = getDownButtonDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = nullptr;

	img = downButtonImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight(), render_commands);
	graphics->popClipArea();
}

void ImageListBoxWidget::drawLeftButton(gcn::Graphics* graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Rectangle dim = getLeftButtonDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = nullptr;

	img = leftButtonImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight(), render_commands);
	graphics->popClipArea();
}

void ImageListBoxWidget::drawRightButton(gcn::Graphics* graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Rectangle dim = getRightButtonDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = nullptr;

	img = rightButtonImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight(), render_commands);
	graphics->popClipArea();
}

void ImageListBoxWidget::drawUpPressedButton(gcn::Graphics* graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Rectangle dim = getUpButtonDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = nullptr;

	img = upPressedButtonImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight(), render_commands);
	graphics->popClipArea();
}

void ImageListBoxWidget::drawDownPressedButton(gcn::Graphics* graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Rectangle dim = getDownButtonDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = nullptr;

	img = downPressedButtonImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight(), render_commands);
	graphics->popClipArea();
}

void ImageListBoxWidget::drawLeftPressedButton(gcn::Graphics* graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Rectangle dim = getLeftButtonDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = nullptr;

	img = leftPressedButtonImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight(), render_commands);
	graphics->popClipArea();
}

void ImageListBoxWidget::drawRightPressedButton(gcn::Graphics* graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Rectangle dim = getRightButtonDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = nullptr;

	img = rightPressedButtonImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight(), render_commands);
	graphics->popClipArea();
}

void ImageListBoxWidget::drawHBar(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Rectangle dim = getHorizontalBarDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = nullptr;

	img = hBarButtonImage;
	img->Resize(dim.width, dim.height);
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight(), render_commands);
	img->SetOriginalSize();

	graphics->popClipArea();
}

void ImageListBoxWidget::drawVBar(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Rectangle dim = getVerticalBarDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = nullptr;

	img = vBarButtonImage;
	img->Resize(dim.width, dim.height);
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight(), render_commands);
	img->SetOriginalSize();

	graphics->popClipArea();
}

void ImageListBoxWidget::drawHMarker(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Rectangle dim = getHorizontalMarkerDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = nullptr;

	img = markerImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight(), render_commands);

	graphics->popClipArea();
}

void ImageListBoxWidget::drawVMarker(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Rectangle dim = getVerticalMarkerDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = nullptr;

	img = markerImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight(), render_commands);

	graphics->popClipArea();
}

gcn::Rectangle ImageListBoxWidget::getVerticalMarkerDimension()
{
	if (!mVBarVisible)
	{
		return gcn::Rectangle(0, 0, 0, 0);
	}

	int length, pos;
	gcn::Rectangle barDim = getVerticalBarDimension();

	if (mContent && mContent->getHeight() != 0)
	{
		length = this->markerImage->getHeight();
	}
	else
	{
		length = barDim.height;
	}

	if (length < mScrollbarWidth)
	{
		length = mScrollbarWidth;
	}

	if (length > barDim.height)
	{
		length = barDim.height;
	}

	if (getVerticalMaxScroll() != 0)
	{
		pos = ((barDim.height - length) * getVerticalScrollAmount())
			/ getVerticalMaxScroll();
	}
	else
	{
		pos = 0;
	}

	return gcn::Rectangle(barDim.x, barDim.y + pos, mScrollbarWidth, length);
}

gcn::Rectangle ImageListBoxWidget::getHorizontalMarkerDimension()
{
	if (!mHBarVisible)
	{
		return gcn::Rectangle(0, 0, 0, 0);
	}

	int length, pos;
	gcn::Rectangle barDim = getHorizontalBarDimension();

	if (mContent && mContent->getWidth() != 0)
	{
		length = this->markerImage->getHeight();
	}
	else
	{
		length = barDim.width;
	}

	if (length < mScrollbarWidth)
	{
		length = mScrollbarWidth;
	}

	if (length > barDim.width)
	{
		length = barDim.width;
	}

	if (getHorizontalMaxScroll() != 0)
	{
		pos = ((barDim.width - length) * getHorizontalScrollAmount())
			/ getHorizontalMaxScroll();
	}
	else
	{
		pos = 0;
	}

	return gcn::Rectangle(barDim.x + pos, barDim.y, length, mScrollbarWidth);
}


/*----------------------------------------------------------------------------
--  DropDownWidget
----------------------------------------------------------------------------*/


/**
**  Set the list
*/
void DropDownWidget::setList(lua_State *lua, lua_Object *lo)
{
	listmodel.setList(lua, lo);
	setListModel(&listmodel);
}

/**
**  Set the drop down size
*/
void DropDownWidget::setSize(int width, int height)
{
	DropDown::setSize(width, height);
	this->getListBox()->setSize(width, height);
}

/*----------------------------------------------------------------------------
--  ImageDropDownWidget
----------------------------------------------------------------------------*/

/**
**  Set the list
*/

void ImageDropDownWidget::setListModel(LuaListModel *listModel)
{
	assert_throw(mScrollArea && mScrollArea->getContent() != nullptr);

	mListBox.setListModel(listModel);

	if (mListBox.getSelected() < 0)
	{
		mListBox.setSelected(0);
	}

	adjustHeight();
}

void ImageDropDownWidget::setList(lua_State *lua, lua_Object *lo)
{
	listmodel.setList(lua, lo);
	setListModel(&listmodel);
}

/**
**  Set the drop down size
*/
void ImageDropDownWidget::setSize(int width, int height)
{
	DropDown::setSize(width, height);
	this->getListBox()->setSize(width, height);
}

void ImageDropDownWidget::setItemImage(const std::string &image_path) {
	itemImage = CGraphic::New(image_path);
	itemImage->Load(preferences::get()->get_scale_factor());
	mListBox.setItemImage(itemImage.get());
}

void ImageDropDownWidget::setDownNormalImage(const std::string &image_path)
{
	DownNormalImage = CGraphic::New(image_path);
	DownNormalImage->Load(preferences::get()->get_scale_factor());
}

void ImageDropDownWidget::setDownPressedImage(const std::string &image_path)
{
	DownPressedImage = CGraphic::New(image_path);
	DownPressedImage->Load(preferences::get()->get_scale_factor());
}

void ImageDropDownWidget::draw(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	assert_throw(mScrollArea && mScrollArea->getContent() != nullptr);
	int h;

	if (mDroppedDown)
	{
		h = mOldH;
	}
	else
	{
		h = getHeight();
	}

	std::shared_ptr<CGraphic> img = this->itemImage;
	if (!this->itemImage || !this->DownNormalImage || !this->DownPressedImage) {
		throw std::runtime_error("Not all graphics for ImageDropDownWidget were set.");
	}

	graphics->drawImage(img.get(), 0, 0, 0, 0, img->getWidth(), img->getHeight(), render_commands);
	
	graphics->setFont(getFont());

	if (mListBox.getListModel() && mListBox.getSelected() >= 0)
	{
		graphics->drawText(mListBox.getListModel()->getElementAt(mListBox.getSelected()),
			2, (h - getFont()->getHeight()) / 2, render_commands);
	}

	drawButton(graphics, render_commands);

	if (mDroppedDown)
	{
		graphics->pushClipArea(mScrollArea->getDimension());
		mScrollArea->draw(graphics, render_commands);
		graphics->popClipArea();
	}
}

void ImageDropDownWidget::drawBorder(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	gcn::Color faceColor = getBaseColor();
	gcn::Color highlightColor, shadowColor;
	int alpha = getBaseColor().a;
	int width = getWidth() + getBorderSize() * 2 - 1;
	int height = getHeight() + getBorderSize() * 2 - 1;
	highlightColor = faceColor + 0x303030;
	highlightColor.a = alpha;
	shadowColor = faceColor - 0x303030;
	shadowColor.a = alpha;

	unsigned int i;
	for (i = 0; i < getBorderSize(); ++i)
	{
		graphics->setColor(shadowColor);
		graphics->drawLine(i,i, width - i, i, render_commands);
		graphics->drawLine(i,i + 1, i, height - i - 1, render_commands);
		graphics->setColor(highlightColor);
		graphics->drawLine(width - i,i + 1, width - i, height - i, render_commands);
		graphics->drawLine(i,height - i, width - i - 1, height - i, render_commands);
	}
}

void ImageDropDownWidget::drawButton(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	int h;
	if (mDroppedDown)
	{
		h = mOldH;
	}
	else
	{
		h = getHeight();
	}
	//Wyrmgus start
//	int x = getWidth() - h;
	int x = getWidth() - (h - 1);
	//Wyrmgus end
	int y = 0;

	std::shared_ptr<CGraphic> img = nullptr;
	if (mDroppedDown) {
		img = this->DownPressedImage;
	} else {
		img = this->DownNormalImage;
	}
	//Wyrmgus start
//	img->Resize(h, h);
	//Wyrmgus end
	graphics->drawImage(img.get(), 0, 0, x, y, h, h, render_commands);
	//Wyrmgus start
//	img->SetOriginalSize();
	//Wyrmgus end
}

int ImageDropDownWidget::getSelected()
{
	assert_throw(mScrollArea && mScrollArea->getContent() != nullptr);

	return mListBox.getSelected();
}

void ImageDropDownWidget::setSelected(int selected)
{
	assert_throw(mScrollArea && mScrollArea->getContent() != nullptr);

	if (selected >= 0)
	{
		mListBox.setSelected(selected);
	}
}

void ImageDropDownWidget::adjustHeight()
{
	assert_throw(mScrollArea && mScrollArea->getContent() != nullptr);

	int listBoxHeight = mListBox.getHeight();
	int h2 = mOldH ? mOldH : getFont()->getHeight();

	setHeight(h2);

	// The addition/subtraction of 2 compensates for the seperation lines
	// seperating the selected element view and the scroll area.

	if (mDroppedDown && getParent())
	{
		int h = getParent()->getHeight() - getY();

		if (listBoxHeight > h - h2 - 2)
		{
			mScrollArea->setHeight(h - h2 - 2);
			setHeight(h);
		}
		else
		{
			setHeight(listBoxHeight + h2 + 2);
			mScrollArea->setHeight(listBoxHeight);
		}
	}

	mScrollArea->setWidth(getWidth());
	mScrollArea->setPosition(0, h2 + 2);
}

void ImageDropDownWidget::setListBox(ImageListBox *listBox)
{
	listBox->setSelected(mListBox.getSelected());
	listBox->setListModel(mListBox.getListModel());
	listBox->addActionListener(this);

	if (mScrollArea->getContent() != nullptr)
	{
		mListBox.removeActionListener(this);
	}

	mListBox = *listBox;

	mScrollArea->setContent(&mListBox);

	if (mListBox.getSelected() < 0)
	{
		mListBox.setSelected(0);
	}
}

void ImageDropDownWidget::setFont(gcn::Font *font)
{
	gcn::Widget::setFont(font);
	mListBox.setFont(font);
}

void ImageDropDownWidget::_mouseInputMessage(const gcn::MouseInput &mouseInput)
{
	gcn::BasicContainer::_mouseInputMessage(mouseInput);

	if (mDroppedDown)
	{
		assert_throw(mScrollArea && mScrollArea->getContent() != nullptr);

		if (mouseInput.y >= mOldH)
		{
			gcn::MouseInput mi = mouseInput;
			mi.y -= mScrollArea->getY();
			mScrollArea->_mouseInputMessage(mi);

			if (mListBox.hasFocus())
			{
				mi.y -= mListBox.getY();
				mListBox._mouseInputMessage(mi);
			}
		}
	}
}

/**
**  StatBoxWidget constructor
**
**  @param width   Width of the StatBoxWidget.
**  @param height  Height of the StatBoxWidget.
*/
StatBoxWidget::StatBoxWidget(int width, int height) : percent(100)
{
	setWidth(width);
	setHeight(height);

	setBackgroundColor(gcn::Color(0, 0, 0));
	setBaseColor(gcn::Color(255, 255, 255));
	setForegroundColor(gcn::Color(128, 128, 128));
}

/**
**  Draw StatBoxWidget.
**
**  @param graphics  Graphic driver used to draw.
**
**  @todo caption seem to be placed upper than the middle.
**  @todo set direction (hor./vert.) and growing direction(up/down, left/rigth).
*/
void StatBoxWidget::draw(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	int width;
	int height;

	width = getWidth();
	height = getHeight();

	graphics->setColor(getBackgroundColor());
	graphics->fillRectangle(gcn::Rectangle(0, 0, width, height), render_commands);

	graphics->setColor(getBaseColor());
	graphics->drawRectangle(gcn::Rectangle(1, 1, width - 2, height - 2), render_commands);

	graphics->setColor(getForegroundColor());
	width = percent * width / 100;
	graphics->fillRectangle(gcn::Rectangle(2, 2, width - 4, height - 4), render_commands);
	graphics->setFont(getFont());
	graphics->drawText(getCaption(),
					   (getWidth() - getFont()->getWidth(getCaption())) / 2,
					   (height - getFont()->getHeight()) / 2, render_commands);
}

/**
**  Set caption of StatBoxWidget.
**
**  @param caption  New value.
*/
void StatBoxWidget::setCaption(const std::string &caption)
{
	this->caption = caption;
	this->setDirty(true);
}

/**
**  Get caption of StatBoxWidget.
*/

const std::string &StatBoxWidget::getCaption() const
{
	return caption;
}

/**
**  Set percent of StatBoxWidget.
**
**  @param percent  New value.
*/
void StatBoxWidget::setPercent(const int percent)
{
	this->setDirty(true);
	this->percent = percent;
}

/**
**  Get percent of StatBoxWidget.
*/
int StatBoxWidget::getPercent() const
{
	return percent;
}


/*----------------------------------------------------------------------------
--  MenuScreen
----------------------------------------------------------------------------*/


/**
**  MenuScreen constructor
*/
MenuScreen::MenuScreen() : Container()
{
	setDimension(gcn::Rectangle(0, 0, Video.Width, Video.Height));
	setOpaque(false);

	// The gui must be set immediately as it is used by widgets
	// when they are added to the container
	oldtop = Gui->getTop();
	Gui->setTop(this);
}

/**
**  Run the menu.  Loops until stop is called.
*/
int MenuScreen::run(const bool loop)
{
	loopResult = 0;
	runLoop = loop;
	running = true;

	CurrentCursorState = CursorState::Point;
	cursor::set_current_cursor(UI.get_cursor(cursor_type::point), true);
	CursorOn = cursor_on::unknown;

	if (this->getDrawMenusUnder()) {
		engine_interface::get()->change_lua_dialog_open_count(1);
	}

	if (loop) {
		const EventCallback *old_callbacks = GetCallbacks();
		SetCallbacks(&GuichanCallbacks);
		while (runLoop && GameResult != GameExit) {
			UpdateDisplay();
			CheckMusicFinished();

			thread_pool::get()->co_spawn_sync([]() -> boost::asio::awaitable<void> {
				co_await WaitEventsOneFrame();
			});
		}
		SetCallbacks(old_callbacks);
		Gui->setTop(this->oldtop);
	} else {
		SetCallbacks(&GuichanCallbacks);
		MenuStack.push(this);
	}

	return this->loopResult;
}

/**
**  Stop the menu from running
*/
void MenuScreen::stop(int result, bool stopAll)
{
	if (running == false) {
		return;
	}

	if (this->getDrawMenusUnder()) {
		engine_interface::get()->change_lua_dialog_open_count(-1);
	}

	if (!this->runLoop) {
		Gui->setTop(this->oldtop);
		assert_throw(MenuStack.top() == this);
		MenuStack.pop();
		if (stopAll) {
			while (!MenuStack.empty()) {
				MenuStack.pop();
			}
		}
		if (MenuStack.empty()) {
			//InterfaceState = IfaceStateNormal;
			if (!CEditor::get()->is_running()) {
				SetCallbacks(&GameCallbacks);
			} else {
				SetCallbacks(&EditorCallbacks);
			}
			game::get()->set_paused(false);
			UI.StatusLine.Clear();
			if (GameRunning) {
				UIHandleMouseMove(CursorScreenPos, stored_key_modifiers);
			}
		}
	}

	runLoop = false;
	loopResult = result;
	running = false;
}

void MenuScreen::addLogicCallback(LuaActionListener *listener)
{
	logiclistener = listener;
}

void MenuScreen::draw(gcn::Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
{
	if (this->drawUnder) {
		gcn::Rectangle r = Gui->getGraphics()->getCurrentClipArea();
		Gui->getGraphics()->popClipArea();
		Gui->draw(oldtop, render_commands);
		Gui->getGraphics()->pushClipArea(r);
	}
	gcn::Container::draw(graphics, render_commands);
}

void MenuScreen::logic()
{
	if (NetConnectRunning == 2) {
		NetworkProcessClientRequest();
	}
	if (NetConnectRunning == 1) {
		NetworkProcessServerRequest();
	}
	if (logiclistener) {
		logiclistener->action("");
	}
	Container::logic();
}
