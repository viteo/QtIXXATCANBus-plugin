#pragma once
#include <qobject.h>
#include <QtSerialBus/qcanbus.h>
#include <QtSerialBus/qcanbusdevice.h>
#include <QtSerialBus/qcanbusfactory.h>
#include "IxxatCanBackend.h"

class IxxatCanBusPlugin : public QObject, public QCanBusFactoryV2
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QCanBusFactory" FILE "plugin.json")
	Q_INTERFACES(QCanBusFactoryV2)

public:
	QList<QCanBusDeviceInfo> availableDevices(QString* errorMessage) const override
	{
		Q_UNUSED(errorMessage);
		return IxxatCanBackend::interfaces();
	}

	// Name passed from availableDevices() list
	QCanBusDevice* createDevice(const QString& interfaceName, QString* errorMessage) const override
	{
		Q_UNUSED(errorMessage);
		auto device = new IxxatCanBackend(interfaceName);
		return device;
	}
};
