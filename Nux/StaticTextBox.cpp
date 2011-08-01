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
#include "Layout.h"
#include "HLayout.h"
#include "VLayout.h"
#include "Validator.h"
#include "StaticTextBox.h"

namespace nux
{
  NUX_IMPLEMENT_OBJECT_TYPE (StaticTextBox);

  StaticTextBox::StaticTextBox (const TCHAR *Caption, NUX_FILE_LINE_DECL)
    :   View (NUX_FILE_LINE_PARAM)
    ,   m_TextAlignment (eAlignTextLeft)
    ,   m_bMinimumWidthMatchText (true)
    ,   m_bDrawBackground (false)
    ,   m_WriteAlpha (true)
  {
    m_BackgroundColor = Color (0xFF343434);
    m_TextColor = Color (1.0f, 1.0f, 1.0f, 1.0f);
    // First, set the default minimum size.
    SetMinimumSize (DEFAULT_WIDGET_WIDTH, PRACTICAL_WIDGET_HEIGHT);

    // The width size of the text is computed in SetText and set as the minimum for the widget.
    // If the text is null or empty, then the default minimum widtth set above remains.
    SetText (Caption);

    SetGeometry (Geometry (0, 0, 3 * DEFAULT_WIDGET_WIDTH, DEFAULT_WIDGET_HEIGHT) );

    // This widget does not use a layout!
    m_Background = 0;
  }

  StaticTextBox::~StaticTextBox()
  {
    delete m_Background;
  }

  long StaticTextBox::ProcessEvent (IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
  {
    long ret = TraverseInfo;

    ret = PostProcessEvent2 (ievent, ret, ProcessEventInfo);
    return ret;
  }

  void StaticTextBox::Draw (GraphicsEngine &GfxContext, bool force_draw)
  {
    Geometry base = GetGeometry();
    {
      GfxContext.PushClippingRectangle (base);

      if (m_bDrawBackground)
      {
        GetPainter().PushDrawLayer (GfxContext, base, m_Background);
        GetPainter().PaintTextLineStatic (GfxContext, GetFont (), GetGeometry(), m_Text.GetTCharPtr(), m_TextColor, m_WriteAlpha, m_TextAlignment);
        GetPainter().PopBackground();
      }
      else
      {
        //GetPainter().PaintBackground(GfxContext, base);
        GetPainter().PaintTextLineStatic (GfxContext, GetFont (), GetGeometry(), m_Text.GetTCharPtr(), m_TextColor, m_WriteAlpha, m_TextAlignment);
      }

      GfxContext.PopClippingRectangle();
    }
  }

  void StaticTextBox::DrawContent (GraphicsEngine &GfxContext, bool force_draw)
  {

  }

  void StaticTextBox::PostDraw (GraphicsEngine &GfxContext, bool force_draw)
  {

  }

  void StaticTextBox::SetText (const TCHAR &Caption)
  {
    NString s (Caption);
    SetText (s);
  }

  void StaticTextBox::SetText (const TCHAR *Caption)
  {
    NString s (Caption);
    SetText (s);
  }

  void StaticTextBox::SetText (const tstring &Caption)
  {
    NString s (Caption);
    SetText (s);
  }

  void StaticTextBox::SetText (const NString &Caption)
  {
    m_Text = Caption;

    if (GetMinWidthMatchText() )
      AdjustMinWidthToMatchText();

    QueueDraw();
  }

  void StaticTextBox::SetMinWidthMatchText (bool b)
  {
    m_bMinimumWidthMatchText = b;

    if (m_bMinimumWidthMatchText)
      AdjustMinWidthToMatchText();
  }

  bool StaticTextBox::GetMinWidthMatchText() const
  {
    return m_bMinimumWidthMatchText;
  }

  void StaticTextBox::AdjustMinWidthToMatchText()
  {
    if (m_Text.Size() == 0)
      return;

    SetMinimumWidth (/*4 + */GetFont ()->GetStringWidth (m_Text.GetTStringRef() ) );
  }

  void StaticTextBox::SetFont (ObjectPtr<FontTexture> Font)
  {
    View::SetFont (Font);

    if (GetMinWidthMatchText() )
      AdjustMinWidthToMatchText();
  }

  void StaticTextBox::SetBackground (AbstractPaintLayer *layer)
  {
    delete m_Background;
    m_Background = layer->Clone();
  }

  bool StaticTextBox::AcceptKeyNavFocus()
  {
    return false;
  }
}
