/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <vector>

#include <afxwin.h>
#include <afxcmn.h>

#include "CAN_ICS_neoVI_Resource.h"
#include "EXTERNAL/icsnVC40.h"

struct SNeoVIDeviceDiscoveryInfo
{
    NeoDevice m_ouDevice {};
    CString m_omModel;
    CString m_omSerial;
    CString m_omIpAddress;
    CString m_omTransport;
    UINT m_unChannelCount {0};
    BOOL m_bRemote {FALSE};
};

extern const int NEOVI_OK;

int nDiscoverNeoVIDevices(std::vector<SNeoVIDeviceDiscoveryInfo>& aDevices,
                          const CString& oRemoteIp,
                          CString& oError);

class CNeoVIDeviceDiscoveryDlg : public CDialog
{
public:
    enum { IDD = IDD_DLG_NEOVI_DEVICE_DISCOVERY };

    CNeoVIDeviceDiscoveryDlg(CWnd* pParent = nullptr, const CString& oInitialIp = CString());

    const SNeoVIDeviceDiscoveryInfo& GetSelectedDevice() const { return m_ouSelectedDevice; }
    const CString& GetManualIp() const { return m_omManualIp; }
    BOOL IsManualIpSelection() const { return m_bUseManualIp; }
    BOOL HasSelection() const { return m_bHasSelection; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnBnClickedRefresh();
    afx_msg void OnItemchangedDeviceList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnNMDblclkDeviceList(NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()

private:
    void vRefreshDeviceList();
    void vSelectRow(int nRow);
    CString sBuildDeviceSummary(const SNeoVIDeviceDiscoveryInfo& sDevice) const;

private:
    CListCtrl m_omListDevices;
    CEdit m_omEditManualIp;
    CStatic m_omStaticStatus;
    std::vector<SNeoVIDeviceDiscoveryInfo> m_ouDevices;
    SNeoVIDeviceDiscoveryInfo m_ouSelectedDevice;
    CString m_omManualIp;
    CString m_omInitialIp;
    BOOL m_bHasSelection {FALSE};
    BOOL m_bUseManualIp {FALSE};
    int m_nLastSelection {-1};
};
