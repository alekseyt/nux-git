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
 *
 */


#include "Nux.h"
#include "WindowCompositor.h"

#include "NuxGraphics/GLError.h"
#include "WindowThread.h"
#include "BaseWindow.h"
#include "MenuPage.h"
#include "PaintLayer.h"
#include "Painter.h"

#include "NuxGraphics/FontTexture.h"
namespace nux
{

// extern TimeCounter gTimeCounter;
// float gTimeAccum = 0;

  WindowCompositor::WindowCompositor()
  {
    OverlayDrawingCommand       = NULL;
    _previous_mouse_over_area   = NULL;
    m_CurrentEvent              = NULL;
    m_CurrentWindow             = NULL;
    m_FocusAreaWindow           = NULL;
    m_MenuWindow                = NULL;
    m_OverlayWindow             = NULL;
    _tooltip_window             = NULL;
    m_TooltipArea               = NULL;
    m_ModalWindow               = NULL;
    m_SelectedWindow            = NULL;
    _menu_chain                  = NULL;
    m_Background                = NULL;
    _tooltip_window             = NULL;
    m_OverlayWindow             = NULL;
    OverlayDrawingCommand       = NULL;
    m_CurrentWindow             = NULL;
    m_MenuWindow                = NULL;
    m_CurrentEvent              = NULL;
    _mouse_focus_area           = NULL;
    _mouse_over_area            = NULL;
    _previous_mouse_over_area   = NULL;
    _keyboard_event_receiver    = NULL;
    _always_on_front_window     = NULL;
    _inside_event_processing    = false;
    _inside_rendering_cycle     = false;
    _exclusive_input_area       = NULL;
    _in_exclusive_input_mode    = false;
    _pending_exclusive_input_mode_action = false;

    _dnd_area                   = NULL;
    _mouse_over_view            = NULL;
    _mouse_owner_view           = NULL;
    _mouse_over_menu_page       = NULL;
    _mouse_owner_menu_page      = NULL;
    _mouse_owner_base_window    = NULL;
    _starting_menu_event_cycle  = false;
    _menu_is_active             = false;
    _enable_nux_new_event_architecture   = true;

    if (GetWindowThread()->GetWindow().HasFrameBufferSupport())
    {
      m_FrameBufferObject = GetGraphicsDisplay()->GetGpuDevice()->CreateFrameBufferObject ();
      // Do not leave the Fbo binded. Deactivate it.
      m_FrameBufferObject->Deactivate ();
    }

    // At this stage, the size of the window may not be known yet.
    // FormatRenderTargets will be called the first time runtime gets into WindowThread::ExecutionLoop
    m_MainColorRT = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (2, 2, 1, BITFMT_R8G8B8A8);
    m_MainDepthRT = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (2, 2, 1, BITFMT_D24S8);

    _menu_chain = new std::list<MenuPage*>;
    m_PopupRemoved = false;
    m_MenuRemoved = false;
    m_ModalWindow = NULL;
    m_Background = new ColorLayer (Color (0xFF4D4D4D));
  }

  WindowCompositor::~WindowCompositor ()
  {
    _window_to_texture_map.clear ();
    m_FrameBufferObject.Release ();
    m_MainColorRT.Release ();
    m_MainDepthRT.Release ();
    _menu_chain->clear();

    std::list< ObjectWeakPtr<BaseWindow> >::iterator it;
    for(it = _view_window_list.begin (); it != _view_window_list.end (); it++)
    {
      //(*it)->UnReference();
    }
    _view_window_list.clear ();

    //for(it = _modal_view_window_list.begin (); it != _modal_view_window_list.end (); it++)
    //{
    //  (*it)->UnReference();
    //}
    _modal_view_window_list.clear ();

    NUX_SAFE_DELETE (_menu_chain);
    NUX_SAFE_DELETE (m_Background);
  }

  bool WindowCompositor::MouseDown (Point pt)
  {
    return false;
  }

  bool WindowCompositor::MouseMove (Point pt)
  {
    return false;
  }

  bool WindowCompositor::MouseUp (Point pt)
  {
    return true;
  }

  BaseWindow* WindowCompositor::GetSelectedWindow()
  {
    return m_SelectedWindow.GetPointer ();
  }

  WindowCompositor::RenderTargetTextures &WindowCompositor::GetWindowBuffer (BaseWindow *window)
  {
    RenderTargetTextures invalid;
    RenderTargetTextures &ret = invalid;
    std::map< BaseWindow*, RenderTargetTextures >::iterator it = _window_to_texture_map.find (window);

    if (it != _window_to_texture_map.end())
    {
      return (*it).second;
    }

    return ret;
  }

  void WindowCompositor::RegisterWindow (BaseWindow *window)
  {
    if (window == 0)
      return;
      
    std::list< ObjectWeakPtr<BaseWindow> >::iterator it = find (_view_window_list.begin(), _view_window_list.end(), window);

    if (it == _view_window_list.end() )
    {
      // The BaseWindow is referenced by the WindowCompositor.
      //window->Reference();
      _view_window_list.push_front (ObjectWeakPtr<BaseWindow> (window));
      m_SelectedWindow = window;

      RenderTargetTextures rt;

      // Don't size the texture to the dimension of the window yet. this will be done later.
      if (GetWindowThread ()->GetWindow().HasFrameBufferSupport() )
      {
        rt.color_rt = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (2, 2, 1, BITFMT_R8G8B8A8);
        rt.depth_rt = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (2, 2, 1, BITFMT_D24S8);
      }

      _window_to_texture_map.insert ( std::map< BaseWindow*, RenderTargetTextures >::value_type ( window, rt) );
    }
  }

  void WindowCompositor::UnRegisterWindow (BaseWindow *window)
  {
    if (window == 0)
      return;

    std::list< ObjectWeakPtr<BaseWindow> >::iterator it = find (_view_window_list.begin(), _view_window_list.end(), window);

    if (it != _view_window_list.end ())
    {
      _view_window_list.erase (it); // @see STL for note about list.erase(it++). It is valid for lists.

      if (_view_window_list.size ())
        m_SelectedWindow = (*_view_window_list.begin ());

      std::map< BaseWindow*, RenderTargetTextures >::iterator it2 = _window_to_texture_map.find (window);

      if (it2 != _window_to_texture_map.end ())
      {
        (*it2).second.color_rt = ObjectPtr<IOpenGLBaseTexture> (0);
        (*it2).second.depth_rt = ObjectPtr<IOpenGLBaseTexture> (0);
        _window_to_texture_map.erase (it2);
      }
    }
  }



  Area* WindowCompositor::GetMouseOwner()
  {
    return _mouse_owner_view;
  }

  //! Get Mouse position relative to the top left corner of the window.
  Point WindowCompositor::GetMousePosition()
  {
    return _mouse_position;
  }

  void WindowCompositor::ResetMousePointerAreas()
  {
    _mouse_over_view        = NULL;
    _mouse_owner_view       = NULL;
    _mouse_over_menu_page   = NULL;
    _mouse_owner_menu_page  = NULL;
  }

  void WindowCompositor::GetAreaUnderMouse(const Point& mouse_position, NuxEventType event_type, InputArea** area_under_mouse_pointer, BaseWindow** window)
  {
    *area_under_mouse_pointer = NULL;

    // Go through the list of BaseWindo and find the first area over which the mouse pointer is.
    std::list< ObjectWeakPtr<BaseWindow> >::iterator window_it;
    window_it = _view_window_list.begin();
    while((*area_under_mouse_pointer == NULL) && (window_it != _view_window_list.end()))
    {
      if((*window_it).IsValid() && (*window_it)->IsVisible())
      {
        *area_under_mouse_pointer = NUX_STATIC_CAST(InputArea*, (*window_it)->FindAreaUnderMouse(mouse_position, event_type));
        if(area_under_mouse_pointer)
        {
          // We have found an area. We are going to exit the while loop.
          *window = (*window_it).GetPointer();
        }
      }
      ++window_it;
    }

    // If area_under_mouse_pointer is NULL, then the mouse pointer is not over any of the BaseWindow. Try the main window layout.
    if(*area_under_mouse_pointer == NULL)
    {
      Layout* main_window_layout = GetWindowThread()->GetMainLayout();
      if(main_window_layout)
        *area_under_mouse_pointer = NUX_STATIC_CAST(InputArea*, main_window_layout->FindAreaUnderMouse(mouse_position, event_type));
    }
  }

