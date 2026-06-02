// MessageWindowSettingsDialog.cpp : implementation file
//

#include "stdafx.h"
#include "BUSMASTER.h"
#include "MessageWindowSettingsDialog.h"

// MessageWindowSettingsDialog

IMPLEMENT_DYNAMIC(MessageWindowSettingsDialog, CPropertySheet)

#include "MessageAttrib.h"

MessageWindowSettingsDialog::MessageWindowSettingsDialog(LPCTSTR pszCaption, MessageWindowSettings* settings)
    :CPropertySheet(pszCaption)
{
    m_hOldResource = AfxGetResourceHandle();
    AfxSetResourceHandle(AfxGetInstanceHandle());

    mSettings = settings;
    TRACE1("MessageWindowSettingsDialog ctor: caption=%s\n", pszCaption);
    // TODO:  Add your specialized code here
    TRACE0("Allocating CAN/LIN/J1939 message settings pages.\n");
    TRACE0("Allocating DB message page.\n");
    odDBMsg = new CPPageMessage(TRUE, mSettings->mMessageAttribute.mMessageIDs, mSettings->mMessageAttribute.mMsgCount, mSettings->mMessageAttribute.mMsgAttributes);
    TRACE0("Allocating non-DB message page.\n");
    odNDBMsg = new CPPageMessage(FALSE, mSettings->mMessageAttribute.mMessageIDs, mSettings->mMessageAttribute.mMsgCount, mSettings->mMessageAttribute.mMsgAttributes);
    TRACE0("Allocating filter page.\n");
    omFilter = new CMsgFilterConfigPage(pszCaption, &mSettings->mFilterDetails, nullptr);
    mMsgBuffConf = nullptr;


    //mMsgBuffConf->vSetBufferSize(m_anMsgBuffSize[CAN]);
    if (true == mSettings->mFilterDetails.mISValidSettings)
    {
        AddPage(omFilter);
    }
    if (true == mSettings->mMessageAttribute.mValidSettings)
    {
        AddPage(odDBMsg);
        AddPage(odNDBMsg);
    }
    if (true == mSettings->mBufferSettings.mISValidSettings)
    {
        TRACE0("Allocating buffer page.\n");
        mMsgBuffConf = new CMsgBufferConfigPage(&settings->mBufferSettings);
        AddPage(mMsgBuffConf);
    }
}

MessageWindowSettingsDialog::~MessageWindowSettingsDialog()
{
    AfxSetResourceHandle(m_hOldResource);
}


BEGIN_MESSAGE_MAP(MessageWindowSettingsDialog, CPropertySheet)
END_MESSAGE_MAP()


// MessageWindowSettingsDialog message handlers


BOOL MessageWindowSettingsDialog::OnInitDialog()
{
    AfxSetResourceHandle(AfxGetInstanceHandle());

    BOOL bResult = CPropertySheet::OnInitDialog();
    TRACE1("MessageWindowSettingsDialog::OnInitDialog page count=%d\n", GetPageCount());

    CString title;
    GetWindowText(title);
    title = "Configure Message Display - " + title;
    SetWindowText(title);

    TRACE1("Message window settings dialog opened with %d pages.\n", GetPageCount());
    if (GetPageCount() == 0)
    {
        TRACE0("Message window settings dialog has no pages to display.\n");
        AfxMessageBox(_("Unable to open the message display settings."), MB_ICONSTOP | MB_OK);
        return FALSE;
    }

    return bResult;
}
