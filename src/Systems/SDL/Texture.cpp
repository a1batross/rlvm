// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
//
// -----------------------------------------------------------------------
#include "log.h"

#ifndef ANDROID
#include "GL/glew.h"
#else
#endif

#include <SDL/SDL.h>

#include <iostream>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <functional>
#include <string>
#include <algorithm>
#include <cmath>

#include "Systems/Base/Colour.hpp"
#include "Systems/Base/GraphicsObject.hpp"
#include "Systems/Base/GraphicsObjectData.hpp"
#include "Systems/Base/SystemError.hpp"
#include "Systems/SDL/SDLGraphicsSystem.hpp"
#include "Systems/SDL/SDLSurface.hpp"
#include "Systems/SDL/SDLUtils.hpp"
#ifndef ANDROID
#include "Systems/SDL/Shaders.hpp"
#endif
#include "Systems/SDL/Texture.hpp"

#include "pygame/alphablit.h"

#ifdef ANDROID
#define GL_TEXTURE_WRAP_R                 0x8072
#endif

using namespace std;

unsigned int Texture::s_screen_width = 0;
unsigned int Texture::s_screen_height = 0;

unsigned int Texture::s_upload_buffer_size = 0;
boost::scoped_array<char> Texture::s_upload_buffer;

// -----------------------------------------------------------------------

void Texture::SetScreenSize(const Size& s) {
  s_screen_width = s.width();
  s_screen_height = s.height();
}

int Texture::ScreenHeight() {
  return s_screen_height;
}

GLenum get_texture_format(GLint bpp)
{
    switch (bpp) {
    case 4:
        return GL_RGBA;
    case 3:
        return GL_RGB;
    }
    throw "Unsupported pixel format";
}

int counter = 0;

// -----------------------------------------------------------------------
// Texture
// -----------------------------------------------------------------------
Texture::Texture(SDL_Surface* surface, int x, int y, int w, int h,
                 unsigned int bytes_per_pixel, int byte_order, int byte_type)
  : x_offset_(x), y_offset_(y), logical_width_(w), logical_height_(h),
    total_width_(surface->w), total_height_(surface->h),
    texture_width_(SafeSize(logical_width_)),
    texture_height_(SafeSize(logical_height_)),
    back_texture_id_(0),
    is_upside_down_(false) {
  surface = SDL_DisplayFormatAlpha(surface);
  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &texture_id_);
  glBindTexture(GL_TEXTURE_2D, texture_id_);
  DebugShowGLErrors();
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  GLenum texture_format = get_texture_format(bytes_per_pixel);

  if (w == total_width_ && h == total_height_) {
    SDL_LockSurface(surface);
    glTexImage2D(GL_TEXTURE_2D, 0, texture_format,
                 texture_width_, texture_height_,
                 0,
                 texture_format, GL_UNSIGNED_BYTE, NULL);
    DebugShowGLErrors();

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surface->w, surface->h,
                    byte_order, byte_type, surface->pixels);
    DebugShowGLErrors();

    SDL_UnlockSurface(surface);
  } else {
    // Cut out the current piece
    char* pixel_data = uploadBuffer(surface->format->BytesPerPixel * w * h);
    char* cur_dst_ptr = pixel_data;

    SDL_LockSurface(surface); {
      char* cur_src_ptr = (char*) surface->pixels;
      cur_src_ptr += surface->pitch * y;

      int row_start = surface->format->BytesPerPixel * x;
      int subrow_size = surface->format->BytesPerPixel * w;
      for (int current_row = 0; current_row < h; ++current_row) {
        memcpy(cur_dst_ptr, cur_src_ptr + row_start, subrow_size);
        cur_dst_ptr += subrow_size;
        cur_src_ptr += surface->pitch;
      }
    }
    SDL_UnlockSurface(surface);

    glTexImage2D(GL_TEXTURE_2D, 0, texture_format,
                 texture_width_, texture_height_,
                 0,
                 texture_format, GL_UNSIGNED_BYTE, NULL);
    DebugShowGLErrors();

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
                    byte_order, byte_type, pixel_data);
    DebugShowGLErrors();
  }

  SDL_FreeSurface(surface);
}

