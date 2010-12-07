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


#ifndef GLDEVICEFACTORY_H
#define GLDEVICEFACTORY_H

#include "GLResource.h"
#include "GLDeviceFrameBufferObject.h"
#include "GLDeviceObjects.h"
#include "GLRenderStates.h"
#include "GLTextureStates.h"
#include "GLTemplatePrimitiveBuffer.h"

namespace nux
{
  template<typename T> class IntrusiveSP;

  struct STREAMSOURCE
  {
    WORD Stream;
    IntrusiveSP<IOpenGLVertexBuffer> VertexBuffer;
    t_u16 StreamOffset;
    t_u16 StreamStride;

    STREAMSOURCE()
    {
      Stream = 0;
      //VertexBuffer = 0;
      StreamOffset = 0;
      StreamStride = 0;
    }

    void ResetStreamSource()
    {
      Stream = 0;
      StreamOffset = 0;
      VertexBuffer = IntrusiveSP<IOpenGLVertexBuffer> (0);
      StreamStride = 0;
    }
  };

  // GPU Graphics information.
  class GpuInfo
  {
  public:

    GpuInfo();

    bool SupportOpenGL11() const    {return _support_opengl_version_11;}
    bool SupportOpenGL12() const    {return _support_opengl_version_12;}
    bool SupportOpenGL13() const    {return _support_opengl_version_13;}
    bool SupportOpenGL14() const    {return _support_opengl_version_14;}
    bool SupportOpenGL15() const    {return _support_opengl_version_15;}
    bool SupportOpenGL20() const    {return _support_opengl_version_20;}
    bool SupportOpenGL21() const    {return _support_opengl_version_21;}
    bool SupportOpenGL30() const    {return _support_opengl_version_30;}
    bool SupportOpenGL31() const    {return _support_opengl_version_31;}
    bool SupportOpenGL33() const    {return _support_opengl_version_33;}
    bool SupportOpenGL32() const    {return _support_opengl_version_32;}
    bool SupportOpenGL40() const    {return _support_opengl_version_40;}
    bool SupportOpenGL41() const    {return _support_opengl_version_41;}

    bool Support_EXT_Swap_Control()             const    {return _support_ext_swap_control;}
    bool Support_ARB_Texture_Rectangle()        const    {return _support_arb_texture_rectangle;}
    bool Support_ARB_Vertex_Program()           const    {return _support_arb_vertex_program;}
    bool Support_ARB_Fragment_Program()         const    {return _support_arb_fragment_program;}
    bool Support_ARB_Shader_Objects()           const    {return _support_arb_shader_objects;}
    bool Support_ARB_Vertex_Shader()            const    {return _support_arb_vertex_shader;}
    bool Support_ARB_Fragment_Shader()          const    {return _support_arb_fragment_shader;}
    bool Support_ARB_Vertex_Buffer_Object()     const    {return _support_arb_vertex_buffer_object;}
    bool Support_ARB_Texture_Non_Power_Of_Two() const    {return _support_arb_texture_non_power_of_two;}
    bool Support_EXT_Framebuffer_Object()       const    {return _support_ext_framebuffer_object;}
    bool Support_EXT_Draw_Range_Elements()      const    {return _support_ext_draw_range_elements;}
    bool Support_EXT_Stencil_Two_Side()         const    {return _support_ext_stencil_two_side;}
    bool Support_EXT_Texture_Rectangle()        const    {return _support_ext_texture_rectangle;}
    bool Support_NV_Texture_Rectangle()         const    {return _support_nv_texture_rectangle;}

    int GetMaxFboAttachment () {return _opengl_max_fb_attachment;}


  private:
    void Setup ();

    bool _support_opengl_version_11;
    bool _support_opengl_version_12;
    bool _support_opengl_version_13;
    bool _support_opengl_version_14;
    bool _support_opengl_version_15;
    bool _support_opengl_version_20;
    bool _support_opengl_version_21;
    bool _support_opengl_version_30;
    bool _support_opengl_version_31;
    bool _support_opengl_version_32;
    bool _support_opengl_version_33;
    bool _support_opengl_version_40;
    bool _support_opengl_version_41;

    int _opengl_max_texture_units;
    int _opengl_max_texture_coords;
    int _opengl_max_texture_image_units;
    int _opengl_max_fb_attachment;
    int _opengl_max_vertex_attributes;