  void WindowCompositor::MouseEventCycle(Event &event)
  {
    // _mouse_owner_view: the view that has the mouse down
    // _mouse_over_view: the view that is directly below the mouse pointer
    
    _mouse_position = Point(event.e_x, event.e_y);

    if(_mouse_owner_view == NULL)
    {
      // Context: The left mouse button is not down over an area.
      // We look for the area where the mouse pointer is located.

      // We should never get here for a NUX_MOUSE_RELEASED event
      if(event.e_event == NUX_MOUSE_PRESSED ||
        (event.e_event == NUX_MOUSE_MOVE) ||
        (event.e_event == NUX_MOUSE_DOUBLECLICK) ||
        (event.e_event == NUX_MOUSE_WHEEL) ||
        (event.e_event == NUX_WINDOW_MOUSELEAVE))
      {
        InputArea* hit_view = NULL;         // The view under the mouse
        BaseWindow* hit_base_window = NULL; // The BaseWindow bellow the mouse pointer.

        // Look for the area below the mouse pointer in the BaseWindow.
        Area *pointer_grab_area = GetPointerGrabArea ();
        if(pointer_grab_area)
        {
          // If there is a pending mouse pointer grab, test that area only
          hit_view = NUX_STATIC_CAST(InputArea*, pointer_grab_area->FindAreaUnderMouse(Point(event.e_x, event.e_y), event.e_event));
          if((hit_view == NULL) && (event.e_event == NUX_MOUSE_PRESSED))
          {
            Geometry geo = pointer_grab_area->GetAbsoluteGeometry();
            int x = event.e_x - geo.x;
            int y = event.e_y - geo.y;

            NUX_STATIC_CAST(InputArea*, pointer_grab_area)->EmitMouseDownOutsideArea(x, y, event.GetMouseState(), event.GetKeyState());
          }
        }
        else
        {
          GetAreaUnderMouse(Point(event.e_x, event.e_y), event.e_event, &hit_view, &hit_base_window);
        }

        Geometry hit_view_geo;
        int hit_view_x = 0;
        int hit_view_y = 0;

        if(hit_view)
        {
          hit_view_geo = hit_view->GetAbsoluteGeometry();
          hit_view_x = event.e_x - hit_view_geo.x;
          hit_view_y = event.e_y - hit_view_geo.y;
        }

        if(event.e_event == NUX_WINDOW_MOUSELEAVE)
        {
          if(_mouse_over_view != NULL)
          {
            // The area where the mouse was in the previous cycle and the area returned by GetAreaUnderMouse are different.
            // The area from the previous cycle receive a "mouse leave signal".
            Geometry geo = _mouse_over_view->GetAbsoluteGeometry();
            int x = event.e_x - geo.x;
            int y = event.e_y - geo.y;

            _mouse_over_view->EmitMouseLeaveSignal(x, y, event.GetMouseState(), event.GetKeyState());
            _mouse_over_view = NULL;
          }
        }
        else if(hit_view && (event.e_event == NUX_MOUSE_MOVE))
        {
          if(hit_view != _mouse_over_view)
          {
            
            if(_mouse_over_view != NULL)
            {
              // The area where the mouse was in the previous cycle and the area returned by GetAreaUnderMouse are different.
              // The area from the previous cycle receive a "mouse leave signal".
              Geometry geo = _mouse_over_view->GetAbsoluteGeometry();
              int x = event.e_x - geo.x;
              int y = event.e_y - geo.y;

              _mouse_over_view->EmitMouseLeaveSignal(x, y, event.GetMouseState(), event.GetKeyState());
            }

            // The area we found under the mouse pointer receives a "mouse enter signal".
            _mouse_over_view = hit_view;
            _mouse_over_view->EmitMouseEnterSignal(hit_view_x, hit_view_y, event.GetMouseState(), event.GetKeyState());
          }

          // Send a "mouse mouse signal".
          _mouse_over_view->EmitMouseMoveSignal(hit_view_x, hit_view_y, event.e_dx, event.e_dy, event.GetMouseState(), event.GetKeyState());
        }
        else if(hit_view && ((event.e_event == NUX_MOUSE_PRESSED) || (event.e_event == NUX_MOUSE_DOUBLECLICK)))
        {
          if((event.e_event == NUX_MOUSE_DOUBLECLICK) && (!hit_view->DoubleClickEnable()))
          {
            // If the area does not accept double click events, transform the event into a mouse pressed.
            event.e_event = NUX_MOUSE_PRESSED;
          }

          if(_mouse_over_view && (hit_view != _mouse_over_view))
          {
            // The area where the mouse was in the previous cycle and the area returned by GetAreaUnderMouse are different.
            // The area from the previous cycle receive a "mouse leave signal".
            // This case should be rare. I would happen if the mouse is over an area and that area is removed and reveals
            // a new area. If the next mouse event is a NUX_MOUSE_PRESSED, then the revealed area will be the one 
            // that is returned by GetAreaUnderMouse.
            Geometry geo = _mouse_over_view->GetAbsoluteGeometry();
            int x = event.e_x - geo.x;
            int y = event.e_y - geo.y;

            _mouse_over_view->EmitMouseLeaveSignal(x, y, event.GetMouseState(), event.GetKeyState());
          }

          _mouse_over_view = hit_view;
          _mouse_owner_view = hit_view;
          _mouse_position_on_owner = Point(hit_view_x, hit_view_y);

          // In the case of a mouse down event, if there is currently a keyboard event receiver and it is different
          // from the area returned by GetAreaUnderMouse, then stop that receiver from receiving anymore keyboard events and switch
          // make the found area the new receiver (if it accept keyboard events).
          if(_mouse_over_view != GetKeyboardEventReceiver())
          {
            if(GetKeyboardEventReceiver())
              GetKeyboardEventReceiver()->EmitEndKeyboardFocus();

            if(_mouse_over_view->AcceptKeyboardEvent())
            {
              _mouse_over_view->EmitStartKeyboardFocus();
              SetKeyboardEventReceiver(_mouse_over_view);
            }
            else
            {
              SetKeyboardEventReceiver(NULL);
            }
          }

          _mouse_over_view->EmitMouseDownSignal(hit_view_x, hit_view_y, event.GetMouseState(), event.GetKeyState());
        }
        else if(hit_view && (event.e_event == NUX_MOUSE_WHEEL))
        {
          hit_view->EmitMouseWheelSignal(hit_view_x, hit_view_y, event.e_wheeldelta, event.GetMouseState(), event.GetKeyState());
        }
        else if(hit_view == NULL)
        {
          if(_mouse_over_view)
          {
            Geometry geo = _mouse_over_view->GetAbsoluteGeometry();
            int x = event.e_x - geo.x;
            int y = event.e_y - geo.y;

            _mouse_over_view->EmitMouseLeaveSignal(x, y, event.GetMouseState(), event.GetKeyState());
          }

          if(GetKeyboardEventReceiver() && (event.e_event == NUX_MOUSE_PRESSED))
          {
            if(GetKeyboardEventReceiver()->KeyboardReceiverIgnoreMouseDownOutside() == false)
            {
              SetKeyboardEventReceiver(NULL);
            }
          }
          _mouse_over_view = 0;
        }
      }
    }
    else
    {
      // Context: The left mouse button down over an area. All events goes to that area.
      // But we still need to know where the mouse is.

      InputArea* hit_view = NULL;         // The view under the mouse
      BaseWindow* hit_base_window = NULL; // The BaseWindow bellow the mouse pointer.

      GetAreaUnderMouse(Point(event.e_x, event.e_y), event.e_event, &hit_view, &hit_base_window);

      Geometry mouse_owner_geo = _mouse_owner_view->GetAbsoluteGeometry();
      int mouse_owner_x = event.e_x - mouse_owner_geo.x;
      int mouse_owner_y = event.e_y - mouse_owner_geo.y;

      // the mouse is down over a view
      if(event.e_event == NUX_MOUSE_MOVE)
      {
        int dx = mouse_owner_x - _mouse_position_on_owner.x;
        int dy = mouse_owner_y - _mouse_position_on_owner.y;

        _mouse_owner_view->EmitMouseDragSignal(mouse_owner_x, mouse_owner_y, dx, dy, event.GetMouseState(), event.GetKeyState());

        if((_mouse_over_view == _mouse_owner_view) && (hit_view != _mouse_owner_view))
        {
          _mouse_owner_view->EmitMouseLeaveSignal(mouse_owner_x, mouse_owner_y, event.GetMouseState(), event.GetKeyState());
          _mouse_over_view = hit_view;
        }
        else if((_mouse_over_view != _mouse_owner_view) && (hit_view == _mouse_owner_view))
        {
          _mouse_owner_view->EmitMouseEnterSignal(mouse_owner_x, mouse_owner_y, event.GetMouseState(), event.GetKeyState());
          _mouse_over_view = _mouse_owner_view;
        }

        _mouse_position_on_owner = Point(mouse_owner_x, mouse_owner_y);
      }
      else if(event.e_event == NUX_MOUSE_RELEASED)
      {
        _mouse_owner_view->EmitMouseUpSignal(mouse_owner_x, mouse_owner_y, event.GetMouseState(), event.GetKeyState());

        if(hit_view == _mouse_owner_view)
        {
          _mouse_owner_view->EmitMouseClickSignal(mouse_owner_x, mouse_owner_y, event.GetMouseState(), event.GetKeyState());
          _mouse_over_view = _mouse_owner_view;
        }
        else
        {
          _mouse_over_view = hit_view;
        }

        _mouse_owner_view = NULL;
        _mouse_position_on_owner = Point(0, 0);
      }
    }
  }

  void WindowCompositor::MenuEventCycle(Event& event)
  {
    // _mouse_owner_menu_page: the menu page that has the mouse down
    // _mouse_over_menu_page: the menu page that is directly below the mouse pointer

    _mouse_position = Point(event.e_x, event.e_y);

    if(_mouse_owner_menu_page == NULL)
    {
      if((event.e_event == NUX_MOUSE_PRESSED) ||
        (event.e_event == NUX_MOUSE_RELEASED) ||
        (event.e_event == NUX_MOUSE_MOVE) ||
        (event.e_event == NUX_MOUSE_DOUBLECLICK) ||
        (event.e_event == NUX_MOUSE_WHEEL))
      {
        // Find the MenuPage under the mouse
        MenuPage* hit_menu_page = NULL;
        std::list<MenuPage*>::iterator menu_it;
        for(menu_it = _menu_chain->begin (); menu_it != _menu_chain->end (); menu_it++)
        {
          // The leaf of the menu chain is in the front of the list.
          hit_menu_page = NUX_STATIC_CAST(MenuPage*, (*menu_it)->FindAreaUnderMouse(Point(event.e_x, event.e_y), event.e_event));
          if(hit_menu_page)
          {
            break;
          }
        }

        Geometry hit_menu_page_geo;
        int hit_menu_page_x = 0;
        int hit_menu_page_y = 0;

        if(hit_menu_page)
        {
          hit_menu_page_geo = hit_menu_page->GetAbsoluteGeometry();
          hit_menu_page_x = event.e_x - hit_menu_page_geo.x;
          hit_menu_page_y = event.e_y - hit_menu_page_geo.y;
        }

        if(hit_menu_page && (event.e_event == NUX_MOUSE_RELEASED))
        {
          hit_menu_page->EmitMouseUpSignal(hit_menu_page_x, hit_menu_page_y, event.GetMouseState(), event.GetKeyState());

          (*_menu_chain->begin())->sigClosingMenu(*_menu_chain->begin());
          (*_menu_chain->begin())->StopMenu();
        }
        else if(hit_menu_page && (event.e_event == NUX_MOUSE_MOVE))
        {
          if(hit_menu_page != _mouse_over_menu_page)
          {
            if(_mouse_over_menu_page != 0)
            {
              Geometry geo = _mouse_over_menu_page->GetAbsoluteGeometry();
              int x = event.e_x - geo.x;
              int y = event.e_y - geo.y;

              _mouse_over_menu_page->EmitMouseLeaveSignal(x, y, event.GetMouseState(), event.GetKeyState());
            }

            _mouse_over_menu_page = hit_menu_page;
            _mouse_over_menu_page->EmitMouseEnterSignal(hit_menu_page_x, hit_menu_page_y, event.GetMouseState(), event.GetKeyState());
          }

          _mouse_over_menu_page->EmitMouseMoveSignal(hit_menu_page_x, hit_menu_page_y, event.e_dx, event.e_dy, event.GetMouseState(), event.GetKeyState());
        }
        else if(hit_menu_page && ((event.e_event == NUX_MOUSE_PRESSED) || (event.e_event == NUX_MOUSE_DOUBLECLICK)))
        {
          if(!hit_menu_page->DoubleClickEnable())
          {

          }

          if(_mouse_over_menu_page && (hit_menu_page != _mouse_over_menu_page))
          {
            Geometry geo = _mouse_over_menu_page->GetAbsoluteGeometry();
            int x = event.e_x - geo.x;
            int y = event.e_y - geo.y;

            _mouse_over_menu_page->EmitMouseLeaveSignal(x, y, event.GetMouseState(), event.GetKeyState());
          }

          _mouse_over_menu_page = hit_menu_page;
          _mouse_owner_menu_page = hit_menu_page;
          _mouse_position_on_owner = Point(hit_menu_page_x, hit_menu_page_y);

          if(_mouse_over_menu_page != GetKeyboardEventReceiver())
          {
            if(_mouse_over_menu_page->AcceptKeyboardEvent())
              SetKeyboardEventReceiver(_mouse_over_menu_page);
          }

          _mouse_over_menu_page->EmitMouseDownSignal(hit_menu_page_x, hit_menu_page_y, event.GetMouseState(), event.GetKeyState());
        }
        else if(hit_menu_page && (event.e_event == NUX_MOUSE_WHEEL))
        {
          hit_menu_page->EmitMouseWheelSignal(hit_menu_page_x, hit_menu_page_y, event.e_wheeldelta, event.GetMouseState(), event.GetKeyState());
        }
        else if(hit_menu_page == NULL)
        {
          if(_mouse_over_menu_page)
          {
            Geometry geo = _mouse_over_menu_page->GetAbsoluteGeometry();
            int x = event.e_x - geo.x;
            int y = event.e_y - geo.y;

            _mouse_over_menu_page->EmitMouseLeaveSignal(x, y, event.GetMouseState(), event.GetKeyState());
          }

          if(event.e_event == NUX_MOUSE_PRESSED || event.e_event == NUX_MOUSE_DOUBLECLICK)
          {
            (*_menu_chain->begin())->sigClosingMenu(*_menu_chain->begin());
            (*_menu_chain->begin())->StopMenu();
          }

          _mouse_over_menu_page = 0;
        }
      }
    }
    else
    {
      // We should never get here for a NUX_MOUSE_PRESSED event.
      MenuPage* hit_menu_page = NULL;
      std::list<MenuPage*>::iterator menu_it;
      for(menu_it = _menu_chain->begin (); menu_it != _menu_chain->end (); menu_it++)
      {
        // The leaf of the menu chain is in the front of the list.
        hit_menu_page = NUX_STATIC_CAST(MenuPage*, (*menu_it)->FindAreaUnderMouse(Point(event.e_x, event.e_y), event.e_event));
        if(hit_menu_page)
        {
          break;
        }
      }

      Geometry mouse_owner_geo = _mouse_owner_menu_page->GetAbsoluteGeometry();
      int mouse_owner_x = event.e_x - mouse_owner_geo.x;
      int mouse_owner_y = event.e_y - mouse_owner_geo.y;

      // the mouse is down over a view
      if(event.e_event == NUX_MOUSE_MOVE)
      {
        int dx = mouse_owner_x - _mouse_position_on_owner.x;
        int dy = mouse_owner_y - _mouse_position_on_owner.y;

        _mouse_owner_menu_page->EmitMouseDragSignal(mouse_owner_x, mouse_owner_y, dx, dy, event.GetMouseState(), event.GetKeyState());

        if((_mouse_over_menu_page == _mouse_owner_menu_page) && (hit_menu_page != _mouse_owner_menu_page))
        {
          _mouse_owner_menu_page->EmitMouseLeaveSignal(mouse_owner_x, mouse_owner_y, event.GetMouseState(), event.GetKeyState());
          _mouse_over_menu_page = hit_menu_page;
        }
        else if((_mouse_over_menu_page != _mouse_owner_menu_page) && (hit_menu_page == _mouse_owner_menu_page))
        {
          _mouse_owner_menu_page->EmitMouseEnterSignal(mouse_owner_x, mouse_owner_y, event.GetMouseState(), event.GetKeyState());
          _mouse_over_menu_page = _mouse_owner_menu_page;
        }

        _mouse_position_on_owner = Point(mouse_owner_x, mouse_owner_y);
      }
      else if(event.e_event == NUX_MOUSE_RELEASED)
      {
        _mouse_owner_menu_page->EmitMouseUpSignal(mouse_owner_x, mouse_owner_y, event.GetMouseState(), event.GetKeyState());

        if(hit_menu_page == _mouse_owner_menu_page)
        {
          _mouse_owner_menu_page->EmitMouseClickSignal(mouse_owner_x, mouse_owner_y, event.GetMouseState(), event.GetKeyState());
          _mouse_over_menu_page = _mouse_owner_menu_page;
        }
        else
        {
          _mouse_over_menu_page = hit_menu_page;
        }

        (*_menu_chain->begin())->sigClosingMenu(*_menu_chain->begin());
        (*_menu_chain->begin())->StopMenu();

        _mouse_owner_menu_page = NULL;
        _mouse_position_on_owner = Point(0, 0);
      }
    }
  }

