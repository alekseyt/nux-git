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
#include "Area.h"
#include "NuxGraphics/GraphicsEngine.h"
#include "Layout.h"
#include "VSplitter.h"
#include "HSplitter.h"
#include "BaseWindow.h"
#include "MenuPage.h"

namespace nux
{

  NUX_IMPLEMENT_OBJECT_TYPE (Area);

  Area::Area (NUX_FILE_LINE_DECL)
    :   InitiallyUnownedObject (NUX_FILE_LINE_PARAM)
    ,   _is_focused (0)
    ,   geometry_ (0, 0, DEFAULT_WIDGET_WIDTH, DEFAULT_WIDGET_HEIGHT)
    ,   _min_size (AREA_MIN_WIDTH, AREA_MIN_HEIGHT)
    ,   _max_size (AREA_MAX_WIDTH, AREA_MAX_HEIGHT)
  {
    _parent_area = NULL;
    next_object_to_key_focus_area_ = NULL;
    has_key_focus_ = false;

    _stretch_factor = 1;
    _layout_properties = NULL;
    visible_ = true;
    sensitive_ = true;

    on_geometry_change_reconfigure_parent_layout_ = true;
    _accept_mouse_wheel_event = false;
    _accept_keyboard_event = false;

    _2d_xform.Identity ();
    _3d_xform.Identity ();
    _3d_area = false;
  }


  Area::~Area()
  {
    if (_layout_properties)
      delete _layout_properties;
  }

  const NString &Area::GetBaseString() const
  {
    return _base_string;
  }

  void Area::SetBaseString (const TCHAR *Caption)
  {
    _base_string = Caption;
  }

  void Area::CheckMinSize()
  {
    int w = _min_size.width;
    w = Max<int>(AREA_MIN_WIDTH, w);
    int h = _min_size.height;
    h = Max<int>(AREA_MIN_HEIGHT, h);

    _min_size = Size(w, h);

    if (_min_size.width > _max_size.width)
    {
      _max_size.width = _min_size.width;
    }

    if (_min_size.height > _max_size.height)
    {
      _max_size.height = _min_size.height;
    }

    if (geometry_.width < _min_size.width)
    {
      geometry_.width = _min_size.width;
    }

    if (geometry_.height < _min_size.height )
    {
      geometry_.height = _min_size.height;
    }
  }

  void Area::CheckMaxSize()
  {
    int w = _max_size.width;
    w = Min<int>(AREA_MAX_WIDTH, w);
    int h = _max_size.height;
    h = Min<int>(AREA_MAX_HEIGHT, h);

    _max_size = Size(w, h);

    if (_min_size.width > _max_size.width)
    {
      _min_size.width = _max_size.width;
    }

    if (_min_size.height > _max_size.height)
    {
      _min_size.height = _max_size.height;
    }

    if (geometry_.width > _max_size.width)
    {
      geometry_.width = _max_size.width;
    }

    if (geometry_.height > _max_size.height)
    {
      geometry_.height = _max_size.height;
    }
  }

  void Area::SetMinimumSize (int w, int h)
  {
    nuxAssert (w >= 0);
    nuxAssert (h >= 0);
    _min_size = Size(w, h);

    CheckMinSize();

    ReconfigureParentLayout();
  }

  void Area::SetMaximumSize (int w, int h)
  {
    nuxAssert (w >= 0);
    nuxAssert (h >= 0);
    _max_size = Size(w, h);

    CheckMaxSize();

    ReconfigureParentLayout();
  }

  void Area::SetMinMaxSize (int w, int h)
  {
    nuxAssert (w >= 0);
    nuxAssert (h >= 0);
    SetMinimumSize (w, h);
    SetMaximumSize (w, h);

    //ReconfigureParentLayout();
  }

  void Area::ApplyMinWidth()
  {
    geometry_.width = _min_size.width;

    ReconfigureParentLayout();
  }

  void Area::ApplyMinHeight()
  {
    geometry_.height = _min_size.height;

    ReconfigureParentLayout();
  }

