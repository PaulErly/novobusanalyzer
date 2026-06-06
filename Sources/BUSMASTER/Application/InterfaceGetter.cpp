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

/**
 * \file      InterfaceGetter.cpp
 * \brief     Source file for global getter functions for interfaces.
 * \author    Ratnadip Choudhury
 * \copyright Copyright (c) 2011, Robert Bosch Engineering and Business Solutions. All rights reserved.
 *
 * Source file for global getter functions for interfaces.
 */


#include "StdAfx.h"
// For some essential data types
// For CAN DIL interface
#include "BaseDIL_CAN.h"
#include "BaseDIL_J1939.h"
#include "BaseDIL_LIN.h"
//#include "DIL_Interface_extern.h"
// For CAN logger interface
#include "FrameProcessor/BaseFrameProcessor_CAN.h"
#include "FrameProcessor/BaseFrameProcessor_J1939.h"
#include "FrameProcessor/BaseFrameProcessor_LIN.h"
#include "FrameProcessor/FrameProcessor_extern.h"
// For Bus statistic interface
#include "BusStatistics.h"
#include "CANDefines.h"
#include "ICluster.h"

//For node simulation interface
#include "NodeSimEx/BaseNodeSim.h"
#include "NodeSimEx/NodeSimEx_Extern.h"
// Application class definition
#include "BUSMASTER.h"
// Mainframe class definition
#include "MainFrm.h"
#include "InterfaceGetter.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <vector>

extern CCANMonitorApp theApp;

namespace
{
    struct ImportedCanSignalData
    {
        std::string name;
        unsigned int startBit = 0;
        unsigned int length = 0;
        eEndianess endian = eIntel;
        eSignalDataType dataType = eUnsigned;
        double factor = 1.0;
        double offset = 0.0;
        std::string unit;
        unsigned __int64 minValue = 0;
        unsigned __int64 maxValue = 0;
        std::map<int, std::string> valueDescriptions;
    };

    struct ImportedCanFrameData
    {
        std::string name;
        unsigned int msgId = 0;
        unsigned int length = 0;
        bool isExtended = false;
        std::vector<ImportedCanSignalData> signals;
    };

    unsigned int nGetNoOfBytesToRead(unsigned int nBitNum, unsigned int nSigLen)
    {
        unsigned int nBytesToRead = 1;
        int nRemainingLength = static_cast<int>(nSigLen) - (8 - nBitNum);

        if (nRemainingLength > 0)
        {
            nBytesToRead += static_cast<unsigned int>(nRemainingLength / 8);
            int nTotalBitsConsidered = ((nBytesToRead - 1) * 8) + (8 - nBitNum);
            if (nTotalBitsConsidered < static_cast<int>(nSigLen))
            {
                nBytesToRead++;
            }
        }

        return nBytesToRead;
    }

    bool bValidateSignal(unsigned int nDLC, unsigned int nByteNum, unsigned int nBitNum,
                         unsigned int nLength, bool bIntel)
    {
        unsigned int nBytesToRead = nGetNoOfBytesToRead(nBitNum, nLength);
        return (bIntel == true) ?
               (static_cast<int>(nByteNum + nBytesToRead - 1) <= static_cast<int>(nDLC)) :
               (static_cast<int>(nByteNum) - static_cast<int>(nBytesToRead) >= 0);
    }

    unsigned long long processSignedValue(unsigned long long nSigValueInBits, int signlalLength, int type)
    {
        if (0 == signlalLength)
        {
            return 0;
        }
        if (1 == type)
        {
            unsigned long long maxValue = static_cast<unsigned long long>(pow(2.0, signlalLength - 1)) - 1;
            if (maxValue < nSigValueInBits)
            {
                unsigned __int64 mask = 0xFFFFFFFFFFFFFFFFULL;
                mask = mask << signlalLength;
                nSigValueInBits = mask | nSigValueInBits;
            }
        }
        return nSigValueInBits;
    }