    bool _support_ext_swap_control;
    bool _support_arb_vertex_program;
    bool _support_arb_fragment_program;
    bool _support_arb_shader_objects;
    bool _support_arb_vertex_shader;
    bool _support_arb_fragment_shader;
    bool _support_arb_vertex_buffer_object;
    bool _support_arb_texture_non_power_of_two;
    bool _support_ext_framebuffer_object;
    bool _support_ext_draw_range_elements;
    bool _support_ext_stencil_two_side;
    bool _support_ext_texture_rectangle;
    bool _support_arb_texture_rectangle; //!< Promoted from GL_EXT_TEXTURE_RECTANGLE to ARB.
    bool _support_nv_texture_rectangle;

    friend class GpuDevice;
  };

  class GpuDevice
  {
  private:
    static STREAMSOURCE _StreamSource[MAX_NUM_STREAM];

    int CreateTexture (
      unsigned int Width
      , unsigned int Height
      , unsigned int Levels
      , BitmapFormat PixelFormat
      , IOpenGLTexture2D **ppTexture
    );

    int CreateRectangleTexture (
      unsigned int Width
      , unsigned int Height
      , unsigned int Levels
      , BitmapFormat PixelFormat
      , IOpenGLRectangleTexture **ppTexture
    );

    int CreateCubeTexture (
      unsigned int EdgeLength
      , unsigned int Levels
      , BitmapFormat PixelFormat
      , IOpenGLCubeTexture **ppCubeTexture
    );

    int CreateVolumeTexture (
      unsigned int Width
      , unsigned int Height
      , unsigned int Depth
      , unsigned int Levels
      , BitmapFormat PixelFormat
      , IOpenGLVolumeTexture **ppVolumeTexture
    );

    int CreateAnimatedTexture (
      unsigned int Width
      , unsigned int Height
      , unsigned int Depth
      , BitmapFormat PixelFormat
      , IOpenGLAnimatedTexture **ppAnimatedTexture
    );

    int CreateVertexBuffer (
      unsigned int Length
      , VBO_USAGE Usage    // Dynamic or WriteOnly
      , IOpenGLVertexBuffer **ppVertexBuffer
    );

    int CreateIndexBuffer (
      unsigned int Length
      , VBO_USAGE Usage    // Dynamic or WriteOnly
      , INDEX_FORMAT Format
      , IOpenGLIndexBuffer **ppIndexBuffer
    );

    int CreatePixelBufferObject (int Size, VBO_USAGE Usage,   // Dynamic or WriteOnly
                                 IOpenGLPixelBufferObject **ppPixelBufferObject
                                );

    int CreateQuery (
      QUERY_TYPE Type,
      IOpenGLQuery **ppQuery);

    int CreateVertexDeclaration (
      const VERTEXELEMENT *pVertexElements,
      IOpenGLVertexDeclaration **ppDecl);

    int CreateFrameBufferObject (
      IOpenGLFrameBufferObject **ppFrameBufferObject);

    int CreateShaderProgram (
      IOpenGLShaderProgram **ppShaderProgram);

    int CreateVertexShader (
      IOpenGLVertexShader **ppVertexShader);

    int CreatePixelShader (
      IOpenGLPixelShader **ppPixelShader);

    int CreateAsmShaderProgram (
      IOpenGLAsmShaderProgram **ppAsmShaderProgram);

    int CreateAsmVertexShader (
      IOpenGLAsmVertexShader **ppAsmVertexShader);

    int CreateAsmPixelShader (
      IOpenGLAsmPixelShader **ppAsmPixelShader);

#if (NUX_ENABLE_CG_SHADERS)
    int CreateCGVertexShader (
      ICgVertexShader **ppCgVertexShader);

    int CreateCGPixelShader (
      ICgPixelShader **ppCgPixelShader);
#endif

  public:
    IntrusiveSP<IOpenGLTexture2D> CreateTexture (
      int Width
      , int Height
      , int Levels
      , BitmapFormat PixelFormat);

    IntrusiveSP<IOpenGLRectangleTexture> CreateRectangleTexture (
      int Width
      , int Height
      , int Levels
      , BitmapFormat PixelFormat);

