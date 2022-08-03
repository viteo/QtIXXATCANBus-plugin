#include "IxxatCanBackend.h"
#include <qvariant.h>

// List all devices and channels (buses)
QList<QCanBusDeviceInfo> IxxatCanBackend::interfaces()
{
	HRESULT hResult;				// error code
	QList<QCanBusDeviceInfo> deviceList;
	IVciDeviceManager* pDeviceManager = NULL;	// device manager
	IVciEnumDevice* pEnum = NULL;		// enumerator handle
	VCIDEVICEINFO sInfo;			// device info
	IVciDevice* pDevice = NULL; // device handle
	VCIDEVICECAPS sCaps;			// device capabilities

	hResult = VciGetDeviceManager(&pDeviceManager);
	if (hResult != VCI_OK)
		return deviceList;
	hResult = pDeviceManager->EnumDevices(&pEnum);
	if (hResult != VCI_OK)
		return deviceList;
	while (pEnum->Next(1, &sInfo, NULL) == VCI_OK)
	{
		pDeviceManager->OpenDevice(sInfo.VciObjectId, &pDevice);
		pDevice->GetDeviceCaps(&sCaps);
		for (int channel = 0; channel < sCaps.BusCtrlCount; channel++)
		{
			if (VCI_BUS_TYPE(sCaps.BusCtrlTypes[channel]) == VCI_BUS_CAN)
				deviceList.append(
					createDeviceInfo(
						QString("VCI%1-CAN%2").arg(sInfo.VciObjectId.AsInt64).arg(channel),
						QString(sInfo.UniqueHardwareId.AsChar),
						QString(sInfo.Description),
						channel, false, false)
				);
		}
		pDevice->Release();
	}
	pEnum->Release();
	pDeviceManager->Release();
	return deviceList;
}

// Prepare device for opening
IxxatCanBackend::IxxatCanBackend(const QString& name)
{
	bool ok;
	auto dev = name.splitRef('-', Qt::SkipEmptyParts, Qt::CaseInsensitive);
	this->devVciId.AsInt64 = dev.first().mid(3).toLongLong();
	this->devChannel = dev.last().mid(3).toUInt(&ok);

	if (!ok)
	{
		this->setError(QStringLiteral("Wrong device name format"), QCanBusDevice::CanBusError::ConfigurationError);
		return;
	}

	IVciDeviceManager* pDeviceManager = NULL;	// device manager
	IVciDevice* pDevice = NULL; // device handle

	VciGetDeviceManager(&pDeviceManager);
	pDeviceManager->OpenDevice(this->devVciId, &pDevice);
	pDeviceManager->Release();

	if (!pDevice)
	{
		this->setError(QStringLiteral("Device not present"), QCanBusDevice::CanBusError::ConfigurationError);
		return;
	}

	pDevice->OpenComponent(CLSID_VCIBAL, IID_IBalObject, (PVOID*)&this->pBalObject);
	pDevice->Release();
}

IxxatCanBackend::~IxxatCanBackend()
{
    this->close();
}


