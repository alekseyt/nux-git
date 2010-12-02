/*
 * Copyright 2010 Inalogic Inc.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Jay Taoko <jay.taoko_AT_gmail_DOT_com>
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
    m_MouseFocusArea            = NULL;
    m_MouseOverArea             = NULL;
    m_PreviousMouseOverArea     = NULL;
    m_CurrentEvent              = NULL;
    m_CurrentWindow             = NULL;
    m_FocusAreaWindow           = NULL;
    m_MenuWindow                = NULL;
    m_OverlayWindow             = NULL;
    _tooltip_window             = NULL;
    m_TooltipArea               = NULL;
    m_ModalWindow               = NULL;
    m_SelectedWindow            = NULL;
    m_MenuList                  = NULL;
    m_Background                = NULL;
    _tooltip_window             = NULL;
    m_OverlayWindow             = NULL;
    OverlayDrawingCommand       = NULL;
    m_CurrentWindow             = NULL;
    m_FocusAreaWindow           = NULL;
    m_MenuWindow                = NULL;
    m_CurrentEvent              = NULL;
    m_MouseFocusArea            = NULL;
    m_MouseOverArea             = NULL;
    m_PreviousMouseOverArea     = NULL;
    _always_on_front_window     = NULL;
    _inside_event_processing    = false;
    _exclusive_input_area       = NULL;
    _in_exclusive_input_mode    = false;
    _pending_exclusive_input_mode_action = false;

    if (GetGraphicsThread()->GetWindow().HasFrameBufferSupport() )
    {
      m_FrameBufferObject = GetThreadGLDeviceFactory()->CreateFrameBufferObject();
      // Do not leave the Fbo binded. Deactivate it.
      m_FrameBufferObject->Deactivate();
    }

    // At this stage, the size of the window may not be known yet.
    // FormatRenderTargets will be called the first time runtime gets into WindowThread::ExecutionLoop
    m_MainColorRT = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (2, 2, 1, BITFMT_R8G8B8A8);
    m_MainDepthRT = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (2, 2, 1, BITFMT_D24S8);

    m_MenuList = new std::list<MenuPage *>;
    m_PopupRemoved = false;
    m_MenuRemoved = false;
    m_ModalWindow = NULL;
    m_Background = new ColorLayer (Color (0xFF4D4D4D) );
  }

  WindowCompositor::~WindowCompositor()
  {
    m_WindowToTextureMap.clear();
    m_FrameBufferObject.Release ();
    m_MainColorRT.Release ();
    m_MainDepthRT.Release ();
    m_MenuList->clear();

    std::list< IntrusiveWeakSP<BaseWindow> >::iterator it;
    for(it = m_WindowList.begin (); it != m_WindowList.end (); it++)
    {
      //(*it)->UnReference();
    }
    m_WindowList.clear ();

    //for(it = m_ModalWindowList.begin (); it != m_ModalWindowList.end (); it++)
    //{
    //  (*it)->UnReference();
    //}
    m_ModalWindowList.clear ();

    NUX_SAFE_DELETE (m_MenuList);
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
    std::map< BaseWindow*, RenderTargetTextures >::iterator it = m_WindowToTextureMap.find (window);

    if (it != m_WindowToTextureMap.end())
    {
      return (*it).second;
    }

    return ret;
  }

  void WindowCompositor::RegisterWindow (BaseWindow *window)
  {
    if (window == 0)
      return;

    std::list< IntrusiveWeakSP<BaseWindow> >::iterator it = find (m_WindowList.begin(), m_WindowList.end(), window);

    if (it == m_WindowList.end() )
    {
      // The BaseWindow is referenced by the WindowCompositor.
      //window->Reference();
      m_WindowList.push_front (IntrusiveWeakSP<BaseWindow> (window));
      m_SelectedWindow = window;

      RenderTargetTextures rt;

      // Don't size the texture to the dimension of the window yet. this will be done later.
      if (GetGraphicsThread()->GetWindow().HasFrameBufferSupport() )
      {
        rt.color_rt = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (2, 2, 1, BITFMT_R8G8B8A8);
        rt.depth_rt = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (2, 2, 1, BITFMT_D24S8);
      }

      m_WindowToTextureMap.insert ( std::map< BaseWindow*, RenderTargetTextures >::value_type ( window, rt) );
    }
  }

  void WindowCompositor::UnRegisterWindow (BaseWindow *window)
  {
    if (window == 0)
      return;

    std::list< IntrusiveWeakSP<BaseWindow> >::iterator it = find (m_WindowList.begin(), m_WindowList.end(), window);

    if (it != m_WindowList.end ())
    {
      m_WindowList.erase (it); // @see STL for note about list.erase(it++). It is valid for lists.

      if (m_WindowList.size ())
        m_SelectedWindow = (*m_WindowList.begin ());

      std::map< BaseWindow*, RenderTargetTextures >::iterator it2 = m_WindowToTextureMap.find (window);

      if (it2 != m_WindowToTextureMap.end ())
      {
        (*it2).second.color_rt = IntrusiveSP<IOpenGLBaseTexture> (0);
        (*it2).second.depth_rt = IntrusiveSP<IOpenGLBaseTexture> (0);
        m_WindowToTextureMap.erase (it2);
      }
    }
  }

  long WindowCompositor::ProcessEventOnObject (IEvent &ievent, Area *object, long TraverseInfo, long ProcessEventInfo)
  {
    if (object == 0)
      return 0;

    long ret = 0;
//    if(GetFocusAreaWindow())
//    {
//        ievent.e_x_root = GetFocusAreaWindow()->GetBaseX();
//        ievent.e_y_root = GetFocusAreaWindow()->GetBaseY();
//    }
//    else
    {
      ievent.e_x_root = m_EventRoot.x; //GetFocusAreaWindow()->GetBaseX();
      ievent.e_y_root = m_EventRoot.y; //GetFocusAreaWindow()->GetBaseY();
    }

    if (object->Type().IsDerivedFromType (View::StaticObjectType))
    {
      View *ic = NUX_STATIC_CAST (View *, object);
      ret = ic->BaseProcessEvent (ievent, TraverseInfo, ProcessEventInfo);
    }
    else if (object->Type().IsObjectType (InputArea::StaticObjectType))
    {
      InputArea *base_area = NUX_STATIC_CAST (InputArea *, object);
      ret = base_area->OnEvent (ievent, TraverseInfo, ProcessEventInfo);
    }
    else if (object->Type().IsDerivedFromType (Layout::StaticObjectType))
    {
      Layout *layout = NUX_STATIC_CAST (Layout *, object);
      ret = layout->ProcessEvent (ievent, TraverseInfo, ProcessEventInfo);
    }
    else
    {
      nuxAssertMsg (0, TEXT ("This should not happen"));
    }

    return ret;
  }

  // NUXTODO: rename as EventCycle
  void WindowCompositor::ProcessEvent (IEvent &ievent)
  {
    // Event processing cycle begins.
    _inside_event_processing = true;
    _pending_exclusive_input_mode_action = false;

    long exclusive_event_cycle_report = 0;
    if (_in_exclusive_input_mode && _exclusive_input_area)
    {
      exclusive_event_cycle_report = ProcessEventOnObject (ievent, _exclusive_input_area, 0, EVENT_CYCLE_EXCLUSIVE);
    }


    if (_in_exclusive_input_mode && (!(exclusive_event_cycle_report & EVENT_CYCLE_EXCLUSIVE_CONTINUE)))
    {
      _inside_event_processing = false;
      return;
    }

    long ret = 0;
    bool base_window_reshuffling = false; // Will be set to true if the BaseWindow are reshuffled.

    std::list<MenuPage *>::iterator menu_it;

    if (ievent.e_event == NUX_WINDOW_EXIT_FOCUS)
    {
      SetCurrentEvent (&ievent);

      if (GetMouseFocusArea ())
        ProcessEventOnObject (ievent, GetMouseFocusArea(), 0, 0);

      SetMouseFocusArea (0);
      SetMouseOverArea (0);
      SetCurrentEvent (0);
    }

    if (GetMouseFocusArea() && (ievent.e_event != NUX_MOUSE_PRESSED) && (!InExclusiveInputMode ()))
    {
      SetCurrentEvent (&ievent);
      SetCurrentWindow (GetFocusAreaWindow());
      ProcessEventOnObject (ievent, GetMouseFocusArea(), 0, 0);

      if (ievent.e_event == NUX_MOUSE_RELEASED)
      {
        SetMouseFocusArea (0);
        // No need to set SetMouseOverArea to NULL.
        //SetMouseOverArea(0);
      }

      if ((ievent.e_event == NUX_MOUSE_RELEASED))
      {
        SetWidgetDrawingOverlay (NULL, NULL);
      }


      // Let all the menu area check the event first. Beside, they are on top of everything else.
      for (menu_it = m_MenuList->begin(); menu_it != m_MenuList->end(); menu_it++)
      {
        // The deepest menu in the menu cascade is in the front of the list.
        ret = (*menu_it)->ProcessEvent (ievent, ret, 0);
      }

      // We cannot get in here.
//         if((ievent.e_event == NUX_MOUSE_PRESSED) && m_MenuList->size())
//         {
//             bool inside = false;
//             for(menu_it = m_MenuList->begin(); menu_it != m_MenuList->end(); menu_it++)
//             {
//                 Geometry geo = (*menu_it)->GetGeometry();
//                 if(PT_IN_BOX( ievent.e_x-ievent.e_x_root, ievent.e_y-ievent.e_y_root, geo.x, geo.x + geo.width, geo.y, geo.y + geo.height ))
//                 {
//                     inside = true;
//                     break;
//                 }
//             }
//
//             if(inside == false)
//             {
//                 (*m_MenuList->begin())->NotifyMouseDownOutsideMenuCascade(ievent.e_x-ievent.e_x_root, ievent.e_y-ievent.e_y_root);
//             }
//         }
      SetCurrentEvent (NULL);
      SetCurrentWindow (NULL);
    }
    else
    {
      SetCurrentEvent (&ievent);

      if (!InExclusiveInputMode ()) // Menus are not enabled in exclusive mode
      {
        if (m_MenuWindow.IsValid())
        {
          ievent.e_x_root = m_MenuWindow->GetBaseX();
          ievent.e_y_root = m_MenuWindow->GetBaseY();
        }

        // Let all the menu area check the event first. Beside, they are on top of everything else.
        for (menu_it = m_MenuList->begin(); menu_it != m_MenuList->end(); menu_it++)
        {
          // The deepest menu in the menu cascade is in the front of the list.
          ret = (*menu_it)->ProcessEvent (ievent, ret, 0);
        }

        if ( (ievent.e_event == NUX_MOUSE_PRESSED) && m_MenuList->size() )
        {
          bool inside = false;

          for (menu_it = m_MenuList->begin(); menu_it != m_MenuList->end(); menu_it++)
          {
            Geometry geo = (*menu_it)->GetGeometry();

            if (PT_IN_BOX ( ievent.e_x - ievent.e_x_root, ievent.e_y - ievent.e_y_root,
                            geo.x, geo.x + geo.width, geo.y, geo.y + geo.height ) )
            {
              inside = true;
              break;
            }
          }

          if (inside == false)
          {
            (*m_MenuList->begin() )->NotifyMouseDownOutsideMenuCascade (ievent.e_x - ievent.e_x_root, ievent.e_y - ievent.e_y_root);
          }
        }

        if (m_MenuWindow.IsValid())
        {
          ievent.e_x_root = 0;
          ievent.e_y_root = 0;
        }

        if ( (ievent.e_event == NUX_MOUSE_RELEASED) )
        {
          SetWidgetDrawingOverlay (NULL, NULL);
        }

        if ( (ievent.e_event == NUX_SIZE_CONFIGURATION) && m_MenuList->size() )
        {
          (*m_MenuList->begin() )->NotifyTerminateMenuCascade();
        }
      }

      long ProcessEventInfo = 0;
      bool MouseIsOverMenu = 0;

      if (ret & eMouseEventSolved)
      {
        // If one menu processed the event, then stop all other element from processing it.
        ProcessEventInfo = eDoNotProcess;
        MouseIsOverMenu = TRUE;
      }

      std::list< IntrusiveWeakSP<BaseWindow> >::iterator it;
      for (it = m_WindowList.begin (); it != m_WindowList.end (); it++)
      {
        // Reset the preemptive hidden/visible status of all base windows.
        IntrusiveWeakSP<BaseWindow> window = (*it); //NUX_STATIC_CAST (BaseWindow *, (*it));
        window->_entering_visible_status = false;
        window->_entering_hidden_status = false;
      }

      if (m_ModalWindowList.size () > 0)
      {
        SetCurrentWindow ((*m_ModalWindowList.begin ()).GetPointer ());
        ret = (*m_ModalWindowList.begin ())->ProcessEvent (ievent, ret, ProcessEventInfo);
        SetCurrentWindow (NULL);
      }
      else
      {
        if ((ievent.e_event == NUX_MOUSE_PRESSED) && (!InExclusiveInputMode ()))
        {
          // There is a possibility we might have to reorder the stack of windows.
          // Cancel the currently selected window.
          m_SelectedWindow = NULL;
          base_window_reshuffling = true;
        }

        for (it = m_WindowList.begin (); it != m_WindowList.end (); it++)
        {
          if ((*it)->IsVisible () && ((*it)->_entering_visible_status == false))
          {
            // Traverse the window from the top of the visibility stack to the bottom.
            if ((*it).GetPointer ())
            {
              SetCurrentWindow ((*it).GetPointer ());
              ret = (*it)->ProcessEvent (ievent, ret, ProcessEventInfo);
              SetCurrentWindow (NULL);

              if ((ret & eMouseEventSolved) && (m_SelectedWindow == 0) && (!InExclusiveInputMode ()))
              {
                // The mouse event was solved in the window pointed by the iterator.
                // There isn't a currently selected window. Make the window pointed by the iterator the selected window.
                base_window_reshuffling = true;
                m_SelectedWindow = (*it);
              }
            }
          }
        }
        
        // Check if the mouse is over a menu. If yes, do not let the main window analyze the event.
        // This avoid mouse in/out messages from widgets below the menu chain.
        if (!MouseIsOverMenu)
        {
          // Let the main window analyze the event.
          ret = GetGraphicsThread ()->ProcessEvent (ievent, ret, ProcessEventInfo) ;
        }
      }

      SetCurrentEvent (0);
    }

    // Event processing cycle has ended.
    _inside_event_processing = false;

    if (!InExclusiveInputMode ())
    {
      if ((base_window_reshuffling == true) && (m_SelectedWindow != 0))
      {
        // Move the newly selected window at the top of the visibility stack.
        PushToFront (m_SelectedWindow.GetPointer ());
      }

      // Make the designated BaseWindow always on top of the stack.
      EnsureAlwaysOnFrontWindow ();
    }

    ExecPendingExclusiveInputAreaAction ();

    CleanMenu ();
  }

  void WindowCompositor::StartModalWindow (IntrusiveWeakSP<BaseWindow> window)
  {
    if (window == 0)
      return;

    std::list< IntrusiveWeakSP<BaseWindow> >::iterator it = find (m_ModalWindowList.begin(), m_ModalWindowList.end(), window);

    if (it == m_ModalWindowList.end() )
    {
      m_ModalWindowList.push_front (window);
    }
  }

  void WindowCompositor::StopModalWindow (IntrusiveWeakSP<BaseWindow> window)
  {
    if (m_ModalWindowList.size () > 0)
    {
      if (*m_ModalWindowList.begin () == window)
        m_ModalWindowList.pop_front ();
    }
  }

  //! Push a floating view at the top of the stack.
  void WindowCompositor::PushToFront (BaseWindow *window)
  {
    if (window == 0)
      return;

    std::list< IntrusiveWeakSP<BaseWindow> >::iterator it = find (m_WindowList.begin(), m_WindowList.end (), window);

    if (it != m_WindowList.end () )
    {
      m_WindowList.erase (it);
      m_WindowList.push_front (IntrusiveWeakSP<BaseWindow> (window));
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

    std::list< IntrusiveWeakSP<BaseWindow> >::iterator it = find (m_WindowList.begin (), m_WindowList.end (), window);

    if (it != m_WindowList.end() )
    {
      m_WindowList.erase (it);
      m_WindowList.push_back (IntrusiveWeakSP<BaseWindow> (window));
    }

    EnsureAlwaysOnFrontWindow ();
  }

  //! Push a floating view just above another floating view.
  void WindowCompositor::PushHigher (BaseWindow *top_floating_view, BaseWindow *bottom_floating_view, bool strict)
  {
    NUX_RETURN_IF_NULL (bottom_floating_view);
    NUX_RETURN_IF_NULL (top_floating_view);
    NUX_RETURN_IF_FALSE (bottom_floating_view != top_floating_view)
    
    std::list< IntrusiveWeakSP<BaseWindow> >::iterator it;
    std::list< IntrusiveWeakSP<BaseWindow> >::iterator it_top;
    std::list< IntrusiveWeakSP<BaseWindow> >::iterator it_bot;
    
    int i = 0;
    int top_pos = -1;
    int bot_pos = -1;

    for (it_top = m_WindowList.begin (), i = 0; it_top != m_WindowList.end (); it_top++, i++)
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

    if ((it_top == m_WindowList.end ()) || (it_bot == m_WindowList.end ()))
    {
      return;
    }

    if ((top_pos < bot_pos) && (strict == false))
    {
      m_WindowList.erase (it_top);
      m_WindowList.insert (it_bot, IntrusiveWeakSP<BaseWindow> (top_floating_view));
    }

    EnsureAlwaysOnFrontWindow ();
  }

  void WindowCompositor::SetAlwaysOnFrontWindow (BaseWindow *window)
  {
    _always_on_front_window = IntrusiveWeakSP<BaseWindow> (window);

    EnsureAlwaysOnFrontWindow ();
  }

  void WindowCompositor::EnsureAlwaysOnFrontWindow ()
  {
    // Do not re-order while we are traversing the list of BaseWindow.
    if (_inside_event_processing)
      return;
    
    if (_always_on_front_window == NULL)
      return;

    std::list< IntrusiveWeakSP<BaseWindow> >::iterator always_top_it = find (m_WindowList.begin(), m_WindowList.end(), _always_on_front_window);
    if ((always_top_it != m_WindowList.end ()) && (always_top_it != m_WindowList.begin ()) && (_always_on_front_window != NULL))
    {
      m_WindowList.erase (always_top_it);
      m_WindowList.push_back (_always_on_front_window);
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

    if (_inside_event_processing)
    {
      _pending_exclusive_input_mode_action = true;
      _exclusive_input_area = input_area;
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
    if (!GetGraphicsThread()->GetWindow().isWindowMinimized())
    {
      //int w, h;
      GetGraphicsThread()->GetGraphicsEngine().SetContext (0, 0, GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth(), GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight() );
      GetGraphicsThread()->GetGraphicsEngine().GetContextSize (m_Width, m_Height);
      GetGraphicsThread()->GetGraphicsEngine().SetViewport (0, 0, m_Width, m_Height);
      GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (m_Width, m_Height);


      if (GetGraphicsThread()->GetWindow().HasFrameBufferSupport() )
      {
        if (force_draw || SizeConfigurationEvent)
        {
          // We fall here after something dramatic has happen to the window such as a resizing. In this case
          // everything must be rendered. This is very costly and should happen rarely.
          if (!GetGraphicsThread()->IsEmbeddedWindow() )
            RenderMainWindowComposition (true, true);

          if (1 /*GetThreadGLDeviceFactory()->GetGraphicsBoardVendor() != BOARD_INTEL*/)
          {
            DrawFloatingWindows (true, m_WindowList, false, true);
            DrawFloatingWindows (true, m_ModalWindowList, true, true);

            DrawMenu (true);
            DrawTooltip (true);
            DrawOverlay (true);
          }
        }
        else if (m_PopupRemoved || m_MenuRemoved)
        {
          // A popup removed cause the whole window to be dirty (at least some part of it).
          // So exchange DrawList with a real Draw.
          if (!GetGraphicsThread()->IsEmbeddedWindow() )
            RenderMainWindowComposition (false, true);

          if (1 /*GetThreadGLDeviceFactory()->GetGraphicsBoardVendor() != BOARD_INTEL*/)
          {
            DrawFloatingWindows (false, m_WindowList, false, true);
            DrawFloatingWindows (false, m_ModalWindowList, true, true);

            DrawMenu (true);
            DrawTooltip (true);
            DrawOverlay (true);
          }
        }
        else
        {
          if (!GetGraphicsThread()->IsEmbeddedWindow() )
            RenderMainWindowComposition (false, true);

          if (1 /*GetThreadGLDeviceFactory()->GetGraphicsBoardVendor() != BOARD_INTEL*/)
          {
            DrawFloatingWindows (false, m_WindowList, false, true);
            DrawFloatingWindows (false, m_ModalWindowList, true, true);

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
          GetPainter().PushDrawColorLayer (GetGraphicsThread()->GetGraphicsEngine(), Geometry (0, 0, m_Width, m_Height), Color (0xFF4D4D4D), true);
          RenderMainWindowComposition (true, false);

          DrawFloatingWindows (true, m_WindowList, false, false);
          DrawFloatingWindows (true, m_ModalWindowList, true, false);
          DrawMenu (true);
          DrawOverlay (true);
          DrawTooltip (true);
          GetPainter().PopBackground();
        }
        else if (m_PopupRemoved || m_MenuRemoved)
        {
          GetPainter().PushDrawColorLayer (GetGraphicsThread()->GetGraphicsEngine(), Geometry (0, 0, m_Width, m_Height), Color (0xFF4D4D4D), true);
          RenderMainWindowComposition (true, false);

          DrawFloatingWindows (false, m_WindowList, false, false);
          DrawFloatingWindows (false, m_ModalWindowList, true, false);
          DrawMenu (true);
          DrawOverlay (true);
          DrawTooltip (true);
          GetPainter().PopBackground();
        }
        else
        {
          RenderMainWindowComposition (false, false);
          DrawFloatingWindows (true, m_WindowList, false, false);
          DrawFloatingWindows (true, m_ModalWindowList, true, false);
          DrawMenu (true);
          DrawOverlay (true);
          DrawTooltip (true);
        }

        GetPainter().EmptyBackgroundStack();
      }

      m_PopupRemoved = false;
      m_MenuRemoved = false;

      GetGraphicsThread()->GetGraphicsEngine().Pop2DWindow();
    }
  }

  void WindowCompositor::DrawMenu (bool force_draw)
  {
    IntrusiveWeakSP<BaseWindow> window = m_MenuWindow;
    int buffer_width = GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth();
    int buffer_height = GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight();

    if (window.IsValid ())
    {
      int x = window->GetBaseX();
      int y = window->GetBaseY();
      GetGraphicsThread()->GetGraphicsEngine().SetContext (x, y, buffer_width, buffer_height);
      GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth(),
          GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight() );
      GetGraphicsThread()->GetGraphicsEngine().EmptyClippingRegion();
    }
    else
    {
      GetGraphicsThread()->GetGraphicsEngine().SetContext (0, 0, buffer_width, buffer_height);
      GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth(),
          GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight() );
      GetGraphicsThread()->GetGraphicsEngine().EmptyClippingRegion();
    }

    std::list<MenuPage *>::reverse_iterator rev_it_menu;

    for (rev_it_menu = m_MenuList->rbegin(); rev_it_menu != m_MenuList->rend( ); rev_it_menu++)
    {
      SetCurrentWindow (m_MenuWindow.GetPointer ());
      (*rev_it_menu)->ProcessDraw (GetGraphicsThread()->GetGraphicsEngine(), force_draw);
      SetCurrentWindow (NULL);
    }

    GetThreadGraphicsContext()->SetContext (0, 0,
                                            GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth(),
                                            GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight() );
  }

  void WindowCompositor::DrawOverlay (bool force_draw)
  {
    IntrusiveWeakSP<BaseWindow> window = m_OverlayWindow;
    int buffer_width = GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth();
    int buffer_height = GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight();

    if (window.IsValid ())
    {
      int x = window->GetBaseX();
      int y = window->GetBaseY();
      GetGraphicsThread()->GetGraphicsEngine().SetContext (x, y, buffer_width, buffer_height);
      GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (buffer_width, buffer_height);
      GetGraphicsThread()->GetGraphicsEngine().EmptyClippingRegion();
    }
    else
      GetGraphicsThread()->GetGraphicsEngine().SetDrawClippingRegion (0, 0, buffer_width, buffer_height);

    if (OverlayDrawingCommand)
    {
      SetCurrentWindow (m_OverlayWindow.GetPointer ());
      OverlayDrawingCommand->OverlayDrawing (GetGraphicsThread()->GetGraphicsEngine() );
      SetCurrentWindow (NULL);
    }

    GetThreadGraphicsContext()->SetContext (0, 0, buffer_width, buffer_height);
  }

  void WindowCompositor::DrawTooltip (bool force_draw)
  {
    IntrusiveWeakSP<BaseWindow> window = _tooltip_window;
    int buffer_width = GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth();
    int buffer_height = GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight();

    if (window.IsValid ())
    {
      int x = window->GetBaseX();
      int y = window->GetBaseY();
      GetGraphicsThread()->GetGraphicsEngine().SetContext (x, y, buffer_width, buffer_height);
      GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (buffer_width, buffer_height);
      GetGraphicsThread()->GetGraphicsEngine().EmptyClippingRegion();
    }
    else
      GetGraphicsThread()->GetGraphicsEngine().SetDrawClippingRegion (0, 0, buffer_width, buffer_height);

    if(m_TooltipText.Size())
    {
        //SetCurrentWindow(_tooltip_window);
        GetPainter().PaintShape(GetGraphicsThread()->GetGraphicsEngine(), _tooltip_geometry, Color(0xA0000000), eSHAPE_CORNER_ROUND10, true);
        GetPainter().PaintTextLineStatic(GetGraphicsThread()->GetGraphicsEngine(), GetSysBoldFont(), _tooltip_text_geometry, m_TooltipText, Color(0xFFFFFFFF));
        //SetCurrentWindow(NULL);
    }

    GetThreadGraphicsContext()->SetContext (0, 0, buffer_width, buffer_height);
  }

  void WindowCompositor::RenderWindowComposition (BaseWindow *window, bool force_draw)
  {
    unsigned int window_width, window_height;
    window_width = GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth();
    window_height = GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight();

    GetPainter().EmptyBackgroundStack();
    SetCurrentWindow (window);
    window->ProcessDraw (GetGraphicsThread()->GetGraphicsEngine(), force_draw || window->IsRedrawNeeded() );
    SetCurrentWindow (NULL);
    GetPainter().EmptyBackgroundStack();
  }

  void WindowCompositor::DrawFloatingWindows (bool force_draw, std::list< IntrusiveWeakSP<BaseWindow> >& WindowList, bool drawModal, bool UseFBO)
  {
    GetGraphicsThread()->GetGraphicsEngine().EmptyClippingRegion();
    // Raw the windows from back to front;
    std::list< IntrusiveWeakSP<BaseWindow> >::reverse_iterator rev_it;

    for (rev_it = WindowList.rbegin(); rev_it != WindowList.rend(); rev_it++)
    {
      if ( (drawModal == false) && (*rev_it)->IsModal() )
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
          if (rt.color_rt.IsValid() /*&& rt.depth_rt.IsValid()*/ && UseFBO)
          {
            t_s32 buffer_width = window->GetBaseWidth();
            t_s32 buffer_height = window->GetBaseHeight();

            if ( (rt.color_rt->GetWidth() != buffer_width) || (rt.color_rt->GetHeight() != buffer_height) )
            {
              rt.color_rt = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_R8G8B8A8);
              rt.depth_rt = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_D24S8);
            }

            m_FrameBufferObject->FormatFrameBufferObject (buffer_width, buffer_height, BITFMT_R8G8B8A8);
            m_FrameBufferObject->SetRenderTarget ( 0, rt.color_rt->GetSurfaceLevel (0) );
            m_FrameBufferObject->SetDepthSurface ( rt.depth_rt->GetSurfaceLevel (0) );
            m_FrameBufferObject->Activate();
            GetGraphicsThread()->GetGraphicsEngine().SetContext (0, 0, buffer_width, buffer_height);
            GetGraphicsThread()->GetGraphicsEngine().SetViewport (0, 0, buffer_width, buffer_height);
            GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (buffer_width, buffer_height);
            GetGraphicsThread()->GetGraphicsEngine().EmptyClippingRegion();
            GetGraphicsThread()->GetGraphicsEngine().SetDrawClippingRegion (0, 0, buffer_width, buffer_height);

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
            //GetGraphicsThread()->GetGraphicsEngine().SetContext(x, y, 0, 0);
            GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth(),
                GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight() );

            //GetGraphicsThread()->GetGraphicsEngine().Push2DModelViewMatrix(mat);
          }

          RenderWindowComposition (/*fbo,*/ window, force_draw);
        }
        
        if (rt.color_rt.IsValid() /*&& rt.depth_rt.IsValid()*/ && UseFBO)
        {
          // GetGraphicsThread()->GetGraphicsEngine().EmptyClippingRegion();
          m_FrameBufferObject->Deactivate();

          // Enable this to render the drop shadow under windows: not perfect yet...
          if (0/*force_draw || WindowNeedRedraw*/)
          {
            unsigned int window_width, window_height;
            window_width = GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth();
            window_height = GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight();
            GetGraphicsThread()->GetGraphicsEngine().SetContext (0, 0, window_width, window_height);
            GetGraphicsThread()->GetGraphicsEngine().EmptyClippingRegion();
            GetGraphicsThread()->GetGraphicsEngine().SetDrawClippingRegion (0, 0, window_width, window_height);
            GetGraphicsThread()->GetGraphicsEngine().SetViewport (0, 0, window_width, window_height);
            GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (window_width, window_height);

            Geometry shadow (window->GetBaseX(), window->GetBaseY(), window->GetBaseWidth(), window->GetBaseHeight() );
            //if(window->IsVisibleSizeGrip())
            {
              shadow.OffsetPosition (4, 4);
              GetPainter().PaintShape (GetGraphicsThread()->GetGraphicsEngine(), shadow, Color (0xFF000000), eSHAPE_CORNER_SHADOW);
            }
//                    else
//                    {
//                        shadow.OffsetPosition(4, 4);
//                        GetPainter().PaintShape(GetGraphicsThread()->GetGraphicsEngine(), shadow, Color(0xFF000000), eSHAPE_CORNER_ROUND10_SHADOW);
//                    }
          }

          if (GetGraphicsThread()->IsEmbeddedWindow() )
          {
            // In embedded mode, floating windows are composited over Nux main window which is probably empty. At least that
            // is how things are at the moment. Compiste the floating windows onto the main texture without blending.
            GetGraphicsThread()->GetGraphicsEngine().GetRenderStates().SetBlend (false, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          }
          else
          {
            GetGraphicsThread()->GetGraphicsEngine().GetRenderStates().SetBlend (true, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          }

          CHECKGL ( glDepthMask (GL_FALSE) );
          {
            //CopyTextureToCompositionRT(rt.color_rt, window->GetBaseX(), window->GetBaseY());
            PresentBufferToScreen (rt.color_rt, window->GetBaseX(), window->GetBaseY(), false, false);
          }
          CHECKGL ( glDepthMask (GL_TRUE) );
          GetGraphicsThread()->GetGraphicsEngine().GetRenderStates().SetBlend (false);
        }
        else
        {
//                int x = window->GetX();
//                int y = window->GetY();
//                Matrix4 mat;
//                mat.Translate(x, y, 0);
          GetGraphicsThread()->GetGraphicsEngine().SetContext (0, 0, 0, 0);
          //GetGraphicsThread()->GetGraphicsEngine().Pop2DModelViewMatrix();
        }
        
        window->_child_need_redraw = false;
      }
      else
      {
        IntrusiveWeakSP<BaseWindow> window = *rev_it;
        window->_child_need_redraw = false;
        window->DoneRedraw ();
      }
    }

    if (UseFBO)
      m_FrameBufferObject->Deactivate();
  }