    IntrusiveSP<IOpenGLCubeTexture> CreateCubeTexture (
      int EdgeLength
      , int Levels
      , BitmapFormat PixelFormat);

    IntrusiveSP<IOpenGLVolumeTexture> CreateVolumeTexture (
      int Width
      , int Height
      , int Depth
      , int Levels
      , BitmapFormat PixelFormat);

    IntrusiveSP<IOpenGLAnimatedTexture> CreateAnimatedTexture (
      int Width
      , int Height
      , int Depth
      , BitmapFormat PixelFormat);

    IntrusiveSP<IOpenGLVertexBuffer> CreateVertexBuffer (
      int Length
      , VBO_USAGE Usage);

    IntrusiveSP<IOpenGLIndexBuffer> CreateIndexBuffer (
      int Length
      , VBO_USAGE Usage    // Dynamic or WriteOnly
      , INDEX_FORMAT Format);

    IntrusiveSP<IOpenGLPixelBufferObject> CreatePixelBufferObject (int Size, VBO_USAGE Usage);

    IntrusiveSP<IOpenGLQuery> CreateQuery (
      QUERY_TYPE Type);

    IntrusiveSP<IOpenGLVertexDeclaration> CreateVertexDeclaration (
      const VERTEXELEMENT *pVertexElements);

    IntrusiveSP<IOpenGLFrameBufferObject> CreateFrameBufferObject();

    IntrusiveSP<IOpenGLShaderProgram> CreateShaderProgram();
    IntrusiveSP<IOpenGLVertexShader> CreateVertexShader();
    IntrusiveSP<IOpenGLPixelShader> CreatePixelShader();
    IntrusiveSP<IOpenGLAsmShaderProgram> CreateAsmShaderProgram();
    IntrusiveSP<IOpenGLAsmVertexShader> CreateAsmVertexShader();
    IntrusiveSP<IOpenGLAsmPixelShader> CreateAsmPixelShader();

#if (NUX_ENABLE_CG_SHADERS)
    IntrusiveSP<ICgVertexShader> CreateCGVertexShader();
    IntrusiveSP<ICgPixelShader> CreateCGPixelShader();
#endif

    // This is for the fixed pipeline
    // When using shaders see how a shader sampler parameter links to a texture with a call to setTexture.
    int SetTexture (unsigned int TextureUnit, IOpenGLBaseTexture *texture);


    void DrawQuad_FixPipe (int x, int y, int width, int height,
                           FLOAT R, FLOAT G, FLOAT B, FLOAT A);

    void DrawTextureQuad_FixPipe (IntrusiveSP<IOpenGLTexture2D> texture, int x, int y, int width, int height,
                                  FLOAT u0, FLOAT v0, FLOAT u1, FLOAT v1);

    void DrawTextureQuad_FixPipe (IntrusiveSP<IOpenGLSurface> surface, int x, int y, int width, int height,
                                  FLOAT u0, FLOAT v0, FLOAT u1, FLOAT v1);

    int DrawIndexedPrimitive (
      IntrusiveSP<IOpenGLIndexBuffer> IndexBuffer,
      IntrusiveSP<IOpenGLVertexDeclaration> VertexDeclaration,
      PRIMITIVE_TYPE PrimitiveType,
      int BaseVertexIndex,
      int MinIndex,
      int NumVertices,
      int StartIndex,
      int PrimitiveCount
    );

    // Draw Primitive without index buffer
    int DrawPrimitive (
      IntrusiveSP<IOpenGLVertexDeclaration> VertexDeclaration,
      PRIMITIVE_TYPE pt_,
      unsigned vtx_start_,
      unsigned num_prims_);

    // Draw Primitive without index buffer, and use a user pointer for the source of the stream.
    int DrawPrimitiveUP (
      IntrusiveSP<IOpenGLVertexDeclaration> VertexDeclaration,
      PRIMITIVE_TYPE PrimitiveType,
      unsigned int PrimitiveCount,
      const void *pVertexStreamZeroData,
      unsigned int VertexStreamZeroStride
    );

    int SetStreamSource (
      unsigned int StreamNumber,
      IntrusiveSP<IOpenGLVertexBuffer> pStreamData,
      unsigned int OffsetInBytes,
      unsigned int Stride,
      unsigned int NumComponent = 0,
      unsigned int ComponentFormat = 0);