  void WindowCompositor::KeyboardEventCycle(Event &event)
  {
    InputArea* keyboard_event_grab_view = NUX_STATIC_CAST(InputArea*, GetKeyboardGrabArea());
    InputArea* keyboard_event_receiver_view = NUX_STATIC_CAST(InputArea*, GetKeyboardEventReceiver());

    if(keyboard_event_grab_view)
    {      
      if(event.e_event == NUX_KEYDOWN)
      {
        keyboard_event_grab_view->EmitKeyDownSignal(event.GetKeySym(), event.e_x11_keycode, event.GetKeyState());
      }
      else if(event.e_event == NUX_KEYUP)
      {
        keyboard_event_grab_view->EmitKeyUpSignal(event.GetKeySym(), event.e_x11_keycode, event.GetKeyState());
      }

      keyboard_event_grab_view->EmitKeyEventSignal(event.GetKeySym(),
        event.GetKeySym(),
        event.GetKeyState(),
        event.GetText(),
        event.GetKeyRepeatCount());
    }
    else if(keyboard_event_receiver_view)
    {
      if(event.e_event == NUX_KEYDOWN)
      {
        keyboard_event_receiver_view->EmitKeyDownSignal(event.GetKeySym(), event.e_x11_keycode, event.GetKeyState());
      }
      else if(event.e_event == NUX_KEYUP)
      {
        keyboard_event_receiver_view->EmitKeyUpSignal(event.GetKeySym(), event.e_x11_keycode, event.GetKeyState());
      }

      keyboard_event_receiver_view->EmitKeyEventSignal(event.e_event,
        event.GetKeySym(),
        event.GetKeyState(),
        event.GetText(),
        event.GetKeyRepeatCount());
    }
  }

  long WindowCompositor::DispatchEventToArea (IEvent &event, Area *object, long TraverseInfo, long ProcessEventInfo)
  {
    if (object == 0)
      return 0;

    long ret = 0;

    event.e_x_root = _event_root.x;
    event.e_y_root = _event_root.y;

    if (object->Type().IsDerivedFromType (InputArea::StaticObjectType))
    {
      InputArea *base_area = NUX_STATIC_CAST (InputArea *, object);
      ret = base_area->OnEvent (event, TraverseInfo, ProcessEventInfo);
    }
    else if (object->Type().IsDerivedFromType (Layout::StaticObjectType))
    {
      Layout *layout = NUX_STATIC_CAST (Layout *, object);
      ret = layout->ProcessEvent (event, TraverseInfo, ProcessEventInfo);
    }
    else
    {
      nuxAssertMsg (0, TEXT ("This should not happen"));
    }

    return ret;
  }

  long WindowCompositor::DispatchEventToView (Event &event, View *view, long TraverseInfo, long ProcessEventInfo)
  {
    if (view == 0)
      return 0;

    long ret = 0;

    event.e_x_root = _event_root.x;
    event.e_y_root = _event_root.y;

    if (view->Type().IsDerivedFromType (InputArea::StaticObjectType))
    {
      //View *base_area = NUX_STATIC_CAST (View *, view);
      ret = view->ProcessEvent (event, TraverseInfo, ProcessEventInfo);
    }
    else
    {
      nuxAssertMsg (0, TEXT ("This should not happen"));
    }

    return ret;    
  }

  // NUXTODO: rename as EventCycle
  void WindowCompositor::ProcessEvent (Event &event)
  {
    if(_enable_nux_new_event_architecture)
    {
      if(((event.e_event >= NUX_MOUSE_PRESSED) && (event.e_event <= NUX_MOUSE_WHEEL)) ||
      (event.e_event == NUX_WINDOW_MOUSELEAVE))
      {
        if(_menu_chain->size())
        {
          MenuEventCycle(event);
        }
        else
        {
          MouseEventCycle(event);
        }

        if(_starting_menu_event_cycle)
        {
          _starting_menu_event_cycle = false;
        }

        CleanMenu();
      }
      
      if((event.e_event >= NUX_KEYDOWN) && (event.e_event <= NUX_KEYUP))
      {
        KeyboardEventCycle(event);
      }

      return;
    }

    // Event processing cycle begins.
    _inside_event_processing = true;

    Area *pointer_grab_area = GetPointerGrabArea ();
    Area *keyboard_grab_area = GetKeyboardGrabArea ();

    // Event Cycle Step 0: pass the event to the pointer or keyboard grab.
    //
    // Go into pointer grab if:
    //    - pointer_grab_area is not NULL
    //    - the event is a mouse event (mouse up/down/move/wheel/doubleclick)
    //    - there is no mouse focus or the area that has the mouse focus is also the pointer_grab_area
    //
    // Go into keyboard grab if:
    //    - keyboard_grab_area is not NULL
    //    - the event is a keyboard event (key up/down)
    //    - there is no mouse focus or the area that has the mouse focus is also the pointer_grab_area    
    if (
        (
          (
            (GetMouseFocusArea () == 0) || (GetMouseFocusArea () == pointer_grab_area)
          ) &&
          pointer_grab_area &&
          (
            (event.e_event == NUX_MOUSE_PRESSED) ||
            (event.e_event == NUX_MOUSE_RELEASED) ||
            (event.e_event == NUX_MOUSE_MOVE) ||
            (event.e_event == NUX_MOUSE_WHEEL) ||
            (event.e_event == NUX_MOUSE_DOUBLECLICK)
          )
        ) ||
        (
          keyboard_grab_area &&
          (
            (event.e_event == NUX_KEYDOWN) ||
            (event.e_event == NUX_KEYUP)
          )
        )
       )
    {
      // Set a system-wide copy the event. Useful for getting the a copy of the full event,
      // for instance in the area's signal callbacks. Area's signal callbacks don't pass the full event as a parameter.
      SetCurrentEvent (&event);

      long ret = 0;

      // If there is a menu open, process it. Menus or drop down menus (combo box) are modal user interface
      // elements. They must be closed before event processing can resume normally.
      {
        ret = MenuEventCycle (event, 0, 0);

        CleanMenu ();

        if (ret & eMouseEventSolved)
        {
          // The menu has claimed the event.
          SetCurrentEvent (NULL);
          return;
        }
      }
      
      // Update the state of view windows (aka BaseWindow) before processing.
      ViewWindowPreEventCycle ();

      if (pointer_grab_area &&
          ((event.e_event == NUX_MOUSE_PRESSED) ||
          (event.e_event == NUX_MOUSE_RELEASED) ||
          (event.e_event == NUX_MOUSE_MOVE) ||
          (event.e_event == NUX_MOUSE_WHEEL) ||
          (event.e_event == NUX_MOUSE_DOUBLECLICK))
      )
      {
        SetProcessingTopView (NUX_STATIC_CAST (BaseWindow*, pointer_grab_area->GetTopLevelViewWindow ()));
        // call the public event cycle processing function of the pointer_grab_area.
        ret = DispatchEventToView (event, NUX_STATIC_CAST (View*, pointer_grab_area), 0, 0);
        SetProcessingTopView (NULL);
      }

      if (keyboard_grab_area &&
          ((event.e_event == NUX_KEYDOWN) ||
          (event.e_event == NUX_KEYUP))
      )
      {
        SetProcessingTopView (NUX_STATIC_CAST (BaseWindow*, keyboard_grab_area->GetTopLevelViewWindow ()));
        // call the public event cycle processing function of the keyboard_grab_area.
        ret = DispatchEventToView (event, NUX_STATIC_CAST (View*, keyboard_grab_area), 0, 0);
        SetProcessingTopView (NULL);
      }

      // Update the state of view windows (aka BaseWindow) after event processing.
      ViewWindowPostEventCycle ();
      
      // Remove the system wide copy of the event.
      SetCurrentEvent (NULL);

      // End of grab event cycle.
      return;
    }

    long ret = 0;
    bool base_window_reshuffling = false; // Will be set to true if the BaseWindow are reshuffled.

    //std::list<MenuPage*>::iterator menu_it;

    if((event.e_event == NUX_SIZE_CONFIGURATION) ||
      (event.e_event == NUX_WINDOW_ENTER_FOCUS))
    {
      SetMouseFocusArea (NULL);
      SetMouseOverArea (NULL);
      SetPreviousMouseOverArea (NULL);
      SetKeyboardEventReceiver (NULL);
    }


    // Event Cycle Step 1: pass the event to the area that has the keyboard focus provided the event is 
    // not NUX_MOUSE_PRESSED, NUX_KEYDOWN or NUX_KEYUP.
    // The area where the mouse is down processes the event by calling its private event cycle processing
    // function directly. For instance, if this area has a layout, then the event will not be processed by
    // the layout and its children. 
    // The event processing goes into a modal type of interaction until the mouse button is released.
    if (GetMouseFocusArea () && (event.e_event != NUX_MOUSE_PRESSED) &&
          (event.e_event != NUX_KEYDOWN) &&
          (event.e_event != NUX_KEYUP)
       )
    {
      // Set a system-wide copy the event.
      SetCurrentEvent (&event);

      SetProcessingTopView (GetFocusAreaWindow());

      // call the private event cycle processing function of the keyboard_grab_area.
      DispatchEventToArea (event, GetMouseFocusArea(), 0, 0);

      if (event.e_event == NUX_MOUSE_RELEASED)
      {
        SetMouseFocusArea (NULL);
        // No need to set SetMouseOverArea to NULL.
        //SetMouseOverArea(0);
      }

      if ((event.e_event == NUX_MOUSE_RELEASED))
      {
        SetWidgetDrawingOverlay (NULL, NULL);
      }

      std::list<MenuPage*>::iterator menu_it;
      // Let all the menu area check the event first.
      for (menu_it = _menu_chain->begin(); menu_it != _menu_chain->end(); menu_it++)
      {
        // The deepest menu in the menu cascade is in the front of the list.
        ret = (*menu_it)->ProcessEvent (event, ret, 0);
      }

      SetCurrentEvent (NULL);
      SetProcessingTopView (NULL);
    }
    else
    {
      // Event Cycle Step 2: Full processing of the event. There is no grab, or mouse focus area.
      // This correspond to the action of moving the mouse or pressing a key.
      SetCurrentEvent (&event);

      // Let the Menus process the event.
      long ret = 0;
      long ProcessEventInfo = 0;
      bool MouseIsOverMenu = 0;
      {
        MenuEventCycle (event, 0, 0);

        if (ret & eMouseEventSolved)
        {
          // If one menu processed the event, then stop all other element from processing it.
          ProcessEventInfo = eDoNotProcess;
          MouseIsOverMenu = TRUE;
        }
      }

      // Let the ViewWindow process the event.
      ViewWindowPreEventCycle ();

      std::list< ObjectWeakPtr<BaseWindow> >::iterator it;


      if(_modal_view_window_list.size() > 0)
      {
        SetProcessingTopView((*_modal_view_window_list.begin()).GetPointer());
        ret = (*_modal_view_window_list.begin())->ProcessEvent(event, ret, ProcessEventInfo);
        SetProcessingTopView(NULL);
      }
      else
      {
        if((event.e_event == NUX_MOUSE_PRESSED) && (!InExclusiveInputMode()))
        {
          // There is a possibility we might have to reorder the stack of windows.
          // Cancel the currently selected window.
          m_SelectedWindow = NULL;
          base_window_reshuffling = true;
        }

        // Traverse the window from the top of the visibility stack to the bottom.
        for(it = _view_window_list.begin (); it != _view_window_list.end (); it++)
        {
          if (!((*it).IsValid ()))
            continue;

          if((*it).GetPointer () && (*it)->IsVisible () && ((*it)->_entering_visible_state == false))
          {
            SetProcessingTopView ((*it).GetPointer ());
            ret = (*it)->ProcessEvent (event, ret, ProcessEventInfo);
            SetProcessingTopView (NULL);

            if ((ret & eMouseEventSolved) && (m_SelectedWindow == 0) && (!InExclusiveInputMode ()))
            {
              // The mouse event was solved in the window pointed by the iterator.
              // There isn't a currently selected window. Make the window pointed by the iterator the selected window.
              base_window_reshuffling = true;
              m_SelectedWindow = (*it);
            }
          }
        }

        // Check if the mouse is over a menu. If yes, do not let the main window analyze the event.
        // This avoid mouse in/out messages from widgets below the menu chain.
        if (!MouseIsOverMenu)
        {
          // Let the main window analyze the event.
          ret = GetWindowThread()->ProcessEvent(event, ret, ProcessEventInfo);
        }
      }

      SetCurrentEvent (0);
    }

    ViewWindowPostEventCycle ();

    CleanMenu ();
  }