bool IxxatCanBackend::OpenSocket()
{
	HRESULT hResult = E_FAIL;
	pCanChannel = 0;
	ICanSocket* pCanSocket = 0;

	if (!this->pBalObject)
		return false;

	hResult = this->pBalObject->OpenSocket(this->devChannel, IID_ICanSocket, (PVOID*)&pCanSocket);
	if (hResult != VCI_OK)
		return false;

	// calculation of the length of a tick in microseconds
	CANCAPABILITIES canCapabilities;
	pCanSocket->GetCapabilities(&canCapabilities);
	tickResolution_usec = 1e6 * canCapabilities.dwTscDivisor / canCapabilities.dwClockFreq;

	// create a message channel
	hResult = pCanSocket->CreateChannel(FALSE, &pCanChannel);
	pCanSocket->Release();
	if (hResult != VCI_OK)
		return false;

	// initialize the message channel
	UINT16 wRxFifoSize = 1024;
	UINT16 wRxThreshold = 1;
	UINT16 wTxFifoSize = 128;
	UINT16 wTxThreshold = 1;
	pReader = 0;
	pWriter = 0;
	hResult = pCanChannel->Initialize(wRxFifoSize, wTxFifoSize);
	if (hResult != VCI_OK)
		return false;
	hResult = pCanChannel->GetReader(&pReader);
	if (hResult != VCI_OK)
		return false;
	pReader->SetThreshold(wRxThreshold);
	hReaderEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	pReader->AssignEvent(hReaderEvent);
	delete receiveNotifier;
	receiveNotifier = new QWinEventNotifier(hReaderEvent, this);
	connect(receiveNotifier, &QWinEventNotifier::activated, this, &IxxatCanBackend::ReceiveMessage);
	receiveNotifier->setEnabled(true);

	hResult = pCanChannel->GetWriter(&pWriter);
	if (hResult != VCI_OK)
		return false;
	pWriter->SetThreshold(wTxThreshold);

	// activate the CAN channel
	hResult = pCanChannel->Activate();
	if (hResult != VCI_OK)
		return false;

	if (hasBusStatus())
	{
		std::function<CanBusStatus()> g = std::bind(&IxxatCanBackend::busStatus, this);
		setCanBusStatusGetter(g);
	}

	std::function<void()> f = std::bind(&IxxatCanBackend::resetController, this);
	setResetControllerFunction(f);

	return true;
}

bool IxxatCanBackend::OpenControl()
{
	// Open the CAN control interface
	pCanControl = NULL;    // control interface

	// Try open the CAN control interface
	if (this->pBalObject->OpenSocket(this->devChannel, IID_ICanControl, (PVOID*)&pCanControl) != VCI_OK)
	{
		// can't get the control interface. it is occupied by another application
		// live with it and move on
		return false;
	}
	// default 1000 kBit/s
	CANINITLINE init =
	{
	  CAN_OPMODE_STANDARD | CAN_OPMODE_EXTENDED | CAN_OPMODE_ERRFRAME, // opmode
	  0,             // bReserved
	  CAN_BT0_1000KB, // bt0
	  CAN_BT1_1000KB  // bt1
	};

	int bitrate = configurationParameter(ConfigurationKey::BitRateKey).toInt();
	switch (bitrate)
	{
	case 5000: init.bBtReg0 = CAN_BT0_5KB; init.bBtReg1 = CAN_BT1_5KB; break;
	case 10000: init.bBtReg0 = CAN_BT0_10KB; init.bBtReg1 = CAN_BT1_10KB; break;
	case 20000: init.bBtReg0 = CAN_BT0_20KB; init.bBtReg1 = CAN_BT1_20KB; break;
	case 50000: init.bBtReg0 = CAN_BT0_50KB; init.bBtReg1 = CAN_BT1_50KB; break;
	case 100000: init.bBtReg0 = CAN_BT0_100KB; init.bBtReg1 = CAN_BT1_100KB; break;
	case 125000: init.bBtReg0 = CAN_BT0_125KB; init.bBtReg1 = CAN_BT1_125KB; break;
	case 250000: init.bBtReg0 = CAN_BT0_250KB; init.bBtReg1 = CAN_BT1_250KB; break;
	case 500000: init.bBtReg0 = CAN_BT0_500KB; init.bBtReg1 = CAN_BT1_500KB; break;
	case 800000: init.bBtReg0 = CAN_BT0_800KB; init.bBtReg1 = CAN_BT1_800KB; break;
	case 1000000: init.bBtReg0 = CAN_BT0_1000KB; init.bBtReg1 = CAN_BT1_1000KB; break;
	}

	pCanControl->InitLine(&init);

	// set the acceptance filter
	QList<QCanBusDevice::Filter> filters = qvariant_cast<QList<QCanBusDevice::Filter>>(configurationParameter(ConfigurationKey::RawFilterKey));
	for (auto& filter : qAsConst(filters))
	{
		UINT32 dwCode = filter.frameId << 1 | (filter.type == QCanBusFrame::FrameType::RemoteRequestFrame);
		UINT32 dwMask = filter.frameIdMask << 1 | (filter.type == QCanBusFrame::FrameType::RemoteRequestFrame);
		if (filter.format == QCanBusDevice::Filter::MatchBaseAndExtendedFormat)
		{
			pCanControl->SetAccFilter(CAN_FILTER_STD, dwCode, dwMask);
			pCanControl->SetAccFilter(CAN_FILTER_EXT, dwCode, dwMask);
		}
		else
		{
			pCanControl->SetAccFilter(filter.format, dwCode, dwMask);
		}
	}

	// start the CAN controller
	pCanControl->StartLine();
	return true;
}

