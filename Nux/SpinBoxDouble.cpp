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

#include "TimerProc.h"
#include "HLayout.h"
#include "VLayout.h"
#include "DoubleValidator.h"
#include "SpinBox_Logic.h"

#include "SpinBoxDouble.h"
namespace nux
{
  const Color SPINBOX_DOUBLE_BUTTON_COLOR = Color (0xFF4D4D4D);
  const Color SPINBOX_DOUBLE_BUTTON_MOUSEOVER_COLOR = Color (0xFF222222);

  SpinBoxDouble::SpinBoxDouble (double Value, double Step, double MinValue, double MaxValue)
    :   m_DoubleValidator (MinValue, MaxValue)
    ,   m_Step (Step)
  {
    InitializeLayout();
    InitializeWidgets();
    SetValue (Value);
  }

  SpinBoxDouble::~SpinBoxDouble()
  {
    DestroyLayout();
  }

  void SpinBoxDouble::InitializeWidgets()
  {
    m_EditLine->SetValidator (&m_DoubleValidator);
    m_EditLine->SetSuffix (TEXT ("") );
    m_EditLine->SetPrefix (TEXT ("") );
    m_EditLine->SetText (NString::Printf (TEXT ("%.3f"), m_DoubleValidator.GetMinimum() ) );

    m_EditLine->SetMinimumSize (1.5 * DEFAULT_WIDGET_WIDTH, DEFAULT_WIDGET_HEIGHT);
    m_EditLine->SetGeometry (Geometry (0, 0, DEFAULT_WIDGET_WIDTH, DEFAULT_WIDGET_HEIGHT) );

    m_SpinnerUpBtn->SetMinimumSize (15, 10);
    m_SpinnerUpBtn->SetGeometry (Geometry (0, 0, 15, 10) );
    m_SpinnerDownBtn->SetMinimumSize (15, 10);
    m_SpinnerDownBtn->SetGeometry (Geometry (0, 0, 15, 10) );

    m_UpTimerCallback = new TimerFunctor;
    m_UpTimerCallback->OnTimerExpired.connect (sigc::mem_fun (this, &SpinBox_Logic::TimerSpinUpBtn) );
    m_DownTimerCallback = new TimerFunctor;
    m_DownTimerCallback->OnTimerExpired.connect (sigc::mem_fun (this, &SpinBox_Logic::TimerSpinDownBtn) );

    // Set the minimum size of this widget.
    // This is use by TextLineEditPropertyItem::GetItemBestHeight
    SetMinimumSize (DEFAULT_WIDGET_WIDTH, PRACTICAL_WIDGET_HEIGHT);

    m_hlayout->AddView (m_EditLine, 1);

    m_vlayout->AddView (m_SpinnerUpBtn, 1);
    m_vlayout->AddView (m_SpinnerDownBtn, 1);
    m_hlayout->AddLayout (m_vlayout, 0);

    SetCompositionLayout (m_hlayout);
  }

  void SpinBoxDouble::InitializeLayout()
  {
    m_hlayout = new HLayout (NUX_TRACKER_LOCATION);
    m_vlayout = new VLayout (NUX_TRACKER_LOCATION);
  }

  void SpinBoxDouble::DestroyLayout()
  {
  }

