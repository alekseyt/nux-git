#include "GLResource.h"
#include "FontTexture.h"
#include "NuxGraphics.h"
#include "IniFile.h"
#include "GLResourceManager.h"

namespace nux
{
    void NuxGraphicsInitialize()
    {

        GNuxGraphicsResources.InitializeResources();
    }

    NUX_IMPLEMENT_GLOBAL_OBJECT(NuxGraphicsResources);

    void NuxGraphicsResources::Constructor()
    {

    }

    void NuxGraphicsResources::Destructor()
    {
        //m_normal_font.Release();
        //m_bold_font.Release();
    }

    void NuxGraphicsResources::InitializeResources()
    {
        FilePath m_FilePath;
        m_FilePath.AddSearchPath(TEXT("")); // for case where fully qualified path is given
        m_FilePath.AddSearchPath(TEXT("./ini"));
        m_FilePath.AddSearchPath(TEXT("../ini"));
        m_FilePath.AddSearchPath(TEXT("../../ini"));
        m_FilePath.AddSearchPath(TEXT("../../../ini"));


        NString file_search = TEXT("inalogic.ini");
        NString FileName = m_FilePath.GetFile(file_search.GetTCharPtr());
        if (FileName == TEXT(""))
        {
            nuxDebugMsg(TEXT("[GLWindowImpl::GLWindowImpl] Can't find inalogic.ini file."));
//             inlWin32MessageBox(NULL, TEXT("Error"), MBTYPE_Ok, MBICON_Error, MBMODAL_ApplicationModal, 
//                 TEXT("Can't find .ini file.\nThe program will exit."));
//             exit(-1);
        }

        if(FileName != TEXT(""))
        {
            NString key_name = TEXT("UseBackupFBO");
            NString section_name = TEXT("OpenGLSystem");

            if(CIniFile::RecordExists(key_name.GetTCharPtr(), section_name.GetTCharPtr(), FileName.GetTCharPtr()) == false)
            {
                nuxDebugMsg(TEXT("[GLWindowImpl::GLWindowImpl] Key [%s] does not exit in .ini file."));
            }
            else
            {
                bool m_BackupFrameBuffer;
                NString value = CIniFile::GetValue(key_name.GetTCharPtr(), section_name.GetTCharPtr(), FileName.GetTCharPtr());
                if((value == TEXT("1")) || (value == TEXT("true")))
                {
                    m_BackupFrameBuffer = true;
                }
                else
                {
                    m_BackupFrameBuffer = false;
                }
            }

            section_name = TEXT("SearchPath");
            key_name = TEXT("FontPath");
            if(CIniFile::RecordExists(key_name.GetTCharPtr(), section_name.GetTCharPtr(), FileName.GetTCharPtr()))
            {
                NString FontPath = CIniFile::GetValue(key_name.GetTCharPtr(), section_name.GetTCharPtr(), FileName.GetTCharPtr());
                FontPath.ParseToArray(m_FontSearchPath, TEXT(";"));
            }
            else
            {
                nuxDebugMsg(TEXT("[GLWindowImpl::GLWindowImpl] Failed to read font search path from .ini file."));
//                 inlWin32MessageBox(NULL, TEXT("Error"), MBTYPE_Ok, MBICON_Error, MBMODAL_ApplicationModal, 
//                     TEXT("Failed to read font search path from .ini file.\nThe program will exit."));
//                 exit(-1);

            }

            key_name = TEXT("UITexturePath");
            if(CIniFile::RecordExists(key_name.GetTCharPtr(), section_name.GetTCharPtr(), FileName.GetTCharPtr()))
            {
                NString UITexturePath = CIniFile::GetValue(key_name.GetTCharPtr(), section_name.GetTCharPtr(), FileName.GetTCharPtr());
                UITexturePath.ParseToArray(m_UITextureSearchPath, TEXT(";"));
            }
            else
            {
                nuxDebugMsg(TEXT("[GLWindowImpl::GLWindowImpl] Failed to read texture search path from .ini file."));
//                 inlWin32MessageBox(NULL, TEXT("Error"), MBTYPE_Ok, MBICON_Error, MBMODAL_ApplicationModal, 
//                     TEXT("Failed to read texture search path from .ini file.\nThe program will exit."));
//                 exit(-1);
            }

            key_name = TEXT("ShaderPath");
            if(CIniFile::RecordExists(key_name.GetTCharPtr(), section_name.GetTCharPtr(), FileName.GetTCharPtr()))
            {
                NString ShaderPath = CIniFile::GetValue(key_name.GetTCharPtr(), section_name.GetTCharPtr(), FileName.GetTCharPtr());
                ShaderPath.ParseToArray(m_ShaderSearchPath, TEXT(";"));
            }
            else
            {
                nuxDebugMsg(TEXT("[GLWindowImpl::GLWindowImpl] Failed to read shader search path from .ini file."));
//                 inlWin32MessageBox(NULL, TEXT("Error"), MBTYPE_Ok, MBICON_Error, MBMODAL_ApplicationModal, 
//                     TEXT("Failed to read shader search path from .ini file.\nThe program will exit."));
//                 exit(-1);
            }
        }

        m_ResourcePathLocation.AddSearchPath(TEXT(""));
        m_ResourcePathLocation.AddSearchPath(TEXT("./"));
        m_ResourcePathLocation.AddSearchPath(TEXT("../"));
        m_ResourcePathLocation.AddSearchPath(TEXT("../../"));
        m_ResourcePathLocation.AddSearchPath(TEXT("./Data"));
        m_ResourcePathLocation.AddSearchPath(TEXT("../Data"));
        m_ResourcePathLocation.AddSearchPath(TEXT("../../Data"));
        m_ResourcePathLocation.AddSearchPath(TEXT("../../../Data"));

        m_ResourcePathLocation.AddSearchPath(m_FontSearchPath);
        m_ResourcePathLocation.AddSearchPath(m_ShaderSearchPath);
        m_ResourcePathLocation.AddSearchPath(m_UITextureSearchPath);

        if(m_normal_font == 0)
            m_normal_font = IntrusiveSP<FontTexture>(new FontTexture(FindResourceLocation(TEXT("Tahoma_size_8.txt"), true).GetTCharPtr()));
        if(m_bold_font == 0)
            m_bold_font = IntrusiveSP<FontTexture>(new FontTexture(FindResourceLocation(TEXT("Tahoma_size_8_bold.txt"), true).GetTCharPtr()));
    }