bool IxxatCanBackend::open()
{
	if (OpenSocket() == false)
	{
		this->setState(QCanBusDevice::UnconnectedState);
		return false;
	}

	(void)OpenControl(); //Ignore the result, because Control might already be opened by another application

	this->setState(QCanBusDevice::ConnectedState);
	return true;
}

void IxxatCanBackend::close()
{
	receiveNotifier->setEnabled(false);
	if (pReader)
	{
		pReader->Release();
		pReader = NULL;
	}
	if (pWriter)
	{
		pWriter->Release();
		pWriter = NULL;
	}
	if (pCanChannel)
	{
		pCanChannel->Release();
		pCanChannel = NULL;
	}
	if (pCanControl)
	{
		pCanControl->StopLine();
		pCanControl->ResetLine();
		pCanControl->Release();
		pCanControl = NULL;
	}
	if (pBalObject)
	{
		pBalObject->Release();
		pBalObject = NULL;
	}
}

bool IxxatCanBackend::writeFrame(const QCanBusFrame& newData)
{
	uint16_t reservedFramesCount = 0;
	PCANMSG pMessage;

	if (!newData.isValid())
	{
		emit errorOccurred(QCanBusDevice::WriteError);
		return false;
	}

	while (pWriter->AcquireWrite((PVOID*)&pMessage, &reservedFramesCount) != VCI_OK) //todo
	{
		//if (retry_count++ > TIMEOUT)
		//	return false;
	}
	uint16_t framesToWrite = 0;
	if (reservedFramesCount > 0)
	{
		pMessage->dwTime = 0;
		pMessage->dwMsgId = newData.frameId();
		switch (newData.frameType())
		{
		case QCanBusFrame::RemoteRequestFrame:
		case QCanBusFrame::DataFrame: pMessage->uMsgInfo.Bytes.bType = CAN_MSGTYPE_DATA; break;
		case QCanBusFrame::InvalidFrame: // handled above with isValid()
		case QCanBusFrame::UnknownFrame:
		case QCanBusFrame::ErrorFrame: pMessage->uMsgInfo.Bytes.bType = CAN_MSGTYPE_ERROR; break;
		}
		pMessage->uMsgInfo.Bytes.bFlags = CAN_MAKE_MSGFLAGS(
			CAN_LEN_TO_SDLC(newData.payload().length()), // DLC
			0, // Data overrun
			configurationParameter(ConfigurationKey::ReceiveOwnKey).toBool(), // Self reception request
			newData.frameType() == QCanBusFrame::RemoteRequestFrame, // RTR Remote transmission request
			newData.hasExtendedFrameFormat() // Extended frame format
		);
		pMessage->uMsgInfo.Bytes.bFlags2 = CAN_MAKE_MSGFLAGS2(0, 0, 0, 0, 0);
		for (uint32_t i = 0; i < pMessage->uMsgInfo.Bits.dlc; i++)
		{
			pMessage->uMsgInfo.Bits.rtr ?
				pMessage->abData[i] = 0 : // If RTR - payload is empty
				pMessage->abData[i] = newData.payload().at(i);
		}
		// for transmit messages exclusively the message type CAN_MSGTYPE_DATA is valid.
		if (pMessage->uMsgInfo.Bytes.bType == CAN_MSGTYPE_DATA)
			framesToWrite = 1;
	}
	if (pWriter->ReleaseWrite(framesToWrite) == VCI_OK)
	{
		emit framesWritten(framesToWrite);
		return true;
	}

	emit errorOccurred(QCanBusDevice::WriteError);
	return false;
}