  void WindowCompositor::ViewWindowPreEventCycle ()
  {
    std::list< ObjectWeakPtr<BaseWindow> >::iterator it;
    for (it = _view_window_list.begin(); it != _view_window_list.end(); it++)
    {
      if (!(*it).IsValid ())
        continue;
      // Reset the preemptive hidden/visible status of all base windows.
      ObjectWeakPtr<BaseWindow> window = (*it);
      window->_entering_visible_state = false;
      window->_entering_hidden_state = false;
    }
  }

  void WindowCompositor::ViewWindowPostEventCycle ()
  {
    std::list< ObjectWeakPtr<BaseWindow> >::iterator it;
    for (it = _view_window_list.begin (); it != _view_window_list.end (); it++)
    {
      ObjectWeakPtr<BaseWindow> window = (*it);

      if (window.IsNull ())
        continue;

      // The view window cannot have both _entering_visible_state and _entering_hidden_state being true at the same time
      //nuxAssert (!(window->_entering_visible_state && window->_entering_hidden_state));

      if (window->_entering_visible_state)
      {
        sigVisibleViewWindow.emit (window.GetPointer ());
      }

      if (window->_entering_hidden_state)
      {
        sigHiddenViewWindow.emit (window.GetPointer ());
      }

      window->_entering_visible_state = false;
      window->_entering_hidden_state = false;
    }

    // Event processing cycle has ended.
    _inside_event_processing = false;

    // Make the designated BaseWindow always on top of the stack.
    EnsureAlwaysOnFrontWindow ();
  }

  long WindowCompositor::MenuEventCycle (Event &event, long TraverseInfo, long ProcessEventInfo)
  {
    long ret = TraverseInfo;
    std::list<MenuPage*>::iterator menu_it;

    if (m_MenuWindow.IsValid())
    {
      event.e_x_root = m_MenuWindow->GetBaseX ();
      event.e_y_root = m_MenuWindow->GetBaseY ();
    }

    // Let all the menu area check the event first. Beside, they are on top of everything else.
    for (menu_it = _menu_chain->begin (); menu_it != _menu_chain->end (); menu_it++)
    {
      // The deepest menu in the menu cascade is in the front of the list.
      ret = (*menu_it)->ProcessEvent (event, ret, ProcessEventInfo);
    }

    if ((event.e_event == NUX_MOUSE_PRESSED) && _menu_chain->size ())
    {
      bool inside = false;

      for (menu_it = _menu_chain->begin (); menu_it != _menu_chain->end (); menu_it++)
      {
        Geometry geo = (*menu_it)->GetGeometry ();

        if (PT_IN_BOX (event.e_x - event.e_x_root, event.e_y - event.e_y_root,
                        geo.x, geo.x + geo.width, geo.y, geo.y + geo.height))
        {
          inside = true;
          break;
        }
      }

      if (inside == false)
      {
        (*_menu_chain->begin ())->NotifyMouseDownOutsideMenuCascade (event.e_x - event.e_x_root, event.e_y - event.e_y_root);
      }
    }

    if (m_MenuWindow.IsValid ())
    {
      event.e_x_root = 0;
      event.e_y_root = 0;
    }

    if ( (event.e_event == NUX_MOUSE_RELEASED) )
    {
      SetWidgetDrawingOverlay (NULL, NULL);
    }

    if ( (event.e_event == NUX_SIZE_CONFIGURATION) && _menu_chain->size() )
    {
      (*_menu_chain->begin() )->NotifyTerminateMenuCascade();
    }

    return ret;
  }

  void WindowCompositor::StartModalWindow (ObjectWeakPtr<BaseWindow> window)
  {
    if (window == 0)
      return;

    std::list< ObjectWeakPtr<BaseWindow> >::iterator it = find (_modal_view_window_list.begin(), _modal_view_window_list.end(), window);

    if (it == _modal_view_window_list.end() )
    {
      _modal_view_window_list.push_front (window);
    }
  }

  void WindowCompositor::StopModalWindow (ObjectWeakPtr<BaseWindow> window)
  {
    if (_modal_view_window_list.size () > 0)
    {
      if (*_modal_view_window_list.begin () == window)
        _modal_view_window_list.pop_front ();
    }
  }

  //! Push a floating view at the top of the stack.
  void WindowCompositor::PushToFront (BaseWindow *window)
  {
    if (window == 0)
      return;

    std::list< ObjectWeakPtr<BaseWindow> >::iterator it = find (_view_window_list.begin(), _view_window_list.end (), window);

    if (it != _view_window_list.end () )
    {
      _view_window_list.erase (it);
      _view_window_list.push_front (ObjectWeakPtr<BaseWindow> (window));
    }

    EnsureAlwaysOnFrontWindow ();
  }

  //! Push a floating view at the bottom of the stack.
  void WindowCompositor::PushToBack (BaseWindow *window)
  {
    if (window == 0)
      return;

    if (window == _always_on_front_window)
      return;

    std::list< ObjectWeakPtr<BaseWindow> >::iterator it = find (_view_window_list.begin (), _view_window_list.end (), window);

    if (it != _view_window_list.end() )
    {
      _view_window_list.erase (it);
      _view_window_list.push_back (ObjectWeakPtr<BaseWindow> (window));
    }

    EnsureAlwaysOnFrontWindow ();
  }

  //! Push a floating view just above another floating view.
  void WindowCompositor::PushHigher (BaseWindow *top_floating_view, BaseWindow *bottom_floating_view, bool strict)
  {
    NUX_RETURN_IF_NULL (bottom_floating_view);
    NUX_RETURN_IF_NULL (top_floating_view);
    NUX_RETURN_IF_FALSE (bottom_floating_view != top_floating_view)
    
    std::list< ObjectWeakPtr<BaseWindow> >::iterator it;
    std::list< ObjectWeakPtr<BaseWindow> >::iterator it_top;
    std::list< ObjectWeakPtr<BaseWindow> >::iterator it_bot;
    
    int i = 0;
    int top_pos = -1;
    int bot_pos = -1;

    for (it_top = _view_window_list.begin (), i = 0; it_top != _view_window_list.end (); it_top++, i++)
    {
      if(*it == bottom_floating_view)
      {
        it_bot = it;
        bot_pos = i;
      }

      if(*it == top_floating_view)
      {
        it_top = it;
        top_pos = i;
      }

      if ((top_pos >= 0) && (bot_pos >= 0))
        break;
    }

    if ((it_top == _view_window_list.end ()) || (it_bot == _view_window_list.end ()))
    {
      return;
    }

    if ((top_pos < bot_pos) && (strict == false))
    {
      _view_window_list.erase (it_top);
      _view_window_list.insert (it_bot, ObjectWeakPtr<BaseWindow> (top_floating_view));
    }

    EnsureAlwaysOnFrontWindow ();
  }

  void WindowCompositor::SetAlwaysOnFrontWindow (BaseWindow *window)
  {
    _always_on_front_window = ObjectWeakPtr<BaseWindow> (window);

    EnsureAlwaysOnFrontWindow ();
  }

  void WindowCompositor::EnsureAlwaysOnFrontWindow ()
  {
    // Do not re-order while we are traversing the list of BaseWindow.
    if (_inside_event_processing)
      return;
    
    if (_always_on_front_window == NULL)
      return;

    std::list< ObjectWeakPtr<BaseWindow> >::iterator always_top_it = find (_view_window_list.begin(), _view_window_list.end(), _always_on_front_window);
    if ((always_top_it != _view_window_list.end ()) && (always_top_it != _view_window_list.begin ()) && (_always_on_front_window != NULL))
    {
      _view_window_list.erase (always_top_it);
      _view_window_list.push_front (_always_on_front_window);
    }
  }

  bool WindowCompositor::EnableExclusiveInputArea (InputArea *input_area)
  {
    NUX_RETURN_VALUE_IF_NULL (input_area, false);

    if (!input_area->Type().IsDerivedFromType (InputArea::StaticObjectType))
    {
      nuxDebugMsg (TEXT("[WindowCompositor::EnableExclusiveInputArea] Invalid input. The object does not inherit from InputArea."));
      return false;
    }

    if (_exclusive_input_area)
    {
      if (input_area == _exclusive_input_area)
        return true;
      return false;
    }

//     if (_pending_exclusive_input_mode_action)
//     {
//       nuxDebugMsg (TEXT("[WindowCompositor::EnableExclusiveInputArea] Eclusive event status has already changed once during event processing."));
//       return false;
//     }

//     if (_inside_event_processing)
//     {
//       _pending_exclusive_input_mode_action = true;
//       _exclusive_input_area = input_area;
//     }
//     else
    {
      // Initiating exclusive mode
      SetMouseFocusArea (NULL);
      // The area where the mouse is located is set to null;
      SetMouseOverArea (NULL);
      SetPreviousMouseOverArea (NULL);

      // Deactivate any active menu chain.
      CleanMenu ();

      _pending_exclusive_input_mode_action = false;
      _in_exclusive_input_mode = true;
      _exclusive_input_area = input_area;
    }

    return true;
  }
  
  bool WindowCompositor::DisableExclusiveInputArea (InputArea *input_area)
  {
    NUX_RETURN_VALUE_IF_NULL (input_area, false);

    if (input_area != _exclusive_input_area)
      return false;

    if (_inside_event_processing)
    {
      _pending_exclusive_input_mode_action = true;
      _exclusive_input_area = 0;
    }
    else
    {
      // Initiating exclusive mode
      SetMouseFocusArea (NULL);
      // The area where the mouse is located is set to null;
      SetMouseOverArea (NULL);
      SetPreviousMouseOverArea (NULL);

      // Deactivate any active menu chain.
      CleanMenu ();

      _pending_exclusive_input_mode_action = false;
      _in_exclusive_input_mode = false;
      _exclusive_input_area = 0;
    }

    return true;
  }

  void WindowCompositor::ExecPendingExclusiveInputAreaAction ()
  {
    if (_pending_exclusive_input_mode_action)
    {
      if (_exclusive_input_area)
      {
        // Initiating exclusive mode
        SetMouseFocusArea (NULL);
        // The area where the mouse is located is set to null;
        SetMouseOverArea (NULL);
        SetPreviousMouseOverArea (NULL);
        _pending_exclusive_input_mode_action = false;
        _in_exclusive_input_mode = true;
      }
      else
      {
        _pending_exclusive_input_mode_action = false;
        _in_exclusive_input_mode = false;
      }
    }
  }

  InputArea *WindowCompositor::GetExclusiveInputArea ()
  {
    return _exclusive_input_area;
  }

  bool WindowCompositor::InExclusiveInputMode ()
  {
    return _in_exclusive_input_mode;
  }

