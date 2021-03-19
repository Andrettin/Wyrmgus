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

#include "guichan/graphics.h"
#include "guichan/exception.h"
#include "guichan/font.h"

namespace gcn
{

    Graphics::Graphics()
    {
        mFont = nullptr;
    }

    bool Graphics::pushClipArea(Rectangle area)
    {
        if (mClipStack.empty())
        {
            ClipRectangle carea;
            carea.x = area.x;
            carea.y = area.y;
            carea.width = area.width;
            carea.height = area.height;
            mClipStack.push(carea);
            return true;
        }

        ClipRectangle top = mClipStack.top();
        ClipRectangle carea;
        carea = area;
        carea.xOffset = top.xOffset + carea.x;
        carea.yOffset = top.yOffset + carea.y;
        carea.x += top.xOffset;
        carea.y += top.yOffset;

        bool result = carea.intersect(top);

        mClipStack.push(carea);

        return result;
    }

    void Graphics::popClipArea()
    {

        if (mClipStack.empty())
        {
        	assert(!"Tried to pop clip area from empty stack.");
            //throw GCN_EXCEPTION("Tried to pop clip area from empty stack.");
        }

        mClipStack.pop();
    }

    const ClipRectangle& Graphics::getCurrentClipArea() const
    {
        if (mClipStack.empty())
        {
        	assert(!"The clip area stack is empty.");
            //throw GCN_EXCEPTION("The clip area stack is empty.");
        }

        return mClipStack.top();
    }

    void Graphics::drawImage(const Image* image, int dstX, int dstY, const wyrmgus::player_color *player_color, unsigned int transparency, std::vector<std::function<void(renderer *)>> &render_commands) const
    {
        drawImage(image, 0, 0, dstX, dstY, image->getWidth(), image->getHeight(), player_color, transparency, false, render_commands);
    }

    void Graphics::setFont(Font* font)
    {
        mFont = font;
    }

	//Wyrmgus start
//    void Graphics::drawText(const std::string& text, int x, int y,
//                            unsigned int alignment)
    void Graphics::drawText(const std::string& text, int x, int y,
                            unsigned int alignment, bool is_normal, std::vector<std::function<void(renderer *)>> &render_commands)
	//Wyrmgus end
    {
        if (mFont == nullptr)
        {
        	assert(!"No font set.");
            //throw GCN_EXCEPTION("No font set.");
        }

        switch (alignment)
        {
          case LEFT:
			  //Wyrmgus start
//              mFont->drawString(this, text, x, y);
              mFont->drawString(this, text, x, y, is_normal, render_commands);
			  //Wyrmgus end
              break;
          case CENTER:
			  //Wyrmgus start
//              mFont->drawString(this, text, x - mFont->getWidth(text) / 2, y);
              mFont->drawString(this, text, x - mFont->getWidth(text) / 2, y, is_normal, render_commands);
			  //Wyrmgus end
              break;
          case RIGHT:
			  //Wyrmgus start
//              mFont->drawString(this, text, x - mFont->getWidth(text), y);
              mFont->drawString(this, text, x - mFont->getWidth(text), y, is_normal, render_commands);
			  //Wyrmgus end
              break;
          default:
          	assert(!"Unknown alignment.");
              //throw GCN_EXCEPTION("Unknown alignment.");
        }
    }
}