  void Area::ApplyMaxWidth()
  {
    geometry_.width = _max_size.width;

    ReconfigureParentLayout();
  }

  void Area::ApplyMaxHeight()
  {
    geometry_.height = _max_size.height;

    ReconfigureParentLayout();
  }

  Size Area::GetMinimumSize() const
  {
    return _min_size;
  }

  Size Area::GetMaximumSize() const
  {
    return _max_size;
  }

  void Area::SetMinimumWidth (int w)
  {
    nuxAssert (w >= 0);
    _min_size.width = w;
    CheckMinSize();
    ReconfigureParentLayout();
  }

  void Area::SetMaximumWidth (int w)
  {
    nuxAssert (w >= 0);
    _max_size.width = w;
    CheckMaxSize();
    ReconfigureParentLayout();
  }

  void Area::SetMinimumHeight (int h)
  {
    nuxAssert (h >= 0);
    _min_size.height = h;
    CheckMinSize();
    ReconfigureParentLayout();
  }

  void Area::SetMaximumHeight (int h)
  {
    nuxAssert (h >= 0);
    _max_size.height = h;
    CheckMaxSize();
    ReconfigureParentLayout();
  }

  int Area::GetMinimumWidth() const
  {
    return _min_size.width;
  }

  int Area::GetMaximumWidth() const
  {
    return _max_size.width;
  }

  int Area::GetMinimumHeight() const
  {
    return _min_size.height;
  }

  int Area::GetMaximumHeight() const
  {
    return _max_size.height;
  }

  unsigned int Area::GetStretchFactor()
  {
    return _stretch_factor;
  }

  void Area::SetStretchFactor (unsigned int sf)
  {
    // re implemented by Layout
    _stretch_factor = sf;
  }

  bool Area::SetParentObject (Area *parent)
  {
    if (parent == 0)
    {
      nuxAssertMsg(0, TEXT("[Area::SetParentObject] Invalid parent obejct."));
      return false;
    }

    if (_parent_area && (_parent_area != parent))
    {
      nuxAssertMsg (0, TEXT ("[Area::SetParentObject] Object already has a parent. You must UnParent the object before you can parenting again.") );
      return false;
    }

    if (_parent_area)
    {
      // Already parented to the same area. Return.
      return true;
    }

    _parent_area = parent;
    Reference();

    return true;
  }

  void Area::UnParentObject()
  {
    if (_parent_area)
    {
      _parent_area = 0;
      UnReference();
    }
  }

  Area *Area::GetParentObject() const
  {
    return _parent_area;
  }

  int Area::GetBaseX     () const
  {
    return geometry_.x;
  }

  int Area::GetBaseY     () const
  {
    return geometry_.y;
  }

  int Area::GetBaseWidth    () const
  {
    return geometry_.width;
  }

  int Area::GetBaseHeight   () const
  {
    return geometry_.height;
  }

  void Area::SetGeometry(int x, int y, int w, int h)
  {
    h = nux::Clamp<int> (h, _min_size.height, _max_size.height);
    w = nux::Clamp<int> (w, _min_size.width, _max_size.width);

    nux::Geometry geometry(x, y, w, h);
    if (geometry_ == geometry)
      return;

    GeometryChangePending();
    geometry_ = geometry;
    ReconfigureParentLayout();
    GeometryChanged();

    OnGeometryChanged.emit(this, geometry_);
  }

  void Area::SetGeometry (const Geometry &geo)
  {
    SetGeometry (geo.x, geo.y, geo.width, geo.height);
  }

  Geometry const& Area::GetGeometry() const
  {
    return geometry_;
  }

  void Area::SetBaseX(int x)
  {
    SetGeometry(x, geometry_.y, geometry_.width, geometry_.height);
  }

  void Area::SetBaseY    (int y)
  {
    SetGeometry (geometry_.x, y, geometry_.width, geometry_.height);
  }

  void Area::SetBaseXY    (int x, int y)
  {
    SetGeometry (x, y, geometry_.width, geometry_.height);
  }