  void WindowCompositor::Draw (bool SizeConfigurationEvent, bool force_draw)
  {
    _inside_rendering_cycle = true;
    if (!GetWindowThread ()->GetWindow().isWindowMinimized())
    {
      //int w, h;
      GetWindowThread ()->GetGraphicsEngine().GetContextSize (m_Width, m_Height);
      GetWindowThread ()->GetGraphicsEngine().SetViewport (0, 0, m_Width, m_Height);
      
      // Reset the Model view Matrix and the projection matrix
      GetWindowThread ()->GetGraphicsEngine().ResetProjectionMatrix ();
      
      GetWindowThread ()->GetGraphicsEngine().ResetModelViewMatrixStack ();
      GetWindowThread ()->GetGraphicsEngine().Push2DTranslationModelViewMatrix (0.375f, 0.375f, 0.0f);


      if (GetWindowThread ()->GetWindow().HasFrameBufferSupport() )
      {
        if (force_draw || SizeConfigurationEvent)
        {
          // We fall here after something dramatic has happen to the window such as a resizing. In this case
          // everything must be rendered. This is very costly and should happen rarely.
          if (!GetWindowThread ()->IsEmbeddedWindow() )
            RenderMainWindowComposition (true, true);

          if (1 /*GetGraphicsDisplay()->GetGpuDevice()->GetGraphicsBoardVendor() != BOARD_INTEL*/)
          {
            RenderTopViews (true, _view_window_list, false, true);
            RenderTopViews (true, _modal_view_window_list, true, true);

            DrawMenu (true);
            DrawTooltip (true);
            DrawOverlay (true);
          }
        }
        else if (m_PopupRemoved || m_MenuRemoved)
        {
          // A popup removed cause the whole window to be dirty (at least some part of it).
          // So exchange DrawList with a real Draw.
          if (!GetWindowThread ()->IsEmbeddedWindow() )
            RenderMainWindowComposition (false, true);

          if (1 /*GetGraphicsDisplay()->GetGpuDevice()->GetGraphicsBoardVendor() != BOARD_INTEL*/)
          {
            RenderTopViews (false, _view_window_list, false, true);
            RenderTopViews (false, _modal_view_window_list, true, true);

            DrawMenu (true);
            DrawTooltip (true);
            DrawOverlay (true);
          }
        }
        else
        {
          if (!GetWindowThread ()->IsEmbeddedWindow() )
            RenderMainWindowComposition (false, true);

          if (1 /*GetGraphicsDisplay()->GetGpuDevice()->GetGraphicsBoardVendor() != BOARD_INTEL*/)
          {
            RenderTopViews (false, _view_window_list, false, true);
            RenderTopViews (false, _modal_view_window_list, true, true);

            DrawMenu (true);
            DrawTooltip (true);
            DrawOverlay (true);
          }
        }
      }
      else
      {
        if (force_draw || SizeConfigurationEvent)
        {
          GetPainter().PushDrawColorLayer (GetWindowThread ()->GetGraphicsEngine(), Geometry (0, 0, m_Width, m_Height), Color (0xFF4D4D4D), true);
          RenderMainWindowComposition (true, false);

          RenderTopViews (true, _view_window_list, false, false);
          RenderTopViews (true, _modal_view_window_list, true, false);
          DrawMenu (true);
          DrawOverlay (true);
          DrawTooltip (true);
          GetPainter().PopBackground();
        }
        else if (m_PopupRemoved || m_MenuRemoved)
        {
          GetPainter().PushDrawColorLayer (GetWindowThread ()->GetGraphicsEngine(), Geometry (0, 0, m_Width, m_Height), Color (0xFF4D4D4D), true);
          RenderMainWindowComposition (true, false);

          RenderTopViews (false, _view_window_list, false, false);
          RenderTopViews (false, _modal_view_window_list, true, false);
          DrawMenu (true);
          DrawOverlay (true);
          DrawTooltip (true);
          GetPainter().PopBackground();
        }
        else
        {
          RenderMainWindowComposition (false, false);
          RenderTopViews (true, _view_window_list, false, false);
          RenderTopViews (true, _modal_view_window_list, true, false);
          DrawMenu (true);
          DrawOverlay (true);
          DrawTooltip (true);
        }

        GetPainter().EmptyBackgroundStack();
      }

      m_PopupRemoved = false;
      m_MenuRemoved = false;

      GetWindowThread ()->GetGraphicsEngine().Pop2DWindow();
    }
    _inside_rendering_cycle = false;
  }

  void WindowCompositor::DrawMenu (bool force_draw)
  {
    ObjectWeakPtr<BaseWindow> window = m_MenuWindow;

    if (window.IsValid ())
    {
      //GetWindowThread ()->GetGraphicsEngine().SetContext (x, y, buffer_width, buffer_height);
      GetWindowThread ()->GetGraphicsEngine().SetOrthographicProjectionMatrix (GetWindowThread ()->GetGraphicsEngine().GetWindowWidth(),
          GetWindowThread ()->GetGraphicsEngine().GetWindowHeight() );
      GetWindowThread ()->GetGraphicsEngine().EmptyClippingRegion();
    }
    else
    {
      GetWindowThread ()->GetGraphicsEngine().SetOrthographicProjectionMatrix (GetWindowThread ()->GetGraphicsEngine().GetWindowWidth(),
          GetWindowThread ()->GetGraphicsEngine().GetWindowHeight() );
      GetWindowThread ()->GetGraphicsEngine().EmptyClippingRegion();
    }

    std::list<MenuPage *>::reverse_iterator rev_it_menu;

    for (rev_it_menu = _menu_chain->rbegin(); rev_it_menu != _menu_chain->rend( ); rev_it_menu++)
    {
      SetProcessingTopView (m_MenuWindow.GetPointer ());
      (*rev_it_menu)->ProcessDraw (GetWindowThread ()->GetGraphicsEngine(), force_draw);
      SetProcessingTopView (NULL);
    }

//     GetGraphicsDisplay()->GetGraphicsEngine()->SetContext (0, 0,
//                                             GetWindowThread ()->GetGraphicsEngine().GetWindowWidth(),
//                                             GetWindowThread ()->GetGraphicsEngine().GetWindowHeight() );
  }

  void WindowCompositor::DrawOverlay (bool force_draw)
  {
    ObjectWeakPtr<BaseWindow> window = m_OverlayWindow;
    int buffer_width = GetWindowThread ()->GetGraphicsEngine().GetWindowWidth();
    int buffer_height = GetWindowThread ()->GetGraphicsEngine().GetWindowHeight();

    if (window.IsValid ())
    {
      //GetWindowThread ()->GetGraphicsEngine().SetContext (x, y, buffer_width, buffer_height);
      GetWindowThread ()->GetGraphicsEngine().SetOrthographicProjectionMatrix (buffer_width, buffer_height);
      GetWindowThread ()->GetGraphicsEngine().EmptyClippingRegion();
    }
    else
      GetWindowThread ()->GetGraphicsEngine().SetOpenGLClippingRectangle (0, 0, buffer_width, buffer_height);

    if (OverlayDrawingCommand)
    {
      SetProcessingTopView (m_OverlayWindow.GetPointer ());
      OverlayDrawingCommand->OverlayDrawing (GetWindowThread ()->GetGraphicsEngine() );
      SetProcessingTopView (NULL);
    }

    //GetGraphicsDisplay()->GetGraphicsEngine()->SetContext (0, 0, buffer_width, buffer_height);
  }

  void WindowCompositor::DrawTooltip (bool force_draw)
  {
    ObjectWeakPtr<BaseWindow> window = _tooltip_window;
    int buffer_width = GetWindowThread ()->GetGraphicsEngine().GetWindowWidth();
    int buffer_height = GetWindowThread ()->GetGraphicsEngine().GetWindowHeight();

    if (window.IsValid ())
    {
      //GetWindowThread ()->GetGraphicsEngine().SetContext (x, y, buffer_width, buffer_height);
      GetWindowThread ()->GetGraphicsEngine().SetOrthographicProjectionMatrix (buffer_width, buffer_height);
      GetWindowThread ()->GetGraphicsEngine().EmptyClippingRegion();
    }
    else
      GetWindowThread ()->GetGraphicsEngine().SetOpenGLClippingRectangle (0, 0, buffer_width, buffer_height);

    if(m_TooltipText.Size())
    {
        //SetProcessingTopView(_tooltip_window);
        GetPainter().PaintShape(GetWindowThread ()->GetGraphicsEngine(), _tooltip_geometry, Color(0xA0000000), eSHAPE_CORNER_ROUND10, true);
        GetPainter().PaintTextLineStatic(GetWindowThread ()->GetGraphicsEngine(), GetSysBoldFont(), _tooltip_text_geometry, m_TooltipText, Color(0xFFFFFFFF));
        //SetProcessingTopView(NULL);
    }

    //GetGraphicsDisplay()->GetGraphicsEngine()->SetContext (0, 0, buffer_width, buffer_height);
  }

  void WindowCompositor::RenderTopViewContent (BaseWindow *window, bool force_draw)
  {
    unsigned int window_width, window_height;
    window_width = GetWindowThread ()->GetGraphicsEngine().GetWindowWidth();
    window_height = GetWindowThread ()->GetGraphicsEngine().GetWindowHeight();

    GetPainter().EmptyBackgroundStack();
    SetProcessingTopView (window);
    window->ProcessDraw (GetWindowThread ()->GetGraphicsEngine(), force_draw || window->IsRedrawNeeded() );
    SetProcessingTopView (NULL);
    GetPainter().EmptyBackgroundStack();
  }