// -----------------------------------------------------------------------

Texture::Texture(render_to_texture, int width, int height)
  : x_offset_(0), y_offset_(0),
    logical_width_(width), logical_height_(height),
    total_width_(width), total_height_(height),
    texture_width_(0), texture_height_(0), texture_id_(0),
    back_texture_id_(0),
    is_upside_down_(true) {
  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &texture_id_);
  glBindTexture(GL_TEXTURE_2D, texture_id_);
  DebugShowGLErrors();
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  texture_width_ = SafeSize(logical_width_);
  texture_height_ = SafeSize(logical_height_);

  // This may fail.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
               texture_width_, texture_height_,
               0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  DebugShowGLErrors();

  glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, logical_width_,
                      logical_height_);
  DebugShowGLErrors();
}

// -----------------------------------------------------------------------

Texture::~Texture() {
  glDeleteTextures(1, &texture_id_);

  if (back_texture_id_)
    glDeleteTextures(1, &back_texture_id_);

  DebugShowGLErrors();
}

// -----------------------------------------------------------------------

char* Texture::uploadBuffer(unsigned int size) {
  if (!s_upload_buffer || size > s_upload_buffer_size) {
    s_upload_buffer.reset(new char[size]);
    s_upload_buffer_size = size;
  }

  return s_upload_buffer.get();
}

// -----------------------------------------------------------------------

void Texture::reupload(SDL_Surface* surface,
                       int offset_x, int offset_y,
                       int x, int y, int w, int h,
                       unsigned int bytes_per_pixel, int byte_order,
                       int byte_type) {
  surface = SDL_DisplayFormatAlpha(surface);
  glBindTexture(GL_TEXTURE_2D, texture_id_);

  if (w == total_width_ && h == total_height_) {
    SDL_LockSurface(surface);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surface->w, surface->h,
                    byte_order, byte_type, surface->pixels);
    DebugShowGLErrors();

    SDL_UnlockSurface(surface);
  } else {
    // Cut out the current piece
    char* pixel_data = uploadBuffer(surface->format->BytesPerPixel * w * h);
    char* cur_dst_ptr = pixel_data;

    SDL_LockSurface(surface); {
      char* cur_src_ptr = (char*) surface->pixels;
      cur_src_ptr += surface->pitch * y;

      int row_start = surface->format->BytesPerPixel * x;
      int subrow_size = surface->format->BytesPerPixel * w;
      for (int current_row = 0; current_row < h; ++current_row) {
        memcpy(cur_dst_ptr, cur_src_ptr + row_start, subrow_size);
        cur_dst_ptr += subrow_size;
        cur_src_ptr += surface->pitch;
      }
    }
    SDL_UnlockSurface(surface);

    glTexSubImage2D(GL_TEXTURE_2D, 0, offset_x, offset_y, w, h,
                    byte_order, byte_type, pixel_data);
    DebugShowGLErrors();
  }
  SDL_FreeSurface(surface);
}

// -----------------------------------------------------------------------

std::string readTextFile(const std::string& file) {
  ifstream ifs(file.c_str());
  if (!ifs) {
    ostringstream oss;
    oss << "Can't open text file: " << file;
    throw SystemError(oss.str());
  }

  string out, line;
  while (getline(ifs, line)) {
    out += line;
    out += '\n';
  }

  return out;
}

// -----------------------------------------------------------------------

#ifndef ANDROID
void printARBLog(GLhandleARB obj) {
  char str[256];
  GLsizei size = 0;
  glGetInfoLogARB(obj, 256, &size, str);
  if (size != 0) {
    cerr << "Log: " << str << endl;
  }
}
#endif

// -----------------------------------------------------------------------

