// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006 Elliot Glaysher
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//  
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//  
// -----------------------------------------------------------------------

#include "Precompiled.hpp"

// -----------------------------------------------------------------------

/**
 * @file   Effect.cpp
 * @author Elliot Glaysher
 * @date   Thu Nov  2 20:35:54 2006
 * 
 * @brief  Base LongOperation for all transition effects on DCs.
 */

#include "Effects/Effect.hpp"
#include "MachineBase/RLMachine.hpp"
#include "Systems/Base/System.hpp"
#include "Systems/Base/GraphicsSystem.hpp"
#include "Systems/Base/EventSystem.hpp"
#include "Systems/Base/Surface.hpp"

#include <iostream>

using namespace std;

// -----------------------------------------------------------------------

Effect::Effect(RLMachine& machine, boost::shared_ptr<Surface> src,
               boost::shared_ptr<Surface> dst,
               int width, int height, int time)
  : m_width(width), m_height(height), m_duration(time), 
    m_startTime(machine.system().event().getTicks()),
    m_machine(machine), m_srcSurface(src), m_dstSurface(dst)
{
  machine.system().graphics().setIsResponsibleForUpdate(false);
}

// -----------------------------------------------------------------------

Effect::~Effect()
{
  m_machine.system().graphics().setIsResponsibleForUpdate(true);
}

// -----------------------------------------------------------------------

/// @todo Riht now, the ctrl pressed behaviour *may* not match
///       RealLive exactly. Verify this.
bool Effect::operator()(RLMachine& machine)
{
  unsigned int time = machine.system().event().getTicks();
  unsigned int currentFrame = time - m_startTime;

  bool ctrlPressed = machine.system().event().ctrlPressed();

  if(currentFrame >= m_duration || ctrlPressed)
  {
    return true;
  }
  else
  {
    // Render to the screen
    GraphicsSystem& graphics = machine.system().graphics();
    graphics.beginFrame();

    if(blitOriginalImage())
    {
      dstSurface().
        renderToScreen(0, 0, width(), height(),
                       0, 0, width(), height(),
                       255);
    }

    performEffectForTime(machine, currentFrame);

    graphics.endFrame();
    return false;
  }
}

// -----------------------------------------------------------------------
// BlitAfterEffectFinishes
// -----------------------------------------------------------------------

void BlitAfterEffectFinishes::performAfterLongOperation(RLMachine& machine)
{
  // Blit DC1 onto DC0, with full opacity, and end the operation
  m_srcSurface->blitToSurface(*m_dstSurface,
                              m_srcX, m_srcY, m_srcWidth, m_srcHeight,
                              m_dstX, m_dstY, m_dstWidth, m_dstHeight, 255);

  // Now force a screen refresh
  machine.system().graphics().markScreenForRefresh();
}

// -----------------------------------------------------------------------

BlitAfterEffectFinishes::BlitAfterEffectFinishes(
  LongOperation* in, boost::shared_ptr<Surface> src, boost::shared_ptr<Surface> dst,
  int srcX, int srcY, int srcWidth, int srcHeight,
  int dstX, int dstY, int dstWidth, int dstHeight)
  : PerformAfterLongOperationDecorator(in), m_srcSurface(src), m_dstSurface(dst),
    m_srcX(srcX), m_srcY(srcY), m_srcWidth(srcWidth), m_srcHeight(srcHeight),
    m_dstX(dstX), m_dstY(dstY), m_dstWidth(dstWidth), m_dstHeight(dstHeight)
{}

// -----------------------------------------------------------------------

BlitAfterEffectFinishes::~BlitAfterEffectFinishes()
{}

// -----------------------------------------------------------------------

void decorateEffectWithBlit(LongOperation*& lop, 
                            boost::shared_ptr<Surface> src,
                            boost::shared_ptr<Surface> dst)
{
  BlitAfterEffectFinishes* blit =
    new BlitAfterEffectFinishes(lop, src, dst, 
                                0, 0, src->width(), src->height(),
                                0, 0, src->width(), src->height());
  lop = blit;
}