  void WindowCompositor::RenderTopViews (bool force_draw, std::list< ObjectWeakPtr<BaseWindow> >& WindowList, bool drawModal, bool use_fbo)
  {
    GetWindowThread ()->GetGraphicsEngine ().EmptyClippingRegion ();
    // Raw the windows from back to front;
    std::list< ObjectWeakPtr<BaseWindow> >::reverse_iterator rev_it;

    for (rev_it = WindowList.rbegin (); rev_it != WindowList.rend (); rev_it++)
    {
      if (!(*rev_it).IsValid ())
        continue;
        
      if ((drawModal == false) && (*rev_it)->IsModal ())
        continue;

      bool WindowNeedRedraw = false;

      if ((*rev_it)->IsVisible() )
      {
        RenderTargetTextures &rt = GetWindowBuffer ((*rev_it).GetPointer ());
        BaseWindow *window = (*rev_it).GetPointer ();
        WindowNeedRedraw = window->IsRedrawNeeded();

        // Based on the areas that requested a rendering inside the BaseWindow, render the BaseWindow or just use its cache. 
        if(force_draw || window->IsRedrawNeeded() || window->ChildNeedsRedraw ())
        {
          if (rt.color_rt.IsValid() /*&& rt.depth_rt.IsValid()*/ && use_fbo)
          {
            t_s32 buffer_width = window->GetBaseWidth();
            t_s32 buffer_height = window->GetBaseHeight();

            if ( (rt.color_rt->GetWidth() != buffer_width) || (rt.color_rt->GetHeight() != buffer_height) )
            {
              rt.color_rt = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_R8G8B8A8);
              rt.depth_rt = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_D24S8);
            }

            m_FrameBufferObject->FormatFrameBufferObject (buffer_width, buffer_height, BITFMT_R8G8B8A8);
            m_FrameBufferObject->SetRenderTarget ( 0, rt.color_rt->GetSurfaceLevel (0) );
            m_FrameBufferObject->SetDepthSurface ( rt.depth_rt->GetSurfaceLevel (0) );
            m_FrameBufferObject->Activate();
            GetWindowThread ()->GetGraphicsEngine().SetViewport (0, 0, buffer_width, buffer_height);
            GetWindowThread ()->GetGraphicsEngine().SetOrthographicProjectionMatrix (buffer_width, buffer_height);
            GetWindowThread ()->GetGraphicsEngine().EmptyClippingRegion();
            GetWindowThread ()->GetGraphicsEngine().SetOpenGLClippingRectangle (0, 0, buffer_width, buffer_height);

            CHECKGL ( glClearColor (0, 0, 0, 0) );
            GLuint clear_color_buffer_bit = (force_draw || window->IsRedrawNeeded() ) ? GL_COLOR_BUFFER_BIT : 0;
            CHECKGL ( glClear (clear_color_buffer_bit | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT) );
          }
          else
          {
            int x = window->GetBaseX();
            int y = window->GetBaseY();
            Matrix4 mat;
            mat.Translate (x, y, 0);
            GetWindowThread ()->GetGraphicsEngine().SetOrthographicProjectionMatrix (GetWindowThread ()->GetGraphicsEngine().GetWindowWidth(),
                GetWindowThread ()->GetGraphicsEngine().GetWindowHeight() );

            //GetWindowThread ()->GetGraphicsEngine().Push2DModelViewMatrix(mat);
          }

          RenderTopViewContent (/*fbo,*/ window, force_draw);
        }
        
        if (rt.color_rt.IsValid() /*&& rt.depth_rt.IsValid()*/ && use_fbo)
        {
          // GetWindowThread ()->GetGraphicsEngine().EmptyClippingRegion();
          m_FrameBufferObject->Deactivate();

          // Enable this to render the drop shadow under windows: not perfect yet...
          if (0/*force_draw || WindowNeedRedraw*/)
          {
            unsigned int window_width, window_height;
            window_width = GetWindowThread ()->GetGraphicsEngine().GetWindowWidth();
            window_height = GetWindowThread ()->GetGraphicsEngine().GetWindowHeight();
            GetWindowThread ()->GetGraphicsEngine().EmptyClippingRegion();
            GetWindowThread ()->GetGraphicsEngine().SetOpenGLClippingRectangle (0, 0, window_width, window_height);
            GetWindowThread ()->GetGraphicsEngine().SetViewport (0, 0, window_width, window_height);
            GetWindowThread ()->GetGraphicsEngine().SetOrthographicProjectionMatrix (window_width, window_height);

            Geometry shadow (window->GetBaseX(), window->GetBaseY(), window->GetBaseWidth(), window->GetBaseHeight() );
            //if(window->IsVisibleSizeGrip())
            {
              shadow.OffsetPosition (4, 4);
              GetPainter().PaintShape (GetWindowThread ()->GetGraphicsEngine(), shadow, Color (0xFF000000), eSHAPE_CORNER_SHADOW);
            }
//                    else
//                    {
//                        shadow.OffsetPosition(4, 4);
//                        GetPainter().PaintShape(GetWindowThread ()->GetGraphicsEngine(), shadow, Color(0xFF000000), eSHAPE_CORNER_ROUND10_SHADOW);
//                    }
          }

          if (GetWindowThread ()->IsEmbeddedWindow() )
          {
            // In embedded mode, floating windows are composited over Nux main window which is probably empty. At least that
            // is how things are at the moment. Compiste the floating windows onto the main texture without blending.
            GetWindowThread ()->GetGraphicsEngine().GetRenderStates().SetBlend (false, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          }
          else
          {
            GetWindowThread ()->GetGraphicsEngine().GetRenderStates().SetBlend (true, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          }

          CHECKGL ( glDepthMask (GL_FALSE) );
          {
            //CopyTextureToCompositionRT(rt.color_rt, window->GetBaseX(), window->GetBaseY());
            GetWindowThread ()->GetGraphicsEngine().ApplyClippingRectangle();
            PresentBufferToScreen (rt.color_rt, window->GetBaseX(), window->GetBaseY(), false, false, window->GetOpacity ());
          }
          CHECKGL ( glDepthMask (GL_TRUE) );
          GetWindowThread ()->GetGraphicsEngine().GetRenderStates().SetBlend (false);
        }
        else
        {
//                int x = window->GetX();
//                int y = window->GetY();
//                Matrix4 mat;
//                mat.Translate(x, y, 0);
          //GetWindowThread ()->GetGraphicsEngine().SetContext (0, 0, 0, 0);
          //GetWindowThread ()->GetGraphicsEngine().Pop2DModelViewMatrix();
        }
        
        window->_child_need_redraw = false;
      }
      else
      {
        ObjectWeakPtr<BaseWindow> window = *rev_it;
        window->_child_need_redraw = false;
        window->DoneRedraw ();
      }
    }

    if (use_fbo)
      m_FrameBufferObject->Deactivate();
  }

// This function is meant to draw the main window and the small windows but it will not traverse the element.
// It just copies to the backbuffer what it already has in  the framebuffer objects.
  void WindowCompositor::PresentRendering()
  {
    m_FrameBufferObject->Deactivate();
    unsigned int window_width, window_height;
    window_width = GetWindowThread ()->GetGraphicsEngine().GetWindowWidth();
    window_height = GetWindowThread ()->GetGraphicsEngine().GetWindowHeight();
    GetWindowThread ()->GetGraphicsEngine().EmptyClippingRegion();
    //GetWindowThread ()->GetGraphicsEngine().SetOpenGLClippingRectangle(0, 0, window_width, window_height);
    GetWindowThread ()->GetGraphicsEngine().SetViewport (0, 0, window_width, window_height);
    GetWindowThread ()->GetGraphicsEngine().SetOrthographicProjectionMatrix (window_width, window_height);

    PresentBufferToScreen (m_MainColorRT, 0, 0, false);

    PageBBox page;
    page.xmin = 0;
    page.xmax = 100;
    page.ymin = 700;
    page.ymax = 730;
    page.x_margin = 0;
    page.y_margin = 0;

    NString FPS = NString::Printf (TEXT ("FPS: %3.2f"), GetWindowThread ()->GetFrameRate() );

    GetWindowThread ()->GetGraphicsEngine().RenderColorTextLineStatic (GetSysBoldFont(), page, FPS, Color (0xffff0000), true, eAlignTextLeft);

    GetWindowThread ()->GetGraphicsEngine().Pop2DWindow();
//     GetWindowThread ()->GetGraphicsEngine().ResetStats();
//     GetWindowThread ()->GetWindow().SwapBuffer();

//    const std::list<BaseWindow*>& W = _view_window_list;
//    std::list<BaseWindow*>::const_reverse_iterator rev_it;
//    for(rev_it = W.rbegin();
//        rev_it != W.rend();
//        rev_it++)
//    {
////        if((drawModal==false) && (*rev_it)->IsModal())
////            continue;
//
//        bool WindowNeedRedraw = false;
//        if((*rev_it)->IsVisible())
//        {
//
//            RenderTargetTextures& rt = GetWindowBuffer(*rev_it);
//            BaseWindow* window = *rev_it;
//            WindowNeedRedraw = window->IsRedrawNeeded();
//
//            if(rt.color_rt.IsValid())
//            {
//                CHECKGL( glEnable(GL_BLEND) );
//                CHECKGL( glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) );
//                CHECKGL( glDepthMask(GL_FALSE) );
//                {
//                    PresentBufferToScreen(rt.color_rt, window->GetX(), window->GetY());
//                }
//                CHECKGL( glDepthMask(GL_TRUE) );
//                CHECKGL( glDisable(GL_BLEND) );
//            }
//        }
//    }
  }

  void WindowCompositor::RenderMainWindowComposition (bool force_draw, bool UseFBO)
  {
    t_s32 buffer_width, buffer_height;

    if (UseFBO)
    {
      buffer_width = GetWindowThread ()->GetGraphicsEngine().GetWindowWidth();
      buffer_height = GetWindowThread ()->GetGraphicsEngine().GetWindowHeight();

      if ( (!m_MainColorRT.IsValid() ) || (!m_MainDepthRT.IsValid() ) || (m_MainColorRT->GetWidth() != buffer_width) || (m_MainColorRT->GetHeight() != buffer_height) )
      {
        m_MainColorRT = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_R8G8B8A8);
        m_MainDepthRT = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_D24S8);
      }

