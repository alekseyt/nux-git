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
#include "Layout.h"
#include "View.h"

namespace nux
{

  NUX_IMPLEMENT_OBJECT_TYPE(View);
  ObjectPtr<IOpenGLFrameBufferObject> View::backup_fbo_;

  View::View(NUX_FILE_LINE_DECL)
  : InputArea(NUX_FILE_LINE_PARAM)
  , redirect_rendering_to_texture_(false)
  , update_backup_texture_(false)
  {
    view_layout_ = NULL;
    draw_cmd_queued_        = false;
    m_TextColor         = Color(1.0f, 1.0f, 1.0f, 1.0f);

    if (backup_fbo_.IsNull())
    {
      backup_fbo_ = GetGraphicsDisplay()->GetGpuDevice()->CreateFrameBufferObject();
    }
  }

  View::~View()
  {
    backup_fbo_.Release();
    backup_texture_.Release();
    backup_depth_texture_.Release();
    
    // It is possible that the window thread has been deleted before the view
    // itself, so check prior to calling.
    WindowThread* wt = GetWindowThread();

    if (wt)
    {
      // It is possible that the object is in the refresh list. Remove it here
      // before it is deleted.
      wt->RemoveObjectFromLayoutQueue(this);
    }

    RemoveLayout();
  }

  long View::ComputeContentSize()
  {
    if (view_layout_)
    {
      PreLayoutManagement();

      int PreWidth = GetBaseWidth();
      int PreHeight = GetBaseHeight();

      long ret = view_layout_->ComputeContentSize();

      PostLayoutManagement(ret);

      int PostWidth = GetBaseWidth();
      int PostHeight = GetBaseHeight();

      long size_compliance = 0;

      // The layout has been resized to tightly pack its content
      if (PostWidth > PreWidth)
      {
        size_compliance |= eLargerWidth; // need scrollbar
      }
      else if (PostWidth < PreWidth)
      {
        size_compliance |= eSmallerWidth;
      }
      else
      {
        size_compliance |= eCompliantWidth;
      }

      // The layout has been resized to tightly pack its content
      if (PostHeight > PreHeight)
      {
        size_compliance |= eLargerHeight; // need scrollbar
      }
      else if (PostHeight < PreHeight)
      {
        size_compliance |= eSmallerHeight;
      }
      else
      {
        size_compliance |= eCompliantHeight;
      }

      return size_compliance;
    }
    else
    {
      PreLayoutManagement();
      int ret = PostLayoutManagement(eCompliantHeight | eCompliantWidth);
      return ret;
    }

    return 0;
  }

  void View::ComputeContentPosition(float offsetX, float offsetY)
  {
    if (view_layout_)
    {
      view_layout_->SetBaseX(GetBaseX());
      view_layout_->SetBaseY(GetBaseY());
      view_layout_->ComputeContentPosition(offsetX, offsetY);
    }
  }

  void View::PreLayoutManagement()
  {
    // Give the managed layout the same size and position as the Control.
    if (view_layout_)
      view_layout_->SetGeometry(GetGeometry());
  }

  long View::PostLayoutManagement(long LayoutResult)
  {
    // Set the geometry of the control to be the same as the managed layout.
    // Only the size is changed. The position of the composition layout hasn't
    // been changed by ComputeContentSize.
    if (view_layout_)
    {
      // If The layout is empty, do not change the size of the parent element.
      if (!view_layout_->IsEmpty())
        Area::SetGeometry(view_layout_->GetGeometry());
    }

    return LayoutResult;
  }

  void View::PreResizeGeometry()
  {

  }

  void View::PostResizeGeometry()
  {
  }

