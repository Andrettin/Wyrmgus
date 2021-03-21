/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004, 2005 darkbits                        Js_./
 * Per Larsson a.k.a finalman                          _RqZ{a<^_aa
 * Olof Naess�n a.k.a jansem/yakslem                _asww7!uY`>  )\a//
 *                                                 _Qhm`] _f "'c  1!5m
 * Visit: http://guichan.darkbits.org             )Qk<P ` _: :+' .'  "{[
 *                                               .)j(] .d_/ '-(  P .   S
 * License: (BSD)                                <Td/Z <fP"5(\"??"\a.  .L
 * Redistribution and use in source and          _dV>ws?a-?'      ._/L  #'
 * binary forms, with or without                 )4d[#7r, .   '     )d`)[
 * modification, are permitted provided         _Q-5'5W..j/?'   -?!\)cam'
 * that the following conditions are met:       j<<WP+k/);.        _W=j f
 * 1. Redistributions of source code must       .$%w\/]Q  . ."'  .  mj$
 *    retain the above copyright notice,        ]E.pYY(Q]>.   a     J@\
 *    this list of conditions and the           j(]1u<sE"L,. .   ./^ ]{a
 *    following disclaimer.                     4'_uomm\.  )L);-4     (3=
 * 2. Redistributions in binary form must        )_]X{Z('a_"a7'<a"a,  ]"[
 *    reproduce the above copyright notice,       #}<]m7`Za??4,P-"'7. ).m
 *    this list of conditions and the            ]d2e)Q(<Q(  ?94   b-  LQ/
 *    following disclaimer in the                <B!</]C)d_, '(<' .f. =C+m
 *    documentation and/or other materials      .Z!=J ]e []('-4f _ ) -.)m]'
 *    provided with the distribution.          .w[5]' _[ /.)_-"+?   _/ <W"
 * 3. Neither the name of Guichan nor the      :$we` _! + _/ .        j?
 *    names of its contributors may be used     =3)= _f  (_yQmWW$#(    "
 *    to endorse or promote products derived     -   W,  sQQQQmZQ#Wwa]..
 *    from this software without specific        (js, \[QQW$QWW#?!V"".
 *    prior written permission.                    ]y:.<\..          .
 *                                                 -]n w/ '         [.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT       )/ )/           !
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY         <  (; sac    ,    '
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING,               ]^ .-  %
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF            c <   r
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR            aga<  <La
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE          5%  )P'-3L
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR        _bQf` y`..)a
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          ,J?4P'.P"_(\?d'.,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES               _Pa,)!f/<[]/  ?"
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT      _2-..:. .r+_,.. .
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     ?a.<%"'  " -'.a_ _,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                     ^
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * For comments regarding functions please see the header file.
 */

#include "stratagus.h"

#include "guichan/widgets/checkbox.h"

namespace gcn
{

    CheckBox::CheckBox()
    {
        setMarked(false);

        setFocusable(true);
        addMouseListener(this);
        addKeyListener(this);
    }

    CheckBox::CheckBox(const std::string &caption, bool marked)
    {
        setCaption(caption);
        setMarked(marked);

        setFocusable(true);
        addMouseListener(this);
        addKeyListener(this);

        adjustSize();
    }

    void CheckBox::draw(Graphics* graphics, std::vector<std::function<void(renderer *)>> &render_commands)
    {
        drawBox(graphics, render_commands);

        graphics->setFont(getFont());
        graphics->setColor(getForegroundColor());

        int h = getHeight() + getHeight() / 2;

        graphics->drawText(getCaption(), h - 2, 0, render_commands);

        if (hasFocus() && !getCaption().empty())
        {
            graphics->drawRectangle(Rectangle(h - 4, 0, getWidth() - h + 3, getHeight()), render_commands);
        }
    }

    void CheckBox::drawBorder(Graphics* graphics, std::vector<std::function<void(renderer *)>> &render_commands)
    {
        Color faceColor = getBaseColor();
        Color highlightColor, shadowColor;
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

    void CheckBox::drawBox(Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands)
    {
        int h = getHeight() - 1;

        int alpha = getBaseColor().a;
        Color faceColor = getBaseColor();
        faceColor.a = alpha;
        Color highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        Color shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;

        graphics->setColor(shadowColor);
        graphics->drawLine(0, 0, h, 0, render_commands);
        graphics->drawLine(0, 1, 0, h, render_commands);

        graphics->setColor(highlightColor);
        graphics->drawLine(h, 1, h, h, render_commands);
        graphics->drawLine(1, h, h - 1, h, render_commands);

        graphics->setColor(getBackgroundColor());
        graphics->fillRectangle(Rectangle(1, 1, h - 1, h - 1), render_commands);

        graphics->setColor(getForegroundColor());

        if (mMarked)
        {
            graphics->drawLine(3, 5, 3, h - 3, render_commands);
            graphics->drawLine(4, 5, 4, h - 3, render_commands);

             graphics->drawLine(5, h - 4, h - 2, 3, render_commands);
             graphics->drawLine(5, h - 5, h - 4, 4, render_commands);
        }
    }

    bool CheckBox::isMarked() const
    {
        return mMarked;
    }

    void CheckBox::setMarked(bool marked)
    {
        mMarked = marked;
    }

    const std::string &CheckBox::getCaption() const
    {
        return mCaption;
    }

    void CheckBox::setCaption(const std::string &caption)
    {
        mCaption = caption;
        setDirty(true);
    }

    bool CheckBox::keyPress(const Key& key)
    {
        if (key.getValue() == Key::K_ENTER ||
            key.getValue() == Key::K_SPACE)
        {
            toggle();
            return true;
        }
        return false;
    }

    void CheckBox::mouseClick(int, int, int button, int)
    {
        if (button == MouseInput::LEFT)
        {
            toggle();
        }
    }

    void CheckBox::adjustSize()
    {
        int height = getFont()->getHeight();

        setHeight(height);
        setWidth(getFont()->getWidth(mCaption) + height + height / 2);
    }

    void CheckBox::toggle()
    {
        mMarked = !mMarked;
        generateAction();
    }
}

