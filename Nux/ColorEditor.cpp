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

#include "NuxGraphics/GLSh_ColorPicker.h"
#include "VLayout.h"
#include "HLayout.h"
#include "CheckBox.h"
#include "EditTextBox.h"
#include "RadioButton.h"
#include "RadioButtonGroup.h"
#include "PushButton.h"
#include "Layout.h"
#include "ColorEditor.h"

namespace nux
{

  static void ThreadColorEditorDialog (NThread *thread, void *InitData)
  {
    VLayout *MainLayout (new VLayout (NUX_TRACKER_LOCATION) );
    ColorEditor *coloreditor (new ColorEditor() );
    coloreditor->ComputeChildLayout(); // necessary so all element of the widget get their rightful size.
    ColorDialogProxy *coloreditorproxy = static_cast<ColorDialogProxy *> (InitData);

    if (coloreditorproxy)
    {
      coloreditor->SetRGB (coloreditorproxy->GetColor() );
      coloreditor->SetColorModel (coloreditorproxy->GetColorModel(), coloreditorproxy->GetColorChannel() );
      coloreditor->sigChange.connect (sigc::mem_fun (coloreditorproxy, &ColorDialogProxy::RecvDialogChange) );
    }

    HLayout *ButtonLayout (new HLayout (TEXT ("Dialog Buttons"), NUX_TRACKER_LOCATION) );

    PushButton *OkButton (new PushButton (TEXT ("OK"), NUX_TRACKER_LOCATION) );
    OkButton->SetMinimumWidth (60);
    OkButton->SetMinimumHeight (20);

    PushButton *CancelButton (new PushButton (TEXT ("Cancel"), NUX_TRACKER_LOCATION) );
    CancelButton->SetMinimumWidth (60);
    CancelButton->SetMinimumHeight (20);

    OkButton->sigClick.connect (sigc::mem_fun (static_cast<WindowThread *> (thread), &WindowThread::TerminateThread) );
    OkButton->sigClick.connect (sigc::bind (sigc::mem_fun (coloreditorproxy, &ColorDialogProxy::RecvDialogOk), coloreditor) );
    CancelButton->sigClick.connect (sigc::bind (sigc::mem_fun (coloreditorproxy, &ColorDialogProxy::RecvDialogCancel), coloreditor) );
    CancelButton->sigClick.connect (sigc::mem_fun (static_cast<WindowThread *> (thread), &WindowThread::TerminateThread) );

    ButtonLayout->SetHorizontalInternalMargin (6);
    ButtonLayout->SetVerticalExternalMargin (2);
    ButtonLayout->AddView (OkButton, 0);
    ButtonLayout->AddView (CancelButton, 0);

    MainLayout->AddView (coloreditor);
    MainLayout->AddLayout (ButtonLayout, 0);
    static_cast<WindowThread *> (thread)->SetLayout (MainLayout);

    MainLayout->SetBaseWidth (1);
    MainLayout->SetBaseHeight (1);
    MainLayout->ComputeLayout2();
    static_cast<WindowThread *> (thread)->SetWindowSize (MainLayout->GetBaseWidth(), MainLayout->GetBaseHeight() );

    // Call StopThreadMonitoring in case the dialog was close by clicking the window close button.
    //coloreditorproxy->StopThreadMonitoring();
  }

  ColorDialogProxy::ColorDialogProxy (bool ModalWindow)
  {
    m_bDialogChange     = false;
    m_bDialogRunning    = false;
    m_ModalWindow       = ModalWindow;
    m_RGBColor          = Color (1.0f, 1.0f, 1.0f, 1.0f);
    m_ColorModel        = color::RGB;
    m_ColorChannel      = color::RED;
  }

  ColorDialogProxy::~ColorDialogProxy()
  {
  }

  void ColorDialogProxy::Start()
  {
    m_PreviousRGBColor = m_RGBColor;

    int Width = 290;
    int Height = 230;
    m_Thread = CreateModalWindowThread (WINDOWSTYLE_TOOL, TEXT ("Color Editor"), Width, Height, GetWindowThread (),
                                        ThreadColorEditorDialog,
                                        this);

    if (m_Thread)
    {
      m_DialogThreadID = m_Thread->GetThreadId();
      m_Thread->Start (0);
    }

    m_bDialogRunning = true;
  }

  bool ColorDialogProxy::IsActive()
  {
    return (m_Thread && (m_Thread->GetThreadState() != THREADSTOP) && m_bDialogRunning);
  }

  void ColorDialogProxy::RecvDialogOk (ColorEditor *coloreditor)
  {
    m_RGBColor = coloreditor->GetRGBColor();
    m_PreviousRGBColor = m_RGBColor;
    m_bDialogChange = true;
    m_bDialogRunning = false;
  }

  void ColorDialogProxy::RecvDialogCancel (ColorEditor *coloreditor)
  {
    m_RGBColor = m_PreviousRGBColor;
    m_bDialogChange = true;
    m_bDialogRunning = false;
  }

  void ColorDialogProxy::RecvDialogChange (ColorEditor *coloreditor)
  {
    m_RGBColor = coloreditor->GetRGBColor();
    m_bDialogChange = true;
  }

  void ColorDialogProxy::StopThreadMonitoring()
  {
    m_RGBColor = m_PreviousRGBColor;
    m_bDialogChange = true;
    m_bDialogRunning = false;
    m_Thread = 0;
    m_DialogThreadID = 0;
  }

  void ColorDialogProxy::SetColor (Color color)
  {
    m_RGBColor = color;
  }

  Color ColorDialogProxy::GetColor()
  {
    return m_RGBColor;
  }

  void ColorDialogProxy::SetPreviousColor (Color color)
  {
    m_PreviousRGBColor = color;
  }

  Color ColorDialogProxy::GetPreviousColor()
  {
    return m_PreviousRGBColor;
  }

  void ColorDialogProxy::SetColorModel (color::Model color_model)
  {
    m_ColorModel = color_model;
  }

