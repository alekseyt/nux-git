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
#include "NuxImage/ImageSurface.h"
#include "ColorArea.h"

namespace nux { //NUX_NAMESPACE_BEGIN

ColorArea::ColorArea(Color color, NUX_FILE_LINE_DECL)
:   ActiveInterfaceObject(NUX_FILE_LINE_PARAM)
,   m_Color(color)
{
    SetMinMaxSize(50, 50);
}

ColorArea::~ColorArea()
{
}

long ColorArea::ProcessEvent(IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
    return TraverseInfo;
}

void ColorArea::Draw(GraphicsContext& GfxContext, bool force_draw)
{
    gPainter.Paint2DQuadColor(GfxContext, GetGeometry(), m_Color);
}

void ColorArea::DrawContent(GraphicsContext& GfxContext, bool force_draw)
{

}

void ColorArea::PostDraw(GraphicsContext& GfxContext, bool force_draw)
{

}

void ColorArea::SetColor(Color color)
{
    m_Color = color;
    NeedRedraw();
}


} //NUX_NAMESPACE_END
