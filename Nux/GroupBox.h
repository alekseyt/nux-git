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


#ifndef GROUPBOX_H
#define GROUPBOX_H

namespace nux
{

  class Layout;

  /*!
      A stretch factor of 0 or 1 for the GroupBox has no effect because the GroupBox is designed to tightly adjust
      to the size of its composition layout. This unlike the Panel widget for instance who does not force itself to
      adjust to the size of its composition layout.
  */
  class GroupBox : public View
  {
  public:
    GroupBox (const TCHAR *Caption = TEXT (""), NUX_FILE_LINE_PROTO);
    ~GroupBox();

    virtual long ProcessEvent (IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
    virtual void Draw (GraphicsEngine &GfxContext, bool force_draw);
    virtual void DrawContent (GraphicsEngine &GfxContext, bool force_draw);
    virtual void PostDraw (GraphicsEngine &GfxContext, bool force_draw);

    virtual bool SetLayout (Layout *layout);
    void SetCaption (const TCHAR *Caption);

  protected:
    virtual bool AcceptKeyNavFocus();

  private:
    virtual void PreLayoutManagement();
    virtual long PostLayoutManagement (long LayoutResult);
    virtual void PositionChildLayout (float offsetX, float offsetY);

    bool bCaptionAvailable;
    InputArea m_CaptionArea;
    Layout *m_layout;
  };
}

#endif // GROUPBOX_H