  color::Model ColorDialogProxy::GetColorModel()
  {
    return m_ColorModel;
  }

  void ColorDialogProxy::SetColorChannel (color::Channel color_channel)
  {
    m_ColorChannel = color_channel;
  }

  color::Channel ColorDialogProxy::GetColorChannel()
  {
    return m_ColorChannel;
  }

  ColorEditor::ColorEditor (NUX_FILE_LINE_DECL)
    : View (NUX_FILE_LINE_PARAM)
    , rgb_(1.0f, 1.0f, 0.0f)
    , hsv_(rgb_)
  {
    m_ColorModel = color::RGB;
    m_ColorChannel = color::RED;
    m_MarkerPosition = Point (0, 0);
    m_VertMarkerPosition = Point (0, 0);

    m_Validator.SetMinimum (0.0);
    m_Validator.SetMaximum (1.0);
    m_Validator.SetDecimals (2);

    m_PickerArea        = new InputArea (NUX_TRACKER_LOCATION);
    m_BaseChannelArea   = new InputArea (NUX_TRACKER_LOCATION);
    m_ColorSquare       = new InputArea (NUX_TRACKER_LOCATION);
    m_hlayout           = new HLayout (NUX_TRACKER_LOCATION);

    m_BaseChannelArea->OnMouseDown.connect (sigc::mem_fun (this, &ColorEditor::RecvMouseDown) );
    m_BaseChannelArea->OnMouseUp.connect (sigc::mem_fun (this, &ColorEditor::RecvMouseUp) );
    m_BaseChannelArea->OnMouseDrag.connect (sigc::mem_fun (this, &ColorEditor::RecvMouseDrag) );

    m_PickerArea->OnMouseDown.connect (sigc::mem_fun (this, &ColorEditor::RecvPickerMouseDown) );
    m_PickerArea->OnMouseUp.connect (sigc::mem_fun (this, &ColorEditor::RecvPickerMouseUp) );
    m_PickerArea->OnMouseDrag.connect (sigc::mem_fun (this, &ColorEditor::RecvPickerMouseDrag) );

    m_ColorSquare->SetMinMaxSize (62, 32);
    m_PickerArea->SetMinimumSize (200, 200);
    m_PickerArea->SetMaximumSize (200, 200);
    m_BaseChannelArea->SetMaximumHeight (200);
    m_BaseChannelArea->SetMinimumWidth (20);
    m_BaseChannelArea->SetMaximumWidth (20);

    m_hlayout->AddView (m_PickerArea, 1);
    m_hlayout->AddLayout (new SpaceLayout (5, 5, 20, 20), 0);
    m_hlayout->AddView (m_BaseChannelArea, 0, eAbove, eFull);
    SetCompositionLayout (m_hlayout);

    // RGB
    {
      redlayout = new HLayout (NUX_TRACKER_LOCATION);
      {
        redcheck = new RadioButton (TEXT ("R:") );
        redcheck->SetMinimumWidth (30);
        redtext = new EditTextBox (TEXT (""), NUX_TRACKER_LOCATION);
        redtext->SetMinimumWidth (36);
        redlayout->AddView (redcheck, 0);
        redlayout->AddView (redtext, 0);
        redcheck->sigStateChanged.connect (sigc::bind ( sigc::bind ( sigc::mem_fun (this, &ColorEditor::RecvCheckColorModel), color::RED), color::RGB ) );
      }
      greenlayout = new HLayout (NUX_TRACKER_LOCATION);
      {
        greencheck = new RadioButton (TEXT ("G:") );
        greencheck->SetMinimumWidth (30);
        greentext = new EditTextBox (TEXT (""), NUX_TRACKER_LOCATION);
        greentext->SetMinimumWidth (36);
        greenlayout->AddView (greencheck, 0);
        greenlayout->AddView (greentext, 0);
        greencheck->sigStateChanged.connect (sigc::bind ( sigc::bind ( sigc::mem_fun (this, &ColorEditor::RecvCheckColorModel), color::GREEN), color::RGB ) );

      }
      bluelayout = new HLayout (NUX_TRACKER_LOCATION);
      {
        bluecheck = new RadioButton (TEXT ("B:") );
        bluecheck->SetMinimumWidth (30);
        bluetext = new EditTextBox (TEXT (""), NUX_TRACKER_LOCATION);
        bluetext->SetMinimumWidth (36);
        bluelayout->AddView (bluecheck, 0);
        bluelayout->AddView (bluetext, 0);
        bluecheck->sigStateChanged.connect (sigc::bind ( sigc::bind ( sigc::mem_fun (this, &ColorEditor::RecvCheckColorModel), color::BLUE), color::RGB ) );

      }
    }

    // HSV
    {
      huelayout = new HLayout (NUX_TRACKER_LOCATION);
      {
        huecheck = new RadioButton (TEXT ("H:") );
        huecheck->SetMinimumWidth (30);
        huetext = new EditTextBox (TEXT (""), NUX_TRACKER_LOCATION);
        huetext->SetMinimumWidth (36);
        huelayout->AddView (huecheck, 0);
        huelayout->AddView (huetext, 0);
        huecheck->sigStateChanged.connect (sigc::bind ( sigc::bind ( sigc::mem_fun (this, &ColorEditor::RecvCheckColorModel), color::HUE), color::HSV ) );
      }
      saturationlayout = new HLayout (NUX_TRACKER_LOCATION);
      {
        saturationcheck = new RadioButton (TEXT ("S:") );
        saturationcheck->SetMinimumWidth (30);
        saturationtext = new EditTextBox (TEXT (""), NUX_TRACKER_LOCATION);
        saturationtext->SetMinimumWidth (36);
        saturationlayout->AddView (saturationcheck, 0);
        saturationlayout->AddView (saturationtext, 0);
        saturationcheck->sigStateChanged.connect (sigc::bind ( sigc::bind ( sigc::mem_fun (this, &ColorEditor::RecvCheckColorModel), color::SATURATION), color::HSV ) );
      }
      valuelayout = new HLayout (NUX_TRACKER_LOCATION);
      {
        valuecheck = new RadioButton (TEXT ("V:") );
        valuecheck->SetMinimumWidth (30);
        valuetext = new EditTextBox (TEXT (""), NUX_TRACKER_LOCATION);
        valuetext->SetMinimumWidth (36);
        valuelayout->AddView (valuecheck, 0);
        valuelayout->AddView (valuetext, 0);
        valuecheck->sigStateChanged.connect (sigc::bind ( sigc::bind ( sigc::mem_fun (this, &ColorEditor::RecvCheckColorModel), color::VALUE), color::HSV ) );
      }
    }

    ctrllayout = new VLayout (NUX_TRACKER_LOCATION);
    ctrllayout->AddView (m_ColorSquare);
    ctrllayout->AddView (new SpaceLayout (20, 20, 10, 10), 1);
    ctrllayout->AddLayout (redlayout, 0);
    ctrllayout->AddLayout (greenlayout, 0);
    ctrllayout->AddLayout (bluelayout, 0);
    ctrllayout->AddLayout (new SpaceLayout (10, 10, 10, 10) );
    ctrllayout->AddLayout (huelayout, 0);
    ctrllayout->AddLayout (saturationlayout, 0);
    ctrllayout->AddLayout (valuelayout, 0);
    ctrllayout->SetHorizontalExternalMargin (2);
    ctrllayout->SetVerticalInternalMargin (2);

//     //ctrllayout->AddView(new SpaceLayout(20,20,20,40), 1);
//     OkButton = new PushButton (TEXT ("OK"), NUX_TRACKER_LOCATION);
//     OkButton->SetMinimumWidth (60);
//     OkButton->SetMinimumHeight (20);
// 
//     CancelButton = new PushButton (TEXT ("Cancel"), NUX_TRACKER_LOCATION);
//     CancelButton->SetMinimumWidth (60);
//     CancelButton->SetMinimumHeight (20);
// 
// //    ctrllayout->AddView(OkButton, 1);
// //    ctrllayout->AddView(CancelButton, 1);

    m_hlayout->AddLayout (ctrllayout, 0);

    radiogroup = new RadioButtonGroup (NUX_TRACKER_LOCATION);
    radiogroup->ConnectButton (redcheck);
    radiogroup->ConnectButton (greencheck);
    radiogroup->ConnectButton (bluecheck);
    radiogroup->ConnectButton (huecheck);
    radiogroup->ConnectButton (saturationcheck);
    radiogroup->ConnectButton (valuecheck);

    m_RedShader = new GLSh_ColorPicker (color::RED);
    m_GreenShader = new GLSh_ColorPicker (color::GREEN);
    m_BlueShader = new GLSh_ColorPicker (color::BLUE);
    m_HueShader = new GLSh_ColorPicker (color::HUE);
    m_SaturationShader = new GLSh_ColorPicker (color::SATURATION);
    m_ValueShader = new GLSh_ColorPicker (color::VALUE);

    redtext->SetText (m_Validator.ToString (255 * rgb_.red) );
    greentext->SetText (m_Validator.ToString (255 * rgb_.green) );
    bluetext->SetText (m_Validator.ToString (255 * rgb_.blue) );
    huetext->SetText (m_Validator.ToString (360 * hsv_.hue) );
    saturationtext->SetText (m_Validator.ToString (100 * hsv_.saturation) );
    valuetext->SetText (m_Validator.ToString (100 * hsv_.value) );
  }

