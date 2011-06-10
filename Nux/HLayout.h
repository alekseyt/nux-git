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


#ifndef HLAYOUT_H
#define HLAYOUT_H

#include "Layout.h"

namespace nux
{

  class HLayout: public Layout
  {
    NUX_DECLARE_OBJECT_TYPE (HLayout, Layout);
  public:
    HLayout (NUX_FILE_LINE_PROTO);
    HLayout (NString name, NUX_FILE_LINE_PROTO);
    ~HLayout ();

    virtual long ComputeLayout2 ();
    virtual void ComputePosition2 (float offsetX, float offsetY);
    virtual void HLayoutManagement (t_s32 width, t_s32 height);
    virtual t_u32 GetMaxStretchFactor ();

    virtual void GetCompositeList (std::list<Area *> *ViewList);

    //! Compute the how elements are spread inside the layout
    /*!
        @param remaining_width  Size that remains after subtracting elements width, inner and outer margins from the content width.
        @param offset_space     The space at the left of all elements.
        @param element_margin   The margin between elements.
    */
    void ComputeStacking (t_s32 remaining_width, t_s32 &offset_space, t_s32 &element_margin);

  protected:
    virtual long DoFocusUp  (IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
    virtual long DoFocusDown (IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
  };
}

#endif // HLAYOUT_H