    unsigned long long GetRawValue(int startBit, bool bIntel, int signlalLength, int type,
                                   unsigned char* pchData, int length)
    {
        unsigned long long nSigValueInBits = 0;
        unsigned int unBitNum = startBit & 7;
        unsigned int byteNumber = startBit / 8 + 1;

        if (pchData != nullptr)
        {
            unsigned int nBytesToRead = nGetNoOfBytesToRead(unBitNum, signlalLength);
            unsigned int CurrBitNum = unBitNum;
            int nByteOrder = (bIntel == true) ? 1 : -1;
            bool bValid = bValidateSignal(length, byteNumber, unBitNum, signlalLength, bIntel);

            if (bValid == true)
            {
                unsigned int nBitsRead = 0;
                for (unsigned int i = 0; i < nBytesToRead; i++)
                {
                    unsigned char byMsgByteVal = pchData[(byteNumber - 1) + (nByteOrder * i)];

                    if (CurrBitNum != 0)
                    {
                        byMsgByteVal >>= CurrBitNum;
                    }

                    unsigned int nCurrBitsToRead = (std::min)(8U - CurrBitNum, signlalLength - nBitsRead);
                    CurrBitNum = 0;
                    unsigned char byMask = static_cast<unsigned char>(pow(2.0, static_cast<int>(nCurrBitsToRead)) - 1);
                    nSigValueInBits |= (static_cast<unsigned long long>(byMsgByteVal & byMask) << nBitsRead);
                    nBitsRead += nCurrBitsToRead;
                }
            }
        }

        return processSignedValue(nSigValueInBits, signlalLength, type);
    }

    class ImportedCanCluster;

    class ImportedCanSignal final : public ISignal
    {
    public:
        ImportedCanSignal(ICluster* pBaseCluster, ImportedCanSignalData signalData)
            : ISignal(pBaseCluster), m_signal(std::move(signalData))
        {
            m_oueElementType = eSignalElement;
            m_strName = m_signal.name;
        }

        ERRORCODE GetEcus(eDIR, std::list<IEcu*>& ouNodes) override
        {
            ouNodes.clear();
            return EC_SUCCESS;
        }

        ERRORCODE GetLength(unsigned int& unSignalLength) override
        {
            unSignalLength = m_signal.length;
            return EC_SUCCESS;
        }

        ERRORCODE GetMinMaxValue(unsigned __int64& unMinValue,
                                 unsigned __int64& unMaxValue) override
        {
            unMinValue = m_signal.minValue;
            unMaxValue = m_signal.maxValue;
            return EC_SUCCESS;
        }

        ERRORCODE MapNode(eDIR, UID_ELEMENT&) override { return EC_SUCCESS; }
        ERRORCODE UnMapNode(eDIR, UID_ELEMENT&) override { return EC_SUCCESS; }
        ERRORCODE GetEncoding(ICoding**) override { return EC_FAILURE; }
        ERRORCODE SetEncoding(UID_ELEMENT&) override { return EC_FAILURE; }

        ERRORCODE GetProperties(SignalProps& ouProps) override
        {
            ouProps.eType = eProtocolType::eCANProtocol;
            ouProps.m_unSignalSize = m_signal.length;
            ouProps.m_ouSignalType = eSignalNormal;
            ouProps.m_ouDataType = m_signal.dataType;
            ouProps.m_ouEndianess = m_signal.endian;
            ouProps.m_nIntialValue = 0;
            ouProps.m_eMultiplex = eNA;
            ouProps.m_nMuliplexedValue = 0;
            ouProps.m_omUnit = m_signal.unit;
            return EC_SUCCESS;
        }

        ERRORCODE SetProperties(SignalProps&) override { return EC_SUCCESS; }

        ERRORCODE GetDataType(eSignalDataType& eDataType) override
        {
            eDataType = m_signal.dataType;
            return EC_SUCCESS;
        }

        ERRORCODE GetUnit(std::string& strUnit) override
        {
            strUnit = m_signal.unit;
            return EC_SUCCESS;
        }

        const std::string& GetNameAsString() const { return m_signal.name; }
        unsigned int GetStartBit() const { return m_signal.startBit; }
        unsigned int GetLengthValue() const { return m_signal.length; }
        eEndianess GetEndianess() const { return m_signal.endian; }
        eSignalDataType GetDataTypeValue() const { return m_signal.dataType; }
        const std::string& GetUnitAsString() const { return m_signal.unit; }

        ERRORCODE GetRawValue(int nStartBit, int nSignalLength, int nByteLength, bool bIntel,
                              const unsigned char* pchData, unsigned __int64& unRawValue) override
        {
            unRawValue = ::GetRawValue(nStartBit, bIntel, nSignalLength,
                                     (m_signal.dataType == eSigned) ? 1 : 0,
                                     const_cast<unsigned char*>(pchData), nByteLength);
            return EC_SUCCESS;
        }

