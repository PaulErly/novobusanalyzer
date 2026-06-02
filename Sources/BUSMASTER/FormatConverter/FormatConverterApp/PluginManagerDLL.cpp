// PluginManager.cpp : implementation file
//

#include "PluginManagerDLL.h"
#include <shlwapi.h>

// CPluginManagerDLL

namespace
{
    bool IsX64ModuleFile(const CString& path)
    {
        CFile file;
        if (!file.Open(path, CFile::modeRead | CFile::shareDenyNone))
        {
            return false;
        }

        const ULONGLONG length = static_cast<ULONGLONG>(file.GetLength());
        if (length < 0x100)
        {
            return false;
        }

        CByteArray buffer;
        buffer.SetSize(static_cast<int>(length));
        if (file.Read(buffer.GetData(), static_cast<UINT>(length)) != length)
        {
            return false;
        }

        const BYTE* bytes = buffer.GetData();
        const DWORD peOffset = *reinterpret_cast<const DWORD*>(bytes + 0x3C);
        if (peOffset + 6 >= length)
        {
            return false;
        }

        const WORD machine = *reinterpret_cast<const WORD*>(bytes + peOffset + 4);
        return machine == IMAGE_FILE_MACHINE_AMD64;
    }
}

typedef HRESULT (*GETCONVERTERINTERFACE)(CBaseConverter*&);

CPluginManagerDLL::CPluginManagerDLL()
{
}

CPluginManagerDLL::~CPluginManagerDLL()
{
}
HRESULT CPluginManagerDLL::LoadConvertersFromFolder(CONST TCHAR* pchPluginFolder)
{
    TCHAR m_acEvalFilePath[MAX_PATH];
    CString strPluginPath;
    if(pchPluginFolder == NULL)
    {
        GetModuleFileName( NULL, m_acEvalFilePath, MAX_PATH );
        PathRemoveFileSpec(m_acEvalFilePath);
        strPluginPath = m_acEvalFilePath;
        strPluginPath += defDEFAULTPLUGINFOLDER;
    }
    else
    {
        strPluginPath = pchPluginFolder;
    }

    if(PathFileExists(strPluginPath) == TRUE)
    {
        SetCurrentDirectory(strPluginPath);

        CFileFind omFileFinder;
        CString strWildCard = defDLLFILEEXTENSION; //look for the plugin files

        BOOL bWorking = omFileFinder.FindFile(strWildCard);
        while (bWorking)
        {
            bWorking = omFileFinder.FindNextFile();
            if (omFileFinder.IsDots() || omFileFinder.IsDirectory())
            {
                continue;
            }
            LoadConverter(omFileFinder.GetFilePath());

        }
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

HRESULT CPluginManagerDLL::LoadConverter(CString& strFileName)
{
    ConverterInfo ConverterInfo;
    if (!IsX64ModuleFile(strFileName))
    {
        TRACE1("Skipping non-x64 converter plugin %s\n", strFileName);
        return S_FALSE;
    }
    ConverterInfo.m_hModule = LoadLibrary(strFileName);
    if ( !ConverterInfo.m_hModule )
    {
        MessageBox(NULL, strFileName, _T("Plugin Loading Error"), MB_OK);
    }
    else
    {
        BOOL bSuccess = TRUE;

        GETCONVERTERINTERFACE pFnBaseConverter;
        pFnBaseConverter = (GETCONVERTERINTERFACE)GetProcAddress(ConverterInfo.m_hModule, defCONVERTERINTERFACE);
        if(pFnBaseConverter != NULL)
        {
            pFnBaseConverter(ConverterInfo.m_pouConverter);
        }
        else
        {
            return S_FALSE;
        }

        //Vlaidation
        if(bSuccess == TRUE)
        {
            m_ConverterList.AddTail(ConverterInfo);
        }
    }
    return S_OK;
}


HRESULT CPluginManagerDLL::UnLoadAllPlugins()
{
    INT_PTR nCount = m_ConverterList.GetCount();
    for(INT_PTR i = 0; i < nCount; i++)
    {
        POSITION pos = m_ConverterList.FindIndex(i);
        ConverterInfo& ouConverterInfo = m_ConverterList.GetAt(pos);

        if(ouConverterInfo.m_pouConverter != NULL)
        {
            delete ouConverterInfo.m_pouConverter;
        }
        if(ouConverterInfo.m_pomPage != NULL)
        {
            delete ouConverterInfo.m_pomPage;
        }
        if( ouConverterInfo.m_hModule != NULL)
        {
            FreeLibrary(ouConverterInfo.m_hModule);
        }
    }
    return S_OK;
}