void IxxatCanBackend::ReceiveMessage()
{
	if (!pReader)
		return;

	PCANMSG pMessages;
	QVector<QCanBusFrame> newFrames;

	// check if messages available
	uint16_t wCount = 0;
	if (pReader->AcquireRead((PVOID*)&pMessages, &wCount) != VCI_OK)
		return;
	for (int i = 0; i < wCount; i++, pMessages++)
	{
		QCanBusFrame frame(pMessages->dwMsgId, QByteArray(reinterpret_cast<char*>(pMessages->abData), pMessages->uMsgInfo.Bits.dlc));
		frame.setTimeStamp(QCanBusFrame::TimeStamp::fromMicroSeconds(pMessages->dwTime * tickResolution_usec));
		frame.setExtendedFrameFormat(pMessages->uMsgInfo.Bits.ext);
		frame.setFrameType(pMessages->uMsgInfo.Bits.rtr ? QCanBusFrame::RemoteRequestFrame : QCanBusFrame::DataFrame);
		if (pMessages->uMsgInfo.Bits.type == CAN_MSGTYPE_ERROR)
		{
			frame.setFrameType(QCanBusFrame::ErrorFrame);
			frame.setErrorStateIndicator(true);
		}
		frame.setLocalEcho(pMessages->uMsgInfo.Bits.srr);
		newFrames.append(std::move(frame));
	}
	pReader->ReleaseRead(wCount);
	enqueueReceivedFrames(newFrames);
}

QString IxxatCanBackend::interpretErrorFrame(const QCanBusFrame& errorFrame)
{
	if (errorFrame.frameType() != QCanBusFrame::ErrorFrame)
		return QString();

	QString errorMsg;

	if (errorFrame.error() & QCanBusFrame::TransmissionTimeoutError)
		errorMsg += QStringLiteral("TX timout\n");

	if (errorFrame.error() & QCanBusFrame::MissingAcknowledgmentError)
		errorMsg += QStringLiteral("Received no ACK on transmission\n");

	if (errorFrame.error() & QCanBusFrame::BusOffError)
		errorMsg += QStringLiteral("Bus off\n");

	if (errorFrame.error() & QCanBusFrame::BusError)
		errorMsg += QStringLiteral("Bus error\n");

	if (errorFrame.error() & QCanBusFrame::ControllerRestartError)
		errorMsg += QStringLiteral("Controller restarted\n");

	if (errorFrame.error() & QCanBusFrame::UnknownError)
		errorMsg += QStringLiteral("Unknown error\n");

	if (errorFrame.error() & QCanBusFrame::LostArbitrationError)
		errorMsg += QStringLiteral("Lost arbitration\n");

	if (errorFrame.error() & QCanBusFrame::ControllerError)
		errorMsg += QStringLiteral("Controller problem\n");

	if (errorFrame.error() & QCanBusFrame::TransceiverError)
		errorMsg = QStringLiteral("Transceiver problem");

	if (errorFrame.error() & QCanBusFrame::ProtocolViolationError)
		errorMsg += QStringLiteral("Protocol violation\n");

	// cut trailing '\n'
	if (!errorMsg.isEmpty())
		errorMsg.chop(1);

	return errorMsg;
}

void IxxatCanBackend::resetController()
{
	if (pCanControl) //if we have the control, we may reset the controller
	{
		pCanControl->StopLine();
		pCanControl->StartLine();
	}
}

bool IxxatCanBackend::hasBusStatus() const
{
	return true;
}

QCanBusDevice::CanBusStatus IxxatCanBackend::busStatus() const
{
	CANCHANSTATUS canStatus;
	if (!pCanChannel)
		return QCanBusDevice::CanBusStatus::Unknown;
	pCanChannel->GetStatus(&canStatus);

	if (canStatus.sLineStatus.dwStatus & (CAN_STATUS_BUSOFF | CAN_STATUS_ININIT))
		return QCanBusDevice::CanBusStatus::BusOff;
	if (canStatus.sLineStatus.dwStatus & (CAN_STATUS_BUSCERR | CAN_STATUS_ERRLIM))
		return QCanBusDevice::CanBusStatus::Error;
	if (canStatus.sLineStatus.dwStatus & (CAN_STATUS_OVRRUN | CAN_STATUS_TXPEND))
		return QCanBusDevice::CanBusStatus::Warning;
	return QCanBusDevice::CanBusStatus::Good;
}