  ColorEditor::~ColorEditor()
  {
    NUX_SAFE_DELETE (m_RedShader);
    NUX_SAFE_DELETE (m_GreenShader);
    NUX_SAFE_DELETE (m_BlueShader);
    NUX_SAFE_DELETE (m_HueShader);
    NUX_SAFE_DELETE (m_SaturationShader);
    NUX_SAFE_DELETE (m_ValueShader);
  }


  long ColorEditor::ProcessEvent (IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
  {
    long ret;

    ret = m_PickerArea->OnEvent (ievent, TraverseInfo, ProcessEventInfo);
    ret = m_BaseChannelArea->OnEvent (ievent, ret, ProcessEventInfo);

    // RGB
    {
      ret = redcheck->OnEvent (ievent, ret, ProcessEventInfo);
      ret = redtext->OnEvent (ievent, ret, ProcessEventInfo);

      ret = greencheck->OnEvent (ievent, ret, ProcessEventInfo);
      ret = greentext->OnEvent (ievent, ret, ProcessEventInfo);

      ret = bluecheck->OnEvent (ievent, ret, ProcessEventInfo);
      ret = bluetext->OnEvent (ievent, ret, ProcessEventInfo);
    }

    // HSV
    {
      ret = huecheck->OnEvent (ievent, ret, ProcessEventInfo);
      ret = huetext->OnEvent (ievent, ret, ProcessEventInfo);

      ret = saturationcheck->OnEvent (ievent, ret, ProcessEventInfo);
      ret = saturationtext->OnEvent (ievent, ret, ProcessEventInfo);

      ret = valuecheck->OnEvent (ievent, ret, ProcessEventInfo);
      ret = valuetext->OnEvent (ievent, ret, ProcessEventInfo);
    }

//     OkButton->OnEvent (ievent, ret, ProcessEventInfo);
//     CancelButton->OnEvent (ievent, ret, ProcessEventInfo);

    ret = PostProcessEvent2 (ievent, ret, ProcessEventInfo);

    return ret;
  }

  void ColorEditor::Draw (GraphicsEngine &GfxContext, bool force_draw)
  {
    Geometry base = GetGeometry();

    GetPainter().PaintBackground (GfxContext, base);
    //GetPainter().Paint2DQuadWireframe(GfxContext, base, Color(COLOR_BACKGROUND_SECONDARY));

    base.OffsetPosition (1, 1);
    base.OffsetSize (-2, -2);

    GfxContext.PushClippingRectangle (base);

    if (m_ColorModel == color::RGB)
    {
      DrawRGB (GfxContext, force_draw);
    }
    else
    {
      DrawHSV (GfxContext, force_draw);
    }

    redcheck->NeedRedraw();
    redtext->NeedRedraw();
    greencheck->NeedRedraw();
    greentext->NeedRedraw();
    bluecheck->NeedRedraw();
    bluetext->NeedRedraw();

    huecheck->NeedRedraw();
    huetext->NeedRedraw();
    saturationcheck->NeedRedraw();
    saturationtext->NeedRedraw();
    valuecheck->NeedRedraw();
    valuetext->NeedRedraw();

//     OkButton->NeedRedraw();
//     CancelButton->NeedRedraw();

    GfxContext.PopClippingRectangle();
  }

// Draw Marker on Base Chanel Area
  void ColorEditor::DrawBaseChannelMarker (GraphicsEngine &GfxContext)
  {
    int marker_position_x;
    int marker_position_y;

    GfxContext.PushClippingRectangle (m_BaseChannelArea->GetGeometry() );

    marker_position_x = m_BaseChannelArea->GetBaseX();
    marker_position_y = m_BaseChannelArea->GetBaseY() + m_VertMarkerPosition.y;
    GetPainter().Draw2DTriangleColor (GfxContext, marker_position_x, marker_position_y - 5,
                                  marker_position_x + 5, marker_position_y,
                                  marker_position_x, marker_position_y + 5, Color (0.0f, 0.0f, 0.0f, 1.0f) );

    GetPainter().Draw2DTriangleColor (GfxContext, marker_position_x, marker_position_y - 4,
                                  marker_position_x + 4, marker_position_y,
                                  marker_position_x, marker_position_y + 4, Color (0.7f, 0.7f, 0.7f, 1.0f) );

    marker_position_x = m_BaseChannelArea->GetBaseX() + m_BaseChannelArea->GetBaseWidth();
    marker_position_y = m_BaseChannelArea->GetBaseY() + m_VertMarkerPosition.y;
    GetPainter().Draw2DTriangleColor (GfxContext, marker_position_x, marker_position_y - 5,
                                  marker_position_x - 5, marker_position_y,
                                  marker_position_x, marker_position_y + 5, Color (0.0f, 0.0f, 0.0f, 1.0f) );

    GetPainter().Draw2DTriangleColor (GfxContext, marker_position_x, marker_position_y - 4,
                                  marker_position_x - 4, marker_position_y,
                                  marker_position_x, marker_position_y + 4, Color (0.7f, 0.7f, 0.7f, 1.0f) );
    GfxContext.PopClippingRectangle();
  }

  void ColorEditor::DrawRGB (GraphicsEngine &GfxContext, bool force_draw)
  {
    int x, y;

    if (m_ColorModel == color::RGB)
    {
      GetPainter().Paint2DQuadColor (GfxContext, m_ColorSquare->GetGeometry(), Color(rgb_) );
      Color BaseChannelTop;
      Color BaseChannelBottom;

      if (m_ColorChannel == color::RED)
      {
        x = rgb_.blue * m_PickerArea->GetBaseWidth();
        y = (1.0f - rgb_.green) * m_PickerArea->GetBaseHeight();

        m_RedShader->SetColor (rgb_.red, rgb_.green, rgb_.blue, 1.0f);
        m_RedShader->SetScreenPositionOffset (GfxContext.GetViewportX (), GfxContext.GetViewportY ());
        BaseChannelTop = Color (1.0f, rgb_.green, rgb_.blue, 1.0f);
        BaseChannelBottom = Color (0.0f, rgb_.green, rgb_.blue, 1.0f);
        m_RedShader->Render (
          m_PickerArea->GetBaseX(),
          m_PickerArea->GetBaseY(),
          0,
          m_PickerArea->GetBaseWidth(),
          m_PickerArea->GetBaseHeight(),
          GfxContext.GetViewportWidth (), GfxContext.GetViewportHeight ()
        );
      }
      else if (m_ColorChannel == color::GREEN)
      {
        x = rgb_.blue * m_PickerArea->GetBaseWidth();
        y = (1.0f - rgb_.red) * m_PickerArea->GetBaseHeight();

        m_GreenShader->SetColor (rgb_.red, rgb_.green, rgb_.blue, 1.0f);
        m_GreenShader->SetScreenPositionOffset (GfxContext.GetViewportX (), GfxContext.GetViewportY ());
        BaseChannelTop = Color (rgb_.red, 1.0f, rgb_.blue, 1.0f);
        BaseChannelBottom = Color (rgb_.red, 0.0f, rgb_.blue, 1.0f);
        m_GreenShader->Render (
          m_PickerArea->GetBaseX(),
          m_PickerArea->GetBaseY(),
          0,
          m_PickerArea->GetBaseWidth(),
          m_PickerArea->GetBaseHeight(),
          GfxContext.GetViewportWidth (), GfxContext.GetViewportHeight ()
        );
      }
      else if (m_ColorChannel == color::BLUE)
      {
        x = rgb_.red * m_PickerArea->GetBaseWidth();
        y = (1.0f - rgb_.green) * m_PickerArea->GetBaseHeight();

        m_BlueShader->SetColor (rgb_.red, rgb_.green, rgb_.blue, 1.0f);
        m_BlueShader->SetScreenPositionOffset (GfxContext.GetViewportX (), GfxContext.GetViewportY ());
        BaseChannelTop = Color (rgb_.red, rgb_.green, 1.0f, 1.0f);
        BaseChannelBottom = Color (rgb_.red, rgb_.green, 0.0f, 1.0f);
        m_BlueShader->Render (
          m_PickerArea->GetBaseX(),
          m_PickerArea->GetBaseY(),
          0,
          m_PickerArea->GetBaseWidth(),
          m_PickerArea->GetBaseHeight(),
          GfxContext.GetViewportWidth (), GfxContext.GetViewportHeight ()
        );
      }

      Geometry pickermarker = Geometry (GetBaseX() + m_MarkerPosition.x - 2, GetBaseY() + m_MarkerPosition.y - 2, 5, 5);
      Geometry basepickermarker = Geometry (m_BaseChannelArea->GetBaseX(), m_BaseChannelArea->GetBaseY() + m_VertMarkerPosition.y, 5, 5);

      Color color (rgb_.red, rgb_.green, rgb_.blue);
      GetPainter().Paint2DQuadWireframe (GfxContext, pickermarker, OneMinusLuminance(rgb_) );

      GetPainter().Paint2DQuadColor (GfxContext, m_BaseChannelArea->GetGeometry(), BaseChannelTop, BaseChannelBottom, BaseChannelBottom, BaseChannelTop);
      // Draw Marker on Base Chanel Area
      DrawBaseChannelMarker (GfxContext);
    }
  }

  void ColorEditor::DrawHSV (GraphicsEngine &GfxContext, bool force_draw)
  {
    int x, y;

    if (m_ColorModel == color::HSV)
    {
      color::RedGreenBlue rgb(hsv_);
      GetPainter().Paint2DQuadColor(GfxContext, m_ColorSquare->GetGeometry(), Color(rgb) );

      Color BaseChannelTop;
      Color BaseChannelBottom;

      if (m_ColorChannel == color::HUE)
      {
        x = hsv_.saturation * m_PickerArea->GetBaseWidth();
        y = (1.0f - hsv_.value) * m_PickerArea->GetBaseHeight();

        m_HueShader->SetColor (hsv_.hue, hsv_.saturation, hsv_.value, 1.0f);
        m_HueShader->SetScreenPositionOffset (GfxContext.GetViewportX (), GfxContext.GetViewportY ());
        m_HueShader->Render (
          m_PickerArea->GetBaseX(),
          m_PickerArea->GetBaseY(),
          0,
          m_PickerArea->GetBaseWidth(),
          m_PickerArea->GetBaseHeight(),
          GfxContext.GetViewportWidth (), GfxContext.GetViewportHeight ()
        );

        Geometry P = m_BaseChannelArea->GetGeometry();

        float s = 1.0f - 1.0f;
        float v = 1.0f;
        float fw = P.GetHeight() / 6;

        Geometry p = Geometry (P.x, P.y, P.GetWidth(), fw);
        GetPainter().Paint2DQuadVGradient (GfxContext, p, Color (1.0f * v, s * v, s * v), Color (1.0f * v, s * v, 1.0f * v) );
        p.SetY (P.y + fw);
        GetPainter().Paint2DQuadVGradient (GfxContext, p, Color (1.0f * v, s * v, 1.0f * v), Color (s * v, s * v, 1.0f * v) );
        p.SetY (P.y + 2 * fw);
        GetPainter().Paint2DQuadVGradient (GfxContext, p, Color (s * v, s * v, 1.0f * v), Color (s * v, 1.0f * v, 1.0f * v) );
        p.SetY (P.y + 3 * fw);
        GetPainter().Paint2DQuadVGradient (GfxContext, p, Color (s * v, 1.0f * v, 1.0f * v), Color (s * v, 1.0f * v, s * v) );
        p.SetY (P.y + 4 * fw);
        GetPainter().Paint2DQuadVGradient (GfxContext, p, Color (s * v, 1.0f * v, s * v), Color (1.0f * v, 1.0f * v, s * v) );
        p.SetY (P.y + 5 * fw);
        p.SetHeight (P.GetHeight() - 5 * fw); // correct rounding errors
        GetPainter().Paint2DQuadVGradient (GfxContext, p, Color (1.0f * v, 1.0f * v, s * v), Color (1.0f * v, s * v, s * v) );

        Geometry pickermarker = Geometry (GetBaseX() + m_MarkerPosition.x - 2, GetBaseY() + m_MarkerPosition.y - 2, 5, 5);
        GetPainter().Paint2DQuadWireframe(GfxContext, pickermarker, OneMinusLuminance(rgb_));
      }
      else if (m_ColorChannel == color::SATURATION)
      {
        x = hsv_.hue * m_PickerArea->GetBaseWidth();
        y = (1.0f - hsv_.value) * m_PickerArea->GetBaseHeight();

        float value = hsv_.value;
        if (value < 0.3f) value = 0.3f;

        m_SaturationShader->SetColor(hsv_.hue, hsv_.saturation, hsv_.value, 1.0f);
        m_SaturationShader->SetScreenPositionOffset (GfxContext.GetViewportX (), GfxContext.GetViewportY ());
        BaseChannelTop = Color(color::RedGreenBlue(color::HueSaturationValue(hsv_.hue, 1.0f, value)));
        BaseChannelBottom = Color(value, value, value, 1.0f);
        m_SaturationShader->Render (
          m_PickerArea->GetBaseX(),
          m_PickerArea->GetBaseY(),
          0,
          m_PickerArea->GetBaseWidth(),
          m_PickerArea->GetBaseHeight(),
          GfxContext.GetViewportWidth (), GfxContext.GetViewportHeight ()
        );

        //Geometry pickermarker = Geometry(GetX() + x - 2, GetY() + y -2, 5, 5);
        Geometry pickermarker = Geometry (GetBaseX() + m_MarkerPosition.x - 2, GetBaseY() + m_MarkerPosition.y - 2, 5, 5);
        GetPainter().Paint2DQuadWireframe (GfxContext, pickermarker, OneMinusLuminance(rgb_) );
        GetPainter().Paint2DQuadColor (GfxContext, m_BaseChannelArea->GetGeometry(), BaseChannelTop, BaseChannelBottom, BaseChannelBottom, BaseChannelTop);
      }
      else if (m_ColorChannel == color::VALUE)
      {
        x = hsv_.hue * m_PickerArea->GetBaseWidth();
        y = (1.0f - hsv_.saturation) * m_PickerArea->GetBaseHeight();

        m_ValueShader->SetColor (hsv_.hue, hsv_.saturation, hsv_.value, 1.0f);
        m_ValueShader->SetScreenPositionOffset (GfxContext.GetViewportX (), GfxContext.GetViewportY ());
        BaseChannelTop = Color(color::RedGreenBlue(color::HueSaturationValue(hsv_.hue, hsv_.saturation, 1.0f)));
        BaseChannelBottom = Color(color::RedGreenBlue(color::HueSaturationValue(hsv_.hue, hsv_.saturation, 0.0f)));
        m_ValueShader->Render (
          m_PickerArea->GetBaseX(),
          m_PickerArea->GetBaseY(),
          0,
          m_PickerArea->GetBaseWidth(),
          m_PickerArea->GetBaseHeight(),
          GfxContext.GetViewportWidth (), GfxContext.GetViewportHeight ()
        );

        //Geometry pickermarker = Geometry(GetX() + x - 2, GetY() + y -2, 5, 5);
        Geometry pickermarker = Geometry (GetBaseX() + m_MarkerPosition.x - 2, GetBaseY() + m_MarkerPosition.y - 2, 5, 5);
        GetPainter().Paint2DQuadWireframe (GfxContext, pickermarker, OneMinusLuminance(rgb_) );
        GetPainter().Paint2DQuadColor (GfxContext, m_BaseChannelArea->GetGeometry(), BaseChannelTop, BaseChannelBottom, BaseChannelBottom, BaseChannelTop);
      }

      // Draw Marker on Base Chanel Area
      DrawBaseChannelMarker (GfxContext);
    }
  }

  void ColorEditor::DrawContent (GraphicsEngine &GfxContext, bool force_draw)
  {
    redcheck->ProcessDraw (GfxContext, force_draw);
    redtext->ProcessDraw (GfxContext, force_draw);
    greencheck->ProcessDraw (GfxContext, force_draw);
    greentext->ProcessDraw (GfxContext, force_draw);
    bluecheck->ProcessDraw (GfxContext, force_draw);
    bluetext->ProcessDraw (GfxContext, force_draw);

    huecheck->ProcessDraw (GfxContext, force_draw);
    huetext->ProcessDraw (GfxContext, force_draw);
    saturationcheck->ProcessDraw (GfxContext, force_draw);
    saturationtext->ProcessDraw (GfxContext, force_draw);
    valuecheck->ProcessDraw (GfxContext, force_draw);
    valuetext->ProcessDraw (GfxContext, force_draw);
  }

  void ColorEditor::PostDraw (GraphicsEngine &GfxContext, bool force_draw)
  {

  }

  void ColorEditor::RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    float BaseValue;

    if (m_ColorModel == color::RGB)
    {
      if (y < 0)
        BaseValue = 1.0f;
      else if (y > m_BaseChannelArea->GetBaseHeight() )
        BaseValue = 0.0f;
      else
        BaseValue = 1.0f - (float) y / (float) m_BaseChannelArea->GetBaseHeight();

      if (m_ColorChannel == color::RED)
        rgb_.red = BaseValue;
      else if (m_ColorChannel == color::GREEN)
        rgb_.green = BaseValue;
      else if (m_ColorChannel == color::BLUE)
        rgb_.blue = BaseValue;

      hsv_ = color::HueSaturationValue(rgb_);
    }

    if (m_ColorModel == color::HSV)
    {
      if (y < 0)
        BaseValue = 1.0f;
      else if (y > m_BaseChannelArea->GetBaseHeight() )
        BaseValue = 0.0f;
      else
        BaseValue = 1.0f - (float) y / (float) m_BaseChannelArea->GetBaseHeight();

      if (m_ColorChannel == color::HUE)
      {
        hsv_.hue = BaseValue;

        if (hsv_.hue >= 1.0f) hsv_.hue = 0.0f;
      }
      else if (m_ColorChannel == color::SATURATION)
        hsv_.saturation = BaseValue;
      else if (m_ColorChannel == color::VALUE)
        hsv_.value = BaseValue;

      rgb_ = color::RedGreenBlue(hsv_);
    }

    redtext->SetText (m_Validator.ToString (255 * rgb_.red) );
    greentext->SetText (m_Validator.ToString (255 * rgb_.green) );
    bluetext->SetText (m_Validator.ToString (255 * rgb_.blue) );
    huetext->SetText (m_Validator.ToString (360 * hsv_.hue) );
    saturationtext->SetText (m_Validator.ToString (100 * hsv_.saturation) );
    valuetext->SetText (m_Validator.ToString (100 * hsv_.value) );
    m_VertMarkerPosition = Point (Clamp<int> (x, 0, m_BaseChannelArea->GetBaseWidth() - 1), Clamp<int> (y, 0, m_BaseChannelArea->GetBaseHeight() - 1) );

    sigChange.emit (this);
    NeedRedraw();
  }

