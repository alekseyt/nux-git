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


#ifndef SCROLLVIEW_H
#define SCROLLVIEW_H

namespace nux
{
  class HScrollBar;
  class VScrollBar;

// Rename it to ScrollArea or ScrollWindow
  class ScrollView: public View
  {
  public:
    ScrollView (NUX_FILE_LINE_PROTO);
    virtual ~ScrollView();

    virtual long ProcessEvent (IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
    void Draw (GraphicsContext &GfxContext, bool force_draw);

    // API
    void EnableVerticalScrollBar (bool b);
    void EnableHorizontalScrollBar (bool b);

  public:
    /*!
        Set the table size to be such that all the content items of the table are visible .
        The scrollbar will be useless as the content is entirely visible all the time. If the table is empty, then it assume its minimum size.
        This is needed for table inside ComboBox drop down memu.
        @param b If b is true, the size of the table is constrained by its content.
        @see IsSizeMatchLayout()
    */
    void SetSizeMatchContent (bool b);

    /*!
        Check if the table size is constrained by its content.
        @return If the return value is true, the table size is constrained by its content.
        @see SetSizeMatchContent
    */
    bool IsSizeMatchContent() const;

//    bool SetMaxSizeMatchLayout(Size size)
//    {
//        m_MaxSizeMatchLayout = size;
//        if(m_MaxSizeMatchLayout.GetWidth() < 0)
//            m_MaxSizeMatchLayout.SetWidth(0);
//        if(m_MaxSizeMatchLayout.GetBaseHeight() < 0)
//            m_MaxSizeMatchLayout.SetHeight(0);
//    }

  private:
    /**
        If True, the scrollbar size will be adjusted to match the size of the content.
        This is useful for the ComboBoxComplex widget.
    */
    bool m_bSizeMatchContent;

  public:
    //! Inherited from Area
    virtual void SetGeometry (const Geometry &geo);

    /////////////////
    //  EMITERS    //
    /////////////////
    void OnSizeGrigMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags);
    void OnSizeGrigMouseDrag (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
    void EmitMouseDrag (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
    //void EmitInternalResize(int x, int y, int w, int h);


    /////////////////
    //  RECEIVERS  //
    /////////////////
    virtual void ScrollLeft (float stepx, int mousedx);
    virtual void ScrollRight (float stepx, int mousedx);
    virtual void ScrollUp (float stepy, int mousedy);
    virtual void ScrollDown (float stepy, int mousedy);

    virtual void ResetScrollToLeft();
    virtual void ResetScrollToRight();
    virtual void ResetScrollToUp();
    virtual void ResetScrollToDown();

    //! X offset of the content
    int m_ContentOffsetX;
    //! Y offset of the content
    int m_ContentOffsetY;

    // Geometry of the layout that encompass the child layouts.
    //! X Position of the content
    int m_ViewContentX;
    //! Y Position of the content
    int m_ViewContentY;
    //! Width of content
    int m_ViewContentWidth;
    //! Height of content
    int m_ViewContentHeight;

    //Client View Area
    //! X position of the scroll view content area
    int m_ViewX;
    //! Y position of the scroll view content area
    int m_ViewY;
    //! Width of the scroll view content area
    int m_ViewWidth;
    //! Height of the scroll view content area
    int m_ViewHeight;

    Geometry m_ViewGeometry;
    Geometry m_ContentGeometry;

    // signals
    sigc::signal<void> SigTest;
    sigc::signal<void> sigMoveWindow;
    sigc::signal<void, int, int, int, int> sigResize;

  protected:
    //SmartPtrGLTexture m_BackgroundTexture;

    // Backup texture to speed up scrolling
    TRefGL<IOpenGLFrameBufferObject> m_FrameBufferObject;
    TRefGL<IOpenGLTexture2D> m_TextureBuffer[2];
    void SwapTextureIndex()
    {
      m_TextureIndex = (m_TextureIndex == 0) ? 1 : 0;
    }
    void SetTextureIndex (int index)
    {
      m_TextureIndex = index;
    }
    int GetTextureIndex()
    {
      return m_TextureIndex;
    }
    int m_TextureIndex;
    bool m_ReformatTexture;


    // ScrollBars
    HScrollBar     *hscrollbar;
    VScrollBar     *vscrollbar;
    bool m_horizontal_scrollbar_enable;
    bool m_vertical_scrollbar_enable;

    int m_SizeGripDragPositionX; //<<<--- remove this
    int m_SizeGripDragPositionY;

    //int viewx;
    // Internal function
    int getBorder() const;
    int getTopBorder() const;
    void setBorder (int border);
    void setTopBorder (int top_border);

    int m_top_border;
    int m_border;

  public:
    void    SetViewContentLeftMargin (int margin)
    {
      m_ViewContentLeftMargin = margin;
    }
    int     GetViewContentLeftMargin() const
    {
      return m_ViewContentLeftMargin;
    }
    void    SetViewContentRightMargin (int margin)
    {
      m_ViewContentRightMargin = margin;
    }
    int     GetViewContentRightMargin() const
    {
      return m_ViewContentRightMargin;
    }
    void    SetViewContentTopMargin (int margin)
    {
      m_ViewContentTopMargin = margin;
    }
    int     GetViewContentTopMargin() const
    {
      return m_ViewContentTopMargin;
    }
    void    SetViewContentBottomMargin (int margin)
    {
      m_ViewContentBottomMargin = margin;
    }
    int     GetViewContentBottomMargin() const
    {
      return m_ViewContentBottomMargin;
    }
  private:
    int m_ViewContentLeftMargin;
    int m_ViewContentRightMargin;
    int m_ViewContentTopMargin;
    int m_ViewContentBottomMargin;

  protected:
    void FormatContent();
    virtual void PreLayoutManagement();
    virtual long PostLayoutManagement (long LayoutResult);
    virtual void PositionChildLayout (float offsetX, float offsetY);

    virtual void PreLayoutManagement2();
    virtual long PostLayoutManagement2 (long LayoutResult);

  public:
    ///////////////////////////////////////////////////////
    // AbstractInterfaceObject
    ///////////////////////////////////////////////////////
  };
}

#endif // SCROLLVIEW_H