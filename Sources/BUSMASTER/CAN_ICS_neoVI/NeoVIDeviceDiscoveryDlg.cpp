/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "CAN_ICS_neoVI_stdafx.h"
#include "NeoVIDeviceDiscovery.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CNeoVIDeviceDiscoveryDlg, CDialog)
    ON_BN_CLICKED(IDC_BUTTON_NEOVI_REFRESH, OnBnClickedRefresh)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_NEOVI_DEVICES, OnItemchangedDeviceList)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST_NEOVI_DEVICES, OnNMDblclkDeviceList)
END_MESSAGE_MAP()

static UINT sGetNeoVIChannelCount(unsigned long deviceType, int hardwareLic)
{
    switch (deviceType)
    {
        case NEODEVICE_BLUE:
        case NEODEVICE_FIRE:
            return 4;
        case NEODEVICE_DW_VCAN:
            return 1;
        case NEODEVICE_VCAN3:
            return (hardwareLic == 8) ? 1U : 2U;
        default:
            return 1;
    }
}

static CString sGetNeoVIModelName(unsigned long deviceType)
{
    switch (deviceType)
    {
        case NEODEVICE_BLUE:   return _T("neoVI Blue");
        case NEODEVICE_FIRE:   return _T("neoVI Fire/Red");
        case NEODEVICE_DW_VCAN:return _T("ValueCAN");
        case NEODEVICE_VCAN3:  return _T("ValueCAN 3");
        default:
            {
                CString omTmp;
                omTmp.Format(_T("DeviceType 0x%lx"), deviceType);
                return omTmp;
            }
    }
}

static CString sGetNeoVITransportName(BOOL bRemote)
{
    return bRemote ? _T("Ethernet") : _T("Local");
}

CNeoVIDeviceDiscoveryDlg::CNeoVIDeviceDiscoveryDlg(CWnd* pParent, const CString& oInitialIp)
    : CDialog(CNeoVIDeviceDiscoveryDlg::IDD, pParent)
    , m_omManualIp(oInitialIp)
    , m_omInitialIp(oInitialIp)
{
}

void CNeoVIDeviceDiscoveryDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_NEOVI_DEVICES, m_omListDevices);
    DDX_Control(pDX, IDC_EDIT_NEOVI_MANUAL_IP, m_omEditManualIp);
    DDX_Control(pDX, IDC_STATIC_NEOVI_STATUS, m_omStaticStatus);
}

BOOL CNeoVIDeviceDiscoveryDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_omListDevices.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_omListDevices.InsertColumn(0, _T("Transport"), LVCFMT_LEFT, 76);
    m_omListDevices.InsertColumn(1, _T("Model"), LVCFMT_LEFT, 124);
    m_omListDevices.InsertColumn(2, _T("Serial"), LVCFMT_LEFT, 78);
    m_omListDevices.InsertColumn(3, _T("IP"), LVCFMT_LEFT, 110);
    m_omListDevices.InsertColumn(4, _T("Channels"), LVCFMT_LEFT, 60);

    m_omEditManualIp.SetWindowText(m_omManualIp);
    vRefreshDeviceList();
    return TRUE;
}