        ERRORCODE GetEnggValueFromRaw(unsigned __int64 dwRawValue, double& dEnggValue) override
        {
            unsigned long long raw = dwRawValue;
            if (m_signal.dataType == eSigned)
            {
                const unsigned int length = (std::min)(m_signal.length, 64U);
                if (length > 0)
                {
                    const unsigned long long signBit = 1ULL << (length - 1);
                    const unsigned long long mask = (length == 64U) ? 0xFFFFFFFFFFFFFFFFULL : ((1ULL << length) - 1ULL);
                    raw &= mask;
                    if ((raw & signBit) != 0)
                    {
                        raw |= ~mask;
                    }
                }
            }
            dEnggValue = static_cast<double>(static_cast<long long>(raw)) * m_signal.factor + m_signal.offset;
            return EC_SUCCESS;
        }

        ERRORCODE GetRawValueFromEng(double dEnggValue, unsigned __int64& dwRawValue) override
        {
            double raw = (dEnggValue - m_signal.offset) / ((m_signal.factor == 0.0) ? 1.0 : m_signal.factor);
            long long signedRaw = static_cast<long long>(std::llround(raw));
            if (m_signal.dataType == eSigned)
            {
                const unsigned int length = (std::min)(m_signal.length, 64U);
                if (length < 64U)
                {
                    const unsigned long long mask = (1ULL << length) - 1ULL;
                    dwRawValue = static_cast<unsigned __int64>(signedRaw) & mask;
                }
                else
                {
                    dwRawValue = static_cast<unsigned __int64>(signedRaw);
                }
            }
            else
            {
                dwRawValue = static_cast<unsigned __int64>((std::max<long long>)(0, signedRaw));
            }
            return EC_SUCCESS;
        }

        ERRORCODE RegisterForChangeNotification(INotifyClusterChange*) override { return EC_SUCCESS; }

    private:
        ImportedCanSignalData m_signal;
    };

    class ImportedCanFrame final : public IFrame
    {
    public:
        ImportedCanFrame(ICluster* pBaseCluster, ImportedCanFrameData frameData)
            : IFrame(pBaseCluster), m_frame(std::move(frameData))
        {
            m_oueElementType = eFrameElement;
            m_strName = m_frame.name;
            for (const auto& sigData : m_frame.signals)
            {
                m_signals.push_back(std::make_unique<ImportedCanSignal>(pBaseCluster, sigData));
            }
        }

        ERRORCODE GetFrameType(eProtocolType& ouType) override
        {
            ouType = eProtocolType::eCANProtocol;
            return EC_SUCCESS;
        }

        ERRORCODE GetFrameId(unsigned int& unFrameId) override
        {
            unFrameId = m_frame.msgId;
            return EC_SUCCESS;
        }

        ERRORCODE GetLength(unsigned int& unFrameLength) override
        {
            unFrameLength = m_frame.length;
            return EC_SUCCESS;
        }

        ERRORCODE GetEcus(eDIR, std::list<IEcu*>& ouNodes) override
        {
            ouNodes.clear();
            return EC_SUCCESS;
        }

        ERRORCODE MapSignal(UID_ELEMENT&, SignalInstanse&) override { return EC_SUCCESS; }
        ERRORCODE UnMapSignal(UID_ELEMENT&) override { return EC_SUCCESS; }
        ERRORCODE MapPdu(UID_ELEMENT&, PduInstanse&) override { return EC_SUCCESS; }
        ERRORCODE UnMapPdu(UID_ELEMENT&) override { return EC_SUCCESS; }
        ERRORCODE MapNode(eDIR, UID_ELEMENT&) override { return EC_SUCCESS; }
        ERRORCODE UnMapNode(eDIR, UID_ELEMENT&) override { return EC_SUCCESS; }

        ERRORCODE SetProperties(FrameProps&) override { return EC_SUCCESS; }

        ERRORCODE GetProperties(FrameProps& ouProps) override
        {
            ouProps.m_eProtocol = eProtocolType::eCANProtocol;
            ouProps.m_nMsgId = m_frame.msgId;
            ouProps.m_unMsgSize = m_frame.length;
            auto* pCanProps = static_cast<CANFrameProps*>(&ouProps);
            if (pCanProps != nullptr)
            {
                pCanProps->m_canMsgType = m_frame.isExtended ? eCan_Extended : eCan_Standard;
            }
            return EC_SUCCESS;
        }

