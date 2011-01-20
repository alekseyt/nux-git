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
 * Authored by: Jay Taoko <jaytaoko@inalogic.com>
 *
 */


#include "Nux.h"

#include "EditTextBox.h"
#include "Layout.h"
#include "HLayout.h"
#include "VLayout.h"
#include "Validator.h"

namespace nux
{
  EditTextBox::EditTextBox (const TCHAR *Caption, NUX_FILE_LINE_DECL)
    :   View (NUX_FILE_LINE_PARAM)
  {
    m_Validator             = NULL;
    BlinkCursor             = false;
    m_ScrollTimerHandler    = 0;
    m_BlinkTimerFunctor     = 0;
    m_WriteAlpha            = true;
    m_Prefix                = TEXT("");
    m_Suffix                = TEXT("");

    SetGeometry (Geometry (0, 0, 3 * DEFAULT_WIDGET_WIDTH, DEFAULT_WIDGET_HEIGHT) );
    SetMinimumSize (DEFAULT_WIDGET_WIDTH, PRACTICAL_WIDGET_HEIGHT);
    SetGeometry (Geometry (0, 0, 3 * DEFAULT_WIDGET_WIDTH, DEFAULT_WIDGET_HEIGHT) );
    OnMouseDown.connect (sigc::mem_fun (this, &EditTextBox::RecvMouseDown) );
    OnMouseDrag.connect (sigc::mem_fun (this, &EditTextBox::RecvMouseDrag) );
    OnMouseUp.connect (sigc::mem_fun (this, &EditTextBox::RecvMouseUp) );
    OnMouseDoubleClick.connect (sigc::mem_fun (this, &EditTextBox::RecvMouseDoubleClick) );

    OnKeyEvent.connect (sigc::mem_fun (this, &EditTextBox::RecvKeyEvent) );

    OnStartFocus.connect (sigc::mem_fun (this, &EditTextBox::RecvStartKeyFocus) );
    OnEndFocus.connect (sigc::mem_fun (this, &EditTextBox::RecvEndKeyFocus) );

    SetText (Caption);
    SetTextColor (Color::White);
    m_BackgroundColor = Color (0xFF343434); //COLOR_TEXTEDIT_BACKGROUNG;
    m_SelectedTextColor = Color (0xFFFAFAFA);
    m_SelectedTextBackgroundColor = Color (0xFF777777);
    m_TextBlinkColor = Color (0xFF003D0A);
    m_CursorColor = Color (0xFFDDDDDD);


    hlayout = new HLayout (NUX_TRACKER_LOCATION);
    SetCompositionLayout (hlayout);

    m_BlinkTimerFunctor = new TimerFunctor();
    m_BlinkTimerFunctor->OnTimerExpired.connect (sigc::mem_fun (this, &EditTextBox::BlinkCursorTimerInterrupt) );

    m_ScrollTimerFunctor = new TimerFunctor();
    m_ScrollTimerFunctor->OnTimerExpired.connect (sigc::mem_fun (this, &EditTextBox::ScrollTimerInterrupt) );
  }

  EditTextBox::~EditTextBox()
  {
    NUX_SAFE_DELETE (m_Validator);

    if (m_BlinkTimerHandler.IsValid() )
      GetTimer().RemoveTimerHandler (m_BlinkTimerHandler);

    m_BlinkTimerHandler = 0;
  }

  void EditTextBox::ScrollTimerInterrupt (void *v)
  {
    Geometry base = GetGeometry();
    IEvent &ievent = GetThreadGLWindow()->GetCurrentEvent();

    int X = ievent.e_x;
    m_KeyboardHandler.CaretAutoScroll (ievent.e_x, ievent.e_y, base);

    if ( ( (X < base.x) && (m_KeyboardHandler.GetCursorPosition() > 0) ) ||
         ( (X > base.x + base.GetWidth() ) && (m_KeyboardHandler.GetCursorPosition() < m_KeyboardHandler.GetLength() ) ) )
    {
      m_ScrollTimerHandler = GetTimer().AddTimerHandler (50, m_ScrollTimerFunctor, this);
    }
    else
    {
      GetTimer().RemoveTimerHandler (m_BlinkTimerHandler);
      m_ScrollTimerHandler = 0;
    }

    // While the mouse is selecting the text, no blinking of cursor
    StopBlinkCursor (false);
    StartBlinkCursor (false);

    NeedRedraw();
  }

  void EditTextBox::BlinkCursorTimerInterrupt (void *v)
  {
    GetTimer().RemoveTimerHandler (m_BlinkTimerHandler);
    m_BlinkTimerHandler = GetTimer().AddTimerHandler (500, m_BlinkTimerFunctor, this);
    BlinkCursor = !BlinkCursor;
    NeedRedraw();
  }

  void EditTextBox::StopBlinkCursor (bool BlinkState)
  {
    GetTimer().RemoveTimerHandler (m_BlinkTimerHandler);
    m_BlinkTimerHandler = 0;
    BlinkCursor = BlinkState;
    NeedRedraw();
  }