// This is really broken and brain dead.
void Texture::renderToScreen(const Rect& src, const Rect& dst, int opacity) {
  int x1 = src.x(), y1 = src.y(), x2 = src.x2(), y2 = src.y2();
  int fdx1 = dst.x(), fdy1 = dst.y(), fdx2 = dst.x2(), fdy2 = dst.y2();
  if (!filterCoords(x1, y1, x2, y2, fdx1, fdy1, fdx2, fdy2))
    return;

  glEnable(GL_TEXTURE_2D);

  // For the time being, we are dumb and assume that it's one texture

  float thisx1 = float(x1) / texture_width_;
  float thisy1 = float(y1) / texture_height_;
  float thisx2 = float(x2) / texture_width_;
  float thisy2 = float(y2) / texture_height_;

  if (is_upside_down_) {
    thisy1 = float(logical_height_ - y1) / texture_height_;
    thisy2 = float(logical_height_ - y2) / texture_height_;
  }

  glBindTexture(GL_TEXTURE_2D, texture_id_);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if 0
  glBegin(GL_QUADS); {
    glColor4ub(255, 255, 255, opacity);
    glTexCoord2f(thisx1, thisy1);
    glVertex2i(fdx1, fdy1);
    glTexCoord2f(thisx2, thisy1);
    glVertex2i(fdx2, fdy1);
    glTexCoord2f(thisx2, thisy2);
    glVertex2i(fdx2, fdy2);
    glTexCoord2f(thisx1, thisy2);
    glVertex2i(fdx1, fdy2);
  }
  glEnd();
#else
  glColor4ub(255, 255, 255, opacity);
  GLfloat vtx1[] = {
    fdx1, fdy1, 0,
    fdx2, fdy1, 0,
    fdx2, fdy2, 0,
    fdx1, fdy2, 0
  };
  GLfloat tex1[] = {
    thisx1,thisy1,
    thisx2,thisy1,
    thisx2,thisy2,
    thisx1,thisy2
  };
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  glVertexPointer(3, GL_FLOAT, 0, vtx1);
  glTexCoordPointer(2, GL_FLOAT, 0, tex1);
  glDrawArrays(GL_TRIANGLE_FAN,0,4);

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
  glBlendFunc(GL_ONE, GL_ZERO);
}

// -----------------------------------------------------------------------

/**
 * @todo A function of this hairiness needs super more amounts of
 *       documentation.
 * @todo When I merge back to trunk, make sure to change the throw
 *       cstrs over to the new exception class.
 */
void Texture::renderToScreenAsColorMask(
  const Rect& src, const Rect& dst, const RGBAColour& rgba, int filter) {
  if (filter == 0) {
#ifndef ANDROID
    if (GLEW_ARB_fragment_shader && GLEW_ARB_multitexture) {
      render_to_screen_as_colour_mask_subtractive_glsl(src, dst, rgba);
    } else
#endif
      render_to_screen_as_colour_mask_subtractive_fallback(
        src, dst, rgba);
  } else {
    render_to_screen_as_colour_mask_additive(
      src, dst, rgba);
  }
}

// -----------------------------------------------------------------------