        ERRORCODE GetSignalList(std::map<ISignal*, SignalInstanse>& mapSignals) override
        {
            mapSignals.clear();
            for (auto& signal : m_signals)
            {
                SignalInstanse inst{};
                unsigned int startBit = signal->GetStartBit();
                inst.m_nStartBit = static_cast<int>(startBit);
                inst.m_ouSignalEndianess = signal->GetEndianess();
                inst.m_nUpdateBitPos = static_cast<int>(startBit);
                mapSignals.insert(std::make_pair(signal.get(), inst));
            }
            return EC_SUCCESS;
        }

        unsigned int GetSignalCount() override
        {
            return static_cast<unsigned int>(m_signals.size());
        }

        ERRORCODE GetPduList(std::map<IPdu*, PduInstanse>&) override { return EC_SUCCESS; }
        ERRORCODE GetUpdatedPdus(unsigned char*, std::map<IPdu*, PduInstanse>&) override { return EC_SUCCESS; }

        ERRORCODE InterpretSignals(const unsigned char* pchData, int nSize,
                                   std::list<InterpreteSignals>& ouSignalInfoList,
                                   bool bIsHex, bool /*formatHex*/ = false) override
        {
            ouSignalInfoList.clear();
            if (pchData == nullptr || nSize <= 0)
            {
                return EC_FAILURE;
            }

            for (auto& signal : m_signals)
            {
                InterpreteSignals info{};
                info.m_omSigName = signal->GetNameAsString();
                unsigned __int64 raw = 0;
                signal->GetRawValue(static_cast<int>(signal->GetStartBit()),
                                    static_cast<int>(signal->GetLengthValue()),
                                    nSize,
                                    signal->GetEndianess() == eIntel,
                                    pchData,
                                    raw);
                double eng = 0.0;
                signal->GetEnggValueFromRaw(raw, eng);

                if (bIsHex)
                {
                    CString rawText;
                    rawText.Format("0x%llX", raw);
                    info.m_omRawValue = rawText.GetString();
                    CString engText;
                    engText.Format("%0.6f", eng);
                    info.m_omEnggValue = engText.GetString();
                }
                else
                {
                    CString rawText;
                    if (signal->GetDataTypeValue() == eSigned)
                    {
                        rawText.Format("%lld", static_cast<long long>(raw));
                    }
                    else
                    {
                        rawText.Format("%llu", raw);
                    }
                    info.m_omRawValue = rawText.GetString();
                    CString engText;
                    engText.Format("%0.6f", eng);
                    info.m_omEnggValue = engText.GetString();
                }
                info.m_omUnit = signal->GetUnitAsString();
                ouSignalInfoList.push_back(info);
            }
            return EC_SUCCESS;
        }

        ERRORCODE InterpretSignals(const unsigned char* pchData, int nSize,
                                   std::vector<SignalValue>& ouSignalInfoList) override
        {
            ouSignalInfoList.clear();
            if (pchData == nullptr || nSize <= 0)
            {
                return EC_FAILURE;
            }
            for (auto& signal : m_signals)
            {
                SignalValue value{};
                value.mName = signal->GetNameAsString();
                value.mUnit = signal->GetUnitAsString();
                unsigned __int64 raw = 0;
                signal->GetRawValue(static_cast<int>(signal->GetStartBit()),
                                    static_cast<int>(signal->GetLengthValue()),
                                    nSize,
                                    signal->GetEndianess() == eIntel,
                                    pchData,
                                    raw);
                if (signal->GetDataTypeValue() == eSigned)
                {
                    value.mValue = static_cast<__int64>(raw);
                    value.mIsSigned = true;
                }
                else
                {
                    value.mUnValue = raw;
                    value.mIsSigned = false;
                }
                double eng = 0.0;
                signal->GetEnggValueFromRaw(raw, eng);
                value.mPhyicalValue = eng;
                ouSignalInfoList.push_back(value);
            }
            return EC_SUCCESS;
        }

        ERRORCODE RegisterForChangeNotification(INotifyClusterChange*) override { return EC_SUCCESS; }