    IntrusiveSP<FontTexture> NuxGraphicsResources::GetFont()
    {
        return m_normal_font;
    }

    IntrusiveSP<FontTexture> NuxGraphicsResources::GetBoldFont()
    {
        return m_bold_font;
    }

    void NuxGraphicsResources::CacheFontTextures(NResourceCache& RsrcCache)
    {
        std::vector<NTexture2D*>::iterator it;

        for(it = m_normal_font->TextureArray.begin(); it != m_normal_font->TextureArray.end(); it++)
        {
            RsrcCache.GetCachedResource((NResource*)(*it)/*NUX_STATIC_CAST(NResource*, *it)*/);
        }

        for(it = m_bold_font->TextureArray.begin(); it != m_bold_font->TextureArray.end(); it++)
        {
            RsrcCache.GetCachedResource((NResource*)(*it)/*NUX_STATIC_CAST(NResource*, *it)*/);
        }
    }

#ifdef NUX_OS_WINDOWS
    NString NuxGraphicsResources::FindResourceLocation(const TCHAR* ResourceFileName, bool ErrorOnFail)
    {
        NString path = m_ResourcePathLocation.GetFile(ResourceFileName);

        if(path == TEXT("") && ErrorOnFail)
        {
            nuxDebugMsg(TEXT("[NuxGraphicsResources::FindResourceLocation] Failed to locate resource file: %s."), ResourceFileName);
            inlWin32MessageBox(NULL, TEXT("Error"), MBTYPE_Ok, MBICON_Error, MBMODAL_ApplicationModal, 
                TEXT("Failed to locate resource file %s.\nThe program will exit."), ResourceFileName);
            exit(1);
        }
        return path;
    }

    NString NuxGraphicsResources::FindUITextureLocation(const TCHAR* ResourceFileName, bool ErrorOnFail)
    {
        FilePath searchpath;
        searchpath.AddSearchPath(m_UITextureSearchPath);
        NString path = searchpath.GetFile(ResourceFileName);

        if((path == TEXT("")) && ErrorOnFail)
        {
            nuxDebugMsg(TEXT("[NuxGraphicsResources::FindResourceLocation] Failed to locate ui texture file: %s."), ResourceFileName);
            inlWin32MessageBox(NULL, TEXT("Error"), MBTYPE_Ok, MBICON_Error, MBMODAL_ApplicationModal, 
                TEXT("Failed to locate ui texture file %s.\nThe program will exit."), ResourceFileName);
            exit(1);
        }
        return path;
    }

