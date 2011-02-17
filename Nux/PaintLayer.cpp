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
#include "NuxGraphics/GraphicsEngine.h"
#include "NuxGraphics/RenderingPipe.h"
#include "Utils.h"
#include "PaintLayer.h"

namespace nux
{

  ColorLayer::ColorLayer (const Color &color, bool write_alpha, const ROPConfig &ROP)
  {
    m_color = color;
    m_write_alpha = write_alpha;
    m_rop = ROP;
  }

  void ColorLayer::Renderlayer (GraphicsEngine &GfxContext)
  {
    t_u32 current_alpha_blend;
    t_u32 current_src_blend_factor;
    t_u32 current_dest_blend_factor;

    // Get the current blend states. They will be restored later.
    GfxContext.GetRenderStates ().GetBlend (current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
    
    GfxContext.GetRenderStates().SetBlend (m_rop.Blend, m_rop.SrcBlend, m_rop.DstBlend);
    GfxContext.QRP_Color (m_geometry.x, m_geometry.y, m_geometry.GetWidth(), m_geometry.GetHeight(), m_color);

    // Restore the blend state
    GfxContext.GetRenderStates ().SetBlend (current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
  }

  AbstractPaintLayer *ColorLayer::Clone() const
  {
    return new ColorLayer(*this);
  }

/////////////////////////////////////////////////////
  ShapeLayer::ShapeLayer (UXStyleImageRef image_style, const Color &color, unsigned long corners, bool write_alpha, const ROPConfig &ROP)
  {
    m_image_style = image_style;
    m_color = color;
    m_write_alpha = write_alpha;
    m_rop = ROP;
    m_rop.Blend = true;
    m_corners = corners;
  }

  void ShapeLayer::Renderlayer (GraphicsEngine &GfxContext)
  {
    t_u32 current_alpha_blend;
    t_u32 current_src_blend_factor;
    t_u32 current_dest_blend_factor;

    // Get the current blend states. They will be restored later.
    GfxContext.GetRenderStates ().GetBlend (current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
    GetPainter().PaintShapeCornerROP (GfxContext, m_geometry, m_color, m_image_style, m_corners, m_write_alpha, m_rop);

    GfxContext.GetRenderStates ().SetBlend (current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
  }

  AbstractPaintLayer *ShapeLayer::Clone() const
  {
    return new ShapeLayer (*this);
  }

/////////////////////////////////////////////////////
  SliceScaledTextureLayer::SliceScaledTextureLayer (UXStyleImageRef image_style, const Color &color, unsigned long corners, bool write_alpha, const ROPConfig &ROP)
  {
    m_image_style = image_style;
    m_color = color;
    m_write_alpha = write_alpha;
    m_rop = ROP;
    m_corners = corners;
  }

  void SliceScaledTextureLayer::Renderlayer (GraphicsEngine &GfxContext)
  {
    GetPainter().PaintTextureShape (GfxContext, m_geometry, m_image_style);
  }

  AbstractPaintLayer *SliceScaledTextureLayer::Clone() const
  {
    return new SliceScaledTextureLayer (*this);
  }

/////////////////////////////////////////////////////
  TextureLayer::TextureLayer (ObjectPtr<IOpenGLBaseTexture> device_texture, TexCoordXForm texxform, const Color &color, bool write_alpha, const ROPConfig &ROP)
  {
    m_device_texture = device_texture;
    m_color = color;
    m_write_alpha = write_alpha;
    m_rop = ROP;
    m_texxform = texxform;
  }

  void TextureLayer::Renderlayer (GraphicsEngine &GfxContext)
  {
    t_u32 current_alpha_blend;
    t_u32 current_src_blend_factor;
    t_u32 current_dest_blend_factor;
    t_u32 current_red_mask;
    t_u32 current_green_mask;
    t_u32 current_blue_mask;
    t_u32 current_alpha_mask;
    
    // Get the current color mask and blend states. They will be restored later.
    GfxContext.GetRenderStates ().GetColorMask (current_red_mask, current_green_mask, current_blue_mask, current_alpha_mask);
    GfxContext.GetRenderStates ().GetBlend (current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);

    
    GfxContext.GetRenderStates ().SetColorMask (GL_TRUE, GL_TRUE, GL_TRUE, m_write_alpha ? GL_TRUE : GL_FALSE);
    GfxContext.GetRenderStates ().SetBlend (m_rop.Blend, m_rop.SrcBlend, m_rop.DstBlend);
    
    GfxContext.QRP_1Tex (m_geometry.x, m_geometry.y, m_geometry.GetWidth(), m_geometry.GetHeight(), m_device_texture,
                              m_texxform, m_color);
    
    // Restore Color mask and blend states.
    GfxContext.GetRenderStates ().SetColorMask (current_red_mask, current_green_mask, current_blue_mask, current_alpha_mask);
    GfxContext.GetRenderStates ().SetBlend (current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
    
  }

  AbstractPaintLayer *TextureLayer::Clone() const
  {
    return new TextureLayer (*this);
  }

  ObjectPtr< IOpenGLBaseTexture> TextureLayer::GetDeviceTexture ()
  {
    return m_device_texture;
  }


}