        unsigned int GetSignalStartBitByIndex(size_t index) const
        {
            return m_signals[index]->GetStartBit();
        }

        std::vector<std::unique_ptr<ImportedCanSignal>>& Signals() { return m_signals; }
        const std::vector<std::unique_ptr<ImportedCanSignal>>& Signals() const { return m_signals; }

        ImportedCanFrameData m_frame;
        std::vector<std::unique_ptr<ImportedCanSignal>> m_signals;
    };

    class ImportedCanCluster final : public ICluster
    {
    public:
        explicit ImportedCanCluster(std::string dbPath)
            : m_dbPath(std::move(dbPath))
        {
        }

        bool BuildFromMsgSignal(CMsgSignal* pMsgSignal)
        {
            if (pMsgSignal == nullptr)
            {
                return false;
            }
            Clear();
            m_etype = CAN;
            m_bNotificationsEnabled = false;

            UINT messageCount = pMsgSignal->unGetNumerOfMessages();
            if (messageCount == 0)
            {
                return true;
            }

            std::vector<UINT> ids(messageCount);
            pMsgSignal->unListGetMessageIDs(ids.data());

            for (UINT msgId : ids)
            {
                sMESSAGE* pMsg = pMsgSignal->psGetMessagePointerInactive(msgId);
                if (pMsg == nullptr)
                {
                    continue;
                }

                ImportedCanFrameData frameData;
                frameData.name = pMsg->m_omStrMessageName.GetString();
                frameData.msgId = pMsg->m_unMessageCode;
                frameData.length = pMsg->m_unMessageLength;
                frameData.isExtended = (pMsg->m_bMessageFrameFormat != FALSE);

                std::vector<ImportedCanSignalData> sigs;
                for (sSIGNALS* pSig = pMsg->m_psSignals; pSig != nullptr; pSig = pSig->m_psNextSignalList)
                {
                    ImportedCanSignalData sigData;
                    sigData.name = pSig->m_omStrSignalName.GetString();
                    sigData.startBit = pSig->m_unStartByte * 8U + pSig->m_byStartBit;
                    sigData.length = pSig->m_unSignalLength;
                    sigData.endian = pSig->m_eFormat;
                    sigData.dataType = (pSig->m_bySignalType == CHAR_INT) ? eSigned : eUnsigned;
                    sigData.factor = pSig->m_fSignalFactor;
                    sigData.offset = pSig->m_fSignalOffset;
                    sigData.unit = pSig->m_omStrSignalUnit.GetString();
                    if (sigData.dataType == eSigned)
                    {
                        sigData.minValue = static_cast<unsigned __int64>(pSig->m_SignalMinValue.n64Value);
                        sigData.maxValue = static_cast<unsigned __int64>(pSig->m_SignalMaxValue.n64Value);
                    }
                    else
                    {
                        sigData.minValue = pSig->m_SignalMinValue.un64Value;
                        sigData.maxValue = pSig->m_SignalMaxValue.un64Value;
                    }
                    for (CSignalDescVal* pDesc = pSig->m_oSignalIDVal; pDesc != nullptr; pDesc = pDesc->m_pouNextSignalSignalDescVal)
                    {
                        sigData.valueDescriptions.emplace(
                            static_cast<int>(pDesc->m_DescValue.n64Value),
                            pDesc->m_omStrSignalDescriptor.GetString());
                    }
                    sigs.push_back(std::move(sigData));
                }

                std::sort(sigs.begin(), sigs.end(),
                          [](const ImportedCanSignalData& lhs, const ImportedCanSignalData& rhs)
                          {
                              if (lhs.startBit != rhs.startBit)
                              {
                                  return lhs.startBit < rhs.startBit;
                              }
                              return lhs.name < rhs.name;
                          });
                frameData.signals = std::move(sigs);
                m_frames.push_back(std::make_unique<ImportedCanFrame>(this, std::move(frameData)));
            }
            return true;
        }

        ERRORCODE GetNextUniqueId(UID_ELEMENT& uid) override
        {
            uid = m_nextUid++;
            return EC_SUCCESS;
        }

