# QT CAN Bus Plugin for IXXAT USB adapters

Here is an implementation of a Qt plugin for the IXXAT USB adapters.
The plugin should be installed in the Qt **canbus** plugin folder then it will be
available in Qt applications.

(*any support and feedback are welcome!*)

## Requirements

- Windows *(developed for Windows, for linux use SocketCAN plugin)*
- Qt Creator *or* Visual Studio

## Build steps

1. Install [IXXAT VCI V4 Windows driver software](https://www.ixxat.com/technical-support/support/windows-driver-software)
2. Check Environment Variables were added:
 - `$(VciSDKDir)` - *{IXXAT-installation-dir}/sdk/vci*
 - `$(VciIDLDir)` - *{IXXAT-installation-dir}/sdk/idl*
3. Open project
 - QT) `*.pro` Configure project using **MSVC** kit
 - VS) `*.vcxproj`  Check project Configuration Properties -> General and select your Platform Toolset
4. Compile for Release (or Debug)
5. Copy `qtixxatcanbus.dll` (and `qtixxatcanbus.pdb` for Debug) to *{QT-installation-dir}/5.XX.X/msvc20XX_XX/plugins/canbus* folder

## Testing

Launch Qt *CAN Bus example* from Qt Creator examples or directly from folder *{QT-installation-dir}\Examples\Qt-5.XX.X\serialbus\can*

Tested with IXXAT USB-to-CAN v2 and USB-to-CAN v2 Pro

## Features

- Configurable bitrate (`ConfigurationKey::BitRateKey`)
- Receive Own flag for messages (`ConfigurationKey::ReceiveOwnKey`)
- Remote Request messages (`QCanBusFrame::RemoteRequestFrame`)
- CAN Filter support (`ConfigurationKey::RawFilterKey`)
- Async message receiving (no polling thread in background)

## TODO 

- ~~Loopback feature~~ (not implemented in hardware)
- CAN FD support
- Extended payload support
- Could be useful to have a deploy target to copy dll and pdb files automatically in Qt plugin folder
- Tests

## Documentation

QT manual and some google search

- [Implementing a Custom CAN Plugin](https://doc.qt.io/qt-5/qtcanbus-backends.html)