  void Area::SetBaseSize (int w, int h)
  {
    SetGeometry (geometry_.x, geometry_.y, w, h);
  }

  void Area::SetBaseWidth (int w)
  {
    SetGeometry (geometry_.x, geometry_.y, w, geometry_.height);
  }

  void Area::SetBaseHeight (int h)
  {
    SetGeometry (geometry_.x, geometry_.y, geometry_.width, h);
  }

  void Area::IncreaseSize (int x, int y)
  {
    geometry_.OffsetPosition (x, y);
    OnResize.emit (geometry_.x, geometry_.y, geometry_.width, geometry_.height );
  }

  long Area::ComputeContentSize()
  {
    return (eCompliantWidth | eCompliantHeight);
  }

  void Area::ComputeContentPosition (float offsetX, float offsetY)
  {

  }

  void Area::SetReconfigureParentLayoutOnGeometryChange(bool reconfigure_parent_layout)
  {
    on_geometry_change_reconfigure_parent_layout_ = reconfigure_parent_layout;
  }
  
  bool Area::ReconfigureParentLayoutOnGeometryChange()
  {
    return on_geometry_change_reconfigure_parent_layout_;
  }

  void Area::ReconfigureParentLayout(Area *child)
  {
    if(on_geometry_change_reconfigure_parent_layout_ == false)
      return;

    if (GetWindowThread () && GetWindowThread ()->IsComputingLayout() )
    {
      // there is no need to do the following while we are already computing the layout.
      // If we do, we will end up in an infinite loop.
      return;
    }

    if (this->Type().IsDerivedFromType (View::StaticObjectType) )
    {
      // The object that is being resized is a View object and it has a parent.
      if (this->OwnsTheReference() == false && this->GetParentObject())
      {
        // Only reference parented areas.
        this->Reference();
      }

      View *ic = static_cast<View *>(this);

      if (ic->CanBreakLayout() )
      {

        if ( (child != 0) &&
             (ic->Type().IsObjectType (VSplitter::StaticObjectType) || ic->Type().IsObjectType (HSplitter::StaticObjectType) ) )
        {
          // If this element is a Splitter, then we submit its child to the refresh list. We don't want to submit the
          // splitter because this will cause a redraw of all parts of the splitter (costly and unnecessary).
          GetWindowThread ()->QueueObjectLayout (child);
        }
        else
        {
          GetWindowThread ()->QueueObjectLayout (ic);
        }
      }
      else if (ic->_parent_area)
        ic->_parent_area->ReconfigureParentLayout (this);
      else
      {
        GetWindowThread ()->QueueObjectLayout (ic);
      }
    }
    else if (this->Type().IsDerivedFromType (Layout::StaticObjectType) )
    {
      // The object that is being resized is a View object and it has a parent.
      if (this->OwnsTheReference() == false && this->GetParentObject())
      {
        // Only reference parented areas.
        this->Reference();
      }

      Layout *layout = static_cast<Layout *>(this);

      if (layout->_parent_area)
      {
        if (layout->_parent_area->Type().IsDerivedFromType (View::StaticObjectType) )
        {
          View *ic = (View *) (layout->_parent_area);

          if (ic->CanBreakLayout() )
          {
            if ( (child != 0) &&
                 (ic->Type().IsObjectType (VSplitter::StaticObjectType) || ic->Type().IsObjectType (HSplitter::StaticObjectType) ) )
            {
              // If the parent of this element is a splitter, then we submit its child to the refresh list. We don't want to submit the
              // splitter because this will cause a redraw of all parts of the splitter (costly and unnecessary).
              GetWindowThread ()->QueueObjectLayout (this);
            }
            else
            {
              GetWindowThread ()->QueueObjectLayout (ic);
            }
          }
          else
          {
            // The parent object of an object of type View is a Layout object type.
            layout->_parent_area->ReconfigureParentLayout (this);
          }
        }
        else
        {
          layout->_parent_area->ReconfigureParentLayout (this);
        }
      }
      else
      {
        // This is possibly the Main layout or the layout of a floating object (popup for example) unless the layout is not part of the object tree.
        GetWindowThread ()->QueueObjectLayout (layout);
      }
    }
    else
    {
      // The object that is being resized is a InputArea object.
      if (this->_parent_area)
      {
        // The object that is being resized is a View object and it has a parent.
        if (this->OwnsTheReference() == false && this->GetParentObject())
        {
          // Only reference parented areas.
          this->Reference();
        }

        // The parent object of an object of type InputArea is a Layout object type.
        this->_parent_area->ReconfigureParentLayout (this);
      }
    }
  }