        ERRORCODE LoadFromFile(const std::string&, std::list<ParsingResults>&, std::list<ParsingResults>&) override { return EC_FAILURE; }
        ERRORCODE SaveToFile(const std::string&) override { return EC_FAILURE; }
        ERRORCODE ValidateCluster(std::list<std::string>&) override { return EC_SUCCESS; }
        ERRORCODE GetDBFilePath(std::string& oustrDbFileList) override { oustrDbFileList = m_dbPath; return EC_SUCCESS; }
        ERRORCODE GetDBFileChecksum(std::string& strDBFileChecksum) override { strDBFileChecksum.clear(); return EC_SUCCESS; }
        ERRORCODE Clear() override
        {
            m_frames.clear();
            m_nextUid = 1;
            return EC_SUCCESS;
        }
        ERRORCODE CreateElement(eClusterElementType, IElement**) override { return EC_FAILURE; }
        ERRORCODE DeleteElement(eClusterElementType, UID_ELEMENT&) override { return EC_FAILURE; }
        ERRORCODE GetProperties(ePropertyType, void*) override { return EC_FAILURE; }
        ERRORCODE SetProperties(ePropertyType, void*) override { return EC_FAILURE; }

        ERRORCODE GetElement(eClusterElementType eType, UID_ELEMENT nId, IElement** pElement) override
        {
            if (pElement == nullptr)
            {
                return EC_FAILURE;
            }
            *pElement = nullptr;
            if (eType != eFrameElement)
            {
                return EC_FAILURE;
            }
            for (auto& frame : m_frames)
            {
                if (frame != nullptr && frame->GetUniqueId() == nId)
                {
                    *pElement = frame.get();
                    return EC_SUCCESS;
                }
            }
            return EC_FAILURE;
        }

        ERRORCODE GetElementList(eClusterElementType eType, std::map<UID_ELEMENT, IElement*>& pElement) override
        {
            pElement.clear();
            if (eType != eFrameElement)
            {
                return EC_SUCCESS;
            }
            for (auto& frame : m_frames)
            {
                if (frame != nullptr)
                {
                    pElement.emplace(frame->GetUniqueId(), frame.get());
                }
            }
            return EC_SUCCESS;
        }

        ERRORCODE GetElementByName(eClusterElementType eType, std::string ouElementName, IElement** pElement) override
        {
            if (pElement == nullptr)
            {
                return EC_FAILURE;
            }
            *pElement = nullptr;
            if (eType != eFrameElement)
            {
                return EC_FAILURE;
            }
            for (auto& frame : m_frames)
            {
                std::string name;
                frame->GetName(name);
                if (name == ouElementName)
                {
                    *pElement = frame.get();
                    return EC_SUCCESS;
                }
            }
            return EC_FAILURE;
        }

        ERRORCODE GetElementListByName(eClusterElementType eType, std::string& strEcuName,
                                       std::list<IElement*>& pElementList) override
        {
            pElementList.clear();
            if (eType != eFrameElement)
            {
                return EC_SUCCESS;
            }
            for (auto& frame : m_frames)
            {
                std::string name;
                frame->GetName(name);
                if (name == strEcuName)
                {
                    pElementList.push_back(frame.get());
                }
            }
            return EC_SUCCESS;
        }

        ERRORCODE GetClusterType(ETYPE_BUS& oueBusType) override { oueBusType = CAN; return EC_SUCCESS; }
        ERRORCODE EnableEventNotofications(bool bEnable = true) override { m_bNotificationsEnabled = bEnable; return EC_SUCCESS; }
        bool isNotoficationsEnabled() override { return m_bNotificationsEnabled; }
        ERRORCODE GetEcu(const std::string&, IEcu**) override { return EC_FAILURE; }
        ERRORCODE GetName(std::string& strClusterName) override { strClusterName = m_dbPath; return EC_SUCCESS; }

        ERRORCODE GetFrame(unsigned int& unId, void*, IFrame** pFrame) override
        {
            if (pFrame == nullptr)
            {
                return EC_FAILURE;
            }
            *pFrame = nullptr;
            for (auto& frame : m_frames)
            {
                unsigned int frameId = 0;
                frame->GetFrameId(frameId);
                if (frameId == unId)
                {
                    *pFrame = frame.get();
                    return EC_SUCCESS;
                }
            }
            return EC_FAILURE;
        }

        ERRORCODE GetEcuList(std::list<IEcu*>& pEcuList) override { pEcuList.clear(); return EC_SUCCESS; }

