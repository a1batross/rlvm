/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004 - 2008 Olof Naess�n and Per Larsson
 *
 *
 * Per Larsson a.k.a finalman
 * Olof Naess�n a.k.a jansem/yakslem
 *
 * Visit: http://guichan.sourceforge.net
 *
 * License: (BSD)
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Guichan nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * For comments regarding functions please see the header file.
 */

// TODO(xyz): rework this, very buggy on android

#include "guichan/opengl/openglgraphics.hpp"

#if defined (_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if defined (__amigaos4__)
#include <mgl/gl.h>
#define glVertex3i glVertex3f
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#elif defined(ANDROID)
#include <GLES/gl.h>
#else
#include <GL/gl.h>
#endif

#ifdef ANDROID
#define glOrtho glOrthof
#endif

#include "guichan/exception.hpp"
#include "guichan/image.hpp"
#include "guichan/opengl/openglimage.hpp"

namespace gcn
{
    OpenGLGraphics::OpenGLGraphics()
    {
        // setTargetPlane(640, 480);
        mAlpha = false;
    }

    OpenGLGraphics::OpenGLGraphics(int width, int height)
    {
        setTargetPlane(width, height);
        mAlpha = false;
    }

    OpenGLGraphics::~OpenGLGraphics()
    {

    }

    void OpenGLGraphics::pushAttribute(GLenum cap) {
        if (!glIsEnabled(cap)) {
            mGlEnabled.push_back(cap);
            glEnable(cap);
            std::cout << "enabled " << (int)cap << std::endl;
        }
    }

    void OpenGLGraphics::popAttributes() {
        for (size_t i = 0; i < mGlEnabled.size(); ++i) {
            // glDisable(mGlEnabled[i]);
        }
        mGlEnabled.clear();
    }

    void OpenGLGraphics::_beginDraw()
    {
#if 0
            // TODO menu kills the game without glPushAttrib
        // GL_COLOR_BUFFER_BIT
        pushAttribute(GL_ALPHA_TEST);
        pushAttribute(GL_BLEND);
        pushAttribute(GL_DITHER);
        pushAttribute(GL_COLOR_LOGIC_OP);
        // GL_DEPTH_BUFFER_BIT
        pushAttribute(GL_DEPTH_TEST);
        // GL_FOG_BIT
        pushAttribute(GL_FOG);
        /*    GL_ENABLE_BIT | */
        pushAttribute(GL_COLOR_MATERIAL);
        pushAttribute(GL_MULTISAMPLE);
        pushAttribute(GL_SAMPLE_COVERAGE);
        // GL_LIGHTING_BIT
        pushAttribute(GL_LIGHTING);
        // GL_SCISSOR_BIT
        pushAttribute(GL_SCISSOR_TEST);
        // GL_TEXTURE_BIT
        pushAttribute(GL_TEXTURE_2D);
        // GL_TRANSFORM_BIT
        pushAttribute(GL_NORMALIZE);
        pushAttribute(GL_RESCALE_NORMAL);
            /*GL_LINE_BIT |
            GL_POINT_BIT |
            GL_POLYGON_BIT |
            GL_STENCIL_BUFFER_BIT |
            GL_POINT_BIT |
            GL_LINE_BIT
        // GL_CURRENT_BIT
            ); */
#endif

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glMatrixMode(GL_TEXTURE);
        glPushMatrix();
        glLoadIdentity();

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        
        glOrtho(0.0,
                (double)mWidth,
                (double)mHeight,
                0.0,
                -1.0,
                1.0);

        /*glDisable(GL_LIGHTING);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_TEXTURE_2D);*/

        glEnable(GL_SCISSOR_TEST);
        // glPointSize(1.0);
        // glLineWidth(1.0);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

        pushClipArea(Rectangle(0, 0, mWidth, mHeight));
    }

    void OpenGLGraphics::_endDraw()
    {
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        glMatrixMode(GL_TEXTURE);
        glPopMatrix();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        // popAttributes();

        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        popClipArea();
    }

    bool OpenGLGraphics::pushClipArea(Rectangle area)
    {
        bool result = Graphics::pushClipArea(area);

        glScissor(mClipStack.top().x,
                  mHeight - mClipStack.top().y - mClipStack.top().height,
                  mClipStack.top().width,
                  mClipStack.top().height);

        return result;
    }

    void OpenGLGraphics::popClipArea()
    {
        Graphics::popClipArea();

        if (mClipStack.empty())
        {
            return;
        }

        glScissor(mClipStack.top().x,
                  mHeight - mClipStack.top().y - mClipStack.top().height,
                  mClipStack.top().width,
                  mClipStack.top().height);
    }

    void OpenGLGraphics::setTargetPlane(int width, int height)
    {
        mWidth = width;
        mHeight = height;
    }

    void OpenGLGraphics::drawImage(const Image* image,
                                   int srcX,
                                   int srcY,
                                   int dstX,
                                   int dstY,
                                   int width,
                                   int height)
    {
		const OpenGLImage* srcImage = dynamic_cast<const OpenGLImage*>(image);

        if (srcImage == NULL)
        {
            throw GCN_EXCEPTION("Trying to draw an image of unknown format, must be an OpenGLImage.");
        }

        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion outside of _beginDraw() and _endDraw()?");
        }

        const ClipRectangle& top = mClipStack.top();

        dstX += top.xOffset;
        dstY += top.yOffset;