    //! Setup a NULL vertex buffer
    void InvalidateVertexBuffer();
    //! Setup a NULL index buffer
    void InvalidateIndexBuffer();
    //! Setup a NULL texture
    void InvalidateTextureUnit (int TextureUnitIndex);

    unsigned int GetPixelStoreAlignment()
    {
      return _PixelStoreAlignment;
    }

    int AllocateUnpackPixelBufferIndex (int *index);
    int FreeUnpackPixelBufferIndex (const int index);
    int BindUnpackPixelBufferIndex (const int index);
    int BindPackPixelBufferIndex (const int index);
    void *LockUnpackPixelBufferIndex (const int index, const int Size);
    void *LockPackPixelBufferIndex (const int index, const int Size);

    void UnlockUnpackPixelBufferIndex (const int index);
    void UnlockPackPixelBufferIndex (const int index);

    // Operations on the current render target.
    void Clear (FLOAT red, FLOAT green, FLOAT blue, FLOAT alpha, FLOAT depth, int stencil);
    void ClearColorRT (FLOAT red, FLOAT green, FLOAT blue, FLOAT alpha);
    void ClearDepthRT (FLOAT depth);
    void ClearStencilRT (int stencil);
    void ClearFloatingPointColorRT (int x, int y, int width, int height,
                                    FLOAT red, FLOAT green, FLOAT blue, FLOAT alpha); // use a Quad.
    void ClearSurfaceWithColor (IntrusiveSP<IOpenGLSurface> s_, const SURFACE_RECT *rect_, float r, float g, float b, float a);

    // All these operations are done on the default frame buffer object: _FrameBufferObject.
    int FormatFrameBufferObject (unsigned int Width, unsigned int Height, BitmapFormat PixelFormat);
    int SetColorRenderTargetSurface (unsigned int ColorAttachmentIndex, IntrusiveSP<IOpenGLSurface> pRenderTargetSurface);
    int SetDepthRenderTargetSurface (IntrusiveSP<IOpenGLSurface> pDepthSurface);
    IntrusiveSP<IOpenGLSurface> GetColorRenderTargetSurface (unsigned int ColorAttachmentIndex);
    IntrusiveSP<IOpenGLSurface> GetDepthRenderTargetSurface();
    // Activate and Deactivate the default framebuffer: _FrameBufferObject.
    void ActivateFrameBuffer();
    void DeactivateFrameBuffer();

  public:
    void SetCurrentFrameBufferObject (IntrusiveSP<IOpenGLFrameBufferObject> fbo); // {_CurrentFrameBufferObject = fbo;}
    IntrusiveSP<IOpenGLFrameBufferObject> GetCurrentFrameBufferObject(); // {return _CurrentFrameBufferObject;}

    void CollectDeviceResource();

  private:
    // Default FrameBufferobject
    IntrusiveSP<IOpenGLFrameBufferObject> _FrameBufferObject;
    IntrusiveSP<IOpenGLFrameBufferObject> _CurrentFrameBufferObject;

    struct PixelBufferObject
    {
      IntrusiveSP<IOpenGLPixelBufferObject> PBO;
      bool   IsReserved;
    };

    unsigned int _PixelStoreAlignment;

    std::vector<PixelBufferObject> _PixelBufferArray;

  private:
    int _DeviceWidth;
    int _DeviceHeight;
    int _ViewportX;
    int _ViewportY;
    int _ViewportWidth;
    int _ViewportHeight;

  public:


#if (NUX_ENABLE_CG_SHADERS)
    CGcontext GetCgContext()
    {
      return m_Cgcontext;
    }
    CGcontext m_Cgcontext;
#endif

    inline bool UsePixelBufferObjects() const
    {
      return _UsePixelBufferObject;
    }
    GpuBrand GetGPUBrand() const
    {
      return _gpu_brand;
    }

    GpuRenderStates &GetRenderStates()
    {
      return *_gpu_render_states;
    }

    GpuInfo &GetGpuInfo()
    {
      return *_gpu_info;
    }

    void ResetRenderStates()
    {
      _gpu_render_states->ResetStateChangeToDefault();
    }
    void VerifyRenderStates()
    {
      _gpu_render_states->CheckStateChange();
    }