void Texture::render_to_screen_as_colour_mask_subtractive_glsl(
  const Rect& src, const Rect& dst, const RGBAColour& rgba) {
#if 0
  int x1 = src.x(), y1 = src.y(), x2 = src.x2(), y2 = src.y2();
  int fdx1 = dst.x(), fdy1 = dst.y(), fdx2 = dst.x2(), fdy2 = dst.y2();
  if (!filterCoords(x1, y1, x2, y2, fdx1, fdy1, fdx2, fdy2))
    return;

  float thisx1 = float(x1) / texture_width_;
  float thisy1 = float(y1) / texture_height_;
  float thisx2 = float(x2) / texture_width_;
  float thisy2 = float(y2) / texture_height_;

  if (is_upside_down_) {
    thisy1 = float(logical_height_ - y1) / texture_height_;
    thisy2 = float(logical_height_ - y2) / texture_height_;
  }

  // If we haven't already, allocate video memory for the back
  // texture.
  //
  // NOTE: Does this code deal with changing the dimensions of the
  // text box? Does it matter?
  if (back_texture_id_ == 0) {
    glGenTextures(1, &back_texture_id_);
    glBindTexture(GL_TEXTURE_2D, back_texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Generate this texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 texture_width_, texture_height_,
                 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    DebugShowGLErrors();
  }

  // Copy the current value of the region where we're going to render
  // to a texture for input to the shader
  glBindTexture(GL_TEXTURE_2D, back_texture_id_);
  int ystart = int(s_screen_height - fdy1 - (fdy2 - fdy1));
  int idx1 = int(fdx1);
  glCopyTexSubImage2D(GL_TEXTURE_2D,
                      0,
                      0, 0,
                      idx1, ystart, texture_width_, texture_height_);
  DebugShowGLErrors();

  glUseProgramObjectARB(Shaders::getColorMaskProgram());

  // Put the back_texture in texture slot zero and set this to be the
  // texture "current_values" in the above shader program.
  glActiveTextureARB(GL_TEXTURE0_ARB);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, back_texture_id_);
  glUniform1iARB(Shaders::getColorMaskUniformCurrentValues(), 0);

  // Put the mask in texture slot one and set this to be the
  // texture "mask" in the above shader program.
  glActiveTextureARB(GL_TEXTURE1_ARB);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texture_id_);
  glUniform1iARB(Shaders::getColorMaskUniformMask(), 1);

  glDisable(GL_BLEND);

  glBegin(GL_QUADS); {
    glColorRGBA(rgba);
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, thisx1, thisy2);
    glMultiTexCoord2fARB(GL_TEXTURE1_ARB, thisx1, thisy1);
    glVertex2i(fdx1, fdy1);
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, thisx2, thisy2);
    glMultiTexCoord2fARB(GL_TEXTURE1_ARB, thisx2, thisy1);
    glVertex2i(fdx2, fdy1);
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, thisx2, thisy1);
    glMultiTexCoord2fARB(GL_TEXTURE1_ARB, thisx2, thisy2);
    glVertex2i(fdx2, fdy2);
    glMultiTexCoord2fARB(GL_TEXTURE0_ARB, thisx1, thisy1);
    glMultiTexCoord2fARB(GL_TEXTURE1_ARB, thisx1, thisy2);
    glVertex2i(fdx1, fdy2);
  }
  glEnd();

  glActiveTextureARB(GL_TEXTURE1_ARB);
  glDisable(GL_TEXTURE_2D);
  glActiveTextureARB(GL_TEXTURE0_ARB);

  glUseProgramObjectARB(0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ZERO);
#else
  // TODO
  renderToScreen(src, dst, rgba.a());
#endif
}

// -----------------------------------------------------------------------

/**
 * This fallback does not accurately render the scene according to
 * standard RealLive. This only negatively shades according to the
 * alpha value, ignoring the rest of the \#WINDOW_ATTR colour.
 *
 * This will probably only occur with mesa software and people with
 * graphics cards > 5 years old.
 */
void Texture::render_to_screen_as_colour_mask_subtractive_fallback(
  const Rect& src, const Rect& dst, const RGBAColour& rgba) {
  int x1 = src.x(), y1 = src.y(), x2 = src.x2(), y2 = src.y2();
  int fdx1 = dst.x(), fdy1 = dst.y(), fdx2 = dst.x2(), fdy2 = dst.y2();
  if (!filterCoords(x1, y1, x2, y2, fdx1, fdy1, fdx2, fdy2))
    return;

  float thisx1 = float(x1) / texture_width_;
  float thisy1 = float(y1) / texture_height_;
  float thisx2 = float(x2) / texture_width_;
  float thisy2 = float(y2) / texture_height_;

  if (is_upside_down_) {
    thisy1 = float(logical_height_ - y1) / texture_height_;
    thisy2 = float(logical_height_ - y2) / texture_height_;
  }

  // First draw the mask
  glBindTexture(GL_TEXTURE_2D, texture_id_);

  /// SERIOUS WTF: gl_blend_func_separate causes a segmentation fault
  /// under the current i810 driver for linux.
//  glBlendFuncSeparate(GL_SRC_ALPHA_SATURATE, GL_ONE_MINUS_SRC_ALPHA,
//                      GL_SRC_COLOR, GL_ONE_MINUS_SRC_ALPHA);
  glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE_MINUS_SRC_ALPHA);

