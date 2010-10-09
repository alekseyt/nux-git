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


#ifndef ABSTRACTOBJECTBASE_H
#define ABSTRACTOBJECTBASE_H

namespace nux   //NUX_NAMESPACE_BEGIN
{

  class Layout;

  enum eControlType
  {
    eNumericValuator,
    eSpinBox,
    eVector3Box,
    eBooleanBox,
    eStaticText,
    ePopupBox
  };

  class View: public InputArea //Area //public sigc::trackable
  {
    NUX_DECLARE_OBJECT_TYPE (View, InputArea);
  public:
    View (NUX_FILE_LINE_DECL);
    virtual ~View();
    //virtual RemoveView(View *ic);

  public:

    virtual long ProcessEvent (IEvent &ievent, long TraverseInfo, long ProcessEventInfo) = 0;


    virtual long ComputeChildLayout();
    virtual void PositionChildLayout (float offsetX, float offsetY);

    // NUXTODO: Find better name
    virtual long ComputeLayout2()
    {
      return ComputeChildLayout();
    };

    // NUXTODO: Find better name
    virtual void ComputePosition2 (float offsetX, float offsetY)
    {
      PositionChildLayout (offsetX, offsetY);
    };

    virtual void PreLayoutManagement();
    virtual long PostLayoutManagement (long LayoutResult);
    virtual void PreResizeGeometry();
    virtual void PostResizeGeometry();

    // NUXTODO: Find better name
    virtual long PostProcessEvent2 (IEvent &ievent, long TraverseInfo, long ProcessEventInfo);

    virtual bool IsLayout() const
    {
      return false;
    }
    virtual bool IsSpaceLayout() const
    {
      return false;
    }
    virtual bool IsArea() const
    {
      return false;
    }
    virtual bool IsView() const
    {
      return true;
    }

    void SetUsingStyleDrawing (bool b)
    {
      m_UseStyleDrawing = b;
    };
    bool IsUsingStyleDrawing() const
    {
      return m_UseStyleDrawing;
    };

    void DisableWidget();
    void EnableWidget();
    bool IsWidgetEnabled();

  protected:
    void InitializeWidgets();
    void InitializeLayout();
    void DestroyLayout();

  private:
    bool m_UseStyleDrawing;
    bool m_IsEnabled;
  private:
    virtual void Draw (GraphicsContext &GfxContext, bool force_draw) = 0;
    virtual void DrawContent (GraphicsContext &GfxContext, bool force_draw);
    virtual void PostDraw (GraphicsContext &GfxContext, bool force_draw);
  public:
    virtual void ProcessDraw (GraphicsContext &GfxContext, bool force_draw);
    //! Causes a redraw. The widget parameter m_NeedRedraw is set to true. The widget Draw(), DrawContent() and PostDraw() are called.
    virtual void NeedRedraw();
    //! Causes a soft redraw. The widget parameter m_NeedRedraw is set to false. The widget DrawContent() and PostDraw() are called.
    virtual void NeedSoftRedraw();
    virtual bool IsRedrawNeeded();
    virtual void DoneRedraw();
    virtual void DrawLayout();

    virtual void OverlayDrawing (GraphicsContext &GfxContext) {}

    //Layout Bridge

    bool SearchInAllSubNodes (Area *bo);
    bool SearchInFirstSubNodes (Area *bo);

    //! Set Geometry
    /*
        Set the Geometry of the View and the geometry of the Default Background Area.
        For simple interface control UI classes (RGBValuator...), this is enough.
        For others, they have to overwrite the function and do the appropriate computations
        for their component.
    */
    virtual void SetGeometry (const Geometry &geo);

    //! Return true if this object can break the layout.
    /*
        Return true if this object can break the layout, meaning, the layout can be done on the composition layout only without
        recomputing the whole window layout.
    */
    virtual bool CanBreakLayout()
    {
      return false;
    }

    //! Set the font to be used by the widget. If Font is null then use the default system font.
    /*!
        Set the font to be used by the widget. If Font is null then use the default system font.
        @param Font The font to use when rendering text.
    */
    virtual void SetFont (IntrusiveSP<FontTexture> Font);

    //! Get the font used for rendering text.
    /*!
        Get the font used for rendering text.
        @return The font to use when rendering text.
    */
    IntrusiveSP<FontTexture> GetFont();

    virtual void SetTextColor (const Color &color);
    virtual Color GetTextColor();

  protected:
    IntrusiveSP<FontTexture> m_font;
    Color m_TextColor;
    virtual Layout *GetCompositionLayout() const;
    virtual void SetCompositionLayout (Layout *lyt);
    void RemoveCompositionLayout();
    bool IsFullRedraw() const
    {
      return m_IsFullRedraw;
    }

  protected:
    Layout *m_CompositionLayout;

    bool m_NeedRedraw;

    //! This parameter is set to true is Draw is Called before ContentDraw. It is read-only and can be accessed by calling IsFullRedraw();
    bool m_IsFullRedraw;
  public:


    friend class Layout;
    friend class Area;

  };

} //NUX_NAMESPACE_END

#endif // ABSTRACTOBJECTBASE_H
