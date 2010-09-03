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


#ifndef IOPENGLVOLUME_H
#define IOPENGLVOLUME_H

NAMESPACE_BEGIN_OGL

class IOpenGLResource;
class IOpenGLVolumeTexture;

class IOpenGLVolume: public IOpenGLResource
{
    DECLARE_OBJECT_TYPE(IOpenGLVolume, IOpenGLResource);

public:
    virtual int RefCount() const;

    int LockBox(
        VOLUME_LOCKED_BOX * pLockedVolume,
        const VOLUME_BOX * pBox);

    int UnlockBox();

    BitmapFormat GetPixelFormat() const;
    unsigned int GetWidth() const;
    unsigned int GetHeight() const;
    unsigned int GetDepth() const;

    unsigned int GetMipLevel() const
    {
        return _SMipLevel;
    }

    unsigned int GetSurfaceTarget() const
    {
        return _SSurfaceTarget;
    }

    int GetLevelDesc(VOLUME_DESC * pDesc)
    {
        pDesc->Width    = GetWidth();
        pDesc->Height   = GetHeight();
        pDesc->Depth    = GetDepth();
        pDesc->PixelFormat   = GetPixelFormat();
        pDesc->Type     = _ResourceType;
        return OGL_OK;
    }

private:
    virtual ~IOpenGLVolume();

    unsigned int InitializeLevel();

    IOpenGLVolume(IOpenGLVolumeTexture* VolumeTexture, GLenum OpenGLID, GLenum TextureTarget, GLenum SurfaceTarget, unsigned int MipLevel)
        : _STextureTarget(TextureTarget)
        , _SSurfaceTarget(SurfaceTarget)
        , _SMipLevel(MipLevel)
        , _VolumeTexture(VolumeTexture)
        , IOpenGLResource(RTVOLUME)
        , _AllocatedUnpackBuffer(0xFFFFFFFF)
    {
        // IOpenGLVolume surfaces are created inside a IOpenGLVolumeTexture.
        // They reside within this class. The reference counting starts once a call to GetVolumeLevel is made to the container object.
        _RefCount = 0;
        _OpenGLID = OpenGLID;
        _LockedBox.pBits = 0;
        _LockedBox.RowPitch = 0;
        _CompressedDataSize = 0;
        _Initialized = 0;
    }

    // _STextureTarget may be
    //      GL_TEXTURE_3D
    GLenum      _STextureTarget;

    // _SSurfaceTarget may be
    //      GL_TEXTURE_3D
    GLenum      _SSurfaceTarget;
    unsigned int        _SMipLevel;

    VOLUME_LOCKED_BOX  _LockedBox;
    VOLUME_BOX        _Box;
    unsigned int           _CompressedDataSize;

    IOpenGLVolumeTexture* _VolumeTexture;
    bool           _Initialized;

    int            _AllocatedUnpackBuffer;

    friend class IOpenGLVolumeTexture;
    friend class TRefGL<IOpenGLVolume>;
};

NAMESPACE_END_OGL

#endif // IOPENGLVOLUME_H
