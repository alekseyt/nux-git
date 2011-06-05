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
#include "Button.h"
#include "PangoText.h"
#include "HLayout.h"
#include "VLayout.h"
#include "TextureArea.h"

namespace nux
{
  NUX_IMPLEMENT_OBJECT_TYPE (Button);

  Button::Button (TextureArea *image, NUX_FILE_LINE_DECL)
      : AbstractButton (NUX_FILE_LINE_PARAM)
      , label (this, "label")
      , image_position (this, "image_position") {
    this->image = image;
    Init();
  }

Button::Button (const std::string label, NUX_FILE_LINE_DECL)
      : AbstractButton (NUX_FILE_LINE_PARAM)
      , label (this, "label")
      , image_position (this, "image_position") {
    this->label = label;
    Init();
  }

Button::Button (const std::string label, TextureArea *image, NUX_FILE_LINE_DECL)
      : AbstractButton (NUX_FILE_LINE_PARAM)
      , label (this, "label")
      , image_position (this, "image_position") {
    this->label = label;
    this->image = image;
    Init();
  }

Button::Button (NUX_FILE_LINE_DECL)
      : AbstractButton (NUX_FILE_LINE_PARAM)
      , label (this, "label")
      , image_position (this, "image_position") {
    Init();
  }

  Button::~Button() {
  }

  void Button::Init () {
    // Set Geometry
    SetMinimumSize (DEFAULT_WIDGET_WIDTH, PRACTICAL_WIDGET_HEIGHT);
    SetTextColor (Colors::Black);
    state.changed.connect (sigc::mem_fun(this, &Button::OnStateChanged));

    // connect up to the imag/label signals
    label.changed.connect (sigc::mem_fun(this, &Button::OnLabelChanged));

    //FIXME - enable this once properties work.
    //image.changed.connect (sigc::mem_fun(this, &Button::OnImageChanged));
    image_position.changed.connect (sigc::mem_fun(this, &Button::OnImagePositionChanged));

    Layout *layout = new HLayout (NUX_TRACKER_LOCATION);
    SetLayout (layout);
    image_position = NUX_POSITION_LEFT;

    RebuildLayout();
  }

  void Button::SetImage (TextureArea *image) {
    this->image = image;
    OnImageChanged (this->image);
  }

  TextureArea *Button::GetImage () {
    return this->image;
  }

  void Button::OnStateChanged (int value) {
    NeedRedraw();
  }

  void Button::OnLabelChanged (std::string value) {
    RebuildLayout ();
  }

  void Button::OnImageChanged (TextureArea *value) {
    RebuildLayout ();
  }

  void Button::OnImagePositionChanged (int value) {
    RebuildLayout ();
  }

  void Button::RebuildLayout () {
    Layout *layout;

    if (image_position == NUX_POSITION_LEFT || image_position == NUX_POSITION_RIGHT) {
      layout = new HLayout (NUX_TRACKER_LOCATION);
    } else {
      layout = new VLayout (NUX_TRACKER_LOCATION);
    }

    PangoText *text = new PangoText(TEXT (label().c_str()));

    if (image_position == NUX_POSITION_LEFT || image_position == NUX_POSITION_TOP) {
      layout->AddView(image, 1, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_MATCHCONTENT);
      layout->AddView(text, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
    } else {
      layout->AddView(text, 1, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_MATCHCONTENT);
      layout->AddView(image, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
    }

    // NOTE - setting the layout here, unreferences the previous one, should cause all the memory
    // to be freed
    SetLayout (layout);

    NeedRedraw();
  }

  void Button::Draw (GraphicsEngine &GfxContext, bool force_draw) {
    Geometry base = GetGeometry();

    //FIXME - nux button theming only supports a few states - low priority really.
    //if (state == NUX_STATE_ACTIVE) {
    if (state == 0) {
      //FIXME - this uses eBUTTON_FOCUS but that's badly named, focus really means "mouse down" or "activated"
      GetPainter().PushDrawSliceScaledTextureLayer (GfxContext, base, eBUTTON_FOCUS, Colors::White, eAllCorners);
      GetPainter().PopBackground();
    //} else if (state == NUX_STATE_PRELIGHT) {
    } else if (state == 1) {
      GetPainter().PushDrawSliceScaledTextureLayer (GfxContext, base, eBUTTON_PRELIGHT, Colors::White, eAllCorners);
      GetPainter().PopBackground();
    } else {
      GetPainter().PushDrawSliceScaledTextureLayer (GfxContext, base, eBUTTON_NORMAL, Colors::White, eAllCorners);
      GetPainter().PopBackground();
    }
  }

  void Button::DrawContent (GraphicsEngine &GfxContent, bool force_draw) {
    nux::Geometry base = GetGeometry ();
    GfxContent.PushClippingRectangle (base);

    if (GetCompositionLayout ())
      GetCompositionLayout ()->ProcessDraw (GfxContent, force_draw);

    GfxContent.PopClippingRectangle();
  }
}