        ERRORCODE GetFrameList(std::list<IFrame*>& pFrameList) override
        {
            pFrameList.clear();
            for (auto& frame : m_frames)
            {
                pFrameList.push_back(frame.get());
            }
            return EC_SUCCESS;
        }

        ERRORCODE GetSignalList(std::list<ISignal*>& pSignalList) override
        {
            pSignalList.clear();
            for (auto& frame : m_frames)
            {
                std::map<ISignal*, SignalInstanse> signals;
                frame->GetSignalList(signals);
                for (auto& entry : signals)
                {
                    pSignalList.push_back(entry.first);
                }
            }
            return EC_SUCCESS;
        }

        ERRORCODE GetPduList(std::list<IPdu*>& pPduList) override { pPduList.clear(); return EC_SUCCESS; }
        eClusterElementType GetElementType(UID_ELEMENT&) override { return eFrameElement; }
        ERRORCODE RegisterForChangeNotification(INotifyClusterChange*, UID_ELEMENT) override { return EC_SUCCESS; }
        ERRORCODE NotifyClusterChange(eAction, UID_ELEMENT&, eClusterElementType&, void*) override { return EC_SUCCESS; }

    private:
        std::string m_dbPath;
        UID_ELEMENT m_nextUid = 1;
        bool m_bNotificationsEnabled = false;
        ETYPE_BUS m_etype = CAN;
        std::vector<std::unique_ptr<ImportedCanFrame>> m_frames;
    };

    std::unique_ptr<ImportedCanCluster> sg_pImportedCanCluster;
}


CBaseFrameProcessor_CAN* GetICANLogger(void)
{
    CBaseFrameProcessor_CAN* Result = nullptr;
    if (FP_GetInterface(FRAMEPROC_CAN, (void**) &Result) == S_OK)
    {
        // Nothing to do at this moment
    }
    if (Result == nullptr)
    {
        TRACE0("GetICANLogger returned nullptr.\n");
    }
    return Result;
}


CBaseFrameProcessor_LIN* GetILINLogger(void)
{
    CBaseFrameProcessor_LIN* Result = nullptr;
    if (FP_GetInterface(FRAMEPROC_LIN, (void**) &Result) == S_OK)
    {
        // Nothing to do at this moment
    }
    if (Result == nullptr)
    {
        TRACE0("GetILINLogger returned nullptr.\n");
    }
    return Result;
}

CBaseFrameProcessor_J1939* GetIJ1939Logger(void)
{
    CBaseFrameProcessor_J1939* Result = nullptr;
    if (FP_GetInterface(FRAMEPROC_J1939, (void**) &Result) == S_OK)
    {
        // Nothing to do at this moment
    }
    if (Result == nullptr)
    {
        TRACE0("GetIJ1939Logger returned nullptr.\n");
    }
    return Result;
}
void ReleaseLogger(eID_COMPONENT interfaceId)
{
    FP_ReleaseInterface(interfaceId);
}
CMainFrame* GetIMainFrame(void)
{
    CMainFrame* Result = static_cast<CMainFrame*> (theApp.m_pMainWnd);
    if (Result == nullptr)
    {
        TRACE0("GetIMainFrame returned nullptr.\n");
    }
    return Result;
}

CFlags* GetIFlags(void)
{
    CFlags* Result = (theApp.pouGetFlagsPtr());
    if (Result == nullptr)
    {
        TRACE0("GetIFlags returned nullptr.\n");
    }
    return Result;
}

CBaseDIL_LIN* GetILINDIL(void)
{
    CBaseDIL_LIN* Result = nullptr;
    if ( DIL_GetInterface( LIN, (void**)&Result ) == S_OK )
    {
        if (Result == nullptr)
        {
            TRACE0("GetILINDIL returned nullptr.\n");
        }
    }
    else
    {
        TRACE0("GetILINDIL interface query failed.\n");
    }
    return Result;
}


CBaseDIL_CAN* GetICANDIL(void)
{
    CBaseDIL_CAN* Result = nullptr;
    if ( DIL_GetInterface( CAN, (void**)&Result ) == S_OK )
    {
        if (Result == nullptr)
        {
            TRACE0("GetICANDIL returned nullptr.\n");
        }
    }
    else
    {
        TRACE0("GetICANDIL interface query failed.\n");
    }
    return Result;
}