  void View::ProcessDraw(GraphicsEngine& graphics_engine, bool force_draw)
  {
    bool update_rendering = false;
    full_view_draw_cmd_ = false;

    if (RedirectRenderingToTexture() && (update_backup_texture_ || force_draw || draw_cmd_queued_))
    {
      model_view_matrix_ = graphics_engine.GetModelViewMatrix();
      perspective_matrix_ = graphics_engine.GetProjectionMatrix();
      //prev_fbo_ = GetGraphicsDisplay()->GetGpuDevice()->GetCurrentFrameBufferObject();

      update_rendering = true;
      BeginBackupTextureRendering(graphics_engine);
    }

    graphics_engine.PushModelViewMatrix(Get2DMatrix());

    if (force_draw)
    {
      GetPainter().PaintBackground(graphics_engine, GetGeometry());
      GetPainter().PushPaintLayerStack();

      draw_cmd_queued_ = true;
      full_view_draw_cmd_ = true;
      Draw(graphics_engine, force_draw);
      DrawContent(graphics_engine, force_draw);
      PostDraw(graphics_engine, force_draw);

      GetPainter().PopPaintLayerStack();
    }
    else
    {
      if (draw_cmd_queued_ || update_backup_texture_)
      {
        GetPainter().PaintBackground(graphics_engine, GetGeometry());
        GetPainter().PushPaintLayerStack();

        full_view_draw_cmd_ = true;
        Draw(graphics_engine, false);
        DrawContent(graphics_engine, false);
        PostDraw(graphics_engine, false);

        GetPainter().PopPaintLayerStack();
      }
      else
      {
        DrawContent(graphics_engine, false);
        PostDraw(graphics_engine, false);
      }
    }

    graphics_engine.PopModelViewMatrix();

    if (RedirectRenderingToTexture() && update_rendering)
    {
      EndBackupTextureRendering(graphics_engine);

//       // Restore the main frame buffer object
//       if (prev_fbo_.IsValid())
//       {
//         prev_fbo_->Activate();
// 
//         graphics_engine.SetViewport(0, 0, prev_fbo_->GetWidth(), prev_fbo_->GetHeight());
//         prev_fbo_->ApplyClippingRegion();
//         graphics_engine.ApplyModelViewMatrix();
//         graphics_engine.SetOrthographicProjectionMatrix(prev_fbo_->GetWidth(), prev_fbo_->GetHeight());
//       }

      TexCoordXForm texxform;
      texxform.uwrap = TEXWRAP_CLAMP;
      texxform.vwrap = TEXWRAP_CLAMP;
      texxform.FlipVCoord(true);
      GetGraphicsDisplay()->GetGraphicsEngine()->QRP_1Tex(GetX(), GetY(), GetWidth(), GetHeight(), backup_texture_, texxform, Color(color::White));
    }

    if (view_layout_)
    {
      view_layout_->ResetQueueDraw();
    }

    draw_cmd_queued_ = false;
    child_draw_cmd_queued_ = false;
    full_view_draw_cmd_ = false;
    update_backup_texture_ = false;
  }

  void View::BeginBackupTextureRendering(GraphicsEngine& graphics_engine)
  {
    ObjectPtr<IOpenGLFrameBufferObject> prev_fbo_ = GetGraphicsDisplay()->GetGpuDevice()->GetCurrentFrameBufferObject();

    graphics_engine.PushModelViewMatrix(Matrix4::TRANSLATE(-GetX(), -GetY(), 0));
    
    const int width = GetWidth();
    const int height = GetHeight();

    graphics_engine.SetOrthographicProjectionMatrix(width, height);

    if (!backup_texture_.IsValid() || (backup_texture_->GetWidth() != width) || (backup_texture_->GetHeight() != height))
    {
      backup_texture_ = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture(width, height, 1, BITFMT_R8G8B8A8, NUX_TRACKER_LOCATION);
      backup_depth_texture_ = GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture(width, height, 1, BITFMT_D24S8, NUX_TRACKER_LOCATION);
    }

    backup_fbo_->FormatFrameBufferObject(width, height, BITFMT_R8G8B8A8);
    backup_fbo_->EmptyClippingRegion();
    backup_fbo_->SetRenderTarget(0, backup_texture_->GetSurfaceLevel(0));
    backup_fbo_->SetDepthSurface(backup_depth_texture_->GetSurfaceLevel(0));
    backup_fbo_->Activate();

    graphics_engine.SetViewport(0, 0, width, height);
  }

  void View::EndBackupTextureRendering(GraphicsEngine& graphics_engine)
  {
    graphics_engine.PopModelViewMatrix();
    GetWindowThread()->GetWindowCompositor().RestoreRenderingSurface();
  }