  void ColorEditor::RecvMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    NeedRedraw();
  }

  void ColorEditor::RecvMouseDrag (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
  {
    RecvMouseDown (x, y, button_flags, key_flags);
  }

  void ColorEditor::RecvPickerMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    if (m_ColorModel == color::RGB)
    {
      if (m_ColorChannel == color::RED)
      {
        if (y < 0)
          rgb_.green = 1.0f;
        else if (y > m_PickerArea->GetBaseHeight() )
          rgb_.green = 0.0f;
        else
          rgb_.green = 1.0f - (float) y / (float) m_PickerArea->GetBaseHeight();

        if (x < 0)
          rgb_.blue = 0.0f;
        else if (x > m_PickerArea->GetBaseWidth() )
          rgb_.blue = 1.0f;
        else
          rgb_.blue = (float) x / (float) m_PickerArea->GetBaseWidth();

      }

      if (m_ColorChannel == color::GREEN)
      {
        if (y < 0)
          rgb_.red = 1.0f;
        else if (y > m_PickerArea->GetBaseHeight() )
          rgb_.red = 0.0f;
        else
          rgb_.red = 1.0f - (float) y / (float) m_PickerArea->GetBaseHeight();

        if (x < 0)
          rgb_.blue = 0.0f;
        else if (x > m_PickerArea->GetBaseWidth() )
          rgb_.blue = 1.0f;
        else
          rgb_.blue = (float) x / (float) m_PickerArea->GetBaseWidth();

      }

      if (m_ColorChannel == color::BLUE)
      {
        if (x < 0)
          rgb_.red = 0.0f;
        else if (x > m_PickerArea->GetBaseWidth() )
          rgb_.red = 1.0f;
        else
          rgb_.red = (float) x / (float) m_PickerArea->GetBaseWidth();

        if (y < 0)
          rgb_.green = 1.0f;
        else if (y > m_PickerArea->GetBaseHeight() )
          rgb_.green = 0.0f;
        else
          rgb_.green = 1.0f - (float) y / (float) m_PickerArea->GetBaseHeight();
      }

      hsv_ = color::HueSaturationValue(rgb_);
      m_MarkerPosition = Point (Clamp<int> (x, 0, m_PickerArea->GetBaseWidth() - 1), Clamp<int> (y, 0, m_PickerArea->GetBaseHeight() - 1) );
    }

    if (m_ColorModel == color::HSV)
    {
      if (m_ColorChannel == color::HUE)
      {
        if (y < 0)
          hsv_.value = 1.0f;
        else if (y > m_PickerArea->GetBaseHeight() )
          hsv_.value = 0.0f;
        else
          hsv_.value = 1.0f - (float) y / (float) m_PickerArea->GetBaseHeight();

        if (x < 0)
          hsv_.saturation = 0.0f;
        else if (x > m_PickerArea->GetBaseWidth() )
          hsv_.saturation = 1.0f;
        else
          hsv_.saturation = (float) x / (float) m_PickerArea->GetBaseWidth();

      }

      if (m_ColorChannel == color::SATURATION)
      {
        if (y < 0)
          hsv_.value = 1.0f;
        else if (y > m_PickerArea->GetBaseHeight() )
          hsv_.value = 0.0f;
        else
          hsv_.value = 1.0f - (float) y / (float) m_PickerArea->GetBaseHeight();

        if (x < 0)
          hsv_.hue = 0.0f;
        else if (x >= m_PickerArea->GetBaseWidth() )
          hsv_.hue = 0.0f;
        else
          hsv_.hue = (float) x / (float) m_PickerArea->GetBaseWidth();

      }

      if (m_ColorChannel == color::VALUE)
      {
        if (x < 0)
          hsv_.hue = 0.0f;
        else if (x >= m_PickerArea->GetBaseWidth() )
          hsv_.hue = 0.0f;
        else
          hsv_.hue = (float) x / (float) m_PickerArea->GetBaseWidth();

        if (y < 0)
          hsv_.saturation = 1.0f;
        else if (y > m_PickerArea->GetBaseHeight() )
          hsv_.saturation = 0.0f;
        else
          hsv_.saturation = 1.0f - (float) y / (float) m_PickerArea->GetBaseHeight();
      }

      rgb_ = color::RedGreenBlue(hsv_);
      m_MarkerPosition = Point (Clamp<int> (x, 0, m_PickerArea->GetBaseWidth() - 1), Clamp<int> (y, 0, m_PickerArea->GetBaseHeight() - 1) );
    }


    redtext->SetText (m_Validator.ToString (255 * rgb_.red) );
    greentext->SetText (m_Validator.ToString (255 * rgb_.green) );
    bluetext->SetText (m_Validator.ToString (255 * rgb_.blue) );
    huetext->SetText (m_Validator.ToString (360 * hsv_.hue) );
    saturationtext->SetText (m_Validator.ToString (100 * hsv_.saturation) );
    valuetext->SetText (m_Validator.ToString (100 * hsv_.value) );

    sigChange.emit (this);
    NeedRedraw();
  }

  void ColorEditor::RecvPickerMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    NeedRedraw();
  }

  void ColorEditor::RecvPickerMouseDrag (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
  {
    RecvPickerMouseDown (x, y, button_flags, key_flags);
  }

void ColorEditor::RecvCheckColorModel (bool b, color::Model ColorModel, color::Channel ColorChannel)
  {
    if (b)
    {
      if ( (ColorModel == color::HSV) && (m_ColorModel == color::RGB) )
      {
        hsv_ = color::HueSaturationValue(rgb_);
      }

      if ( (ColorModel == color::RGB) && (m_ColorModel == color::HSV) )
      {
        rgb_ = color::RedGreenBlue(hsv_);
      }

      m_ColorModel = ColorModel;
      m_ColorChannel = ColorChannel;
    }

    if (b && (ColorModel == color::RGB) )
    {
      int x = 0;
      int y = 0;
      int z = 1;

      if (m_ColorChannel == color::RED)
      {
        z = (1.0f - rgb_.red) * m_PickerArea->GetBaseHeight();
        y = (1.0f - rgb_.green) * m_PickerArea->GetBaseHeight();
        x = rgb_.blue * m_PickerArea->GetBaseWidth();
      }

      if (m_ColorChannel == color::GREEN)
      {
        z = (1.0f - rgb_.green) * m_PickerArea->GetBaseHeight();
        y = (1.0f - rgb_.red) * m_PickerArea->GetBaseHeight();
        x = rgb_.blue * m_PickerArea->GetBaseWidth();
      }

      if (m_ColorChannel == color::BLUE)
      {
        z = (1.0f - rgb_.blue) * m_PickerArea->GetBaseHeight();
        y = (1.0f - rgb_.green) * m_PickerArea->GetBaseHeight();
        x = rgb_.red * m_PickerArea->GetBaseWidth();
      }

      m_VertMarkerPosition = Point (Clamp<int> (0, 0, m_BaseChannelArea->GetBaseWidth() - 1), Clamp<int> (z, 0, m_BaseChannelArea->GetBaseHeight() - 1) );
      m_MarkerPosition = Point (Clamp<int> (x, 0, m_PickerArea->GetBaseWidth() - 1), Clamp<int> (y, 0, m_PickerArea->GetBaseHeight() - 1) );

      redtext->SetText (m_Validator.ToString (255 * rgb_.red) );
      greentext->SetText (m_Validator.ToString (255 * rgb_.green) );
      bluetext->SetText (m_Validator.ToString (255 * rgb_.blue) );
    }

    if (b && (ColorModel == color::HSV) )
    {
      int x = 0;
      int y = 0;
      int z = 1;

      if (m_ColorChannel == color::HUE)
      {
        z = (1.0f - hsv_.hue) * m_PickerArea->GetBaseHeight();
        y = (1.0f - hsv_.value) * m_PickerArea->GetBaseHeight();
        x = hsv_.saturation * m_PickerArea->GetBaseWidth();
      }

      if (m_ColorChannel == color::SATURATION)
      {
        z = (1.0f - hsv_.saturation) * m_PickerArea->GetBaseHeight();
        y = (1.0f - hsv_.value) * m_PickerArea->GetBaseHeight();
        x = hsv_.hue * m_PickerArea->GetBaseWidth();
      }

      if (m_ColorChannel == color::VALUE)
      {
        z = (1.0f - hsv_.value) * m_PickerArea->GetBaseHeight();
        y = (1.0f - hsv_.saturation) * m_PickerArea->GetBaseHeight();
        x = hsv_.hue * m_PickerArea->GetBaseWidth();
      }

      m_VertMarkerPosition = Point (Clamp<int> (0, 0, m_BaseChannelArea->GetBaseWidth() - 1), Clamp<int> (z, 0, m_BaseChannelArea->GetBaseHeight() - 1) );
      m_MarkerPosition = Point (Clamp<int> (x, 0, m_PickerArea->GetBaseWidth() - 1), Clamp<int> (y, 0, m_PickerArea->GetBaseHeight() - 1) );

      huetext->SetText (m_Validator.ToString (360 * hsv_.hue) );
      saturationtext->SetText (m_Validator.ToString (100 * hsv_.saturation) );
      valuetext->SetText (m_Validator.ToString (100 * hsv_.value) );
    }

    NeedRedraw();
  }

  Color ColorEditor::GetRGBColor() const
  {
    return Color(rgb_);
  }

  void ColorEditor::SetRGB(Color const& rgb)
  {
    SetRGB(rgb.red, rgb.green, rgb.blue );
  }

  void ColorEditor::SetRGB (double r, double g, double b)
  {
    rgb_ = color::RedGreenBlue(Clamp<double>(r, 0.0, 1.0),
                               Clamp<double>(g, 0.0, 1.0),
                               Clamp<double> (b, 0.0, 1.0));
    hsv_ = color::HueSaturationValue(rgb_);
    RecvCheckColorModel(true, m_ColorModel, m_ColorChannel);
    sigChange.emit (this);
  }

  void ColorEditor::SetHSV (double h, double s, double v)
  {
    hsv_ = color::HueSaturationValue(Clamp<double>(h, 0.0, 1.0),
                                     Clamp<double>(s, 0.0, 1.0),
                                     Clamp<double>(v, 0.0, 1.0));
    rgb_ = color::RedGreenBlue(hsv_);
    RecvCheckColorModel(true, m_ColorModel, m_ColorChannel);
    sigChange.emit (this);
  }

  void ColorEditor::SetRed(double red)
  {
    SetRGB(red, rgb_.green, rgb_.blue);
  }

  void ColorEditor::SetGreen(double green)
  {
    SetRGB(rgb_.red, green, rgb_.blue);
  }

  void ColorEditor::SetBlue(double blue)
  {
    SetRGB(rgb_.red, rgb_.green, blue);
  }

  void ColorEditor::SetHue (double hue)
  {
    SetHSV(hue, hsv_.saturation, hsv_.value);
  }

  void ColorEditor::SetSaturation(double saturation)
  {
    SetHSV(hsv_.hue, saturation, hsv_.value);
  }

  void ColorEditor::SetValue(double value)
  {
    SetHSV(hsv_.hue, hsv_.saturation, value);
  }

  void ColorEditor::SetColorModel(color::Model colormodel,
                                  color::Channel colorchannel)
  {
    if (colormodel == color::HSV)
    {
      if ( (colorchannel != color::HUE) &&
           (colorchannel != color::SATURATION) &&
           (colorchannel != color::VALUE) )
      {
        nuxDebugMsg (TEXT ("[ColorEditor::SetColorModel] The color model (HSV) and the color channel don't match.") );
        return;
      }
    }

    if (colormodel == color::RGB)
    {
      if ( (colorchannel != color::RED) &&
           (colorchannel != color::GREEN) &&
           (colorchannel != color::BLUE) )
      {
        nuxDebugMsg (TEXT ("[ColorEditor::SetColorModel] The color model (RGB) and the color channel don't match.") );
        return;
      }
    }

    m_ColorModel = colormodel;
    m_ColorChannel = colorchannel;
    RecvCheckColorModel (true, m_ColorModel, m_ColorChannel);

    if (m_ColorChannel == color::RED)
      radiogroup->ActivateButton (redcheck);
    else if (m_ColorChannel == color::GREEN)
      radiogroup->ActivateButton (greencheck);
    else if (m_ColorChannel == color::BLUE)
      radiogroup->ActivateButton (bluecheck);
    else if (m_ColorChannel == color::HUE)
      radiogroup->ActivateButton (huecheck);
    else if (m_ColorChannel == color::SATURATION)
      radiogroup->ActivateButton (saturationcheck);
    else if (m_ColorChannel == color::VALUE)
      radiogroup->ActivateButton (valuecheck);
  }

  color::Model ColorEditor::GetColorModel() const
  {
    return m_ColorModel;
  }

  color::Channel ColorEditor::GetColorChannel() const
  {
    return m_ColorChannel;
  }

  bool ColorEditor::AcceptKeyNavFocus()
  {
    return false;
  }
}