    NString NuxGraphicsResources::FindShaderLocation(const TCHAR* ResourceFileName, bool ErrorOnFail)
    {
        FilePath searchpath;
        searchpath.AddSearchPath(m_ShaderSearchPath);
        NString path = searchpath.GetFile(ResourceFileName);

        if((path == TEXT("")) && ErrorOnFail)
        {
            nuxDebugMsg(TEXT("[NuxGraphicsResources::FindResourceLocation] Failed to locate shader file: %s."), ResourceFileName);
            inlWin32MessageBox(NULL, TEXT("Error"), MBTYPE_Ok, MBICON_Error, MBMODAL_ApplicationModal, 
                TEXT("Failed to locate shader file %s.\nThe program will exit."), ResourceFileName);
            exit(1);
        }
        return path;
    }

    NString NuxGraphicsResources::FindFontLocation(const TCHAR* ResourceFileName, bool ErrorOnFail)
    {
        FilePath searchpath;
        searchpath.AddSearchPath(m_FontSearchPath);
        NString path = searchpath.GetFile(ResourceFileName);

        if((path == TEXT("")) && ErrorOnFail)
        {
            nuxDebugMsg(TEXT("[NuxGraphicsResources::FindResourceLocation] Failed to locate font file file: %s."), ResourceFileName);
            inlWin32MessageBox(NULL, TEXT("Error"), MBTYPE_Ok, MBICON_Error, MBMODAL_ApplicationModal, 
                TEXT("Failed to locate font file %s.\nThe program will exit."), ResourceFileName);
            exit(1);
        }
        return path;
    }
#elif defined(NUX_OS_LINUX)
    NString NuxGraphicsResources::FindResourceLocation(const TCHAR* ResourceFileName, bool ErrorOnFail)
    {
        NString path = m_ResourcePathLocation.GetFile(ResourceFileName);

        if(path == TEXT("") && ErrorOnFail)
        {
            nuxDebugMsg(TEXT("[NuxGraphicsResources::FindResourceLocation] Failed to locate resource file: %s."), ResourceFileName);
            /*inlWin32MessageBox(NULL, TEXT("Error"), MBTYPE_Ok, MBICON_Error, MBMODAL_ApplicationModal, 
                TEXT("Failed to locate resource file %s.\nThe program will exit."), ResourceFileName);*/
            exit(1);
        }
        return path;
    }

    NString NuxGraphicsResources::FindUITextureLocation(const TCHAR* ResourceFileName, bool ErrorOnFail)
    {
        FilePath searchpath;
        searchpath.AddSearchPath(m_UITextureSearchPath);
        NString path = searchpath.GetFile(ResourceFileName);

        if((path == TEXT("")) && ErrorOnFail)
        {
            nuxDebugMsg(TEXT("[NuxGraphicsResources::FindResourceLocation] Failed to locate ui texture file: %s."), ResourceFileName);
            /*inlWin32MessageBox(NULL, TEXT("Error"), MBTYPE_Ok, MBICON_Error, MBMODAL_ApplicationModal, 
                TEXT("Failed to locate ui texture file %s.\nThe program will exit."), ResourceFileName);*/
            exit(1);
        }
        return path;
    }

    NString NuxGraphicsResources::FindShaderLocation(const TCHAR* ResourceFileName, bool ErrorOnFail)
    {
        FilePath searchpath;
        searchpath.AddSearchPath(m_ShaderSearchPath);
        NString path = searchpath.GetFile(ResourceFileName);

        if((path == TEXT("")) && ErrorOnFail)
        {
            nuxDebugMsg(TEXT("[NuxGraphicsResources::FindResourceLocation] Failed to locate shader file: %s."), ResourceFileName);
            /*inlWin32MessageBox(NULL, TEXT("Error"), MBTYPE_Ok, MBICON_Error, MBMODAL_ApplicationModal, 
                TEXT("Failed to locate shader file %s.\nThe program will exit."), ResourceFileName);*/
            exit(1);
        }
        return path;
    }

    NString NuxGraphicsResources::FindFontLocation(const TCHAR* ResourceFileName, bool ErrorOnFail)
    {
        FilePath searchpath;
        searchpath.AddSearchPath(m_FontSearchPath);
        NString path = searchpath.GetFile(ResourceFileName);

        if((path == TEXT("")) && ErrorOnFail)
        {
            nuxDebugMsg(TEXT("[NuxGraphicsResources::FindResourceLocation] Failed to locate font file file: %s."), ResourceFileName);
            /*inlWin32MessageBox(NULL, TEXT("Error"), MBTYPE_Ok, MBICON_Error, MBMODAL_ApplicationModal, 
                TEXT("Failed to locate font file %s.\nThe program will exit."), ResourceFileName);*/
            exit(1);
        }
        return path;
    }
#endif

}