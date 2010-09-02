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


#ifndef RANGEVALUEPROPERTYITEM_H
#define RANGEVALUEPROPERTYITEM_H

NAMESPACE_BEGIN_GUI

//#include "RangeValue.h"
class RangeValue;

class RangeValuePropertyItem: public SectionProperty, public RangeValue
{
    NODE_XML_CLASS_MEMBER(RangeValuePropertyItem);
    NODE_SKIP_CHILD(false);
    NODE_SIG_RECEIVER(RecvPropertyChange, const weaksmptr(RangeValue));

public:
    RangeValuePropertyItem(const char* name, float Value = 0.0f, float MinValue = 0.0f, float MaxValue = 1.0f);
    virtual ~RangeValuePropertyItem();

    virtual long ProcessPropertyEvent(IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
    virtual void DrawProperty(GraphicsContext& GfxContext, TableCtrl* table, bool force_draw, Geometry geo, const BasePainter& Painter, RowHeader* row, const std::vector<header2>& column_vector, Color ItemBackgroundColor = Color(0x0));
    virtual void ComputePropertyLayout(int x, int y, RowHeader* row, const std::vector<header2>& column_vector);
    virtual int GetItemBestHeight();
private:
};

NAMESPACE_END_GUI

#endif // RANGEVALUEPROPERTYITEM_H
