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
#include "HLayout.h"
#include "EditTextBox.h"
#include "CheckBox.h"

namespace nux { //NUX_NAMESPACE_BEGIN

CheckBox::CheckBox(const TCHAR* Caption, bool state, NUX_FILE_LINE_DECL)
:   AbstractButton(Caption, NUX_FILE_LINE_PARAM)
{
    m_hlayout = 0;
    m_TextArea = 0;

    InitializeLayout();
    InitializeWidgets();
    SetState(state);
    SetCaption(Caption);

    if(Caption)m_TextArea->OnMouseClick.connect(sigc::mem_fun(this, &CheckBox::RecvClick));
    if(Caption)m_TextArea->OnMouseMove.connect(sigc::mem_fun(this, &CheckBox::RecvMouseMove));
    if(Caption)m_TextArea->OnMouseEnter.connect(sigc::mem_fun(this, &CheckBox::RecvMouseEnter));
    if(Caption)m_TextArea->OnMouseLeave.connect(sigc::mem_fun(this, &CheckBox::RecvMouseLeave));
    if(Caption)m_TextArea->OnMouseUp.connect(sigc::mem_fun(this, &CheckBox::RecvMouseUp));
    if(Caption)m_TextArea->OnMouseDown.connect(sigc::mem_fun(this, &CheckBox::RecvMouseDown));
}

CheckBox::~CheckBox()
{
    DestroyLayout();
}

void CheckBox::InitializeWidgets()
{
    // Set Signals
    m_CheckArea->OnMouseClick.connect(sigc::mem_fun(this, &CheckBox::RecvClick));
    OnMouseClick.connect(sigc::mem_fun(this, &CheckBox::RecvClick));

    m_CheckArea->OnMouseMove.connect(sigc::mem_fun(this, &CheckBox::RecvMouseMove));
    OnMouseMove.connect(sigc::mem_fun(this, &CheckBox::RecvMouseMove));

    m_CheckArea->OnMouseEnter.connect(sigc::mem_fun(this, &CheckBox::RecvMouseEnter));
    OnMouseEnter.connect(sigc::mem_fun(this, &CheckBox::RecvMouseEnter));

    m_CheckArea->OnMouseLeave.connect(sigc::mem_fun(this, &CheckBox::RecvMouseLeave));
    OnMouseLeave.connect(sigc::mem_fun(this, &CheckBox::RecvMouseLeave));

    m_CheckArea->OnMouseUp.connect(sigc::mem_fun(this, &CheckBox::RecvMouseUp));
    OnMouseUp.connect(sigc::mem_fun(this, &CheckBox::RecvMouseUp));

    m_CheckArea->OnMouseDown.connect(sigc::mem_fun(this, &CheckBox::RecvMouseDown));
    OnMouseDown.connect(sigc::mem_fun(this, &CheckBox::RecvMouseDown));

    // Set Geometry
    m_CheckArea->SetMinimumSize(14, 14);
    m_CheckArea->SetGeometry(Geometry(0, 0, DEFAULT_WIDGET_WIDTH, DEFAULT_WIDGET_HEIGHT));

    m_TextArea->SetMinimumSize(14, 14);

    // Do not configure m_TextArea-> This is done in setCaption according to the size of the caption text.

    m_hlayout->SetHorizontalInternalMargin(4);
    m_hlayout->AddView(m_CheckArea, 0);
    m_hlayout->AddView(m_TextArea, 0);

    // This is convenient to make the layout and the CheckBox fit the check area and the caption area.
    // Since the check area is bigger than 4x4, it will force the layout and the CheckBox to grow.
    // This is useful if the CheckBox is put in a vertical layout and it has a stretch factor of 0. Then the width of the CheckBox
    // will be adjusted to fit the minimum width of the check area and the caption area.
    {
        m_hlayout->SetMinimumSize(1, 1);
        SetMinimumSize(14, 14);
        ApplyMinWidth();
        ApplyMinHeight();
    }

    SetCompositionLayout(m_hlayout);
}

void CheckBox::InitializeLayout()
{
    m_hlayout   = new HLayout("CheckBox");
    m_CheckArea = new CoreArea();
    m_TextArea  = new CoreArea();
}

void CheckBox::DestroyLayout()
{
}


long CheckBox::ProcessEvent(IEvent &ievent, long TraverseInfo, long ProcessEventInfo) 
{
    long ret = TraverseInfo;
    ret = m_CheckArea->OnEvent(ievent, ret, ProcessEventInfo);
    if(m_TextArea->GetBaseString().Length() !=0 )
    {
        ret = m_TextArea->OnEvent(ievent, ret, ProcessEventInfo);
    }
    ret = PostProcessEvent2(ievent, ret, ProcessEventInfo);
    return ret;
}

void CheckBox::Draw(GraphicsContext& GfxContext, bool force_draw)
{
    Geometry base = GetGeometry();
    
    gPainter.PaintBackground(GfxContext, base);
    gPainter.PaintTextLineStatic(GfxContext, GetFont(), m_TextArea->GetGeometry(), m_TextArea->GetBaseString().GetTCharPtr(), GetTextColor(), eAlignTextLeft);
    InteractState is;
    is.is_on = m_State;
    is.is_focus = m_TextArea->HasMouseFocus() ||
        HasMouseFocus() ||
        m_CheckArea->HasMouseFocus();

    is.is_prelight = m_TextArea->IsMouseInside()
        || IsMouseInside() ||
        m_CheckArea->IsMouseInside();

    gPainter.PaintCheckBox(GfxContext, m_CheckArea->GetGeometry(), is, Color(0xff000000));
}

void CheckBox::DrawContent(GraphicsContext& GfxContext, bool force_draw)
{

}

void CheckBox::PostDraw(GraphicsContext& GfxContext, bool force_draw)
{

}

void CheckBox::SetCaption(const TCHAR* caption)
{
    if(caption == 0 || (StringLength(caption) == 0))
    {
        m_TextArea->SetBaseString(TEXT(""));
    }
    else
        m_TextArea->SetBaseString(caption);
    m_TextArea->SetMinimumWidth(4 + GetFont()->GetStringWidth(m_TextArea->GetBaseString().GetTCharPtr()));
}

const NString& CheckBox::GetCaption() const
{
    return m_TextArea->GetBaseString();
}


void CheckBox::SetState(bool State)
{
    m_State = State;
    NeedRedraw();
}

void CheckBox::SetState(bool State, bool EmitSignal)
{
    m_State = State;
    if(EmitSignal)
    {
        sigToggled.emit();
        sigStateChanged.emit(m_State);
    }
    NeedRedraw();
}

bool CheckBox::GetState() const
{
    return m_State;
}

void CheckBox::RecvClick(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
    m_State = !m_State;
    sigStateToggled.emit(this);
    sigStateChanged.emit(m_State);
    NeedRedraw();
}

void CheckBox::RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
    NeedRedraw();
}

void CheckBox::RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
    NeedRedraw();
}

void CheckBox::RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
    NeedRedraw();
}

void CheckBox::RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
    NeedRedraw();
}

void CheckBox::RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
    NeedRedraw();
}

void CheckBox::EmitStateSignal()
{
    sigStateChanged.emit(m_State);
}


} //NUX_NAMESPACE_END