      m_FrameBufferObject->FormatFrameBufferObject (buffer_width, buffer_height, BITFMT_R8G8B8A8);
      m_FrameBufferObject->SetRenderTarget ( 0, m_MainColorRT->GetSurfaceLevel (0) );
      m_FrameBufferObject->SetDepthSurface ( m_MainDepthRT->GetSurfaceLevel (0) );
      m_FrameBufferObject->Activate();
    }
    else
    {
      buffer_width = GetWindowThread ()->GetGraphicsEngine().GetWindowWidth();
      buffer_height = GetWindowThread ()->GetGraphicsEngine().GetWindowHeight();
    }

    GetWindowThread ()->GetGraphicsEngine().EmptyClippingRegion();
    GetWindowThread ()->GetGraphicsEngine().SetOpenGLClippingRectangle (0, 0, buffer_width, buffer_height);
    GetWindowThread ()->GetGraphicsEngine().SetViewport (0, 0, buffer_width, buffer_height);
    GetWindowThread ()->GetGraphicsEngine().SetOrthographicProjectionMatrix (buffer_width, buffer_height);
    {
      CHECKGL ( glClear (/*GL_COLOR_BUFFER_BIT |*/ GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT) );
      //Begin 2D Drawing
      {
        if (force_draw)
        {
          GetPainter().PushDrawLayer (GetWindowThread ()->GetGraphicsEngine(), Geometry (0, 0, buffer_width, buffer_height), m_Background);
          //GetPainter().PushBackground(GetWindowThread ()->GetGraphicsEngine(), Geometry(0, 0, buffer_width, buffer_height), m_Background, true);

          GetWindowThread ()->ProcessDraw (GetWindowThread ()->GetGraphicsEngine(), true);

          nuxAssert (GetWindowThread ()->GetGraphicsEngine().GetNumberOfClippingRegions() == 0);
          GetPainter().PopBackground();
          GetPainter().EmptyBackgroundStack();
        }
        else
        {
          GetPainter().PushLayer (GetWindowThread ()->GetGraphicsEngine(), Geometry (0, 0, buffer_width, buffer_height), m_Background);
          //GetPainter().PushBackground(GetWindowThread ()->GetGraphicsEngine(), Geometry(0, 0, buffer_width, buffer_height), m_Background, false);

          GetWindowThread ()->ProcessDraw (GetWindowThread ()->GetGraphicsEngine(), false);

          nuxAssert (GetWindowThread ()->GetGraphicsEngine().GetNumberOfClippingRegions() == 0);
          GetPainter().PopBackground();
          GetPainter().EmptyBackgroundStack();
        }
      }
      // End 2D Drawing
    }

    GetWindowThread ()->GetGraphicsEngine().SetOrthographicProjectionMatrix (buffer_width, buffer_height);
    if (UseFBO)
    {
      m_FrameBufferObject->Deactivate();
    }

    unsigned int window_width, window_height;
    window_width = GetWindowThread ()->GetGraphicsEngine().GetWindowWidth();
    window_height = GetWindowThread ()->GetGraphicsEngine().GetWindowHeight();
    GetWindowThread ()->GetGraphicsEngine().EmptyClippingRegion();
    GetWindowThread ()->GetGraphicsEngine().SetOpenGLClippingRectangle (0, 0, window_width, window_height);
    GetWindowThread ()->GetGraphicsEngine().SetViewport (0, 0, window_width, window_height);
    GetWindowThread ()->GetGraphicsEngine().SetOrthographicProjectionMatrix (window_width, window_height);

    // Render the Buffer on the screen
    if (UseFBO)
    {
      PresentBufferToScreen (m_MainColorRT, 0, 0, false);
    }
  }

  void WindowCompositor::SetMainColorRT()
  {
    t_s32 buffer_width, buffer_height;
    buffer_width = GetWindowThread ()->GetGraphicsEngine().GetWindowWidth();
    buffer_height = GetWindowThread ()->GetGraphicsEngine().GetWindowHeight();

    if ( (!m_MainColorRT.IsValid() ) || (m_MainColorRT->GetWidth() != buffer_width) || (m_MainColorRT->GetHeight() != buffer_height) )
    {
      m_MainColorRT = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_R8G8B8A8);
    }

    // Setup the Composition Render Target
    m_FrameBufferObject->FormatFrameBufferObject (buffer_width, buffer_height, BITFMT_R8G8B8A8);
    m_FrameBufferObject->SetRenderTarget (0, m_MainColorRT->GetSurfaceLevel (0));
    m_FrameBufferObject->SetDepthSurface (ObjectPtr<IOpenGLSurface> (0));
    m_FrameBufferObject->Activate();
    GetWindowThread ()->GetGraphicsEngine().SetOrthographicProjectionMatrix (buffer_width, buffer_height);
    GetWindowThread ()->GetGraphicsEngine().EmptyClippingRegion();
    GetWindowThread ()->GetGraphicsEngine().SetOpenGLClippingRectangle (0, 0, buffer_width, buffer_height);
  }

  void WindowCompositor::CopyTextureToMainColorRT (ObjectPtr<IOpenGLBaseTexture> HWTexture, int x, int y)
  {
    SetMainColorRT();
    HWTexture->SetFiltering (GL_NEAREST, GL_NEAREST);
    HWTexture->BindTextureToUnit (GL_TEXTURE0);
    GetWindowThread ()->GetGraphicsEngine().DisableAllTextureMode (GL_TEXTURE1);

    int TexWidth = HWTexture->GetWidth();
    int TexHeight = HWTexture->GetHeight();

    TexCoordXForm texxform;
    texxform.FlipVCoord (true);
    texxform.uscale = 1.0f;
    texxform.vscale = 1.0f;
    texxform.uwrap = TEXWRAP_REPEAT;
    texxform.vwrap = TEXWRAP_REPEAT;
    GetWindowThread ()->GetGraphicsEngine().QRP_1Tex (x, y, TexWidth, TexHeight, HWTexture, texxform, color::White);
  }

  void WindowCompositor::SetCompositionRT()
  {
    t_s32 buffer_width, buffer_height;
    buffer_width = GetWindowThread ()->GetGraphicsEngine().GetWindowWidth();
    buffer_height = GetWindowThread ()->GetGraphicsEngine().GetWindowHeight();

    if ( (!m_CompositionRT.IsValid() ) || (m_CompositionRT->GetWidth() != buffer_width) || (m_CompositionRT->GetHeight() != buffer_height) )
    {
      m_CompositionRT = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_R8G8B8A8);
    }

    // Setup the Composition Render Target
    m_FrameBufferObject->FormatFrameBufferObject (buffer_width, buffer_height, BITFMT_R8G8B8A8);
    m_FrameBufferObject->SetRenderTarget (0, m_CompositionRT->GetSurfaceLevel (0));
    m_FrameBufferObject->SetDepthSurface (ObjectPtr<IOpenGLSurface> (0));
    m_FrameBufferObject->Activate();
    GetWindowThread ()->GetGraphicsEngine().SetOrthographicProjectionMatrix (buffer_width, buffer_height);
    GetWindowThread ()->GetGraphicsEngine().EmptyClippingRegion ();
    GetWindowThread ()->GetGraphicsEngine().SetOpenGLClippingRectangle (0, 0, buffer_width, buffer_height);
  }

  void WindowCompositor::CopyTextureToCompositionRT (ObjectPtr<IOpenGLBaseTexture> HWTexture, int x, int y)
  {
    SetCompositionRT();
    HWTexture->SetFiltering (GL_NEAREST, GL_NEAREST);
    HWTexture->BindTextureToUnit (GL_TEXTURE0);
    GetWindowThread ()->GetGraphicsEngine().DisableAllTextureMode (GL_TEXTURE1);

    int TexWidth = HWTexture->GetWidth();
    int TexHeight = HWTexture->GetHeight();

    TexCoordXForm texxform;
    texxform.FlipVCoord (true);
    texxform.uscale = 1.0f;
    texxform.vscale = 1.0f;
    texxform.uwrap = TEXWRAP_REPEAT;
    texxform.vwrap = TEXWRAP_REPEAT;
    GetWindowThread ()->GetGraphicsEngine().QRP_1Tex (x, y, TexWidth, TexHeight, HWTexture, texxform, color::White);
  }

  void WindowCompositor::PresentBufferToScreen (ObjectPtr<IOpenGLBaseTexture> HWTexture, int x, int y, bool RenderToMainTexture, bool BluredBackground, float opacity)
  {
    nuxAssert (HWTexture.IsValid() );

    if (HWTexture.IsNull() )
      return;

    t_s32 window_width, window_height;
    window_width = GetWindowThread ()->GetGraphicsEngine().GetWindowWidth();
    window_height = GetWindowThread ()->GetGraphicsEngine().GetWindowHeight();


    if (RenderToMainTexture && (HWTexture != m_MainColorRT) )
    {
      nuxAssert (m_MainColorRT->GetWidth() == window_width);
      nuxAssert (m_MainColorRT->GetHeight() == window_height);
      m_FrameBufferObject->FormatFrameBufferObject (window_width, window_height, BITFMT_R8G8B8A8);
      m_FrameBufferObject->SetRenderTarget ( 0, m_MainColorRT->GetSurfaceLevel (0) );
      m_FrameBufferObject->SetDepthSurface ( m_MainDepthRT->GetSurfaceLevel (0) );
      m_FrameBufferObject->Activate();
    }
    else
    {
      GetGraphicsDisplay()->GetGpuDevice()->DeactivateFrameBuffer();
    }

    GetWindowThread()->GetGraphicsEngine().EmptyClippingRegion();
    GetWindowThread()->GetGraphicsEngine().SetOpenGLClippingRectangle (0, 0, window_width, window_height);
    GetWindowThread()->GetGraphicsEngine().SetViewport (0, 0, window_width, window_height);
    GetWindowThread()->GetGraphicsEngine().SetOrthographicProjectionMatrix (window_width, window_height);

    // Render the MAINFBO
    {
      int src_width, src_height;
      src_width = HWTexture->GetWidth();
      src_height = HWTexture->GetHeight();

      TexCoordXForm texxform0;
      texxform0.FlipVCoord (true);

      if (GetWindowThread()->IsEmbeddedWindow())
      {
        // Compose Nux's main texture onto another surface (a texture or the back buffer) with blending.
        GetWindowThread ()->GetGraphicsEngine().GetRenderStates().SetBlend (true, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      }

      GetGraphicsDisplay()->GetGraphicsEngine()->QRP_1Tex (x, y, src_width, src_height, HWTexture, texxform0, Color (1.0f, 1.0f, 1.0f, opacity));
      GetWindowThread ()->GetGraphicsEngine().GetRenderStates().SetBlend (false);
    }
  }

  void WindowCompositor::AddMenu(MenuPage *menu, BaseWindow *window, bool OverrideCurrentMenuChain)
  {
    if(_menu_chain->size() == 0)
    {
      // A menu is opening.
      _starting_menu_event_cycle = true;
      _menu_is_active = true;
    }

    std::list<MenuPage*>::iterator it = find(_menu_chain->begin(), _menu_chain->end(), menu);
    if(it == _menu_chain->end())
    {
      // When adding a MenuPage, make sure that it is a child of the MenuPage in _menu_chain->begin().
      if (_menu_chain->size() )
      {
        if (menu->GetParentMenu() != (*_menu_chain->begin() ) )
        {
          if (OverrideCurrentMenuChain)
          {
            // Remove the current menu chain
            for (it = _menu_chain->begin(); it != _menu_chain->end(); it++)
            {
              // Stop all pages
              (*it)->StopMenu();
            }

            _menu_chain->clear();
          }
          else
          {
            // The MenuPage argument is not a descendent of the current menu chain.
            return;
          }
        }
      }

      m_MenuWindow = window;
      // The deepest menu is added in front of the list and tested first for events.
      _menu_chain->push_front (menu);
    }
  }

  // Be careful never call this function while you are iterating through the elements of _menu_chain.
  void WindowCompositor::RemoveMenu (MenuPage *menu)
  {
    std::list<MenuPage *>::iterator it = find(_menu_chain->begin(), _menu_chain->end(), menu);

    if(it == _menu_chain->end())
    {
      return;
    }

    _menu_chain->erase (it);
    m_MenuRemoved = true;

    if(_menu_is_active && (_menu_chain->size() == 0))
    {
      // The menu is closed
      _menu_is_active         = false;
      ResetMousePointerAreas();
      m_MenuWindow            = NULL;
    }
  }

  void WindowCompositor::CleanMenu()
  {
    if (_menu_chain->size() == 0)
      return;

    std::list<MenuPage *>::iterator menu_it = _menu_chain->begin();

    while (menu_it != _menu_chain->end() )
    {
      if ( (*menu_it)->IsActive() == false)
      {
        if (GetMouseFocusArea () == (*menu_it))
        {
          SetMouseFocusArea (NULL);
        }
        if (GetMouseOverArea () == (*menu_it))
        {
          SetMouseOverArea (NULL);
        }
        if (GetPreviousMouseOverArea () == (*menu_it))
        {
          SetPreviousMouseOverArea (NULL);
        }                
 
        menu_it = _menu_chain->erase (menu_it);
        m_MenuRemoved = true;
      }
      else
      {
        menu_it++;
      }
    }

    if (_menu_is_active && (_menu_chain->size() == 0))
    {
      _menu_is_active         = false;
      ResetMousePointerAreas();
      m_MenuWindow            = NULL;
    }
  }

  void WindowCompositor::SetWidgetDrawingOverlay (InputArea *ic, BaseWindow* OverlayWindow)
  {
    OverlayDrawingCommand = ic;
    m_OverlayWindow = OverlayWindow;
  }

  InputArea *WindowCompositor::GetWidgetDrawingOverlay()
  {
    return OverlayDrawingCommand;
  }

  void WindowCompositor::SetTooltip(InputArea* TooltipArea, const TCHAR *TooltipText, int x, int y)
  {
    _tooltip_window = GetProcessingTopView();
    m_TooltipArea = TooltipArea;
    m_TooltipText = TooltipText;
    m_TooltipX = x;
    m_TooltipY = y;

    if(m_TooltipText.Size())
    {
      int w = GetSysBoldFont()->GetCharStringWidth(m_TooltipText.GetTCharPtr());
      int h = GetSysBoldFont()->GetFontHeight();

      _tooltip_text_geometry = Geometry(
          m_TooltipX + 10,
          m_TooltipY - h/2 - 2,
          w,
          h + 4);

      _tooltip_geometry = _tooltip_text_geometry;
      _tooltip_geometry.OffsetSize(20, 8);
      _tooltip_geometry.OffsetPosition(-10, -4);

      _tooltip_mainwindow_geometry = _tooltip_geometry;

      if(_tooltip_window.IsValid())
      {
        _tooltip_mainwindow_geometry.OffsetPosition(_tooltip_window->GetBaseX(), _tooltip_window->GetBaseY());
      }
    }
    else
    {
      _tooltip_mainwindow_geometry = _tooltip_geometry = _tooltip_text_geometry = Geometry(0, 0, 0, 0);
    }
  }

  Geometry WindowCompositor::GetTooltipGeometry() const
  {
    return _tooltip_geometry;
  }

  Geometry WindowCompositor::GetTooltipMainWindowGeometry() const
  {
      return _tooltip_mainwindow_geometry;
  }

  void WindowCompositor::CancelTooltip()
  {
    _tooltip_window = NULL;
    m_TooltipArea = NULL;
    m_TooltipText = TEXT ("");
  }

  bool WindowCompositor::ValidateMouseInsideTooltipArea(int x, int y)
  {
    NUX_RETURN_VALUE_IF_FALSE(m_TooltipArea, false);
    NUX_RETURN_VALUE_IF_FALSE(_tooltip_window.IsValid(), false);

    Geometry geo = m_TooltipArea->GetGeometry();
    geo.OffsetPosition(_tooltip_window->GetBaseX(), _tooltip_window->GetBaseY());

    return geo.IsPointInside(x, y);
  }

  bool WindowCompositor::IsTooltipActive()
  {
    NUX_RETURN_VALUE_IF_FALSE(m_TooltipArea, false);
    return true;
  }

  void WindowCompositor::SetMouseFocusArea (InputArea *area)
  {
    _mouse_focus_area = area;

    if (area == 0)
    {
      _event_root = Point(0, 0);
    }
    else
    {
      _mouse_focus_area_conn.disconnect ();
      _mouse_focus_area_conn = area->OnDestroyed.connect (sigc::mem_fun (this, &WindowCompositor::OnMouseFocusAreaDestroyed));
    }

    if (area)
      SetFocusAreaWindow (GetProcessingTopView ());
    else
      SetFocusAreaWindow (NULL);
  }

  InputArea *WindowCompositor::GetMouseFocusArea()
  {
    return _mouse_focus_area;
  }

  void WindowCompositor::SetMouseOverArea (InputArea *area)
  {
    _mouse_over_area = area;

    _mouse_over_area_conn.disconnect ();
    if (area)
      _mouse_over_area_conn = area->OnDestroyed.connect (sigc::mem_fun (this, &WindowCompositor::OnMouseOverAreaDestroyed));
//     if(_mouse_over_area)
//         nuxDebugMsg(TEXT("StackManager: Set MouseOver Area"));
//     else
//         nuxDebugMsg(TEXT("StackManager: Set MouseOver Area to 0"));
  }

  InputArea *WindowCompositor::GetMouseOverArea()
  {
    return _mouse_over_area;
  }

  void WindowCompositor::SetPreviousMouseOverArea (InputArea *area)
  {
    _previous_mouse_over_area = area;

    _previous_mouse_over_area_conn.disconnect ();
    if (area)
      _previous_mouse_over_area_conn = area->OnDestroyed.connect (sigc::mem_fun (this, &WindowCompositor::OnPreviousMouseOverAreaDestroyed));
//     if(area)
//         nuxDebugMsg(TEXT("StackManager: Set Previous MouseOver Area"));
//     else
//         nuxDebugMsg(TEXT("StackManager: Set Previous MouseOver Area to 0"));
  }

  InputArea *WindowCompositor::GetPreviousMouseOverArea()
  {
    return _previous_mouse_over_area;
  }


  void WindowCompositor::OnKeyboardEventReceiverDestroyed (Object* area)
  {
    if (_keyboard_event_receiver == area)
    {
      _keyboard_event_receiver = NULL;
    }
  }
  
  void WindowCompositor::SetKeyboardEventReceiver(InputArea *area)
  {
    InputArea* keyboard_grab_area = GetKeyboardGrabArea();

    if (keyboard_grab_area && (area != keyboard_grab_area))
    {
      // The area that has the keyboard grab has the priority. Disregard the keyboard focus area.
      nuxDebugMsg("[WindowCompositor::SetKeyboardEventReceiver] There is a keyboard grab pending. Cannot change the keyboard event receiver.")
      return;
    }

    if (_keyboard_event_receiver == area)
    {
        return;
    }

    if (_keyboard_event_receiver)
    {
      _keyboard_event_receiver->OnStopKeyboardReceiver.emit ();
    }

    _keyboard_event_receiver = area;

    if (_keyboard_event_receiver)
    {
      _keyboard_event_receiver->OnStartKeyboardReceiver.emit ();
    }


    _keyboard_event_receiver_conn.disconnect ();
    if (area)
      _keyboard_event_receiver_conn = area->OnDestroyed.connect (sigc::mem_fun (this, &WindowCompositor::OnKeyboardEventReceiverDestroyed));
  }

  InputArea* WindowCompositor::GetKeyboardEventReceiver ()
  {
    return _keyboard_event_receiver;
  }

  void WindowCompositor::SetBackgroundPaintLayer (AbstractPaintLayer *bkg)
  {
    NUX_SAFE_DELETE (m_Background);
    m_Background = bkg->Clone();
  }

  void WindowCompositor::FloatingAreaConfigureNotify (int Width, int Height)
  {
    std::list< ObjectWeakPtr<BaseWindow> >::iterator it;

    for (it = _view_window_list.begin(); it != _view_window_list.end(); it++)
    {
      if (!(*it).IsValid ())
        continue;
      if ( (*it)->IsVisible() )
      {
        (*it)->NotifyConfigurationChange (Width, Height);
      }
    }
  }

  void WindowCompositor::FormatRenderTargets (int width, int height)
  {
    int buffer_width = GetWindowThread ()->GetGraphicsEngine().GetWindowWidth();
    int buffer_height = GetWindowThread ()->GetGraphicsEngine().GetWindowHeight();

    nuxAssert (buffer_width >= 1);
    nuxAssert (buffer_height >= 1);

    m_MainColorRT = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_R8G8B8A8);
    m_CompositionRT = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_R8G8B8A8);
    m_MainDepthRT = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_D24S8);

    // Clear the buffer the first time...
    m_FrameBufferObject->FormatFrameBufferObject (buffer_width, buffer_height, BITFMT_R8G8B8A8);
    m_FrameBufferObject->SetRenderTarget ( 0, m_MainColorRT->GetSurfaceLevel (0) );
    m_FrameBufferObject->SetDepthSurface ( m_MainDepthRT->GetSurfaceLevel (0) );
    m_FrameBufferObject->Activate();

    CHECKGL ( glClearColor (0.0f, 0.0f, 0.0f, 0.0f) );
    CHECKGL ( glClearDepth (1.0f) );
    CHECKGL ( glClearStencil (0) );
    CHECKGL ( glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT) );
    m_FrameBufferObject->Deactivate();
    CHECKGL ( glClearColor (0.0f, 0.0f, 0.0f, 0.0f) );
    CHECKGL ( glClearDepth (1.0f) );
    CHECKGL ( glClearStencil (0) );
    CHECKGL ( glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT) );