  void Area::RequestBottomUpLayoutComputation (Area *bo_initiator)
  {
    if (_parent_area && _parent_area->IsLayout() )
    {
      _parent_area->RequestBottomUpLayoutComputation (bo_initiator);
    }
  }

  void Area::SetLayoutProperties (LayoutProperties *properties)
  {
    if (_layout_properties)
      delete _layout_properties;

    _layout_properties = properties;
  }

  Area::LayoutProperties * Area::GetLayoutProperties ()
  {
    return _layout_properties;
  }

  void Area::SetVisible (bool visible)
  {
    if (visible_ == visible)
      return;

    visible_ = visible;

    OnVisibleChanged.emit (this, visible_);
  }

  bool Area::IsVisible ()
  {
    return visible_;
  }

  void Area::SetSensitive (bool sensitive)
  {
    if (sensitive_ == sensitive)
      return;

    sensitive_ = sensitive;

    OnSensitiveChanged.emit (this, sensitive_);
  }

  bool Area::IsSensitive ()
  {
    return sensitive_;
  }

  MinorDimensionPosition Area::GetPositioning()
  {
    return _positioning;
  }

  void Area::SetPositioning (MinorDimensionPosition p)
  {
    _positioning = p;
  }

  MinorDimensionSize Area::GetExtend()
  {
    return _extend;
  }

  void Area::SetExtend (MinorDimensionSize ext)
  {
    _extend = ext;
  }

  float Area::GetPercentage()
  {
    return _percentage;
  }

  void Area::SetPercentage (float p)
  {
    _percentage = p;
  }

  bool Area::IsLayoutDone()
  {
    return _layout_done;
  }

  void Area::SetLayoutDone (bool b)
  {
    _layout_done = b;
  }

  bool Area::IsArea () const
  {
    return this->Type ().IsDerivedFromType (Area::StaticObjectType);;
  }

  bool Area::IsInputArea () const
  {
    return this->Type ().IsDerivedFromType (InputArea::StaticObjectType);;
  }

  bool Area::IsView () const
  {
    return this->Type ().IsDerivedFromType (View::StaticObjectType);;
  }

  bool Area::IsLayout () const
  {
    return this->Type ().IsDerivedFromType (Layout::StaticObjectType);
  }

  bool Area::IsViewWindow () const
  {
    return this->Type ().IsDerivedFromType (BaseWindow::StaticObjectType);
  }

  bool Area::IsSpaceLayout() const
  {
    return this->Type().IsDerivedFromType(SpaceLayout::StaticObjectType);;
  }

  void Area::Set2DMatrix (const Matrix4 &mat)
  {
    _2d_xform = mat;
  }

  void Area::Set2DTranslation (float tx, float ty, float tz)
  {
    _2d_xform.Translate (tx, ty, tz);
  }

  Matrix4 Area::Get2DMatrix () const
  {
    return _2d_xform;
  }

  Matrix4 Area::Get3DMatrix () const
  {
    return _3d_xform;
  }

  bool Area::Is3DArea () const
  {
    return _3d_area;
  }

  static void MatrixXFormGeometry (const Matrix4 &matrix, Geometry &geo)
  {
    Vector4 in (geo.x, geo.y, 0, 1);
    // This is mean only for translation matrices. It will not work with matrices containing rotations or scalings.
    Vector4 out = matrix * in;
    geo.x = out.x;
    geo.y = out.y;
  }