#if 0
  glBegin(GL_QUADS); {
    glColorRGBA(rgba);
    glTexCoord2f(thisx1, thisy1);
    glVertex2i(fdx1, fdy1);
    glTexCoord2f(thisx2, thisy1);
    glVertex2i(fdx2, fdy1);
    glTexCoord2f(thisx2, thisy2);
    glVertex2i(fdx2, fdy2);
    glTexCoord2f(thisx1, thisy2);
    glVertex2i(fdx1, fdy2);
  }
  glEnd();
#else
  glColor4ub(rgba.r(), rgba.g(), rgba.b(), rgba.a());
  GLfloat vtx1[] = {
    fdx1, fdy1, 0,
    fdx2, fdy1, 0,
    fdx2, fdy2, 0,
    fdx1, fdy2, 0
  };
  GLfloat tex1[] = {
    thisx1,thisy1,
    thisx2,thisy1,
    thisx2,thisy2,
    thisx1,thisy2
  };

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  glVertexPointer(3, GL_FLOAT, 0, vtx1);
  glTexCoordPointer(2, GL_FLOAT, 0, tex1);
  glDrawArrays(GL_TRIANGLE_FAN,0,4);

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif

  glBlendFunc(GL_ONE, GL_ZERO);
}

// -----------------------------------------------------------------------

void Texture::render_to_screen_as_colour_mask_additive(
  const Rect& src, const Rect& dst, const RGBAColour& rgba) {
  int x1 = src.x(), y1 = src.y(), x2 = src.x2(), y2 = src.y2();
  int fdx1 = dst.x(), fdy1 = dst.y(), fdx2 = dst.x2(), fdy2 = dst.y2();
  if (!filterCoords(x1, y1, x2, y2, fdx1, fdy1, fdx2, fdy2))
    return;

  float thisx1 = float(x1) / texture_width_;
  float thisy1 = float(y1) / texture_height_;
  float thisx2 = float(x2) / texture_width_;
  float thisy2 = float(y2) / texture_height_;

  if (is_upside_down_) {
    thisy1 = float(logical_height_ - y1) / texture_height_;
    thisy2 = float(logical_height_ - y2) / texture_height_;
  }

  // First draw the mask
  glBindTexture(GL_TEXTURE_2D, texture_id_);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#if 0
  glBegin(GL_QUADS); {
    glColorRGBA(rgba);
    glTexCoord2i(thisx1, thisy1);
    glVertex2f(fdx1, fdy1);
    glTexCoord2i(thisx2, thisy1);
    glVertex2f(fdx2, fdy1);
    glTexCoord2i(thisx2, thisy2);
    glVertex2f(fdx2, fdy2);
    glTexCoord2i(thisx1, thisy2);
    glVertex2f(fdx1, fdy2);
  }
  glEnd();
#else
  glColor4ub(rgba.r(), rgba.g(), rgba.b(), rgba.a());
  GLfloat vtx1[] = {
    fdx1, fdy1, 0,
    fdx2, fdy1, 0,
    fdx2, fdy2, 0,
    fdx1, fdy2, 0
  };
  GLfloat tex1[] = {
    thisx1,thisy1,
    thisx2,thisy1,
    thisx2,thisy2,
    thisx1,thisy2
  };

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  glVertexPointer(3, GL_FLOAT, 0, vtx1);
  glTexCoordPointer(2, GL_FLOAT, 0, tex1);
  glDrawArrays(GL_TRIANGLE_FAN,0,4);

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif

  glBlendFunc(GL_ONE, GL_ZERO);
}

// -----------------------------------------------------------------------