        // Find OpenGL texture coordinates
        float texX1 = srcX / (float)srcImage->getTextureWidth();
        float texY1 = srcY / (float)srcImage->getTextureHeight();
        float texX2 = (srcX+width) / (float)srcImage->getTextureWidth();
        float texY2 = (srcY+height) / (float)srcImage->getTextureHeight();

        glBindTexture(GL_TEXTURE_2D, srcImage->getTextureHandle());

        glEnable(GL_TEXTURE_2D);

        // Check if blending already is enabled
        if (!mAlpha)
        {
            glEnable(GL_BLEND);
        }

        // Draw a textured quad -- the image
#if 0
        glBegin(GL_QUADS);
        glTexCoord2f(texX1, texY1);
        glVertex3i(dstX, dstY, 0);

        glTexCoord2f(texX1, texY2);
        glVertex3i(dstX, dstY + height, 0);

        glTexCoord2f(texX2, texY2);
        glVertex3i(dstX + width, dstY + height, 0);

        glTexCoord2f(texX2, texY1);
        glVertex3i(dstX + width, dstY, 0);
        glEnd();
#else
        GLfloat vtx1[] = {
          static_cast<GLfloat>(dstX), static_cast<GLfloat>(dstY), 0,
          static_cast<GLfloat>(dstX), static_cast<GLfloat>(dstY + height), 0,
          static_cast<GLfloat>(dstX + width), static_cast<GLfloat>(dstY + height), 0,
          static_cast<GLfloat>(dstX + width), static_cast<GLfloat>(dstY), 0
        };
        GLfloat tex1[] = {
          texX1,texY1,
          texX1,texY2,
          texX2,texY2,
          texX2,texY1
        };

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        glVertexPointer(3, GL_FLOAT, 0, vtx1);
        glTexCoordPointer(2, GL_FLOAT, 0, tex1);
        glDrawArrays(GL_TRIANGLE_FAN,0,4);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
        // glDisable(GL_TEXTURE_2D);

        // Don't disable blending if the color has alpha
        if (!mAlpha)
        {
            // glDisable(GL_BLEND);
        }
    }

    void OpenGLGraphics::drawPoint(int x, int y)
    {
        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion outside of _beginDraw() and _endDraw()?");
        }

        const ClipRectangle& top = mClipStack.top();

        x += top.xOffset;
        y += top.yOffset;

#if 0
        glBegin(GL_POINTS);
        glVertex2i(x, y);
        glEnd();
#endif
    }

    void OpenGLGraphics::drawLine(int x1, int y1, int x2, int y2)
    {
        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion outside of _beginDraw() and _endDraw()?");
        }

        const ClipRectangle& top = mClipStack.top();

        x1 += top.xOffset;
        y1 += top.yOffset;
        x2 += top.xOffset;
        y2 += top.yOffset;

#if 0
        glBegin(GL_LINES);
        glVertex2f(x1 + 0.375f,
                   y1 + 0.375f);
        glVertex2f(x2 + 1.0f - 0.375f,
                   y2 + 1.0f - 0.375f);
        glEnd();

        glBegin(GL_POINTS);
        glVertex2f(x2 + 1.0f - 0.375f,
                   y2 + 1.0f - 0.375f);
        glEnd();

        glBegin(GL_POINTS);
        glVertex2f(x1 + 0.375f,
                   y1 + 0.375f);
        glEnd();
#endif
    }

    void OpenGLGraphics::drawRectangle(const Rectangle& rectangle)
    {
        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion outside of _beginDraw() and _endDraw()?");
        }

        const ClipRectangle& top = mClipStack.top();
#if 0
        glBegin(GL_LINE_LOOP);
        glVertex2f(rectangle.x + top.xOffset,
                   rectangle.y + top.yOffset);
        glVertex2f(rectangle.x + rectangle.width + top.xOffset - 1.0f,
                   rectangle.y + top.yOffset + 0.375f);
        glVertex2f(rectangle.x + rectangle.width + top.xOffset - 1.0f,
                   rectangle.y + rectangle.height + top.yOffset);
        glVertex2f(rectangle.x + top.xOffset,
                   rectangle.y + rectangle.height + top.yOffset);
        glEnd();
#endif
    }

    void OpenGLGraphics::fillRectangle(const Rectangle& rectangle)
    {
        if (mClipStack.empty())
        {
            throw GCN_EXCEPTION("Clip stack is empty, perhaps you called a draw funtion outside of _beginDraw() and _endDraw()?");
        }

        const ClipRectangle& top = mClipStack.top();

#if 0
        glBegin(GL_QUADS);
        glVertex2i(rectangle.x + top.xOffset,
                   rectangle.y + top.yOffset);
        glVertex2i(rectangle.x + rectangle.width + top.xOffset,
                   rectangle.y + top.yOffset);
        glVertex2i(rectangle.x + rectangle.width + top.xOffset,
                   rectangle.y + rectangle.height + top.yOffset);
        glVertex2i(rectangle.x + top.xOffset,
                   rectangle.y + rectangle.height + top.yOffset);
        glEnd();
#endif
    }

    void OpenGLGraphics::setColor(const Color& color)
    {
        mColor = color;
        glColor4ub((GLubyte) color.r,
                   (GLubyte) color.g,
                   (GLubyte) color.b,
                   (GLubyte) color.a);

        mAlpha = color.a != 255;

        if (mAlpha)
        {
            glEnable(GL_BLEND);
        }
    }

    const Color& OpenGLGraphics::getColor() const
    {
        return mColor;
    }
}