    //! Create a texture that the system supports. Rectangle texture or 2D texture.
    /*!
      @Width        Texture width.
      @Height       Texture height.
      @Levels       Texture number of mipmaps. If 0, all the mipmaps levels are created
      @PixelFormat  Texture format.
      @return       A device texture. Depending on the system capabilities returns a IntrusiveSP<IOpenGLTexture2D> or IntrusiveSP<IOpenGLRectangleTexture>.
    */
    IntrusiveSP<IOpenGLBaseTexture> CreateSystemCapableDeviceTexture (
      int Width
      , int Height
      , int Levels
      , BitmapFormat PixelFormat);

    //! Created a cached texture
    /*!
      @return A cached texture. Depending on the system capabilities, returns a Texture2D or TextureRectangle.
    */
    BaseTexture* CreateSystemCapableTexture ();

    bool SUPPORT_GL_ARB_TEXTURE_NON_POWER_OF_TWO() const
    {
      return _gpu_info->Support_ARB_Texture_Non_Power_Of_Two ();
    }

    bool SUPPORT_GL_EXT_TEXTURE_RECTANGLE()    const
    {
      return _gpu_info->Support_EXT_Texture_Rectangle ();
    }

    bool SUPPORT_GL_ARB_TEXTURE_RECTANGLE()    const
    {
      return _gpu_info->Support_ARB_Texture_Rectangle ();
    }
    
  private:

    // 
    int _opengl_major;  //!< OpenGL major version.
    int _opengl_minor;  //!< OpenGL minor version.
    int _glsl_version_major;  //!< GLSL major version.
    int _glsl_version_minor;  //!< GLSL major version.

    NString _board_vendor_string;     //!< GPU vendor sting.
    NString _board_renderer_string;   //!< GPU renderer sting.
    NString _openGL_version_string;   //!< OpenGL version string.
    NString _glsl_version_string;     //!< GLSL version string.
    GpuBrand _gpu_brand;              //!< GPU brand.

    bool _UsePixelBufferObject;

    bool OGL_EXT_SWAP_CONTROL;
    bool GL_ARB_VERTEX_PROGRAM;
    bool GL_ARB_FRAGMENT_PROGRAM;
    bool GL_ARB_SHADER_OBJECTS;
    bool GL_ARB_VERTEX_SHADER;
    bool GL_ARB_FRAGMENT_SHADER;
    bool GL_ARB_VERTEX_BUFFER_OBJECT;
    bool GL_ARB_TEXTURE_NON_POWER_OF_TWO;
    bool GL_EXT_FRAMEBUFFER_OBJECT;
    bool GL_EXT_DRAW_RANGE_ELEMENTS;
    bool GL_EXT_STENCIL_TWO_SIDE;
    bool GL_EXT_TEXTURE_RECTANGLE;
    bool GL_ARB_TEXTURE_RECTANGLE; //!< Promoted from GL_EXT_TEXTURE_RECTANGLE to ARB.
    bool GL_NV_TEXTURE_RECTANGLE;

    GpuRenderStates *_gpu_render_states;
    GpuInfo *_gpu_info;

  public:
    
#if defined (NUX_OS_WINDOWS)
    GpuDevice (unsigned int DeviceWidth, unsigned int DeviceHeight, BitmapFormat DeviceFormat,
      HDC device_context,
      HGLRC &opengl_rendering_context,
      int req_opengl_major = 1,   // requested opengl major version.
      int req_opengl_minor = 0,   // requested opengl minor version.
      bool opengl_es_20 = false);

#elif defined (NUX_OS_LINUX)
    GpuDevice (t_u32 DeviceWidth, t_u32 DeviceHeight, BitmapFormat DeviceFormat,
      Display *display,
      Window window,
      bool has_new_glx_support,
      GLXFBConfig fb_config,
      GLXContext &opengl_rendering_context,
      int req_opengl_major = 1,   // requested opengl major version.
      int req_opengl_minor = 0,   // requested opengl minor version.
      bool opengl_es_20 = false);
#endif
    
    ~GpuDevice();
    friend class IOpenGLSurface;
    friend class GraphicsEngine;
  };

}

#endif // GLDEVICEFACTORY_H