  void Area::InnerGetAbsoluteGeometry (Geometry &geometry)
  {
    if (this->Type ().IsDerivedFromType (BaseWindow::StaticObjectType) || (this == GetWindowThread ()->GetMainLayout ()))
    {
      geometry.OffsetPosition (geometry_.x, geometry_.y);
      return;
    }

    MatrixXFormGeometry (_2d_xform, geometry);

    Area *parent = GetParentObject ();
    if (parent)
      parent->InnerGetAbsoluteGeometry (geometry);
  }

  Geometry Area::GetAbsoluteGeometry () const
  {
    if (Type().IsDerivedFromType(BaseWindow::StaticObjectType) ||
      Type().IsDerivedFromType(MenuPage::StaticObjectType) ||
      (this == GetWindowThread()->GetMainLayout()))
    {
      // Do not apply the _2D_xform matrix  to a BaseWindow or the main layout
      return geometry_;
    }
    else
    {
      nux::Geometry geo = geometry_;
      MatrixXFormGeometry (_2d_xform, geo);

      Area *parent = GetParentObject ();
      if (parent)
        parent->InnerGetAbsoluteGeometry (geo);

      return geo;
    }
  }

  int Area::GetAbsoluteX () const
  {
    return GetAbsoluteGeometry ().x;
  }

  int Area::GetAbsoluteY () const
  {
    return GetAbsoluteGeometry ().y;
  }

  int Area::GetAbsoluteWidth () const
  {
    return GetAbsoluteGeometry ().width;
  }

  int Area::GetAbsoluteHeight () const
  {
    return GetAbsoluteGeometry ().height;
  }

  void Area::InnerGetRootGeometry (Geometry &geometry)
  {
    if (this->Type ().IsDerivedFromType (BaseWindow::StaticObjectType) || (this == GetWindowThread ()->GetMainLayout ()))
      return;

    MatrixXFormGeometry (_2d_xform, geometry);

    Area *parent = GetParentObject ();
    if (parent)
      parent->InnerGetRootGeometry (geometry);
  }

  Geometry Area::GetRootGeometry () const
  {
    nux::Geometry geo = geometry_;
    MatrixXFormGeometry (_2d_xform, geo);

    if (Type().IsDerivedFromType(BaseWindow::StaticObjectType) || (this == GetWindowThread()->GetMainLayout()))
    {
      return geo;
    }
    else
    {
      Area *parent = GetParentObject ();
      if (parent)
        parent->InnerGetRootGeometry (geo);

      return geo;
    }
  }

  int Area::GetRootX () const
  {
    return GetRootGeometry ().x;
  }

  int Area::GetRootY () const
  {
    return GetRootGeometry ().y;
  }

  int Area::GetRootWidth () const
  {
    return GetRootGeometry ().width;
  }

  int Area::GetRootHeight() const
  {
    return GetRootGeometry().height;
  }

  Area* Area::GetToplevel()
  {
    return GetRootParent();
  }

  Area* Area::GetRootParent()
  {
    if (Type().IsDerivedFromType(BaseWindow::StaticObjectType) || (this == GetWindowThread()->GetMainLayout()))
    {
      return this;
    }

    Area* parent = GetParentObject();
    if (!parent) //we didn't find a way to salvation!
    {
      return 0;
    }
    return parent->GetRootParent();
  }

  Area* Area::GetTopLevelViewWindow()
  {
    Area* area = GetRootParent();

    if (area && area->IsViewWindow ())
      return area;

    return NULL;
  }

  bool Area::HasTopLevelParent ()
  {
    if (GetRootParent())
    {
      return true;
    }
    return false;
  }

  bool Area::IsChildOf(Area* parent)
  {
    if (this == parent)
      return true;

    if (!parent || !_parent_area)
      return false;

    return _parent_area->IsChildOf(parent);    
  }

  void Area::QueueRelayout ()
  {
    nux::GetWindowThread ()->QueueObjectLayout (this);
  }
  
  void Area::SetAcceptKeyboardEvent(bool accept_keyboard_event)
  {
    _accept_keyboard_event = accept_keyboard_event;
  }