// This function is meant to draw the main window and the small windows but it will not traverse the element.
// It just copies to the backbuffer what it already has in  the framebuffer objects.
  void WindowCompositor::PresentRendering()
  {
    m_FrameBufferObject->Deactivate();
    unsigned int window_width, window_height;
    window_width = GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth();
    window_height = GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight();
    GetGraphicsThread()->GetGraphicsEngine().SetContext (0, 0, window_width, window_height);
    GetGraphicsThread()->GetGraphicsEngine().EmptyClippingRegion();
    //GetGraphicsThread()->GetGraphicsEngine().SetDrawClippingRegion(0, 0, window_width, window_height);
    GetGraphicsThread()->GetGraphicsEngine().SetViewport (0, 0, window_width, window_height);
    GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (window_width, window_height);

    PresentBufferToScreen (m_MainColorRT, 0, 0, false);

    PageBBox page;
    page.xmin = 0;
    page.xmax = 100;
    page.ymin = 700;
    page.ymax = 730;
    page.x_margin = 0;
    page.y_margin = 0;

    NString FPS = NString::Printf (TEXT ("FPS: %3.2f"), GetGraphicsThread()->GetFrameRate() );

    GetGraphicsThread()->GetGraphicsEngine().RenderColorTextLineStatic (GetSysBoldFont(), page, FPS, Color (0xffff0000), true, eAlignTextLeft);

    GetGraphicsThread()->GetGraphicsEngine().Pop2DWindow();
//     GetGraphicsThread()->GetGraphicsEngine().ResetStats();
//     GetGraphicsThread()->GetWindow().SwapBuffer();

//    const std::list<BaseWindow*>& W = m_WindowList;
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
      buffer_width = GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth();
      buffer_height = GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight();

      if ( (!m_MainColorRT.IsValid() ) || (!m_MainDepthRT.IsValid() ) || (m_MainColorRT->GetWidth() != buffer_width) || (m_MainColorRT->GetHeight() != buffer_height) )
      {
        m_MainColorRT = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_R8G8B8A8);
        m_MainDepthRT = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_D24S8);
      }

      m_FrameBufferObject->FormatFrameBufferObject (buffer_width, buffer_height, BITFMT_R8G8B8A8);
      m_FrameBufferObject->SetRenderTarget ( 0, m_MainColorRT->GetSurfaceLevel (0) );
      m_FrameBufferObject->SetDepthSurface ( m_MainDepthRT->GetSurfaceLevel (0) );
      m_FrameBufferObject->Activate();
    }
    else
    {
      buffer_width = GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth();
      buffer_height = GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight();
    }

    GetGraphicsThread()->GetGraphicsEngine().SetContext (0, 0, buffer_width, buffer_height);
    GetGraphicsThread()->GetGraphicsEngine().EmptyClippingRegion();
    GetGraphicsThread()->GetGraphicsEngine().SetDrawClippingRegion (0, 0, buffer_width, buffer_height);
    GetGraphicsThread()->GetGraphicsEngine().SetViewport (0, 0, buffer_width, buffer_height);
    GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (buffer_width, buffer_height);
    {
      CHECKGL ( glClear (/*GL_COLOR_BUFFER_BIT |*/ GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT) );
      //Begin 2D Drawing
      {
        if (force_draw)
        {
          GetPainter().PushDrawLayer (GetGraphicsThread()->GetGraphicsEngine(), Geometry (0, 0, buffer_width, buffer_height), m_Background);
          //GetPainter().PushBackground(GetGraphicsThread()->GetGraphicsEngine(), Geometry(0, 0, buffer_width, buffer_height), m_Background, true);

          GetGraphicsThread()->ProcessDraw (GetGraphicsThread()->GetGraphicsEngine(), true);

          nuxAssert (GetGraphicsThread()->GetGraphicsEngine().GetNumberOfClippingRegions() == 0);
          GetPainter().PopBackground();
          GetPainter().EmptyBackgroundStack();
        }
        else
        {
          GetPainter().PushLayer (GetGraphicsThread()->GetGraphicsEngine(), Geometry (0, 0, buffer_width, buffer_height), m_Background);
          //GetPainter().PushBackground(GetGraphicsThread()->GetGraphicsEngine(), Geometry(0, 0, buffer_width, buffer_height), m_Background, false);

          GetGraphicsThread()->ProcessDraw (GetGraphicsThread()->GetGraphicsEngine(), false);

          nuxAssert (GetGraphicsThread()->GetGraphicsEngine().GetNumberOfClippingRegions() == 0);
          GetPainter().PopBackground();
          GetPainter().EmptyBackgroundStack();
        }
      }
      // End 2D Drawing
    }

    if (UseFBO)
    {
      m_FrameBufferObject->Deactivate();
    }

    unsigned int window_width, window_height;
    window_width = GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth();
    window_height = GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight();
    GetGraphicsThread()->GetGraphicsEngine().SetContext (0, 0, window_width, window_height);
    GetGraphicsThread()->GetGraphicsEngine().EmptyClippingRegion();
    GetGraphicsThread()->GetGraphicsEngine().SetDrawClippingRegion (0, 0, window_width, window_height);
    GetGraphicsThread()->GetGraphicsEngine().SetViewport (0, 0, window_width, window_height);
    GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (window_width, window_height);

    // Render the Buffer on the screen
    if (UseFBO)
    {
      PresentBufferToScreen (m_MainColorRT, 0, 0, false);
    }
  }

  void WindowCompositor::SetMainColorRT()
  {
    t_s32 buffer_width, buffer_height;
    buffer_width = GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth();
    buffer_height = GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight();

    if ( (!m_MainColorRT.IsValid() ) || (m_MainColorRT->GetWidth() != buffer_width) || (m_MainColorRT->GetHeight() != buffer_height) )
    {
      m_MainColorRT = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_R8G8B8A8);
    }

    // Setup the Composition Render Target
    m_FrameBufferObject->FormatFrameBufferObject (buffer_width, buffer_height, BITFMT_R8G8B8A8);
    m_FrameBufferObject->SetRenderTarget (0, m_MainColorRT->GetSurfaceLevel (0));
    m_FrameBufferObject->SetDepthSurface (IntrusiveSP<IOpenGLSurface> (0));
    m_FrameBufferObject->Activate();
    GetGraphicsThread()->GetGraphicsEngine().SetContext (0, 0, buffer_width, buffer_height);
    GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (buffer_width, buffer_height);
    GetGraphicsThread()->GetGraphicsEngine().EmptyClippingRegion();
    GetGraphicsThread()->GetGraphicsEngine().SetDrawClippingRegion (0, 0, buffer_width, buffer_height);
  }

  void WindowCompositor::CopyTextureToMainColorRT (IntrusiveSP<IOpenGLBaseTexture> HWTexture, int x, int y)
  {
    SetMainColorRT();
    HWTexture->SetFiltering (GL_NEAREST, GL_NEAREST);
    HWTexture->BindTextureToUnit (GL_TEXTURE0);
    GetGraphicsThread()->GetGraphicsEngine().DisableAllTextureMode (GL_TEXTURE1);

    int TexWidth = HWTexture->GetWidth();
    int TexHeight = HWTexture->GetHeight();

    TexCoordXForm texxform;
    texxform.FlipVCoord (true);
    texxform.uscale = 1.0f;
    texxform.vscale = 1.0f;
    texxform.uwrap = TEXWRAP_REPEAT;
    texxform.vwrap = TEXWRAP_REPEAT;
    GetGraphicsThread()->GetGraphicsEngine().QRP_GLSL_1Tex (x, y, TexWidth, TexHeight, HWTexture, texxform, Color::White);
  }

  void WindowCompositor::SetCompositionRT()
  {
    t_s32 buffer_width, buffer_height;
    buffer_width = GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth();
    buffer_height = GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight();

    if ( (!m_CompositionRT.IsValid() ) || (m_CompositionRT->GetWidth() != buffer_width) || (m_CompositionRT->GetHeight() != buffer_height) )
    {
      m_CompositionRT = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_R8G8B8A8);
    }

    // Setup the Composition Render Target
    m_FrameBufferObject->FormatFrameBufferObject (buffer_width, buffer_height, BITFMT_R8G8B8A8);
    m_FrameBufferObject->SetRenderTarget (0, m_CompositionRT->GetSurfaceLevel (0));
    m_FrameBufferObject->SetDepthSurface (IntrusiveSP<IOpenGLSurface> (0));
    m_FrameBufferObject->Activate();
    GetGraphicsThread()->GetGraphicsEngine().SetContext (0, 0, buffer_width, buffer_height);
    GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (buffer_width, buffer_height);
    GetGraphicsThread()->GetGraphicsEngine().EmptyClippingRegion ();
    GetGraphicsThread()->GetGraphicsEngine().SetDrawClippingRegion (0, 0, buffer_width, buffer_height);
  }

  void WindowCompositor::CopyTextureToCompositionRT (IntrusiveSP<IOpenGLBaseTexture> HWTexture, int x, int y)
  {
    SetCompositionRT();
    HWTexture->SetFiltering (GL_NEAREST, GL_NEAREST);
    HWTexture->BindTextureToUnit (GL_TEXTURE0);
    GetGraphicsThread()->GetGraphicsEngine().DisableAllTextureMode (GL_TEXTURE1);

    int TexWidth = HWTexture->GetWidth();
    int TexHeight = HWTexture->GetHeight();

    TexCoordXForm texxform;
    texxform.FlipVCoord (true);
    texxform.uscale = 1.0f;
    texxform.vscale = 1.0f;
    texxform.uwrap = TEXWRAP_REPEAT;
    texxform.vwrap = TEXWRAP_REPEAT;
    GetGraphicsThread()->GetGraphicsEngine().QRP_GLSL_1Tex (x, y, TexWidth, TexHeight, HWTexture, texxform, Color::White);
  }

  void WindowCompositor::PresentBufferToScreen (IntrusiveSP<IOpenGLBaseTexture> HWTexture, int x, int y, bool RenderToMainTexture, bool BluredBackground)
  {
    nuxAssert (HWTexture.IsValid() );

    if (HWTexture.IsNull() )
      return;

    t_s32 window_width, window_height;
    window_width = GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth();
    window_height = GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight();


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
      GetThreadGLDeviceFactory()->DeactivateFrameBuffer();
    }

    GetGraphicsThread()->GetGraphicsEngine().SetContext (0, 0, window_width, window_height);
    GetGraphicsThread()->GetGraphicsEngine().EmptyClippingRegion();
    GetGraphicsThread()->GetGraphicsEngine().SetDrawClippingRegion (0, 0, window_width, window_height);
    GetGraphicsThread()->GetGraphicsEngine().SetViewport (0, 0, window_width, window_height);
    GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (window_width, window_height);


    // Render the MAINFBO
    {
      //-->GetGraphicsThread()->GetGraphicsEngine().DrawQuadTexture

      GetGraphicsThread()->GetGraphicsEngine().EnableTextureMode (GL_TEXTURE0, GL_TEXTURE_RECTANGLE_ARB);
      HWTexture->BindTextureToUnit (GL_TEXTURE0);

      int src_width, src_height;
      src_width = HWTexture->GetWidth();
      src_height = HWTexture->GetHeight();

      if (1 /*!BluredBackground*/)
      {
        TexCoordXForm texxform0;
        texxform0.FlipVCoord (true);

        if (GetGraphicsThread()->IsEmbeddedWindow() )
        {
          // Compose Nux's main texture onto another surface (a texture or the back buffer) with blending.
          GetGraphicsThread()->GetGraphicsEngine().GetRenderStates().SetBlend (true, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        GetThreadGraphicsContext()->QRP_GLSL_1Tex (x, y, src_width, src_height, HWTexture, texxform0, Color::White);
        GetGraphicsThread()->GetGraphicsEngine().GetRenderStates().SetBlend (false);
      }
      else
      {
//         HWTexture->SetFiltering (GL_NEAREST, GL_NEAREST);
//         HWTexture->BindTextureToUnit (GL_TEXTURE0);
// 
//         m_BlurTexture->SetFiltering (GL_NEAREST, GL_NEAREST);
//         m_BlurTexture->BindTextureToUnit (GL_TEXTURE1);
// 
//         TexCoordXForm texxform0;
//         texxform0.SetFilter (TEXFILTER_NEAREST, TEXFILTER_NEAREST);
//         texxform0.FlipVCoord (true);
//         TexCoordXForm texxform1;
//         texxform1.SetFilter (TEXFILTER_NEAREST, TEXFILTER_NEAREST);
//         texxform1.FlipVCoord (true);
//         texxform1.uoffset = (float) x / (float) src_width;
//         texxform1.voffset = (float) y / (float) src_height;
//         texxform1.SetTexCoordType (TexCoordXForm::OFFSET_COORD);
// 
//         GetGraphicsThread()->GetGraphicsEngine().QRP_GLSL_2Tex (x, y, src_width, src_height,
//             HWTexture, texxform0, Color::White,
//             m_BlurTexture, texxform1, Color::White);
      }

      //GetGraphicsThread()->GetGraphicsEngine().DisableAllTextureMode(GL_TEXTURE1);
      //GetGraphicsThread()->GetGraphicsEngine().SetEnvModeSelectColor(GL_TEXTURE0);
    }
  }

  void WindowCompositor::AddMenu(MenuPage *menu, BaseWindow *window, bool OverrideCurrentMenuChain)
  {
    std::list<MenuPage*>::iterator it = find(m_MenuList->begin(), m_MenuList->end(), menu);
    if(it == m_MenuList->end())
    {
      // When adding a MenuPage, make sure that it is a child of the MenuPage in m_MenuList->begin().
      if (m_MenuList->size() )
      {
        if (menu->GetParentMenu() != (*m_MenuList->begin() ) )
        {
          if (OverrideCurrentMenuChain)
          {
            // Remove the current menu chain
            for (it = m_MenuList->begin(); it != m_MenuList->end(); it++)
            {
              // Stop all pages
              (*it)->StopMenu();
            }

            m_MenuList->clear();
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
      m_MenuList->push_front (menu);
    }
  }

  // Be carefull never call this function while you are iterating through the elements of m_MenuList.
  void WindowCompositor::RemoveMenu (MenuPage *menu)
  {
    std::list<MenuPage *>::iterator it = find (m_MenuList->begin(), m_MenuList->end(), menu);

    if (it == m_MenuList->end() )
    {
      return;
    }

    m_MenuList->erase (it);
    m_MenuRemoved = true;

    if (m_MenuList->size() == 0)
      m_MenuWindow = NULL;
  }

  void WindowCompositor::CleanMenu()
  {
    if (m_MenuList->size() == 0)
      return;

    std::list<MenuPage *>::iterator menu_it = m_MenuList->begin();

    while (menu_it != m_MenuList->end() )
    {
      if ( (*menu_it)->IsActive() == false)
      {
        menu_it = m_MenuList->erase (menu_it);
        m_MenuRemoved = true;
      }
      else
        menu_it++;
    }

    if (m_MenuList->size() == 0)
      m_MenuWindow = NULL;
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
    _tooltip_window = GetCurrentWindow();
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
    m_MouseFocusArea = area;

    if (area == 0)
    {
      m_EventRoot.Set (0, 0);
    }

    SetFocusAreaWindow (GetCurrentWindow() );
  }

  InputArea *WindowCompositor::GetMouseFocusArea()
  {
    return m_MouseFocusArea;
  }

  void WindowCompositor::SetMouseOverArea (InputArea *area)
  {
    m_MouseOverArea = area;
//     if(m_MouseOverArea)
//         nuxDebugMsg(TEXT("StackManager: Set MouseOver Area"));
//     else
//         nuxDebugMsg(TEXT("StackManager: Set MouseOver Area to 0"));
  }

  InputArea *WindowCompositor::GetMouseOverArea()
  {
    return m_MouseOverArea;
  }

  void WindowCompositor::SetPreviousMouseOverArea (InputArea *area)
  {
    m_PreviousMouseOverArea = area;
//     if(area)
//         nuxDebugMsg(TEXT("StackManager: Set Previous MouseOver Area"));
//     else
//         nuxDebugMsg(TEXT("StackManager: Set Previous MouseOver Area to 0"));
  }

  InputArea *WindowCompositor::GetPreviousMouseOverArea()
  {
    return m_PreviousMouseOverArea;
  }

  void WindowCompositor::SetBackgroundPaintLayer (AbstractPaintLayer *bkg)
  {
    NUX_SAFE_DELETE (m_Background);
    m_Background = bkg->Clone();
  }

  void WindowCompositor::FloatingAreaConfigureNotify (int Width, int Height)
  {
    std::list< IntrusiveWeakSP<BaseWindow> >::iterator it;

    for (it = m_WindowList.begin(); it != m_WindowList.end(); it++)
    {
      if ( (*it)->IsVisible() )
      {
        (*it)->NotifyConfigurationChange (Width, Height);
      }
    }
  }

  void WindowCompositor::FormatRenderTargets (int width, int height)
  {
    int buffer_width = GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth();
    int buffer_height = GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight();

    nuxAssert (buffer_width >= 1);
    nuxAssert (buffer_height >= 1);

    m_MainColorRT = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_R8G8B8A8);
    m_CompositionRT = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_R8G8B8A8);
    m_MainDepthRT = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (buffer_width, buffer_height, 1, BITFMT_D24S8);

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

//     m_BlurTexture   = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (Max (buffer_width, 1), Max (buffer_height, 1), 1, BITFMT_R8G8B8A8);
//     m_FullSceneMip0 = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (Max (buffer_width / 2, 1), Max (buffer_height / 2, 1), 1, BITFMT_R8G8B8A8);
//     m_FullSceneMip1 = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (Max (buffer_width / 4, 1), Max (buffer_height / 4, 1), 1, BITFMT_R8G8B8A8);
//     m_FullSceneMip2 = GetThreadGLDeviceFactory()->CreateSystemCapableDeviceTexture (Max (buffer_width / 8, 1), Max (buffer_height / 8, 1), 1, BITFMT_R8G8B8A8);
  }

//   void WindowCompositor::UpdatePostProcessRT()
//   {
//     int buffer_width = GetGraphicsThread()->GetGraphicsEngine().GetWindowWidth();
//     int buffer_height = GetGraphicsThread()->GetGraphicsEngine().GetWindowHeight();
// 
//     nuxAssert (buffer_width >= 1);
//     nuxAssert (buffer_height >= 1);
// 
//     GetGraphicsThread()->GetGraphicsEngine().EmptyClippingRegion();
// 
//     int src_width, src_height;
//     int dst_width, dst_height;
//     {
//       src_width = buffer_width;
//       src_height = buffer_height;
//       dst_width = Max (buffer_width / 2, 1);
//       dst_height = Max (buffer_height / 2, 1);
// 
//       m_FrameBufferObject->FormatFrameBufferObject (dst_width, dst_height, BITFMT_R8G8B8A8);
//       m_FrameBufferObject->SetRenderTarget ( 0, m_FullSceneMip0->GetSurfaceLevel (0) );
//       m_FrameBufferObject->SetDepthSurface (IntrusiveSP<IOpenGLSurface> (0));
//       m_FrameBufferObject->Activate();
// 
//       GetGraphicsThread()->GetGraphicsEngine().SetContext (0, 0, dst_width, dst_height);
//       GetGraphicsThread()->GetGraphicsEngine().SetViewport (0, 0, dst_width, dst_height);
//       GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (dst_width, dst_height);
// 
//       m_CompositionRT->SetFiltering (GL_LINEAR, GL_LINEAR);
// 
//       TexCoordXForm texxform0;
//       texxform0.uwrap = TEXWRAP_CLAMP;
//       texxform0.vwrap = TEXWRAP_CLAMP;
//       texxform0.FlipVCoord (true);
//       GetThreadGraphicsContext()->QRP_GLSL_1Tex (0, 0, dst_width, dst_height, m_CompositionRT, texxform0, Color::White);
//     }
// 
//     {
//       src_width = Max (buffer_width / 2, 1);
//       src_height = Max (buffer_height / 2, 1);
//       dst_width = Max (buffer_width / 4, 1);
//       dst_height = Max (buffer_height / 4, 1);
// 
//       m_FrameBufferObject->FormatFrameBufferObject (dst_width, dst_height, BITFMT_R8G8B8A8);
//       m_FrameBufferObject->SetRenderTarget ( 0, m_FullSceneMip1->GetSurfaceLevel (0) );
//       m_FrameBufferObject->SetDepthSurface (IntrusiveSP<IOpenGLSurface> (0));
//       m_FrameBufferObject->Activate();
// 
//       GetGraphicsThread()->GetGraphicsEngine().SetContext (0, 0, dst_width, dst_height);
//       GetGraphicsThread()->GetGraphicsEngine().SetViewport (0, 0, dst_width, dst_height);
//       GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (dst_width, dst_height);
// 
// 
//       m_FullSceneMip0->SetFiltering (GL_LINEAR, GL_LINEAR);
//       TexCoordXForm texxform0;
//       texxform0.uwrap = TEXWRAP_CLAMP;
//       texxform0.vwrap = TEXWRAP_CLAMP;
//       texxform0.FlipVCoord (true);
//       GetThreadGraphicsContext()->QRP_GLSL_1Tex (0, 0, dst_width, dst_height, m_FullSceneMip0, texxform0, Color::White);
//     }
// 
//     {
//       src_width = Max (buffer_width / 4, 1);
//       src_height = Max (buffer_height / 4, 1);
//       dst_width = Max (buffer_width / 8, 1);
//       dst_height = Max (buffer_height / 8, 1);
// 
//       m_FrameBufferObject->FormatFrameBufferObject (dst_width, dst_height, BITFMT_R8G8B8A8);
//       m_FrameBufferObject->SetRenderTarget ( 0, m_FullSceneMip2->GetSurfaceLevel (0) );
//       m_FrameBufferObject->SetDepthSurface (IntrusiveSP<IOpenGLSurface> (0));
//       m_FrameBufferObject->Activate();
// 
//       GetGraphicsThread()->GetGraphicsEngine().SetContext (0, 0, dst_width, dst_height);
//       GetGraphicsThread()->GetGraphicsEngine().SetViewport (0, 0, dst_width, dst_height);
//       GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (dst_width, dst_height);
// 
// 
//       m_FullSceneMip1->SetFiltering (GL_LINEAR, GL_LINEAR);
//       TexCoordXForm texxform0;
//       texxform0.uwrap = TEXWRAP_CLAMP;
//       texxform0.vwrap = TEXWRAP_CLAMP;
//       texxform0.FlipVCoord (true);
//       GetThreadGraphicsContext()->QRP_GLSL_1Tex (0, 0, dst_width, dst_height, m_FullSceneMip1, texxform0, Color::White);
//     }
// 
//     {
//       src_width = buffer_width;
//       src_height = buffer_height;
//       dst_width = buffer_width;
//       dst_height = buffer_height;
// 
//       m_FrameBufferObject->FormatFrameBufferObject (buffer_width, buffer_height, BITFMT_R8G8B8A8);
//       m_FrameBufferObject->SetRenderTarget ( 0, m_BlurTexture->GetSurfaceLevel (0) );
//       m_FrameBufferObject->SetDepthSurface (IntrusiveSP<IOpenGLSurface> (0));
//       m_FrameBufferObject->Activate();
// 
//       GetGraphicsThread()->GetGraphicsEngine().SetContext (0, 0, dst_width, dst_height);
//       GetGraphicsThread()->GetGraphicsEngine().SetViewport (0, 0, dst_width, dst_height);
//       GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (dst_width, dst_height);
// 
//       m_CompositionRT->SetFiltering (GL_LINEAR, GL_LINEAR);
//       m_CompositionRT->BindTextureToUnit (GL_TEXTURE0);
// 
//       m_FullSceneMip0->SetFiltering (GL_LINEAR, GL_LINEAR);
//       m_FullSceneMip0->BindTextureToUnit (GL_TEXTURE1);
// 
//       m_FullSceneMip1->SetFiltering (GL_LINEAR, GL_LINEAR);
//       m_FullSceneMip1->BindTextureToUnit (GL_TEXTURE2);
// 
//       m_FullSceneMip2->SetFiltering (GL_LINEAR, GL_LINEAR);
//       m_FullSceneMip2->SetWrap (GL_CLAMP, GL_CLAMP, GL_CLAMP);
//       m_FullSceneMip2->BindTextureToUnit (GL_TEXTURE3);
// 
//       TexCoordXForm texxform0;
//       texxform0.SetFilter (TEXFILTER_LINEAR, TEXFILTER_LINEAR);
//       TexCoordXForm texxform1;
//       texxform1.SetFilter (TEXFILTER_LINEAR, TEXFILTER_LINEAR);
//       TexCoordXForm texxform2;
//       texxform2.SetFilter (TEXFILTER_LINEAR, TEXFILTER_LINEAR);
//       TexCoordXForm texxform3;
//       texxform3.SetFilter (TEXFILTER_LINEAR, TEXFILTER_LINEAR);
// 
//       GetGraphicsThread()->GetGraphicsEngine().QRP_GLSL_4Tex (0, 0, dst_width, dst_height,
//           m_CompositionRT, texxform0, Color::White,
//           m_FullSceneMip0, texxform1, Color::White,
//           m_FullSceneMip1, texxform2, Color::White,
//           m_FullSceneMip2, texxform3, Color::White);
//     }
// 
//     GetGraphicsThread()->GetGraphicsEngine().DisableAllTextureMode (GL_TEXTURE1);
//     GetGraphicsThread()->GetGraphicsEngine().DisableAllTextureMode (GL_TEXTURE2);
//     GetGraphicsThread()->GetGraphicsEngine().DisableAllTextureMode (GL_TEXTURE3);
//     m_FrameBufferObject->Deactivate();
//     m_CompositionRT->SetFiltering (GL_NEAREST, GL_NEAREST);
// 
//     // Restore Main Frame Buffer
//     m_FrameBufferObject->FormatFrameBufferObject (buffer_width, buffer_height, BITFMT_R8G8B8A8);
//     m_FrameBufferObject->SetRenderTarget (0, m_CompositionRT->GetSurfaceLevel (0) );
//     m_FrameBufferObject->SetDepthSurface (IntrusiveSP<IOpenGLSurface> (0));
//     m_FrameBufferObject->Activate();
// 
//     GetGraphicsThread()->GetGraphicsEngine().SetContext (0, 0, buffer_width, buffer_height);
//     GetGraphicsThread()->GetGraphicsEngine().Push2DWindow (buffer_width, buffer_height);
//   }
// 
//   IntrusiveSP< IOpenGLBaseTexture > WindowCompositor::GetScreenBlurTexture()
//   {
//     return m_BlurTexture;
//   }
}

