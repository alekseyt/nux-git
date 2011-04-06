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


#ifndef RANGEVALUEINTEGERPROPERTYITEM_H
#define RANGEVALUEINTEGERPROPERTYITEM_H

namespace nux
{

//#include "RangeValueInteger.h"
  class RangeValueInteger;

  class RangeValueIntegerPropertyItem: public SectionProperty, public RangeValueInteger
  {
    NODE_XML_CLASS_MEMBER (RangeValueIntegerPropertyItem);
    NODE_SKIP_CHILD (false);
    NODE_SIG_RECEIVER (RecvPropertyChange, RangeValueInteger *);

  public:
    RangeValueIntegerPropertyItem (const char *name, int Value = 0, int MinValue = 0, int MaxValue = 100);
    virtual ~RangeValueIntegerPropertyItem();

    virtual long ProcessPropertyEvent (IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
    virtual void DrawProperty (GraphicsEngine &GfxContext, TableCtrl *table, bool force_draw, Geometry geo, const BasePainter &Painter, RowHeader *row, const std::vector<ColumnHeader>& column_vector, Color ItemBackgroundColor = Color (0x0) );
    virtual void ComputePropertyLayout (int x, int y, RowHeader *row, const std::vector<ColumnHeader>& column_vector);
    virtual int GetItemBestHeight();
  private:
  };


}

#endif // RANGEVALUEINTEGERPROPERTYITEM_H