  bool Area::AcceptKeyboardEvent() const
  {
    return _accept_keyboard_event;
  }

  void Area::SetAcceptMouseWheelEvent(bool accept_mouse_wheel_event)
  {
    _accept_mouse_wheel_event = accept_mouse_wheel_event;
  }

  bool Area::AcceptMouseWheelEvent() const
  {
    return _accept_mouse_wheel_event;
  }

  bool Area::TestMousePointerInclusion(const Point& mouse_position, NuxEventType event_type)
  {
    if (IsSensitive() == false)
      return false;

    bool mouse_pointer_inside_area = false;

    if (Type().IsDerivedFromType(MenuPage::StaticObjectType))
    {
      // A MenuPage geometry is already in absolute coordinates.
      mouse_pointer_inside_area = geometry_.IsInside(mouse_position);
    }
    else
    {
      mouse_pointer_inside_area = GetAbsoluteGeometry().IsInside(mouse_position);
    }

    if ((event_type == NUX_MOUSE_WHEEL) && mouse_pointer_inside_area)
    {
      if (_accept_mouse_wheel_event == false)
        return NULL;
    }

    return mouse_pointer_inside_area;
  }

  bool Area::TestMousePointerInclusionFilterMouseWheel(const Point& mouse_position, NuxEventType event_type)
  {
    if (IsSensitive() == false)
      return false;

    bool mouse_pointer_inside_area = false;

    if (Type().IsDerivedFromType(MenuPage::StaticObjectType))
    {
      // A MenuPage geometry is already in absolute coordinates.
      mouse_pointer_inside_area = geometry_.IsInside(mouse_position);
    }
    else
    {
      mouse_pointer_inside_area = GetAbsoluteGeometry().IsInside(mouse_position);
    }

    return mouse_pointer_inside_area;
  }

  Area* Area::FindAreaUnderMouse(const Point& mouse_position, NuxEventType event_type)
  {
    return NULL;
  }
  
  Area* Area::FindKeyFocusArea(unsigned int key_symbol,
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

  void Area::SetPathToKeyFocusArea()
  {
    has_key_focus_ = true;
    next_object_to_key_focus_area_ = NULL;

    Area* child = this;
    Area* parent = GetParentObject();

    while (parent)
    {
      parent->next_object_to_key_focus_area_ = child;
      parent->next_object_to_key_focus_area_->Reference();
      parent->has_key_focus_ = false;
      child = parent;
      parent = parent->GetParentObject();
    }
  }

  void Area::ResetDownwardPathToKeyFocusArea()
  {
    has_key_focus_ = false;
    if (next_object_to_key_focus_area_)
    {
      next_object_to_key_focus_area_->ResetDownwardPathToKeyFocusArea();
    }
    if(next_object_to_key_focus_area_)
      next_object_to_key_focus_area_->UnReference();

    next_object_to_key_focus_area_ = NULL;
  }

  void Area::ResetUpwardPathToKeyFocusArea()
  {
    has_key_focus_ = false;
    if (_parent_area)
    {
      _parent_area->ResetUpwardPathToKeyFocusArea();
    }
    if(next_object_to_key_focus_area_)
      next_object_to_key_focus_area_->UnReference();

    next_object_to_key_focus_area_ = NULL;
  }

  bool Area::InspectKeyEvent(unsigned int eventType,
    unsigned int keysym,
    const char* character)
  {
    return false;
  }

  bool Area::AcceptKeyNavFocus()
  {
    if (IsSensitive() == false)
      return false;

    return true;
  }

  Area* Area::KeyNavIteration(KeyNavDirection direction)
  {
    return NULL;
  }

  bool Area::HasKeyFocus() const
  {
    return has_key_focus_;
  }

  bool Area::IsMousePointerInside() const
  {
    Geometry geo = GetAbsoluteGeometry();
    Point position = GetWindowCompositor().GetMousePosition();
    
    if (geo.IsInside(position))
      return true;

    return false;
  }
}