CBaseDILI_J1939* GetIJ1939DIL(void)
{
    CBaseDILI_J1939* Result = nullptr;
    if ( DIL_GetInterface( J1939, (void**)&Result ) == S_OK )
    {
        if (Result == nullptr)
        {
            TRACE0("GetIJ1939DIL returned nullptr.\n");
        }
    }
    else
    {
        TRACE0("GetIJ1939DIL interface query failed.\n");
    }
    return Result;
}

CBaseBusStatisticCAN* GetICANBusStat(void)
{
    CBaseBusStatisticCAN* Result = nullptr;
    if (BS_GetInterface(CAN, (void**) &Result) == S_OK)
    {
        if (Result == nullptr)
        {
            TRACE0("GetICANBusStat returned nullptr.\n");
        }
    }
    else
    {
        TRACE0("GetICANBusStat interface query failed.\n");
    }
    return Result;
}

CBaseBusStatisticLIN* GetILINBusStat(void)
{
    CBaseBusStatisticLIN* Result = nullptr;
    if (BS_GetInterface(LIN, (void**) &Result) == S_OK)
    {
        if (Result == nullptr)
        {
            TRACE0("GetILINBusStat returned nullptr.\n");
        }
    }
    else
    {
        TRACE0("GetILINBusStat interface query failed.\n");
    }
    return Result;
}
void ReleaseBusStat(ETYPE_BUS bus)
{
    BS_ReleaseInterface(bus);
}


CMsgSignal* GetIMsgDB(void)
{
    return theApp.m_pouMsgSignal;
}

extern DWORD g_dwClientID;

DWORD dwGetMonitorClientID(void)
{
    return g_dwClientID;
}

CBaseNodeSim* GetICANNodeSim(void)
{
    CBaseNodeSim* Result = nullptr;
    if (NS_GetInterface(CAN, (void**) &Result) == S_OK)
    {
        // Nothing to do at this moment
    }
    if (Result == nullptr)
    {
        TRACE0("GetICANNodeSim returned nullptr.\n");
    }
    return Result;
}
CBaseNodeSim* GetILINNodeSim(void)
{
    CBaseNodeSim* Result = nullptr;
    if (NS_GetInterface(LIN, (void**) &Result) == S_OK)
    {
        // Nothing to do at this moment
    }
    if (Result == nullptr)
    {
        TRACE0("GetILINNodeSim returned nullptr.\n");
    }
    return Result;
}

CBaseNodeSim* GetIJ1939NodeSim(void)
{
    CBaseNodeSim* Result = nullptr;
    if (NS_GetInterface(J1939, (void**) &Result) == S_OK)
    {
        // Nothing to do at this moment
    }
    if (Result == nullptr)
    {
        TRACE0("GetIJ1939NodeSim returned nullptr.\n");
    }
    return Result;
}

bool RegisterImportedCanDatabaseForTransmit(CMsgSignal* pMsgSignal,
                                           const CString& dbPath)
{
    CMainFrame* pMainFrame = GetIMainFrame();
    IBMNetWorkService* pNetwork = nullptr;
    if (pMainFrame != nullptr)
    {
        pMainFrame->getDbSetService(&pNetwork);
    }
    if (pMainFrame == nullptr || pNetwork == nullptr)
    {
        TRACE0("Imported CAN database transmit registration failed: network unavailable.\n");
        return false;
    }
    if (pMsgSignal == nullptr)
    {
        TRACE0("Imported CAN database transmit registration failed: database object unavailable.\n");
        return false;
    }

    auto pCluster = std::make_unique<ImportedCanCluster>(dbPath.GetString());
    if (!pCluster->BuildFromMsgSignal(pMsgSignal))
    {
        TRACE0("Imported CAN database transmit registration failed: no CAN frames were built.\n");
        return false;
    }

    if (EC_SUCCESS != pNetwork->SetChannelCount(CAN, 1))
    {
        TRACE0("Imported CAN database transmit registration failed: SetChannelCount returned failure.\n");
        return false;
    }

    if (EC_SUCCESS != pNetwork->SetDBService(CAN, 0, 0, pCluster.get()))
    {
        TRACE0("Imported CAN database transmit registration failed: SetDBService returned failure.\n");
        return false;
    }

    sg_pImportedCanCluster = std::move(pCluster);
    TRACE0("Imported CAN database transmit registration completed successfully.\n");
    return true;
}