void CNeoVIDeviceDiscoveryDlg::vSelectRow(int nRow)
{
    if (nRow < 0 || nRow >= static_cast<int>(m_ouDevices.size()))
    {
        return;
    }

    m_nLastSelection = nRow;
    for (int i = 0; i < m_omListDevices.GetItemCount(); ++i)
    {
        m_omListDevices.SetItemState(i, 0, LVIS_SELECTED | LVIS_FOCUSED);
    }

    m_omListDevices.SetItemState(nRow, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    m_omListDevices.EnsureVisible(nRow, FALSE);
    m_ouSelectedDevice = m_ouDevices[nRow];
    m_bHasSelection = TRUE;
    m_bUseManualIp = FALSE;
}

CString CNeoVIDeviceDiscoveryDlg::sBuildDeviceSummary(const SNeoVIDeviceDiscoveryInfo& sDevice) const
{
    CString omSummary;
    omSummary.Format(_T("%s, SN %s, %s, %u channels"),
                     sDevice.m_omModel.GetString(),
                     sDevice.m_omSerial.GetString(),
                     sDevice.m_omTransport.GetString(),
                     sDevice.m_unChannelCount);
    return omSummary;
}

void CNeoVIDeviceDiscoveryDlg::vRefreshDeviceList()
{
    m_omListDevices.DeleteAllItems();
    m_ouDevices.clear();
    m_bHasSelection = FALSE;
    m_bUseManualIp = FALSE;
    m_nLastSelection = -1;

    m_omEditManualIp.GetWindowText(m_omManualIp);

    CString omError;
    const int nResult = nDiscoverNeoVIDevices(m_ouDevices, m_omManualIp, omError);
    if (nResult != NEOVI_OK && omError.IsEmpty())
    {
        omError = _T("neoVI discovery failed.");
    }

    for (size_t i = 0; i < m_ouDevices.size(); ++i)
    {
        const auto& sDevice = m_ouDevices[i];
        const int nItem = m_omListDevices.InsertItem(static_cast<int>(i), sDevice.m_omTransport);
        m_omListDevices.SetItemText(nItem, 1, sDevice.m_omModel);
        m_omListDevices.SetItemText(nItem, 2, sDevice.m_omSerial);
        m_omListDevices.SetItemText(nItem, 3, sDevice.m_omIpAddress);
        CString omChannels;
        omChannels.Format(_T("%u"), sDevice.m_unChannelCount);
        m_omListDevices.SetItemText(nItem, 4, omChannels);
    }

    if (!m_ouDevices.empty())
    {
        vSelectRow(0);
        CString omStatus;
        omStatus.Format(_T("Found %zu neoVI device(s)."), m_ouDevices.size());
        m_omStaticStatus.SetWindowText(omStatus);
    }
    else
    {
        m_omStaticStatus.SetWindowText(omError.IsEmpty() ? _T("No neoVI devices discovered.") : omError);
    }
}

void CNeoVIDeviceDiscoveryDlg::OnBnClickedRefresh()
{
    vRefreshDeviceList();
}

void CNeoVIDeviceDiscoveryDlg::OnItemchangedDeviceList(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_LISTVIEW* pNMListView = reinterpret_cast<NM_LISTVIEW*>(pNMHDR);
    if ((pNMListView->uNewState & LVIS_SELECTED) != 0)
    {
        const int nRow = m_omListDevices.GetNextItem(-1, LVNI_SELECTED);
        if (nRow >= 0 && nRow < static_cast<int>(m_ouDevices.size()))
        {
            m_nLastSelection = nRow;
            m_ouSelectedDevice = m_ouDevices[nRow];
            m_bHasSelection = TRUE;
            m_bUseManualIp = FALSE;
            m_omEditManualIp.SetWindowText(m_ouSelectedDevice.m_omIpAddress);
        }
    }
    *pResult = 0;
}

void CNeoVIDeviceDiscoveryDlg::OnNMDblclkDeviceList(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    if (m_bHasSelection || !m_omManualIp.IsEmpty())
    {
        EndDialog(IDOK);
    }
    *pResult = 0;
}

void CNeoVIDeviceDiscoveryDlg::OnOK()
{
    m_omEditManualIp.GetWindowText(m_omManualIp);
    if (m_nLastSelection >= 0 && m_nLastSelection < static_cast<int>(m_ouDevices.size()))
    {
        m_ouSelectedDevice = m_ouDevices[m_nLastSelection];
        m_bHasSelection = TRUE;
        m_bUseManualIp = FALSE;
        if (m_ouSelectedDevice.m_omIpAddress.IsEmpty() == FALSE)
        {
            m_omManualIp = m_ouSelectedDevice.m_omIpAddress;
        }
        EndDialog(IDOK);
        return;
    }

    if (!m_omManualIp.IsEmpty())
    {
        m_bUseManualIp = TRUE;
        m_bHasSelection = FALSE;
        m_ouSelectedDevice.m_omIpAddress = m_omManualIp;
        m_ouSelectedDevice.m_omTransport = _T("Ethernet");
        m_ouSelectedDevice.m_omModel = _T("Manual IP");
        m_ouSelectedDevice.m_omSerial = _T("");
        m_ouSelectedDevice.m_unChannelCount = 0;
        EndDialog(IDOK);
        return;
    }

    if (m_ouDevices.empty())
    {
        AfxMessageBox(_T("No neoVI device is selected and no manual IP was entered."));
        return;
    }

    EndDialog(IDOK);
}
