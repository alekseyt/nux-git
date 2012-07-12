// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012 Inalogic® Inc.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License, as
 * published by the  Free Software Foundation; either version 2.1 or 3.0
 * of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License along with this program. If not, see <http://www.gnu.org/licenses/>
 *
 * Authored by: Tim Penhey <tim.penhey@canonical.com>
 *
 */
#ifndef NUX_CORE_ANIMATION_CONTROLLER_H
#define NUX_CORE_ANIMATION_CONTROLLER_H

#include <sigc++/signal.h>

namespace nux
{
namespace animation
{
class Animation;

class TickSource
{
public:
  virtual ~TickSource();

  sigc::signal<void, long long> tick;
};

/**
 * The Animation Controller is responsible for updating running animations.
 *
 * The controller is constructed with a tick source, and operates as a
 * pseudo-singleton.  A controller must be created somewhere, and once it is,
 * it set as the default controller.  It is expected that there is only one
 * animation controller.
 */
class Controller
{
public:
  static Controller* Instance();

  Controller();
  ~Controller();

  virtual void AddAnimation(Animation* animation) = 0;
  virtual void RemoveAnimation(Animation* animation) = 0;

private:
  Controller(Controller const&) = delete;
  Controller& operator=(Controller const&) = delete;
};

class AnimationController : public Controller, public sigc::trackable
{
public:
  AnimationController(TickSource& tick_source);
  ~AnimationController();

  // tick is expected to be ever increasing
  virtual void OnTick(long long tick);

  virtual void AddAnimation(Animation* animation);
  virtual void RemoveAnimation(Animation* animation);

private:
  struct Impl;
  Impl* pimpl;
};



}} // close namespaces

#endif
