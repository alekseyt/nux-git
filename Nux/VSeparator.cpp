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

#include "AbstractSeparator.h"
#include "VSeparator.h"

NAMESPACE_BEGIN_GUI

VSeparator::VSeparator()
//:   AbstractSeparator(0xFF999999, 0.0f, 151.0f, 10)
{
    InitializeWidgets();
    InitializeLayout();
}

VSeparator::VSeparator(const Color& color, float Alpha0, float Alpha1, int Border)
:   AbstractSeparator(color, Alpha0, Alpha1, Border)
{
    InitializeWidgets();
    InitializeLayout();
}

VSeparator::~VSeparator()
{

}

void VSeparator::InitializeWidgets()
{
    SetMinimumWidth(3);
    SetMaximumWidth(3);
}

void VSeparator::InitializeLayout()
{

}

void VSeparator::DestroyLayout()
{

}

long VSeparator::ProcessEvent(IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
{
    return TraverseInfo;
}

void VSeparator::Draw(GraphicsContext& GfxContext, bool force_draw)
{
    Geometry base = GetGeometry();
    int x0 = base.x + base.GetWidth()/2;
    base.OffsetPosition(0, 3);
    base.OffsetSize(0, -6);
    GetThreadGraphicsContext()->GetRenderStates().SetBlend(TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if(base.GetHeight() - 2 * m_BorderSize > 0)
    {
        Color color0 = m_Color;
        Color color1 = m_Color;
        color0.SetAlpha(m_Alpha0);
        color1.SetAlpha(m_Alpha1);
        gPainter.Draw2DLine(GfxContext, x0, base.y, x0, base.y + m_BorderSize, color0, color1);
        gPainter.Draw2DLine(GfxContext, x0, base.y + m_BorderSize, x0, base.y + base.GetHeight() - m_BorderSize, color1, color1);
        gPainter.Draw2DLine(GfxContext, x0, base.y + base.GetHeight() - m_BorderSize, x0, base.y + base.GetHeight(), color1, color0);
    }
    else
    {
        Color color1 = m_Color;
        color1.SetAlpha(m_Alpha1);
        gPainter.Draw2DLine(GfxContext, x0, base.y, x0, base.y + base.GetHeight(), color1);
    }
    GetThreadGraphicsContext()->GetRenderStates().SetBlend(FALSE);
}
NAMESPACE_END_GUI