  void EditTextBox::StartBlinkCursor (bool BlinkState)
  {
    m_BlinkTimerHandler = GetTimer().AddTimerHandler (500, m_BlinkTimerFunctor, this);
    BlinkCursor = BlinkState;
    NeedRedraw();
  }

  void EditTextBox::SetValidator (const Validator *validator)
  {
    nuxAssert (validator != 0);
    NUX_SAFE_DELETE (m_Validator);
    m_Validator = validator->Clone();
  }

  long EditTextBox::ProcessEvent (IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
  {
    long ret = TraverseInfo;
    ret = PostProcessEvent2 (ievent, ret, ProcessEventInfo);
    return ret;
  }

  void EditTextBox::Draw (GraphicsEngine &GfxContext, bool force_draw)
  {
    Geometry base = GetGeometry();

    {
      GfxContext.PushClippingRectangle (base);

      GetPainter().Paint2DQuadColor (GfxContext, base, Color (m_BackgroundColor) );

      if (HasKeyboardFocus() )
      {

        GetPainter().PaintColorTextLineEdit (GfxContext, GetGeometry(),
                                         m_KeyboardHandler.GetTextLine(),
                                         GetTextColor(),
                                         m_WriteAlpha,
                                         m_SelectedTextColor,
                                         m_SelectedTextBackgroundColor,
                                         m_TextBlinkColor,
                                         m_CursorColor,
                                         !BlinkCursor,
                                         m_KeyboardHandler.GetCursorPosition(),
                                         m_KeyboardHandler.GetPositionX(),
                                         m_KeyboardHandler.GetTextSelectionStart(),
                                         m_KeyboardHandler.GetTextSelectionEnd()
                                        );
      }
      else
      {
        GetPainter().PaintTextLineStatic (GfxContext, GetFont (), GetGeometry(),
                                      m_KeyboardHandler.GetTextLine(),
                                      GetTextColor() );
      }
    }
    GfxContext.PopClippingRectangle();
  }

  void EditTextBox::DrawContent (GraphicsEngine &GfxContext, bool force_draw)
  {

  }

  void EditTextBox::PostDraw (GraphicsEngine &GfxContext, bool force_draw)
  {

  }

  void EditTextBox::SetText (const TCHAR &Caption)
  {
    NString s (Caption);
    SetText (s);
  }

  void EditTextBox::SetText (const TCHAR *Caption)
  {
    NString s (Caption);
    SetText (s);
  }

  void EditTextBox::SetText (const tstring &Caption)
  {
    NString s (Caption);
    SetText (s);
  }

  void EditTextBox::SetText (const NString &Caption)
  {
    NString s (Caption);
    s.RemovePrefix (m_Prefix);
    s.RemoveSuffix (m_Suffix);

    if (ValidateKeyboardEntry (s.GetTCharPtr ()))
    {
      m_Text = (m_Prefix + s) + m_Suffix;
      m_KeyboardHandler.SetText (m_Text.GetTStringRef ());
      m_temporary_caption = m_Text;
      sigSetText.emit (this);
    }

    NeedRedraw();
  }


  const TCHAR *EditTextBox::GetText() const
  {
    return m_Text.GetTCharPtr();
  }

//! Return a caption string striping out the prefix and the suffix
  NString EditTextBox::GetCleanText() const
  {
    NString CleanText (m_Text);
    CleanText.RemovePrefix (m_Prefix);
    CleanText.RemoveSuffix (m_Suffix);
    return CleanText.m_string;
  }

  void EditTextBox::RecvMouseDoubleClick (int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    m_KeyboardHandler.SelectAllText();
    NeedRedraw();
  }

  void EditTextBox::RecvMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    m_KeyboardHandler.MouseUp (x, y);

    if (m_ScrollTimerHandler.IsValid() )
    {
      GetTimer().RemoveTimerHandler (m_ScrollTimerHandler);
      m_ScrollTimerHandler = 0;
    }

    NeedRedraw();
  }

  void EditTextBox::RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    if (HasKeyboardFocus() == false)
    {
      // First mouse down
      m_KeyboardHandler.EnterFocus();
    }
    else
    {
      // Second mouse down and more
      m_KeyboardHandler.UnselectAllText();
      m_KeyboardHandler.MouseDown (x, y);

      // Always make the cursor visible when a mouse down happens.
      StopBlinkCursor (false);
      StartBlinkCursor (false);
    }