void Texture::renderToScreen(const Rect& src, const Rect& dst,
                             const int opacity[4]) {
#if 0
  // For the time being, we are dumb and assume that it's one texture
  int x1 = src.x(), y1 = src.y(), x2 = src.x2(), y2 = src.y2();
  int fdx1 = dst.x(), fdy1 = dst.y(), fdx2 = dst.x2(), fdy2 = dst.y2();
  if (!filterCoords(x1, y1, x2, y2, fdx1, fdy1, fdx2, fdy2))
    return;

  float thisx1 = float(x1) / texture_width_;
  float thisy1 = float(y1) / texture_height_;
  float thisx2 = float(x2) / texture_width_;
  float thisy2 = float(y2) / texture_height_;

  glBindTexture(GL_TEXTURE_2D, texture_id_);

  // Blend when we have less opacity
  if (find_if(opacity, opacity + 4, [](int o) { return o < 255; }) !=
      opacity + 4) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  glBegin(GL_QUADS); {
    glColor4ub(255, 255, 255, opacity[0]);
    glTexCoord2f(thisx1, thisy1);
    glVertex2i(fdx1, fdy1);
    glColor4ub(255, 255, 255, opacity[1]);
    glTexCoord2f(thisx2, thisy1);
    glVertex2i(fdx2, fdy1);
    glColor4ub(255, 255, 255, opacity[2]);
    glTexCoord2f(thisx2, thisy2);
    glVertex2i(fdx2, fdy2);
    glColor4ub(255, 255, 255, opacity[3]);
    glTexCoord2f(thisx1, thisy2);
    glVertex2i(fdx1, fdy2);
  }
  glEnd();
  glBlendFunc(GL_ONE, GL_ZERO);
#else
  // TODO
  renderToScreen(src, dst, (opacity[0] + opacity[1] + opacity[2] + opacity[3]) / 4);
#endif
}

// -----------------------------------------------------------------------

void Texture::renderToScreenAsObject(
  const GraphicsObject& go,
  const SDLSurface& surface,
  const Rect& srcRect,
  const Rect& dstRect,
  int alpha) {
  // This all needs to be pushed up out of the rendering code and down into
  // either GraphicsObject or the individual GraphicsObjectData subclasses.

  // TODO: Temporary hack while I wait to convert all of this machinery to
  // Rects.
  int xSrc1 = srcRect.x();
  int ySrc1 = srcRect.y();
  int xSrc2 = srcRect.x2();
  int ySrc2 = srcRect.y2();

  int fdx1 = dstRect.x(), fdy1 = dstRect.y(), fdx2 = dstRect.x2(),
      fdy2 = dstRect.y2();
  if (!filterCoords(xSrc1, ySrc1, xSrc2, ySrc2,
                    fdx1, fdy1, fdx2, fdy2)) {
    return;
  }

  // Convert the pixel coordinates into [0,1) texture coordinates
  float thisx1 = float(xSrc1) / texture_width_;
  float thisy1 = float(ySrc1) / texture_height_;
  float thisx2 = float(xSrc2) / texture_width_;
  float thisy2 = float(ySrc2) / texture_height_;

  glBindTexture(GL_TEXTURE_2D, texture_id_);

  glPushMatrix(); {
    // Translate to where the object starts.
    glTranslatef(fdx1, fdy1, 0);

    int width = fdx2 - fdx1;
    int height = fdy2 - fdy1;

    // Rotate the texture around the point (origin + position + reporigin)
    float x_rep = (width / 2.0f) + go.xRepOrigin();
    float y_rep = (height / 2.0f) + go.yRepOrigin();

    glTranslatef(x_rep, y_rep, 0);
    glRotatef(float(go.rotation()) / 10, 0, 0, 1);
    glTranslatef(-x_rep, -y_rep, 0);

    // RealLive has its own complex shading/tinting system which we implement
    // in a shader if available. It's costly enough that we make sure we need
    // to use it.
    bool using_shader = false;
#if 0
    if ((go.light() ||
         go.tint() != RGBColour::Black() ||
         go.colour() != RGBAColour::Clear() ||
         go.mono() ||
         go.invert()) &&
        GLEW_ARB_fragment_shader && GLEW_ARB_multitexture) {
      // Image
      glActiveTexture(GL_TEXTURE0_ARB);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, texture_id_);
      glUseProgramObjectARB(Shaders::getObjectProgram());
      glUniform1iARB(Shaders::getObjectUniformImage(), 0);

      // Colour/Tint/Etc.
      Shaders::loadObjectUniformFromGraphicsObject(go);

      // Alpha.
      glUniform1fARB(Shaders::getObjectUniformAlpha(),
                     alpha / 255.0f);

      // Our final blending color has to be all white here.
      using_shader = true;
    } else
#endif
    {
      // The shader takes care of the alpha for us, so we need to specify when
      // not using it.
      glColor4ub(255, 255, 255, alpha);
    }

    // Make this so that when we have composite 1, we're doing a pure
    // additive blend, (ignoring the alpha channel?)
    switch (go.compositeMode()) {
      case 0:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
      case 1:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
      case 2: {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
#if 0
        glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
#endif
        break;
      }
      default: {
        ostringstream oss;
        oss << "Invalid composite_mode in render: " << go.compositeMode();
        throw SystemError(oss.str());
      }
    }

#if 0
    glBegin(GL_QUADS); {
      glTexCoord2f(thisx1, thisy1);
      glVertex2i(0, 0);
      glTexCoord2f(thisx2, thisy1);
      glVertex2i(width, 0);
      glTexCoord2f(thisx2, thisy2);
      glVertex2i(width, height);
      glTexCoord2f(thisx1, thisy2);
      glVertex2i(0, height);
    }
    glEnd();
#else
  GLfloat vtx1[] = {
    0, 0, 0,
    width, 0, 0,
    width, height, 0,
    0, height, 0
  };
  GLfloat tex1[] = {
    thisx1,thisy1,
    thisx2,thisy1,
    thisx2,thisy2,
    thisx1,thisy2
  };

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  glVertexPointer(3, GL_FLOAT, 0, vtx1);
  glTexCoordPointer(2, GL_FLOAT, 0, tex1);
  glDrawArrays(GL_TRIANGLE_FAN,0,4);

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif

#if 0
    if (using_shader) {
      glUseProgramObjectARB(0);
    }
#endif

#if 0
    glBlendEquation(GL_FUNC_ADD);
#endif
    glBlendFunc(GL_ONE, GL_ZERO);
  }
  glPopMatrix();

  DebugShowGLErrors();
}

