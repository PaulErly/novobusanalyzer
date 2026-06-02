// FormatConverterWnd.cpp : implementation file
//


#include "FormatConverterWnd.h"
#include "DefConverterPage.h"
#include <HtmlHelp.h>
#include <locale.h>

namespace
{
    class CScopedResourceHandle
    {
    public:
        explicit CScopedResourceHandle(HINSTANCE hNewResource)
            : m_hOldResource(AfxGetResourceHandle())
        {
            AfxSetResourceHandle(hNewResource);
        }

        ~CScopedResourceHandle()
        {
            AfxSetResourceHandle(m_hOldResource);
        }

    private:
        HINSTANCE m_hOldResource;
    };
}
// CFormatConverterWnd

IMPLEMENT_DYNAMIC(CFormatConverterWnd, CPropertySheet)

CFormatConverterWnd::CFormatConverterWnd(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
    :CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
    m_pomDefConverterPage = NULL;
}

CFormatConverterWnd::CFormatConverterWnd(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
    :CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
    m_pomDefConverterPage = NULL;
}

CFormatConverterWnd::~CFormatConverterWnd()
{
    if( m_pouPluginManager != NULL)
    {
        m_pouPluginManager->UnLoadAllPlugins();
        delete m_pouPluginManager;
    }
    if(m_pomDefConverterPage != NULL)
    {
        delete m_pomDefConverterPage;
    }
}

void CFormatConverterWnd::vGettextBusmaster(void)
{
    setlocale(LC_ALL,"");
    bindtextdomain("BUSMASTER", getenv("LOCALDIR") );
    textdomain("BUSMASTER");
}

BEGIN_MESSAGE_MAP(CFormatConverterWnd, CPropertySheet)
    ON_WM_CREATE()
    ON_WM_SHOWWINDOW()
    ON_COMMAND ( ID_HELP, &CFormatConverterWnd::OnHelp )
END_MESSAGE_MAP()


// CFormatConverterWnd message handlers

BOOL CFormatConverterWnd::OnInitDialog()
{
    CScopedResourceHandle resourceScope(AfxGetInstanceHandle());
    BOOL bResult = CPropertySheet::OnInitDialog();

    SetIcon( AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE);

    // TODO:  Add your specialized code here
    ModifyStyle(0, WS_MINIMIZEBOX);
    CButton* omBtn;
    WINDOWPLACEMENT omWndPlace;

    vGettextBusmaster();

    omBtn = reinterpret_cast<CButton*>(GetDlgItem(ID_APPLY_NOW));
    if (omBtn != nullptr)
    {
        omBtn->ShowWindow(SW_HIDE);
        omBtn->GetWindowPlacement(&omWndPlace);
    }

    omBtn = reinterpret_cast<CButton*>(GetDlgItem(IDOK));
    if (omBtn != nullptr)
    {
        omBtn->ShowWindow(SW_HIDE);
    }



    omBtn = reinterpret_cast<CButton*>(GetDlgItem(IDCANCEL));
    if (omBtn != nullptr)
    {
        omBtn->SetWindowText(_("Close"));
        omBtn->SetWindowPlacement(&omWndPlace);
    }
    else
    {
        TRACE0("Format Converter dialog template is missing IDCANCEL.\n");
    }

    if (!m_bConvertersLoaded)
    {
        TRACE0("Format Converter will load converter tabs after the window is shown.\n");
    }
    return bResult;
}

void CFormatConverterWnd::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CPropertySheet::OnShowWindow(bShow, nStatus);
    if (bShow && !m_bConvertersLoaded)
    {
        m_bConvertersLoaded = (LoadConverters() == S_OK);
        if (m_bConvertersLoaded)
        {
            TRACE0("Format Converter tabs loaded after the window became visible.\n");
        }
        else
        {
            TRACE0("Format Converter did not load any converter tabs.\n");
        }
    }
}

int CFormatConverterWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CPropertySheet::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }

    // TODO:  Add your specialized creation code here

    return 0;
}

void CFormatConverterWnd::OnHelp ()
{
    TCHAR szPath[MAX_PATH] = {};
    if (GetModuleFileName(nullptr, szPath, MAX_PATH) == 0)
    {
        AfxMessageBox(_("Failed to launch help."), MB_ICONWARNING | MB_OK);
        return;
    }

    PathRemoveFileSpec(szPath);
    CString omStrPath = CString(szPath) + _T("\\BUSMASTER.chm::/topics/format_converters.html");
    
    // Make it as content display always
    if (::HtmlHelp(nullptr, omStrPath, HH_DISPLAY_TOPIC, 0) == nullptr)
    {
        AfxMessageBox(_("Failed to launch help."), MB_ICONWARNING | MB_OK);
    }
}

BOOL CFormatConverterWnd::Create(CWnd* pParentWnd , DWORD dwStyle , DWORD dwExStyle)
{
    // TODO: Add your specialized code here and/or call the base class

    CPropertySheet::Create(pParentWnd, dwStyle, dwExStyle);
    //SetActivePage(nCount);
    SetActivePage(1);
    return TRUE;
}


HRESULT CFormatConverterWnd::LoadConverters()
{
    CScopedResourceHandle resourceScope(AfxGetInstanceHandle());
    if (m_bConvertersLoaded)
    {
        return S_OK;
    }
    m_pouPluginManager = new CPluginManagerDLL();
    m_pouPluginManager->LoadConvertersFromFolder();

    INT nCount = 0;

    //m_pomDefConverterPage->m_psp.dwFlags &= ~PSP_HIDEHEADER;

    for(INT i = 0; i < m_pouPluginManager->m_ConverterList.GetCount(); i++)
    {
        POSITION pos = m_pouPluginManager->m_ConverterList.FindIndex(i);
        ConverterInfo& ouConverterInfo = m_pouPluginManager->m_ConverterList.GetAt(pos);
        if(ouConverterInfo.m_pouConverter->bHaveOwnWindow() == TRUE)
        {
            //CPropertyPage *pPage = NULL;
            ouConverterInfo.m_pouConverter->GetPropertyPage(ouConverterInfo.m_pomPage);
            if(ouConverterInfo.m_pomPage != NULL)
            {
                ouConverterInfo.m_pomPage->m_psp.dwFlags &= ~PSP_HASHELP;
                AddPage(ouConverterInfo.m_pomPage);
                nCount++;
            }
        }
    }
    m_pomDefConverterPage = new CDefConverterPage(nCount);
    m_pomDefConverterPage->SetPluginManager(m_pouPluginManager);
    m_pomDefConverterPage->m_psp.dwFlags &= ~PSP_HASHELP;
    AddPage(m_pomDefConverterPage);

    TRACE1("Format Converter loaded %d converter tabs.\n", nCount);
    m_bConvertersLoaded = true;

    return S_OK;

}

BOOL CFormatConverterWnd::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: Add your specialized code here and/or call the base class
    cs.dwExStyle = cs.dwExStyle | WS_EX_TOOLWINDOW;
    return CPropertySheet::PreCreateWindow(cs);
}
