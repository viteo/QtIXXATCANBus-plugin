//////////////////////////////////////////////////////////////////////////
// IXXAT Automation GmbH
//////////////////////////////////////////////////////////////////////////
/**
  VCI driver and device GUIDs.

  @file "vci4105guid.h"
*/
//////////////////////////////////////////////////////////////////////////
// (C) 2002-2011 IXXAT Automation GmbH,all rights reserved
//////////////////////////////////////////////////////////////////////////

#ifndef VCI4105GUID_H
#define VCI4105GUID_H

//////////////////////////////////////////////////////////////////////////
//  include files
//////////////////////////////////////////////////////////////////////////
#include <stdtype.h>

//////////////////////////////////////////////////////////////////////////
//  constants and macros
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// data types
//////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * Driver and Device Classes from the iPC-IXC161/PCI driver (VCI105Wx.SYS)
 ****************************************************************************
 * DriverClass = {63D85058-8DD1-41D2-A3FF-2D25F3049152}
 * DeviceClass = {789CE2B8-814B-48A9-816A-C2E7C1E7D731}
 ****************************************************************************/
DEFINE_GUID(GUID_IPCIXC16PCI_DRIVER,
       0x63d85058,0x8dd1,0x41d2,0xa3,0xff,0x2d,0x25,0xf3,0x04,0x91,0x52);

DEFINE_GUID(GUID_IPCIXC16PCI_DEVICE,
       0x789ce2b8,0x814b,0x48a9,0x81,0x6a,0xc2,0xe7,0xc1,0xe7,0xd7,0x31);


#endif //VCI4105GUID_H
