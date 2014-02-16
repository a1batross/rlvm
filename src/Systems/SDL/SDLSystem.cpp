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

#include "Systems/SDL/SDLSystem.hpp"

#include <sstream>
#include <SDL/SDL.h>

#include "MachineBase/RLMachine.hpp"
#include "Systems/Base/GraphicsObject.hpp"
#include "Systems/Base/GraphicsObjectData.hpp"
#include "Systems/Base/Platform.hpp"
#include "Systems/SDL/SDLEventSystem.hpp"
#include "Systems/SDL/SDLGraphicsSystem.hpp"
#include "Systems/SDL/SDLSoundSystem.hpp"
#include "Systems/SDL/SDLTextSystem.hpp"
#include "libReallive/defs.h"
#include "libReallive/gameexe.h"

using namespace std;
using namespace libReallive;

// -----------------------------------------------------------------------

SDLSystem::SDLSystem(Gameexe& gameexe)
    : System(), gameexe_(gameexe) {
  // First, initialize SDL's video subsystem.
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    ostringstream ss;
    ss << "Video initialization failed: " << SDL_GetError();
    throw Error(ss.str());
  }

  // Initialize the various subsystems
  graphics_system_.reset(new SDLGraphicsSystem(*this, gameexe));
  event_system_.reset(new SDLEventSystem(*this, gameexe));
  text_system_.reset(new SDLTextSystem(*this, gameexe));
  sound_system_.reset(new SDLSoundSystem(*this));

  event_system_->addMouseListener(graphics_system_.get());
  event_system_->addMouseListener(text_system_.get());
}

// -----------------------------------------------------------------------

SDLSystem::~SDLSystem() {
  event_system_->removeMouseListener(text_system_.get());
  event_system_->removeMouseListener(graphics_system_.get());

  // Some combinations of SDL and FT on the Mac require us to destroy the
  // Platform first. This will crash on Tiger if this isn't here, but it won't
  // crash under Linux...
  platform_.reset();

  // Force the deletion of the various systems before we shut down
  // SDL.
  sound_system_.reset();
  graphics_system_.reset();
  event_system_.reset();
  text_system_.reset();

  SDL_Quit();
}

// -----------------------------------------------------------------------

extern bool global_texture_reload;

void SDLSystem::run(RLMachine& machine) {
  // Give the event handler a chance to run.
  event_system_->executeEventSystem(machine);
  text_system_->executeTextSystem();
  sound_system_->executeSoundSystem();
  if (!global_texture_reload)
    graphics_system_->executeGraphicsSystem(machine);

  if (platform())
    platform()->run(machine);

  boost::shared_ptr<LongOperation> longop = machine.currentLongOperation();

  int sleep_time = longop ? longop->sleepTime() : 0;

  // If forceWait is set, we've detected that the RealLive bytecode is trying
  // to call refresh() really fast in a loop and that we should inject some
  // sleep so the CPU doesn't burn.
  if (forceWait() && sleep_time < 10)
    sleep_time = 10;

  // If the longop wants us to sleep for a really long time, we also have a
  // problem because we cant't handle mouse input smoothly. So we have a
  // different maximum sleep time depending on whether the mouse is in the
  // window.
  int max_time = event_system_->mouseInsideWindow() ? 20 : 50;
  if (sleep_time > max_time)
    sleep_time = max_time;

  if (!forceFastForward() && sleep_time) {
    event_system_->wait(sleep_time);
    setForceWait(false);
  }
}

// -----------------------------------------------------------------------

GraphicsSystem& SDLSystem::graphics() {
  return *graphics_system_;
}

// -----------------------------------------------------------------------

EventSystem& SDLSystem::event() {
  return *event_system_;
}

// -----------------------------------------------------------------------

Gameexe& SDLSystem::gameexe() {
  return gameexe_;
}

// -----------------------------------------------------------------------

SDLTextSystem& SDLSystem::text() {
  return *text_system_;
}

// -----------------------------------------------------------------------

SoundSystem& SDLSystem::sound() {
  return *sound_system_;
}

SDLGraphicsSystem* getSDLGraphics(System& system) {
  return static_cast<SDLGraphicsSystem*>(&system.graphics());
}
