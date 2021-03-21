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

#pragma once

#include "guichan/keylistener.h"
#include "guichan/mouselistener.h"
#include "guichan/platform.h"
#include "guichan/widget.h"

namespace gcn
{
    /**
     * A RadioButton which can be grouped into RadioButtons groups. In a
     * RadioButton group, only one of the RadioButtons can be selected.
     */
    class GCN_CORE_DECLSPEC RadioButton :
        public Widget,
        public MouseListener,
        public KeyListener
    {
    public:

        /**
         * Constructor.
         */
        RadioButton();

        /**
         * Constructor.
         *
         * @param caption the Radiobutton caption.
         * @param group the group the RadioButton belongs to.
         * @param marked true if the RadioButton should be marked.
         */
        RadioButton(const std::string &caption,
                    const std::string &group,
                    bool marked=false);

        /**
         * Destructor.
         */
        virtual ~RadioButton();

        /**
         * Draws the box i.a not the caption.
         *
         * @param graphics a Graphics object to draw with.
         */
        virtual void drawBox(Graphics *graphics, std::vector<std::function<void(renderer *)>> &render_commands);

        /**
         * Checks if the RadioButton is marked.
         *
         * @return true if the RadioButton is marked.
         */
        virtual bool isMarked() const;

        /**
         * Sets the RadioButton to be marked.
         *
         * @param marked true if the RadioButton should be marked.
         */
        virtual void setMarked(bool marked);

        /**
         * Gets the RadioButton caption.
         *
         * @return the RadioButton caption.
         */
        virtual const std::string &getCaption() const;

        /**
         * Sets the RadioButton caption.
         *
         * @param caption the RadioButton caption.
         */
        virtual void setCaption(const std::string &caption);

        /**
         * Sets the group the RadioButton should belong to.
         *
         * @param group the name of the group.
         */
        virtual void setGroup(const std::string &group);

        /**
         * Gets the group the RadioButton belongs to.
         *
         * @return the group the RadioButton belongs to.
         */
        virtual const std::string &getGroup() const;

        /**
         * Adjusts the RadioButtons size to fit the font size.
         */
        virtual void adjustSize();


        // Inherited from Widget

        virtual void draw(Graphics* graphics, std::vector<std::function<void(renderer *)>> &render_commands) override;

        virtual void drawBorder(Graphics* graphics, std::vector<std::function<void(renderer *)>> &render_commands) override;


        // Inherited from KeyListener

        virtual bool keyPress(const Key& key);


        // Inherited from MouseListener

        virtual void mouseClick(int x, int y, int button, int count);

    protected:
        bool mMarked;
        std::string mCaption;
        std::string mGroup;

        typedef std::multimap<std::string, RadioButton *> GroupMap;
        typedef GroupMap::iterator GroupIterator;

        static GroupMap mGroupMap;
    };
}
