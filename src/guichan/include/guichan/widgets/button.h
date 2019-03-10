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

#ifndef GCN_BUTTON_HPP
#define GCN_BUTTON_HPP

#include <string>

#include "guichan/platform.h"
#include "guichan/widget.h"

namespace gcn
{
    /**
     * A regular button. Add an ActionListener to it to know when it
     * has been clicked.
     *
     * NOTE: You can only have text (a caption) on the button. If you want it
     *       to handle, for instance images, you can implement an ImageButton
     *       of your own and overload member functions from Button.
     */
    class GCN_CORE_DECLSPEC Button : public Widget,
                                     public MouseListener,
                                     public KeyListener
    {
    public:
        /**
         * Constructor.
         */
        Button();

        /**
         * Constructor.
         *
         * @param caption the caption of the Button.
         */
        Button(const std::string& caption);

        /**
         * Sets the Button caption.
         *
         * @param caption the Button caption.
         */
        virtual void setCaption(const std::string& caption);

        /**
         * Gets the Button caption.
         *
         * @return the Button caption.
         */
        virtual const std::string& getCaption() const;

        /**
         * Sets the alignment for the caption.
         *
         * @param alignment Graphics::LEFT, Graphics::CENTER or Graphics::RIGHT
         * @see Graphics
         */
        virtual void setAlignment(unsigned int alignment);

        /**
         * Gets the alignment for the caption.
         *
         * @return alignment of caption.
         */
        virtual unsigned int getAlignment() const;

		//Wyrmgus start
        /**
         * Sets whether the Button continuously generates an action while pressed.
         *
         * @param bool.
         */
        virtual void setActsPressed(bool actsPressed);

        /**
         * Gets whether the Button generates an action just by being pressed.
         *
         * @return true or false.
         */
        virtual bool actsPressed() const;
		//Wyrmgus end

        /**
         * Adjusts the buttons size to fit the content.
         */
        virtual void adjustSize();

        /**
         * Checks if the button is pressed down. Useful when drawing.
         *
         * @return true if the button is pressed down.
         */
        virtual bool isPressed() const;


        //Inherited from Widget

        virtual void draw(Graphics* graphics);

        virtual void drawBorder(Graphics* graphics);

        virtual void lostFocus();

		virtual void hotKeyPress();
		virtual void hotKeyRelease();

        // Inherited from MouseListener

        virtual void mouseClick(int x, int y, int button, int count);

        virtual void mousePress(int x, int y, int button);

        virtual void mouseRelease(int x, int y, int button);


        // Inherited from KeyListener

        virtual bool keyPress(const Key& key);

        virtual bool keyRelease(const Key& key);

    protected:
        std::string mCaption;
        bool mMouseDown, mKeyDown, mHotKeyDown;
        //Wyrmgus start
        bool mActsPressed;
        //Wyrmgus end
        unsigned int mAlignment;
    };
}

#endif