  void View::Draw(GraphicsEngine &graphics_engine, bool force_draw)
  {

  }

  void View::DrawContent(GraphicsEngine &graphics_engine, bool force_draw)
  {

  }

  void View::PostDraw(GraphicsEngine &graphics_engine, bool force_draw)
  {

  }

  void View::QueueDraw()
  {
    if (draw_cmd_queued_)
      return;

    WindowThread* application = GetWindowThread();
    if (application)
    {
      application->AddToDrawList(this);
      application->RequestRedraw();
    }
    
    // Report to a parent view with redirect_rendering_to_texture_ set to true that one of its children
    // needs to be redrawn.
    ReportDrawToRedirectedView();

//     if (view_layout_)
//     {
//       // If this view has requested a draw, then all of it children in the view_layout_
//       // need to be redrawn as well.
//       view_layout_->QueueDraw();
//     }

    draw_cmd_queued_ = true;
    queue_draw.emit(this);
  }

  void View::NeedSoftRedraw()
  {
    //GetWindowCompositor()..AddToDrawList(this);
    WindowThread* application = GetWindowThread();
    if (application)
    {
        application->AddToDrawList(this);
        application->RequestRedraw();
    }
    //draw_cmd_queued_ = false;
  }

  bool View::IsRedrawNeeded()
  {
    return draw_cmd_queued_;
  }

  bool View::IsFullRedraw() const
  {
    return full_view_draw_cmd_;
  }

  void View::DoneRedraw()
  {
    draw_cmd_queued_ = false;

    if (view_layout_)
    {
      view_layout_->DoneRedraw();
    }
  }

  Layout* View::GetLayout()
  {
    return view_layout_;
  }

  Layout *View::GetCompositionLayout()
  {
    return GetLayout();
  }

  bool View::SetLayout(Layout *layout)
  {
    NUX_RETURN_VALUE_IF_NULL(layout, false);
    nuxAssert(layout->IsLayout());
    NUX_RETURN_VALUE_IF_TRUE(view_layout_ == layout, true);

    Area *parent = layout->GetParentObject();

    if (parent == this)
    {
      nuxAssert(view_layout_ == layout);
      return false;
    }
    else if (parent != 0)
    {
      nuxDebugMsg(0, "[View::SetCompositionLayout] Object already has a parent. You must UnParent the object before you can parenting again.");
      return false;
    }

    if (view_layout_)
    {
      /* we need to emit the signal before the unparent, just in case
         one of the callbacks wanted to use this object */

      LayoutRemoved.emit(this, view_layout_);
      view_layout_->UnParentObject();
    }
    layout->SetParentObject(this);
    view_layout_ = layout;

    GetWindowThread()->QueueObjectLayout(this);

    LayoutAdded.emit(this, view_layout_);

    view_layout_->queue_draw.connect(sigc::mem_fun(this, &View::ChildViewQueuedDraw));
    view_layout_->child_queue_draw.connect(sigc::mem_fun(this, &View::ChildViewQueuedDraw));
    return true;
  }

  void View::ChildViewQueuedDraw(Area* area)
  {
    if (child_draw_cmd_queued_)
      return;
    child_draw_cmd_queued_ = true;
    child_queue_draw.emit(area);
  }

  void View::OnChildFocusChanged(/*Area *parent,*/ Area *child)
  {
    ChildFocusChanged.emit(/*parent,*/ child);
  }

  bool View::SetCompositionLayout(Layout *layout)
  {
    return SetLayout(layout);
  }

  void View::RemoveLayout()
  {
    NUX_RETURN_IF_NULL(view_layout_);

    if (view_layout_)
      view_layout_->UnParentObject();

    view_layout_ = 0;
  }

  void View::RemoveCompositionLayout()
  {
    RemoveLayout();
  }

  bool View::SearchInAllSubNodes(Area *bo)
  {
    if (view_layout_)
      return view_layout_->SearchInAllSubNodes(bo);

    return false;
  }

