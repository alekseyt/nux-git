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


#ifndef ABSTRACTPAINTLAYER_H
#define ABSTRACTPAINTLAYER_H

namespace nux
{

  class AbstractPaintLayer
  {
  public:

    enum PaintLayerType
    {
      PaintLayer_Color,
      PaintLayer_Shape,
      PaintLayer_ShapeROP,
      PaintLayer_Texture,
      PaintLayer_DeviceTexture,
      PaintLayer_ImageStyle,
    };

    AbstractPaintLayer ();
    virtual ~AbstractPaintLayer ();
    virtual AbstractPaintLayer *Clone () const = 0; //Virtual Constructor Idiom

    virtual void Renderlayer (GraphicsEngine &GfxContext) = 0;

    virtual void SetGeometry (const Geometry &geo);

    //! Preserve the model view matrix at the moment layer is pushed on the paint layer stack. See Painter class.
    void SetModelViewMatrix (const Matrix4 &mat);

    Matrix4 GetModelViewMatrix ();

    Geometry GetGeometry() const;

  protected:
    Geometry _geometry;
    Matrix4  _model_view_matrix;
  };

}

#endif // ABSTRACTPAINTLAYER_H