    NeedRedraw();
  }

  void EditTextBox::RecvMouseDrag (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
  {
    Geometry base = GetGeometry();

    int X = x + base.x;

    if ( (!m_ScrollTimerHandler.IsValid() ) && ( (X < base.x) || (X > base.x + base.GetWidth() ) ) )
    {
      m_ScrollTimerHandler = GetTimer().AddTimerHandler (25, m_ScrollTimerFunctor, this);
    }
    else if ( (X >= base.x) && (X < base.x + base.GetWidth() ) )
    {
      m_KeyboardHandler.MouseDrag (x, y);

      // While the mouse is selecting the text, no blinking of cursor
      StopBlinkCursor (false);
      StartBlinkCursor (false);
    }

    NeedRedraw();
  }

  long EditTextBox::PostLayoutManagement (long LayoutResult)
  {
    long ret = View::PostLayoutManagement (LayoutResult);

    m_KeyboardHandler.SetClipRegion (GetGeometry() );
    return ret;
  }


  void EditTextBox::RecvKeyEvent (
    GraphicsEngine  &GfxContext, /*Graphics Context for text operation*/
    unsigned long   eventType  , /*event type*/
    unsigned long   keysym     , /*event keysym*/
    unsigned long   state      , /*event state*/
    TCHAR           character  , /*character*/
    unsigned short  keyCount     /*key repeat count*/)
  {
    m_KeyboardHandler.ProcessKey (eventType, keysym, state, character, GetGeometry() );


    // When a key event happens, show the cursor.
    StopBlinkCursor (false);
    // Start a new blink cycle with the cursor originally visible.
    StartBlinkCursor (false);

    if (character)
    {
      sigCharacter.emit (this, character);
      sigEditChange.emit (this);
    }

    if (keysym == NUX_VK_ENTER || keysym == NUX_KP_ENTER)
    {
      NString str (m_KeyboardHandler.GetTextLine() );
      str.RemoveSuffix (m_Suffix);

      if (ValidateKeyboardEntry (str.m_string.c_str() ) )
      {
        m_Text = m_KeyboardHandler.GetTextLine();
        m_temporary_caption = m_Text;
        sigValidateKeyboardEntry.emit (this, m_Text);
        sigValidateEntry.emit (this);
        m_KeyboardHandler.SelectAllText();
      }
      else
      {
        m_Text = m_temporary_caption;
        m_KeyboardHandler.SetText (m_Text);
        m_KeyboardHandler.SelectAllText();
      }
    }

    NeedRedraw();
  }

  bool EditTextBox::ValidateKeyboardEntry (const TCHAR *text) const
  {
    if (m_Validator)
    {
      if (m_Validator->Validate (text) == Validator::Acceptable)
      {
        return true;
      }
      else
      {
        return false;
      }
    }

    return true;
  }

  void EditTextBox::EscapeKeyboardFocus()
  {
    SetKeyboardFocus (false);
    // Revert back the caption text
    m_Text = m_temporary_caption;
    sigEscapeKeyboardFocus.emit (this);
    NeedRedraw();
  }

  void EditTextBox::EnteringKeyboardFocus()
  {
    m_KeyboardHandler.SetText (m_Text.GetTStringRef() );
    m_KeyboardHandler.SelectAllText();
    // Preserve the current caption text. If ESC is pressed while we have keyboard focus then
    // the previous caption text is restored
    m_temporary_caption = m_Text;
    sigStartKeyboardFocus.emit (this);
    NeedRedraw();
  }

  void EditTextBox::QuitingKeyboardFocus()
  {
    NString CleanText (m_KeyboardHandler.GetTextLine() );
    CleanText.RemovePrefix (m_Prefix);
    CleanText.RemoveSuffix (m_Suffix);

    if (ValidateKeyboardEntry (CleanText.GetTCharPtr() ) )
    {
      CleanText = m_Prefix + CleanText;
      CleanText = CleanText + m_Suffix;

      m_Text = CleanText.m_string; //m_KeyboardHandler.GetTextLine();
      m_KeyboardHandler.SetText (CleanText.m_string);
      m_temporary_caption = m_Text;
      sigValidateKeyboardEntry.emit (this, m_Text.GetTStringRef() );
      sigValidateEntry.emit (this);
    }
    else
    {
      m_Text = m_temporary_caption;
      m_KeyboardHandler.SetText (m_Text.GetTStringRef() );
      m_KeyboardHandler.SelectAllText();
    }

    NeedRedraw();
  }

  void EditTextBox::RecvStartKeyFocus()
  {
    EnteringKeyboardFocus();
    m_BlinkTimerHandler = GetTimer().AddTimerHandler (500, m_BlinkTimerFunctor, this);
  }

  void EditTextBox::RecvEndKeyFocus()
  {
    QuitingKeyboardFocus();
    GetTimer().RemoveTimerHandler (m_BlinkTimerHandler);
    m_BlinkTimerHandler = 0;
    BlinkCursor = false;
  }

  void EditTextBox::SetDoubleValue (double d)
  {
    SetText (NString::Printf ("%f", d) );
  }

  void EditTextBox::SetIntegerValue (int i)
  {
    SetText (NString::Printf ("%d", i) );
  }

  void EditTextBox::SetTextBackgroundColor (const Color &color)
  {
    m_BackgroundColor = color;
    NeedRedraw();
  }

  Color EditTextBox::GetTextBackgroundColor() const
  {
    return m_BackgroundColor;
  }

  bool EditTextBox::IsEmpty()
  {
    if (m_Text == TEXT ("") )
    {
      return true;
    }

    return false;
  }

}
