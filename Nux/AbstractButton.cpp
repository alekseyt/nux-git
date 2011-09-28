/*
 * Copyright 2010 Inalogic® Inc.
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
 * Authored by: Jay Taoko <jaytaoko@inalogic.com>
 *              Gordon Allott <gord.allott@canonical.com>
 *
 */

#include "Nux.h"
#include "AbstractButton.h"
#include "HLayout.h"

namespace nux
{
  NUX_IMPLEMENT_OBJECT_TYPE(AbstractButton);

  AbstractButton::AbstractButton(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , visual_state_(STATE_NORMAL)
  {
    active_ = false;
    persistent_active_state_ = true;

    mouse_click.connect(sigc::mem_fun(this, &AbstractButton::RecvClick));
    mouse_down.connect(sigc::mem_fun(this, &AbstractButton::RecvMouseDown));
    mouse_double_click.connect(sigc::mem_fun(this, &AbstractButton::RecvMouseDown));
    mouse_up.connect(sigc::mem_fun(this, &AbstractButton::RecvMouseUp));
    mouse_move.connect(sigc::mem_fun(this, &AbstractButton::RecvMouseMove));
    mouse_enter.connect(sigc::mem_fun(this, &AbstractButton::RecvMouseEnter));
    mouse_leave.connect(sigc::mem_fun(this, &AbstractButton::RecvMouseLeave));
  }

  AbstractButton::~AbstractButton()
  {

  }

  ButtonVisualState AbstractButton::GetVisualState()
  {
    return visual_state_;
  }

  void AbstractButton::RecvClick(int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    if(persistent_active_state_)
    {
      active_ = !active_;
    }

    activated.emit(this);
    QueueDraw();
  }

  void AbstractButton::RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags)
  {

    visual_state_ = STATE_PRELIGHT;
    changed_visual_state.emit(this);
    QueueDraw();
  }

  void AbstractButton::RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    visual_state_ = STATE_PRESSED;
    changed_visual_state.emit(this);
    QueueDraw();
  }

  void AbstractButton::RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
  {

  }

  void AbstractButton::RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    visual_state_ = STATE_PRELIGHT;
    changed_visual_state.emit(this);
    QueueDraw();
  }

  void AbstractButton::RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    visual_state_ = STATE_NORMAL;
    changed_visual_state.emit(this);
    QueueDraw();
  }
}