  bool View::SearchInFirstSubNodes(Area *bo)
  {
    if (view_layout_)
      return view_layout_->SearchInFirstSubNodes(bo);

    return false;
  }

  void View::SetGeometry(const Geometry &geo)
  {
    Area::SetGeometry(geo);
    ComputeContentSize();
    PostResizeGeometry();
  }

  void View::SetFont(ObjectPtr<FontTexture> font)
  {
    _font = font;
  }

  ObjectPtr<FontTexture> View::GetFont()
  {
    if (!_font.IsValid())
      _font = GetSysFont();
    return _font;
  }

  void View::SetTextColor(const Color &color)
  {
    m_TextColor = color;
  }

  Color View::GetTextColor()
  {
    return m_TextColor;
  }

  void View::EnableView()
  {
    view_enabled_ = true;
  }

  void View::DisableView()
  {
    view_enabled_ = false;
  }

  void View::SetEnableView(bool enable)
  {
    if (enable)
    {
      EnableView();
    }
    else
    {
      DisableView();
    }
  }

  bool View::IsViewEnabled() const
  {
    return view_enabled_;
  }

  void View::GeometryChangePending()
  {
    QueueDraw();
  }

  void View::GeometryChanged()
  {
    QueueDraw();
  }

  Area* View::FindAreaUnderMouse(const Point& mouse_position, NuxEventType event_type)
  {
    bool mouse_inside = TestMousePointerInclusionFilterMouseWheel(mouse_position, event_type);

    if (mouse_inside == false)
      return NULL;

    if (view_layout_)
    {
      Area* view = view_layout_->FindAreaUnderMouse(mouse_position, event_type);

      if (view)
        return view;
    }

    if ((event_type == NUX_MOUSE_WHEEL) && (!AcceptMouseWheelEvent()))
      return NULL;
    return this;
  }

  Area* View::FindKeyFocusArea(unsigned int key_symbol,
                      unsigned long x11_key_code,
                      unsigned long special_keys_state)
  {
    if (has_key_focus_)
    {
      return this;
    }
    else if (next_object_to_key_focus_area_)
    {
      return next_object_to_key_focus_area_->FindKeyFocusArea(key_symbol, x11_key_code, special_keys_state);
    }
    return NULL;
  }

  Area* View::KeyNavIteration(KeyNavDirection direction)
  {
    if (next_object_to_key_focus_area_)
    {
      return NULL;
    }

    if (IsVisible() == false)
      return NULL;

    if (AcceptKeyNavFocus())
    {
      QueueDraw();
      return this;
    }
    else if (view_layout_)
    {
      return view_layout_->KeyNavIteration(direction);
    }

    return NULL;
  }

  bool View::AcceptKeyNavFocus()
  {
    return true;
  }

  void View::SetRedirectRenderingToTexture(bool redirect)
  {
    if (redirect_rendering_to_texture_ == redirect)
    {
      return;
    }

    if ((redirect_rendering_to_texture_ == false) && redirect)
    {
      update_backup_texture_ = true;
    }

    redirect_rendering_to_texture_ = redirect;
    if (redirect == false)
    {
      // Free the texture of this view
      backup_texture_.Release();
    }
  }

  bool View::RedirectRenderingToTexture() const
  {
    return redirect_rendering_to_texture_;
  }

  void View::SetUpdateBackupTexture(bool update)
  {
    update_backup_texture_ = update;
  }

  bool View::UpdateBackupTexture()
  {
    return update_backup_texture_;
  }

  void View::ReportDrawToRedirectedView()
  {
    Area* parent = GetParentObject();

    while (parent && !parent->Type().IsDerivedFromType(View::StaticObjectType))
    {
      parent = parent->GetParentObject();
    }

    if (parent)
    {
      View* view = static_cast<View*>(parent);
      if (view->RedirectRenderingToTexture() && (view->UpdateBackupTexture() == false))
      {
        view->SetUpdateBackupTexture(true);
        view->ReportDrawToRedirectedView();
      }
      else if (view->RedirectRenderingToTexture() && (view->UpdateBackupTexture() == true))
      {
        return;
      }
      else
      {
        view->ReportDrawToRedirectedView();
      }
    }

  }

}
