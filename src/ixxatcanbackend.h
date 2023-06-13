#pragma once
#ifndef IxxatCanBackend_H
#define IxxatCanBackend_H

#include <QtSerialBus/qcanbusframe.h>
#include <QtSerialBus/qcanbusdevice.h>
#include <QtSerialBus/qcanbusdeviceinfo.h>
#include <qwineventnotifier.h>
#include "vcisdk.h"

QT_BEGIN_NAMESPACE

class IxxatCanBackend : public QCanBusDevice
{
    Q_OBJECT
public:
    explicit IxxatCanBackend(const QString& name);
    ~IxxatCanBackend();

    bool open() override;
    void close() override;
    bool writeFrame(const QCanBusFrame& newData) override;
    QString interpretErrorFrame(const QCanBusFrame& errorFrame) override;

    static QList<QCanBusDeviceInfo> interfaces();

private:
    void resetController();
    bool hasBusStatus() const;
    QCanBusDevice::CanBusStatus busStatus() const;

    bool OpenSocket();
    bool OpenControl();
    void ReceiveMessage();

    VCIID devVciId = {}; // VCI specific unique ID
    uint devChannel = -1; // Channel number
    IBalObject* pBalObject = NULL; // Bus Access Layer object
    ICanChannel* pCanChannel = NULL; // Channel interface
    ICanControl* pCanControl = NULL; // Control interface
    PFIFOREADER pReader = NULL; // Channel reader
    PFIFOWRITER pWriter = NULL; // Channel writer
    HANDLE hReaderEvent = NULL; // Reader event
    QWinEventNotifier* receiveNotifier = nullptr;
    float tickResolution_usec;
};

QT_END_NAMESPACE

#endif // IxxatCanBackend_H
