#pragma once
#include <qobject.h>
#include <QtSerialBus/qcanbus.h>
#include <QtSerialBus/qcanbusdevice.h>
#include <QtSerialBus/qcanbusfactory.h>
#include "IxxatCanBackend.h"

#if QT_VERSION >= 0x060000
#define Q_CANBUS_FACTORY_INTERFACE QCanBusFactory
#else
#define Q_CANBUS_FACTORY_INTERFACE QCanBusFactoryV2
#endif

class IxxatCanBusPlugin : public QObject, public Q_CANBUS_FACTORY_INTERFACE
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QCanBusFactory" FILE "plugin.json")
	Q_INTERFACES(Q_CANBUS_FACTORY_INTERFACE)

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