// -----------------------------------------------------------------------

static float our_round(float r) {
  return (r > 0.0f) ? floor(r + 0.5f) : ceil(r - 0.5f);
}

bool Texture::filterCoords(int& x1, int& y1, int& x2, int& y2,
                           int& dx1, int& dy1, int& dx2, int& dy2) {
  // POINT
  using std::max;
  using std::min;

  // Input: raw image coordinates
  // Output: false if this doesn't intersect with the texture piece we hold.
  //         true otherwise, and set the local coordinates
  int w1 = x2 - x1;
  int h1 = y2 - y1;

  // First thing we do is an intersection test to see if this input
  // range intersects the virtual range this Texture object holds.
  //
  /// @bug s/>/>=/?
  if (x1 + w1 >= x_offset_ && x1 < x_offset_ + logical_width_ &&
      y1 + h1 >= y_offset_ && y1 < y_offset_ + logical_height_) {
    // Do an intersection test in terms of the virtual coordinates
    int virX = max(x1, x_offset_);
    int virY = max(y1, y_offset_);
    int w = min(x1+w1, x_offset_ + logical_width_) - max(x1, x_offset_);
    int h = min(y1+h1, y_offset_ + logical_height_) - max(y1, y_offset_);

    // Adjust the destination coordinates
    int dx_width = dx2 - dx1;
    int dy_height = dy2 - dy1;
    float dx1Off = (virX - x1) / float(w1);
    dx1 = our_round(dx1 + (dx_width * dx1Off));
    float dx2Off = w / float(w1);
    dx2 = our_round(dx1 + (dx_width * dx2Off));
    float dy1Off = (virY - y1) / float(h1);
    dy1 = our_round(dy1 + (dy_height * dy1Off));
    float dy2Off = h / float(h1);
    dy2 = our_round(dy1 + (dy_height * dy2Off));

    // Output the source intersection in real (instead of
    // virtual) coordinates
    x1 = virX - x_offset_;
    x2 = x1 + w;
    y1 = virY - y_offset_;
    y2 = y1 + h;

    return true;
  }

  return false;
}