//     m_BlurTexture   = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (Max (buffer_width, 1), Max (buffer_height, 1), 1, BITFMT_R8G8B8A8);
//     m_FullSceneMip0 = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (Max (buffer_width / 2, 1), Max (buffer_height / 2, 1), 1, BITFMT_R8G8B8A8);
//     m_FullSceneMip1 = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (Max (buffer_width / 4, 1), Max (buffer_height / 4, 1), 1, BITFMT_R8G8B8A8);
//     m_FullSceneMip2 = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (Max (buffer_width / 8, 1), Max (buffer_height / 8, 1), 1, BITFMT_R8G8B8A8);
  }

  void WindowCompositor::RestoreRenderingSurface ()
  {
    BaseWindow *top_view = GetProcessingTopView ();

    if (top_view && _inside_rendering_cycle)
    {
      nuxAssert (top_view->Type ().IsDerivedFromType (BaseWindow::StaticObjectType));

      RenderTargetTextures rt = GetWindowBuffer (top_view);

      int buffer_width = top_view->GetBaseWidth();
      int buffer_height = top_view->GetBaseHeight();

      nuxAssert (buffer_width >= 1);
      nuxAssert (buffer_height >= 1);

      if ((rt.color_rt->GetWidth () != buffer_width) || (rt.color_rt->GetHeight () != buffer_height))
      {
        rt.color_rt = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_R8G8B8A8);
        rt.depth_rt = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_D24S8);
      }

      m_FrameBufferObject->FormatFrameBufferObject (buffer_width, buffer_height, BITFMT_R8G8B8A8);
      m_FrameBufferObject->SetRenderTarget (0, rt.color_rt->GetSurfaceLevel (0));
      m_FrameBufferObject->SetDepthSurface (rt.depth_rt->GetSurfaceLevel (0));
      m_FrameBufferObject->Activate ();

      GetWindowThread ()->GetGraphicsEngine ().SetViewport (0, 0, buffer_width, buffer_height);
      GetWindowThread ()->GetGraphicsEngine ().SetOrthographicProjectionMatrix (buffer_width, buffer_height);
      GetWindowThread ()->GetGraphicsEngine ().ApplyClippingRectangle ();
    }
    else
    {
      int buffer_width = GetWindowThread ()->GetGraphicsEngine ().GetWindowWidth ();
      int buffer_height = GetWindowThread ()->GetGraphicsEngine ().GetWindowHeight ();

      nuxAssert (buffer_width >= 1);
      nuxAssert (buffer_height >= 1);
      // Restore Main Frame Buffer
      m_FrameBufferObject->FormatFrameBufferObject (buffer_width, buffer_height, BITFMT_R8G8B8A8);
      m_FrameBufferObject->SetRenderTarget (0, m_CompositionRT->GetSurfaceLevel (0));
      m_FrameBufferObject->SetDepthSurface (ObjectPtr<IOpenGLSurface> (0));
      m_FrameBufferObject->Activate ();

      GetWindowThread ()->GetGraphicsEngine ().SetViewport (0, 0, buffer_width, buffer_height);
      GetWindowThread ()->GetGraphicsEngine ().SetOrthographicProjectionMatrix (buffer_width, buffer_height);
      GetWindowThread ()->GetGraphicsEngine ().ApplyClippingRectangle ();
    }
  }

  void* WindowCompositor::GetBackupTextureData (BaseWindow *base_window, int &width, int &height, int &format)
  {
    width = height = format = 0;

    NUX_RETURN_VALUE_IF_NULL (base_window, 0);

    std::map< BaseWindow*, struct RenderTargetTextures >::iterator it;

    it = _window_to_texture_map.find (base_window);

    if (it == _window_to_texture_map.end ())
    {
      return 0;
    }

    if ((*it).second.color_rt.IsNull ())
    {
      return 0;
    }

    return (*it).second.color_rt->GetSurfaceData (0, width, height, format);
  }


  void WindowCompositor::ResetDnDArea()
  {
    SetDnDArea (NULL);
  }

  void WindowCompositor::SetDnDArea (InputArea* area)
  {
#if defined (NUX_OS_LINUX)
    if (_dnd_area == area)
      return;

    if (_dnd_area)
    {
      _dnd_area->HandleDndLeave ();
      _dnd_area->UnReference ();
    }
    _dnd_area = area;
    
    if (_dnd_area)
    {
      _dnd_area->Reference ();
      _dnd_area->HandleDndEnter ();
    }
#endif
  }

  InputArea* WindowCompositor::GetDnDArea ()
  {
    return _dnd_area;
  }


  bool WindowCompositor::GrabPointerAdd (InputArea* area)
  {
    NUX_RETURN_VALUE_IF_NULL (area, false);
    bool result = true;

    if (GetPointerGrabArea () == area)
    {
      nuxDebugMsg (TEXT ("[WindowCompositor::GrabPointerAdd] The area already has the grab"));
      return result;
    }
    
    if (GetWindow ().PointerGrabData () != this)
      result = GetWindow ().GrabPointer (NULL, this, true);

    if (result)
      _pointer_grab_stack.push_front (area);

    // reset the mouse pointers areas.
    ResetMousePointerAreas();

    return result;
  }

  bool WindowCompositor::GrabPointerRemove (InputArea* area)
  {
    NUX_RETURN_VALUE_IF_NULL (area, false);

    std::list<InputArea*>::iterator it;

    // find the first instance of the area pointer in the stack
    it = find (_pointer_grab_stack.begin(), _pointer_grab_stack.end(), area);

    if (it == _pointer_grab_stack.end ())
      return false;

    _pointer_grab_stack.erase (it);
    
    if (_pointer_grab_stack.empty ())
      GetWindow ().UngrabPointer (this);
    
    // reset the mouse pointers areas.
    ResetMousePointerAreas();

    return true;
  }

  bool WindowCompositor::IsInPointerGrabStack (InputArea* area)
  {
    NUX_RETURN_VALUE_IF_NULL (area, false);

    std::list<InputArea*>::iterator it;
    it = find (_pointer_grab_stack.begin(), _pointer_grab_stack.end(), area);

    if (it == _pointer_grab_stack.end ())
      return false;

    return true;
  }

  InputArea* WindowCompositor::GetPointerGrabArea ()
  {
    if (_pointer_grab_stack.empty ())
      return NULL;

    return (*_pointer_grab_stack.begin ());
  }

  //////////////////////////
  bool WindowCompositor::GrabKeyboardAdd (InputArea* area)
  {
    NUX_RETURN_VALUE_IF_NULL (area, false);
    bool result = true;

    if (GetKeyboardGrabArea () == area)
    {
      nuxDebugMsg (TEXT ("[WindowCompositor::GrabKeyboardAdd] The area already has the grab"));
      return result;
    }

    
    if (GetWindow ().KeyboardGrabData () != this)
      result = GetWindow ().GrabKeyboard (NULL, this, true);
    
    if (result)
    {
      _keyboard_grab_stack.push_front (area);
      
      // Must be called only after the area has been added to the front of _keyboard_grab_stack.
      SetKeyboardEventReceiver (area);
    }
    
    return result;
  }

  bool WindowCompositor::GrabKeyboardRemove (InputArea* area)
  {
    NUX_RETURN_VALUE_IF_NULL (area, false);

    std::list<InputArea*>::iterator it;

    // find the first instance of the area keyboard in the stack
    it = find (_keyboard_grab_stack.begin(), _keyboard_grab_stack.end(), area);

    if (it == _keyboard_grab_stack.end ())
      return false;

    _keyboard_grab_stack.erase (it);
    if (_keyboard_grab_stack.empty ())
      GetWindow ().UngrabKeyboard (this);

    // Must be called only after the area has been added to the front of _keyboard_grab_stack.
    if (_keyboard_grab_stack.empty ())
    {
      SetKeyboardEventReceiver (NULL);
    }
    else
    {
      it = _keyboard_grab_stack.begin ();
      SetKeyboardEventReceiver (*it);
    }

    
    return true;
  }

  bool WindowCompositor::IsInKeyboardGrabStack (InputArea* area)
  {
    NUX_RETURN_VALUE_IF_NULL (area, false);

    std::list<InputArea*>::iterator it;
    it = find (_keyboard_grab_stack.begin(), _keyboard_grab_stack.end(), area);

    if (it == _keyboard_grab_stack.end ())
      return false;

    return true;
  }

  InputArea* WindowCompositor::GetKeyboardGrabArea ()
  {
    if (_keyboard_grab_stack.size () == 0)
      return NULL;

    return (*_keyboard_grab_stack.begin ());
  }
}