  long SpinBoxDouble::ProcessEvent (IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
  {
    long ret = TraverseInfo;
    ret = m_SpinnerUpBtn->OnEvent (ievent, ret, ProcessEventInfo);
    ret = m_SpinnerDownBtn->OnEvent (ievent, ret, ProcessEventInfo);
    ret = m_EditLine->ProcessEvent (ievent, ret, ProcessEventInfo);
    ret = PostProcessEvent2 (ievent, ret, ProcessEventInfo);
    return ret;
  }

  void SpinBoxDouble::Draw (GraphicsEngine &GfxContext, bool force_draw)
  {
    Geometry base = GetGeometry();
    GetPainter().PaintBackground (GfxContext, base);

    if (m_EditLine->IsMouseInside() || m_SpinnerUpBtn->IsMouseInside() || m_SpinnerDownBtn->IsMouseInside() )
    {

      GetPainter().PaintShapeCorner (GfxContext, m_SpinnerUpBtn->GetGeometry(), SPINBOX_DOUBLE_BUTTON_MOUSEOVER_COLOR, eSHAPE_CORNER_ROUND4,
                                 eCornerTopRight, false);
      GetPainter().PaintShapeCorner (GfxContext, m_SpinnerDownBtn->GetGeometry(), SPINBOX_DOUBLE_BUTTON_MOUSEOVER_COLOR, eSHAPE_CORNER_ROUND4,
                                 eCornerBottomRight, false);
    }
    else
    {
      GetPainter().PaintShapeCorner (GfxContext, m_SpinnerUpBtn->GetGeometry(), SPINBOX_DOUBLE_BUTTON_COLOR, eSHAPE_CORNER_ROUND4,
                                 eCornerTopRight, false);
      GetPainter().PaintShapeCorner (GfxContext, m_SpinnerDownBtn->GetGeometry(), SPINBOX_DOUBLE_BUTTON_COLOR, eSHAPE_CORNER_ROUND4,
                                 eCornerBottomRight, false);
    }

    GeometryPositioning gp (eHACenter, eVACenter);
    Geometry GeoPo = ComputeGeometryPositioning (m_SpinnerUpBtn->GetGeometry(), GetTheme().GetImageGeometry (eSPINER_UP), gp);

    if (m_SpinnerUpBtn->IsMouseInside() )
      GetPainter().PaintShape (GfxContext, GeoPo, Color (0xFFFFFFFF), eSPINER_UP);
    else
      GetPainter().PaintShape (GfxContext, GeoPo, Color (0xFFFFFFFF), eSPINER_UP);


    gp.SetAlignment (eHACenter, eVACenter);
    GeoPo = ComputeGeometryPositioning (m_SpinnerDownBtn->GetGeometry(), GetTheme().GetImageGeometry (eSPINER_DOWN), gp);

    if (m_SpinnerDownBtn->IsMouseInside() )
      GetPainter().PaintShape (GfxContext, GeoPo, Color (0xFFFFFFFF), eSPINER_DOWN);
    else
      GetPainter().PaintShape (GfxContext, GeoPo, Color (0xFFFFFFFF), eSPINER_DOWN);

    m_EditLine->NeedRedraw();
  }

  void SpinBoxDouble::DrawContent (GraphicsEngine &GfxContext, bool force_draw)
  {
    m_EditLine->ProcessDraw (GfxContext, force_draw);
  }

  void SpinBoxDouble::PostDraw (GraphicsEngine &GfxContext, bool force_draw)
  {

  }

  void SpinBoxDouble::SetValue (double value)
  {
    m_Value = m_DoubleValidator.GetClampedValue (value);
//    if(m_Value < m_DoubleValidator.GetMinimum())
//        m_Value = m_DoubleValidator.GetMinimum();
//    if(m_Value > m_DoubleValidator.GetMaximum())
//        m_Value = m_DoubleValidator.GetMaximum();
    m_EditLine->SetText (NString::Printf ("%.3f", m_Value));
    sigValueChanged.emit (this);
    sigValue.emit (m_Value);
    NeedRedraw();
  }

  double SpinBoxDouble::GetValue() const
  {
    return m_Value;
  }

  void SpinBoxDouble::SetStep (double d)
  {
    m_Step = d;

    if (m_Step <= 0)
      m_Step = 1.0;

    NeedRedraw();
  }

  double SpinBoxDouble::GetStep() const
  {
    return m_Step;
  }

  double SpinBoxDouble::GetMinValue() const
  {
    return m_DoubleValidator.GetMinimum();
  }

  double SpinBoxDouble::GetMaxValue() const
  {
    return m_DoubleValidator.GetMaximum();
  }

  void SpinBoxDouble::SetRange (double MinValue, double Maxvalue)
  {
    m_DoubleValidator.SetMinimum (MinValue);
    m_DoubleValidator.SetMaximum (Maxvalue);
    m_Value = m_DoubleValidator.GetClampedValue (m_Value);
    sigValueChanged.emit (this);
    sigValue.emit (m_Value);
    NeedRedraw();
  }

  void SpinBoxDouble::ImplementIncrementBtn()
  {
    SetValue (m_Value + m_Step);

    if (m_Value < m_DoubleValidator.GetMaximum() )
    {
      if (m_UpTimerHandler.IsValid() )
        m_UpTimerHandler = GetTimer().AddTimerHandler (100, m_UpTimerCallback, 0);
      else
        m_UpTimerHandler = GetTimer().AddTimerHandler (800, m_UpTimerCallback, 0);

      NeedRedraw();
    }

    sigValueChanged.emit (this);
    sigIncrement.emit (this);
    sigValue.emit (m_Value);
  }

  void SpinBoxDouble::ImplementDecrementBtn()
  {
    SetValue (m_Value - m_Step);

    if (m_Value > m_DoubleValidator.GetMinimum() )
    {
      if (m_DownTimerHandler.IsValid() )
        m_DownTimerHandler = GetTimer().AddTimerHandler (100, m_DownTimerCallback, 0);
      else
        m_DownTimerHandler = GetTimer().AddTimerHandler (800, m_DownTimerCallback, 0);

      NeedRedraw();
    }

    sigValueChanged.emit (this);
    sigDecrement.emit (this);
    sigValue.emit (m_Value);
  }

  void SpinBoxDouble::ImplementValidateEntry()
  {
    double ret = 0;
    ret = CharToDouble (m_EditLine->GetCleanText().GetTCharPtr() );
    {
      m_Value = m_DoubleValidator.GetClampedValue (ret);
      m_EditLine->SetText (NString::Printf ("%.3f", m_Value) );
      sigValueChanged.emit (this);
      sigValue.emit (m_Value);
//
//        if(m_Value < m_DoubleValidator.GetMinimum())
//        {
//            m_Value = m_DoubleValidator.GetMinimum();
//            m_EditLine->SetText(NString::Printf("%.3f", m_Value));
//        }
//        if(m_Value > m_DoubleValidator.GetMaximum())
//        {
//            m_Value = m_DoubleValidator.GetMaximum();
//            m_EditLine->SetText(NString::Printf("%.3f", m_Value));
//        }
    }
//     else
//     {
//         m_EditLine->SetText(NString::Printf("%.3f", m_Value));
//         sigValueChanged.emit(this);
//         sigValue.emit(m_Value);
//     }
  }

}
