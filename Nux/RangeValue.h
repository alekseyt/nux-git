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


#ifndef RANGEVALUE_H
#define RANGEVALUE_H

namespace nux
{

  class HLayout;
  class EditTextBox;

  class RangeValue : public View //public ValuatorAbstraction
  {
  public:
    RangeValue (float Value = 0, float MinValue = 0.0f, float MaxValue = 1.0f, NUX_FILE_LINE_PROTO);
    virtual ~RangeValue();

    virtual long ProcessEvent (IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
    void DrawMarker (GraphicsEngine &GfxContext);
    virtual void Draw (GraphicsEngine &GfxContext, bool force_draw);
    virtual void DrawContent (GraphicsEngine &GfxContext, bool force_draw);
    virtual void PostDraw (GraphicsEngine &GfxContext, bool force_draw);

    /////////////////
    //  RECEIVERS  //
    /////////////////
    void SetRange (float min_value, float max_value);
    void SetValue (float value);
    float GetValue() const;
    float GetMinValue() const
    {
      return m_min;
    }
    float GetMaxValue() const
    {
      return m_max;
    }

    void SetBackgroundColor (const Color &color);
    const Color GetBackgroundColor() const;

    void setStartToEndColor (Color color_start, Color color_end)
    {
      m_StartColor = color_start;
      m_EndColor = color_end;
    }
    void setStartColor (Color color)
    {
      m_StartColor = color;
    }
    void setEndColor (Color color)
    {
      m_EndColor = color;
    }
    void setProgressColor (Color color)
    {
      m_ProgressColor = color;
    }
    void EnableDrawProgress (bool b)
    {
      m_EnableDrawProgress = b;
    }

    ////////////////
    //  EMITTERS  //
    ////////////////
    void OnReceiveMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags);
    void OnReceiveMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags);
    void OnReceiveMouseDrag (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
    void OnKeyboardFocus();
    void OnLostKeyboardFocus();
    void OnValidateKeyboardEntry (EditTextBox *textbox, const NString &text);

    bool IsCtrlKeyPressed() const
    {
      return (m_CTRL_KEY ? true : false);
    }

    // signals
    sigc::signal<void, RangeValue *> sigValueChanged;
    sigc::signal<void, float> sigFloatChanged;
    sigc::signal<void, float> sigMouseDown;
    sigc::signal<void, float> sigMouseUp;
    sigc::signal<void, float> sigMouseDrag;
    sigc::signal<void, float> sigSetTypedValue;
    //sigc::signal<void, const char*> sigValidateKeyboarEntry;

    void EmitFloatChangedSignal();

  protected:
    void InitializeWidgets();
    void InitializeLayout();
    void DestroyLayout();

  protected:
    HLayout        *hlayout;
    EditTextBox    *m_ValueString;
    CoreArea       *m_Percentage;
    Color m_StartColor;
    Color m_EndColor;
    Color m_ProgressColor;
    Color m_BackgroundColor;
    bool m_EnableDrawProgress;

    long m_CTRL_KEY;

    float m_Value;
    float m_min, m_max;

  };

}

#endif // RANGEVALUE_H


